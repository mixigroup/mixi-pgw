/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       deq_burst.c
    @brief      dequeue packet from software ring
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
 dequeue burst.\n
 *******************************************************************************
 dequeue from ring\n
 *******************************************************************************
 @param[in]     pnode   node object
 @param[in]     order   NODE_IPC_INGRESS or NODE_IPC_EGRESS
 @param[in/out] packets packet 
 @param[in]     length  count of burst
 @return RETCD  0>exists , count of data, 0<=empty or error
*/
RETCD pgw_deq_burst(node_ptr pnode, INT order, packet_ptr packets[], U32 length){
    U32 ret = 0;
    packet_ptr  cur, curtmp;
    // ingress ring
    if (order == INGRESS){
        pthread_mutex_lock(&pnode->ingress_queue_mtx);
        TAILQ_FOREACH_SAFE(cur, &pnode->ingress_queue, link, curtmp) {
            packets[ret] = cur;
            ret ++;
            TAILQ_REMOVE(&pnode->ingress_queue, cur, link);
            if (ret >= length){
                break;
            }
        }
        pnode->ingress_queue_count -= (size_t)ret;
        pthread_mutex_unlock(&pnode->ingress_queue_mtx);
    }else {
        pthread_mutex_lock(&pnode->egress_queue_mtx);
        TAILQ_FOREACH_SAFE(cur, &pnode->egress_queue, link, curtmp) {
            packets[ret] = cur;
            ret ++;
            TAILQ_REMOVE(&pnode->egress_queue, cur, link);
            if (ret >= length){
                break;
            }
        }
        pnode->egress_queue_count -= (size_t)ret;
        pthread_mutex_unlock(&pnode->egress_queue_mtx);
    }
    return(ret);
}

