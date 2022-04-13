/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       lookup_tpl.hpp
    @brief      lookup
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
#ifndef __MIXIPGW_LOOKUP_TPL_HPP
#define __MIXIPGW_LOOKUP_TPL_HPP

#include "mixi_pgw_data_plane_def.hpp"
#include <pthread.h>
#include <functional>

namespace MIXIPGW{

  /** *****************************************************
   * @brief
   * Lookup template\n
   * \n
   */
  template<typename T,int S,typename K>
  class Lookup{
  public:
      class Iterator{
      public:
          Iterator():ptr_(NULL){}
          Iterator(T* ptr): ptr_(ptr){}
          T&  operator*() { return(*ptr_); }
          T*  operator->() { return(ptr_); }
          bool operator==(const Iterator cp) { return(ptr_==cp.ptr_);}
          bool operator!=(const Iterator cp) { return(ptr_!=cp.ptr_);}
      private:
          T* ptr_;
      };
  public:
      static Lookup<T,S,K>* Create(void);
      virtual ~Lookup();
  private:
      Lookup();
  public:
      int  Add(const K,T*, int);
      int  Del(const K,int);
      void Clock(void);
      void Clear();
      //
      Iterator Find(const K key, const int flag = 0);
      Iterator End();
      T& operator[](const K key);
      void SwapSide(const K key);
      void EnumerateForUpdate(std::function<void(T*,void*)>,void*);
  private:
      T*      data_side_a_[S];
      T*      data_side_b_[S];
      T*      end_;
      uint64_t  data_side_bmp_[S>>6];
      uint64_t  virtual_clock_counter_;
      uint64_t  virtual_clock_counter_prv_;
  };
};// namespace MIXIPGW

#include "lookup_tpl.hh"

#endif // __MIXIPGW_LOOKUP_TPL_HPP

