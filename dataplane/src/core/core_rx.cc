/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_rx.cc
    @brief      Rx Core Implemented
*******************************************************************************
	forwarding and distribute burst packets received from NIC-Rx ring\n
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
   Rx Core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     port   portid
   @param[in]     queue  queue id
   @param[in]     lcore  core id
 */
MIXIPGW::CoreRx::CoreRx(PORTID port,QUEUEID queue,COREID lcore
#if defined(COUNTER_RX) || defined(TUNNEL_RX)
        ,unsigned type
        ):CoreInterface(port, queue, lcore), type_(type){
#else
        ):CoreInterface(port, queue, lcore){
#endif
    bzero(&output_cnt_rx_, sizeof(output_cnt_rx_));
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /port: %u/ queue: %u\n", lcore, port, queue);
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  Core Type [RX] return
 */
TYPE MIXIPGW::CoreRx::GetType(void){
    return(TYPE::RX);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreRx::GetObjectName(void){
    return("CoreRx");
}
/**
   connect Rx Core to specific ring\n
   *******************************************************************************
   Rx = [TO] direction = software ring\n
   throw exception when configuring unknown ring.\n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreRx::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::TO){
        ring_rx_.push_back((struct rte_ring*)ring);
        printf("[rx] -> worker ring [%p:%s]\n", ring ,((struct rte_ring*)ring)->name);
#ifdef __TEST_MODE__
    }else if (order == ORDER::FROM){
        ring_test_from_ = (struct rte_ring*)ring;
        printf(">>test mode<< input ring [%p:%s]\n", ring ,((struct rte_ring*)ring)->name);
#endif
    }else{
        rte_exit(EXIT_FAILURE, "dequeu ring cannot be set.(%u)\n", order);
    }
    // count of worker rings connected to self.
    SetN(KEY::CNT_WORKER, (unsigned)ring_rx_.size());
    return(0);
}
/**
   RX Core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 batch process, forward/enqueue bulk received packets from NIC
   to worker core with load balancing by ip address.
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreRx::Cycle(void* arg, uint64_t* cnt){
    int  n = 0;
    char* pkt;
    int offset;
    // destination worker : roundrobin selection.
    while(1){
#ifdef __TEST_MODE__
        // input packet with test ring 
        auto nburst = rte_ring_sc_dequeue_burst(ring_test_from_, (void**)input_, burst_fm_, NULL);
#else
        // read packets from NIC
        auto nburst = rte_eth_rx_burst(port_, queue_, input_, (uint16_t) burst_fm_);
#endif
        if (unlikely(nburst == 0)) {
            break;
        }
        // prefetch first.
        for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
            rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
        }
        // send packet , while prefetching.
        auto wk_cnt = GetN(KEY::CNT_WORKER);
        for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
            rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
            //
            pkt = rte_pktmbuf_mtod(input_[n], char *);

            #if defined(COUNTER_RX)
            #define FLOW_HASH_POS   (65)
            auto ofset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
            auto inip = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);

            if(type_ == COUNTER_INGRESS){
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 0; // 4 octet of inner ipv4.src
            else
                    offset = 0; // 8 octet of inner ipv6.src
            } else{
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 4;  // 4 octet of inner ipv4.dst
                else
                    offset = 16; // 8 octet of inner ipv6.dst
            }

            #elif defined(TUNNEL_RX)
            #define FLOW_HASH_POS   (29)
            if(type_ == TUNNEL_EGRESS){
                auto ofset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
                auto inip = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 36; // 4 octet of inner ipv4.src
                else
                    offset = 36; // 8 octet of inner ipv6.src
            } else{
                auto ofset = sizeof(struct ether_hdr);
                auto inip = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 4;  // 4 octet of ipv4.dst
                else
                    offset = 16; // 8 octet of ipv6.dst
            }
            #endif

            #if defined(COUNTER_RX) || defined(TUNNEL_RX)
            SendOrBufferRx(arg, (unsigned)pkt[FLOW_HASH_POS + offset]%wk_cnt, input_[n]);
            #else
            #define FLOW_HASH_POS   (33)    // 4 octet of ip.dst
            /**
             * worker forwarding destinations is selected by 4 octet of ip address.
             * */
            SendOrBufferRx(arg, (unsigned)pkt[FLOW_HASH_POS]%wk_cnt, input_[n]);
            #endif
        }
        // remained.
        for (; n < nburst; n++) {
            //
            pkt = rte_pktmbuf_mtod(input_[n], char *);

            #if defined(COUNTER_RX)
            auto ofset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
            auto inip = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);

            if(type_ == COUNTER_INGRESS){
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 0; // 4 octet of inner ipv4.src
            else
                    offset = 0; // 8 octet of inner ipv6.src
            } else{
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 4;  // 4 octet of inner ipv4.dst
                else
                    offset = 16; // 8 octet of inner ipv6.dst
            }

            #elif defined(TUNNEL_RX)
            if(type_ == TUNNEL_EGRESS){
                auto ofset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
                auto inip = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 36; // 4 octet of inner ipv4.src
                else
                    offset = 36; // 8 octet of inner ipv6.src
            } else{
                auto ofset = sizeof(struct ether_hdr);
                auto inip = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);
                if ((inip->version_ihl&0xf0) == 0x40)
                    offset = 4;  // 4 octet of ipv4.dst
                else
                    offset = 16; // 8 octet of ipv6.dst
            }
            #endif

            #if defined(COUNTER_RX) || defined(TUNNEL_RX)
            SendOrBufferRx(arg, (unsigned)pkt[FLOW_HASH_POS + offset]%wk_cnt, input_[n]);
            #else
            SendOrBufferRx(arg, (unsigned)pkt[FLOW_HASH_POS]%wk_cnt, input_[n]);
            #endif
        }
        break;
    }
    // every xxx K times(depends on environment), flush burst buffers.
    if (flush_delay_ && (unlikely((*cnt) >= flush_delay_))) {
        BurstFlushRx(arg);
        (*cnt) = 0;
    }
    return(0);
}
/**
   bulk enqueue to worker core.\n
   *******************************************************************************
   enqueue burst packets to worker core ring.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     mbuf mbuf address transferred to next Core.  address 
 */
