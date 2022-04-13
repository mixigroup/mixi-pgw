/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_delete_bearer.c
    @brief      gtpc : delete bearer receive event
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

static RETCD _accept(gtp_packet_ptr pckt, node_ptr pnode, struct gtpc_parse_state* cs);
static RETCD _denied(gtp_packet_ptr pckt, node_ptr pnode,struct gtpc_parse_state* cs);

/**
  event : GTPC Delete Bearer Request\n
 *******************************************************************************
 Rx -> gtpc delete bearer\n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_delete_bearer_req(packet_ptr pckt, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    gtp_packet_ptr  parsed_pkt = NULL;
    U64 teid_u64 = 0;
    struct gtpc_parse_state    parse_state;
    DBPROVIDER_BIND  lookup_bind[1];
    char sql[512] = {0};

    delete_session_node_ext_ptr tnep = (delete_session_node_ext_ptr)pnode->node_opt;
    PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpc_delete_bearer_req..(%p : %p : %u/%p)\n", pnode, (void*)pthread_self(), pnode->index, pckt);
    bzero(&parse_state, sizeof(parse_state));

    // collect parameters necessary for Delete Bearer Response from request packet.
    if (gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pckt->data, pckt->datalen) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_alloc_packet_from_payload(%p : %u).", pckt->data, pckt->datalen);
        return(ERR);
    }
    // save peer address
    parsed_pkt->saddrlen = pckt->saddrlen;
    memcpy(&parsed_pkt->saddr, &pckt->saddr, pckt->saddrlen);
    parsed_pkt->caddrlen = pckt->caddrlen;
    memcpy(&parsed_pkt->caddr, &pckt->caddr, pckt->caddrlen);
    // save tunnel identifier, sequence number
    parse_state.gtph_seqno = gtpc_header(pckt)->q.sq_t.seqno;
    parse_state.gtph_teid  = ntohl(gtpc_header(pckt)->t.teid);
    // parse gtpc packet
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);

    // find by teid
    teid_u64 = parse_state.gtph_teid;
    lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    lookup_bind[0].buffer = &teid_u64;
    lookup_bind[0].is_null = 0;
    //
    pthread_mutex_lock(&__mysql_mutex);
    if (DBPROVIDER_STMT_RESET(tnep->stmt) != 0) { pgw_panic("failed. mysql_stmt_reset(%p).", tnep->stmt); }
    if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p).", tnep->stmt); }
    if (DBPROVIDER_EXECUTE(tnep->stmt, tnep->bind.bind, on_tunnel_record, &parse_state) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
    pthread_mutex_unlock(&__mysql_mutex);

    // not found teid, response denied
    if (!(parse_state.flag & HAVE_RECORD)){
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
    // update status with Delete Bearer Request parameter
    snprintf(sql, sizeof(sql)-1, DELETE_SESSION_UPD_SQL,
             parse_state.gtph_teid,
             parse_state.ebi_r.ebi.low);
    //
    if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
    //
    return(_accept(parsed_pkt, pnode, &parse_state));
}


/**
  Delete Bearer Response Accept\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Create Session process container
 @return int  0==OK,0!=error
 */
RETCD _accept(gtp_packet_ptr rpckt, node_ptr pnode, struct gtpc_parse_state* cs){
    gtpc_cause_t        cause;
    gtpc_ebi_t          ebi;
    gtpc_pco_t          pco;
    packet_ptr          cpckt;
    gtp_packet_ptr      lpckt;
    RETCD               success = ERR,have_pco = ERR;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_DELETE_BEARER_RES) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_ACCEPTED) != OK){ break; }
        if (gtpc_ebi_set(&ebi, GTPC_INSTANCE_ORDER_0, cs->ebi) != OK){ break; }
        have_pco = setup_pco(&pco, cs);
        //
        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_EBI, (U8*)&ebi, gtpc_ebi_length(&ebi)) != OK){ break; }
        if (have_pco == OK){
            if (gtpc_append_item(lpckt, GTPC_TYPE_PCO, (U8*)&pco, gtpc_pco_length(&pco)) != OK){ break; }
        }
#ifdef __DEBUG_PACKET__
        gtpc_packet_print(lpckt);
#endif
        success = OK;
        break;
    }
    if (lpckt){ gtpc_copy_packet_addr(lpckt, rpckt); }
    gtpc_free_packet(&rpckt);

    if (success != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. setup.\n");
        return(ERR);
    }
    // convert gtp_packet -> packet
    if ((cpckt = gtpc_convert_to_packet(lpckt)) == NULL){
        pgw_panic("failed. gtpc_convert_to_packet.");
    }
    gtpc_header(cpckt)->t.teid = cs->sgw_c_teid;
    gtpc_header(cpckt)->q.sq_t.seqno = cs->gtph_seqno;
    // swap address
    pgw_swap_address(cpckt);
    // enque packet to reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_accept delete bearer response).");
    }
    return(OK);
}
/**
  Delete Bearer Response Denied \n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Create Session process container
 @return int  0==OK,0!=error
 */
RETCD _denied(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs){
    gtpc_cause_t        cause;
    gtpc_ebi_t          ebi;
    gtpc_recovery_t     recovery;
    packet_ptr          cpckt;
    gtp_packet_ptr      lpckt;
    RETCD               success = ERR;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_DELETE_SESSION_RES) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_DENIED_APN_ACCESS) != OK){ break; }
        if (gtpc_ebi_set(&ebi, GTPC_INSTANCE_ORDER_0, cs->ebi) != OK){ break; }
        if (gtpc_recovery_set(&recovery, PGW_RECOVERY_COUNT) != OK){ break; }

        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_EBI, (U8*)&ebi, gtpc_ebi_length(&ebi)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_RECOVERY, (U8*)&recovery, gtpc_recovery_length(&recovery)) != OK){ break; }
        if ((cs->flag&HAVE_PCO)==HAVE_PCO){
            if (gtpc_append_item(lpckt, GTPC_TYPE_PCO, (U8*)&cs->pco, gtpc_pco_length(&cs->pco)) != OK){ break; }
        }
        success = OK;
#ifdef __DEBUG_PACKET__
        gtpc_packet_print(lpckt);
#endif
        break;
    }
    if (lpckt){ gtpc_copy_packet_addr(lpckt, rpckt); }
    gtpc_free_packet(&rpckt);

    if (success != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. setup.\n");
        return(ERR);
    }
    // convert gtp_packet -> packet
    if ((cpckt = gtpc_convert_to_packet(lpckt)) == NULL){
        pgw_panic("failed. gtpc_convert_to_packet.");
    }
    gtpc_header(cpckt)->t.teid = cs->sgw_c_teid;
    gtpc_header(cpckt)->q.sq_t.seqno = cs->gtph_seqno;
    // swap address
    pgw_swap_address(cpckt);
    // enque packet to reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  delete bearer response).");
    }
    return(OK);
}


