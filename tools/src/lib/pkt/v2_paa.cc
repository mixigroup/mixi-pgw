//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/packet/gtpc_items.hpp"

//
namespace MIXIPGW_TOOLS{
  Paa::Paa(){
      memset(&paa_,0,sizeof(paa_));
      set(0,NULL,0);
  }
  Paa::Paa(void* src,size_t len){
      attach(src,len);
  }
  Paa::Paa(uint8_t flags,void* paa,size_t len){
      memset(&paa_,0,sizeof(paa_));
      set(flags, paa, len);
  }
  Paa::~Paa(){ }
  void Paa::set(uint8_t flags, void* paa, size_t len){
      paa_.c.flags = flags;
      if (paa != NULL && len > 0 && len < sizeof(paa_.paa)){
          memcpy(paa_.paa, paa, len);
          paa_.head.length = htons(len+1);
      }else{
          paa_.head.length = htons(0);
      }
      paa_.head.type = type();
  }
  int Paa::type(void) const{
      return(GTPC_TYPE_PAA);
  }
  void* Paa::ref(void) const{
      return((void*)&paa_);
  }
  size_t Paa::len(void) const{
      return(ntohs(paa_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Paa::attach(void* src,size_t len){
      gtpc_paa_ptr p = (gtpc_paa_ptr)src;
      if (len <= sizeof(gtpc_comm_header_t)){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) > 0 && ntohs(p->head.length) < sizeof(paa_.paa)){
          memcpy(&paa_, p, ntohs(p->head.length) + sizeof(gtpc_comm_header_t));
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS