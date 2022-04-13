//
// Created by mixi on 2017/01/16.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Pdn::Pdn(){
      memset(&pdn_,0,sizeof(pdn_));
      set(0);
  }
  Pdn::Pdn(void* src,size_t len){
      attach(src,len);
  }
  Pdn::Pdn(uint8_t pdn_type){
      memset(&pdn_,0,sizeof(pdn_));
      set(pdn_type);
  }
  Pdn::~Pdn(){ }
  void Pdn::set(uint8_t pdn_type){
      pdn_.head.type = type();
      pdn_.head.length = htons(1);
      pdn_.c.bit.pdn_type = pdn_type;
  }
  int Pdn::type(void) const{
      return(GTPC_TYPE_PDN_TYPE);
  }
  void* Pdn::ref(void) const{
      return((void*)&pdn_);
  }
  size_t Pdn::len(void) const{
      return(sizeof(gtpc_pdn_type_t));
  }
  int Pdn::attach(void* src,size_t len){
      gtpc_pdn_type_ptr p = (gtpc_pdn_type_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Pdn::attach. (%d)..%d", len, sizeof(gtpc_pdn_type_t));
      }
      if (len != 5){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == 1){
          memcpy(&pdn_, p, len);
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS