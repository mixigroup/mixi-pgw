//
// Created by mixi on 2017/04/25.
//
#ifndef MIXIPGW_TOOLS_LINK_H
#define MIXIPGW_TOOLS_LINK_H
#include "mixipgw_tools_def.h"

namespace MIXIPGW_TOOLS{
  // teid/ipv4 valid check with link
  typedef struct link {
      struct _stat{
          __be32 valid:1;
          __be32 type: 8;
          __be32 padd:23;
      }stat;
      __be32      sgw_teid_u;
      __be32      sgw_teid_c;
      __be32      sgw_ipv4;
      __be32      pgw_ipv4;
      union _w{
          struct _pgw_teid{
              __be32 gcnt:16;
              __be32 gid:8;
              __be32 sid:8;
          }pgw_teid;
          __be32 pgw_teid_uc;
      }w;
      //
      union _u {
          struct _bitrate{
              __be16      s5;
              __be16      sgi;
              __be16      qos;
              __be16      qci;
          }bitrate;
      }u;
  }__attribute__ ((packed)) link_t,*link_ptr;
  //
  static_assert(MAX_STRAGE_PER_GROUP%sizeof(uint64_t)==0, "need 64bit  alligned");
  static_assert(MAX_STRAGE_PER_GROUP%64==0,               "need 64byte alligned");
  static_assert(MAX_STRAGE_PER_GROUP%sizeof(link_t)==0,   "need 32byte alligned");
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_LINK_H
