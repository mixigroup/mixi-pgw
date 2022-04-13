//
// Created by mixi on 2017/07/05.
//

#ifndef MIXIPGW_TOOLS_POLICY_H
#define MIXIPGW_TOOLS_POLICY_H


#include "mixipgw_tools_def.h"

/**
 * rewrite ip.dst_ipv4 -> vlan id.
 * + without vlan, not supported(do not extend ether header)
 */
//
namespace MIXIPGW_TOOLS{
  typedef struct policy{
      struct _stat{
          uint32_t    valid:1;
          uint32_t    padd00:15;
          uint32_t    linked_vlanid:16;
      }stat;
      //
      uint32_t    linked_ipv4;
  }policy_t, *policy_ptr;
  //
  static_assert(sizeof(policy_t)==8,   "need 8byte(policy_t)");
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_POLICY_H
