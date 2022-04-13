//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Rat::Rat(){
      memset(&rat_,0,sizeof(rat_));
      set(0);
  }
  Rat::Rat(void* src,size_t len){
      attach(src,len);
  }
  Rat::Rat(uint8_t rat){
      memset(&rat_,0,sizeof(rat_));
      set(rat);
  }
  Rat::~Rat(){ }
  void Rat::set(uint8_t rat){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Rat::set(%p, %x)", this, rat);
      }
      rat_.rat_type = rat;
      rat_.head.type = type();
      rat_.head.length = htons(1);
  }
  int Rat::type(void) const{
      return(GTPC_TYPE_RAT_TYPE);
  }
  void* Rat::ref(void) const{
      return((void*)&rat_);
  }
  size_t Rat::len(void) const{
      return(sizeof(gtpc_rat_t));
  }
  int Rat::attach(void* src,size_t len){
      gtpc_rat_ptr p = (gtpc_rat_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Rat::attach. (%d)..%d", len, sizeof(gtpc_rat_t));
      }
      if (len != sizeof(gtpc_rat_t)){
          return(RETERR);
      }
      if (p->head.type == type() && p->head.length == htons(1)){
          memcpy(&rat_, p, sizeof(gtpc_rat_t));
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS