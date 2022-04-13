//
// Created by mixi on 2017/07/03.
//

#ifndef MIXIPGW_TOOLS_DUAL_BUFFERED_LOOKUP_NBIT_TPL_HPP
#define MIXIPGW_TOOLS_DUAL_BUFFERED_LOOKUP_NBIT_TPL_HPP

#include <pthread.h>
#include <functional>

namespace MIXIPGW_TOOLS{

  template<typename T,int S,typename K>
  class LookupTable{
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
      static LookupTable<T,S,K>* Create(void);
      static LookupTable<T,S,K>* Init(void);
      virtual ~LookupTable();
  private:
      LookupTable();
  public:
      int  Add(const K,T*, int);
      int  Del(const K,int);
      void Clock(void);
      void Clear();
      void ClearNoticeBmp64(const K);
      //
      Iterator Find(const K key, const int flag = 0);
      uint64_t FindNoticeBmp64(const K);
      Iterator End();
      T& operator[](const K key);
      void SwapSide(const K key);
      void NotifyChange(const K key);
      void EnumerateForUpdate(std::function<void(T*,void*)>,void*);
  private:
      T*      data_side_a_[S];
      T*      data_side_b_[S];
      T*      end_;
      uint64_t  data_side_bmp_[S>>6];
      uint64_t  data_notice_bmp_[S>>6];
      uint64_t  virtual_clock_counter_;
      uint64_t  virtual_clock_counter_prv_;
  };
}; // namespace MIXIPGW_TOOLS

#include "lookup_table_tpl.hh"





#endif //MIXIPGW_TOOLS_DUAL_BUFFERED_LOOKUP_NBIT_TPL_HPP
