//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_BFD_H
#define MIXIPGW_TOOLS_BFD_H

#include "mixipgw_tools_def.h"

/*
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Vers |  Diag   |Sta|P|F|C|A|D|M|  Detect Mult  |    Length     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       My Discriminator                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Your Discriminator                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Desired Min TX Interval                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Required Min RX Interval                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 Required Min Echo RX Interval                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   An optional Authentication Section MAY be present:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Auth Type   |   Auth Len    |    Authentication Data...     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
 *
 *
 */
#define BFDDFLT_DEMANDMODE          false
#define BFDDFLT_DETECTMULT          ((uint8_t)3)
#define BFDDFLT_DESIREDMINTX        1000000
#define BFDDFLT_REQUIREDMINRX       1000000
#define BFDDFLT_UDPPORT             ((uint16_t)3784)

#define BFD_VERSION                 1
#define BFD_MINPKTLEN               24    /* Minimum length of control packet */
#define BFD_MINPKTLEN_AUTH          26    /* Minimum length of control packet with Auth section */
#define BFD_1HOPTTLVALUE            255
#define BFD_DOWNMINTX               1000000
#define BFD_HASHSIZE                251        /* Should be prime */
#define BFD_MKHKEY(val)             ((val) % BFD_HASHSIZE)
#define BFD_SRCPORTINIT             49142
#define BFD_SRCPORTMAX              65536

//
#define BFDSTATE_ADMINDOWN          0
#define BFDSTATE_DOWN               1
#define BFDSTATE_INIT               2
#define BFDSTATE_UP                 3
//
#define BFD_NODIAG                  0
#define BFDDIAG_DETECTTIMEEXPIRED   1
#define BFDDIAG_ECHOFAILED          2


#define BFDDIAG_NEIGHBORSAIDDOWN    3
#define BFDDIAG_FWDPLANERESET       4
#define BFDDIAG_PATHDOWN            5
#define BFDDIAG_CONCATPATHDOWN      6
#define BFDDIAG_ADMINDOWN           7
#define BFDDIAG_RCONCATPATHDOWNW    8

#define BFD_ON                      (1)
#define BFD_OFF                     (0)
#define BFD_RPS                     (10)    /* recieve bfd packet per sec */


namespace MIXIPGW_TOOLS{
  typedef struct bfd{
      union _h {
          struct _head{
              uint8_t  diag:5;
              uint8_t  vers:3;
          }head;
          uint8_t flags;
      }h;
      union _u {
          struct _bit{
              uint8_t  multipoint:1;
              uint8_t  demand:1;
              uint8_t  auth:1;
              uint8_t  cpi:1;
              uint8_t  final:1;
              uint8_t  poll:1;
              uint8_t  state:2;
          }bit;
          uint8_t flags;
      }u;
      uint8_t       detect_mult;
      uint8_t       length;
      uint32_t      my_discr;
      uint32_t      your_discr;
      uint32_t      min_tx_int;
      uint32_t      min_rx_int;
      uint32_t      min_echo_rx_int;
  }__attribute__ ((packed)) bfd_t,*bfd_ptr;

}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_BFD_H
