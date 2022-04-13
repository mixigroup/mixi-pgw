//
// Created by mixi on 2017/04/25.
//
#ifndef LTEECP_DATAPLANE_DEF_H_H
#define LTEECP_DATAPLANE_DEF_H_H

#include "mixipgw_tools_def.h"

#define GTPU_VERSION_1                          (1)
#define GTPU_PROTO_GTP                          (1)
#define GTPU_NPDU_OFF                           (0)
#define GTPU_NPDU_ON                            (1)
#define GTPU_ECHO_REQ                           (1)
#define GTPU_ECHO_RES                           (2)
#define GTPU_ERR_IND                            (26)
#define GTPU_HEAD_NTFY                          (31)
#define GTPU_G_PDU                              (255)
#define GTPU_FLAGMASK                           (0x07)
#define GTPU_IPV4HL                             (5)

#define GTPU_EXTEND_0                           (0)
#define GTPU_EXTEND_1                           (1)
#define GTPU_SEQ_0                              (0)
#define GTPU_SEQ_1                              (1)
#define GTPU_TYPE_RECOVERY                      14
#define GTPU0_PORT	                           3386
#define GTPU1_PORT	                           2152
#define GTPU1_F_NPDU                           0x01
#define GTPU1_F_SEQ	                           0x02

#define GTPU1_F_EXTHDR	                       0x04
#define GTPU1_F_MASK	                       0x07

#define GTPU_TEID_TYPE                         16
#define GTPU_PEER_ADDRESS                      133


namespace MIXIPGW_TOOLS{
  typedef struct gtpu_header {    /* According to 3GPP TS 29.060. */
      union _u
      {
          struct _v1_flags{
              __u8 npdu:1;
              __u8 sequence:1;
              __u8 extension:1;
              __u8 reserve:1;
              __u8 proto:1;
              __u8 version:3;
          }v1_flags;
          __u8 flags;
      }u;
      __u8        type;
      __be16      length;
      __be32      tid;
  } __attribute__ ((packed)) gtpu_header_t,*gtpu_header_ptr;

  // encap header
  typedef struct encap_header {
      struct ether_header eh_enc;
      __u8                vlan[4];
      struct ip           ip_enc;
      struct udphdr       udp_enc;
      struct gtpu_header  gtp_enc;
//    uint32_t            seq;
  }__attribute__ ((packed)) encap_header_t, *encap_header_ptr;

  // tunnel endpoint identifier data i
  typedef struct gtpu_teid_i{
      __u8        type;
      __be32      val;
  }__attribute__ ((packed)) gtpu_teid_i_t,*gtpu_teid_i_ptr;
  // gtpu peer address
  typedef struct gtpu_peer_address{
      __u8        type;
      __be16      length;
      __be32      val;
  }__attribute__ ((packed)) gtpu_peer_address_t,*gtpu_peer_address_ptr;

  // Error Indication
  typedef struct gtpu_err_indication{
      gtpu_teid_i_t         teid;
      gtpu_peer_address_t   peer;
  }__attribute__ ((packed)) gtpu_err_indication_t,*gtpu_err_indication_ptr;

}; // namespace MIXIPGW_TOOLS

#endif //LTEECP_DATAPLANE_DEF_H_H
