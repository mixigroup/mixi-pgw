/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_tx.c
    @brief      packet send event
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
  event : TX Core\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_tx(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    txnode_ext_ptr tnep = (txnode_ext_ptr)pnode->node_opt;
    RETCD       nburst, sent = 0,ret = ERR;
    INT         n,m,sock;
    packet_ptr  pkts[32] = {NULL};
    char        sql[256] = {0};
    //
    int idx[PGW_NODE_TX] = { PGW_NODE_GTPU_ECHO_REQ,
                             PGW_NODE_GTPC_ECHO_REQ,
                             PGW_NODE_TIMER,
                             PGW_NODE_GTPC_CREATE_SESSION_REQ,
#ifndef SINGLE_CREATE_SESSION
                             PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0,
#endif
                             PGW_NODE_GTPC_MODIFY_BEARER_REQ,
                             PGW_NODE_GTPC_OTHER_REQ,
                             PGW_NODE_GTPU_OTHER_REQ,
    };
    for(n = 0;n < PGW_NODE_TX;n++){
        nburst = pgw_deq_burst(pinst->nodes[idx[n]], EGRESS, pkts, 32);
        if (nburst > 0){
        //  PGW_LOG("event_tx..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
            sent += nburst;
            for(m = 0;m < nburst;m++){
                // send on either gtpc-interface or gtpu-interface
                if (idx[n] == PGW_NODE_TIMER){
                    sock = -1;
                    if (pkts[m]->saddr.sin_port == htons(GTPU1_PORT)){
                        sock = tnep->sock_gtpu;
                    }else if (pkts[m]->saddr.sin_port == htons(GTPC_PORT)){
                        sock = tnep->sock_gtpc;
                    }else{
                        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid address.(%08x/%u).\n",
                                pkts[m]->saddr.sin_addr.s_addr,
                                ntohs(pkts[m]->saddr.sin_port)
                        );
                        ret = NOTFOUND;
                    }
                    if (sock != -1){
                        ret = pgw_send_sock(sock, &pkts[m]->caddr, pkts[m]->caddrlen, pkts[m]->data, pkts[m]->datalen);
                    }
                }else{
                    ret = pgw_send(pinst, &pkts[m]->saddr, &pkts[m]->caddr, pkts[m]->caddrlen, pkts[m]->data, pkts[m]->datalen);
                }
                if (ret != OK){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_send.(%d).\n", ret);
                    // when invalid server address, set status = error
                    if (ret == NOTFOUND){
                        snprintf(sql, sizeof(sql)-1, INVALID_STATUS_SQL,
                                 KEEPALIVE_STAT_ERR,
                                 inet_ntoa(pkts[m]->saddr.sin_addr),
                                 ntohs(pkts[m]->saddr.sin_port),
                                 pnode->handle->server_gtpc->server_id);
                        if (mysql_query((MYSQL*)pnode->dbhandle, sql) != 0){
                            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                        }
                    }
                }
                // send completed, can be release packet
                pgw_free_packet(pkts[m]);
            }
        }
    }
    if (sent > 0){
        return(OK);
    }
    return(ERR);
}
