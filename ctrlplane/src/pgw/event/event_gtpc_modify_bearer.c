/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_modify_bearer.c
    @brief      gtpc : modify bearer receive event
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

static RETCD _on_modify_bearer(packet_ptr pckt, node_ptr pnode);
static RETCD _accept(gtp_packet_ptr pckt, node_ptr pnode, struct gtpc_parse_state* cs);
static RETCD _denied(gtp_packet_ptr pckt, node_ptr pnode,struct gtpc_parse_state* cs);

/**
  event : GTPC Modify Bearer Request\n
 *******************************************************************************
 Rx -> gtpc modify bearer\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_modify_bearer_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m;
    packet_ptr  pkts[32] = {NULL};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpc_modify_bearer_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
        for(m = 0;m < nburst;m++){
            _on_modify_bearer(pkts[m], pnode);
            pgw_free_packet(pkts[m]);
        }
        return(OK);
    }
    return(ERR);
}

/**
  event : GTPC Modify Bearer Request\n
 *******************************************************************************
 Modify Bearer Processing\n
 *******************************************************************************
 @parma[in]   pckt   1packet 
 @param[in]   pnode  node object
 @return int  0==OK,0!=error
 */
RETCD _on_modify_bearer(packet_ptr pckt, node_ptr pnode){
    gtp_packet_ptr  parsed_pkt = NULL;
    U64 teid_u64 = 0;
    DBPROVIDER_BIND  lookup_bind[1];
    struct in_addr  addr;
    struct gtpc_parse_state    parse_state;
    char sql[512] = {0},cip[32] = {0},uip[32] = {0};

    modify_bearer_node_ext_ptr tnep = (modify_bearer_node_ext_ptr)pnode->node_opt;
    bzero(&parse_state, sizeof(parse_state));

    // collect parameters necessary for Modify Bearer Response from request packet
    if (gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pckt->data, pckt->datalen) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_alloc_packet_from_payload(%p : %u).\n", pckt->data, pckt->datalen);
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
    // no required parameter exists in Modify Bearer

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
    //
    if (!(parse_state.flag & HAVE_RECORD)){
        // not found teid -> response denied.
        PGW_LOG(PGW_LOG_LEVEL_ERR, "missing teid(%u).\n", parse_state.gtph_teid);
        return(_denied(parsed_pkt, pnode, &parse_state));
    }

    //
    parse_state.pgw_teid &= 0x0fffffff;
    parse_state.pgw_teid |= (0xf0000000&((parse_state.ebi_b.ebi.low&0xf)<<28));

    // supported only ipv4(from SGW)
    memcpy(&addr, parse_state.c_teid.blk, 4);
    snprintf(cip, sizeof(cip) - 1,"%s", inet_ntoa(addr));
    memcpy(&addr, parse_state.u_teid.blk, 4);
    snprintf(uip, sizeof(uip) - 1,"%s", inet_ntoa(addr));

    // update response parameters by Modify Bearer Request
    if (parse_state.c_teid.teid_grekey && parse_state.u_teid.teid_grekey) {
        if (parse_state.flag & HAVE_RAT) {
            if (parse_state.flag & HAVE_RECOVERY) {
                snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RAT_UPD_R_SQL,
                         parse_state.c_teid.teid_grekey, cip,
                         parse_state.u_teid.teid_grekey, uip,
                         parse_state.pgw_teid, parse_state.rat.rat_type,
                         parse_state.recovery.recovery_restart_counter,
                         parse_state.imsi_num);
            }else{
                snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RAT_UPD_SQL,
                         parse_state.c_teid.teid_grekey, cip,
                         parse_state.u_teid.teid_grekey, uip,
                         parse_state.pgw_teid, parse_state.rat.rat_type,
                         parse_state.imsi_num);
            }
        } else {
            if (parse_state.flag & HAVE_RECOVERY) {
                snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_UPD_R_SQL,
                         parse_state.c_teid.teid_grekey, cip,
                         parse_state.u_teid.teid_grekey, uip,
                         parse_state.pgw_teid,
                         parse_state.recovery.recovery_restart_counter,
                         parse_state.imsi_num);
            }else{
                snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_UPD_SQL,
                         parse_state.c_teid.teid_grekey, cip,
                         parse_state.u_teid.teid_grekey, uip,
                         parse_state.pgw_teid,
                         parse_state.imsi_num);
            }
        }
        if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
            // response denied.
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
            return(_denied(parsed_pkt, pnode, &parse_state));
        }
        // use new teid
        parse_state.sgw_c_teid = parse_state.c_teid.teid_grekey;
    }else {
        // only RAT-CHANGE
        if (parse_state.flag & HAVE_RAT) {
            if (parse_state.flag & HAVE_RECOVERY) {
                snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RATONLY_UPD_R_SQL, parse_state.rat.rat_type, parse_state.recovery.recovery_restart_counter, parse_state.imsi_num);
            }else{
                snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RATONLY_UPD_SQL, parse_state.rat.rat_type, parse_state.imsi_num);
            }
            if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
                // response denied.
                PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                return(_denied(parsed_pkt, pnode, &parse_state));
            }
        }
    }
    // when is ebi exists in Bearer Context, save to database.
    if (parse_state.flag & HAVE_EBI_B){
        snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_EBI_UPD_SQL, parse_state.ebi_b.ebi.low, parse_state.imsi_num);
        if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
            // response denied.
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
            return(_denied(parsed_pkt, pnode, &parse_state));
        }
    }
    return(_accept(parsed_pkt, pnode, &parse_state));
}
/**
  Modify Bearer Response Accept\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Create Session process container
 @return int  0==OK,0!=error
 */
