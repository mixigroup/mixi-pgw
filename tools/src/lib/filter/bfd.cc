//
// Created by mixi on 2017/04/21.
//
#include "mixipgw_tools_def.h"
#include "lib/filter/bfd.hpp"
#include "lib/pktheader.hpp"
#include "lib/logger.hpp"
#include "lib/const/bfd.h"
namespace MIXIPGW_TOOLS{
  int Bfd::OnInitFilter(void){
      bfd_recv_counter_ = 0;
      return(RETOK);
  }
  int Bfd::OnFilter(PktInterface* pkt ,int* flag){
      SwapMac(pkt);
      //
      struct in_addr tmp_addr = IP(pkt)->ip_dst;
      IP(pkt)->ip_dst = IP(pkt)->ip_src;
      IP(pkt)->ip_src = tmp_addr;
      IP(pkt)->ip_ttl = 0xff;
      // 2017/05/26:
      // bfd notification every 3 sec after status up.
      // Raise token replenishment bit per notification.
      // event trigger(  every 1s:100ms x 10)
//    if ((bfd_recv_counter_++) > BFD_RPS){
          (*flag) |= PROC_TRIGGER_1S_EVENT;
//        bfd_recv_counter_ = 0;
//    }
      // BFD is not modified checksum.
      // ip->ip_sum = 0;
      // ip->ip_sum = wrapsum(checksum(ip, sizeof(*ip), 0));
      //
      UDP(pkt)->uh_sum = 0;
      //
      Logger::LOGDBG("(state : %u/your: %u/my  : %u/ cpuid:%u/ thread:%p)", BFD(pkt)->u.bit.state, ntohl(BFD(pkt)->your_discr), ntohl(BFD(pkt)->my_discr), getpid(), (void*)pthread_self());
      uint32_t tmp_discr = BFD(pkt)->your_discr;
      BFD(pkt)->your_discr = BFD(pkt)->my_discr;
      BFD(pkt)->my_discr = tmp_discr;
      uint32_t MYDSCR = 10000;
      //
      if (BFD(pkt)->u.bit.state == BFDSTATE_DOWN){
          if (!tmp_discr){
              BFD(pkt)->u.bit.state = BFDSTATE_INIT;
              BFD(pkt)->my_discr = MYDSCR;
          }else{
              BFD(pkt)->u.bit.state = BFDSTATE_DOWN;
              BFD(pkt)->my_discr = MYDSCR;
          }
      }else if (BFD(pkt)->u.bit.state == BFDSTATE_ADMINDOWN){
          BFD(pkt)->u.bit.state = BFDSTATE_DOWN;
      }else if (BFD(pkt)->u.bit.state == BFDSTATE_INIT){
          BFD(pkt)->u.bit.state = BFDSTATE_UP;
          BFD(pkt)->my_discr = MYDSCR;
      }else if (BFD(pkt)->u.bit.state == BFDSTATE_UP){
          BFD(pkt)->u.bit.state = BFDSTATE_UP;
          BFD(pkt)->my_discr = MYDSCR;
      }else{
          BFD(pkt)->u.bit.state = BFDSTATE_DOWN;
          BFD(pkt)->my_discr = MYDSCR;
          BFD(pkt)->your_discr = 0;
      }
      if (BFD(pkt)->u.bit.state == BFDSTATE_UP){
          MF_BFD_SET_UP((*flag));
      }else{
          MF_BFD_SET_DOWN((*flag));
      }
      (*flag) |= PROC_NEED_SEND;
      return(0);
  }
};// namespace MIXIPGW_TOOLS


