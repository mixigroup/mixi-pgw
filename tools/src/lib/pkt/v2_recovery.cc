//
// Created by mixi on 2017/01/10.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Recovery::Recovery(){
      memset(&recovery_,0,sizeof(recovery_));
      set(0);
  }
  Recovery::Recovery(void* src,size_t len){
      attach(src,len);
  }
  Recovery::Recovery(uint8_t cnt){
      memset(&recovery_,0,sizeof(recovery_));
      set(cnt);
  }
  Recovery::~Recovery(){ }
  void Recovery::set(uint8_t cnt){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Recovery::set(%x)", cnt);
      }
      recovery_.head.type = type();
      recovery_.head.length = htons(1);
      recovery_.recovery_restart_counter = cnt;
  }
  int Recovery::type(void) const{
      return(GTPC_TYPE_RECOVERY);
  }
  void* Recovery::ref(void) const{
      return((void*)&recovery_);
  }
  size_t Recovery::len(void) const{
      return(sizeof(gtpc_recovery_t));
  }
  int Recovery::attach(void* src,size_t len){
      gtpc_recovery_ptr p = (gtpc_recovery_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Recovery::attach. (%d)..%d", len, sizeof(gtpc_recovery_t));
      }
      if (len != sizeof(gtpc_recovery_t)){
          return(RETERR);
      }
      if (p->head.type == type() && p->head.length == htons(1)){
          memcpy(&recovery_, p, sizeof(gtpc_recovery_t));
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS