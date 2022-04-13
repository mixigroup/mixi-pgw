//
// Created by mixi on 2017/01/16.
//

#include "mixipgw_tools_def.h"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Any::Any():length_(0){
      memset(&payload_,0,sizeof(payload_));
  }
  Any::Any(void* src,size_t len):length_(len){
      attach(src,len);
  }
  Any::~Any(){ }
  int Any::type(void) const{
      return(0);
  }
  void* Any::ref(void) const{
      return((void*)payload_);
  }
  size_t Any::len(void) const{
      return(length_);
  }
  int Any::attach(void* src,size_t len){
      length_ = MIN(sizeof(payload_),len);
      memcpy(&payload_, src, length_);
      return(RETOK);
  }
}; // namespace MIXIPGW_TOOLS