//
// Created by mixi on 2017/04/25.
//

#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/filter/gtpu_counter_ingress.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"
#include "lib/logger.hpp"

#include "lib/const/link.h"
#include "lib/const/counter.h"
#include "lib/const/gtpu.h"

namespace MIXIPGW_TOOLS{
  typedef DualBufferedLookupTable<link_t>           TEID_TBL;   // must be initialized teid table at netmap thread context
  typedef DualBufferedLookupTable<teid_counter_t>   COUNT_TBL;  // for count table, netmap thread is write-side.
  //
  int CounterIngress::OnInitFilter(void){
      bzero(calc_buffer_, sizeof(calc_buffer_));
      bzero(count_buffer_, sizeof(count_buffer_));
      return(RETOK);
  }
  //
  int CounterIngress::OnFilter(PktInterface* pkt ,int* flag){
      struct ip           *ip_i = IP(pkt);
      struct udphdr       *udp_i = UDP(pkt);
      gtpu_header_ptr     gtp =  GTPU(pkt);
      __be32              teid_in = ntohl(GTPU(pkt)->tid);
      //
      Logger::LOGDBG("route_ingress : %x", pkt->Length());
      if (gtp->u.v1_flags.version != GTPU_VERSION_1){ Logger::LOGERR("invalid gtp-u-v1 version"); return(RETERR); }
      if (gtp->u.v1_flags.proto != GTPU_PROTO_GTP){ Logger::LOGERR("invalid gtp-u-v1 protocol"); return(RETERR); }
      if (gtp->u.v1_flags.npdu != GTPU_NPDU_OFF){ Logger::LOGERR("invalid gtp-u-v1 npdu flag"); return(RETERR); }
      if (gtp->type != GTPU_G_PDU){ Logger::LOGERR("invalid gtp-u-v1 type"); return(RETERR); }
      if (teid_in == 0) { Logger::LOGERR("teid is zero."); return(RETERR);  }

      // tunneling main.
      {   TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(teid_in));
          if (tbl == NULL){
              return(0);
          }
          auto it = tbl->Find(teid_in, 0);
          if (it == tbl->End()){
              // return error
              Logger::LOGERR("response error indication.(%x)", teid_in);
              // response error indication.
              size_t  tlen = ((char*)gtp - (char*)EH(pkt)) + sizeof(gtpu_err_indication_t);
              gtpu_err_indication_ptr pind = (gtpu_err_indication_ptr)(((char*)gtp) + sizeof(gtpu_header_t) + (gtp->u.flags&GTPU_FLAGMASK?4:0));
              // swap
              struct in_addr tmp_addr = ip_i->ip_dst;
              ip_i->ip_dst = ip_i->ip_src;
              ip_i->ip_src = tmp_addr;
              ip_i->ip_len = htons(tlen - sizeof(struct ether_header));
              ip_i->ip_sum = 0;
              ip_i->ip_sum = Misc::Wrapsum(Misc::Checksum(ip_i, sizeof(*ip_i), 0));
              //
              uint16_t tmp_port = udp_i->uh_dport;
              udp_i->uh_dport = udp_i->uh_sport;
              udp_i->uh_sport = tmp_port;
              udp_i->uh_ulen = htons(tlen - sizeof(struct ether_header) - (GTPU_IPV4HL*4));
              udp_i->uh_sum  = 0;
              //
              pind->teid.type = GTPU_TEID_TYPE;
              pind->teid.val = gtp->tid;
              pind->peer.type = GTPU_PEER_ADDRESS;
              pind->peer.length = htons(4);
              pind->peer.val = tmp_addr.s_addr;
              //
              gtp->length  = htons(tlen - sizeof(struct ether_header) - (GTPU_IPV4HL*4) - sizeof(struct udphdr) - 4);
              pkt->Length(tlen);
          }else{
              // next hop = translater(decap).
            //ip_i->ip_dst.s_addr = MAKE_NEXT_HOP(ip_i->ip_dst.s_addr,FID_TRANS_I);
              uint32_t nexthop = process_param_->Get(ProcessParameter::USG_NEXT_HOP_IPV4);
              ip_i->ip_dst.s_addr = nexthop; 
              ip_i->ip_sum = 0;
              ip_i->ip_sum = Misc::Wrapsum(Misc::Checksum(ip_i, sizeof(*ip_i), 0));
          }
          // zero copy.
          SwapMac(pkt);
          (*flag) |= PROC_NEED_SEND;
      }
      uint64_t prev_size = 0, curr_size = 0;
      auto gcnt = Misc::TeidGcnt(teid_in);
      prev_size = calc_buffer_[gcnt];
      curr_size = (prev_size + pkt->Length());
      calc_buffer_[gcnt] = curr_size;
      count_buffer_[gcnt] ++;
      // 1MB boundary
      if (prev_size > 0 && curr_size > 0){
          if ((curr_size>>20) > (prev_size>>20)){
              COUNT_TBL* ptbl = (COUNT_TBL*)process_param_->Get(ProcessParameter::TBL_COUNTER_I, Misc::TeidGid(teid_in));
              if (ptbl == NULL){
                  return(0);
              }
              auto itb = ptbl->Find(teid_in, 1);
              if (itb == ptbl->End()){
                  throw "exception. ";
              }
              // update and swap.
              itb->size_ingress = curr_size;
              itb->count_ingress = count_buffer_[gcnt];
              itb->stat.valid=LINKMAPSTAT_ON;
              itb->stat.epoch_w = (uint32_t)time(NULL);
              ptbl->SwapSide(teid_in);
              ptbl->NotifyChange(teid_in);
              //
              if (Module::VERBOSE() > PCAPLEVEL_MIN){
                  Logger::LOGINF("boundary 1MB(%u: " FMT_LLU ")", prev_size, curr_size);
              }
          }
      }
      // Increment counter value
      return(RETOK);
  }
};