void MIXIPGW::CoreRx::SendOrBufferRx(void* arg, unsigned wk, struct rte_mbuf* mbuf){
    auto curpos = output_cnt_rx_[wk];

    // stores into buffer without sending out immediately(< buset size)
    output_rx_[wk][curpos ++] = mbuf;
    if (likely(curpos < burst_to_)) {
        output_cnt_rx_[wk] = curpos;
        return;
    }
    // queuing immediately(>= buset size)
    auto ret = rte_ring_sp_enqueue_bulk(ring_rx_[wk], (void **) output_rx_[wk], burst_to_, NULL);
    // drop (not enough buffer)
    if (unlikely(ret == 0)) {
        for (auto k = 0; k < burst_to_; k ++) {
            rte_pktmbuf_free(output_rx_[wk][k]);
        }
        counter_.Inc(0, burst_to_, 0, 0);
    }
    output_cnt_rx_[wk] = 0;
}
/**
   send out remaining packets  to worker ring.\n
   *******************************************************************************
   time out burst enqueue\n
   *******************************************************************************
   @param[in]     arg  application instance address
 */
void MIXIPGW::CoreRx::BurstFlushRx(void* arg){
    auto wkcnt  = GetN(CNT_WORKER);
    //
    for(auto n = 0;n < wkcnt;n++){
        if (unlikely(output_cnt_rx_[n]==0)){
            continue;
        }
        // transfer packet to worker
        auto ret = rte_ring_sp_enqueue_bulk(ring_rx_[n], (void **) output_rx_[n], output_cnt_rx_[n], NULL);
        if (unlikely(ret == 0)) {
            for (auto k = 0; k < output_cnt_rx_[n]; k ++) {
                rte_pktmbuf_free(output_rx_[n][k]);
            }
            counter_.Inc(0, output_cnt_rx_[n], 0, 0);
        }
        output_cnt_rx_[n] = 0;
    }
    // numa--off
#ifdef __DEBUG__
    rte_pause();
    pthread_yield();
#endif
}
