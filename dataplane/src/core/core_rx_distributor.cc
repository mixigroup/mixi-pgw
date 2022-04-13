/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_rx_distributor.cc
    @brief      Distributor Rx Core implement
*******************************************************************************
	forwarding and distribute burst packets received from NIC-Rx ring\n
     - case gtpu/udp.dstport==9999, delegate processing to pgw-egress, \n
     - case gtpu, delegate processing to pgw-ingress, \n
     - case other than above, forward to tap interface\n
	\n
*******************************************************************************
    @date       created(16/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 16/nov/2017 
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;

typedef enum CORE_RX_DIST_RING{
    INGRESS = 0,
    EGRESS,
    MAXCOUNT
}_CORE_RX_DIST_RING;

/**
   Rx Distributor Core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     port   portid
   @param[in]     queue  queue id
   @param[in]     lcore  core id
 */
MIXIPGW::CoreRxDistributor::CoreRxDistributor(PORTID port,QUEUEID queue,COREID lcore)
    :CoreInterface(port, queue, lcore), ring_tap_tx_(NULL){
    bzero(&output_cnt_rx_, sizeof(output_cnt_rx_));
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /port: %u/ queue: %u\n", lcore, port, queue);
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  core type [RX] return
 */
TYPE MIXIPGW::CoreRxDistributor::GetType(void){
    return(TYPE::RX);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreRxDistributor::GetObjectName(void){
    return("CoreRxDistributor");
}
/**
  called before cycle loop at once\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     arg  application instance address
 @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreRxDistributor::BeforeCycle(void* arg){
    if (GetN(KEY::CNT_WORKER) != CORE_RX_DIST_RING::MAXCOUNT){
        rte_panic("core Rx distributor num of ring.%u \n", (unsigned) GetN(KEY::CNT_WORKER)); 
    }
    return(0);
}

/**
   connect Rx Distributor Core to specific ring\n
   *******************************************************************************
   Rx, [TO] direction = software ring\n
   throw exception when configuring unknown ring.\n
   ring_rx_[0] == pgw - ingress-side ring\n
   ring_rx_[1] == pgw - egress-side ring\n
   required, 2 destination rings.\n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreRxDistributor::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::TO){
        ring_rx_.push_back((struct rte_ring*)ring);
        printf("[rx] -> worker ring [%p:%s]\n", ring ,((struct rte_ring*)ring)->name);
    }else if (order == ORDER::EXTEND){
        ring_tap_tx_ = (struct rte_ring*)ring;
        printf("[rx] -> tap tx ring [%p:%s]\n", ring ,((struct rte_ring*)ring)->name);
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
   \n
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreRxDistributor::Cycle(void* arg, uint64_t* cnt){
    int  n = 0;
    // destination worker : round robin seelction.
    while(1){
#ifdef __TEST_MODE__
        // input packet with test ring 
        auto nburst = rte_ring_sc_dequeue_burst(ring_test_from_, (void**)input_, burst_fm_, NULL);
#else
        // read packets from NIC.
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
        for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
            rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
            auto udpdport = is_udp(input_[n]);
            if (udpdport == htons(PGWEGRESSPORT)){
                // forwarding to PGW egress
                SendOrBufferDistributeRx(arg, CORE_RX_DIST_RING::EGRESS, input_[n]);
            }else{
                // == gtpu -> forwarding to PGW ingress 
                // != gtpu -> forwarding to tap protocol statck
                auto teid = is_gtpu(input_[n]);
                if (!teid){
                    SendTapInterface(arg, input_[n]);
                }else{
                    SendOrBufferDistributeRx(arg, CORE_RX_DIST_RING::INGRESS, input_[n]);
                }
            }
        }
        // remained.
        for (; n < nburst; n++) {
            auto udpdport = is_udp(input_[n]);
            if (udpdport == htons(PGWEGRESSPORT)){
                SendOrBufferDistributeRx(arg, CORE_RX_DIST_RING::EGRESS, input_[n]);
            }else{
                auto teid = is_gtpu(input_[n]);
                if (!teid){
                    SendTapInterface(arg, input_[n]);
                }else{
                    SendOrBufferDistributeRx(arg, CORE_RX_DIST_RING::INGRESS, input_[n]);
                }
            }
        }
        break;
    }
    // every xxx K times(depends on environment), flush burst buffers.
    if (flush_delay_ && (unlikely((*cnt) == flush_delay_))) {
        BurstFlushDistributeRx(arg);
        (*cnt) = 0;
    }
    return(0);
}
/**
   bulk enqueue to worker core.\n
   *******************************************************************************
   enqueue burst packets to worker core ring\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     mbuf mbuf address transferred to next Core.  address 
 */
void MIXIPGW::CoreRxDistributor::SendOrBufferDistributeRx(void* arg, unsigned wk, struct rte_mbuf* mbuf){
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
   send out remaining packets to worker ring\n
   *******************************************************************************
   time out burst enqueue\n
   *******************************************************************************
   @param[in]     arg  application instance address
 */
void MIXIPGW::CoreRxDistributor::BurstFlushDistributeRx(void* arg){
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
            counter_.Inc(0, burst_to_, 0, 0);
        }
        output_cnt_rx_[n] = 0;
    }
}
/**
   forward packet to tap interface\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     mbur forwarding mbuf address 
 */
void MIXIPGW::CoreRxDistributor::SendTapInterface(void* arg, struct rte_mbuf* mbuf){
    if (rte_ring_sp_enqueue(ring_tap_tx_, mbuf) < 0){
        rte_pktmbuf_free(mbuf);
    }
}
