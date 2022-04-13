/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_gtpu2gre_worker.cc
    @brief      gtpu to gre exchange worker core implemented
*******************************************************************************
	tunnel server : placement in internet downstream path : lacated behind PRE\n
	convert GTPU to GRE.(no change in packet size)\n
	\n
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
   gtpu to gre exchange worker core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     lcore  core id
   @param[in]     pgwipv4 ipv4 address at PGW 
 */
MIXIPGW::CoreGtpu2GreWorker::CoreGtpu2GreWorker(COREID lcore, unsigned pgwipv4)
        :CoreInterface(0, 0, lcore), pgwipv4_(pgwipv4){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /pgwipv4: %08x\n", lcore, pgwipv4);
}
/**
   gtpu to gre exchange worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::CoreGtpu2GreWorker::~CoreGtpu2GreWorker(){
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CoreGtpu2GreWorker::GetType(void){
    return(TYPE::WORKER);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreGtpu2GreWorker::GetObjectName(void){
    return("CoreGtpu2GreWorker");
}

/**
   obtain value find by key.\n
   *******************************************************************************
   override  , option value =PGW ipv4\n
   *******************************************************************************
   @param[in]     key   key
   @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CoreGtpu2GreWorker::GetN(KEY t){
    if (t == KEY::OPT){
        return(pgwipv4_);
    }
    return(CoreInterface::GetN(t));
}
/**
   set key value.\n
   *******************************************************************************
   override  , option value=PGW ipv4\n
   *******************************************************************************
   @param[in]     key key
   @param[in]     v   value
 */
void MIXIPGW::CoreGtpu2GreWorker::SetN(KEY key,VALUE v){
    if (key == KEY::OPT){
        pgwipv4_ = v;
        return;
    }
    CoreInterface::SetN(key, v);
}

/**
   gtpu to gre exchange , connect woker core  to specific ring \n
   *******************************************************************************
   gtpu to gre exchange in worker core  , [FROM/TO] both direction are software ring\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreGtpu2GreWorker::SetRingAddr(ORDER order, void* ring){
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
   gtpu to gre exchange worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 batch process, bulk + dequeued packets parse one by one, and judgement.\n
   case ARP or internal ICMP -> pass through -> enqueue to forwarding ring.\n
   other packets are enqueued, after exchange GTPU to GRE.\n
   number of cycle is used to event in specific cycle.
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreGtpu2GreWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;
    //
    auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_, NULL);
    if (unlikely(nburst == -ENOENT)) {
        return(0);
    }
    if (unlikely(nburst == 0)){
        return(0);
    }
    // prefetch first.
    for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
    }
    // exchange GTPU to GRE every packet, while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
        // pass through target packet.
        auto opr = is_arp_or_innericmp(input_[n]);
        if (opr == PACKET_TYPE::PASS){
            SendOrBuffer(arg, input_[n]);
            continue;
        }else if (opr == PACKET_TYPE::VLAN){
            counter_.Inc(rte_pktmbuf_pkt_len(input_[n]), 1, 0, 0);
            rte_pktmbuf_free(input_[n]);
            continue;
        }
        //
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
    return(0);
}
/**
   exchange gtpu capsule to GRE capsule.\n
   *******************************************************************************
   CoreEncapWorker process encapsulates GTPU packets with ipv4.\n
   *******************************************************************************
   @param[in]     inpkt capsuled packet 
 */
RETCD MIXIPGW::CoreGtpu2GreWorker::ModifyPacket(struct rte_mbuf* inpkt){
    static const unsigned char nops[12] = {0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01};
    static const unsigned char greo[4] = {0x00,0x00,0x80,0x00};
    //
    auto ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto nop= rte_pktmbuf_mtod_offset(inpkt, char*, sizeof(ether_hdr) + sizeof(ipv4_hdr));
    auto gre= rte_pktmbuf_mtod_offset(inpkt, char*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(nops));
    // rewrite outer destijnation ip address to PGW address.
    ip->version_ihl         = 0x48;           // version4 + ip option 2
    ip->dst_addr            = pgwipv4_;
    ip->next_proto_id       = IPPROTO_GRE;    // gre
    ip->hdr_checksum        = 0;
    // fill udp header(was originally located) region to ip-option(0x01=nop) 8 octets.
    rte_memcpy(nop, nops, sizeof(nops));
    // set GTPU header(was originally located) region to GRE.
    rte_memcpy(gre, greo, sizeof(greo));
    //
    inpkt->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_TUNNEL_GRE);
    //
    return(0);
}

