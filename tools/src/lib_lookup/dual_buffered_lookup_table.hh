//
// Created by mixi on 2017/04/18.
//

#ifndef DATAPLANE_LINK_HH
#define DATAPLANE_LINK_HH
#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/module.hpp"

namespace MIXIPGW_TOOLS{
  extern pthread_mutex_t extern_mtx;

  template<typename T> DualBufferedLookupTable<T>* DualBufferedLookupTable<T>::Create(void){
      return(new DualBufferedLookupTable<T>());
  }
  template<typename T> DualBufferedLookupTable<T>* DualBufferedLookupTable<T>::Init(void){
      static DualBufferedLookupTable<T>* linktbl = NULL;
      pthread_mutex_lock(&extern_mtx);
      if (!linktbl){
          linktbl = DualBufferedLookupTable<T>::Create();
      }
      pthread_mutex_unlock(&extern_mtx);
      return(linktbl);
  }
  template<typename T> DualBufferedLookupTable<T>::~DualBufferedLookupTable(){
      for(int n = 0;n < MAX_STRAGE_PER_GROUP;n++){
          free(data_side_a_[n]);
          free(data_side_b_[n]);
      }
      free(end_);
  }
  template<typename T> DualBufferedLookupTable<T>::DualBufferedLookupTable(){
      // instanciate.
      /*
       * [important]
       * initialization process must be called on the main thread.
       */
      if ((end_ = (T*)malloc(sizeof(T))) == NULL){
          throw "malloc for link_ptr(end)";
      }
      for(int n = 0;n < MAX_STRAGE_PER_GROUP;n++){
          if ((data_side_a_[n] = (T*)malloc(sizeof(T))) == NULL ||
              (data_side_b_[n] = (T*)malloc(sizeof(T))) == NULL){
              throw "malloc for link_ptr(side_[a/b])";
          }
      }
      Clear();
  }
  template<typename T> void DualBufferedLookupTable<T>::Clock(void){
      virtual_clock_counter++;
      // fprintf(stderr, "VCLOCK[%p]..vcc :" FMT_LLU "/vpc :" FMT_LLU "\n",(void*)this, virtual_clock_counter, virtual_clock_counter_prv);
  }
  template<typename T> void DualBufferedLookupTable<T>::Clear(){
      int n;
      memset(data_side_bmp_,0,sizeof(data_side_bmp_));
      memset(data_notice_bmp_,0,sizeof(data_notice_bmp_));
      for(n = 0;n < MAX_STRAGE_PER_GROUP;n++){
          memset(data_side_a_[n], 0, sizeof(*data_side_a_[n]));
          memset(data_side_b_[n], 0, sizeof(*data_side_b_[n]));
      }
      memset(end_,  0, sizeof(*end_));
      virtual_clock_counter = 0;
      virtual_clock_counter_prv = 0;
  }
  template<typename T> void DualBufferedLookupTable<T>::ClearNoticeBmp64(const __be16 gcnt){
      if (gcnt <= MAX_STRAGE_PER_GROUP){
          data_notice_bmp_[gcnt>>6] = 0;
      }
  }
  template<typename T> typename DualBufferedLookupTable<T>::Iterator DualBufferedLookupTable<T>::Find(const __be32 key, const int flag){
      /*
       * find process
       * 1. determine side of valid
       * 2. return valid side data
       */
      union _teidcnv cnv;
      cnv.pgw_teid_uc = key;
      __be16 gcnt = cnv.pgw_teid.gcnt;

      if (gcnt > MAX_STRAGE_PER_GROUP){
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
              if (data_side_a_[gcnt]->stat.valid==LINKMAPSTAT_ON){
                  return(Iterator(data_side_a_[gcnt]));
              }
          }else{
              if (data_side_b_[gcnt]->stat.valid==LINKMAPSTAT_ON){
                  return(Iterator(data_side_b_[gcnt]));
              }
          }
      }
//    fprintf(stderr, "failed..stat.valid!=ON .find(%u - %u)\n", key , gcnt);
      return(End());
  }
  template<typename T> uint64_t DualBufferedLookupTable<T>::FindNoticeBmp64(const __be16 gcnt){
      if (gcnt > MAX_STRAGE_PER_GROUP){
           return(0);
      }
      return(data_notice_bmp_[(gcnt>>6)]);
  }

  template<typename T> typename DualBufferedLookupTable<T>::Iterator DualBufferedLookupTable<T>::End(){ return(DualBufferedLookupTable<T>::Iterator(end_)); }
  template<typename T> T& DualBufferedLookupTable<T>::operator[](const __be32 key){
      union _teidcnv cnv;
      cnv.pgw_teid_uc = key;
      __be16 gcnt = cnv.pgw_teid.gcnt;
      if (gcnt > MAX_STRAGE_PER_GROUP){
          fprintf(stderr, "capability over ,link operator[] %u - %u\n", gcnt, MAX_STRAGE_PER_GROUP);
          throw "invalid index";
      }
      Iterator itr = Find(key);
      if (itr != End()){
          itr->stat.valid = LINKMAPSTAT_ON;
          return((*itr));
      }
      itr = Find(key, 1);
      if (itr == End()){
          throw "exception operator[]";
      }
      itr->stat.valid = LINKMAPSTAT_ON;
      SwapSide(key);
      return((*itr));
  }
  template<typename T> void DualBufferedLookupTable<T>::SwapSide(const __be32 key){
      union _teidcnv cnv;
      cnv.pgw_teid_uc = key;
      __be16 gcnt = cnv.pgw_teid.gcnt;
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      data_side_bmp_[(gcnt>>6)]^=bits;
  }
  template<typename T> void DualBufferedLookupTable<T>::NotifyChange(const __be32 key){
      union _teidcnv cnv;
      cnv.pgw_teid_uc = key;
      __be16 gcnt = cnv.pgw_teid.gcnt;
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      data_notice_bmp_[(gcnt>>6)]^=bits;
  }
  template <typename T> void DualBufferedLookupTable<T>::EnumerateForUpdate(std::function<void(T*,void*)> func, void* udata){
      __be32 gcnt;
      //
      for(gcnt = 1; gcnt < MAX_STRAGE_PER_GROUP;gcnt++){
          uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
          // return at back side.
          if (data_side_bmp_[(gcnt>>6)]&bits){
              *(data_side_b_[gcnt]) = *(data_side_a_[gcnt]);
              func(data_side_b_[gcnt],udata);
              data_side_b_[gcnt]->stat.valid  = LINKMAPSTAT_ON;
          }else{
              *(data_side_a_[gcnt]) = *(data_side_b_[gcnt]);
              func(data_side_a_[gcnt],udata);
              data_side_a_[gcnt]->stat.valid  = LINKMAPSTAT_ON;
          }
      }
      // swap status bits in bulk.
      for(gcnt = 0; gcnt < (MAX_STRAGE_PER_GROUP>>6);gcnt++){
          data_side_bmp_[gcnt]^=((uint64_t)-1);
      }
  }
  template<typename T>int DualBufferedLookupTable<T>::Add(__be32 key, T* itm,int nochk_counter){
      DualBufferedLookupTable<T>::Iterator itr = NULL;
      // read phase
      if (nochk_counter){
          if (virtual_clock_counter <= virtual_clock_counter_prv){
              fprintf(stderr, "failed[%p]..add(%u) vcc :" FMT_LLU "/vpc :" FMT_LLU "\n",(void*)this, key , virtual_clock_counter, virtual_clock_counter_prv);
              return(RETERR);
          }
          virtual_clock_counter_prv = virtual_clock_counter;
      }
      itm->stat.valid = LINKMAPSTAT_ON;

      // access back side.
      if ((itr = Find(key, 1)) == End()){
          throw "exception not found(add_link).";
      }
      // update and swap.
      memcpy(&(*itr), itm, sizeof(*itm));
      SwapSide(key);
      NotifyChange(key);
      //
      return(RETOK);
  }
  template<typename T>int DualBufferedLookupTable<T>::Del(__be32 key, int nochk_counter){
      DualBufferedLookupTable<T>::Iterator itr = NULL;
      if (nochk_counter){
          // read phase
          if (virtual_clock_counter <= virtual_clock_counter_prv){
              fprintf(stderr, "failed[%p]..delete(%u) vcc :" FMT_LLU "/vpc :" FMT_LLU "\n",(void*)this, key , virtual_clock_counter, virtual_clock_counter_prv);
              return(RETERR);
          }
          virtual_clock_counter_prv = virtual_clock_counter;
      }
      // access back side.
      if ((itr = Find(key, 1)) == End()){
          throw "exception not found(add_link).";
      }
      // update and swap.
      itr->stat.valid = LINKMAPSTAT_OFF;
      SwapSide(key);
      NotifyChange(key);
      //
      return(RETOK);
  }

}; // namespace MIXIPGW_TOOLS

#endif //DATAPLANE_LINK_HH
