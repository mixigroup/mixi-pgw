//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/buffer.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"

//
namespace MIXIPGW_TOOLS{
  Fteid::Fteid(){
      memset(&fteid_,0,sizeof(fteid_));
      set(0,0,0,NULL,NULL);
  }
  Fteid::Fteid(void* src,size_t len){
      attach(src,len);
  }
  Fteid::Fteid(uint8_t inst, uint8_t iftype, uint32_t teid,uint8_t* ipv4,uint8_t* ipv6){
      memset(&fteid_,0,sizeof(fteid_));
      set(inst, iftype, teid, ipv4, ipv6);
  }

  Fteid::~Fteid(){ }
  void Fteid::set(uint8_t inst, uint8_t iftype, uint32_t teid,uint8_t* ipv4,uint8_t* ipv6){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Fteid::set(%x, %x %x %p %p)", inst, iftype, teid, ipv4, ipv6);
      }
      size_t offset = 0;
      fteid_.head.length = 0;
      fteid_.head.type = type();
      fteid_.head.inst.instance = inst;
      fteid_.c.flags = 0;
      fteid_.teid_grekey = htonl(teid);
      fteid_.c.bit.iftype = iftype;
      //
      if (ipv4 != NULL){
          fteid_.c.bit.v4 = 1;
          memcpy(fteid_.blk, ipv4, 4);
          offset += 4;
      }
      if (ipv6 != NULL){
          fteid_.c.bit.v6 = 1;
          memcpy(fteid_.blk, ipv6, 16);
          offset += 16;
      }
      fteid_.head.length = htons(offset + 5);
  }
  uint32_t Fteid::teid(void){
      return((uint32_t)(ntohl(fteid_.teid_grekey)));
  }

  std::string Fteid::ipv(void){
#define IPV6BINFMT  ("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x")
#define IPV4BINFMT  ("%u.%u.%u.%u")
      char  bf[64] = {0};
      if (fteid_.c.bit.v6){
          snprintf(bf, sizeof(bf)-1, IPV6BINFMT,
                   fteid_.blk[0],fteid_.blk[1],
                   fteid_.blk[2],fteid_.blk[3],
                   fteid_.blk[4],fteid_.blk[5],
                   fteid_.blk[6],fteid_.blk[7],
                   fteid_.blk[8],fteid_.blk[9],
                   fteid_.blk[10],fteid_.blk[11],
                   fteid_.blk[12],fteid_.blk[13],
                   fteid_.blk[14],fteid_.blk[15]);
      }else if (fteid_.c.bit.v4){
          snprintf(bf, sizeof(bf)-1, IPV4BINFMT,
                   fteid_.blk[0],fteid_.blk[1],
                   fteid_.blk[2],fteid_.blk[3]);
      }
      return(bf);
  }
  int Fteid::type(void) const{
      return(GTPC_TYPE_F_TEID);
  }
  void* Fteid::ref(void) const{
      return((void*)&fteid_);
  }
  size_t Fteid::len(void) const{
      return(ntohs(fteid_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Fteid::attach(void* src,size_t len){
      gtpc_f_teid_ptr p = (gtpc_f_teid_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Fteid::attach. (%d)..%d", len, sizeof(gtpc_f_teid_t));
      }
      if (len < 13){    // dg 10-1-1-75:Fully Qualified TEID(F-TEID)
          Logger::LOGERR("Fteid::attach failed.(%d, %d)", len, sizeof(gtpc_comm_header_t));
          return(RETERR);
      }
      if (p->head.type == type() && p->head.length >= htons(9)){
          memcpy(&fteid_, p, MIN(sizeof(fteid_), len));
          return(RETOK);
      }
      Logger::LOGERR("Fteid::attach... failed.(%d, %d)", p->head.length, p->head.type);
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS