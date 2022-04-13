//
// Created by mixi on 2017/04/25.
//


#include "mixipgw_tools_def.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/filter/gtpu_echo.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/misc.hpp"
#include "lib/const/gtpu.h"

namespace MIXIPGW_TOOLS{
  //
  int GtpuEcho::OnFilter(PktInterface* pkt ,int* flag){
      struct ether_header *eh = EH(pkt);
      struct ip *ip = IP(pkt);
      struct udphdr *udp = UDP(pkt);
      gtpu_header_ptr gtp =  GTPU(pkt);

      //
      struct in_addr tmp_addr = ip->ip_dst;
      ip->ip_dst = ip->ip_src;
      ip->ip_src = tmp_addr;
      ip->ip_sum = 0;
      ip->ip_sum = Misc::Wrapsum(Misc::Checksum(ip, sizeof(*ip), 0));
      //
      uint16_t tmp_port = udp->uh_dport;
      udp->uh_dport = udp->uh_sport;
      udp->uh_sport = tmp_port;
      udp->uh_sum   = 0;

      // echo response type.
      gtp->type = GTPU_ECHO_RES;

      SwapMac(pkt);
      (*flag) |= PROC_NEED_SEND;
      //
      return(0);
  }
}; // namespace MIXIPGW_TOOLS


