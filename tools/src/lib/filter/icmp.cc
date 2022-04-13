//
// Created by mixi on 2017/04/24.
//

#include "mixipgw_tools_def.h"
#include "lib/filter/icmp.hpp"
#include "lib/pktheader.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"
typedef MIXIPGW_TOOLS::ProcessParameter PP;

typedef struct icmpv6_pseudo_header{
    struct in6_addr     srcaddr;
    struct in6_addr     dstaddr;
    uint32_t            payloadlen;
    uint32_t            next;
}__attribute__ ((packed)) icmpv6_pseudo_header_t,*icmpv6_pseudo_header_ptr;

static_assert(sizeof(icmpv6_pseudo_header_t)==40, "need 40 octet");

//
namespace MIXIPGW_TOOLS{
  int Icmp::OnFilter(PktInterface* pkt ,int* flag){
      struct ip           *ip_i = IP(pkt);
      struct ip6_hdr      *ip_6 = IPV6(pkt);
      uint32_t             hlen = 0;

      //
      if (Module::VERBOSE() >= PCAPLEVEL_ICMP){
          Logger::APPEND_PCAP(0, (const char*)EH(pkt),pkt->Length());
      }
      // ipv4
      if (CHK_ETHERTYPE(EH(pkt),ETHERTYPE_IP)){
          hlen = (sizeof(struct ether_header) + pkt->LengthVlan() + pkt->LengthIp());
          struct in_addr tmp_addr = IP(pkt)->ip_dst;
          IP(pkt)->ip_dst = IP(pkt)->ip_src;
          IP(pkt)->ip_src = tmp_addr;
          IP(pkt)->ip_sum = 0;
          IP(pkt)->ip_sum = Misc::Wrapsum(Misc::Checksum(IP(pkt), sizeof(struct ip), 0));
          //
          ICMP(pkt)->icmp_type = ICMP_ECHOREPLY;
          ICMP(pkt)->icmp_cksum = 0;
          ICMP(pkt)->icmp_cksum = Misc::Wrapsum(Misc::Checksum(ICMP(pkt), pkt->Length() - hlen, 0));
      }else{
          hlen = (sizeof(struct ether_header) + pkt->LengthVlan() + pkt->LengthIp());
          __uint8_t tmp_addr[16];
          memcpy(tmp_addr, ip_6->ip6_dst.s6_addr, sizeof(tmp_addr));
          memcpy(ip_6->ip6_dst.s6_addr, ip_6->ip6_src.s6_addr, sizeof(tmp_addr));
          memcpy(ip_6->ip6_src.s6_addr, tmp_addr, sizeof(tmp_addr));

          // pseudo header
          char icmpv6bf[ETHER_MAX_LEN];
          icmpv6_pseudo_header_ptr pseudo = (icmpv6_pseudo_header_ptr)icmpv6bf;
          memcpy(pseudo->srcaddr.s6_addr, ip_6->ip6_src.s6_addr, sizeof(pseudo->srcaddr.s6_addr));
          memcpy(pseudo->dstaddr.s6_addr, ip_6->ip6_dst.s6_addr, sizeof(pseudo->dstaddr.s6_addr));
          pseudo->payloadlen = ip_6->ip6_plen;
          pseudo->next = htonl(ip_6->ip6_nxt);
          
          // ICMP echo
          if (ICMPV6(pkt)->icmp6_type == ICMP6_ECHO_REQUEST){
              ICMPV6(pkt)->icmp6_type = ICMP6_ECHO_REPLY;
              ICMPV6(pkt)->icmp6_cksum = 0;
              memcpy((pseudo+1), ICMPV6(pkt), pkt->Length() - hlen); 
              ICMPV6(pkt)->icmp6_cksum = Misc::Wrapsum(Misc::Checksum(pseudo, pkt->Length() - hlen + sizeof(*pseudo), 0));
          // ICMP neighbor solicitaion(as arp)
          }else if (ICMPV6(pkt)->icmp6_type == ND_NEIGHBOR_SOLICIT){
              struct nd_neighbor_solicit* nnsl = (struct nd_neighbor_solicit*)ICMPV6(pkt);
              struct nd_neighbor_advert*  nnad = (struct nd_neighbor_advert*)ICMPV6(pkt);
              struct nd_opt_hdr*          nopt = (struct nd_opt_hdr*)(nnad+1);
              char*                       maca = (char*)(nopt+1);
              //
              if ((process_param_->Get(PP::TXT_MAC_DST))[0] == 0){
                  Logger::LOGERR("invalid missing mac(config)");
                  return(RETERR);
              }
              if ((pkt->Length() - hlen) != (sizeof(struct nd_neighbor_advert) + sizeof(struct nd_opt_hdr) + ETHER_ADDR_LEN)){
                  Logger::LOGERR("invalid request length");
              }
              Logger::LOGINF("target:(%04X%04X%04X%04X-%04X%04X%04X%04X)",
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[0]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[2]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[4]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[6]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[8]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[10]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[12]),
                             *((uint16_t*)&nnsl->nd_ns_target.s6_addr[14]));

              nnad->nd_na_flags_reserved = (ND_NA_FLAG_SOLICITED|ND_NA_FLAG_OVERRIDE);
              nnad->nd_na_type  = ND_NEIGHBOR_ADVERT;
              nnad->nd_na_code  = 0;
              nnad->nd_na_cksum = 0;
              memcpy((pseudo+1), ICMPV6(pkt), pkt->Length() - hlen); 
              nnad->nd_na_cksum = Misc::Wrapsum(Misc::Checksum(pseudo, pkt->Length() - hlen + sizeof(*pseudo), 0));
              nopt->nd_opt_type = ND_OPT_TARGET_LINKADDR;
              nopt->nd_opt_len  = 1;    //  The length of the option in units of 8 octets
              memcpy(maca, process_param_->Get(PP::TXT_MAC_DST), ETHER_ADDR_LEN);
          }
      }
      //
      //
      SwapMac(pkt);
      (*flag) |= PROC_NEED_SEND;
      //
      if (Module::VERBOSE() >= PCAPLEVEL_ICMP){
          Logger::APPEND_PCAP(0, (const char*)EH(pkt),pkt->Length());
      }
      return(RETOK);
  }
}; // namespace MIXIPGW_TOOLS
