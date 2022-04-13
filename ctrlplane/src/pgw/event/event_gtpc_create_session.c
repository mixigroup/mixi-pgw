/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_create_session.c
    @brief      gtpc : create session receive event
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
#include "pgw_ext.h"


static pthread_mutex_t  _sscanf_mtx_;
static RETCD _on_create_session(packet_ptr pckt, node_ptr pnode);

static RETCD _accept(gtp_packet_ptr pckt, node_ptr pnode, struct gtpc_parse_state* cs);
static RETCD _denied(gtp_packet_ptr pckt, node_ptr pnode,struct gtpc_parse_state* cs);

/**
  event : GTPC Create Session Request\n
 *******************************************************************************
 Rx -> gtpc create session\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_create_session_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m;
    packet_ptr  pkts[32] = {NULL};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        PGW_LOG(PGW_LOG_LEVEL_INF, "event_gtpc_create_session_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
        for(m = 0;m < nburst;m++){
            _on_create_session(pkts[m], pnode);
            pgw_free_packet(pkts[m]);
        }
        return(OK);
    }
    return(ERR);
}
/**
  event : GTPC Create Session Request\n
 *******************************************************************************
 Create Session processing\n
 *******************************************************************************
 @param[in]   pckt   1packet 
 @param[in]   pnode  node object
 @return int  0==OK,0!=error
 */
RETCD _on_create_session(packet_ptr pckt, node_ptr pnode){
    U64 msisdn_num = 0,imsi_num = 0;
    gtp_packet_ptr  parsed_pkt = NULL;
    DBPROVIDER_BIND lookup_bind[1];
    struct in_addr  addr;
    char sql[512] = {0},cip[32] = {0},uip[32] = {0};
    struct gtpc_parse_state    parse_state;
    create_session_node_ext_ptr tnep = (create_session_node_ext_ptr)pnode->node_opt;
    //
    bzero(&parse_state, sizeof(parse_state));
    bzero(lookup_bind, sizeof(lookup_bind));

    // collect parameters necessary for Create Session Response from request packet
    if (gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pckt->data, pckt->datalen) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_alloc_packet_from_payload(%p : %u).", pckt->data, pckt->datalen);
        return(ERR);
    }
    // save peer address
    parsed_pkt->saddrlen = pckt->saddrlen;
    memcpy(&parsed_pkt->saddr, &pckt->saddr, pckt->saddrlen);
    parsed_pkt->caddrlen = pckt->caddrlen;
    memcpy(&parsed_pkt->caddr, &pckt->caddr, pckt->caddrlen);
    // save sequence number
    parse_state.gtph_seqno = gtpc_header(pckt)->q.sq_t.seqno;
    //
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    if ((parse_state.flag&INCLUDED_ALL) != INCLUDED_ALL){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "malformed create session packet.(%u)\n", parse_state.flag);
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
    // valid create session request
    msisdn_num = gtpc_digits_get(parse_state.msisdn.digits, GTPC_MSISDN_LEN);
    imsi_num = gtpc_digits_get(parse_state.imsi.digits, GTPC_IMSI_LEN);

    // update gtp[c/u] from binded ip address
    if (pnode->handle && pnode->handle->server_gtpc){
        bzero(sql, sizeof(sql));
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_PGWIP_UPD_SQL,
                 pnode->handle->server_gtpc->ip,
                 pnode->handle->server_gtpc->ip,
                imsi_num);
        if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s)\n", sql);
            return(_denied(parsed_pkt, pnode, &parse_state));
        }
    }
    if (imsi_num && msisdn_num){
        // msisdn is not determined at contract(imsi only)
        // imsi(1) x msisdn(N)
        bzero(sql, sizeof(sql));
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_IMSI_MSISDN_UPD_SQL, msisdn_num, imsi_num);
        if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s)\n", sql);
            return(_denied(parsed_pkt, pnode, &parse_state));
        }
    }
    // lookup by imsi
    lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    lookup_bind[0].buffer = &imsi_num;
    lookup_bind[0].is_null = 0;
    //
    pthread_mutex_lock(&__mysql_mutex);
    if (DBPROVIDER_STMT_RESET(tnep->stmt) != 0) { pgw_panic("failed. mysql_stmt_reset(%p : %u).", tnep->stmt, (unsigned)imsi_num); }
    if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p : %u).", tnep->stmt, (unsigned)imsi_num); }
    if (DBPROVIDER_EXECUTE(tnep->stmt, tnep->bind.bind, on_tunnel_record, &parse_state) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
    pthread_mutex_unlock(&__mysql_mutex);
    //
    // not found by imsi, find by msisdn in failsafe.
    if (!(parse_state.flag & HAVE_RECORD)){
        lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
        lookup_bind[0].buffer = &msisdn_num;
        lookup_bind[0].is_null = 0;
        //
        pthread_mutex_lock(&__mysql_mutex);
        if (DBPROVIDER_STMT_RESET(tnep->stmt_msisdn) != 0) { pgw_panic("failed. mysql_stmt_reset(%p : %u).", tnep->stmt_msisdn, (unsigned)imsi_num); }
        if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt_msisdn, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p : %u).", tnep->stmt_msisdn, (unsigned)imsi_num); }
        if (DBPROVIDER_EXECUTE(tnep->stmt_msisdn, tnep->bind.bind, on_tunnel_record, &parse_state) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
        pthread_mutex_unlock(&__mysql_mutex);

        // not found by even imsi or msisdn
        if (!(parse_state.flag & HAVE_RECORD)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "missing imsi -> msisdn.(%llu/%llu)\n", imsi_num, msisdn_num);
            return(_denied(parsed_pkt, pnode, &parse_state));
        }
        imsi_num = parse_state.imsi_num;
    }
    // supported only ipv4(from SGW)
    memcpy(&addr, parse_state.c_teid.blk, 4);
    snprintf(cip, sizeof(cip) - 1,"%s", inet_ntoa(addr));
    memcpy(&addr, parse_state.u_teid.blk, 4);
    snprintf(uip, sizeof(uip) - 1,"%s", inet_ntoa(addr));

    // update pgw_teid with ebi from CreateSessionRequest as prefix
    // to pgw_teid collected from database.
    // those changes are also effect to database
    // ex) src teid : 0xdeadbeaf, ebi(4 bit) = 0x7 -> new teid : 0x7eadbeaf
    //     replace first 4 bits with ebi.
    parse_state.pgw_teid &= 0x0fffffff;
    parse_state.pgw_teid |= (0xf0000000&((parse_state.ebi_b.ebi.low&0xf)<<28));

    // update Response parameters by Create Session Request
    bzero(sql, sizeof(sql));

    if (parse_state.flag & HAVE_RECOVERY){
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_UPD_R_SQL,
                 parse_state.c_teid.teid_grekey, cip,
                 parse_state.u_teid.teid_grekey, uip,
                 parse_state.bqos.qci,
                 parse_state.ebi_b.ebi.low,
                 parse_state.pgw_teid,
                 parse_state.recovery.recovery_restart_counter,
                 imsi_num);
    }else{
        snprintf(sql, sizeof(sql)-1, CREATE_SESSION_UPD_SQL,
                 parse_state.c_teid.teid_grekey, cip,
                 parse_state.u_teid.teid_grekey, uip,
                 parse_state.bqos.qci,
                 parse_state.ebi_b.ebi.low,
                 parse_state.pgw_teid,
                 imsi_num);
    }
    if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
        // response denied.
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s)\n", sql);
        return(_denied(parsed_pkt, pnode, &parse_state));
    }
    return(_accept(parsed_pkt, pnode, &parse_state));
}

/**
  Create Session Response Accept\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Create Session processing container
 @return int  0==OK,0!=error
 */
