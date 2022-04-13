/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_gtpu_other.c
    @brief      gtpu : other receive event
*******************************************************************************
    - error indication\n
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
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#ifndef IPP_ICMP6
#define  IPP_ICMP6   58
#endif
#ifndef IPV6_VERSION
#define IPV6_VERSION 0x60
#endif

#ifndef ND_RA_FLAG_RTPREF_LOW
#define ND_RA_FLAG_RTPREF_LOW   0x18 /* 00011000 */
#endif





typedef struct icmpv6_pseudo_header{
    struct in6_addr     srcaddr;
    struct in6_addr     dstaddr;
    U16                 payloadlen;
    U16                 next;
}__attribute__ ((packed)) icmpv6_pseudo_header_t,*icmpv6_pseudo_header_ptr;

static unsigned _checksum(const void *data, unsigned short len, unsigned sum){
    unsigned  _sum   = sum;
    unsigned  _count = len;
    unsigned short* _addr  = (unsigned short*)data;
    //
    while( _count > 1 ) {
        _sum += ntohs(*_addr);
        _addr++;
        _count -= 2;
    }
    if(_count > 0 ){
        _sum += ntohs(*_addr);
    }
    while (_sum>>16){
        _sum = (_sum & 0xffff) + (_sum >> 16);
    }
    return(~_sum);
}
static unsigned short _wrapsum(unsigned sum){
    sum = sum & 0xFFFF;
    return (htons(sum));
}

