/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_echo.c
    @brief      gtpc echoevent
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

struct _observer_container{
    RETCD result;
    U32 in_counter;
    U32 in_ip;
    U32 in_expire;
    U32 ot_ip;
    U32 ot_counter;
    U32 ot_expire;
};
static void _observer_sgw_peers(node_ptr pnode, U32 ipv4, U32 counter);

/**
  event : GTPC Echo Request Core\n
 *******************************************************************************
 Rx -> gtpc echo [request/ response]\n
 - request:\n
 convert to response, send to tx via software ring\n
 - response:\n
 update status by dst[ip/port],src[ip/port]\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_echo_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m;
    packet_ptr  pkt = NULL;
    packet_ptr  pkts[32] = {NULL};
    char        sql[1024] = {0};
    char        sip[64] = {0},dip[64] = {0};
    gtpc_recovery_ptr   precv;
    unsigned    hlen = sizeof(gtpc_header_t);
    U32         restart_counter = (U32)-1;
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpc_echo_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
        for(m = 0;m < nburst;m++){
            pthread_mutex_lock(&__mysql_mutex);
            if (gtpc_header(pkts[m])->type == GTPC_ECHO_REQ){
                // initialize to 0 count of re-start as received
                if (gtpc_header(pkts[m])->v2_flags.teid) {
                    if (ntohs(gtpc_header(pkts[m])->length)== ((hlen-4) + sizeof(gtpc_recovery_t))){
                        precv = (gtpc_recovery_ptr)(((char*)pkts[m]->data) + sizeof(gtpc_header_t));
                        if (precv->head.type == GTPC_TYPE_RECOVERY){
                            restart_counter = precv->recovery_restart_counter;
                            precv->recovery_restart_counter = 0;
                        }
                    }
                }else{
                    if (ntohs(gtpc_header(pkts[m])->length)== ((hlen-8) + sizeof(gtpc_recovery_t))){
                        precv = (gtpc_recovery_ptr)(((char*)pkts[m]->data) + (sizeof(gtpc_header_t)-4));
                        if (precv->head.type == GTPC_TYPE_RECOVERY){
                            restart_counter = precv->recovery_restart_counter;
                            precv->recovery_restart_counter = 0;
                        }
                    }
                }
                // observe SGW peer(with restart counter)
                if (restart_counter != (U32)-1){
                    _observer_sgw_peers(pnode, (U32)pkts[m]->saddr.sin_addr.s_addr,restart_counter);
                }
                // reply to request
                if (pgw_duplicate_packet(&pkt, pkts[m]) != OK){
                    pgw_panic("failed. memory allocate.");
                }
                pgw_swap_address(pkt);
                if (pgw_enq(pnode, EGRESS, pkt) != OK){
                    pgw_panic("failed. enq(gtpu echo req).");
                }
                // convert GTPC Echo Request -> GTPC Echo Response, and reply
                gtpc_header(pkts[m])->type = GTPC_ECHO_RES;
                pgw_swap_address(pkts[m]);
                if (pgw_enq(pnode, EGRESS, pkts[m]) != OK){
                    pgw_panic("failed. enq(gtpc echo req).");
                }
            }else if (gtpc_header(pkts[m])->type == GTPC_ECHO_RES){
                // replace src/dst in response(keepalive table)
                strncpy(sip, inet_ntoa(pkts[m]->saddr.sin_addr), sizeof(sip)-1);
                strncpy(dip, inet_ntoa(pkts[m]->caddr.sin_addr), sizeof(dip)-1);
                snprintf(sql, sizeof(sql)-1, CHANGE_STATUS_SQL,
                         KEEPALIVE_STAT_OK,
                         dip, ntohs(pkts[m]->caddr.sin_port),
                         sip, ntohs(pkts[m]->saddr.sin_port),
                         pnode->handle->server_gtpc->server_id);
                if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                }
                PGW_LOG(PGW_LOG_LEVEL_DBG, "== GTPC RES(%p:%p) ==\n\t%s\n========\n", (void*)pthread_self(), pnode->dbhandle, sql);

                // release packet(break here)
                pgw_free_packet(pkts[m]);
            }
            pthread_mutex_unlock(&__mysql_mutex);
        }
        return(OK);
    }
    return(ERR);
}
/**
  observe SGW peer \n
 *******************************************************************************
  observe counter value every ip address \n
 *******************************************************************************
 @parma[in]   pnode   node object
 @param[in]   ipv4    ip address(only ipv4 in SGW)
 @param[in]   counter count of re-start
 */
static RETCD _on_sgw_peer_find_extend(void* u, sgw_peer_ptr p){
    struct _observer_container * c = (struct _observer_container*)u;
    c->ot_counter = p->counter;
    c->ot_ip = p->ip;
    c->ot_expire = p->expire;
    c->result = OK;
    // extend expire time
    p->expire = c->in_expire;
    return(OK);
}
void _observer_sgw_peers(node_ptr pnode, U32 ipv4, U32 counter){
    RETCD ret;
    struct in_addr  addr;
    sgw_peers_ptr   peers;
    struct _observer_container  container;
    handle_ptr  inst = NULL;
    char sql[256] = {0};
    char ip[32] = {0};
    //
    if (!pnode || !ipv4 || counter==(U32)-1){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. _observer_sgw_peers.(%p/%u,%u).\n", pnode, ipv4, counter);
        return;
    }
    if (!(peers = pnode->peers_table)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "peers_table is null..\n");
        return;
    }
    if (!(inst = pnode->handle)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "instance handle is null..\n");
        return;
    }

    bzero(&container, sizeof(container));
    container.result = ERR;
    container.in_counter = counter;
    container.in_ip = ipv4;
    container.in_expire = time(NULL);

    ret = pgw_get_sgw_peer(peers, ipv4, _on_sgw_peer_find_extend, &container);
    if (ret != OK || container.result != OK) {
        // not exists -> regist to cache
        if(pgw_set_sgw_peer(peers, container.in_ip, container.in_counter, container.in_expire) != OK) {
            pgw_panic("exception. pgw_append_sgw_peer.");
        }
    }else if (container.ot_counter != container.in_counter){
        // does not match with cache ->
        // disconnect sessions below counter value 
        // among sessions associated with ip address of SGW.
        memcpy(&addr, &container.in_ip, sizeof(addr));

        strncpy(ip, inet_ntoa(addr), sizeof(ip)-1);
        snprintf(sql, sizeof(sql)-1, ECHO_RESTART_UPD_SQL, ip, container.in_counter);
        if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
        }
    }else {
        // match with cache -> no operation.
        PGW_LOG(PGW_LOG_LEVEL_INF, "checked from cache._observer_sgw_peers(%u/%u/%u)\n", container.in_ip, container.in_counter, container.in_expire);
    }
}
