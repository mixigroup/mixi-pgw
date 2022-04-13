//
// Created by mixi on 2017/01/16.
//

#include "mixipgw_tools_def.h"
#include "lib/packet/gtpc_items.hpp"

namespace MIXIPGW_TOOLS{
  ApnRestriction::ApnRestriction(){
      memset(&apn_,0,sizeof(apn_));
      set(0);
  }
  ApnRestriction::ApnRestriction(void* src,size_t len){
      attach(src,len);
  }
  ApnRestriction::ApnRestriction(uint8_t flag){
      memset(&apn_,0,sizeof(apn_));
      set(flag);
  }
  ApnRestriction::~ApnRestriction(){ }
  void ApnRestriction::set(uint8_t apn_restriction_type){
      apn_.head.type = type();
      apn_.head.length = htons(1);
      apn_.restriction_type = apn_restriction_type;
  }
  int ApnRestriction::type(void) const{
      return(GTPC_TYPE_APN_RESTRICTION);
  }
  void* ApnRestriction::ref(void) const{
      return((void*)&apn_);
  }
  size_t ApnRestriction::len(void) const{
      return(sizeof(gtpc_apn_restriction_t));
  }
  int ApnRestriction::attach(void* src,size_t len){
      gtpc_apn_restriction_ptr p = (gtpc_apn_restriction_ptr)src;
      if (len != 5){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == 1){
          memcpy(&apn_, p, len);
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS