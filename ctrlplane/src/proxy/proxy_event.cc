/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy_event.cc
    @brief      gtpc proxy event実装
*******************************************************************************
*******************************************************************************
    @date       created(09/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/apr/2018 
      -# Initial Version
******************************************************************************/
#include "proxy.hpp"
#include "routes.hpp"

static void _ModifyEchoReq(gtpc_header_ptr rh){
    gtpc_recovery_ptr   precv;
    // initialize restart counter to 0.
    if (rh->v2_flags.teid) {
        if (ntohs(rh->length)== ((sizeof(gtpc_header_t)-4) + sizeof(gtpc_recovery_t))){
            precv = (gtpc_recovery_ptr)(((char*)rh) + sizeof(gtpc_header_t));
            if (precv->head.type == GTPC_TYPE_RECOVERY){
                precv->recovery_restart_counter = 0;
            }
        }
    }else{
        if (ntohs(rh->length)== ((sizeof(gtpc_header_t)-8) + sizeof(gtpc_recovery_t))){
            precv = (gtpc_recovery_ptr)(((char*)rh) + (sizeof(gtpc_header_t)-4));
            if (precv->head.type == GTPC_TYPE_RECOVERY){
                precv->recovery_restart_counter = 0;
            }
        }
    }
    // convert GTPC Echo Request -> GTPC Echo Response, and forwarding
    rh->type = GTPC_ECHO_RES;
}

static int _ParseImsi(U8* rbuf, U32 rlen, U64* imsi, gtpc_parse_state_ptr cs){
    auto rh = (gtpc_header_ptr)rbuf;
    auto rhv1 = (gtpc_v1_header_ptr)rbuf;
    U16             offset,extlen,datalen;
    U8              *payload,*datap;
    gtp_packet_ptr  parsed_pkt = NULL;
    auto pmsgs = gtpcv1_message_table();
    const struct messge_in_gtpc_v1* pmsg;

    // gtpc parse 
    if (rh->v2_flags.version == GTPC_VERSION_2) {
        // gtpc - V2
        // decide delivery destination with imsi in Create Session Request
        if (gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)rbuf, rlen) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_alloc_packet_from_payload(%p : %u).", rbuf, rlen);
            return(ERR);
        }
        // imsi must be specidied.
        gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, cs);
        if ((cs->flag&HAVE_IMSI) != HAVE_IMSI){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "missing ie:imsi(%llu).", cs->flag);
            return(ERR);
        }
        if ((cs->flag&HAVE_FTEID_C) != HAVE_FTEID_C){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "missing ie:fteid-c(%llu).", cs->flag);
            gtpc_free_packet(&parsed_pkt);
            return(ERR);
        }
        gtpc_free_packet(&parsed_pkt);
    }else{
        // gtpc - V1
        offset = sizeof(gtpc_v1_header_t);
        if (rhv1->f.gtpc_v1_flags.sequence ||
            rhv1->f.gtpc_v1_flags.npdu ||
            rhv1->f.gtpc_v1_flags.extension){
            offset += 4;
        }
        if (rhv1->f.gtpc_v1_flags.extension){
            payload = ((U8*)rhv1)+offset;
            for(;offset < rlen;){
                extlen = (*(((U8*)rhv1)+offset))<<2;
                offset += extlen;
                // next extension?
                if (*(((char*)rhv1)+offset-1) == 0){
                    break;
                }
            }
        }
        for(;offset < rlen;){
            payload = ((U8*)rhv1)+offset;
            pmsg = &pmsgs[(*payload)];
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
                if (((iterate_func)(pmsg->parser))(pmsg->type, datap, datalen, cs) != 0){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid parser(%02x/%02x).", pmsg->type, (*payload));
                    return(ERR);
                }
            }
            offset += extlen;
        }
        if ((cs->flag_v1&HAVE_V1_IMSI) != HAVE_V1_IMSI){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "missing ie:imsi(%llu).", cs->flag);
            return(ERR);
        }
    }
    //
    (*imsi) = gtpc_digits_get(cs->imsi.digits, GTPC_IMSI_LEN);

    return(OK);
}


/**
  time out event\n
 *******************************************************************************
  callback\n
 *******************************************************************************
 @param[in]     sock    socket descriptor
 @param[in]     what    reason
 @param[in]     arg     user data
 */
void Proxy::OnTimeOut(evutil_socket_t sock, short what, void* arg){
    // time out 
    auto routes = (Routes*)arg;
    ASSERT(routes);
    static U64 counter = 0;
    static U64 counter_session = 0;
    //
    if (counter++ > 6000 /* 60 sec */){
        routes->KeepAlive();
        PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnTimeOut(%llu).", counter);
        counter = 0;
        //  group cache reconstruction
        if (Proxy::reload_){
            routes->Cache();
            Proxy::reload_ = 0;
        }
    }
    if (counter_session++ > 360000){
        routes->KeepAlive();
        PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnTimeOut(%llu).", counter_session);
        counter_session = 0;
        routes->StatExpire(counter_session / 100);
    }
}
/**
  External Socketreceive \n
 *******************************************************************************
  callback\n
 *******************************************************************************
 @param[in]     sock    socket descriptor
 @param[in]     what    reason
 @param[in]     arg     user data
 */
void Proxy::OnRecieveExt(evutil_socket_t sock, short what, void* arg){
    // udppacket receive 
    U8 rbuf[PCKTLEN_UDP];
    struct sockaddr_in ssa;
    unsigned int slen = sizeof(ssa);
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    ssize_t rlen;
    gtpc_header_ptr rh;
    gtpc_v1_header_ptr rhv1;
    packet_t        pckt;
    struct gtpc_parse_state    parse_state;
    auto routes = (Routes*)arg;
    U64 imsi_num;
    GROUP   gid;
    ROUTEINTS   routeints;
    int csock;
    U32 teid;
    const char* addr;
    struct sockaddr_in  caddr;
    U16 port;
    //
    ASSERT(routes);

    bzero(&ssa, sizeof(ssa));

    // read gtp header(don't proceed)
    rlen = recvfrom(sock, rbuf, sizeof(gtpc_header_t) -4, MSG_PEEK, (struct sockaddr*)&ssa, &slen);
    // when could not read to header length then, drop.
    if (rlen != (sizeof(gtpc_header_t)-4)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid read header(%d)\n", (INT)rlen);
        goto error;
    }
    rh = (gtpc_header_ptr)rbuf;
    rhv1 = (gtpc_v1_header_ptr)rbuf;
    if (ntohs(rh->length) < 4 || ntohs(rh->length) > ETHER_MAX_LEN){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid header length(%d)", ntohs(rh->length));
        goto error;
    }
    pckt.data = (U8*)rh;
    pckt.datalen = sizeof(*rh);
    // gtpcvalidate header
    if (gtpc_validate_header(&pckt)!=OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_validate_header(%d)", ntohs(rh->length));
        goto error;
    }
    pckt.datalen = 4 + ntohs(rh->length);
    if (rhv1->f.gtpc_v1_flags.version == GTPC_VERSION_1){
        if (rhv1->f.gtpc_v1_flags.sequence || rhv1->f.gtpc_v1_flags.npdu || rhv1->f.gtpc_v1_flags.extension){
            pckt.datalen += 4;
        }
    }
    // read remained paylod.
    rlen = recvfrom(sock, rbuf, pckt.datalen, 0, NULL, NULL);
    // error if could not read it all together.
    if (rlen < 4 || rlen != pckt.datalen){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not enough len(%d != %u)\n", (INT)rlen, pckt.datalen);
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        routes->StatReject(ssa.sin_addr.s_addr, 0, rlen, 0);
        return;
    }
    if (getsockname(sock, (struct sockaddr *)&sa, &salen) < 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "getsockname(%d)\n", (INT)rlen);
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        routes->StatReject(ssa.sin_addr.s_addr, 0, rlen, 0);
        return;
    }
    // create outer route dynamically with peer address(SGW : ipv4 only)
    if (routes->SetExtRoute(&ssa) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "SetExtRoute(%u)\n", (U32)ssa.sin_addr.s_addr);
        goto error;
    }
    // Create Session Request / Echo Request
    if (rh->v2_flags.version == GTPC_VERSION_2){
        if (rh->type != GTPC_CREATE_SESSION_REQ && rh->type != GTPC_ECHO_REQ) {
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid gtpc header(%u : %u).", rh->v2_flags, rh->type);
            goto error;
        }
    // Create PDP/Update PDP/Delete PDP
    }else if (rhv1->f.gtpc_v1_flags.version == GTPC_VERSION_1){
        if (rhv1->type < GTPC_V1_MSG_CREATE_PDP_CONTEXT_REQ || rhv1->type > GTPC_V1_MSG_DELETE_PDP_CONTEXT_RES){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid gtpc-v1 header(%u : %u).", rhv1->f.gtpc_v1_flags, rhv1->type);
            goto error;
        }
    }else {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid gtpc version(%u).", rh->v2_flags.version);
        goto error;
    }
    if (rh->type == GTPC_ECHO_REQ){
        // reply - socket
        csock = routes->ExtSock();
        _ModifyEchoReq(rh);
        // send
        if (pgw_send_sock(csock, &ssa, sizeof(struct sockaddr_in), rh, pckt.datalen) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed.pgw_send_sock(%u).", pckt.datalen);
            goto error;
        }
        PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnRecieveExt. Echo Req[%u]", ssa.sin_addr.s_addr);
        routes->StatAdd(ssa.sin_addr.s_addr, gid, pckt.datalen, 0, 0);
        return;
    }
    // gtpc parse [v1/v2]
    if (_ParseImsi(rbuf, pckt.datalen, &imsi_num, &parse_state) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. parse imsi(%p : %u).", rbuf, pckt.datalen);
        goto error;
    }
    if (routes->FindGroupByImsi(imsi_num, gid) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "FindGroupByImsi(%llu).", imsi_num);
        goto error;
    }
    // target route find by group
    if (routes->FindInternlRouteByGroupFromCache(gid, routeints) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "FindInternlRouteByGroupFromCache(%u).", gid);
        goto error;
    }
    memcpy(&teid, &parse_state.c_teid.teid_grekey, 4);

    // cache before route transfer (sgw.gtpc.teid , gtpc.header.seq)  => (imsi, sgw.gtpc.teid , gtpc.header.seq, sgw peer address)
    if (routes->SetSession(imsi_num, rh->q.sq_t.seqno, teid, &ssa) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "SetSession(%llu).", imsi_num);
        goto error;
    }
    // use target routes[0]
    // FIXME: for multiple target routes, least connection control
    port = (U16)routeints[0].Port();
    // target PGW address 
    if ((addr = routeints[0].Host()) == NULL){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "missing (null)Host.");
        goto error;
    }
    // internal socket
    csock = routes->IntSock();

    // set destination address
    memset(&caddr, 0, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &caddr.sin_addr.s_addr);
    // send
    if (pgw_send_sock(csock, &caddr, sizeof(struct sockaddr_in), pckt.data, pckt.datalen) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed.pgw_send_sock(%u)%s.", pckt.datalen, addr);
        goto error;
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnRecieveExt.[%u -> %s/%llu/%08x/%08x]", ssa.sin_addr.s_addr, addr, imsi_num, rh->t.teid, rh->q.seqno);
    routes->StatAdd(ssa.sin_addr.s_addr, gid, pckt.datalen, 0, 0);
    return;
error:
    PGW_LOG(PGW_LOG_LEVEL_ERR, "Proxy::OnRecieveExt. Error[%u /%llu/%08x/%08x]", ssa.sin_addr.s_addr, imsi_num, rh->t.teid, rh->q.seqno);
    routes->StatReject(ssa.sin_addr.s_addr, gid, rlen, 0);
}


/**
  Internal Socketreceive \n
 *******************************************************************************
  callback\n
 *******************************************************************************
 @param[in]     sock    socket descriptor
 @param[in]     what    reason
 @param[in]     arg     user data
 */
void Proxy::OnRecieveInt(evutil_socket_t sock, short what, void* arg){
    // udppacket receive 
    char rbuf[PCKTLEN_UDP];
    struct sockaddr_in ssa;
    unsigned int slen = sizeof(ssa);
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    ssize_t rlen;
    gtpc_header_ptr rh;
    gtpc_v1_header_ptr rhv1;
    packet_t        pckt;
    gtp_packet_ptr  parsed_pkt = NULL;
    struct gtpc_parse_state    parse_state;
    auto routes = (Routes*)arg;
    U8 ip[4] = {0};
    char iptxt[32] = {0};
    INADDR addr;
    GROUP gid;
    U64 imsi_num;
    RouteSession    sess;
    int csock;
    gtpc_recovery_ptr   precv;

    //
    ASSERT(routes);
    bzero(&ssa, sizeof(ssa));

    // read gtp header(don't proceed)
    rlen = recvfrom(sock, rbuf, sizeof(gtpc_header_t) -4, MSG_PEEK, (struct sockaddr*)&ssa, &slen);
    // when could not read to header length then , drop.
    if (rlen != (sizeof(gtpc_header_t)-4)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid read header(%d)\n", (INT)rlen);
        goto error;
    }
    rh = (gtpc_header_ptr)rbuf;
    rhv1 = (gtpc_v1_header_ptr)rbuf;
    if (ntohs(rh->length) < 4 || ntohs(rh->length) > ETHER_MAX_LEN){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid header length(%d)", ntohs(rh->length));
        goto error;
    }
    pckt.data = (U8*)rh;
    pckt.datalen = sizeof(*rh);
    // gtpcvalidate header
    if (gtpc_validate_header(&pckt)!=OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_validate_header(%d)", ntohs(rh->length));
        goto error;
    }
    pckt.datalen = 4 + ntohs(rh->length);
    if (rhv1->f.gtpc_v1_flags.version == GTPC_VERSION_1){
        if (rhv1->f.gtpc_v1_flags.sequence || rhv1->f.gtpc_v1_flags.npdu || rhv1->f.gtpc_v1_flags.extension){
            pckt.datalen += 4;
        }
    }
    // read remained payload.
    rlen = recvfrom(sock, rbuf, pckt.datalen, 0, NULL, NULL);
    // error if could not read it all together.
    if (rlen < 4 || rlen != pckt.datalen){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not enough len(%d != %u)\n", (INT)rlen, pckt.datalen);
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        goto error;
    }
    //
    if (getsockname(sock, (struct sockaddr *)&sa, &salen) < 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "getsockname(%d)\n", (INT)rlen);
        goto error;
    }
    // gtpc parse 
    // Create Session Request / Echo Request
    if (rh->v2_flags.version == GTPC_VERSION_2){
        if (rh->type != GTPC_CREATE_SESSION_RES && rh->type != GTPC_ECHO_REQ){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid gtpc header(%u : %u).", rh->v2_flags, rh->type);
            goto error;
        }
    // Create PDP/Update PDP/Delete PDP
    }else if (rhv1->f.gtpc_v1_flags.version == GTPC_VERSION_1){
        if (rhv1->type < GTPC_V1_MSG_CREATE_PDP_CONTEXT_REQ || rhv1->type > GTPC_V1_MSG_DELETE_PDP_CONTEXT_RES){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid gtpc-v1 header(%u : %u).", rhv1->f.gtpc_v1_flags, rhv1->type);
            goto error;
        }
    }else {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid gtpc version(%u).", rh->v2_flags.version);
        goto error;
    }
    if (rh->v2_flags.version == GTPC_VERSION_2 && rh->type == GTPC_ECHO_REQ){
        // reply - socket
        csock = routes->IntSock();
        _ModifyEchoReq(rh);
        // send
        if (pgw_send_sock(csock, &ssa, sizeof(struct sockaddr_in), rh, pckt.datalen) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed.pgw_send_sock(%u).", pckt.datalen);
            goto error;
        }
        PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnRecieveInt. Echo Req[%u]", ssa.sin_addr.s_addr);
        //
        routes->StatAdd(sa.sin_addr.s_addr, gid, 0, pckt.datalen, 0);
        return;
    }
    // session find by teid.
    if (routes->FindSessionBySeqnoTeid(rh->q.seqno, rh->t.teid, sess) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "FindSessionBySeqnoTeid(%08x/%08x).", rh->q.seqno, rh->t.teid);
        goto error;
    }
    // target SGW address 
    if (sess.Host() == NULL){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "missing (null)Host - sgw.");
        goto error;
    }
    // outer-side: socket
    csock = routes->ExtSock();
    // send
    if (pgw_send_sock(csock, sess.Host(), sizeof(struct sockaddr_in), pckt.data, pckt.datalen) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed.pgw_send_sock(%u).", pckt.datalen);
        goto error;
    }
    gtpc_free_packet(&parsed_pkt);
    PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnRecieveInt.[%u -> %08x/%llu/%08x/%08x]", ssa.sin_addr.s_addr, sess.Host()->sin_addr.s_addr, imsi_num, rh->t.teid, rh->q.seqno);
    //
    routes->StatAdd(sa.sin_addr.s_addr, gid, 0, pckt.datalen, 0);
    return;