RETCD _accept(gtp_packet_ptr rpckt, node_ptr pnode, struct gtpc_parse_state* cs){
    gtpc_bearer_ctx_t   bctx;
    gtpc_cause_t        cause;
    gtpc_pco_t          pco;
    gtpc_ebi_t          ebi_lnk;
    gtpc_ebi_t          ebi;
    gtpc_charging_id_t  chrg;
    gtpc_recovery_t     recovery;
    gtpc_msisdn_t       msisdn;
    gtpc_apn_restriction_t  apn;
    gtpc_ambr_t         ambr;
    packet_ptr          cpckt;
    gtpc_private_extension_t    private_ext;
    RETCD               success = ERR,have_pco = ERR;
    gtp_packet_ptr      lpckt;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_MODIFY_BEARER_RES) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_ACCEPTED) != OK){ break; }
        if (gtpc_msisdn_set(&msisdn, cs->msisdn_num_) != OK){ break; }
        have_pco = setup_pco(&pco, cs);
        if (gtpc_ebi_set(&ebi, GTPC_INSTANCE_ORDER_0, cs->ebi_b.ebi.low) != OK){ break; }
        if (gtpc_ebi_set(&ebi_lnk, GTPC_INSTANCE_ORDER_0, 5) != OK){ break; }
        if (gtpc_charging_id_set(&chrg, cs->pgw_teid) != OK){ break; }
        if (gtpc_recovery_set(&recovery, PGW_RECOVERY_COUNT) != OK){ break; }
        if (gtpc_private_extension_set(&private_ext, cs) != OK){ break; }
        if (gtpc_apn_restriction_set(&apn, GTPC_APN_RESTRICTION) != OK){ break; }
        if (gtpc_ambr_set(&ambr, ntohl(cs->ambr_r.uplink), ntohl(cs->ambr_r.downlink)) != OK){ break; }
        //
        if (gtpc_bearer_ctx_set(&bctx) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&ebi, gtpc_ebi_length(&ebi)) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&chrg, gtpc_charging_id_length(&chrg)) != OK){ break; }
        //
        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_MSISDN, (U8*)&msisdn, gtpc_msisdn_length(&msisdn)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_EBI, (U8*)&ebi_lnk, gtpc_ebi_length(&ebi_lnk)) != OK){ break; }
//      if (gtpc_append_item(lpckt, GTPC_TYPE_AMBR, (U8*)&ambr, gtpc_ambr_length(&ambr)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_APN_RESTRICTION, (U8*)&apn, gtpc_apn_restriction_length(&apn)) != OK){ break; }

        if (have_pco == OK){
            if (gtpc_append_item(lpckt, GTPC_TYPE_PCO, (U8*)&pco, gtpc_pco_length(&pco)) != OK){ break; }
        }
        if (gtpc_append_item(lpckt, GTPC_TYPE_BEARER_CTX, (U8*)&bctx, gtpc_bearer_ctx_length(&bctx)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_RECOVERY, (U8*)&recovery, gtpc_recovery_length(&recovery)) != OK){ break; }

        // private extension(add last)
        // modify bearer Request / excluding RAT-CHANGE only
        if (cs->c_teid.teid_grekey && cs->u_teid.teid_grekey) {
            if (gtpc_append_item(lpckt, GTPC_TYPE_PRIVATE_EXTENSION, (U8*)&private_ext, sizeof(private_ext)) != OK){ break; }
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
    // enqueue pakcet to reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_accept modify bearer response).");
    }
    return(OK);
}
/**
  Modify Bearer Response Denied \n
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
    gtp_packet_ptr      lpckt;
    packet_ptr          cpckt;
    RETCD               success = ERR;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_MODIFY_BEARER_RES) != OK){ break; }
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
    // even if accept is not possible,
    // use new teid notified in modify bearer.
    gtpc_header(cpckt)->t.teid = cs->sgw_c_teid;
    gtpc_header(cpckt)->q.sq_t.seqno = cs->gtph_seqno;
    // swap address
    pgw_swap_address(cpckt);
    // enqueue packet into reply ring.
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  modify bearer response).");
    }
    return(OK);
}
