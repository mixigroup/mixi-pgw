/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_egress_tunnel_worker.cc
    @brief      PGW - egress tunnel mode worker core implemented
*******************************************************************************
	PGW : egress tunnel mode : \n
    \n
*******************************************************************************
    @date       created(27/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 27/apr/2018 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "../../inc/core.hpp"

using namespace MIXIPGW;

/**
   PGW - Egress tunnel mode worker core interface  :  constructor \n
   *******************************************************************************
   constructor \n
   *******************************************************************************
   @param[in]     lcore         core id
   @param[in]     dburi         databaseuri
   @parma[in]     serverid      database server id
   @param[in]     cpuid         thread for host stack(binlog) cpu/core
   @param[in]     reserved      not supported(reserved)
   @param[in]     srcipv4       ipsource address
   @param[in]     groupid       group id
   @param[in]     pgwid         PGW id
 */
MIXIPGW::CorePgwEgressTunnelWorker::CorePgwEgressTunnelWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned reserved, unsigned srcipv4,unsigned groupid, unsigned pgwid)
        :CorePgwBaseWorker(lcore, dburi, serverid, cpuid, reserved, srcipv4, groupid, pgwid,1, TYPE::PGW_EGRESS){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/dburi: %s/serverid: %u/cpuid: %u/greipv4: %08x/gresrcipv4: %08x/groupid: %u/pgwid: %u\n", lcore, dburi, serverid, cpuid, reserved, srcipv4, groupid, pgwid);

    gtpuh_ = (struct gtpu_hdr*)malloc(sizeof(struct gtpu_hdr));
    udph_  = (struct udp_hdr*)malloc(sizeof(struct udp_hdr));
    iph_   = (struct ipv4_hdr*)malloc(sizeof(struct ipv4_hdr));
    //
    if (!gtpuh_ || !udph_ || !iph_){
        rte_panic("malloc failed.\n");
    }
    bzero(gtpuh_, sizeof(*gtpuh_));
    bzero(udph_,  sizeof(*udph_));
    bzero(iph_,   sizeof(*iph_));
    // set default value - ip/udp/gtpu
    gtpuh_->u.v1_flags.proto = GTPU_PROTO_GTP;
    gtpuh_->u.v1_flags.version = GTPU_VERSION_1;
    gtpuh_->type = GTPU_G_PDU;
    //
    udph_->src_port    = htons(GTPU_PORT);
    udph_->dst_port    = htons(GTPU_PORT);
    udph_->dgram_len   = 0;
    udph_->dgram_cksum = 0;
    //
    iph_->version_ihl   = 0x45; // version4 : header length default 5
    iph_->type_of_service = 0;
    iph_->total_length  = 0;
    iph_->packet_id     = 1;
    iph_->fragment_offset = 0;
    iph_->time_to_live  = 64;
    iph_->next_proto_id = IPPROTO_UDP;
    iph_->hdr_checksum  = 0;
    iph_->src_addr      = htonl(srcipv4);
    iph_->dst_addr      = 0;
}

/**
   PGW - Egress tunnel mode worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CorePgwEgressTunnelWorker::~CorePgwEgressTunnelWorker(){
    free(gtpuh_);
    free(udph_);
    free(iph_);
    gtpuh_ = NULL;
    udph_  = NULL;
    iph_   = NULL;
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CorePgwEgressTunnelWorker::GetType(void){
    return(TYPE::PGW_EGRESS);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CorePgwEgressTunnelWorker::GetObjectName(void){
    return("CorePgwEgressTunnelWorker");
}

/**
   tunnel mode PGW Egress \n
   *******************************************************************************
   encap \n
   *******************************************************************************
   @param[in]     inpkt change encapsulate target packet 
 */
