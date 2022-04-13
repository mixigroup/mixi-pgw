//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_POLICER_H
#define MIXIPGW_TOOLS_POLICER_H

#include "mixipgw_tools_def.h"

/**
https://tools.ietf.org/html/rfc4115
 * policer ( Green/Yellow/Red )
 *
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

              Figure 1: Traffic Metering/Marking Algorithm

   In Figure 1, we has X(t-) and X(t+) to indicate the value of a
   parameter X right before and right after time t.
 *
 */
//
#define  POLICER_COLOR_GREEN    (0)
#define  POLICER_COLOR_YELLOW   (1)
#define  POLICER_COLOR_RED      (2)
//
namespace MIXIPGW_TOOLS{
  typedef struct policer{
      struct _stat{
          uint32_t    valid:1;
          uint32_t    padd00:16;
          uint32_t    padd01:15;
          uint32_t    epoch_w:32;
      }stat;
      //
      uint32_t    commit_rate;
      uint32_t    commit_burst_size;
      uint32_t    commit_information_rate;
      uint32_t    excess_rate;
      uint32_t    excess_burst_size;
      uint32_t    excess_information_rate;
  }policer_t, *policer_ptr;
  //
  static_assert(MAX_STRAGE_PER_GROUP%sizeof(policer_t)==0,   "need 32byte alligned(policer)");
}; // namespace MIXIPGW_TOOLS



#endif //MIXIPGW_TOOLS_POLICER_H
