//
// Created by mixi on 2017/01/12.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Ambr::Ambr(){
      memset(&ambr_,0,sizeof(ambr_));
      set(0,0);
  }
  Ambr::Ambr(void* src,size_t len){
      attach(src,len);
  }
  Ambr::Ambr(uint32_t uplink,uint32_t downlink){
      memset(&ambr_,0,sizeof(ambr_));
      set(uplink, downlink);
  }
  Ambr::~Ambr(){ }
  void Ambr::set(uint32_t uplink, uint32_t downlink){
      ambr_.uplink = htonl(uplink);
      ambr_.downlink = htonl(downlink);
      ambr_.head.type = type();
      ambr_.head.length = htons(8);
  }
  uint32_t Ambr::uplink(void){
      return(ntohl(ambr_.uplink));
  }
  uint32_t Ambr::downlink(void){
      return(ntohl(ambr_.downlink));
  }
  int Ambr::type(void) const{
      return(GTPC_TYPE_AMBR);
  }
  void* Ambr::ref(void) const{
      return((void*)&ambr_);
  }
  size_t Ambr::len(void) const{
      return(sizeof(gtpc_ambr_t));
  }
  int Ambr::attach(void* src,size_t len){
      gtpc_ambr_ptr p = (gtpc_ambr_ptr)src;
      if (len != sizeof(gtpc_ambr_t)){
          Logger::LOGERR("Ambr::attach failed.(%d, %d)", len, sizeof(gtpc_ambr_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == 8){
          memcpy(&ambr_, p, sizeof(gtpc_ambr_t));
          return(RETOK);
      }
      Logger::LOGERR("Ambr::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS