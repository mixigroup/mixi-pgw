/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpu_header.c
    @brief      gtpuaccess header
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


/**
 gtpupacket  : access header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
gtpu_header_ptr gtpu_header(packet_ptr pkt){
    if (!pkt){
        return(NULL);
    }
    return((gtpu_header_ptr)pkt->data);
}