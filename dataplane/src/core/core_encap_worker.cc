/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_encap_worker.cc
    @brief      encapworker core implemented
*******************************************************************************
	tunnel server : placement in internet downstream path\n
	encapsulate ip packet into GTPU\n
	\n
*******************************************************************************
    @date       created(27/sep/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 27/sep/2017 
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;

/**
   encapsulate worker core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     lcore  core id
   @param[in]     srcipv4 srcipv4 at encapsulate
 */
MIXIPGW::CoreEncapWorker::CoreEncapWorker(COREID lcore, unsigned srcipv4)
    :CoreInterface(0, 0, lcore), srcipv4_(srcipv4){
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
    // default values - ip/udp/gtpu
    gtpuh_->u.v1_flags.proto = GTPU_PROTO_GTP;
    gtpuh_->u.v1_flags.version = GTPU_VERSION_1;
    gtpuh_->type = GTPU_G_PDU;
    //
    udph_->src_port    = htons(PGWEGRESSPORT);
    udph_->dst_port    = htons(PGWEGRESSPORT);
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
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /srcipv4: %08x\n", lcore, srcipv4);
}
/**
   encapsulate worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::CoreEncapWorker::~CoreEncapWorker(){
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
TYPE MIXIPGW::CoreEncapWorker::GetType(void){
    return(TYPE::WORKER);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreEncapWorker::GetObjectName(void){
    return("CoreEncapWorker");
}
/**
   obtain value find by key.\n
   *******************************************************************************
   override, call CoreInterface::GetN ,except KEY::OPT\n
   *******************************************************************************
   @param[in]     key   key
   @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CoreEncapWorker::GetN(KEY t){
    if (t == KEY::OPT){
        return(srcipv4_);
    }
    return(CoreInterface::GetN(t));
}
/**
   encapsulate , connect woker core  to specific ring \n
   *******************************************************************************
   encapsulate in worker core  , [FROM/TO] both direction are software ring\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreEncapWorker::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){              // from = Rx -> Worker between Indicates ring 
        ring_fm_ = (struct rte_ring*)ring;
        printf("rx -> [worker] ring [%p:%s]\n", ring_fm_ ,ring_fm_->name);
    }else if (order == ORDER::TO){          // to   = Worker -> Tx between Indicates ring 
        ring_to_ = (struct rte_ring*)ring;
        printf("[worker] -> tx ring [%p:%s]\n", ring_to_ ,ring_to_->name);
    }else{
        rte_exit(EXIT_FAILURE, "unknown ring order.(%u)\n", order);
    }
    return(0);
}
/**
   encapsulate\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     inpkt  packet for encapsulate 
 */
RETCD MIXIPGW::CoreEncapWorker::ModifyPacket(struct rte_mbuf* inpkt){
    auto len = (sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr));
    auto ofst = sizeof(struct ether_hdr);

    auto prevlen = rte_pktmbuf_pkt_len(inpkt);
    auto pkt = rte_pktmbuf_prepend(inpkt, len);
    if (!pkt){
        counter_.Inc(0, 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_CRIT, "failed. prepend(%u)\n", prevlen);
        return(1);
    }
    // overwrite eth and move
    rte_memcpy(pkt, (pkt + len), sizeof(struct ether_hdr));
    // copy original ip address to outside.
    if (RTE_ETH_IS_IPV4_HDR(inpkt->packet_type)) {
        auto ip4 = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr *, sizeof(struct ether_hdr) + len);
        iph_->dst_addr = ip4->dst_addr;
    }else if (RTE_ETH_IS_IPV6_HDR(inpkt->packet_type)) {
        auto eth = rte_pktmbuf_mtod(inpkt, struct ether_hdr *);
        auto ip6 = rte_pktmbuf_mtod_offset(inpkt, struct ipv6_hdr *, sizeof(struct ether_hdr) + len);
        // ipv4 address is lacated in ipv6/40-64
        rte_memcpy(&iph_->dst_addr, &ip6->dst_addr[4], 4);
        //  first 8 bits are fixed value
        iph_->dst_addr = ((iph_->dst_addr&0xffffff00)|0x0000000a);

        // change next protocol to ipv4
        eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    }else{
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_CRIT, "failed. iphead(%u)\n", prevlen);
        return(1);
    }

    iph_->total_length  = rte_cpu_to_be_16(rte_pktmbuf_pkt_len(inpkt) - sizeof(struct ether_hdr));
    iph_->hdr_checksum  = 0;
    iph_->hdr_checksum  = _wrapsum(_checksum(iph_, 20, 0));
    udph_->dgram_cksum  = 0;
    udph_->dgram_len    = rte_cpu_to_be_16(rte_pktmbuf_pkt_len(inpkt) - sizeof(struct ether_hdr) - sizeof(struct ipv4_hdr));
    gtpuh_->length      = rte_cpu_to_be_16(rte_pktmbuf_pkt_len(inpkt) - sizeof(struct ether_hdr) - sizeof(struct ipv4_hdr) - sizeof(struct udp_hdr) - 8);
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
    PGW_LOG(RTE_LOG_DEBUG, "encap ..");
#ifdef __PCAP_DEBUG_ENCAP__
    PGW_LOG(RTE_LOG_DEBUG, "encap .. pcap");
    pthread_mutex_lock(&__pcap_mutex);
    pcap_open();
    pcap_append(0, pkt, rte_pktmbuf_pkt_len(inpkt));
    pcap_close();
    pthread_mutex_unlock(&__pcap_mutex);
#endif
    return(0);
}
/**
   encapsulate worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 patch process, bulk + dequeued packets parse one by one, and judgement\n
   ARP or internal ICMP case -> pass-though -> enqueue to Tx\n
   other packets are enqueued to encapsulate -> Tx\n
   number of cycle is used to events in specific cycle.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreEncapWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;
    //
    auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_,NULL);
    if (unlikely(nburst == -ENOENT)) {
        goto resume_next;
    }
    if (unlikely(nburst == 0)){
        goto resume_next;
    }
    PGW_LOG(RTE_LOG_DEBUG, "dequeue worker from [%p:%s]..(burst: %u/ count: %u)\n", ring_fm_ ,ring_fm_->name, (unsigned)burst_fm_, (unsigned)nburst);

    // prefetch first.
    for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
    }
    // encapsulate every packet while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
        //
        auto opr = is_arp_or_innericmp(input_[n]);
        if (opr == PACKET_TYPE::PASS){
            SendOrBuffer(arg, input_[n]);
            continue;
        }else if (opr == PACKET_TYPE::VLAN){
            counter_.Inc(rte_pktmbuf_pkt_len(input_[n]), 1, 0, 0);
            rte_pktmbuf_free(input_[n]);
            continue;
        }
        if (ModifyPacket(input_[n]) == 0){
            SendOrBuffer(arg, input_[n]);
        }
    }
    // remained.
    for (; n < nburst; n++) {
        auto opr = is_arp_or_innericmp(input_[n]);
        if (opr == PACKET_TYPE::PASS){
            SendOrBuffer(arg, input_[n]);
            continue;
        }else if (opr == PACKET_TYPE::VLAN){
            counter_.Inc(rte_pktmbuf_pkt_len(input_[n]), 1, 0, 0);
            rte_pktmbuf_free(input_[n]);
            continue;
        }
        if (ModifyPacket(input_[n]) == 0){
            SendOrBuffer(arg, input_[n]);
        }
    }
resume_next:
    // every xxx K times(depends on environment), flush burst buffer.
    if (flush_delay_ && (unlikely((*cnt) == flush_delay_))) {
        BurstFlush(arg);
        (*cnt) = 0;
    }
    return(0);
}

