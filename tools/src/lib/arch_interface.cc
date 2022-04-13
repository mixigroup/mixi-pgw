//
// Created by mixi on 2017/04/24.
//
#include "mixipgw_tools_def.h"
#include "lib/const/gtpu.h"
#include "lib/interface/arch_interface.hpp"
#include "lib/arch/netmap.hpp"
#include "lib/arch/osxsim.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
//
namespace MIXIPGW_TOOLS{
  // default packet type: judge handler
  class DefaultEvent:public ArchIfEvent{
  public:
      virtual int OnPacketType(PktInterface* pkt, int flg, int level){
          return(ArchIf::GetPacketType(pkt, flg, level));
      }
  };
  static DefaultEvent   defaultEvent;
  //
  int ArchIf::Init(ArchIf** pparch,ProcessParameter* param){
      // TODO: other interfaces.
#ifdef __APPLE__
      (*pparch) = new Osxsim(param);
#else
      (*pparch) = new Netmap(param);
#endif
      (*pparch)->filter_level_  = param->Get(ProcessParameter::USG_FILTER_EXT);
      (*pparch)->module_flag_   = param->Get(ProcessParameter::USG_MODULE_FLAG);
      (*pparch)->udp_ctrl_port_ = param->Get(ProcessParameter::USH_UDP_CTRL_PORT);
      (*pparch)->event_         = &defaultEvent;

      return(RETOK);
  }
  int ArchIf::UnInit(ArchIf** pparch){
      if (pparch != NULL){
          if (*pparch != NULL){
              // TODO: other interfaces.
              delete (Netmap*)(*pparch);
          }
          (*pparch) = NULL;
      }
      return(RETOK);
  }
  int ArchIf::EventPacketType(ArchIfEvent* event){
      event_ = event;
      return(RETOK);
  }

  int ArchIf::GetPacketType(PktInterface* p,int flag, int ctrlport){
      int udplen = (sizeof(struct ether_header) + LEN_VLAN(EH(p)) + sizeof(struct ip) + sizeof(struct udphdr));
      p->Type(RETERR);
      // ARP
      if (p->Length() >= sizeof(struct ether_header) && CHK_ETHERTYPE(EH(p), ETHERTYPE_ARP)) {
          p->Type(RETPTARP);
          return(RETPTARP);
      }
      if (p->Length() >= (sizeof(struct ether_header) + LEN_VLAN(EH(p)) + sizeof(struct ip))){
          if (CHK_ETHERTYPE(EH(p),ETHERTYPE_IP) && IP(p)->ip_v == IPVERSION){
              // ICMP:v4
              if (IP(p)->ip_p == IPPROTO_ICMP && IP(p)->ip_ttl != 0x00 && ICMP(p)->icmp_type == ICMP_ECHO){
                  // for loopback simulater (DEBUG)
                  if (IP(p)->ip_src.s_addr == ntohl(0xdeadbeaf)){
                      p->Type(RETPTOVERIP);
                      return(RETPTOVERIP);
                  }else{
                      p->Type(RETPTICMP);
                      return(RETPTICMP);
                  }
              }
              // GTP-[U/C] or BFD
              if (p->Length() >= udplen && ntohs(UDP(p)->uh_ulen) >= sizeof(gtpu_header_t) && IP(p)->ip_p == IPPROTO_UDP){
                  if (UDP(p)->uh_dport == htons(GTPU_PORT) || UDP(p)->uh_dport == htons(_DBG_GTPU_+GTPU_PORT) /* && gtpu->length >= (sizeof(gtpu_header_t) - 8)*/){
                      if (GTPU(p)->type == GTPU_ECHO_REQ){
                          p->Type(RETPTGTPUECHOREQ);
                          return(RETPTGTPUECHOREQ);
                      }
                      p->Type(RETPTGTPU);
                      return(RETPTGTPU);
                  }else if (ntohs(UDP(p)->uh_dport) == GTPC_PORT){
                      p->Type(RETPTGTPC);
                      return(RETPTGTPC);
                  }else if (ntohs(UDP(p)->uh_dport) == BFDC_PORT){
                      p->Type(RETPTBFDC);
                      return(RETPTBFDC);
                  }
              }
              // udp control
              if (p->Length() >= udplen && (IP(p)->ip_p == IPPROTO_UDP && ntohs(UDP(p)->uh_dport) == ctrlport)){
                  p->Type(RETPTCTRL);
                  return(RETPTCTRL);
              }
              // when decode lane dont need RETPTOVERIP(encap)
              if (MF_IS_DECAP(flag)){ return(RETERR); }
              // udp/tcp over IP
              if (p->Length() >= udplen && (IP(p)->ip_p == IPPROTO_UDP || IP(p)->ip_p == IPPROTO_TCP || IP(p)->ip_p == IPPROTO_ICMP)){
                  p->Type(RETPTOVERIP);
                  return(RETPTOVERIP);
              }
          }else if (CHK_ETHERTYPE(EH(p),ETHERTYPE_IPV6)){
              // ICMP:6
              if (IPV6(p)->ip6_nxt == IPPROTO_ICMPV6 && IPV6(p)->ip6_hlim != 0x00){
                  p->Type(RETPTICMP);
                  return(RETPTICMP);
              }
              // GTP-[U/C] or BFD:over ipv6
              if (p->Length() >= udplen && ntohs(UDP(p)->uh_ulen) >= sizeof(gtpu_header_t) && IPV6(p)->ip6_nxt == IPPROTO_UDP){
                  if (UDP(p)->uh_dport == htons(GTPU_PORT) || UDP(p)->uh_dport == htons(_DBG_GTPU_+GTPU_PORT) /* && gtpu->length >= (sizeof(gtpu_header_t) - 8)*/){
                      if (GTPU(p)->type == GTPU_ECHO_REQ){
                          p->Type(RETPTGTPUECHOREQ);
                          return(RETPTGTPUECHOREQ);
                      }
                      p->Type(RETPTGTPU);
                      return(RETPTGTPU);
                  }else if (ntohs(UDP(p)->uh_dport) == GTPC_PORT){
                      p->Type(RETPTGTPC);
                      return(RETPTGTPC);
                  }else if (ntohs(UDP(p)->uh_dport) == BFDC_PORT){
                      p->Type(RETPTBFDC);
                      return(RETPTBFDC);
                  }
              }
              // udp control
              if (p->Length() >= udplen && (IPV6(p)->ip6_nxt == IPPROTO_UDP && ntohs(UDP(p)->uh_dport) == ctrlport)){
                  p->Type(RETPTCTRL);
                  return(RETPTCTRL);
              }
              // when decode lane dont need RETPTOVERIP(encap)
              if (MF_IS_DECAP(flag)){ return(RETERR); }
              // udp/tcp over ipv6
              if (p->Length() >= udplen && (IPV6(p)->ip6_nxt == IPPROTO_UDP || IPV6(p)->ip6_nxt == IPPROTO_TCP)){
                  p->Type(RETPTOVERIP);
                  return(RETPTOVERIP);
              }
          }
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS
