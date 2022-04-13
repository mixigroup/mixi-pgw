/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpc_v1_any.c
    @brief      gtpc : version1 any packet receive event
*******************************************************************************
*******************************************************************************
    @date       created(24/jan/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 24/jan/2018 
      -# Initial Version
******************************************************************************/
#include "../pgw.h"


static RETCD _denied(gtp_packet_ptr pckt, node_ptr pnode,struct gtpc_parse_state* cs);
static RETCD _accept_create_pdp_ctx(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs);
static RETCD _accept_update_pdp_ctx(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs);
static RETCD _accept_delete_pdp_ctx(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs);

static RETCD _on_create_pdp_ctx(struct gtpc_parse_state* cs, void* opt);
static RETCD _on_update_pdp_ctx(struct gtpc_parse_state* cs, void* opt);
static RETCD _on_delete_pdp_ctx(struct gtpc_parse_state* cs, void* opt);

static struct messge_in_gtpc_v1 msgs_gtpc_v1[GTPC_V1_IE_MAX] = {
        {  0,  0, NULL},
        { GTPC_V1_IE_CAUSE,             2 , on_gtpcv1_parse_root},  { GTPC_V1_IE_IMSI,              9 , on_gtpcv1_parse_root},
        { GTPC_V1_IE_RAI,               7 , NULL},                  { GTPC_V1_IE_TLLI,              5 , NULL},
        { GTPC_V1_IE_P_TMSI,            5 , NULL},
        {  0,  0, NULL},
        {  0,  0, NULL},
        { GTPC_V1_IE_REORDERING_REQ,    2 , NULL},                  { GTPC_V1_IE_AUTH_TRIPLET,      29},
        {  0,  0, NULL},
        { GTPC_V1_IE_MAP_CAUSE,         2 , NULL},                  { GTPC_V1_IE_P_TMSI_SIGN,       4 , NULL},
        { GTPC_V1_IE_MS_VALIDATED,      2 , NULL},                  { GTPC_V1_IE_RECOVERY,          2 , NULL},
        { GTPC_V1_IE_SELECTION_MODE,    2 , NULL},                  { GTPC_V1_IE_TEID_DATA_I,       5 , on_gtpcv1_parse_root},
        { GTPC_V1_IE_TEID_CTRL_PLANE,   5 , on_gtpcv1_parse_root},  { GTPC_V1_IE_TEID_DATA_II,      6 , NULL},
        { GTPC_V1_IE_TEARDOWN_IND,      2 , NULL},                  { GTPC_V1_IE_NSAPI,             2 , on_gtpcv1_parse_root},
        { GTPC_V1_IE_RANAP_CAUSE,       2 , NULL},                  { GTPC_V1_IE_RAB_CONTEXT,       10, NULL},
        { GTPC_V1_IE_RADIO_PRIORITY_SMS,2 , NULL},                  { GTPC_V1_IE_RADIO_PRIORITY,    2 , NULL},
        { GTPC_V1_IE_PACKET_FLOW_ID,    3 , NULL},                  { GTPC_V1_IE_CHARGING_CHARACTERISTICS, 3 , NULL},
        { GTPC_V1_IE_TRACE_REFERENCE,   3 , NULL},                  { GTPC_V1_IE_TRACE_TYPE,        3 , NULL},
        { GTPC_V1_IE_MSNOT_REACHABLE_REASON, 2 , NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL},
        { GTPC_V1_IE_CHARGING_ID,       5 , NULL},
        { GTPC_V1_IE_END_USER_ADDRESS,  0 , NULL},                  { GTPC_V1_IE_MM_CONTEXT,        0 , NULL},
        { GTPC_V1_IE_PDP_CONTEXT,       0 , NULL},                  { GTPC_V1_IE_ACCESS_POINT_NAME, 0 , NULL},
        { GTPC_V1_IE_PCO,               0 , on_gtpcv1_parse_root},  { GTPC_V1_IE_GSN_ADDRESS,       0 , on_gtpcv1_parse_root},
        { GTPC_V1_IE_MSISDN,            0 , on_gtpcv1_parse_root},  { GTPC_V1_IE_QOS,               0 , on_gtpcv1_parse_root},
        { GTPC_V1_IE_AUTH_Q,            0 , NULL},                  { GTPC_V1_IE_TRAFFIC_FLOW_TEMPLATE,  0 , NULL},
        { GTPC_V1_IE_TARGET_ID,         0 , NULL},                  { GTPC_V1_IE_UTRAN,             0 , NULL},
        { GTPC_V1_IE_RAB_SETUP,         0 , NULL},                  { GTPC_V1_IE_EXTENSION_HEADER_TYPE_LIST,  0 , NULL},
        { GTPC_V1_IE_TRIGGER_ID,        0 , NULL},                  { GTPC_V1_IE_OMC_ID,            0 , NULL},
        { GTPC_V1_IE_RAN_TRANSPORT_CONTAINER,  0 , NULL},           { GTPC_V1_IE_PDP_CONTEXT_PRIOR, 0 , NULL},
        { GTPC_V1_IE_ADDITIONAL_RAB_SETUP,  0 , NULL},              { GTPC_V1_IE_SGSN_NUMBER,       0 , NULL},
        { GTPC_V1_IE_COMMON_FLAGS,      0 , NULL},                  { GTPC_V1_IE_APN_RESTRICTION,   0 , NULL},
        { GTPC_V1_IE_RADIO_LCS,         0 , NULL},                  { GTPC_V1_IE_RAT_TYPE,          0 , NULL},
        { GTPC_V1_IE_USER_LOCATION_INFO,0 , NULL},                  { GTPC_V1_IE_MS_TIME_ZONE,      0 , NULL},
        { GTPC_V1_IE_IMEI,              0 , NULL},                  { GTPC_V1_IE_CAMEL_CHARGING,    0 , NULL},
        { GTPC_V1_IE_MBMS_UE_CONTEXT,   0 , NULL},                  { GTPC_V1_IE_TMGI,              0 , NULL},
        { GTPC_V1_IE_RIM_ROUTING_ADDRESS, 0 , NULL},                { GTPC_V1_IE_MBMS_PCO,          0 , NULL},
        { GTPC_V1_IE_MBMS_SA,           0 , NULL},                  { GTPC_V1_IE_SOURCE_RNC_PDCP,   0 , NULL},
        { GTPC_V1_IE_ADDITIONAL_TRACE_INFO, 0 , NULL},              { GTPC_V1_IE_HOP_COUNTER,       0 , NULL},
        { GTPC_V1_IE_SELECTED_PLMN_ID,  0 , NULL},                  { GTPC_V1_IE_MBMS_SESSION_ID,   0 , NULL},
        { GTPC_V1_IE_MBMS_2G3G_ID,      0 , NULL},                  { GTPC_V1_IE_ENHANCED_NSAPI,    0 , NULL},
        { GTPC_V1_IE_MBMS_SESSION_DURATION,  0 , NULL},             { GTPC_V1_IE_ADDITIONAL_MBMS_TRACE_INFO,  0 , NULL},
        { GTPC_V1_IE_MBMS_SESSION_REPETITION_NUMBER,  0 , NULL},    { GTPC_V1_IE_MBMS_TIME_TO_DATA_TRANSFER,  0 , NULL},
        { GTPC_V1_IE_PS_HANDOVER_REQ,   0 , NULL},                  { GTPC_V1_IE_BSS,               0 , NULL},
        { GTPC_V1_IE_CELL_ID,           0 , NULL},                  { GTPC_V1_IE_PDU_NUMBERS,       0 , NULL},
        { GTPC_V1_IE_BSSGP_CAUSE,       0 , NULL},                  { GTPC_V1_IE_REQUIRED_MBMS_BEARER_CAP,  0 , NULL},
        { GTPC_V1_IE_RIM_ROUTING_ADDRESS_DISC,  0 , NULL},          { GTPC_V1_IE_PFCS,              0 , NULL},
        { GTPC_V1_IE_PS_HANDOVER_XID,   0 , NULL},                  { GTPC_V1_IE_MS_INFO_CHANGE,    0 , NULL},
        { GTPC_V1_IE_DIRECT_TUNNEL_FLAGS,  0 , NULL},               { GTPC_V1_IE_CORRELATION_ID,    0 , NULL},
        { GTPC_V1_IE_BEARER_CTRL_MODE,  0 , NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        {  0,  0, NULL}, {  0,  0, NULL},
        { GTPC_V1_IE_CHARGING_GATEWAY_ADDRESS, 0 , NULL},
        {  0,  0, NULL}, {  0,  0, NULL}, {  0,  0, NULL},
        { GTPC_V1_IE_PRIVATE_EXTENSION, 0 , NULL},
};

/**
  gtpc-v1:message table \n
 *******************************************************************************
 gtpc-v1 messge table: gtpc-v1\n
 *******************************************************************************
 @return messge_in_gtpc_v1_ptr  message table on gtpc-v1 : NULL==error
 */
const messge_in_gtpc_v1_ptr gtpcv1_message_table(void){
    return(msgs_gtpc_v1);
}

/**
  event : GTPC Version1 Any Request\n
 *******************************************************************************
 Rx -> gtpc version1 request \n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_v1_any_req(packet_ptr pckt, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    gtp_packet_ptr  packet = NULL;
    U16             gtph_seqno = 0;
    U16             offset,extlen,datalen;
    U8              *payload,*datap;
    gtpc_parse_state_t parse_state;
    const struct messge_in_gtpc_v1* pmsg;
    //
    PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpc_v1_any_req..(%p : %p : %u/%p)\n", pnode, (void*)pthread_self(), pnode->index, pckt);

    //
    bzero(&parse_state, sizeof(parse_state));

    // GTPC Version1 [CreatePDPContext/ UpdatePDPContext/ DeletePDPContext]
    offset = sizeof(gtpc_v1_header_t);
    // save sequnce number
    if (gtpc_v1_header(pckt)->f.gtpc_v1_flags.sequence){
        memcpy(&gtph_seqno, ((char*)pckt->data) + offset, sizeof(gtph_seqno));
        parse_state.gtph_seqno = ntohs(gtph_seqno);
    }
    if (gtpc_v1_header(pckt)->f.gtpc_v1_flags.sequence ||
        gtpc_v1_header(pckt)->f.gtpc_v1_flags.npdu ||
        gtpc_v1_header(pckt)->f.gtpc_v1_flags.extension){
        offset += 4;
    }
    if (gtpc_v1_header(pckt)->f.gtpc_v1_flags.extension){
        payload = ((U8*)pckt->data)+offset;
        for(;offset < pckt->datalen;){
            extlen = (*(((U8*)pckt->data)+offset))<<2;
            offset += extlen;
            // next extension?
            if (*(((char*)pckt->data)+offset-1) == 0){
                break;
            }
        }
    }
    for(;offset < pckt->datalen;){
        payload = ((U8*)pckt->data)+offset;
        pmsg = &msgs_gtpc_v1[(*payload)];
        //
        if (pmsg->type == 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid type(%02x/%02x).", pmsg->type, (*payload));
            return(ERR);
        }
        if (pmsg->len <= 0){
            // variable length
            memcpy(&extlen, payload + sizeof(U8), sizeof(extlen));
            datalen = ntohs(extlen);
            extlen = ntohs(extlen) + sizeof(U8) + sizeof(U16);
            datap = payload;
        }else{
            // fixed length
            extlen = pmsg->len;
            datalen = extlen - sizeof(U8);
            datap = payload;
        }
        if (pmsg->parser != NULL){
            if (((iterate_func)(pmsg->parser))(pmsg->type, datap, datalen, &parse_state) != 0){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid parser(%02x/%02x).", pmsg->type, (*payload));
                return(ERR);
            }
        }
        offset += extlen;
    }
    parse_state.gtph_teid = gtpc_v1_header(pckt)->tid;
    parse_state.reqtype_v1 = gtpc_v1_header(pckt)->type;
    switch(parse_state.reqtype_v1){
        case GTPC_V1_MSG_ECHO_REQ:
        case GTPC_V1_MSG_NODE_ALIVE_REQ:
        case GTPC_V1_MSG_REDIRECTION_REQ:
        case GTPC_V1_MSG_CREATE_PDP_CONTEXT_REQ:
        case GTPC_V1_MSG_UPDATE_PDP_CONTEXT_REQ:
        case GTPC_V1_MSG_DELETE_PDP_CONTEXT_REQ:
        case GTPC_V1_MSG_INITIATE_PDP_CONTEXT_ACTIVATION_REQ:
        case GTPC_V1_MSG_PDU_NOTIFICATION_REQ:
        case GTPC_V1_MSG_PDU_NOTIFICATION_REJECT_REQ:
            parse_state.reqtype_v1 ++;
            break;
        default:
            PGW_LOG(PGW_LOG_LEVEL_ERR, "not implemented.message(%02x).", parse_state.reqtype_v1);
            return(ERR);
    }
    //
    if (gtpc_alloc_packet(&packet, GTPC_UNKNOWN) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_alloc_packet(%p : %u).", pckt->data, pckt->datalen);
        return(ERR);
    }
    // save peer address
    packet->saddrlen = pckt->saddrlen;
    memcpy(&packet->saddr, &pckt->saddr, pckt->saddrlen);
    packet->caddrlen = pckt->caddrlen;
    memcpy(&packet->caddr, &pckt->caddr, pckt->caddrlen);
    // validate required parameters
    if (gtpc_v1_header(pckt)->type == GTPC_V1_MSG_CREATE_PDP_CONTEXT_REQ){
        if ((parse_state.flag_v1 & REQUIRED_CREATE_PDP)!=REQUIRED_CREATE_PDP){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. create pdp required (%u).", parse_state.flag_v1);
            return(_denied(packet, pnode, &parse_state));
        }
        if (_on_create_pdp_ctx(&parse_state, ext) == OK){
            return(_accept_create_pdp_ctx(packet, pnode, &parse_state));
        }
#if 1
    }else if (gtpc_v1_header(pckt)->type == GTPC_V1_MSG_UPDATE_PDP_CONTEXT_REQ){
        if ((parse_state.flag_v1 & REQUIRED_UPDATE_PDP)!=REQUIRED_UPDATE_PDP){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. update pdp required. (%u).", parse_state.flag_v1);
            return(_denied(packet, pnode, &parse_state));
        }
        if (_on_update_pdp_ctx(&parse_state, ext) == OK){
            return(_accept_update_pdp_ctx(packet, pnode, &parse_state));
        }
    }else if (gtpc_v1_header(pckt)->type == GTPC_V1_MSG_DELETE_PDP_CONTEXT_REQ){
        if ((parse_state.flag_v1 & REQUIRED_DELETE_PDP)!=REQUIRED_DELETE_PDP){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. delete pdp required. (%u).", parse_state.flag_v1);
            return(_denied(packet, pnode, &parse_state));
        }
        if (_on_delete_pdp_ctx(&parse_state, ext) == OK){
            return(_accept_delete_pdp_ctx(packet, pnode, &parse_state));
        }
#endif
    }
    return(_denied(packet, pnode, &parse_state));
}


/**
  gtpc version1 create pdp context \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   cs     processing container
 @param[in]   ext    option address
 @return int  0==OK,0!=error
 */
RETCD _on_create_pdp_ctx(struct gtpc_parse_state* cs, void* ext){
    U64 msisdn_num, imsi_num;
    DBPROVIDER_BIND  lookup_bind[1];
    struct in_addr  addr;
    char sql[512] = {0},cip[32] = {0},uip[32] = {0};
    node_ptr    pnode = (node_ptr)ext;
    delete_session_node_ext_ptr tnep = (delete_session_node_ext_ptr)pnode->node_opt;

    bzero(lookup_bind, sizeof(lookup_bind));

    // find by imsi
    // valid Create Session Request Packet
    msisdn_num = gtpc_digits_get(cs->msisdn.digits, GTPC_MSISDN_LEN);
    imsi_num = gtpc_digits_get(cs->imsi.digits, GTPC_IMSI_LEN);

    // find by msisdn
    lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    lookup_bind[0].buffer = &imsi_num;
    lookup_bind[0].is_null = 0;
    //
    pthread_mutex_lock(&__mysql_mutex);
    if (DBPROVIDER_STMT_RESET(tnep->stmt_v1) != 0) { pgw_panic("failed. mysql_stmt_reset(%p : %u).", tnep->stmt_v1, (unsigned)imsi_num); }
    if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt_v1, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p : %u).", tnep->stmt_v1, (unsigned)imsi_num); }
    if (DBPROVIDER_EXECUTE(tnep->stmt_v1, tnep->bind.bind, on_tunnel_record, cs) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
    pthread_mutex_unlock(&__mysql_mutex);
    //
    if (!(cs->flag & HAVE_RECORD)){
        // not exists contract information for imsi, reply denied
        PGW_LOG(PGW_LOG_LEVEL_ERR, "missing imsi.(%llu)\n", imsi_num);
        return(ERR);
    }
    // supported only ipv4(SGW notified)
    memcpy(&addr, cs->c_teid.blk, 4);
    snprintf(cip, sizeof(cip) - 1,"%s", inet_ntoa(addr));
    memcpy(&addr, cs->u_teid.blk, 4);
    snprintf(uip, sizeof(uip) - 1,"%s", inet_ntoa(addr));

    // update response parameters by Create PDP Contexgt Request
    snprintf(sql, sizeof(sql)-1, CREATE_PDP_CONTEXT_UPD_SQL,
             cs->c_teid.teid_grekey, cip,
             cs->u_teid.teid_grekey, uip,
             0,
             0,
             cs->pgw_teid,
             imsi_num,
             imsi_num);
    if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
        // could not save to database(SGW data) -> reply denied.
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s)\n", sql);
        return(ERR);
    }
    return(OK);
}
/**
  gtpc version1 update pdp context \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   cs     processing container
 @param[in]   ext    option address
 @return int  0==OK,0!=error
 */
RETCD _on_update_pdp_ctx(struct gtpc_parse_state* cs, void* ext){
    U64 teid_u64 = 0;
    DBPROVIDER_BIND  lookup_bind[1];
    node_ptr    pnode = (node_ptr)ext;
    struct in_addr  addr;
    delete_session_node_ext_ptr tnep = (delete_session_node_ext_ptr)pnode->node_opt;
    char sql[512] = {0},cip[32] = {0},uip[32] = {0};

    // find by teid
    teid_u64 = ntohl(cs->gtph_teid);
    lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    lookup_bind[0].buffer = &teid_u64;
    lookup_bind[0].is_null = 0;
    //
    pthread_mutex_lock(&__mysql_mutex);
    if (DBPROVIDER_STMT_RESET(tnep->stmt_v1_mb) != 0) { pgw_panic("failed. mysql_stmt_reset(%p).", tnep->stmt_v1_mb); }
    if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt_v1_mb, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p).", tnep->stmt_v1_mb); }
    if (DBPROVIDER_EXECUTE(tnep->stmt_v1_mb, tnep->bind.bind, on_tunnel_record, cs) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
    pthread_mutex_unlock(&__mysql_mutex);
    //
    if (!(cs->flag & HAVE_RECORD)){
        // not found teid -> response denied
        PGW_LOG(PGW_LOG_LEVEL_ERR, "missing teid(%u).\n", cs->gtph_teid);
        return(ERR);
    }
    // supported only ipv4(SGW notified)
    memcpy(&addr, cs->c_teid.blk, 4);
    snprintf(cip, sizeof(cip) - 1,"%s", inet_ntoa(addr));
    memcpy(&addr, cs->u_teid.blk, 4);
    snprintf(uip, sizeof(uip) - 1,"%s", inet_ntoa(addr));
    if ((cs->flag_v1&(HAVE_V1_TEID_DATA|HAVE_V1_TEID_CTRL)) == (HAVE_V1_TEID_DATA|HAVE_V1_TEID_CTRL)){
        snprintf(sql, sizeof(sql)-1, UPDATE_PDP_CONTEXT_UPD_SQL,
                cs->c_teid.teid_grekey, cip,
                cs->u_teid.teid_grekey, uip,
                cs->imsi_num);
        if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
            return(ERR);
        }
        // use new teid
        cs->sgw_c_teid = ntohl(cs->c_teid.teid_grekey);
    }else{
        // use exists teid at database
        cs->sgw_c_teid = ntohl(cs->sgw_c_teid);
    }
    return(OK);
}
/**
  gtpc version1 delete pdp context \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   cs     processing container
 @param[in]   ext    option address
 @return int  0==OK,0!=error
 */
RETCD _on_delete_pdp_ctx(struct gtpc_parse_state* cs, void* ext){
    U64 teid_u64 = 0;
    DBPROVIDER_BIND  lookup_bind[1];
    node_ptr    pnode = (node_ptr)ext;
    delete_session_node_ext_ptr tnep = (delete_session_node_ext_ptr)pnode->node_opt;
    char sql[512] = {0};

    // find by teid
    teid_u64 = cs->gtph_teid;
    lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    lookup_bind[0].buffer = &teid_u64;
    lookup_bind[0].is_null = 0;
    //
    pthread_mutex_lock(&__mysql_mutex);
    if (DBPROVIDER_STMT_RESET(tnep->stmt) != 0) { pgw_panic("failed. mysql_stmt_reset(%p).", tnep->stmt); }
    if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p).", tnep->stmt); }
    if (DBPROVIDER_EXECUTE(tnep->stmt, tnep->bind.bind, on_tunnel_record, cs) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
    pthread_mutex_unlock(&__mysql_mutex);
    //
    if (!(cs->flag & HAVE_RECORD)){
        // not found teid -> response denied
        PGW_LOG(PGW_LOG_LEVEL_ERR, "missing teid(%u).\n", cs->gtph_teid);
        return(ERR);
    }
    // update response parameters by Delete Bearer Request
    snprintf(sql, sizeof(sql)-1, DELETE_SESSION_UPD_SQL, cs->gtph_teid, cs->nsapi);
    if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
        return(ERR);
    }

    return(OK);
}



/**
  gtpc version1 response accepted \n
 *******************************************************************************
 Create PDP Context Response\n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     processing container
 @return int  0==OK,0!=error
 */
RETCD _accept_create_pdp_ctx(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs) {
    packet_ptr cpckt;
    gtp_packet_ptr lpckt;
    U8  type;
    U16 seqno,len;
    U32 ueipv;
    gtpc_cause_v1_t cause;
    gtpc_reordering_v1_t reorder;
    gtpc_recovery_v1_t recvr;
    gtpc_teid_v1_t teid;
    gtpc_end_user_address_v1_t endua;
    gtpc_gsn_address_v1_t gsnaddr;
    gtpc_pco_v1_t   pco;
    gtpc_nsapi_v1_t nsapi;
    gtpc_charginid_v1_t charging;
    //
    if(gtpc_v1_alloc_packet(&lpckt, cs->reqtype_v1) != OK) { pgw_panic("failed. gtpc_alloc_packet"); }
    if(lpckt) { gtpc_copy_packet_addr(lpckt, rpckt); }
    gtpc_free_packet(&rpckt);

    cs->sgw_c_teid = ntohl(cs->c_teid.teid_grekey);

    // header (sequence number)
    seqno = cs->gtph_seqno;
    seqno = htons(seqno);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(seqno)"); }
    seqno = 0;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(extend)"); }
    // cause
    cause.type = GTPC_V1_IE_CAUSE;
    cause.cause = GTPC_V1_CAUSE_ACCEPTED;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &cause, sizeof(cause)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(cause)"); }
    // reordering
    reorder.type = GTPC_V1_IE_REORDERING_REQ;
    reorder.value = 0;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &reorder, sizeof(reorder)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(reorder)"); }

    // recovery
    // FIXME: set re-start count from parameter-table
    recvr.type = GTPC_V1_IE_RECOVERY;
    recvr.counter = 1;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &recvr, sizeof(recvr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(recovr)"); }
    // teid : data i
    teid.type = GTPC_V1_IE_TEID_DATA_I;
    teid.teid = htonl(cs->pgw_teid);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &teid, sizeof(teid)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(teid_u)"); }
    // teid : control
    teid.type = GTPC_V1_IE_TEID_CTRL_PLANE;
    teid.teid = htonl(cs->pgw_teid);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &teid, sizeof(teid)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(teid_c)"); }

    // nsapi
    if(cs->flag_v1 & HAVE_V1_NSAPI) {
        nsapi.type = GTPC_V1_IE_NSAPI;
        nsapi.nsapi_value = cs->nsapi;
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) &nsapi, sizeof(nsapi)) != OK) {
            pgw_panic("failed. gtpc_v1_append_item.(nsapi)");
        }
    }
    // charging id
    charging.type = GTPC_V1_IE_CHARGING_ID;
    charging.id = 0;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &charging, sizeof(charging)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(charging)"); }

    // end user address (ipv6:not supported)
    endua.type = GTPC_V1_IE_END_USER_ADDRESS;
    endua.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(endua));
    endua.reserved = 0xf;
    endua.pdp_type_organization = GTPC_V1_EUA_ORGANIZATION_IETF;
    endua.pdp_type_number = GTPC_V1_EUA_NUMBER_IP;
    //
    ueipv = ntohl(cs->ue_ipv_n.s_addr);
    ueipv |= (PGW_IP8 << 24);
    ueipv = htonl(ueipv);
    memcpy(&endua.pdp_address, &ueipv, sizeof(ueipv));
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &endua, sizeof(endua)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(endua)"); }

    // pco
    if (cs->flag_v1 & HAVE_V1_PCO){
        if (setup_pco_v1(&pco, cs) == OK){
            len = (ntohs(pco.len) + sizeof(U8) + sizeof(U16));
            if(gtpc_v1_append_item(lpckt, 0, (U8 *) &pco, len) != OK) { pgw_panic("failed. gtpc_v1_append_item.(pco)"); }
        }
    }

    // ggsn address for control plane
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_c_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(gsnaddr-ctrl)"); }

    // ggsn address for user traffic
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_u_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(gsnaddr-data)"); }

    // alternative ggsn address for control plane
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_c_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(alternative gsnaddr-ctrl)"); }

    // alternative ggsn address for user traffic
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_u_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(alternative gsnaddr-data)"); }


    // qos receive したqosをそのまま受け容れる
    // no related between MTU ,and SDU from 3GPP TS 23.107 V14.0.0 (2017-03)
    if (cs->flag_v1 & HAVE_V1_QOS) {
        type = GTPC_V1_IE_QOS;
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) &type, sizeof(type)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(qos - type)"); }
        len = htons(cs->qos_len);
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) &len, sizeof(len)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(qos - len)"); }
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) cs->qos, cs->qos_len) != OK) { pgw_panic("failed. gtpc_v1_append_item.(qos - payload)"); }
    }
    gtpc_v1_header_(lpckt)->tid = htonl(cs->sgw_c_teid);
    // convert gtp_packet -> packet
    if ((cpckt = gtpc_convert_to_packet(lpckt)) == NULL){
        pgw_panic("failed. gtpc_convert_to_packet.");
    }
    // swap address.
    pgw_swap_address(cpckt);
    // enqueue packet into reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  gtpc v1 denied response).");
    }
    return(OK);
}

/**
  gtpc version1 response accepted \n
 *******************************************************************************
 Update PDP Context Response\n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     processing container
 @return int  0==OK,0!=error
 */
RETCD _accept_update_pdp_ctx(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs){
    packet_ptr cpckt;
    gtp_packet_ptr lpckt;
    U8  type;
    U16 seqno,len;
    gtpc_cause_v1_t cause;
    gtpc_recovery_v1_t recvr;
    gtpc_teid_v1_t teid;
    gtpc_charginid_v1_t charging;
    gtpc_pco_v1_t   pco;
    gtpc_gsn_address_v1_t gsnaddr;
    //
    if(gtpc_v1_alloc_packet(&lpckt, cs->reqtype_v1) != OK) { pgw_panic("failed. gtpc_alloc_packet"); }
    if(lpckt) { gtpc_copy_packet_addr(lpckt, rpckt); }
    gtpc_free_packet(&rpckt);

    // header (sequence number)
    seqno = cs->gtph_seqno;
    seqno = htons(seqno);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(seqno)"); }
    seqno = 0;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(extend)"); }
    // cause
    cause.type = GTPC_V1_IE_CAUSE;
    cause.cause = GTPC_V1_CAUSE_ACCEPTED;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &cause, sizeof(cause)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(cause)"); }

    // recovery
    // FIXME: set re-start count from parameter-table
    recvr.type = GTPC_V1_IE_RECOVERY;
    recvr.counter = 1;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &recvr, sizeof(recvr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(recovr)"); }
    // teid : data i
    teid.type = GTPC_V1_IE_TEID_DATA_I;
    teid.teid = htonl(cs->pgw_teid);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &teid, sizeof(teid)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(teid_u)"); }
#if 1
    // teid : control
    teid.type = GTPC_V1_IE_TEID_CTRL_PLANE;
    teid.teid = htonl(cs->pgw_teid);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &teid, sizeof(teid)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(teid_c)"); }
#endif
    // charging id
    charging.type = GTPC_V1_IE_CHARGING_ID;
    charging.id = 0;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &charging, sizeof(charging)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(charging)"); }
    // pco
    if (cs->flag_v1 & HAVE_V1_PCO){
        if (setup_pco_v1(&pco, cs) == OK){
            len = (ntohs(pco.len) + sizeof(U8) + sizeof(U16));
            if(gtpc_v1_append_item(lpckt, 0, (U8 *) &pco, len) != OK) { pgw_panic("failed. gtpc_v1_append_item.(pco)"); }
        }
    }
    // ggsn address for control plane
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_c_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(gsnaddr-ctrl)"); }

    // ggsn address for user traffic
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_u_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(gsnaddr-data)"); }

#if 0
    // alternative ggsn address for control plane
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_c_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(alternative gsnaddr-ctrl)"); }

    // alternative ggsn address for user traffic
    gsnaddr.type = GTPC_V1_IE_GSN_ADDRESS;
    gsnaddr.len = htons(GTPCV1_IE_VARIABLE_HEADER_LEN(gsnaddr));
    memcpy(&gsnaddr.adrs, &cs->pgw_u_ipv_n, GTPC_V1_GSNADDR_LEN);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &gsnaddr, sizeof(gsnaddr)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(alternative gsnaddr-data)"); }
#endif
    // accept received qos in directly.
    // no related between MTU ,and SDU from 3GPP TS 23.107 V14.0.0 (2017-03)
    if (cs->flag_v1 & HAVE_V1_QOS) {
        type = GTPC_V1_IE_QOS;
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) &type, sizeof(type)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(qos - type)"); }
        len = htons(cs->qos_len);
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) &len, sizeof(len)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(qos - len)"); }
        if(gtpc_v1_append_item(lpckt, 0, (U8 *) cs->qos, cs->qos_len) != OK) { pgw_panic("failed. gtpc_v1_append_item.(qos - payload)"); }
    }

    gtpc_v1_header_(lpckt)->tid = htonl(cs->sgw_c_teid);
    // convert gtp_packet -> packet
    if ((cpckt = gtpc_convert_to_packet(lpckt)) == NULL){
        pgw_panic("failed. gtpc_convert_to_packet.");
    }
    // swap address
    pgw_swap_address(cpckt);
    // enqueue packet into reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  gtpc v1 denied response).");
    }
    return(OK);
}



