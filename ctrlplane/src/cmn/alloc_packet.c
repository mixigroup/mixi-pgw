/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       free_packet.c
    @brief      release packet
*******************************************************************************
*******************************************************************************
    @date       created(08/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 08/nov/2017
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"

extern U64  pgw_memory_alloc_count;
extern pthread_mutex_t  pgw_memory_alloc_count_mtx;

/**
 allocate packet.\n
 *******************************************************************************
 allocate 1 packet\n
 *******************************************************************************
 @param[in/out] packet  packet 
 @param[in]     length  data length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_alloc_packet(packet_ptr* packet, U32 length){
    // Use contiguous memory regions for packet header and payload.
    (*packet) = (packet_ptr)malloc(sizeof(packet_t) + length);
    if (!(*packet)){
        return(ERR);
    }
    pthread_mutex_lock(&pgw_memory_alloc_count_mtx);
    pgw_memory_alloc_count++;
    pthread_mutex_unlock(&pgw_memory_alloc_count_mtx);
    bzero((*packet), sizeof(packet_t) + length);
    (*packet)->data = (U8*)&(*packet)[1];
    (*packet)->datalen = length;

    return(OK);
}
/**
  swap packet address.\n
 *******************************************************************************
 swap packet address\n
 *******************************************************************************
 @param[in]     packet  packet 
 @return RETCD  0==success,0!=error
 */
RETCD pgw_swap_address(packet_ptr packet){
    struct sockaddr_in  tmp_addr;
    ssize_t             tmp_addrlen;

    tmp_addr = packet->saddr;
    tmp_addrlen = packet->saddrlen;

    packet->saddr = packet->caddr;
    packet->saddrlen = packet->caddrlen;

    packet->caddr = tmp_addr;
    packet->caddrlen = tmp_addrlen;

    return(OK);
}
/**
 duplicate packet.\n
 *******************************************************************************
 duplicate 1 packet\n
 *******************************************************************************
 @param[in/out] dst     outputpacket 
 @param[in]     src     input packet 
 @return RETCD  0==success,0!=error
*/
RETCD pgw_duplicate_packet(packet_ptr* dst, const packet_ptr src){
    if (pgw_alloc_packet(dst, src->datalen) != OK){
        return(ERR);
    }
    // copy header region
    (*dst)->datalen = src->datalen;
    memcpy(&((*dst)->saddr), &src->saddr, sizeof(struct sockaddr_in));
    (*dst)->saddrlen = src->saddrlen;
    memcpy(&((*dst)->caddr), &src->caddr, sizeof(struct sockaddr_in));
    (*dst)->caddrlen = src->caddrlen;

    // copy payload region
    memcpy((*dst)->data, src->data, src->datalen);

    return(OK);
}




