//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_COUNTER_H
#define MIXIPGW_TOOLS_COUNTER_H
#include "mixipgw_tools_def.h"

namespace MIXIPGW_TOOLS{
  // teid counters(64 octet + type(4))
  typedef struct teid_counter {
      struct _stat{
          uint32_t    valid:1;
          uint32_t    epoch_w:16;
          uint32_t    padd00:15;
          uint32_t    padd01:32;
      }stat;
      __uint64_t  drop_ingress;
      __uint64_t  drop_egress;
      __uint64_t  size_ingress;
      __uint64_t  size_egress;
      __uint64_t  count_ingress;
      __uint64_t  count_egress;
      __uint64_t  start_usec;
      //
  }__attribute__ ((packed)) teid_counter_t,*teid_counter_ptr;

  static_assert(sizeof(teid_counter_t)%64==0,               "need 64byte alligned");
}; // namespace MIXIPGW_TOOLS
// teid counters type
#define TEID_COUNTER_DC         (0)
#define TEID_COUNTER_DROP_I     (1)     // count of drop (at invalid teid) ingress
#define TEID_COUNTER_DROP_E     (2)     // count of drop (at invalid teid) egress
#define TEID_COUNTER_SIZE_I     (3)     // after ip-packet size ingress
#define TEID_COUNTER_SIZE_E     (4)     // after ip-packet size egress
#define TEID_COUNTER_MAX        (5)



#endif //MIXIPGW_TOOLS_COUNTER_H
