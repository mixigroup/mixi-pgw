/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpu_validate_header.c
    @brief      validate gtpu header
*******************************************************************************
*******************************************************************************
    @date       created(13/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 13/nov/2017 
      -# Initial Version
******************************************************************************/

#include "gtpu_ext.h"
#include "pgw_ext.h"
/**
 gtpupacket  : validate header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet first address
 @return RETCD  0==success,0!=error
*/
RETCD gtpu_validate_header(packet_ptr pkt){
    gtpu_header_ptr gtp = (gtpu_header_ptr)pkt->data;
    //
    if (gtp->v1_flags.version != GTPU_VERSION_1){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid version(%d)", (INT)gtp->v1_flags.version);
        return(ERR);
    }
    if (gtp->v1_flags.proto != GTPU_PROTO_GTP){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid proto(%d)", (INT)gtp->v1_flags.proto);
        return(ERR);
    }
    if (gtp->v1_flags.npdu != GTPU_NPDU_OFF){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid npdu(%d)", (INT)gtp->v1_flags.npdu);
        return(ERR);
    }
    if (gtp->type != GTPU_ECHO_REQ && gtp->type != GTPU_G_PDU && gtp->type != GTPU_ECHO_RES){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid type(%d)", (INT)gtp->type);
        return(ERR);
    }
    return(OK);
}
