//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Indication::Indication(){
      memset(&indication_,0,sizeof(indication_));
      set(NULL,0);
  }
  Indication::Indication(void* src,size_t len){
      attach(src,len);
  }
  Indication::Indication(uint32_t flag){
      memset(&indication_,0,sizeof(indication_));
      set(&flag,sizeof(flag));
  }
  Indication::~Indication(){ }
  void Indication::set(void* indication, size_t len){
      if (indication != NULL && len <= (sizeof(gtpc_indication_flags_t) - sizeof(gtpc_comm_header_t))){
          memcpy(&indication_.c.flags, indication, len);
      }
      indication_.head.type = type();
      indication_.head.length = htons(len);   // dg 10-1-1-70. 6 octets.
  }
  int Indication::type(void) const{
      return(GTPC_TYPE_INDICATION);
  }
  void* Indication::ref(void) const{
      return((void*)&indication_);
  }
  size_t Indication::len(void) const{
      return(ntohs(indication_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Indication::attach(void* src,size_t len){
//    Logger::LOGINF("Indication::attach. (%d)..%d", len, sizeof(gtpc_indication_flags_t));
      gtpc_indication_flags_ptr p = (gtpc_indication_flags_ptr)src;
      if (len > sizeof(gtpc_indication_flags_t)){
          Logger::LOGERR("Indication::attach failed.(%d, %d)", len, sizeof(gtpc_indication_flags_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) > 0 && ntohs(p->head.length) < (sizeof(indication_) - sizeof(gtpc_comm_header_t))){
          memcpy(&indication_, p, ntohs(p->head.length) + sizeof(gtpc_comm_header_t));
          if (Module::VERBOSE() > PCAPLEVEL_PARSE){
//            Logger::LOGINF("Indication::attach. success.");
          }
          return(RETOK);
      }
      Logger::LOGERR("Indication::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS
