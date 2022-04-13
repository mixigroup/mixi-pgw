/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpu_ext.h
    @brief      mixi_pgw_ctrl_plane gtpu access , c function define, common header
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
#ifndef MIXI_PGW_GTPU_EXT_H
#define MIXI_PGW_GTPU_EXT_H

#include "gtpu_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 gtpupacket  : header access \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
gtpu_header_ptr gtpu_header(packet_ptr pkt);

/**
 gtpupacket  : validate header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpu_validate_header(packet_ptr pkt);


#ifdef __cplusplus
}
#endif


#endif //MIXI_PGW_GTPU_EXT_H
