/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       start_parallel.c
    @brief      start parallel nodes
*******************************************************************************
*******************************************************************************
    @date       created(09/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 network team. Released Under the MIT license
*******************************************************************************
    @par        History
    - 09/nov/2017 
      -# Initial Version
******************************************************************************/

#include "node_ext.h"
#include "pgw_ext.h"
#include "gtpc_ext.h"
#include <mysql.h>

/**
 start parallel nodes thread func.\n
 *******************************************************************************
 start parallel rings, database accessor\n
 assign 2 rings(up, down) to each thread\n
 *******************************************************************************
 @param[in]     pinst     instance address
 @return RETCD  0==success,0!=error
*/
RETCD node_start_paralell(handle_ptr pinst){
    PTR val = NULL;
    INT len  = 0;
    U32 n;
    //
    if (!pinst){
        return(ERR);
    }
    //
    if (pgw_get_property(pinst, NODE_CNT, &val, &len) != OK){
        return(ERR);
    }
    if ((pinst->nodecnt = (*(U32*)val)) > MAX_DSTCNT){
        return(ERR);
    }
    pinst->flags = 0;
    // allocate node, those connect to proxy-server with software ring
    for(n = 0;n < pinst->nodecnt;n++){
        PGW_LOG(PGW_LOG_LEVEL_INF, "allocate node..(%p : %u)\n", (void*)pthread_self(), n);
        node_ptr pnode = (node_ptr)malloc(sizeof(node_t));
        if (pnode == NULL) {
            pgw_panic("not enough memocy (%d : %s)\n", errno, strerror(errno));
        }
        bzero(pnode, sizeof(*pnode));
        pnode->index        = n;
        pnode->halt         = 0;
        pnode->tocounter    = 0;
        pnode->handle       = pinst;
        pnode->next         = pinst->node;
        pinst->node         = pnode;
        pinst->nodes[n]     = pnode;

        //
        if (pthread_mutex_init(&pnode->ingress_queue_mtx, NULL) != 0 ||
                pthread_mutex_init(&pnode->egress_queue_mtx, NULL) != 0){
            pgw_panic("pthread_mutex_init (%d : %s)\n", errno, strerror(errno));
        }
        pnode->ingress_queue_count = 0;
        pnode->ingress_queue_drops = 0;
        pnode->egress_queue_count = 0;
        pnode->egress_queue_drops = 0;
        TAILQ_INIT(&pnode->ingress_queue);
        TAILQ_INIT(&pnode->egress_queue);
        pnode->peers_table = NULL;

        // start thread at every node
        if (pthread_create(&pnode->thread, NULL, node_thread, pnode)) {
            PGW_LOG(PGW_LOG_LEVEL_INF, "failed. pthread_create(%d: %s) .. node\n", errno, strerror(errno));
            return(ERR);
        }
    }
    pinst->flags |= ON;
    return(OK);
}
