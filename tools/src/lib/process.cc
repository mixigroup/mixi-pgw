//
// Created by mixi on 2017/04/21.
//
#include "mixipgw_tools_def.h"
#include "lib/process.hpp"
#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  ProcessParameter::ProcessParameter():ProcessParameter(NULL){ }
  ProcessParameter::ProcessParameter(FilterContainer* filters):filters_(filters){
      bzero(str_buf_, sizeof(str_buf_));
      bzero(ull_buf_, sizeof(ull_buf_));
      bzero(ush_buf_, sizeof(ush_buf_));
      bzero(usg_buf_, sizeof(usg_buf_));
      bzero(lookup_tbl_,sizeof(lookup_tbl_));
  }
  ProcessParameter::~ProcessParameter(){}
  //
  void ProcessParameter::Set(_TXT_ key,const char* val){
      bzero(str_buf_[key],sizeof(str_buf_[0]));
      memcpy(str_buf_[key], val, MIN(strlen(val), sizeof(str_buf_[0])-1));
  }
  void ProcessParameter::Set(_USG_ key,const unsigned val){ usg_buf_[key] = val; }
  void ProcessParameter::Set(_USH_ key,const unsigned short val){ ush_buf_[key] = val; }
  void ProcessParameter::Set(_ULL_ key,const unsigned long long val){ ull_buf_[key] = val; }
  void ProcessParameter::Set(_TBL_ key,const unsigned skey, void* val){
      if ((signed)skey >= 0 && skey < TBL_SKEY_MAX){
          lookup_tbl_[key][skey] = val;
      }
  }
  void ProcessParameter::Set(FilterContainer* filters){ filters_ = filters; }
  //
  const char* ProcessParameter::Get(_TXT_ key){ return(str_buf_[key]); }
  const unsigned ProcessParameter::Get(_USG_ key){ return(usg_buf_[key]); }
  const unsigned short ProcessParameter::Get(_USH_ key){ return(ush_buf_[key]); }
  const unsigned long long ProcessParameter::Get(_ULL_ key){ return(ull_buf_[key]); }
  void* ProcessParameter::Get(_TBL_ key,unsigned skey){
      if ((signed)skey >= 0 && skey < TBL_SKEY_MAX){
          return(lookup_tbl_[key][skey]);
      }
      return(NULL);
  }
  FilterContainer* ProcessParameter::Get(void) { return(filters_); }
  //
  void ProcessParameter::Copy(_TXT_ dst,_TXT_ src){
      if (dst != src){  memcpy(str_buf_[dst], str_buf_[src], sizeof(str_buf_[0])); }
  }
  void ProcessParameter::Copy(_USG_ dst,_USG_ src){
      if (dst != src) { usg_buf_[dst] = usg_buf_[src]; }
  }
  void ProcessParameter::Copy(_USH_ dst,_USH_ src){
      if (dst != src) { ush_buf_[dst] = usg_buf_[src]; }
  }
  void ProcessParameter::Copy(_ULL_ dst,_ULL_ src){
      if (dst != src) { ull_buf_[dst] = ull_buf_[src]; }
  }
}; // namespace MIXIPGW_TOOLS

