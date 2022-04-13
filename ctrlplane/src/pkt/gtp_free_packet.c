/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_free_packet.c
    @brief      gtpcpacket  release
*******************************************************************************
*******************************************************************************
    @date       created(12/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 12/nov/2017 
      -# Initial Version
******************************************************************************/

#include "gtpc_ext.h"

extern U64  gtp_memory_status_count;
extern U64  gtp_memory_free_count;


/**
 gtpcpacket , release \n
 *******************************************************************************
 packet  release\n
 *******************************************************************************
 @param[in]    ppkt  packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_free_packet(gtp_packet_ptr* ppkt){
    static const U8 magic[4] = {0xde,0xad,0xc0,0xde};
    gtp_item_ptr p1,p2;
    if ((*ppkt) == NULL){
        return(ERR);
    }
    if (memcmp(((U8*)(*ppkt)) + sizeof(gtp_packet_t) + ETHER_MAX_LEN, magic, sizeof(magic)) != 0){
        // buffer over run?
        gtp_memory_status_count++;
    }
    gtp_memory_free_count++;

    p1 = TAILQ_FIRST(&(*ppkt)->items);
    while(p1 != NULL){
        p2 = TAILQ_NEXT(p1, link);
        free(p1);
        p1 = p2;
    }
    (*ppkt)->packet = NULL;
    free((*ppkt));
    (*ppkt) = NULL;
    //
    return(OK);
}
/**
 gtp packet, copy address \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   dpkt  dest packet 
 @param[in]   spkt  copy source : packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_copy_packet_addr(gtp_packet_ptr dpkt, gtp_packet_ptr spkt){
    dpkt->saddrlen = spkt->saddrlen;
    memcpy(&dpkt->saddr, &spkt->saddr, spkt->saddrlen);
    dpkt->caddrlen = spkt->caddrlen;
    memcpy(&dpkt->caddr, &spkt->caddr, spkt->caddrlen);
    return(OK);
}

