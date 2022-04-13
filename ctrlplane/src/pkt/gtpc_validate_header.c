/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_validate_header.c
    @brief      validate gtpc header
*******************************************************************************
*******************************************************************************
    @date       created(08/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 07/nov/2017 
      -# Initial Version
******************************************************************************/

#include "gtpc_ext.h"
#include "pgw_ext.h"

/**
 gtpcpacket  : validate header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet first address
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_validate_header(packet_ptr pkt){
    gtpc_header_ptr gtp = (gtpc_header_ptr)pkt->data;
    size_t offset;
    //
    if (ntohs(gtp->length) < sizeof(gtpc_comm_header_t)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid header length.(%d)\n", ntohs(gtp->length));
        return(ERR);
    }
    if (gtp->v2_flags.version == GTPC_VERSION_2){
        if (gtp->v2_flags.piggy != GTPC_PIGGY_OFF){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "unsupported intercace(%u,%u)\n",gtp->v2_flags.version,  gtp->v2_flags.piggy);
            return(ERR);
        }
    }else if (gtp->v2_flags.version == GTPC_VERSION_1){
        return(OK);
    }
    if (!gtpc_type_issupported(gtp->type)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "unsupported type(%d)\n", gtp->type);
        return(ERR);
    }
    offset = gtpc_type_headerlen(gtp->type);
    if (offset > sizeof(gtpc_header_t)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid length(%d, %p, %d)", (INT)offset, gtp , ntohs(gtp->length));
        return(ERR);
    }
    return(OK);
}

/**
 gtpcpacket  : validate header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet first address
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_validate_header_(gtp_packet_ptr pkt){
    packet_t    packet;
    packet.datalen  = pkt->offset;
    packet.data     = pkt->packet;
    return(gtpc_validate_header(&packet));
}
