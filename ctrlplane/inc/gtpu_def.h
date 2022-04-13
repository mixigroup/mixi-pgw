/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpu_def.h
    @brief      mixi_pgw_ctrl_plane gtpu define
*******************************************************************************
*******************************************************************************
    @date       created(13/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 13/nov/2017 
      -# Initial Version
******************************************************************************/
#ifndef MIXI_PGW_GTPU_H
#define MIXI_PGW_GTPU_H

#include "pgw_def.h"

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


typedef struct gtpu_header {
    struct _v1_flags{
        U8  npdu:1;
        U8  sequence:1;
        U8  extension:1;
        U8  reserve:1;
        U8  proto:1;
        U8  version:3;
    }v1_flags;
    U8      type;
    U16     length;
    U32     tid;
} __attribute__ ((packed)) gtpu_header_t,*gtpu_header_ptr;

// tunnel endpoint identifier data i
typedef struct gtpu_teid_i{
    U8      type;
    U32     val;
}__attribute__ ((packed)) gtpu_teid_i_t,*gtpu_teid_i_ptr;

// gtpu peer address
typedef struct gtpu_peer_address{
    U8      type;
    U16     length;
    U32  val;
}__attribute__ ((packed)) gtpu_peer_address_t,*gtpu_peer_address_ptr;

// Error Indication
typedef struct gtpu_err_indication{
    gtpu_teid_i_t         teid;
    gtpu_peer_address_t   peer;
}__attribute__ ((packed)) gtpu_err_indication_t,*gtpu_err_indication_ptr;


#endif //MIXI_PGW_GTPU_H
