/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_resume_notification.c
    @brief      gtpc : resume notification receive event
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
  event : GTPC Resume Notification Request\n
 *******************************************************************************
 Rx -> gtpc resume notification \n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_resume_notification_req(packet_ptr pckt, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    gtp_packet_ptr  parsed_pkt = NULL;
    U64 imsi_num = 0,teid_u64 = 0;
    DBPROVIDER_BIND  lookup_bind[1];
    struct gtpc_parse_state    parse_state;
    char sql[512] = {0};

    delete_session_node_ext_ptr tnep = (delete_session_node_ext_ptr)pnode->node_opt;
    PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpc_resume_notification_req..(%p : %p : %u/%p)\n", pnode, (void*)pthread_self(), pnode->index, pckt);
    bzero(&parse_state, sizeof(parse_state));

    // collect parameters neccesarry for Resume Notification Response from Request packet
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
    // parse 
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    if ((parse_state.flag&HAVE_IMSI) != HAVE_IMSI){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "malformed resume notification packet.(%u)\n", parse_state.flag);
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
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

    // not found teid -> response denied
    if (!(parse_state.flag & HAVE_RECORD)){
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
    // update status by Resume Notification Request
    imsi_num = gtpc_digits_get(parse_state.imsi.digits, GTPC_IMSI_LEN);
    snprintf(sql, sizeof(sql)-1, RESUME_NOTIFY_UPD_SQL, imsi_num);
    //
    pthread_mutex_lock(&__mysql_mutex);
    if (mysql_query((MYSQL*)pnode->dbhandle, sql) != 0){
        pthread_mutex_unlock(&__mysql_mutex);
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
    pthread_mutex_unlock(&__mysql_mutex);
    //
    return(_accept(parsed_pkt, pnode, &parse_state));
}

/**
  Resume Notification Response Accept\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Resume Notification processing container
 @return int  0==OK,0!=error
 */
RETCD _accept(gtp_packet_ptr rpckt, node_ptr pnode, struct gtpc_parse_state* cs){
    gtpc_cause_t        cause;
    packet_ptr          cpckt;
    gtp_packet_ptr      lpckt;
    RETCD               success = ERR;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_RESUME_ACK) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_ACCEPTED) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
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
    // enqueue packet to reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_accept delete bearer response).");
    }
    return(OK);
}
/**
  Resume Notification Response Denied \n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Resume Notification processing container
 @return int  0==OK,0!=error
 */
RETCD _denied(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs){
    gtpc_cause_t        cause;
    packet_ptr          cpckt;
    gtp_packet_ptr      lpckt;
    RETCD               success = ERR;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_RESUME_ACK) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_DENIED_APN_ACCESS) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
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
    // enqueue packet into reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  delete bearer response).");
    }
    return(OK);
}