error:
    if (parsed_pkt){
        gtpc_free_packet(&parsed_pkt);
    }
    PGW_LOG(PGW_LOG_LEVEL_ERR, "Proxy::OnRecieveInt. Error[%u /%llu/%08x/%08x]", ssa.sin_addr.s_addr, imsi_num, rh->t.teid, rh->q.seqno);
    routes->StatReject(sa.sin_addr.s_addr, gid, 0, rlen);
}


/**
  stats Socketreceive \n
 *******************************************************************************
  callback\n
 *******************************************************************************
 @param[in]     sock    socket descriptor
 @param[in]     what    reason
 @param[in]     arg     user data
 */
void Proxy::OnRecieveStats(evutil_socket_t sock, short what, void* arg){
    // udppacket receive 
    U8 rbuf[PCKTLEN_UDP];
    struct sockaddr_in ssa;
    unsigned int slen = sizeof(ssa);
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    ssize_t rlen;
    auto routes = (Routes*)arg;
    int csock,n;
    auto reslen = sizeof(proxy_stats_header_t)+sizeof(proxy_stats_result_counter_t);
    proxy_stats_header_ptr  rh = NULL;
    proxy_stats_result_counter_ptr rc = NULL;
    proxy_stats_request_data_ptr rq = NULL;
    U32 filter_ip = (U32)-1;
    U16 filter_groupid = (U16)-1;

    //
    ASSERT(routes);

    // read stats header(don't proceed)
    rlen = recvfrom(sock, rbuf, sizeof(proxy_stats_header_t), MSG_PEEK, (struct sockaddr*)&ssa, &slen);
    // when could not read to header length then, drop.
    if (rlen != (sizeof(proxy_stats_header_t))){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid read header(%d)\n", (INT)rlen);
        goto error;
    }
    rh = (proxy_stats_header_ptr)rbuf;
    rc = (proxy_stats_result_counter_ptr)&rbuf[sizeof(proxy_stats_header_t)];
    if (ntohs(rh->length) != sizeof(proxy_stats_request_data_t)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid header length(%d)", ntohs(rh->length));
        goto error;
    }
    // read payload.
    rlen = recvfrom(sock, rbuf, ntohs(rh->length), 0, NULL, NULL);
    // error if could not read it all together.
    if (rlen != ntohs(rh->length)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not enough len(%d != %u)\n", (INT)rlen, ntohs(rh->length));
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        return;
    }
    if (getsockname(sock, (struct sockaddr *)&sa, &salen) < 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "getsockname(%d)\n", (INT)rlen);
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        return;
    }
    // validate request packet
    if (rh->magic != PSTAT_MAGIC){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid magic(%u != %u)\n", PSTAT_MAGIC, rh->magic);
        return;
    }
    if ((rh->type&0xF0) != PSTAT_REQUEST){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid request type(%02x)\n", rh->type);
        return;
    }
    if (rh->extlength != 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not implemented extra length(%04x)\n", rh->extlength);
        return;
    }
    // request
    rq = (proxy_stats_request_data_ptr)&rbuf[sizeof(proxy_stats_header_t) + (rh->extlength << 2)];
    if (PSTAT_RTYPE_MASK(rq->param.type) == (PSTAT_RTYPE_GROUPID|PSTAT_RTYPE_IP)){
        filter_ip = rq->param.ip;
        filter_groupid = rq->param.groupid;
    }else if (PSTAT_RTYPE_MASK(rq->param.type) == PSTAT_RTYPE_IP){
        filter_ip = rq->param.ip;
    }
    // construct response header
    rh->magic  = PSTAT_MAGIC;
    rh->length = htons(reslen);
    rh->type   = (rh->type&0x0F);
    rh->extlength = 0;
    rh->time    = (U32)time(NULL);

    // construct response payload data.
    bzero(rc, sizeof(*rc));
    for(n = 0;n < PSTAT_COUNT_MAX;n++){
        rc->val[n] = htonll(routes->StatCount(PSTAT_COUNT(n), filter_ip, filter_groupid));
    }
    //
    csock = routes->StatsSock();
    // send
    if (pgw_send_sock(csock, &ssa, sizeof(struct sockaddr_in), rh, reslen) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed.pgw_send_sock(%u).", reslen);
        goto error;
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "Proxy::OnRecieveStats. Req[%08x/%u]", ssa.sin_addr.s_addr, reslen);
    return;
error:
    PGW_LOG(PGW_LOG_LEVEL_ERR, "Proxy::OnRecieveStats. Error");
}
