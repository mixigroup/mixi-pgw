/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       lookup_tpl.hh
    @brief      template source(lookup)
*******************************************************************************
   \n
   \n
*******************************************************************************
    @date       created(30/oct/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 30/oct/2017 
      -# Initial Version
******************************************************************************/
#ifndef __MIXIPGW_LOOKUP_TPL_HH
#define __MIXIPGW_LOOKUP_TPL_HH

#include "lookup_tpl.hpp"

#ifndef STAT_ON
# define STAT_ON        (0)
#endif
#ifndef STAT_OFF
# define STAT_OFF       (1)
#endif

namespace MIXIPGW{
  static pthread_mutex_t init_mtx_;

/**
   lookup : generate table instance\n
   *******************************************************************************
   instanciate function - static\n
   *******************************************************************************
   @return Lookup<T,S,K>* instance address
 */
  template<typename T,int S,typename K> Lookup<T,S,K>* Lookup<T,S,K>::Create(void){
      return(new Lookup<T,S,K>());
  }
/**
   lookup\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
  template<typename T,int S,typename K> Lookup<T,S,K>::~Lookup(){
      for(int n = 0;n < S;n++){
          free(data_side_a_[n]);
          free(data_side_b_[n]);
      }
      free(end_);
  }
/**
   lookup \n
   *******************************************************************************
   default constructor\n
   *******************************************************************************
 */
  template<typename T,int S,typename K> Lookup<T,S,K>::Lookup(){
      // instanciate.
      /*
       * [important]
       * initialization process must be called on the main thread.
       */
      if ((end_ = (T*)malloc(sizeof(T))) == NULL){
          throw "malloc for link_ptr(end)";
      }
      for(int n = 0;n < S;n++){
          if ((data_side_a_[n] = (T*)malloc(sizeof(T))) == NULL ||
              (data_side_b_[n] = (T*)malloc(sizeof(T))) == NULL){
              throw "malloc for link_ptr(side_[a/b])";
          }
      }
      Clear();
  }
/**
   +1 to virtual clock\n
   *******************************************************************************
   \n
   *******************************************************************************
 */
  template<typename T,int S,typename K> void Lookup<T,S,K>::Clock(void){
      virtual_clock_counter_++;
  }
/**
   initialize \n
   *******************************************************************************
   \n
   *******************************************************************************
 */
  template<typename T,int S,typename K> void Lookup<T,S,K>::Clear(){
      int n;
      memset(data_side_bmp_,0,sizeof(data_side_bmp_));
      for(n = 0;n < S;n++){
          memset(data_side_a_[n], 0, sizeof(*data_side_a_[n]));
          memset(data_side_b_[n], 0, sizeof(*data_side_b_[n]));
          data_side_a_[n]->stat.valid = STAT_OFF;
          data_side_b_[n]->stat.valid = STAT_OFF;
      }
      memset(end_,  0, sizeof(*end_));
      virtual_clock_counter_ = 0;
      virtual_clock_counter_prv_ = 0;
  }
  /**
   find by\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     gcnt find key( := index )
   @param[in]     flag 0!= back-side + include inactive/ 0== front-side + not includijng inactive
   @return RETCD  0==success,0!=error
 */
  template<typename T,int S,typename K> typename Lookup<T,S,K>::Iterator Lookup<T,S,K>::Find(const K gcnt, const int flag){
      /*
       * find process
       * 1. determine side of valid
       * 2. return valid side data
       */
      if (gcnt > (S-1)){
          return(End());
      }
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      if (flag){
          // back side is always return.
          if (!(data_side_bmp_[(gcnt>>6)]&bits)){
              return(Iterator(data_side_a_[gcnt]));
          }else{
              return(Iterator(data_side_b_[gcnt]));
          }
      }else{
          if ((data_side_bmp_[(gcnt>>6)]&bits)){
              if (data_side_a_[gcnt]->stat.valid==STAT_ON){
                  return(Iterator(data_side_a_[gcnt]));
              }
          }else{
              if (data_side_b_[gcnt]->stat.valid==STAT_ON){
                  return(Iterator(data_side_b_[gcnt]));
              }
          }
      }
      return(End());
  }
  /**
   termination iterator\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return LookupTable<T,S,K>::Iterator iterator
  */
  template<typename T,int S,typename K> typename Lookup<T,S,K>::Iterator Lookup<T,S,K>::End(){ return(Lookup<T,S,K>::Iterator(end_)); }
  template<typename T,int S,typename K> T& Lookup<T,S,K>::operator[](const K gcnt){
      if (gcnt > (S-1)){
          throw "invalid index";
      }
      Iterator itr = Find(gcnt);
      if (itr != End()){
          itr->stat.valid = STAT_ON;
          return((*itr));
      }
      itr = Find(gcnt, 1);
      if (itr == End()){
          throw "exception operator[]";
      }
      itr->stat.valid = STAT_ON;
      SwapSide(gcnt);
      return((*itr));
  }
  template<typename T,int S,typename K> void Lookup<T,S,K>::SwapSide(const K gcnt){
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      data_side_bmp_[(gcnt>>6)]^=bits;
  }
  template <typename T,int S,typename K> void Lookup<T,S,K>::EnumerateForUpdate(std::function<void(T*,void*)> func, void* udata){
      K gcnt;
      //
      for(gcnt = 1; gcnt < S;gcnt++){
          uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
          // return at back side.
          if (data_side_bmp_[(gcnt>>6)]&bits){
              *(data_side_b_[gcnt]) = *(data_side_a_[gcnt]);
              func(data_side_b_[gcnt],udata);
              data_side_b_[gcnt]->stat.valid  = STAT_ON;
          }else{
              *(data_side_a_[gcnt]) = *(data_side_b_[gcnt]);
              func(data_side_a_[gcnt],udata);
              data_side_a_[gcnt]->stat.valid  = STAT_ON;
          }
      }
      // turn over bits in batch in finally
      for(gcnt = 0; gcnt < (S>>6);gcnt++){
          data_side_bmp_[gcnt]^=((uint64_t)-1);
      }
  }
  template<typename T,int S,typename K>int Lookup<T,S,K>::Add(K gcnt, T* itm,int nochk_counter){
      Lookup<T,S,K>::Iterator itr = NULL;
      // read phase
      if (nochk_counter){
          if (virtual_clock_counter_ <= virtual_clock_counter_prv_){
              return(-1);
          }
          virtual_clock_counter_prv_ = virtual_clock_counter_;
      }
      itm->stat.valid = STAT_ON;

      // access back side.
      if ((itr = Find(gcnt, 1)) == End()){
          throw "exception not found(add_link).";
      }
      // update and swap.
      memcpy(&(*itr), itm, sizeof(*itm));
      SwapSide(gcnt);
      //
      return(0);
  }
  template<typename T,int S,typename K>int Lookup<T,S,K>::Del(K gcnt, int nochk_counter){
      Lookup<T,S,K>::Iterator itr = NULL;
      if (nochk_counter){
          // read phase
          if (virtual_clock_counter_ <= virtual_clock_counter_prv_){
              return(-1);
          }
          virtual_clock_counter_prv_ = virtual_clock_counter_;
      }
      // access back side.
      if ((itr = Find(gcnt, 1)) == End()){
          throw "exception not found(add_link).";
      }
      // update and swap.
      itr->stat.valid = STAT_OFF;
      SwapSide(gcnt);
      //
      return(0);
  }
};// namespace MIXIPGW

#endif // __MIXIPGW_LOOKUP_TPL_HH
