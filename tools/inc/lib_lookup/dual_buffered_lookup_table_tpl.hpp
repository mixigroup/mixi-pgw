//
// Created by mixi on 2017/01/10.
//

#ifndef MIXIPGW_TOOLS_DUAL_BUFFERED_LOOKUP_TABLE_TPL_H
#define MIXIPGW_TOOLS_DUAL_BUFFERED_LOOKUP_TABLE_TPL_H

#include "mixipgw_tools_def.h"
#include <functional>
//
namespace MIXIPGW_TOOLS{

  union _teidcnv{
      struct _ue_ipv4{
          __be32 gcnt:16;
          __be32 gid:8;
          __be32 fix:8;
      }ue_ipv4;
      struct _pgw_teid{
          __be32 gcnt:16;
          __be32 gid:8;
          __be32 sid:8;
      }pgw_teid;
      __be32 pgw_teid_uc;
  };
  //
  template<typename T>
  class DualBufferedLookupTable{
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
      static DualBufferedLookupTable<T>* Create(void);
      static DualBufferedLookupTable<T>* Init(void);
      virtual ~DualBufferedLookupTable();
  private:
      DualBufferedLookupTable();
  public:
      int  Add(const __be32,T*, int);
      int  Del(const __be32,int);
      void Clock(void);
      void Clear();
      void ClearNoticeBmp64(const __be16);
      //
      Iterator Find(const __be32 key, const int flag = 0);
      uint64_t FindNoticeBmp64(const __be16);
      Iterator End();
      T& operator[](const __be32 key);
      void SwapSide(const __be32 key);
      void NotifyChange(const __be32 key);
      void EnumerateForUpdate(std::function<void(T*,void*)>,void*);
  private:
      T*      data_side_a_[MAX_STRAGE_PER_GROUP];
      T*      data_side_b_[MAX_STRAGE_PER_GROUP];
      T*      end_;
      uint64_t  data_side_bmp_[MAX_STRAGE_PER_GROUP>>6];
      uint64_t  data_notice_bmp_[MAX_STRAGE_PER_GROUP>>6];
      uint64_t  virtual_clock_counter;
      uint64_t  virtual_clock_counter_prv;
  };
}; // namespace MIXIPGW_TOOLS
#include "src/lib_lookup/dual_buffered_lookup_table.hh"

#endif //MIXIPGW_TOOLS_DUAL_BUFFERED_LOOKUP_TABLE_TPL_H
