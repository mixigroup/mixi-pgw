//
// Created by mixi on 2017/01/13.
//

/*
3GPP TS 29.274 V11.3.0
8.15 Bearer Quality of Service (Bearer QoS)

The UL/DL MBR and GBR fields are encoded as kilobits per second (1 kbps = 1000 bps) in binary value.
For non-GBR bearers, both the UL/DL MBR and GBR should be set to zero.
The range of QCI, Maximum bit rate for uplink, Maximum bit rate for downlink,
Guaranteed bit rate for uplink and Guaranteed bit rate for downlink are specified in 3GPP TS 36.413 [10].
 */


#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Bqos::Bqos(){
      memset(&bqos_,0,sizeof(bqos_));
      set(0,0,0,0,0,0);
  }
  Bqos::Bqos(void* src,size_t len){
      attach(src,len);
  }
  Bqos::Bqos(uint8_t flags,uint8_t qci,uint64_t max_uplink,uint64_t max_downlink,uint64_t guaranteed_uplink,uint64_t guaranteed_downlink){
      memset(&bqos_,0,sizeof(bqos_));
      set(flags, qci, max_uplink, max_downlink, guaranteed_uplink, guaranteed_downlink);
  }
  Bqos::~Bqos(){ }
  void Bqos::set(uint8_t flags,uint8_t qci,uint64_t max_uplink,uint64_t max_downlink,uint64_t guaranteed_uplink,uint64_t guaranteed_downlink){
      bqos_.c.flags = flags;
      bqos_.qci = qci;
      max_uplink = HTONLL(max_uplink);
      max_downlink = HTONLL(max_downlink);
      guaranteed_uplink = HTONLL(guaranteed_uplink);
      guaranteed_downlink = HTONLL(guaranteed_downlink);
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Bqos::set\n\tuplink: %llx\n\tdownlink: %llx\n\tguplink: %llx\n\tgdownlink: %llx",
                         max_uplink,max_downlink,guaranteed_uplink,guaranteed_downlink);
      }
      //
      memcpy(&bqos_.rate[0],&((char*)&max_uplink)[3],5);
      memcpy(&bqos_.rate[5],&((char*)&max_downlink)[3],5);
      memcpy(&bqos_.rate[10],&((char*)&guaranteed_uplink)[3],5);
      memcpy(&bqos_.rate[15],&((char*)&guaranteed_downlink)[3],5);

      bqos_.head.length = htons(sizeof(bqos_) - sizeof(gtpc_comm_header_t));
      bqos_.head.type = type();
  }
  uint64_t Bqos::get(MODE mode){
      uint64_t val(0);
      switch(mode){
          case UPLINK:
              memcpy(&((char*)&val)[3],&bqos_.rate[0],5);
              return(NTOHLL(val));
          case DOWNLINK:
              memcpy(&((char*)&val)[3],&bqos_.rate[5],5);
              return(NTOHLL(val));
          case G_UPLINK:
              memcpy(&((char*)&val)[3],&bqos_.rate[10],5);
              return(NTOHLL(val));
          case G_DOWNLINK:
              memcpy(&((char*)&val)[3],&bqos_.rate[15],5);
              return(NTOHLL(val));
          case QCI:
              val = (uint64_t)bqos_.qci;
              return(val);
      }
      return(0);
  }
  int Bqos::type(void) const{
      return(GTPC_TYPE_BEARER_QOS);
  }
  void* Bqos::ref(void) const{
      return((void*)&bqos_);
  }
  size_t Bqos::len(void) const{
      return(sizeof(bqos_));
  }
  int Bqos::attach(void* src,size_t len){
      gtpc_bearer_qos_ptr p = (gtpc_bearer_qos_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Bqos::attach. (%d)..%d", len, sizeof(gtpc_bearer_qos_t));
      }
      if (len != sizeof(gtpc_bearer_qos_t)){
          Logger::LOGERR("Bqos::attach failed.(%d, %d)", len, sizeof(gtpc_bearer_qos_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == (sizeof(bqos_) - sizeof(gtpc_comm_header_t))){
          memcpy(&bqos_, p, sizeof(bqos_));
          return(RETOK);
      }
      Logger::LOGERR("Bqos::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS
