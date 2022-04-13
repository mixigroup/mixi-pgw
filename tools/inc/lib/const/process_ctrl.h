//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_PROCESS_CTRL_H
#define MIXIPGW_TOOLS_PROCESS_CTRL_H
#include "mixipgw_tools_def.h"

namespace MIXIPGW_TOOLS{

  struct in6_addr;
// ctrl packet(128 octet)
  typedef struct process_ctrl{
      __be32      type;
      __be32      teid;
      __be32      lnkip;
      __be32      dstipv4;
      uint8_t     dstipv6[IPV6_LEN];
      __be32      dstport;
      union _u {
          struct _qos{
              uint32_t    commit_burst_size;
              uint32_t    commit_information_rate;
              uint32_t    excess_burst_size;
              uint32_t    excess_information_rate;
              uint8_t     qos[2];
              uint8_t     padding[74];
          }qos;
          uint8_t     padding[92];
      }u;

  }__attribute__ ((packed)) process_ctrl_t,*process_ctrl_ptr;
  //
  static_assert(sizeof(process_ctrl_t)==128,   "need 128byte alligned");
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_PROCESS_CTRL_H