/**
  event : GTPU other Request\n
 *******************************************************************************
 Rx -> gtpu other req\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpu_other_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m,icmp6_rs = ERR;
    packet_ptr  pkts[32] = {NULL};
    packet_ptr  packet = NULL;
    struct ip6_hdr  *ip6h = NULL;
    struct icmp6_hdr* icmp6 = NULL;
    struct nd_router_advert* ndra = NULL;
    struct nd_opt_mtu* ndom  = NULL;
    struct nd_opt_prefix_info* ndopi = NULL;
    struct gtpc_parse_state parse_state;
    gtpu_other_node_ext_ptr tnep = (gtpu_other_node_ext_ptr)pnode->node_opt;
    gtpu_err_indication_ptr perrind;
    U64         pgw_teid = 0;
    DBPROVIDER_BIND  lookup_bind[1];
    unsigned    checksum = 0;
    char sql[512] = {0};
    char cip[32] = {0};
    struct in_addr  addr;
    U8                        checksum_buffer[2048] = {0};
    icmpv6_pseudo_header_ptr  icmpv6_pseudo = (icmpv6_pseudo_header_ptr)checksum_buffer;
    U16         mlen = sizeof(struct icmp6_hdr) + sizeof(struct ip6_hdr);
    U8          ipv6_src_addr[16] = { 0xfe,0x80,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x02};
    U8          ipv6_dst_addr[16] = { 0xff,0x02,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        for(m = 0;m < nburst;m++){
            icmp6_rs = ERR;
            if (gtpu_header(pkts[m])->type == GTPU_G_PDU){
                // validate ICMPv6-RouterSolicitation over ipv6
                if (ntohs(gtpu_header(pkts[m])->length) >= mlen){
                    ip6h = (struct ip6_hdr*)(pkts[m]->data + sizeof(gtpu_header_t));
                    icmp6 = (struct icmp6_hdr*)(pkts[m]->data + sizeof(gtpu_header_t) + sizeof(struct ip6_hdr));
                    if (ip6h->ip6_vfc == IPV6_VERSION &&
                        ip6h->ip6_nxt == IPP_ICMP6 &&
                        ntohs(ip6h->ip6_plen) >= sizeof(struct icmp6_hdr) &&
                        icmp6->icmp6_type == ND_ROUTER_SOLICIT){
                        //
                        if (pgw_alloc_packet(&packet, sizeof(gtpu_header_t) +
                                                            sizeof(struct ip6_hdr) +
                                                            sizeof(struct nd_router_advert) +
                                                            sizeof(struct nd_opt_mtu) +
                                                            sizeof(struct nd_opt_prefix_info)) != OK){
                            pgw_panic("failed. pgw_alloc_packet.");
                        }
                        // copy all
                        memcpy(packet->data, pkts[m]->data, MIN(pkts[m]->datalen, packet->datalen));
                        // save peer address
                        packet->saddrlen = pkts[m]->saddrlen;
                        memcpy(&packet->saddr, &pkts[m]->saddr, pkts[m]->saddrlen);
                        packet->caddrlen = pkts[m]->caddrlen;
                        memcpy(&packet->caddr, &pkts[m]->caddr, pkts[m]->caddrlen);

                        //
                        ip6h    = (struct ip6_hdr*)(packet->data + sizeof(gtpu_header_t));
                        icmp6   = (struct icmp6_hdr*)(packet->data + sizeof(gtpu_header_t) + sizeof(struct ip6_hdr));
                        ndra    = (struct nd_router_advert*)icmp6;
                        ndom    = (struct nd_opt_mtu*)&ndra[1];
                        ndopi   = (struct nd_opt_prefix_info*)&ndom[1];
                        // ipv6
                        memcpy(&ip6h->ip6_src, ipv6_src_addr, sizeof(ipv6_src_addr));
                        memcpy(&ip6h->ip6_dst, ipv6_dst_addr, sizeof(ipv6_dst_addr));
                        ip6h->ip6_plen = htons(sizeof(struct nd_router_advert) +
                                               sizeof(struct nd_opt_mtu) +
                                               sizeof(struct nd_opt_prefix_info));
                        // router advertizing.
                        ndra->nd_ra_hdr.icmp6_type = ND_ROUTER_ADVERT;
                        ndra->nd_ra_hdr.icmp6_cksum = 0;
                        ndra->nd_ra_curhoplimit = 0x40;
                        ndra->nd_ra_flags_reserved = (ND_RA_FLAG_OTHER|ND_RA_FLAG_RTPREF_LOW);
                        ndra->nd_ra_router_lifetime = htons(1800);
                        ndra->nd_ra_reachable = 0;
                        ndra->nd_ra_retransmit = 0;
                        // icmpv6-option mtu
                        bzero(ndom, sizeof(*ndom));
                        ndom->nd_opt_mtu_type   = ND_OPT_MTU;
                        ndom->nd_opt_mtu_len    = (sizeof(*ndom)>>3);
                        ndom->nd_opt_mtu_mtu    = htonl(1500);
                        // icmpv6-option prefix info
                        bzero(ndopi, sizeof(*ndopi));
                        ndopi->nd_opt_pi_type   = ND_OPT_PREFIX_INFORMATION;
                        ndopi->nd_opt_pi_len    = (sizeof(*ndopi)>>3);
                        ndopi->nd_opt_pi_prefix_len = 64;
                        ndopi->nd_opt_pi_flags_reserved = (ND_OPT_PI_FLAG_AUTO);
                        ndopi->nd_opt_pi_valid_time = htonl(86400);
                        ndopi->nd_opt_pi_preferred_time = htonl(43200);

                        //
                        pgw_swap_address(packet);
                        gtpu_header(packet)->length = htons(sizeof(struct ip6_hdr) +
                                                            sizeof(struct nd_router_advert) +
                                                            sizeof(struct nd_opt_mtu) +
                                                            sizeof(struct nd_opt_prefix_info));
                        packet->datalen = ntohs(gtpu_header(packet)->length) + sizeof(gtpu_header_t);
                        // pseudo header
                        memcpy(&icmpv6_pseudo->srcaddr, ipv6_src_addr, sizeof(ipv6_src_addr));
                        memcpy(&icmpv6_pseudo->dstaddr, ipv6_dst_addr, sizeof(ipv6_dst_addr));
                        icmpv6_pseudo->payloadlen    = ip6h->ip6_plen;
                        icmpv6_pseudo->next          = htons(ip6h->ip6_nxt);
                        // SELECT sgw_gtpu_teid FROM tunenl WHERE pgw_teid = ?
                        bzero(lookup_bind, sizeof(lookup_bind));
                        bzero(&parse_state, sizeof(parse_state));
                        // find by pgw_teid
                        pgw_teid   = ntohl(gtpu_header(packet)->tid);
                        lookup_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
                        lookup_bind[0].buffer = &pgw_teid;
                        lookup_bind[0].is_null = 0;
                        //
                        pthread_mutex_lock(&__mysql_mutex);
                        if (DBPROVIDER_STMT_RESET(tnep->stmt) != 0) { pgw_panic("failed. mysql_stmt_reset(%p : %u).", tnep->stmt, (unsigned)pgw_teid); }
                        if (DBPROVIDER_STMT_BIND_PARAM(tnep->stmt, lookup_bind) != 0) { pgw_panic("failed. mysql_stmt_bind_param(%p : %u).", tnep->stmt, (unsigned)pgw_teid); }
                        if (DBPROVIDER_EXECUTE(tnep->stmt, tnep->bind.bind, on_tunnel_record, &parse_state) != OK){ PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_mysql_execute..(%p:%p)\n", (void*)pthread_self(), pnode); }
                        pthread_mutex_unlock(&__mysql_mutex);

                        if (parse_state.flag&HAVE_RECORD){
                            U32 ueipv;
                            U8  ipv6[16] = {0x24, 0x01, 0xf1, 0x00,
                                            (((0x0f&pnode->handle->ext_set_num)<<4)|(0x0f&pnode->handle->pgw_unit_num)), 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00};
                            // valid teid.
                            PGW_LOG(PGW_LOG_LEVEL_DBG, "recieved ICMPv6/RS..(%u - %u : %p : [%u]%u/%d)\n",(unsigned)pgw_teid, parse_state.sgw_c_teid, (void*)pthread_self(), pnode->index, m, nburst);
                            // RS responsed
                            icmp6_rs = OK;
                            gtpu_header(packet)->tid = parse_state.sgw_c_teid;
                            // generate prefix(ipv6) from UE ip
                            ueipv = ntohl(parse_state.ue_ipv_n.s_addr);
                            if ((ueipv>>24)){ break; }
                            ueipv |= (PGW_IP8<<24);
                            ueipv = htonl(ueipv);
                            memcpy(&ipv6[5], &((char*)&ueipv)[1], 3);
                            memcpy(&ndopi->nd_opt_pi_prefix, ipv6, sizeof(ipv6));
                            // icmpv6 - checksum
                            memcpy(icmpv6_pseudo+1, ndra, ntohs(ip6h->ip6_plen));
                            checksum = _checksum(icmpv6_pseudo, sizeof(*icmpv6_pseudo) + ntohs(ip6h->ip6_plen), 0);
                            ndra->nd_ra_hdr.icmp6_cksum = _wrapsum(checksum);
                            //
                            if (pgw_enq(pnode, EGRESS, packet) != OK){
                                pgw_panic("failed. enq(gtpu icmpv6 Router Solicitation)");
                            }
                        }else{
                            pgw_free_packet(packet);
                            PGW_LOG(PGW_LOG_LEVEL_ERR, "missing teid/RS..(%u : %p : [%u]%u/%d)\n",(unsigned)pgw_teid, (void*)pthread_self(), pnode->index, m, nburst);
                        }
                    }
                }
                if (icmp6_rs != OK){
                    // gtpu -> Error Indicator(exclude icmp6_sa)
                    // FIXME: only logging?
                    PGW_LOG(PGW_LOG_LEVEL_DBG, "event_gtpu_other_req..(%p : %p : [%u]%u/%d)\n", pnode, (void*)pthread_self(), pnode->index, m, nburst);
                }
            }else if (gtpu_header(pkts[m])->type == GTPU_ERR_IND){
                mlen = sizeof(gtpu_header_t) + ((gtpu_header(pkts[m])->v1_flags.extension | gtpu_header(pkts[m])->v1_flags.sequence | gtpu_header(pkts[m])->v1_flags.npdu)?4:0);
                //
                if ((mlen + sizeof(gtpu_err_indication_t)) == pkts[m]->datalen){
                    perrind = (gtpu_err_indication_ptr)(pkts[m]->data + mlen);
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "error indicator..(%u : %p : [%u]%u/%d/%u)\n",(unsigned)perrind->teid.val, (void*)pthread_self(), pnode->index, m, nburst, mlen);

                    memcpy(&addr, &perrind->peer.val, 4);
                    snprintf(cip, sizeof(cip) - 1,"%s", inet_ntoa(addr));

                    // delete session for target ip/teid, Erorr Indication
                    snprintf(sql, sizeof(sql)-1, ERROR_IND_UPD_SQL, ntohl(perrind->teid.val), cip);
                    //
                    pthread_mutex_lock(&__mysql_mutex);
                    if (DBPROVIDER_QUERY(pnode->dbhandle, sql) != 0){
                        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                    }
                    pthread_mutex_unlock(&__mysql_mutex);
                }
            }
            pgw_free_packet(pkts[m]);
        }
        return(OK);
    }
    return(ERR);
}
