//
// Created by mixi on 2017/04/25.
//

#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/filter/xepc_ctrl.hpp"
#include "lib/pktheader.hpp"
#include "lib/logger.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"

#include "lib/const/link.h"
#include "lib/const/policer.h"
#include "lib/const/process_ctrl.h"

namespace MIXIPGW_TOOLS{
  typedef DualBufferedLookupTable<link_t>       TEID_TBL;       // must be initialized teid table at netmap thread context.
  typedef DualBufferedLookupTable<policer_t>    POLICER_TBL;    // must be initialized policy table at arbitor thread context.
  //
  int EpcCtrl::OnFilter(PktInterface* pkt ,int* flag){
      process_ctrl_ptr pctl = (process_ctrl_ptr)UDPP(pkt);
      const uint16_t rlen = (sizeof(struct ether_header)+LEN_VLAN(EH(pkt))+(IP(pkt)->ip_hl * 4)+sizeof(struct udphdr)+sizeof(process_ctrl_t));
      //
      if (pkt->Length() != rlen){
          Logger::LOGERR("invalid length(%d:%u)%u,%u,%u,%u,%u",
                         pkt->Length(),
                         rlen,
                         sizeof(struct ether_header),
                         LEN_VLAN(EH(pkt)),
                         IP(pkt)->ip_hl,
                         sizeof(struct udphdr),
                         sizeof(process_ctrl_t)

          );
          return(0);
      }
      if (pctl->type & (PROCCTRL_INSERT_ITM | PROCCTRL_DELETE_ITM)){
          link_t tl;
          memset(&tl, 0, sizeof(tl));
          tl.pgw_ipv4 = pctl->lnkip;
          tl.w.pgw_teid_uc = pctl->teid;
          tl.sgw_ipv4 = pctl->dstipv4;
          memcpy(&tl.u.bitrate.qos, pctl->u.qos.qos, 2);
          //
          TEID_TBL* tbl = (TEID_TBL*)process_param_->Get(ProcessParameter::TBL_TEID, Misc::TeidGid(ntohl(pctl->teid)));
          if (tbl == NULL){
              return(0);
          }
          /*
           * DualBufferedLookupTable<link_t> is lock-free with 2 threads
           * (both read and write)
           *  Netmap thread is only read side and cannot write in this context.
           */
          if (pctl->type & PROCCTRL_INSERT_ITM){
              // tbl->Add(pctl->teid, &tl, 1);
          }else{
              // tbl->Del(pctl->teid, 1);
          }
      }else if (pctl->type & PROCCTRL_BASERATE || pctl->type & PROCCTRL_TRGTRATE){
          /*
           * DualBufferedLookupTable<policer_t> is lock-free as well.
           * Writable on netmap thread-side(in this context)
           * Only 1 item can be updated in single lock-free sequence.
           * Ensure high parallel performance by applying protocol
           *    that requires reading between updates.
           */
          POLICER_TBL* tbl = (POLICER_TBL*)process_param_->Get(ProcessParameter::TBL_POLICER, Misc::TeidGid(ntohl(pctl->teid)));
          if (tbl == NULL){
              return(0);
          }
          // find [back/front] side.
          auto it = tbl->Find(pctl->teid, 0);
          auto itbk = tbl->Find(pctl->teid, 1);
          if (it != tbl->End() && itbk != tbl->End()){
              itbk->commit_burst_size         = pctl->u.qos.commit_burst_size;
              itbk->commit_information_rate   = pctl->u.qos.commit_information_rate;
              itbk->excess_burst_size         = pctl->u.qos.excess_burst_size;
              itbk->excess_information_rate   = pctl->u.qos.excess_information_rate;
              // re calc.
              itbk->commit_rate = MIN(it->commit_burst_size, it->commit_rate + it->commit_information_rate);
              itbk->excess_rate = MIN(it->excess_burst_size , it->excess_rate + it->excess_information_rate);
              itbk->stat.valid  = LINKMAPSTAT_ON;
              //
              tbl->SwapSide(pctl->teid);
          }
      }
      SwapMac(pkt);
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }
};// namespace MIXIPGW_TOOLS
