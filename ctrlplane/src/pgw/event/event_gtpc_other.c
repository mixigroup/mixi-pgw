/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_other.c
    @brief      gtpc : other receive event
*******************************************************************************
    - delete bearer\n
    - delete session\n
    - suspend notification\n
    - resume notification)\n
*******************************************************************************
    @date       created(28/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 28/nov/2017 
      -# Initial Version
******************************************************************************/
#include "../pgw.h"


/**
  event : GTPC other Request\n
 *******************************************************************************
 Rx -> gtpc other req\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_other_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst,ret;
    INT         m;
    packet_ptr  pkts[32] = {NULL};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpc_other_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
        for(m = 0;m < nburst;m++){
            ret = OK;
            if (gtpc_header(pkts[m])->v2_flags.version == GTPC_VERSION_2){
                if (gtpc_header(pkts[m])->type == GTPC_DELETE_SESSION_REQ){
                    ret = event_gtpc_delete_session_req(pkts[m], ext);
                }else if (gtpc_header(pkts[m])->type == GTPC_DELETE_BEARER_REQ){
                    ret = event_gtpc_delete_bearer_req(pkts[m], ext);
                }else if (gtpc_header(pkts[m])->type == GTPC_RESUME_NOTIFICATION){
                    ret = event_gtpc_resume_notification_req(pkts[m], ext);
                }else if (gtpc_header(pkts[m])->type == GTPC_SUSPEND_NOTIFICATION){
                    ret = event_gtpc_suspend_notification_req(pkts[m], ext);
                }
            }else{
                ret = event_gtpc_v1_any_req(pkts[m], ext);
            }
            pgw_free_packet(pkts[m]);
        }
        return(OK);
    }
    return(ERR);
}