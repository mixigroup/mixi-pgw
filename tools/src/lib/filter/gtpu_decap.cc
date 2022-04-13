//
// Created by mixi on 2017/04/26.
//


#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/filter/gtpu_decap.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"
#include "lib/logger.hpp"

#include "lib/const/link.h"
#include "lib/const/policer.h"
#include "lib/const/gtpu.h"

namespace MIXIPGW_TOOLS{
  typedef DualBufferedLookupTable<link_t>       TEID_TBL;       // must be initialized teid table at netmap thread context.
  typedef DualBufferedLookupTable<policer_t>    POLICER_TBL;    // for policy table , netmap thread is write-side only.

  //
  int TranslatorDecapEnforcer::OnInitFilter(void){
      recalc_counter_ = 0;
      return(RETOK);
  }
  int TranslatorDecapMarker::OnInitFilter(void){
      recalc_counter_ = 0;
      return(RETOK);
  }
  //
  int TranslatorDecapEnforcer::OnFilter(PktInterface* pkt ,int* flag){
      if (GTPU(pkt)->u.v1_flags.version != GTPU_VERSION_1){ Logger::LOGERR("invalid gtp-u-v1 version"); return(RETERR); }
      if (GTPU(pkt)->u.v1_flags.proto != GTPU_PROTO_GTP){ Logger::LOGERR("invalid gtp-u-v1 protocol"); return(RETERR); }
      if (GTPU(pkt)->u.v1_flags.npdu != GTPU_NPDU_OFF){ Logger::LOGERR("invalid gtp-u-v1 npdu flag"); return(RETERR); }
      if (GTPU(pkt)->type != GTPU_G_PDU){ Logger::LOGERR("invalid gtp-u-v1 type"); return(RETERR); }
      __be32              teid = ntohl(GTPU(pkt)->tid);
      size_t vlan_offset = LEN_VLAN(EH(pkt));
      size_t vlan_extend_size = 0;
      struct ip* ip =NULL;

      // extend when not exists vlan.
      if (vlan_offset == 0){
          // use , vlan tag id <= 1
          EH(pkt)->ether_type = htons(ETHERTYPE_VLAN);
          (*(((u_short*)(EH(pkt)+1))+0))=0x0001;
          (*(((u_short*)(EH(pkt)+1))+1))=htons(ETHERTYPE_IP);
          vlan_extend_size = 4;
      }
      // pointer of IP header after vlan extended.
      ip = (struct ip*)(((char*)pkt->Header(PktHeader::IP)) + vlan_extend_size);
      //
      if (IP_I(pkt)->ip_v == IPVERSION){
          // prepare next.  iphdr copy(not decided yet)
          // nm_pkt_copy(ip_t, ph->req + sizeof(struct ether_header), ntohs(ip_t->ip_len));
          memmove(ip, IP_I(pkt), ntohs(IP_I(pkt)->ip_len));

      }else{
          // ipv6
          Logger::LOGERR("not implemented.");
          return(RETERR);
      }
      // decap-> recalculate packet length(shorter)
      size_t tlen = pkt->Length() - (size_t)(((char*)IP_I(pkt)) - ((char*)ip));
      pkt->Length(tlen);

      // tunneling main.
      {   TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(teid));
          if (tbl == NULL){
              return(0);
          }
          auto it = tbl->Find(teid, 0);
          if (it == tbl->End()){
              return(0);
          }
          // calculate policy -> apply to ip header
          {   POLICER_TBL* ptbl = (POLICER_TBL*)process_param_->Get(ProcessParameter::TBL_POLICER, Misc::TeidGid(teid));
              if (ptbl == NULL){
                  return(0);
              }
              // Recalculate Policy event(set flags from BFD counter)
              // Increment recalculation counter of epoch seconds since process started.
              //    (recalculationg all tables is verry costly.)
              //    - starting 0, every 1 second(sufficiently huge area), so 32-bit overflow can be ignored.
              // ----
              // re calc policer.
              if ((*flag) & PROC_TRIGGER_1S_EVENT){
                  Logger::LOGINF("PROC_TRIGGER_1S_EVENT(gtpu_decap.) >> start.");
                  recalc_counter_++;
                  (*flag) &= ~PROC_TRIGGER_1S_EVENT;
                  Logger::LOGINF("PROC_TRIGGER_1S_EVENT:%010u(gtpu_decap.) << completed", recalc_counter_);
              }
              auto itb = ptbl->Find(teid, 1);
              if (itb == ptbl->End()){
                  throw "exception. ";
              }
              auto itf = ptbl->Find(teid, 0);
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
                  Logger::LOGERR("drop: RED marker: (%u).", teid, tlen);
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
                      ip->ip_tos = (u_char)(IPTOS_DSCP(it->u.bitrate.qos) | IPTOS_ECN(0));
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
                      // color red >> drop.
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
                  ptbl->SwapSide(teid);
                  ptbl->NotifyChange(teid);
              }
          }
      }
      //
      uint32_t decap_vlanid = process_param_->Get(ProcessParameter::USG_NEXT_HOP_VLANID); 
      (*(((u_short*)(EH(pkt)+1))+0)) = htons(decap_vlanid);
      SwapMac(pkt);

      ip->ip_sum = 0;
      ip->ip_sum = Misc::Wrapsum(Misc::Checksum(ip, sizeof(struct ip), 0));
      if (Module::VERBOSE() > PCAPLEVEL_MIN){
          Logger::LOGINF("sum: %u", ip->ip_sum);
      }
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }


  //
  int TranslatorDecapMarker::OnFilter(PktInterface* pkt ,int* flag){
      if (GTPU(pkt)->u.v1_flags.version != GTPU_VERSION_1){ Logger::LOGERR("invalid gtp-u-v1 version"); return(RETERR); }
      if (GTPU(pkt)->u.v1_flags.proto != GTPU_PROTO_GTP){ Logger::LOGERR("invalid gtp-u-v1 protocol"); return(RETERR); }
      if (GTPU(pkt)->u.v1_flags.npdu != GTPU_NPDU_OFF){ Logger::LOGERR("invalid gtp-u-v1 npdu flag"); return(RETERR); }
      if (GTPU(pkt)->type != GTPU_G_PDU){ Logger::LOGERR("invalid gtp-u-v1 type"); return(RETERR); }
      __be32              teid = ntohl(GTPU(pkt)->tid);
      size_t vlan_offset = LEN_VLAN(EH(pkt));
      size_t vlan_extend_size = 0;
      int    isred = 0;
      struct ip* ip =NULL;

      // extend when not exists vlan.
      if (vlan_offset == 0){
          // use , vlan tag id <= 1
          EH(pkt)->ether_type = htons(ETHERTYPE_VLAN);
          (*(((u_short*)(EH(pkt)+1))+0))=0x0001;
          (*(((u_short*)(EH(pkt)+1))+1))=htons(ETHERTYPE_IP);
          vlan_extend_size = 4;
      }
      // pointer of IP header after vlan extended.
      ip = (struct ip*)(((char*)pkt->Header(PktHeader::IP)) + vlan_extend_size);
      //
      if (IP_I(pkt)->ip_v == IPVERSION){
          // prepare next.  iphdr copy(not decided yet)
          // nm_pkt_copy(ip_t, ph->req + sizeof(struct ether_header), ntohs(ip_t->ip_len));
          memmove(ip, IP_I(pkt), ntohs(IP_I(pkt)->ip_len));

      }else{
          // ipv6
          Logger::LOGERR("not implemented.");
          return(RETERR);
      }
      // decap-> recalculate packet length(shorter)
      size_t tlen = pkt->Length() - (size_t)(((char*)IP_I(pkt)) - ((char*)ip));
      pkt->Length(tlen);

      // tunneling main.
      {   TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(teid));
          if (tbl == NULL){
              return(0);
          }
          auto it = tbl->Find(teid, 0);
          if (it == tbl->End()){
              return(0);
          }
          // judge policy
          POLICER_TBL* ptbl = (POLICER_TBL*)process_param_->Get(ProcessParameter::TBL_POLICER, Misc::TeidGid(teid));
          if (ptbl == NULL){
              Logger::LOGERR("missing policy table.(%u)", teid);
              return(0);
          }
          // Recalculate policy event(set flags from BFD counter)
          // re calc policer.
          if ((*flag) & PROC_TRIGGER_1S_EVENT){
              Logger::LOGINF("PROC_TRIGGER_1S_EVENT(gtpu_decap.- marker) >> start.");
              recalc_counter_++;
              (*flag) &= ~PROC_TRIGGER_1S_EVENT;
              Logger::LOGINF("PROC_TRIGGER_1S_EVENT:%010u(gtpu_decap.- marker) << completed", recalc_counter_);
          }
          auto itb = ptbl->Find(teid, 1);
          if (itb == ptbl->End()){
              throw "exception. ";
          }
          auto itf = ptbl->Find(teid, 0);
          //
          if (itf == ptbl->End()){
              isred = 1;
          }else{
              // since last calculation, recalculation counter has advanced
              // => replenish token by number of seconds elaplsed.
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
                  itb->stat.epoch_w=recalc_counter_;
              }else if (pkt->Length() <= itf->excess_rate){
                  // yellow
                  itb->excess_rate = (itf->excess_rate - pkt->Length());
                  itb->stat.valid=LINKMAPSTAT_ON;
                  itb->stat.epoch_w=recalc_counter_;
              }else{
                  // encap(when internet -> sgw direction, do not drop)
                  isred = 1;
              }
              // update and swap.
              ptbl->SwapSide(teid);
          }
          // QOS
          if (isred){
              ip->ip_tos = (u_char)(IPTOS_DSCP(it->u.bitrate.qos) | IPTOS_ECN(0));
          }
      }
      //
      uint32_t decap_vlanid = process_param_->Get(ProcessParameter::USG_NEXT_HOP_VLANID);
      (*(((u_short*)(EH(pkt)+1))+0)) = htons(decap_vlanid);
      SwapMac(pkt);

      ip->ip_sum = 0;
      ip->ip_sum = Misc::Wrapsum(Misc::Checksum(ip, sizeof(struct ip), 0));
      if (Module::VERBOSE() > PCAPLEVEL_MIN){
          Logger::LOGINF("sum: %u", ip->ip_sum);
      }
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }


}; // namespace MIXIPGW_TOOLS
