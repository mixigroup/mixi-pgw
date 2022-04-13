//
// Created by mixi on 2017/04/26.
//

#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/filter/gtpu_encap.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"
#include "lib/logger.hpp"

#include "lib/const/link.h"
#include "lib/const/policer.h"
#include "lib/const/gtpu.h"
#include <stdexcept>

using namespace MIXIPGW_TOOLS;


namespace MIXIPGW_TOOLS{
  typedef DualBufferedLookupTable<link_t>       TEID_TBL;       // must be initialized teid tagble at netmap thread context.
  typedef DualBufferedLookupTable<policer_t>    POLICER_TBL;    // for policy table, netmap thread is write-side only.
  //
  int TranslatorEncapEnforcer::OnInitFilter(void){
      // prepare header regions in thread local heap
      // (for use as padding during encapsulate)
      encap_ = (encap_header_ptr)malloc(sizeof(encap_header_t));
      if (encap_ == NULL){
          throw std::runtime_error("");
      }
      struct udphdr       *udp_e= &encap_->udp_enc;
      struct gtpu_header  *gtp_e= &encap_->gtp_enc;
      //
      udp_e->uh_sport           = htons(GTPU_PORT);
      udp_e->uh_dport           = htons(_DBG_GTPU_+GTPU_PORT);
      gtp_e->u.v1_flags.version = GTPU_VERSION_1;
      gtp_e->u.v1_flags.proto   = GTPU_PROTO_GTP;
      gtp_e->u.v1_flags.npdu    = GTPU_NPDU_OFF;
      gtp_e->type               = GTPU_G_PDU;
      recalc_counter_           = 0;
      //
      return(RETOK);
  }
  int TranslatorEncapEnforcer::OnUnInitFilter(void){
      if (encap_){ free(encap_); }
      encap_ = NULL;
      return(RETOK);
  }
  //
  int TranslatorEncapMarker::OnFilter(PktInterface* pkt ,int* flag){
      struct ether_header *eh_i = EH(pkt);
      struct ip           *ip_i = IP(pkt);
      // encapsulate gtpu
      struct ip           *ip_e = &encap_->ip_enc;
      struct udphdr       *udp_e= &encap_->udp_enc;
      struct gtpu_header  *gtp_e= &encap_->gtp_enc;
 //   uint32_t            *gtp_seq=&encap_->seq;
      size_t              tlen  = pkt->Length() - pkt->LengthVlan() + sizeof(encap_header_t) - sizeof(struct ether_header);
      int                 isred = 0;
      size_t vlan_offset = LEN_VLAN(eh_i);
      size_t vlan_extend_size = 0;
      static uint32_t     counter = 0;
      // extend when vlan flag not exists.
      if (vlan_offset == 0){
          // vlan tag id = 1
          EH(pkt)->ether_type = htons(ETHERTYPE_VLAN);
          (*(((u_short*)(EH(pkt)+1))+0))=0x0001;
          (*(((u_short*)(EH(pkt)+1))+1))=htons(ETHERTYPE_IP);
          vlan_extend_size = 4;
      }else if (vlan_offset == 8){
          throw "not implemented vlan-length. 8. ";
      }
#ifdef __WITHOUT_ECM__
      __be32                findtrgt = (ip_i->ip_src.s_addr);
#else
      __be32                findtrgt = (ip_i->ip_dst.s_addr);
#endif
      if (ip_i->ip_src.s_addr == htonl(0xdeadbeaf)){
          findtrgt = ICMP(pkt)->icmp_ip.ip_dst.s_addr;
          ip_i->ip_src.s_addr = MAKE_NEXT_HOP(ip_i->ip_dst.s_addr, FID_REFLECT);
      }
      // setup ip
      ip_e->ip_len   = htons(tlen - sizeof(struct ether_header) - 4);
      ip_e->ip_hl    = 5;
      ip_e->ip_v     = IPVERSION;
      udp_e->uh_sum  = 0;
      udp_e->uh_ulen = htons(tlen - sizeof(struct ether_header) - 4 - (GTPU_IPV4HL*4));
      gtp_e->length  = htons(tlen - sizeof(struct ether_header) - 4 - (GTPU_IPV4HL*4) - sizeof(struct udphdr) - 8);
      gtp_e->tid     = 0;
      gtp_e->u.v1_flags.sequence = GTPU_SEQ_0;
//    (*gtp_seq)     = counter++;
      if (counter > 0x7fff){ counter = 0; }
      //
      if (ip_i->ip_v == IPVERSION){
          // tunneling main.
          TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(findtrgt));
          if (tbl == NULL){
              Logger::LOGERR("missing teid table.(%u)", findtrgt);
              return(0);
          }
          auto it = tbl->Find(findtrgt, 0);
          if (it == tbl->End()){
              Logger::LOGERR("not found target.(%u)", findtrgt);
              return(0);
          }
          // from tunnel link
          gtp_e->tid = htonl(it->w.pgw_teid_uc);
          // judgement policy.
          POLICER_TBL* ptbl = (POLICER_TBL*)process_param_->Get(ProcessParameter::TBL_POLICER, Misc::TeidGid(findtrgt));
          if (ptbl == NULL){
              Logger::LOGERR("missing policy table.(%u)", findtrgt);
              return(0);
          }
          // re calc policer.
          if ((*flag) & PROC_TRIGGER_1S_EVENT){

              Logger::LOGINF("PROC_TRIGGER_1S_EVENT(gtpu_encap - marker) >> start.");
              recalc_counter_++;
              (*flag) &= ~PROC_TRIGGER_1S_EVENT;
              Logger::LOGINF("PROC_TRIGGER_1S_EVENT:%010u(gtpu_encap - marker) << completed", recalc_counter_);
          }
          auto itb = ptbl->Find(findtrgt, 1);
          if (itb == ptbl->End()){
              throw "exception. ";
          }
          auto itf = ptbl->Find(findtrgt, 0);
          //
          if (itf == ptbl->End()){
              isred = 1;
          }else{
              // since last calculation, recalculation counter has advanced
              // => repenish token by number of seconds elapsed.
              auto dt = (recalc_counter_ - itf->stat.epoch_w);
              if (dt){
                  itf->commit_rate = MIN(itf->commit_burst_size, itf->commit_rate + (itf->commit_information_rate * dt));
                  itf->excess_rate = MIN(itf->excess_burst_size, itf->excess_rate + (itf->excess_information_rate * dt));
              }
              // front -> back(value only)
              itb->commit_rate          = itf->commit_rate;
              itb->commit_burst_size    = itf->commit_burst_size;
              itb->commit_information_rate  = itf->commit_information_rate;
              itb->excess_rate          = itf->excess_rate;
              itb->excess_burst_size    = itf->excess_burst_size;
              itb->excess_information_rate  = itf->excess_information_rate;
              //
              if (pkt->Length() <= itf->commit_rate){
                  // green
                  itb->commit_rate = (itf->commit_rate - pkt->Length());
                  itb->stat.valid=LINKMAPSTAT_ON;
              }else if (pkt->Length() <= itf->excess_rate){
                  // yellow
                  itb->excess_rate = (itf->excess_rate - pkt->Length());
                  itb->stat.valid=LINKMAPSTAT_ON;
              }else{
                  // encap(internet -> when sgw direction do not drop)
                  isred = 1;
              }
              // update and swap.
              ptbl->SwapSide(findtrgt);
          }
          // QOS
          if (isred){
              ip_e->ip_tos = (u_char)(IPTOS_DSCP(it->u.bitrate.qos) | IPTOS_ECN(0));
          }
      }else{
          Logger::LOGERR("not implemented ip version(%u)", ip_i->ip_v);
          return(RETERR);
      }
      // modify dst [ip (on ip header) / teid (on gtpu header)]
//    ip_e->ip_tos = ip_i->ip_tos;
      ip_e->ip_id  = ip_i->ip_id;
      ip_e->ip_off = ip_i->ip_off;
      uint32_t nexthop = process_param_->Get(ProcessParameter::USG_NEXT_HOP_IPV4);
//    ip_e->ip_dst.s_addr = MAKE_NEXT_HOP(ip_i->ip_dst.s_addr,FID_COUNTER_E);
      ip_e->ip_dst.s_addr = nexthop;
      ip_e->ip_src.s_addr = ip_i->ip_dst.s_addr;
      ip_e->ip_p   = IPPROTO_UDP;
      ip_e->ip_sum = 0;
      ip_e->ip_sum = Misc::Wrapsum(Misc::Checksum(ip_e, sizeof(struct ip), 0));

