/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_alloc_packet.c
    @brief      gtpcpacket  allocate
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
#include "pgw_ext.h"

extern U64  gtp_memory_alloc_count;

/**
 gtp[c/u]packet  : generate \n
 *******************************************************************************
 packet allocate , gtpcpacket (combined gtpu header) in default\n
 *******************************************************************************
 @param[in]   ppkt  packet 
 @param[in]   type  packet type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_alloc_packet(gtp_packet_ptr* ppkt, U8 type){
    static const U8 magic[4] = {0xde,0xad,0xc0,0xde};
    gtpc_header_ptr gtpch;
    // set magic code on last 4 octet)
    (*ppkt) = (gtp_packet_ptr)malloc(sizeof(gtp_packet_t) + ETHER_MAX_LEN + sizeof(U32));
    if ((*ppkt) == NULL){
        return(ERR);
    }
    bzero((*ppkt), sizeof(gtp_packet_t) + ETHER_MAX_LEN + sizeof(U32));
    gtp_memory_alloc_count++;
    (*ppkt)->count  = 0;
    (*ppkt)->length = ETHER_MAX_LEN;
    (*ppkt)->packet = ((U8*)&((*ppkt)[1]));
    memcpy((((U8*)(*ppkt)) + sizeof(gtp_packet_t) + ETHER_MAX_LEN), magic, sizeof(magic));
    TAILQ_INIT(&(*ppkt)->items);
    // by type
    gtpch = (gtpc_header_ptr)(*ppkt)->packet;
    if (type == GTPC_ECHO_REQ || type == GTPC_ECHO_RES){
        (*ppkt)->offset = sizeof(gtpc_header_t) - 4;
        gtpch->length           = htons(4);
        gtpch->v2_flags.teid = GTPC_TEID_OFF;
    }else{
        (*ppkt)->offset = sizeof(gtpc_header_t);
        gtpch->length           = htons(8);
        gtpch->v2_flags.teid = GTPC_TEID_ON;
    }
    gtpch->v2_flags.piggy   = GTPC_PIGGY_OFF;
    gtpch->v2_flags.version = GTPC_VERSION_2;
    gtpch->type             = type;
    //
    return(OK);
}
/**
 gtpv1packet  : generate \n
 *******************************************************************************
 packet allocate , gtpcpacket (combined gtpu header) in default\n
 *******************************************************************************
 @param[in]   ppkt  packet 
 @param[in]   type  packet type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_v1_alloc_packet(gtp_packet_ptr* ppkt, U8 type){
    static const U8 magic[4] = {0xde,0xad,0xc0,0xde};
    gtpc_v1_header_ptr gtpch;
    // set magic code on last 4 octet
    (*ppkt) = (gtp_packet_ptr)malloc(sizeof(gtp_packet_t) + ETHER_MAX_LEN + sizeof(U32));
    if ((*ppkt) == NULL){
        return(ERR);
    }
    bzero((*ppkt), sizeof(gtp_packet_t) + ETHER_MAX_LEN + sizeof(U32));
    gtp_memory_alloc_count++;
    (*ppkt)->count  = 0;
    (*ppkt)->length = ETHER_MAX_LEN;
    (*ppkt)->packet = ((U8*)&((*ppkt)[1]));
    memcpy((((U8*)(*ppkt)) + sizeof(gtp_packet_t) + ETHER_MAX_LEN), magic, sizeof(magic));
    TAILQ_INIT(&(*ppkt)->items);
    // by type
    gtpch = (gtpc_v1_header_ptr)(*ppkt)->packet;
    (*ppkt)->offset = sizeof(gtpc_v1_header_t);
    gtpch->length   = htons(0);
    gtpch->type     = type;
    gtpch->f.gtpc_v1_flags.version      = GTPC_VERSION_1;
    gtpch->f.gtpc_v1_flags.sequence     = GTPC_V1_SEQNUM_YES;
    gtpch->f.gtpc_v1_flags.npdu         = GTPC_V1_NPDU_NO;
    gtpch->f.gtpc_v1_flags.extension    = GTPC_V1_EXTENSION_NO;
    gtpch->f.gtpc_v1_flags.proto        = GTPC_V1_PT_GTP;
    gtpch->f.gtpc_v1_flags.reserve      = 0  ;
    //
    return(OK);
}

/**
 gtp packet : generate from payload \n
 *******************************************************************************
 packet allocate(gtp[c/u] general) from payload\n
 *******************************************************************************
 @param[in]   ppkt  packet 
 @param[in]   type  packet type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_alloc_packet_from_payload(gtp_packet_ptr* ppkt, U8* payload, U16 len){
    gtpc_header_ptr gtpch;
    packet_t        pkt;
    U16             offset,cnt;
    //
    if (!payload || !len){
        return(ERR);
    }
    pkt.data = payload;
    pkt.datalen = len;
    gtpch = (gtpc_header_ptr)payload;

    if (gtpc_validate_header(&pkt) != OK){
        return(ERR);
    }
    // payload length and packet format  must be equal.
    if ((ntohs(gtpch->length) + 4) != len){
        return(ERR);
    }
    if ((offset = gtpc_type_headerlen(gtpch->type)) > len){
        return(ERR);
    }
    // allocate parse object
    if (gtpc_alloc_packet(ppkt, gtpch->type) != OK){
        return(ERR);
    }
    // only header 
    if (offset == len){
        return(OK);
    }
    // start parse
    for(cnt = 0; offset < len && cnt < GTPC_CAPABILITY_ITEM_COUNT; cnt ++){
        gtpc_comm_header_ptr pitm = (gtpc_comm_header_ptr)(payload + offset);
        uint16_t hlen = ntohs(pitm->length) + sizeof(gtpc_comm_header_t);
        //
        if (gtpc_append_item((*ppkt), pitm->type, (payload + offset), hlen) != OK){
            break;
        }
        offset += hlen;
        //
        if (offset > len){
            // case in malformed item.
            PGW_LOG(PGW_LOG_LEVEL_ERR, "malformed ...(%d, %d, %d)GtpcPkt::attach", offset, len, hlen);
            return(ERR);
        }else if (offset == len){
            return(OK);
        }
    }
    gtpc_free_packet(ppkt);
    //
    return(ERR);
}
/**
 convert gtp packet -> generic packet\n
 *******************************************************************************
 gtp packet(gtp_packet_ptr) size = ETHER_FRAME
 \n
 *******************************************************************************
 @param[in]   pkt  packet 
 @return packet_ptr generic packet 
*/
packet_ptr gtpc_convert_to_packet(gtp_packet_ptr pkt){
    packet_ptr conv;
    // no - IP flagment 
    // TODO: MTU < 1500? limit with tap interface
    // available convert gtp_packet -> packet direction
    if (pgw_alloc_packet(&conv, pkt->offset) != OK){
        return(NULL);
    }
    memcpy(conv->data, pkt->packet, pkt->offset);
    conv->datalen = pkt->offset;

    conv->saddrlen = pkt->saddrlen;
    memcpy(&conv->saddr, &pkt->saddr, pkt->saddrlen);
    conv->caddrlen = pkt->caddrlen;
    memcpy(&conv->caddr, &pkt->caddr, pkt->caddrlen);
    //
    gtpc_free_packet(&pkt);
    //
    return(conv);
}

/**
 gtp packet : hex output \n
 *******************************************************************************
 print out hex memory status\n
 *******************************************************************************
 @param[in] pkt   packet 
*/
void gtpc_packet_print(gtp_packet_ptr pkt){
#ifndef __DEBUG_PACKET__
    return;
#endif
    U16 n;
    if (PGW_LOG_LEVEL < PGW_LOG_LEVEL_DBG){
        return;
    }
    if (!pkt || !pkt->packet || !pkt->length){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "pkt is null.\n");
        return;
    }
    fprintf(stderr, "========\n");
    fprintf(stderr, "len[%u] offset[%u]\n", pkt->length, pkt->offset);
    fprintf(stderr, "========\n");
    for(n = 0;n < pkt->offset;n++){
        if (n%4==0) { fprintf(stderr, " "); }
        if (n%32==0) { fprintf(stderr, "\n"); }
        fprintf(stderr, "%02x" , (U8)pkt->packet[n]);
    }
    fprintf(stderr, "\n--------\n");
    for(n = (pkt->length);n < (pkt->length + sizeof(U32));n++){
        fprintf(stderr, "%02x" , (U8)pkt->packet[n]);
    }
    fprintf(stderr, "\n========\n");
}
