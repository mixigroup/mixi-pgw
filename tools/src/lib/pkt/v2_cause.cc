//
// Created by mixi on 2017/01/12.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Cause::Cause(){
      uint16_t sn(0xffff);
      memset(&cause_,0,sizeof(cause_));
      set(&sn, 2);
  }
  Cause::Cause(void* src,size_t len){
      attach(src,len);
  }
  Cause::Cause(uint8_t cause){
      memset(&cause_,0,sizeof(cause_));
      cause_.cause = cause;
      cause_.head.length = htons(2);
      cause_.head.type = type();
  }

  Cause::~Cause(){ }
  void Cause::set(void* cause, size_t len){
      if (cause != NULL && (len == 2 || len == 6)){
          memcpy(&cause_.cause, cause, len);
          cause_.head.length = htons(len);
      }else{
          cause_.head.length = 0;
      }
      cause_.head.type = type();
  }
  int Cause::type(void) const{
      return(GTPC_TYPE_CAUSE);
  }
  void* Cause::ref(void) const{
      return((void*)&cause_);
  }
  size_t Cause::len(void) const{
      return(ntohs(cause_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Cause::attach(void* src,size_t len){
      gtpc_cause_ptr p = (gtpc_cause_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Cause::attach. (%d)..%d", len, sizeof(gtpc_cause_t));
      }
      if (len != 6 && len != 10){
          Logger::LOGERR("Cause::attach failed.(%d, 6 or 10)", len);
          return(RETERR);
      }
      if (p->head.type == type() && (ntohs(p->head.length) == 2 || ntohs(p->head.length) == 6 )){
          memcpy(&cause_, p, len);
          if (Module::VERBOSE() > PCAPLEVEL_DBG){
              Logger::LOGINF("success. Cause::attach(%d)", len);
          }
          return(RETOK);
      }
      Logger::LOGERR("Cause::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS