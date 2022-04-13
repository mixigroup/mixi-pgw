/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpu_echo.c
    @brief      gtpu echoevent
*******************************************************************************
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
  event : GTPU Echo Request Core\n
 *******************************************************************************
 Rx -> gtpu echo [request/ response]\n
 - request:\n
 convert to response, send to tx via software ring
 - response:\n
 update status by dst[ip/port],src[ip/port]\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpu_echo_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m;
    packet_ptr  pkt = NULL;
    packet_ptr  pkts[32] = {NULL};
    char        sql[1024] = {0};
    char        sip[64] = {0}, dip[64] = {0};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        for(m = 0;m < nburst;m++){
            if (gtpu_header(pkts[m])->type == GTPU_ECHO_REQ){
                // response to request
                if (pgw_duplicate_packet(&pkt, pkts[m]) != OK){
                    pgw_panic("failed. memory allocate.");
                }
                pgw_swap_address(pkt);
                if (pgw_enq(pnode, EGRESS, pkt) != OK){
                    pgw_panic("failed. enq(gtpu echo req).");
                }
                //
                PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpu_echo_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
                // reply converted GTPU Echo Request -> GTPU Echo Response
                gtpu_header(pkts[m])->type = GTPU_ECHO_RES;
                pgw_swap_address(pkts[m]);
                if (pgw_enq(pnode, EGRESS, pkts[m]) != OK){
                    pgw_panic("failed. enq(gtpu echo req).");
                }
            }else if (gtpu_header(pkts[m])->type == GTPU_ECHO_RES){
                PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpu_echo_res..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
                // replace src/dst in response(keepalive table)
                strncpy(sip, inet_ntoa(pkts[m]->saddr.sin_addr), sizeof(sip)-1);
                strncpy(dip, inet_ntoa(pkts[m]->caddr.sin_addr), sizeof(dip)-1);
                snprintf(sql, sizeof(sql)-1, CHANGE_STATUS_SQL,
                         KEEPALIVE_STAT_OK,
                         dip, ntohs(pkts[m]->caddr.sin_port),
                         sip, ntohs(pkts[m]->saddr.sin_port),
                         pnode->handle->server_gtpc->server_id);

                pthread_mutex_lock(&__mysql_mutex);
                if (mysql_query((MYSQL*)pnode->dbhandle, sql) != 0){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                }
                pthread_mutex_unlock(&__mysql_mutex);

                PGW_LOG(PGW_LOG_LEVEL_DBG, "== GTPU RES(%p:%p) ==\n\t%s\n========\n",pnode->dbhandle, (void*)pthread_self(),  sql);
                // release packet(break here.)
                pgw_free_packet(pkts[m]);
            }else{
                PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. event_gtpu_echo_[req/res].\n");
            }
        }
        return(OK);
    }
    return(ERR);
}