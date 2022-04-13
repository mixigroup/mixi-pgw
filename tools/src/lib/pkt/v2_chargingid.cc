//
// Created by mixi on 2017/01/16.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"

//
namespace MIXIPGW_TOOLS{
  ChargingId::ChargingId(){
      memset(&chargingid_,0,sizeof(chargingid_));
      set(0);
  }
  ChargingId::ChargingId(void* src,size_t len){
      attach(src,len);
  }
  ChargingId::ChargingId(uint32_t charging_id){
      memset(&chargingid_,0,sizeof(chargingid_));
      set(charging_id);
  }
  ChargingId::~ChargingId(){ }
  void ChargingId::set(uint32_t charging_id){
      chargingid_.charging_id = charging_id;
      chargingid_.head.type = type();
      chargingid_.head.length = htons(4);
  }
  int ChargingId::type(void) const{
      return(GTPC_TYPE_CHARGING_ID);
  }
  void* ChargingId::ref(void) const{
      return((void*)&chargingid_);
  }
  size_t ChargingId::len(void) const{
      return(sizeof(gtpc_charging_id_t));
  }
  int ChargingId::attach(void* src,size_t len){
      gtpc_charging_id_ptr p = (gtpc_charging_id_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("ChargingId::attach. (%d)..%d", len, sizeof(gtpc_charging_id_t));
      }
      if (len != 8){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == 4){
          memcpy(&chargingid_, p, len);
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS