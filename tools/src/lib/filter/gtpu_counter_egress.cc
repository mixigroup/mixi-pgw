//
// Created by mixi on 2017/04/25.
//

#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/filter/gtpu_counter_egress.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"
#include "lib/logger.hpp"

#include "lib/const/link.h"
#include "lib/const/counter.h"
#include "lib/const/gtpu.h"

namespace MIXIPGW_TOOLS{
  typedef DualBufferedLookupTable<link_t>           TEID_TBL;   // must be initialized teid table at netmap thread context.
  typedef DualBufferedLookupTable<teid_counter_t>   COUNT_TBL;  // for count table, netmap thread is write-side.
  
  struct pseudo{
    uint32_t saddr;
    uint32_t daddr;
    uint8_t  hold;
    uint8_t  pt;
    uint16_t len;
  };
  //
  int CounterEgress::OnInitFilter(void){
      bzero(calc_buffer_, sizeof(calc_buffer_));
      bzero(count_buffer_, sizeof(count_buffer_));

      return(RETOK);
  }
  //
  int CounterEgress::OnFilter(PktInterface* pkt ,int* flag){
      __be32    teid_in = ntohl(GTPU(pkt)->tid);
      // tunneling main.
      {   TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(teid_in));
          if (tbl == NULL){
              return(0);
          }
          auto it = tbl->Find(teid_in, 0);
          if (it == tbl->End()){
              return(0);
          }
          // next hop = sgw.
          GTPU(pkt)->tid = htonl(it->sgw_teid_u);
        //IP(pkt)->ip_src.s_addr = IP(pkt)->ip_dst.s_addr;
          uint32_t srcip = process_param_->Get(ProcessParameter::USG_NEXT_HOP_IPV4);
          IP(pkt)->ip_src.s_addr = srcip; 
          IP(pkt)->ip_dst.s_addr = it->sgw_ipv4;
          IP(pkt)->ip_sum = 0;

          auto ipsm = Misc::Checksum(IP(pkt), sizeof(struct ip), 0);
          IP(pkt)->ip_sum = Misc::Wrapsum(ipsm);
          UDP(pkt)->uh_sum = 0;
          // zero copy.
          SwapMac(pkt);
          (*flag) |= PROC_NEED_SEND;
          if (Module::VERBOSE() > PCAPLEVEL_MIN){
              Logger::LOGINF("egress.(%04x)", ipsm);
          }
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
              // Increments counter value
              COUNT_TBL* ptbl = (COUNT_TBL*)process_param_->Get(ProcessParameter::TBL_COUNTER_E, Misc::TeidGid(teid_in));
              if (ptbl == NULL){
                  return(0);
              }
              auto itb = ptbl->Find(teid_in, 1);
              if (itb == ptbl->End()){
                  throw "exception. ";
              }
              // update and swap.
              itb->size_egress = curr_size;
              itb->count_egress = count_buffer_[gcnt];
              itb->stat.epoch_w = (uint32_t)time(NULL);
              itb->stat.valid=LINKMAPSTAT_ON;
              ptbl->SwapSide(teid_in);
              ptbl->NotifyChange(teid_in);
              if (Module::VERBOSE() > PCAPLEVEL_MIN){
                  Logger::LOGINF("boundary 1MB(%u: " FMT_LLU ")", prev_size, curr_size);
              }
          }
      }
      return(0);
  }
}; // namespace MIXIPGW_TOOLS


