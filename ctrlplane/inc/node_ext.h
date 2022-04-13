/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       node_ext.h
    @brief      mixi_pgw_ctrl_plane node c function define, common header
*******************************************************************************
*******************************************************************************
    @date       created(09/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 09/nov/2017 
      -# Initial Version
******************************************************************************/
#ifndef MIXI_PGW_NODE_EXT_H
#define MIXI_PGW_NODE_EXT_H

#include "pgw_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 start parallel nodes thread func.\n
 *******************************************************************************
 start parallel rings, database accessor\n
 assign 2 rings(up, down) to each thread\n
 *******************************************************************************
 @param[in]     pinst     instance address
 @return RETCD  0==success,0!=error
*/
RETCD node_start_paralell(handle_ptr pinst);
/**
 node thread.\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     arg     thread args
 @return void* no supported
*/
void* node_thread(void* arg);




#ifdef __cplusplus
}
#endif


#endif //MIXI_PGW_NODE_EXT_H