RETCD MIXIPGW::CorePgwEgressTunnelWorker::ModifyPacket(struct rte_mbuf* inpkt){
    auto eth = rte_pktmbuf_mtod(inpkt, struct ether_hdr *);
    auto len = (sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr));
    auto ofst = sizeof(struct ether_hdr);
    uint32_t  ipkey = 0;
    udat_ = (mbuf_userdat_ptr)&(inpkt->udata64);
    bzero(udat_, sizeof(*udat_));
    auto prevlen = rte_pktmbuf_pkt_len(inpkt);
    char *pkt;
    //
    auto opr = is_user(inpkt);
    if (opr == PACKET_TYPE::PASS){
        udat_->flag = MBFU_FLAG_DC;
        return(0);
    }
    //
    auto ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr *, sizeof(struct ether_hdr));
    if(ip->src_addr == htonl(SIXRD_ENCAPPER_ADDR)){
        //ipv6/ipv4
        auto ip6 = rte_pktmbuf_mtod_offset(inpkt, struct ipv6_hdr *, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
        // ipv4 address is lacated in ipv6/40-64
        rte_memcpy(&ipkey, &ip6->dst_addr[4], 4);
        //   first 8 bits are fixed value
        ipkey = ((ipkey&0xffffff00)|0x0000000a);
    }else{
        //ipv4
        auto ip4 = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr *, sizeof(struct ether_hdr));
        ipkey = ip4->dst_addr;
        //
    }
    // find from lookup table
    udat_->size = (prevlen - sizeof(ether_hdr));
    udat_->teid = findkey_;
    udat_->color= POLICER_COLOR_GREEN;
    udat_->mode = POLICER_MODE_LTE;
    findkey_    = ipkey&0xffffff00;
    now_        = time(NULL);
    //
    if (Find(findkey_,
             [](lookup_ptr itm, void* arg){
                 auto pthis = (CorePgwEgressTunnelWorker*)arg;
                 // current source is callback inside mutex lock.
                 // be careful with nested mutex locks.
                 /*
                              +---------------------------------+
                              |periodically every T sec.        |
                              | Tc(t+)=MIN(CBS, Tc(t-)+CIR*T)   |
                              | Te(t+)=MIN(EBS, Te(t-)+EIR*T)   |
                              +---------------------------------+
                     Packet of size
                         B arrives   /----------------\
                    ---------------->|color-blind mode|
                                     |       OR       |YES  +---------------+
                                     |  green packet  |---->|packet is green|
                                     |      AND       |     |Tc(t+)=Tc(t-)-B|
                                     |    B <= Tc(t-) |     +---------------+
                                     \----------------/
                                             |
                                             | NO
                                             v
                                     /----------------\
                                     |color-blind mode|
                                     |       OR       |YES  +----------------+
                                     | NOT red packet |---->|packet is yellow|
                                     |      AND       |     |Te(t+)=Te(t-)-B |
                                     |    B <= Te(t-) |     +----------------+
                                     \----------------/
                                             |
                                             | NO
                                             v
                                     +---------------+
                                     |packet is red  |
                                     +---------------+
                  *
                  *
                  * */
                 //
                 // recalculation counter has advanced since last calculation
                 //     -> refill tokens for seconds elapsed.
                 auto dt = (pthis->now_ - itm->epoch_w);
                 if (dt){
                     itm->commit_rate = MIN(itm->commit_burst_size, itm->commit_rate + (itm->commit_information_rate * dt));
                     itm->excess_rate = MIN(itm->excess_burst_size, itm->excess_rate + (itm->excess_information_rate * dt));
                     itm->epoch_w = pthis->now_;
                 }
#ifdef __TEST_MODE__
                 printf("CorePgwEgressTunnelWorker::ModifyPacket/any color(%u - %u:%u) %u /%u\n",
                        dt,
                        itm->epoch_w,
                        pthis->udat_->size,
                        itm->commit_rate,
                        itm->excess_rate);
#endif
                 if (pthis->udat_->size <= itm->commit_rate){
                     // green
                     itm->commit_rate = (itm->commit_rate - pthis->udat_->size);
                     pthis->udat_->color = POLICER_COLOR_GREEN;
                 }else if (pthis->udat_->size <= itm->excess_rate){
                     // yellow
                     itm->excess_rate = (itm->excess_rate - pthis->udat_->size);
                     pthis->udat_->color = POLICER_COLOR_YELLOW;
                 }else{
                     // red
                     pthis->udat_->color = POLICER_COLOR_RED;
                 }
                 pthis->lookup_itm_ = (*itm);
                 return(0);
             }, this) != 0){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        // forward GTPU packet to tap send-side ring.
        // enable saving of error packets with common tools
        // such as tcpdump in userland processes.
        if (ring_err_ind_){
            if (rte_ring_sp_enqueue(ring_err_ind_, inpkt) < 0){
                rte_pktmbuf_free(inpkt);
                PGW_LOG(RTE_LOG_ERR, "rte_ring_sp_enqueue - find(%p)\n", ring_err_ind_);
            }
            return(1);
        }
        udat_->flag = MBFU_FLAG_DC;
        return(0);
    }
    //
    if(ip->src_addr == htonl(SIXRD_ENCAPPER_ADDR)){
        // change next protocol to ipv4.
        eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
        auto prepend_len = sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
        pkt = rte_pktmbuf_prepend(inpkt, prepend_len);
        rte_memcpy(pkt, (pkt + prepend_len), sizeof(struct ether_hdr));
    }else{
        auto prepend_len = sizeof(ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
        pkt = rte_pktmbuf_prepend(inpkt, prepend_len);
        rte_memcpy(pkt, (pkt + prepend_len), sizeof(struct ether_hdr));
    }

    iph_->total_length  = rte_cpu_to_be_16(rte_pktmbuf_pkt_len(inpkt) - sizeof(struct ether_hdr));
    iph_->hdr_checksum  = 0;
    iph_->hdr_checksum  = _wrapsum(_checksum(iph_, 20, 0));
    udph_->dgram_cksum  = 0;
    udph_->dgram_len    = rte_cpu_to_be_16(rte_pktmbuf_pkt_len(inpkt) - sizeof(struct ether_hdr) - sizeof(struct ipv4_hdr));
    gtpuh_->length      = rte_cpu_to_be_16(rte_pktmbuf_pkt_len(inpkt) - sizeof(struct ether_hdr) - sizeof(struct ipv4_hdr) - sizeof(struct udp_hdr) - 8);

    // sgw:gtpu - set teid
    gtpuh_->tid         = lookup_itm_.sgw_gtpu_teid;
    // sgw destination ip address - gre source ipv4=use as pgw ipv4
    iph_->src_addr      = lookup_itm_.pgw_gtpu_ipv4;
    iph_->dst_addr      = lookup_itm_.sgw_gtpu_ipv4;
    iph_->hdr_checksum  = 0;
    iph_->hdr_checksum  = _wrapsum(_checksum(iph_, 20, 0));

    // ipv4
    ofst = sizeof(struct ether_hdr);
    rte_memcpy(pkt + ofst, iph_, sizeof(*iph_));
    // udp
    ofst += sizeof(ipv4_hdr);
    rte_memcpy(pkt + ofst, udph_, sizeof(*udph_));
    // gtpu
    ofst += sizeof(udp_hdr);
    rte_memcpy(pkt + ofst, gtpuh_, sizeof(*gtpuh_));
    //
    inpkt->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
    // use rte_mbuf.userdata
    udat_->flag = MBFU_FLAG_PDU;
    udat_->mode = lookup_itm_.stat.mode;
    return(0);
}

