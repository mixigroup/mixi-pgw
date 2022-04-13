/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       stop.c
    @brief      stop PGW sytem
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 07/nov/2017 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"

/**
 stop instance.\n
 *******************************************************************************
 stop client library\n
 *******************************************************************************
 @param[in]     pinst     instance address
 @param[in]     callback  callback
 @param[in]     userdata  callback user data
 @return RETCD  0==success,0!=error
*/
RETCD pgw_stop(handle_ptr pinst, pgw_callback callback, PTR userdata){
#ifndef __TESTING__
    server_ptr pserver = NULL;
#endif
    node_ptr   pnode;

    PGW_LOG(PGW_LOG_LEVEL_INF, "pgw_stop\n");

    if (!pinst){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "pgw_stop:instance is null\n");
        return(ERR);
    }
#ifndef __TESTING__
    pserver = pinst->server_gtpc;
    if (!pserver){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "pgw_stop:server is null - gtpc\n");
        return(ERR);
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "gtpc server ..wait..(%p)\n", (void*)pinst->server_gtpc);
    // wait for (gtpc)udp server stopped
    pserver->halt++;
    pthread_join(pserver->thread, NULL);

    pserver = pinst->server_gtpu;
    if (!pserver){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "pgw_stop:server is null - gtpu\n");
        return(ERR);
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "gtpu server ..wait..(%p)\n", (void*)pinst->server_gtpu);
    // wait for (gtpu)udp server stopped
    pserver->halt++;
    pthread_join(pserver->thread, NULL);
#endif
    // wait for node stopped
    for (pnode = pinst->node; pnode!=NULL; pnode = pnode->next) {
        PGW_LOG(PGW_LOG_LEVEL_INF, "node(%u) ..wait..\n", pnode->index);
        pnode->halt++;
        pthread_join(pnode->thread, NULL);
    }
    return(OK);
}