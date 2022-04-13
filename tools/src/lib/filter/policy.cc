//
// Created by mixi on 2017/07/05.
//


#include "mixipgw_tools_def.h"
#include "lib/const/policy.h"
#include "lib/filter/policy.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/misc.hpp"
#include "lib_lookup/lookup_table_tpl.hpp"

#ifndef IN_VLANID
#define IN_VLANID ntohs(10)
#endif

#ifndef OUT_VLANID_MIN
#define OUT_VLANID_MIN (11)
#endif
#ifndef OUT_VLANID_MAX
#define OUT_VLANID_MAX (13)
#endif
#ifndef OUT_VLANID_DBG
#define OUT_VLANID_DBG (4000)
#endif

//
namespace MIXIPGW_TOOLS{
  typedef LookupTable<policy_t,1048576,uint32_t>    POLICY_TBL;

  int Policy::OnFilter(PktInterface* pkt ,int* flag){
      struct ether_header *eh_i = EH(pkt);
      struct ip           *ip_i = IP(pkt);
      struct ip6_hdr      *ip_6 = IPV6(pkt);
      size_t vlan_offset = LEN_VLAN(eh_i);

      // throw exception, when vlan flag not exists.
      // Unexpected configuration of network specific devices in route.
      if (vlan_offset != 4){
          throw "not implemented vlan-length. !=4. ";
      }
      if ((*(((u_short*)(eh_i+1))+0)) != IN_VLANID){
          Logger::LOGERR("invalid input vlanid.(%u)", (*(((u_short*)(eh_i+1))+0)));
          return(0);
      }
      __be32    findtrgt = 0;

      // 24bit
      // |012345678901234567890123|
      // |----+-------------------|
      // |gid | unique identifier |
      // |----+-------------------|
      if (ip_i->ip_v == IPVERSION){
          // ipv4 /8
          findtrgt = ntohl(((uint32_t)(ip_i->ip_dst.s_addr)))&0x000FFFFF;
          if (Module::VERBOSE() > PCAPLEVEL_PARSE){
              Logger::LOGINF("ipv4[%08x]", ip_i->ip_dst.s_addr);
          }
      }else{
          // ipv6/40
          findtrgt = ntohl(*((uint32_t*)(&ip_6->ip6_dst.s6_addr[4])))&0x000FFFFF;
      }
      // ipdst -> link find by vlanid
      POLICY_TBL* tbl = (POLICY_TBL*)process_param_->Get(ProcessParameter::TBL_POLICY, 0);
      if (tbl == NULL){
          Logger::LOGERR("missing policy table.(%u)", findtrgt);
          return(0);
      }
      auto it = tbl->Find(findtrgt, 0);
      if (it == tbl->End()){
          Logger::LOGERR("not found policy target.(%u)", findtrgt);
          return(0);
      }
      if ((it->stat.linked_vlanid < OUT_VLANID_MIN || it->stat.linked_vlanid > OUT_VLANID_MAX) && it->stat.linked_vlanid != OUT_VLANID_DBG){
          struct in_addr ia;
          ia.s_addr = (it->linked_ipv4);
          Logger::LOGERR("invalid output vlanid.(%u)", it->stat.linked_vlanid);
          Logger::LOGINF("in[%u]->out[%u]/findipv[%08X:%s]/linkedip[%08X]/ipdst[%08X]",ntohs(*(((u_short*)(eh_i+1))+0)),(it->stat.linked_vlanid), (findtrgt), inet_ntoa(ia), it->linked_ipv4, ip_i->ip_dst.s_addr );
          return(0);
      }  
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          struct in_addr ia;
          ia.s_addr = (it->linked_ipv4);
          Logger::LOGINF("in[%u]->out[%u]/findipv[%08X:%s]/linkedip[%08X]",ntohs(*(((u_short*)(eh_i+1))+0)),(it->stat.linked_vlanid), (findtrgt), inet_ntoa(ia), it->linked_ipv4 );
      } 

      // reconfigure vlanid for each destination ip address.
      (*(((u_short*)(eh_i+1))+0))=htons(it->stat.linked_vlanid);
      // response to sender.
      SwapMac(pkt);
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }
}; // namespace MIXIPGW_TOOLS
