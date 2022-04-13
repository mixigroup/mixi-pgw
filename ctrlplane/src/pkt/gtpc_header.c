/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_header.c
    @brief      gtpcaccess header
*******************************************************************************
*******************************************************************************
    @date       created(09/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/nov/2017 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"
#include "gtpc_ext.h"


/**
 gtpcpacket  : access header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return gtpc_header_ptr  gtpc-v2packet address , NULL==error
*/
gtpc_header_ptr gtpc_header(packet_ptr pkt){
    if (!pkt){
        return(NULL);
    }
    return((gtpc_header_ptr)pkt->data);
}
/**
 gtpcpacket  : access header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return gtpc_header_ptr  gtpc-v2packet address , NULL==error
*/
gtpc_header_ptr gtpc_header_(gtp_packet_ptr pkt){
    if (!pkt){
        return(NULL);
    }
    return((gtpc_header_ptr)pkt->packet);
}

/**
 gtpc-v1packet  : header access \n
 *******************************************************************************
 gtp-c version 1\n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return gtpc_v1_header_ptr  gtpc-v1packet  address  , NULL==error
*/
gtpc_v1_header_ptr gtpc_v1_header(packet_ptr pkt){
    if (!pkt){
        return(NULL);
    }
    return((gtpc_v1_header_ptr)pkt->data);
}

/**
 gtpc-v1packet  : access header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return gtpc_header_ptr  gtpc-v2packet  address  , NULL==error
*/
gtpc_v1_header_ptr gtpc_v1_header_(gtp_packet_ptr pkt){
    if (!pkt){
        return(NULL);
    }
    return((gtpc_v1_header_ptr)pkt->packet);
}
