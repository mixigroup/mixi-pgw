/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_greterm_worker.cc
    @brief      gre terminate worker core implemented
*******************************************************************************
    tunnel server : placement in internet upstream path, POD : terminate GRE\n
    (for testing)\n
*******************************************************************************
    @date       created(06/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 06/oct/2017 
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;

/**
   gre terminate worker core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     lcore  core id
   @param[in]     reserved not supported(reserved)
 */
MIXIPGW::CoreGretermWorker::CoreGretermWorker(COREID lcore, unsigned reserved)
        :CoreInterface(0, 0, lcore), opt_(reserved){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /reserved: %08x\n", lcore, reserved);
}
/**
   gre terminate worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::CoreGretermWorker::~CoreGretermWorker(){}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CoreGretermWorker::GetType(void){
    return(TYPE::WORKER);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreGretermWorker::GetObjectName(void){
    return("CoreGretermWorker");
}

/**
   gre terminate , connect woker core  to specific ring \n
   *******************************************************************************
   gre terminate in worker core  , [FROM/TO] both direction are software ring\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreGretermWorker::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){              // from = Rx -> Worker between Indicates ring 
        ring_fm_ = (struct rte_ring*)ring;
    }else if (order == ORDER::TO){          // to   = Worker -> Tx between Indicates ring 
        ring_to_ = (struct rte_ring*)ring;
    }else{
        rte_exit(EXIT_FAILURE, "unknown ring order.(%u)\n", order);
    }
    return(0);
}
/**
   gre terminate worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 patch process, bulk + dequeued packets parse one by one, and judgement\n
   ARP or internal ICMP case -> pass-though -> enqueue to forwarding ring\n
   other packets are enqueued after GRE termination\n
   number of cycle is used to events in specific cycle.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreGretermWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;
    //
    auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_, NULL);
    if (unlikely(nburst == -ENOENT)) {
        goto resume_next;
    }
    if (unlikely(nburst == 0)){
        goto resume_next;
    }
    // prefetch first.
    for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
    }
    // GRE termination every packet, while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
        // pass through target packet.
        auto opr = check_decapability(input_[n]);
        if (opr == PACKET_TYPE::PASS){
            PGW_LOG(RTE_LOG_INFO, "pass through: %s \n", ring_fm_->name);
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
        auto opr = check_decapability(input_[n]);
        if (opr == PACKET_TYPE::PASS){
            PGW_LOG(RTE_LOG_INFO, "pass through: %s \n", ring_fm_->name);
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
/**
   gre terminate.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     inpkt gre terminate target packets
 */
RETCD MIXIPGW::CoreGretermWorker::ModifyPacket(struct rte_mbuf* inpkt){
    struct ether_hdr eh;
    auto ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto iphlen = ((ip->version_ihl&0x0f)<<2);
#ifndef __CANNOT_USE_GRE__
    // processes only GRE, other packets are dropped.
    if (ip->next_proto_id != IPPROTO_GRE){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_ERR, "not gre(%u)\n", iphlen );
        return(1);
    }
    // only GRE at ip option(8 octet)
    // 8 * 4 = 32(ip header default:20 + option 12)
    if (iphlen != 32){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_ERR, "invalid ip option(%u)\n", iphlen );
        return(1);
    }
#else
    if (ip->next_proto_id != IPPROTO_UDP){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_ERR, "not udp(%u)\n", iphlen );
        return(1);
    }
    if (iphlen != 20){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_ERR, "invalid ip option(%u)\n", iphlen );
        return(1);
    }
#endif
    memcpy(&eh, rte_pktmbuf_mtod(inpkt, char*), sizeof(eh));
    // decap
    if (!rte_pktmbuf_adj(inpkt, 36)){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        rte_pktmbuf_free(inpkt);
        PGW_LOG(RTE_LOG_ERR, "failed. decap(%u)\n", iphlen );
        return(1);
    }
    ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    if ((ip->version_ihl&0xf0) != 0x40){
        eh.ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv6);
        inpkt->packet_type &= ~RTE_PTYPE_L3_IPV4;
        inpkt->packet_type |= RTE_PTYPE_L3_IPV6;
        PGW_LOG(RTE_LOG_DEBUG, "ipv6[%02x : %02x]\n", ip->version_ihl, ip->version_ihl&0xf0);
    }else{
        inpkt->packet_type |= RTE_PTYPE_L3_IPV4;
        PGW_LOG(RTE_LOG_DEBUG, "ipv4[%02x : %02x]\n", ip->version_ihl, ip->version_ihl&0xf0);
    }
    memcpy(rte_pktmbuf_mtod(inpkt, char*), &eh, sizeof(eh));
    PGW_LOG(RTE_LOG_DEBUG, "greterminate/ ipv: %02x/saddr: %08x/daddr: %08x/proto: %02x/ipchecksum: %04x\n",
		    ip->version_ihl,
		    ip->src_addr,
		    ip->dst_addr,
		    ip->next_proto_id,
		    ip->hdr_checksum);
    //
#ifdef __PCAP_DEBUG__ 
    PGW_LOG(RTE_LOG_DEBUG, "decap.. pcap(%u)", rte_pktmbuf_pkt_len(inpkt));
    pthread_mutex_lock(&__pcap_mutex);
    pcap_open();
    pcap_append(0, rte_pktmbuf_mtod(inpkt,char*), rte_pktmbuf_pkt_len(inpkt));
    pcap_close();
    pthread_mutex_unlock(&__pcap_mutex);
#endif

    return(0);
}