      // 64bit aligned fast copy
      // supported only upper protocol of ip packet.
      // 1 slot/2K buffers exists in netmap environment.
//    memmove((char*)(EH(pkt)) + sizeof(encap_header_t)),ip_i, ntohs(ip_i->ip_len));
      #define MCPY_SKIP (8)
      unsigned short iplen = ntohs(ip_i->ip_len);
      iplen = iplen + (MCPY_SKIP-(iplen%MCPY_SKIP));
      for(int n = (iplen-MCPY_SKIP);n >= 0;n -= MCPY_SKIP){
          memcpy(((char*)EH(pkt)) + sizeof(encap_header_t) + n, ((char*)ip_i) + n, MCPY_SKIP);
      }
      memcpy(((char*)IP(pkt)) + vlan_extend_size, ip_e, sizeof(struct ip));
      memcpy(((char*)UDP(pkt)) + vlan_extend_size, udp_e, sizeof(struct udphdr));
      memcpy(((char*)GTPU(pkt)) + vlan_extend_size, gtp_e, sizeof(gtpu_header_t));
      //
      pkt->Length(tlen);
      //
      uint32_t decap_vlanid = process_param_->Get(ProcessParameter::USG_NEXT_HOP_VLANID); 
      (*(((u_short*)(EH(pkt)+1))+0)) = htons(decap_vlanid);
      //
      SwapMac(pkt);
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }
  //
  int TranslatorEncapEnforcer::OnFilter(PktInterface* pkt ,int* flag){
      struct ether_header *eh_i = EH(pkt);
      struct ip           *ip_i = IP(pkt);
      // encapsulate gtpu
      struct ip           *ip_e = &encap_->ip_enc;
      struct udphdr       *udp_e= &encap_->udp_enc;
      struct gtpu_header  *gtp_e= &encap_->gtp_enc;
      //   uint32_t            *gtp_seq=&encap_->seq;
      size_t              tlen  = pkt->Length() - pkt->LengthVlan() + sizeof(encap_header_t) - sizeof(struct ether_header);
      int                 isred = 0;
      size_t vlan_offset = LEN_VLAN(eh_i);
      size_t vlan_extend_size = 0;
      static uint32_t     counter = 0;
      //
      if (vlan_offset != 4){
          throw "not implemented vlan-length. !=4. ";
      }
#ifdef __WITHOUT_ECM__
      __be32                findtrgt = (ip_i->ip_src.s_addr);
#else
      __be32                findtrgt = (ip_i->ip_dst.s_addr);
#endif
      // setup ip
      ip_e->ip_len   = htons(tlen - sizeof(struct ether_header) - 4);
      ip_e->ip_hl    = 5;
      ip_e->ip_v     = IPVERSION;
      udp_e->uh_sum  = 0;
      udp_e->uh_ulen = htons(tlen - sizeof(struct ether_header) - 4 - (GTPU_IPV4HL*4));
      gtp_e->length  = htons(tlen - sizeof(struct ether_header) - 4 - (GTPU_IPV4HL*4) - sizeof(struct udphdr) - 8);
      gtp_e->tid     = 0;
      gtp_e->u.v1_flags.sequence = GTPU_SEQ_0;
      //
      if (ip_i->ip_v == IPVERSION){
          // tunneling main.
          TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(findtrgt));
          if (tbl == NULL){
              Logger::LOGERR("missing teid table.(%u)", findtrgt);
              return(0);
          }
          auto it = tbl->Find(findtrgt, 0);
          if (it == tbl->End()){
              Logger::LOGERR("not found target.(%u)", findtrgt);
              return(0);
          }
          // from tunnel link
          gtp_e->tid = htonl(it->w.pgw_teid_uc);

          // calculate policy -> apply to ip header
          {   POLICER_TBL* ptbl = (POLICER_TBL*)process_param_->Get(ProcessParameter::TBL_POLICER, Misc::TeidGid(findtrgt));
              if (ptbl == NULL){
                  return(0);
              }
              // Recalculate policy event(set flags from BFD counter)
              // re calc policer.
              if ((*flag) & PROC_TRIGGER_1S_EVENT){
                  Logger::LOGINF("PROC_TRIGGER_1S_EVENT(gtpu_encap - enforcer) >> start.");
                  recalc_counter_++;
                  (*flag) &= ~PROC_TRIGGER_1S_EVENT;
                  Logger::LOGINF("PROC_TRIGGER_1S_EVENT:%010u(gtpu_encap - enforcer) << completed", recalc_counter_);
              }
              auto itb = ptbl->Find(findtrgt, 1);
              if (itb == ptbl->End()){
                  throw "exception. ";
              }
              auto itf = ptbl->Find(findtrgt, 0);
              /*

                           +---------------------------------+
                           |periodically every T sec.        |
                           | Tc(t+)=MIN(CBS, Tc(t-)+CIR*T)   |
                           | Te(t+)=MIN(EBS, Te(t-)+EIR*T)   |
                           +---------------------------------+

                  Packet of size
                      B arrives   /----------------\
                 ---------------->|color-blind mode|
                                  |       OR       |YES  +---------------+
                                  |  green packet  |---->|packet is green|
                                  |      AND       |     |Tc(t+)=Tc(t-)-B|
                                  |    B <= Tc(t-) |     +---------------+
                                  \----------------/
                                          |
                                          | NO
                                          v
                                  /----------------\
                                  |color-blind mode|
                                  |       OR       |YES  +----------------+
                                  | NOT red packet |---->|packet is yellow|
                                  |      AND       |     |Te(t+)=Te(t-)-B |
                                  |    B <= Te(t-) |     +----------------+
                                  \----------------/
                                          |
                                          | NO
                                          v
                                  +---------------+
                                  |packet is red  |
                                  +---------------+
               *
               *
               * */
              //
              if (itf == ptbl->End()){
                  Logger::LOGERR("drop: RED marker: (%u).", findtrgt, tlen);
                  return(0);
              }else{
                  // since last calculation, recalculation counter has advanced
                  // => replenish token by number of seconds elapsed.
                  auto dt = (recalc_counter_ - itf->stat.epoch_w);
                  if (dt){
                      itf->commit_rate = MIN(itf->commit_burst_size, itf->commit_rate + (itf->commit_information_rate * dt));
                      itf->excess_rate = MIN(itf->excess_burst_size, itf->excess_rate + (itf->excess_information_rate * dt));
                  }

                  // front -> back(value only)
                  itb->commit_rate          = itf->commit_rate;
                  itb->commit_burst_size    = itf->commit_burst_size;
                  itb->commit_information_rate  = itf->commit_information_rate;
                  itb->excess_rate          = itf->excess_rate;
                  itb->excess_burst_size    = itf->excess_burst_size;
                  itb->excess_information_rate  = itf->excess_information_rate;
                  //
                  if (pkt->Length() <= itf->commit_rate){
                      itb->commit_rate = (itf->commit_rate - pkt->Length());
                      itb->stat.valid=LINKMAPSTAT_ON;
                      itb->stat.epoch_w=recalc_counter_;
                      if (Module::VERBOSE() > PCAPLEVEL_MIN){
                          Logger::LOGINF("[GREEN]CR : %u/CBS : %u/CIR : %u. ER : %u/ EBS: %u/EIR: %u",
                                         itb->commit_rate,
                                         itb->commit_burst_size,
                                         itb->commit_information_rate,
                                         itb->excess_rate,
                                         itb->excess_burst_size,
                                         itb->excess_information_rate
                          );
                      }
                      // green ok
                  }else if (pkt->Length() <= itf->excess_rate){
                      itb->excess_rate = (itf->excess_rate - pkt->Length());
                      itb->stat.valid=LINKMAPSTAT_ON;
                      itb->stat.epoch_w=recalc_counter_;
                      // yellow
                      ip_e->ip_tos = (u_char)(IPTOS_DSCP(it->u.bitrate.qos) | IPTOS_ECN(0));
                      if (Module::VERBOSE() > PCAPLEVEL_MIN){
                          Logger::LOGINF("[YELLOW]CR : %u/CBS : %u/CIR : %u. ER : %u/ EBS: %u/EIR: %u",
                                         itb->commit_rate,
                                         itb->commit_burst_size,
                                         itb->commit_information_rate,
                                         itb->excess_rate,
                                         itb->excess_burst_size,
                                         itb->excess_information_rate
                          );
                      }
                  }else{
                      // color red .. drop.
                      (*flag) |= PROC_RFC4115_RED;
                      if (Module::VERBOSE() > PCAPLEVEL_MIN){
                          Logger::LOGINF("[RED]CR : %u/CBS : %u/CIR : %u. ER : %u/ EBS: %u/EIR: %u",
                                         itf->commit_rate,
                                         itf->commit_burst_size,
                                         itf->commit_information_rate,
                                         itf->excess_rate,
                                         itf->excess_burst_size,
                                         itf->excess_information_rate
                          );
                      }
                      return(0);
                  }
                  // update and swap.
                  ptbl->SwapSide(findtrgt);
                  ptbl->NotifyChange(findtrgt);
              }
          }
      }else{
          Logger::LOGERR("not implemented ip version(%u)", ip_i->ip_v);
          return(RETERR);
      }
      // modify dst [ip (on ip header) / teid (on gtpu header)]
      ip_e->ip_id  = ip_i->ip_id;
      ip_e->ip_off = ip_i->ip_off;
      uint32_t nexthop = process_param_->Get(ProcessParameter::USG_NEXT_HOP_IPV4);
      ip_e->ip_dst.s_addr = nexthop;
      ip_e->ip_src.s_addr = ip_i->ip_dst.s_addr;
      ip_e->ip_p   = IPPROTO_UDP;
      ip_e->ip_sum = 0;
      ip_e->ip_sum = Misc::Wrapsum(Misc::Checksum(ip_e, sizeof(struct ip), 0));

      #define MCPY_SKIP (8)
      unsigned short iplen = ntohs(ip_i->ip_len);
      iplen = iplen + (MCPY_SKIP-(iplen%MCPY_SKIP));
      for(int n = (iplen-MCPY_SKIP);n >= 0;n -= MCPY_SKIP){
          memcpy(((char*)EH(pkt)) + sizeof(encap_header_t) + n, ((char*)ip_i) + n, MCPY_SKIP);
      }
      memcpy(((char*)IP(pkt)) + vlan_extend_size, ip_e, sizeof(struct ip));
      memcpy(((char*)UDP(pkt)) + vlan_extend_size, udp_e, sizeof(struct udphdr));
      memcpy(((char*)GTPU(pkt)) + vlan_extend_size, gtp_e, sizeof(gtpu_header_t));
      //
      pkt->Length(tlen);
      //
      uint32_t decap_vlanid = process_param_->Get(ProcessParameter::USG_NEXT_HOP_VLANID);
      (*(((u_short*)(EH(pkt)+1))+0)) = htons(decap_vlanid);
      //
      SwapMac(pkt);
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }

}; // namespace MIXIPGW_TOOLS