/**
  gtpc version1 response accepted \n
 *******************************************************************************
 Delete PDP Context Response\n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     processing container
 @return int  0==OK,0!=error
 */
RETCD _accept_delete_pdp_ctx(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs){
    packet_ptr cpckt;
    gtp_packet_ptr lpckt;
    U16 seqno,len;
    gtpc_cause_v1_t cause;
    gtpc_pco_v1_t   pco;
    //
    if(gtpc_v1_alloc_packet(&lpckt, cs->reqtype_v1) != OK) { pgw_panic("failed. gtpc_alloc_packet"); }
    if(lpckt) { gtpc_copy_packet_addr(lpckt, rpckt); }
    gtpc_free_packet(&rpckt);
    //
    cs->sgw_c_teid = ntohl(cs->sgw_c_teid);


    // header (sequence number)
    seqno = cs->gtph_seqno;
    seqno = htons(seqno);
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(seqno)"); }
    seqno = 0;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(extend)"); }
    // cause
    cause.type = GTPC_V1_IE_CAUSE;
    cause.cause = GTPC_V1_CAUSE_ACCEPTED;
    if(gtpc_v1_append_item(lpckt, 0, (U8 *) &cause, sizeof(cause)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(cause)"); }
    // pco
    if (cs->flag_v1 & HAVE_V1_PCO){
        if (setup_pco_v1(&pco, cs) == OK){
            len = (ntohs(pco.len) + sizeof(U8) + sizeof(U16));
            if(gtpc_v1_append_item(lpckt, 0, (U8 *) &pco, len) != OK) { pgw_panic("failed. gtpc_v1_append_item.(pco)"); }
        }
    }
    gtpc_v1_header_(lpckt)->tid = htonl(cs->sgw_c_teid);
    // convert gtp_packet -> packet
    if ((cpckt = gtpc_convert_to_packet(lpckt)) == NULL){
        pgw_panic("failed. gtpc_convert_to_packet.");
    }
    // swap address
    pgw_swap_address(cpckt);
    // enqueue packet into reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  gtpc v1 denied response).");
    }
    return(OK);
}



/**
  gtpc version1 response Denied \n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   rpckt   1packet 
 @param[in]   pnode  node object
 @param[in]   cs     processing container
 @return int  0==OK,0!=error
 */
RETCD _denied(gtp_packet_ptr rpckt, node_ptr pnode,struct gtpc_parse_state* cs){
    packet_ptr          cpckt;
    gtp_packet_ptr      lpckt;
    U16                 seqno;
    gtpc_cause_v1_t     cause;
    //
    if (gtpc_v1_alloc_packet(&lpckt, cs->reqtype_v1) != OK){
        pgw_panic("failed. gtpc_alloc_packet");
    }
    if (lpckt){ gtpc_copy_packet_addr(lpckt, rpckt); }
    gtpc_free_packet(&rpckt);
    //
    cs->sgw_c_teid = ntohl(cs->c_teid.teid_grekey);
    //
    seqno = cs->gtph_seqno;
    seqno = htons(seqno);
    if (gtpc_v1_append_item(lpckt, 0, (U8*)&seqno, sizeof(seqno)) != OK){ pgw_panic("failed. gtpc_v1_append_item.(seqno)"); }
    seqno = 0;
    if (gtpc_v1_append_item(lpckt, 0, (U8*)&seqno, sizeof(seqno)) != OK){ pgw_panic("failed. gtpc_v1_append_item.(extend)"); }
    cause.type = GTPC_V1_IE_CAUSE;
    cause.cause = GTPC_V1_CAUSE_NO_RESOURCE_AVAILABLE;
    if (gtpc_v1_append_item(lpckt, 0, (U8*)&cause, sizeof(cause)) != OK){ pgw_panic("failed. gtpc_v1_append_item."); }
    gtpc_v1_header_(lpckt)->tid = htonl(cs->sgw_c_teid);

    // convert gtp_packet -> packet
    if ((cpckt = gtpc_convert_to_packet(lpckt)) == NULL){ pgw_panic("failed. gtpc_convert_to_packet."); }
    // swap address
    pgw_swap_address(cpckt);
    // enqueue packet into reply ring
    if (pgw_enq(pnode, EGRESS, cpckt) != OK){
        pgw_panic("failed. enq(_denied  gtpc v1 denied response).");
    }
    return(OK);
}

/**
  Setup PCO Response\n
 *******************************************************************************
 gtpc version 1\n
 *******************************************************************************
 @param[in]   pco   pco-v1 struct
 @param[in]   cs    parse status
 @return int  0==OK,0!=error
 */
RETCD setup_pco_v1(gtpc_pco_v1_ptr pco, struct gtpc_parse_state* cs){
    U8      pcobuffer[255] = {0};
    U16     pcolen = 0;
    // pco request is exists
    if (cs->flag_v1&(PCO_ALL|HAVE_PCO_IPCP_P_DNS|HAVE_PCO_IPCP_S_DNS)){
        bzero(pco, sizeof(*pco));
        // DNS query
        if ((cs->flag_v1&HAVE_PCO_DNS_IPV64)==HAVE_PCO_DNS_IPV64){
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPV4_VAL, sizeof(PCO_DNS_IPV4_VAL));
            pcolen += sizeof(PCO_DNS_IPV4_VAL);
        }else if ((cs->flag_v1&HAVE_PCO_DNS_IPV6)==HAVE_PCO_DNS_IPV6){
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPV6_VAL, sizeof(PCO_DNS_IPV6_VAL));
            pcolen += sizeof(PCO_DNS_IPV6_VAL);
        }else if ((cs->flag_v1&HAVE_PCO_DNS_IPV4)==HAVE_PCO_DNS_IPV4){
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPV4_VAL, sizeof(PCO_DNS_IPV4_VAL));
            pcolen += sizeof(PCO_DNS_IPV4_VAL);
        }
        // MTU notified
        if ((cs->flag_v1&HAVE_PCO_NONIP_LINK_MTU)==HAVE_PCO_NONIP_LINK_MTU){
            pco->pco_head.extension = GTPC_PCO_EXTENSION_ON;
            memcpy(&pcobuffer[pcolen], PCO_MTU_NONIP_VAL, sizeof(PCO_MTU_NONIP_VAL));
            pcolen += sizeof(PCO_MTU_NONIP_VAL);
        }else if ((cs->flag_v1&HAVE_PCO_IPV4_LINK_MTU)==HAVE_PCO_IPV4_LINK_MTU){
            pco->pco_head.extension = GTPC_PCO_EXTENSION_ON;
            memcpy(&pcobuffer[pcolen], PCO_MTU_IPV4_VAL, sizeof(PCO_MTU_IPV4_VAL));
            pcolen += sizeof(PCO_MTU_IPV4_VAL);
        }
        // IPCP Configuration
        // from rfc1877
        if ((cs->flag_v1&(HAVE_PCO_IPCP_S_DNS|HAVE_PCO_IPCP_P_DNS))==(HAVE_PCO_IPCP_S_DNS|HAVE_PCO_IPCP_P_DNS)) {
            pco->pco_head.extension = GTPC_PCO_EXTENSION_ON;
            memcpy(&pcobuffer[pcolen], PCO_DNS_IPCP_IPV4_VAL, sizeof(PCO_DNS_IPCP_IPV4_VAL));
            pcolen += sizeof(PCO_DNS_IPCP_IPV4_VAL);
        }
        //
        if (pcolen > 1){
            bzero(pco->pco, sizeof(pco->pco));
            if ((pcolen+1) >= sizeof(pco->pco)){
                return(ERR);
            }
            pco->type = GTPC_V1_IE_PCO;
            pco->len = htons(pcolen + 1);
            memcpy(pco->pco, pcobuffer, pcolen);
            return(OK);
        }
    }
    return(ERR);
}