RETCD _accept(gtp_packet_ptr rpckt, node_ptr pnode, struct gtpc_parse_state* cs){
    gtpc_bearer_ctx_t   bctx;
    gtpc_cause_t        cause;
    gtpc_f_teid_t       pgw_c_fteid;
    gtpc_f_teid_t       pgw_u_fteid;
    gtpc_paa_t          paa_ue;
    gtpc_apn_restriction_t  apn;
    gtpc_ambr_t         ambr;
    gtpc_pco_t          pco;
    gtpc_ebi_t          ebi;
    gtpc_bearer_qos_t   bearer_qos;
    gtpc_charging_id_t  chrg;
    gtpc_recovery_t     recovery;
    packet_ptr          cpckt;
    gtpc_private_extension_t    private_ext;
    RETCD               success = ERR,have_pco = ERR;
    gtp_packet_ptr      lpckt = NULL;
    U32                 ueipv = 0;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_CREATE_SESSION_RES) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_ACCEPTED) != OK){ break; }
        if (gtpc_f_teid_set(&pgw_c_fteid, GTPC_INSTANCE_ORDER_1, GTPC_FTEIDIF_S5S8_PGW_GTPC,
                            cs->pgw_teid, (U8*)&cs->pgw_c_ipv_n, NULL) != OK){ break; }
        if ((cs->flag & HAVE_PDN) == HAVE_PDN){
            U8  ipv6[17] = {0x40, 0x24, 0x01, 0xf1, 0x00,
                            (((0x0f&pnode->handle->ext_set_num)<<4)|(0x0f&pnode->handle->pgw_unit_num)), 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x01};
            U8  ipv64[21] = {0};
            ueipv = ntohl(cs->ue_ipv_n.s_addr);

            if ((ueipv>>24)){ break; }
            ueipv |= (PGW_IP8<<24);
            ueipv = htonl(ueipv);
            memcpy(&ipv6[6], &((char*)&ueipv)[1], 3);
            memcpy(&ipv64, ipv6, sizeof(ipv6));
            memcpy(&ipv64[17], &ueipv, 4);
            // 2401:f100:XY00::/40
            // when PDN type is specified, returns specified ipv[4/6/both]
            if (cs->pdn.bit.pdn_type == GTPC_PDN_IPV4){
                if (gtpc_paa_set(&paa_ue, GTPC_PAA_PDNTYPE_IPV4, (U8*)&ueipv, 4) != OK){ break; }
            }else if (cs->pdn.bit.pdn_type == GTPC_PDN_IPV6){
                if (gtpc_paa_set(&paa_ue, GTPC_PAA_PDNTYPE_IPV6, ipv6, sizeof(ipv6)) != OK){ break; }
            }else{
#ifdef __IPV64__
                if (gtpc_paa_set(&paa_ue, GTPC_PAA_PDNTYPE_BOTH, ipv64, sizeof(ipv64)) != OK){ break; }
#else
                // when ipv4/6 is specified, choose ipv4
                // there is an android6.x terminal that cannot communicate on ipv6 near MTU
                // if (gtpc_paa_set(&paa_ue, GTPC_PAA_PDNTYPE_BOTH, ipv64, sizeof(ipv64)) != OK){ break; }
                if (gtpc_paa_set(&paa_ue, GTPC_PAA_PDNTYPE_IPV4, (U8*)&ueipv, 4) != OK){ break; }
#endif
            }
        }else{
            // when PDN type is not specified, choose ipv4
            if (gtpc_paa_set(&paa_ue, GTPC_PAA_PDNTYPE_IPV4, (U8*)&ueipv, 4) != OK){ break; }
        }
        if (gtpc_apn_restriction_set(&apn, GTPC_APN_RESTRICTION) != OK){ break; }
        if (gtpc_ambr_set(&ambr, ntohl(cs->ambr_r.uplink), ntohl(cs->ambr_r.downlink)) != OK){ break; }
        have_pco = setup_pco(&pco, cs);
        if (gtpc_f_teid_set(&pgw_u_fteid, GTPC_INSTANCE_ORDER_2, GTPC_FTEIDIF_S5S8_PGW_GTPU,
                            cs->pgw_teid, (U8*)&cs->pgw_u_ipv_n, NULL) != OK){ break; }
        if (gtpc_ebi_set(&ebi, GTPC_INSTANCE_ORDER_0, cs->ebi_b.ebi.low) != OK){ break; }
        if (gtpc_gtpc_bearer_qos_set(&bearer_qos, GTPC_BQOS_FLAG, GTPC_BQOS_QCI, 0, 0, 0, 0) != OK) { break; }
        if (gtpc_charging_id_set(&chrg, cs->pgw_teid) != OK){ break; }
        if (gtpc_recovery_set(&recovery, PGW_RECOVERY_COUNT) != OK){ break; }
        if (gtpc_private_extension_set(&private_ext, cs) != OK){ break; }
        //
        if (gtpc_bearer_ctx_set(&bctx) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&ebi, gtpc_ebi_length(&ebi)) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&pgw_u_fteid, gtpc_f_teid_length(&pgw_u_fteid)) != OK){ break; }
        //if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&bearer_qos, gtpc_bearer_qos_length(&bearer_qos)) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&chrg, gtpc_charging_id_length(&chrg)) != OK){ break; }
        //
        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_F_TEID, (U8*)&pgw_c_fteid, gtpc_f_teid_length(&pgw_c_fteid)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_PAA, (U8*)&paa_ue, gtpc_paa_length(&paa_ue)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_APN_RESTRICTION, (U8*)&apn, gtpc_apn_restriction_length(&apn)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_AMBR, (U8*)&ambr, gtpc_ambr_length(&ambr)) != OK){ break; }
        if (have_pco == OK){
            if (gtpc_append_item(lpckt, GTPC_TYPE_PCO, (U8*)&pco, gtpc_pco_length(&pco)) != OK){ break; }
        }
        if (gtpc_append_item(lpckt, GTPC_TYPE_BEARER_CTX, (U8*)&bctx, gtpc_bearer_ctx_length(&bctx)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_RECOVERY, (U8*)&recovery, gtpc_recovery_length(&recovery)) != OK){ break; }

        // private extension(add last)
        if (gtpc_append_item(lpckt, GTPC_TYPE_PRIVATE_EXTENSION, (U8*)&private_ext, sizeof(private_ext)) != OK){ break; }

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
    gtpc_header(cpckt)->t.teid = cs->c_teid.teid_grekey;
    gtpc_header(cpckt)->q.sq_t.seqno = cs->gtph_seqno;
    // swap address
    pgw_swap_address(cpckt);
    // enque packet to reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_accept response).");
    }
    // warnings, received PCO but did not reply
    if ((cs->flag&HAVE_PCO) && (have_pco != OK)){
        PGW_LOG(PGW_LOG_LEVEL_WRN, "not responsed pco.\n");
    }
    return(OK);
}

/**
  Create Session Response Denied \n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     Create Session process container
 @return int  0==OK,0!=error
 */
RETCD _denied(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs){
    gtpc_bearer_ctx_t   bctx;
    gtpc_cause_t        cause;
    gtpc_ebi_t          ebi;
    packet_ptr          cpckt;
    RETCD               success = ERR;
    gtp_packet_ptr      lpckt = NULL;
    //
    while(1){
        if (gtpc_alloc_packet(&lpckt, GTPC_CREATE_SESSION_RES) != OK){ break; }
        if (gtpc_cause_set(&cause, GTPC_CAUSE_REQUEST_DENIED_APN_ACCESS) != OK){ break; }
        if (gtpc_bearer_ctx_set(&bctx) != OK){ break; }
        if (gtpc_ebi_set(&ebi, GTPC_INSTANCE_ORDER_0, 0) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_bearer_ctx_add_child(&bctx, (U8*)&ebi, gtpc_ebi_length(&ebi)) != OK){ break; }
        //
        if (gtpc_append_item(lpckt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)) != OK){ break; }
        if (gtpc_append_item(lpckt, GTPC_TYPE_BEARER_CTX, (U8*)&bctx, gtpc_bearer_ctx_length(&bctx)) != OK){ break; }
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
    gtpc_header(cpckt)->t.teid = cs->c_teid.teid_grekey;
    gtpc_header(cpckt)->q.sq_t.seqno = cs->gtph_seqno;
    // swap address.
    pgw_swap_address(cpckt);
    // enque packet to reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied response).");
    }
    return(OK);
}



/**
  SELECT callback for tunnel table\n
 *******************************************************************************
 duplicate records are not considered\n
 *******************************************************************************
 @parma[in]   counter   count of records
 @param[in]   clmncnt   count of columns
 @param[in]   col       columns
 @param[in]   rec       record
 @param[in]   arg       callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_tunnel_record(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* arg){
    struct gtpc_parse_state* cs = (struct gtpc_parse_state*)arg;
    char    dns_bf[32] = {0};
    size_t  tmphexlen = 0;
    //
    if (cs == NULL || clmncnt != CRSES_LOOKUP_CLMN_MAX){
        return(ERR);
    }
    // duplicate records are not considered
    cs->sgw_c_teid    = (U32)rec[CRSES_LOOKUP_CLMN_SGW_C_TEID].u.nval;
    cs->pgw_teid      = (U32)rec[CRSES_LOOKUP_CLMN_PGW_TEID].u.nval;
    cs->imsi_num      = rec[CRSES_LOOKUP_CLMN_IMSI].u.nval;
    cs->msisdn_num_   = rec[CRSES_LOOKUP_CLMN_MSISDN].u.nval;
    cs->bitrate_s5    = rec[CRSES_LOOKUP_CLMN_BITRT_S5].u.nval;
    cs->bitrate_sgi   = rec[CRSES_LOOKUP_CLMN_BITRT_SGI].u.nval;
    cs->ebi           = (U8)rec[CRSES_LOOKUP_CLMN_EBI].u.nval;
#define STRMINCPY(a,b)  strncpy(a, rec[b].u.sval, MIN(sizeof(a)-1,strlen(rec[b].u.sval)))
    //
    STRMINCPY(cs->ue_ipv,    CRSES_LOOKUP_CLMN_UEIPV);
    STRMINCPY(cs->pgw_u_ipv, CRSES_LOOKUP_CLMN_PGW_U_IPV);
    STRMINCPY(cs->pgw_c_ipv, CRSES_LOOKUP_CLMN_PGW_C_IPV);
    STRMINCPY(dns_bf,        CRSES_LOOKUP_CLMN_DNS);
    //
    inet_aton(cs->ue_ipv,    &cs->ue_ipv_n);
    inet_aton(cs->pgw_u_ipv, &cs->pgw_u_ipv_n);
    inet_aton(cs->pgw_c_ipv, &cs->pgw_c_ipv_n);


    // convert saved dns(pco)/ hex string on database ascii -> bin
    cs->dns_len = 0;
    bzero(cs->dns,sizeof(cs->dns));
    if (strlen(dns_bf) > 0){
        if (strlen(dns_bf)%2==0 && strlen(dns_bf)/2 < sizeof(cs->dns)){
            for(int n=0; n < (strlen(dns_bf)/2);n++,tmphexlen+=2){
                pthread_mutex_lock(&_sscanf_mtx_);
                sscanf(&dns_bf[tmphexlen],"%2hhx",&cs->dns[n]);
                pthread_mutex_unlock(&_sscanf_mtx_);
            }
            cs->dns_len = (strlen(dns_bf)/2);
        }
    }
    //
    cs->flag |= HAVE_RECORD;

    return(OK);
}
/**
  Setup PCO response\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   pco   pco struct
 @param[in]   cs    parse status
 @return int  0==OK,0!=error
 */
RETCD setup_pco(gtpc_pco_ptr pco, struct gtpc_parse_state* cs){
    U8      pcobuffer[255] = {0};
    U16     pcolen = 0;
    // pco request is exists
    if (cs->flag&PCO_ALL){

        bzero(pco, sizeof(*pco));
        // DNS query
        if ((cs->flag&HAVE_PCO_DNS_IPV64)==HAVE_PCO_DNS_IPV64){
            if ((cs->flag & HAVE_PDN) == HAVE_PDN){
#ifdef __IPV64__
                memcpy(&pcobuffer[pcolen], PCO_DNS_IPV64_VAL, sizeof(PCO_DNS_IPV64_VAL));
                pcolen += sizeof(PCO_DNS_IPV64_VAL);
#else
                if (cs->pdn.bit.pdn_type == GTPC_PDN_IPV4V6){
                    memcpy(&pcobuffer[pcolen], PCO_DNS_IPV4_VAL, sizeof(PCO_DNS_IPV4_VAL));
                    pcolen += sizeof(PCO_DNS_IPV4_VAL);
                }else{
                    memcpy(&pcobuffer[pcolen], PCO_DNS_IPV64_VAL, sizeof(PCO_DNS_IPV64_VAL));
                    pcolen += sizeof(PCO_DNS_IPV64_VAL);
                }
#endif
            }else{
                memcpy(&pcobuffer[pcolen], PCO_DNS_IPV64_VAL, sizeof(PCO_DNS_IPV64_VAL));
                pcolen += sizeof(PCO_DNS_IPV64_VAL);
            }
        }else if ((cs->flag&HAVE_PCO_DNS_IPV6)==HAVE_PCO_DNS_IPV6){
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPV6_VAL, sizeof(PCO_DNS_IPV6_VAL));
            pcolen += sizeof(PCO_DNS_IPV6_VAL);
        }else if ((cs->flag&HAVE_PCO_DNS_IPV4)==HAVE_PCO_DNS_IPV4){
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPV4_VAL, sizeof(PCO_DNS_IPV4_VAL));
            pcolen += sizeof(PCO_DNS_IPV4_VAL);
        }
        // notify MTU
        if ((cs->flag&HAVE_PCO_NONIP_LINK_MTU)==HAVE_PCO_NONIP_LINK_MTU){
            pco->pco_head.extension = GTPC_PCO_EXTENSION_ON;
            memcpy(&pcobuffer[pcolen], PCO_MTU_NONIP_VAL, sizeof(PCO_MTU_NONIP_VAL));
            pcolen += sizeof(PCO_MTU_NONIP_VAL);
        }else if ((cs->flag&HAVE_PCO_IPV4_LINK_MTU)==HAVE_PCO_IPV4_LINK_MTU){
            pco->pco_head.extension = GTPC_PCO_EXTENSION_ON;
            memcpy(&pcobuffer[pcolen], PCO_MTU_IPV4_VAL, sizeof(PCO_MTU_IPV4_VAL));
            pcolen += sizeof(PCO_MTU_IPV4_VAL);
        }
        // IPCP Configuration
        // from rfc1877
        if ((cs->flag&(HAVE_PCO_IPCP_S_DNS|HAVE_PCO_IPCP_P_DNS))==(HAVE_PCO_IPCP_S_DNS|HAVE_PCO_IPCP_P_DNS)) {
            pco->pco_head.extension = GTPC_PCO_EXTENSION_ON;
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPCP_IPV4_VAL, sizeof(PCO_DNS_IPCP_IPV4_VAL));
            pcolen += sizeof(PCO_DNS_IPCP_IPV4_VAL);
        }
        //
        if (pcolen > 1){
            return(gtpc_pco_set(pco, (U8*)pcobuffer, (pcolen + 1/* PCO header: 1 octet */)));
        }
    }
    return(ERR);
}

