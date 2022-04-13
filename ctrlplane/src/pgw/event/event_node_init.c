/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_node_init.c
    @brief      initialize node.
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
  event : initialize node\n
 *******************************************************************************
 initialize it every node\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_node_init(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    char sql[1024] = {0};
    DBPROVIDER_HANDLE db = (DBPROVIDER_HANDLE)pnode->dbhandle;
    sgw_peers_ptr   peers = NULL;

    PGW_LOG(PGW_LOG_LEVEL_INF, "event_node_init..(%p : %p : %u)\n", pnode, (void*)pthread_self(), pnode->index);
    //
    if (pnode->index == PGW_NODE_TIMER){
        timernode_ext_ptr   tnep;
        //
        tnep = (timernode_ext_ptr)malloc(sizeof(timernode_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(timernode_ext_t));
        }
        bzero(tnep, sizeof(timernode_ext_t));

        TAILQ_INIT(&tnep->upkeys);
        if (pnode->handle->server_gtpc){
            snprintf(sql, sizeof(sql)-1, KEEPALIVE_SQL,
                     pnode->handle->server_gtpc->ip,
                     pnode->handle->server_gtpc->server_id);
        }else{
            snprintf(sql, sizeof(sql)-1, KEEPALIVE_SQL, SRCADDRESS, SRCADDRESS);
        }
        pnode->node_opt = tnep;
        //
        PGW_LOG(PGW_LOG_LEVEL_INF, "event_node_init..timer(%p : %p : %u : %s)\n", pnode, (void*)pthread_self(), pnode->index, sql);
        if ((tnep->stmt = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        // initialize bind regions
        DBPROVIDER_BIND_INIT(&tnep->bind);

        return(OK);
#ifndef SINGLE_CREATE_SESSION
    }else if (pnode->index == PGW_NODE_GTPC_CREATE_SESSION_REQ ||
              pnode->index == PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0){
#else
    }else if (pnode->index == PGW_NODE_GTPC_CREATE_SESSION_REQ){
#endif
        create_session_node_ext_ptr   tnep;
        //
        tnep = (create_session_node_ext_ptr)malloc(sizeof(create_session_node_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(create_session_node_ext_t));
        }
        bzero(tnep, sizeof(create_session_node_ext_t));
        pnode->node_opt = tnep;
        //
        PGW_LOG(PGW_LOG_LEVEL_INF, "<<create session>>event_node_init..(%p : %p : %p : %u : %s)\n",db,  pnode, (void*)pthread_self(), pnode->index, sql);
        // find by imsi(prepared statement)
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_IMSI_LOOKUP_SQL);
        if ((tnep->stmt = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt), DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        // find by msisdn(Prepared Statement)
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_MSISDN_LOOKUP_SQL);
        if ((tnep->stmt_msisdn = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt_msisdn, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt_msisdn, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt_msisdn),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)",DBPROVIDER_ERROR(db));
            }
        }
        // initialize bind regions
        DBPROVIDER_BIND_INIT(&tnep->bind);

    }else if (pnode->index == PGW_NODE_GTPC_MODIFY_BEARER_REQ){
        modify_bearer_node_ext_ptr   tnep;
        //
        tnep = (modify_bearer_node_ext_ptr)malloc(sizeof(modify_bearer_node_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(modify_bearer_node_ext_t));
        }
        bzero(tnep, sizeof(modify_bearer_node_ext_t));
        snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_LOOKUP_SQL);
        pnode->node_opt = tnep;
        //
        PGW_LOG(PGW_LOG_LEVEL_INF, "<<modify bearer>>event_node_init..(%p : %p : %p : %u : %s)\n", db,  pnode, (void*)pthread_self(), pnode->index, sql);
        if ((tnep->stmt = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        // initialize bind regions
        DBPROVIDER_BIND_INIT(&tnep->bind);

    }else if (pnode->index == PGW_NODE_GTPC_OTHER_REQ){
        delete_session_node_ext_ptr   tnep;
        //
        tnep = (delete_session_node_ext_ptr)malloc(sizeof(delete_session_node_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(delete_session_node_ext_t));
        }
        bzero(tnep, sizeof(delete_session_node_ext_t));
        snprintf(sql, sizeof(sql)-1, DELETE_SESSION_LOOKUP_SQL);
        pnode->node_opt = tnep;
        //
        PGW_LOG(PGW_LOG_LEVEL_INF, "<<gtpc other>>event_node_init..(%p : %p : %p : %u : %s)\n",db, pnode, (void*)pthread_self(), pnode->index, sql);
        if ((tnep->stmt = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        //
        bzero(sql, sizeof(sql));
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_IMSI_LOOKUP_SQL);
        if ((tnep->stmt_v1 = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt_v1, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt_v1, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt_v1),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        //
        bzero(sql, sizeof(sql));
        snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_LOOKUP_SQL);
        if ((tnep->stmt_v1_mb = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt_v1_mb, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt_v1_mb, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt_v1_mb),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)",DBPROVIDER_ERROR(db));
            }
        }
        // initialize bind regions
        DBPROVIDER_BIND_INIT(&tnep->bind);

    }else if (pnode->index == PGW_NODE_GTPU_OTHER_REQ){
        gtpu_other_node_ext_ptr tnep;
        //
        tnep = (gtpu_other_node_ext_ptr)malloc(sizeof(gtpu_other_node_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc - gtpu_othrer_node_ext(%u)", (unsigned)sizeof(gtpu_other_node_ext_t));
        }
        bzero(tnep, sizeof(gtpu_other_node_ext_t));
        snprintf(sql, sizeof(sql)-1, ICMPV6_RA_LOOKUP_SQL);
        pnode->node_opt = tnep;
        //
        PGW_LOG(PGW_LOG_LEVEL_INF, "<<gtpu - other>>event_node_init..(%p : %p : %p : %u : %s)\n",db, pnode, (void*)pthread_self(), pnode->index, sql);
        if ((tnep->stmt = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (DBPROVIDER_STMT_PREPARE(tnep->stmt, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)", DBPROVIDER_STMT_ERROR(tnep->stmt),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        // initialize bind regions
        DBPROVIDER_BIND_INIT(&tnep->bind);
    }else if (pnode->index == PGW_NODE_TX){
        // TX node ->  prepare descriptros for tx.
        txnode_ext_ptr  tnep;
        //
        tnep = (txnode_ext_ptr)malloc(sizeof(txnode_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(txnode_ext_t));
        }
        bzero(tnep, sizeof(txnode_ext_t));
        pnode->node_opt = tnep;
        // gtpc side TX socket
        if (((handle_ptr)(pnode->handle))->server_gtpc){
            tnep->sock_gtpc = ((handle_ptr)(pnode->handle))->server_gtpc->sock;
            if (tnep->sock_gtpc < 0) {
                pgw_panic("failed. create socket(%d: %s)", errno, strerror(errno));
            }
        }
        // gtpu side TX socket
        if (((handle_ptr)(pnode->handle))->server_gtpu){
            tnep->sock_gtpu = ((handle_ptr)(pnode->handle))->server_gtpu->sock;
            if (tnep->sock_gtpu < 0) {
                pgw_panic("failed. create socket(%d: %s)", errno, strerror(errno));
            }
        }
        return(OK);
    }else if (pnode->index == PGW_NODE_GTPC_ECHO_REQ){
        // cache table for management SGW peers
        if (pgw_create_sgw_peers(0, &peers) != OK){
            pgw_panic("failed. pgw_create_sgw_peers .. ");
        }
        pnode->peers_table = peers;
    }
    return(ERR);
}