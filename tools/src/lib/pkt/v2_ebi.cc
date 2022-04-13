//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"

//
namespace MIXIPGW_TOOLS{
  Ebi::Ebi(){
      memset(&ebi_,0,sizeof(ebi_));
      set(NULL,0);
  }
  Ebi::Ebi(void* src,size_t len){
      attach(src,len);
  }
  Ebi::Ebi(uint8_t inst, void* src,size_t len){
      memset(&ebi_,0,sizeof(ebi_));
      ebi_.head.inst.instance = inst;
      set(src,len);
  }
  Ebi::Ebi(uint8_t inst,uint8_t ebi){
      memset(&ebi_,0,sizeof(ebi_));
      set(NULL,0);
      ebi_.head.inst.instance = inst;
      ebi_.ebi.low = ebi;
  }
  Ebi::~Ebi(){ }
  void Ebi::set(void* ebi, size_t len){
      if (ebi != NULL && len == 1){
          memcpy(&ebi_.ebi, ebi, 1);
      }
      ebi_.head.type = type();
      ebi_.head.length = htons(1);
  }
  int Ebi::type(void) const{
      return(GTPC_TYPE_EBI);
  }
  void* Ebi::ref(void) const{
      return((void*)&ebi_);
  }
  size_t Ebi::len(void) const{
      return(sizeof(gtpc_ebi_t));
  }
  int Ebi::attach(void* src,size_t len){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Ebi::attach. (%d)..%d", len, sizeof(gtpc_ebi_t));
      }
      gtpc_ebi_ptr p = (gtpc_ebi_ptr)src;
      if (len != sizeof(gtpc_ebi_t)){
          Logger::LOGERR("Ebi::attach failed.(%d, %d)", len, sizeof(gtpc_ebi_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == 1){
          memcpy(&ebi_, p, sizeof(gtpc_ebi_t));
          return(RETOK);
      }
      Logger::LOGERR("Ebi::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS