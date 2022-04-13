/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       enq.c
    @brief      enqueue packet to software ring
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
/**
 enqueue .\n
 *******************************************************************************
 enqueue to ring\n
 *******************************************************************************
 @param[in]     pnode   node object
 @param[in]     order   NODE_IPC_INGRESS or NODE_IPC_EGRESS
 @param[in]     packet  packet 
 @return RETCD  0==success,0!=error
*/
RETCD pgw_enq(node_ptr pnode, INT order, packet_ptr packet){
    // ingres ring
    if (order == INGRESS){
        pthread_mutex_lock(&pnode->ingress_queue_mtx);
        if (pnode->ingress_queue_count >= RING_SIZE_MAX){
            // set as error, when ring maximum buffer length was exceeded
            // TODO: forward number of dropped packets to error aggregation server. 
            pnode->ingress_queue_drops ++;
            if ((pnode->ingress_queue_drops % 100) == 0){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "ingress : drop..(%u)\n", (unsigned)pnode->ingress_queue_drops);
            }
        }else{
            pnode->ingress_queue_count ++;
            TAILQ_INSERT_TAIL(&pnode->ingress_queue, packet, link);
        }
        pthread_mutex_unlock(&pnode->ingress_queue_mtx);
    }else {
        pthread_mutex_lock(&pnode->egress_queue_mtx);
        if (pnode->egress_queue_count >= RING_SIZE_MAX){
            // set as error, when ring maximum buffer length was exceeded
            // TODO: forward number of dropped packets to error aggregation server.
            pnode->egress_queue_drops ++;
            if ((pnode->egress_queue_drops % 100) == 0){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "egress : drop..(%u)\n", (unsigned)pnode->egress_queue_drops);
            }
        }else{
            pnode->egress_queue_count ++;
            TAILQ_INSERT_TAIL(&pnode->egress_queue, packet, link);
        }
        pthread_mutex_unlock(&pnode->egress_queue_mtx);
    }
    return(OK);
}