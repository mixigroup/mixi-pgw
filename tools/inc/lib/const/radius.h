//
// Created by mixi on 2017/06/13.
//

#ifndef MIXIPGW_TOOLS_RADIUS_H
#define MIXIPGW_TOOLS_RADIUS_H

#include "mixipgw_tools_def.h"

/*
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Code      |  Identifier   |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                         Authenticator                         |
   |                                                               |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Attributes ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-
 *
 *
 */
#define RADIUS_CODE_ACCESS_REQUEST       (1)
#define RADIUS_CODE_ACCESS_ACCEPT        (2)
#define RADIUS_CODE_ACCESS_REJECT        (3)
#define RADIUS_CODE_ACCOUNTING_REQUEST   (4)
#define RADIUS_CODE_ACCOUNTING_RESPONSE  (5)


#define RADIUS_CONTENT_TYPE_USER_NAME               (1)
#define RADIUS_CONTENT_TYPE_NAS_IP_ADDRESS          (4)
#define RADIUS_CONTENT_TYPE_NAS_PORT                (5)
#define RADIUS_CONTENT_TYPE_SERVICE_TYPE            (6)
#define RADIUS_CONTENT_TYPE_FRAMED_IP_ADDRESS       (8)
#define RADIUS_CONTENT_TYPE_CALLING_STATION_ID      (31)
#define RADIUS_CONTENT_TYPE_ACCT_STATUS_TYPE        (40)
#define RADIUS_CONTENT_TYPE_NAS_IPV6_ADDRESS        (95)
#define RADIUS_CONTENT_TYPE_FRAMED_IPV6_PREFIX      (97)

#define RADIUS_CONTENT_TYPE_VENDOR                  (26)
#define RADIUS_CONTENT_TYPE_VENDOR_3GPP_PDP_TYPE    (3)

#define RADIUS_3GPP_VENDOR_CODE_PDP_IPV4            (0)
#define RADIUS_3GPP_VENDOR_CODE_PDP_PPP             (1)
#define RADIUS_3GPP_VENDOR_CODE_PDP_IPV6            (2)
#define RADIUS_3GPP_VENDOR_CODE_PDP_IPV4V6          (3)


#define RADIUS_DUPLICATE_DELAY      (1)
#define RADIUS_BUFFER_SIZE_MIN      (sizeof(radius_header_t))
#define RADIUS_BUFFER_SIZE_MAX      (4096)

#define RADIUS_MAKE_VENDOR_CODE(c)  ((((uint32_t)(RADIUS_CONTENT_TYPE_VENDOR<<16))&0xFFFF0000)|(((uint32_t)c)&0x0000FFFF))
#define RADIUS_VENDOR_VSA_CODE(c)   (((uint32_t)c)&0x0000FFFF)
namespace MIXIPGW_TOOLS{
  typedef struct radius_header{
      uint8_t   code;
      uint8_t   identifier;
      uint16_t  len;
      uint8_t   auth[16];
  }__attribute__ ((packed)) radius_header_t,*radius_header_ptr;


  typedef struct radius_link {
      uint64_t  key;
      // ipaddress
      __be32    ipv4;
      __be32    ipv6[4];
      __be32    nasipv;

      struct _stat{
          __be32 valid :1;
          __be32 type  :8;
          __be32 active:1;
          __be32 expire:1;
          __be32 ipv:3;     // 1=ipv4,2=ipv6,3=both
          __be32 padd :18;
      }stat;
  }__attribute__ ((packed)) radius_link_t,*radius_link_ptr;
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_RADIUS_H
