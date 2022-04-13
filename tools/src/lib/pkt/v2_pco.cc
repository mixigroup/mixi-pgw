//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"

//
namespace MIXIPGW_TOOLS{
  Pco::Pco(){
      memset(&pco_,0,sizeof(pco_));
      set(NULL, 0);
  }
  Pco::Pco(void* src,size_t len){
      attach(src,len);
  }
  Pco::Pco(const char* hexstr){
      memset(&pco_,0,sizeof(pco_));
      pco_.head.length = htons(0);
      pco_.head.type = type();
      //
      if (hexstr!=NULL){
          if (strlen(hexstr)%2==0 && strlen(hexstr)/2 < sizeof(pco_.pco)){
              uint8_t tmphex[sizeof(pco_.pco)] = {0};
              size_t  tmphexlen = 0;
              for(int n=0; n < (strlen(hexstr)/2);n++,tmphexlen+=2){
                  sscanf(&hexstr[tmphexlen],"%2hhx",&tmphex[n]);
              }
              set(tmphex, (tmphexlen/2));
          }
      }
  }
  Pco::~Pco(){ }
  void Pco::set(void* pco, size_t len){
      if (pco != NULL && len > 0 && len < sizeof(pco_.pco)){
          memcpy(pco_.pco, pco, len);
          pco_.head.length = htons(len);
      }else{
          pco_.head.length = htons(0);
      }
      pco_.head.type = type();
  }
  int Pco::type(void) const{
      return(GTPC_TYPE_PCO);
  }
  void* Pco::ref(void) const{
      return((void*)&pco_);
  }
  size_t Pco::len(void) const{
      return(ntohs(pco_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Pco::attach(void* src,size_t len){
      gtpc_pco_ptr p = (gtpc_pco_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Pco::attach. (%d)..%d", len, sizeof(gtpc_pco_t));
      }
      if (len <= sizeof(gtpc_comm_header_t)){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) > 0 && ntohs(p->head.length) < sizeof(pco_.pco)){
          memcpy(&pco_, p, ntohs(p->head.length) + sizeof(gtpc_comm_header_t));
          if (Module::VERBOSE() > PCAPLEVEL_PARSE){
              Logger::LOGINF("Pco::attach. success.");
          }
          return(RETOK);
      }
      Logger::LOGERR("Pco::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      GtpcPkt::dump_impl("Pco::attach failed.", src, len);

      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS