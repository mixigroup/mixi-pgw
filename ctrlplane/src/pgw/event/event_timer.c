/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_timer.c
    @brief      timer event
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

static RETCD on_rec(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* udat);


/**
  event : timer Core\n
 *******************************************************************************
 do not modify node object in timer context\n
 \n
 - start life monitoring for GTP[u/c]-directed servers configured in database\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_timer(handle_ptr pinst, void* ext){
    server_ptr pserver = (server_ptr)ext;
    upkey_ptr  cur, curtmp, freeptr;
    char sql[1024] = {0};

    // use timer node
    node_ptr node = pinst->nodes[PGW_NODE_TIMER];
    // GTP[U/C] echo request to Peer
    // that has exceeded 60 sec since last update
    if (node && node->node_opt){
        timernode_ext_ptr tnep = (timernode_ext_ptr)node->node_opt;
        //
        pthread_mutex_lock(&__mysql_mutex);
        if (DBPROVIDER_EXECUTE(tnep->stmt, tnep->bind.bind, on_rec, pinst) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. event_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pserver);
            pgw_panic("failed. event_mysql_execute.(%p:%s:%s).\n", tnep->stmt, DBPROVIDER_STMT_ERROR(tnep->stmt), DBPROVIDER_ERROR(node->dbhandle));
        }
        pthread_mutex_unlock(&__mysql_mutex);
        //
        pthread_mutex_lock(&__mysql_mutex);
        TAILQ_FOREACH_SAFE(cur, &tnep->upkeys, link, curtmp) {
            snprintf(sql, sizeof(sql)-1, CHANGE_STATUS_SQL, KEEPALIVE_STAT_WAIT,
                     cur->src_ip, cur->src_port,
                     cur->dst_ip, cur->dst_port,
                     node->handle->server_gtpc->server_id);
//          PGW_LOG("== REQ ==\n\t%s\n========\n", sql);
            if (DBPROVIDER_QUERY(node->dbhandle, sql) != 0){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s: %s).\n", sql, DBPROVIDER_ERROR(node->dbhandle));
                pgw_panic("failed. mysql_query.(%s : %s).\n", sql, DBPROVIDER_ERROR(node->dbhandle));
            }
            freeptr = cur;
            TAILQ_REMOVE(&tnep->upkeys, cur, link);
            free(freeptr);
        }
        pthread_mutex_unlock(&__mysql_mutex);
    }
    return(ERR);
}

/**
  record callback\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   counter   count of record
 @param[in]   clmncnt   count of columns
 @param[in]   col       column struct address
 @param[in]   rec       record struct address
 @param[in]   udat      user data
 @return int  0==OK,0!=error
 */
RETCD on_rec(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* udat){
    // send gtp[c/u] echo to destination
    packet_ptr  pkt = NULL;
    U16 len = sizeof(gtpc_header_t) + sizeof(gtpc_recovery_t) - sizeof(U32);
    handle_ptr pinst = (handle_ptr)udat;
    timernode_ext_ptr tnode = NULL;
    upkey_ptr ukey = NULL;
    //
    if (pinst == NULL){
        return(ERR);
    }
    if (clmncnt != KEEPALIVE_CLMN_MAX){
        return(ERR);
    }
    // server_type==0(only master server) -> send keepalive
    // slave server -> response to keepalive
    if (rec[KEEPALIVE_CLMN_SERVER_TYPE].u.nval != KEEPALIVE_SERVER_TYPE_MASTER){
        return(ERR);
    }
    if (pinst->nodes[PGW_NODE_TIMER] && pinst->nodes[PGW_NODE_TIMER]->node_opt){
        tnode = (timernode_ext_ptr)pinst->nodes[PGW_NODE_TIMER]->node_opt;
    }
    if (tnode == NULL){
        return(ERR);
    }
    if (pgw_alloc_packet(&pkt, len) != OK){
        pgw_panic("failed. memory allocate.");
    }
    // destination address
    pkt->caddr.sin_family    = AF_INET;
    pkt->caddr.sin_port      = htons(rec[KEEPALIVE_CLMN_DST_PORT].u.nval);
    inet_pton(AF_INET, rec[KEEPALIVE_CLMN_DST_IP].u.sval ,&pkt->caddr.sin_addr.s_addr);
    pkt->caddrlen            = sizeof(struct sockaddr_in);
    // source address
    pkt->saddr.sin_family    = AF_INET;
    pkt->saddr.sin_port      = htons(rec[KEEPALIVE_CLMN_SRC_PORT].u.nval);
    inet_pton(AF_INET, rec[KEEPALIVE_CLMN_SRC_IP].u.sval ,&pkt->saddr.sin_addr.s_addr);
    pkt->saddrlen            = sizeof(struct sockaddr_in);

    // save update keys after callback completed.
    if ((ukey = (upkey_ptr)malloc(sizeof(upkey_t))) == NULL){
        pgw_panic("failed. memory allocate... upkey");
    }
    memcpy(ukey->src_ip, rec[KEEPALIVE_CLMN_SRC_IP].u.sval, MIN(sizeof(ukey->src_ip), sizeof(rec[KEEPALIVE_CLMN_SRC_IP].u.sval)));
    ukey->src_port = rec[KEEPALIVE_CLMN_SRC_PORT].u.nval;
    memcpy(ukey->dst_ip, rec[KEEPALIVE_CLMN_DST_IP].u.sval, MIN(sizeof(ukey->dst_ip), sizeof(rec[KEEPALIVE_CLMN_DST_IP].u.sval)));
    ukey->dst_port = rec[KEEPALIVE_CLMN_DST_PORT].u.nval;
    ukey->next_status = KEEPALIVE_STAT_WAIT;

    //
    TAILQ_INSERT_TAIL(&(tnode->upkeys), ukey, link);

    //
    if (rec[KEEPALIVE_CLMN_PROTO].u.nval == KEEPALIVE_PROTO_GTPC){
        static U32 gtpc_seqnum = GTP_ECHO_SEQNUM_MIN;
        PGW_LOG(PGW_LOG_LEVEL_DBG, ">> GTPC >>\n\tsrc[%08x:%u] dst[%08x:%u]\n\tukey src[%s:%u] - dst[%s:%u]\n<<<<<<<<\n",
                pkt->saddr.sin_addr.s_addr,
                pkt->saddr.sin_port,
                pkt->caddr.sin_addr.s_addr,
                pkt->caddr.sin_port,
                ukey->src_ip, ukey->src_port,
                ukey->dst_ip, ukey->dst_port
        );
        // GTPC echo request
        gtpc_header_ptr gtpch = (gtpc_header_ptr)pkt->data;
        gtpc_recovery_ptr gtpc_rcvr = (gtpc_recovery_ptr)(pkt->data + sizeof(gtpc_header_t) - sizeof(U32));
        //
        gtpch->v2_flags.version = GTPC_VERSION_2;
        gtpch->v2_flags.teid    = GTPC_TEID_OFF;
        gtpch->v2_flags.piggy   = GTPC_PIGGY_OFF;
        gtpch->type             = GTPC_ECHO_REQ;
        gtpch->length           = htons(sizeof(gtpc_recovery_t)+4);
        gtpch->t.sq.seqno       = BE24(gtpc_seqnum++);
        gtpc_rcvr->head.type    = GTPC_TYPE_RECOVERY;
        gtpc_rcvr->head.length  = htons(1);
        gtpc_rcvr->recovery_restart_counter = 8;
        pkt->datalen            = len;
        if (gtpc_seqnum>=GTP_ECHO_SEQNUM_MAX){ gtpc_seqnum = GTP_ECHO_SEQNUM_MIN; }
    }else if (rec[KEEPALIVE_CLMN_PROTO].u.nval == KEEPALIVE_PROTO_GTPU){
        static U32 gtpu_seqnum = GTP_ECHO_SEQNUM_MIN;
        PGW_LOG(PGW_LOG_LEVEL_DBG, ">> GTPU >>\n\tsrc[%08x:%u] dst[%08x:%u]\n\tukey src[%s:%u] - dst[%s:%u]\n<<<<<<<<\n",
                pkt->saddr.sin_addr.s_addr,
                pkt->saddr.sin_port,
                pkt->caddr.sin_addr.s_addr,
                pkt->caddr.sin_port,
                ukey->src_ip, ukey->src_port,
                ukey->dst_ip, ukey->dst_port
        );
        // GTPU echo request
        gtpu_header_ptr gtpuh = (gtpu_header_ptr)pkt->data;
        //
        gtpuh->v1_flags.npdu = GTPU_NPDU_OFF;
        gtpuh->v1_flags.sequence = GTPU_SEQ_1;
        gtpuh->v1_flags.extension = GTPU_EXTEND_0;
        gtpuh->v1_flags.proto = GTPU_PROTO_GTP;
        gtpuh->v1_flags.version = GTPU_VERSION_1;
        gtpuh->type     = GTPU_ECHO_REQ;
        gtpuh->length   = htons(4);
        gtpuh->tid      = 0;
        *(U32*)&(gtpuh[1]) = htons(gtpu_seqnum++);
        pkt->datalen    = sizeof(gtpu_header_t)+4;
        if (gtpu_seqnum>=GTP_ECHO_SEQNUM_MAX){ gtpu_seqnum = GTP_ECHO_SEQNUM_MIN; }
    }else{
        pgw_free_packet(pkt);
        pgw_panic("?????");

        return(ERR);
    }
    if (pgw_enq(pinst->nodes[PGW_NODE_TIMER], EGRESS, pkt) != OK){
        pgw_panic("failed. enq(gtpu echo req).");
    }
    return(OK);
}

/**
  event : time out  Keepalive Fire\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_keepalive_timeout(handle_ptr pinst, void* ext){
    node_ptr node = (node_ptr)ext;
    unsigned long sid = (unsigned long)-1, nsid = (unsigned long)-1;
    int ret = -1;
    //
    if (node){
        pthread_mutex_lock(&__mysql_mutex);
        sid  = mysql_thread_id((MYSQL*)node->dbhandle);
        ret  = mysql_ping((MYSQL*)node->dbhandle);
        nsid = mysql_thread_id((MYSQL*)node->dbhandle);
        pthread_mutex_unlock(&__mysql_mutex);
    }
//  PGW_LOG(PGW_LOG_LEVEL_DBG, "%012llu: %p / database handle(%p) [%08x - %08x: %d]\n", node->tocounter, (void*)pthread_self(), node->dbhandle,  sid, nsid, ret);
    return(ret);
}

