//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  ServingNetwork::ServingNetwork(){
      memset(&servingnetwork_,0,sizeof(servingnetwork_));
      set(NULL,0);
  }
  ServingNetwork::ServingNetwork(void* src,size_t len){
      attach(src,len);
  }
  ServingNetwork::ServingNetwork(gtpc_numberdigit_ptr pd){
      memset(&servingnetwork_,0,sizeof(servingnetwork_));
      set(pd,sizeof(servingnetwork_.digits));
  }
  ServingNetwork::~ServingNetwork(){ }
  void ServingNetwork::set(void* servingnetwork,size_t len){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("ServingNetwork::set(%p, %x)", servingnetwork, len);
      }
      if (sizeof(servingnetwork_.digits) == len){
          memcpy(servingnetwork_.digits, servingnetwork, len);
      }
      servingnetwork_.head.type = type();
      servingnetwork_.head.length = htons(3);
  }
  int ServingNetwork::type(void) const{
      return(GTPC_TYPE_SERVING_NETWORK);
  }
  void* ServingNetwork::ref(void) const{
      return((void*)&servingnetwork_);
  }
  size_t ServingNetwork::len(void) const{
      return(sizeof(gtpc_serving_network_t));
  }
  int ServingNetwork::attach(void* src,size_t len){
      gtpc_serving_network_ptr p = (gtpc_serving_network_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("ServingNetwork::attach. (%d)..%d", len, sizeof(gtpc_serving_network_t));
      }
      if (len != sizeof(gtpc_serving_network_t)){
          Logger::LOGERR("ServingNetwork::attach failed.(%d, %d)", len, sizeof(gtpc_serving_network_t));
          GtpcPkt::dump_impl("ServingNetwork::attach failed.", src, len);
          return(RETERR);
      }
      if (p->head.type == type() && p->head.length == htons(3)){
          memcpy(&servingnetwork_, p, sizeof(servingnetwork_));
          return(RETOK);
      }
      Logger::LOGERR("ServingNetwork::attach... failed.(%d, %d)", p->head.length, p->head.type);
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS