/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_tx.cc
    @brief      TX Core implement
*******************************************************************************
    forwarding burst packet received from connected worker cores to NIC-Tx ring.\n
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
   TX Core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     port   portid
   @param[in]     lcore  core id
 */
MIXIPGW::CoreTx::CoreTx(PORTID port,COREID lcore)
    :CoreInterface(port, 0, lcore){
    bzero(&output_cnt_, sizeof(output_cnt_));
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /port: %u\n", lcore, port);
}
MIXIPGW::CoreTx::CoreTx(PORTID port,QUEUEID queue,COREID lcore)
    :CoreInterface(port, queue, lcore){
    bzero(&output_cnt_, sizeof(output_cnt_));
}

#ifdef __TEST_MODE__
MIXIPGW::CoreTx::CoreTx(PORTID port,QUEUEID queue,COREID lcore,unsigned debug_flag)
        :CoreInterface(port, queue, lcore){
    bzero(&output_cnt_, sizeof(output_cnt_));
    debug_flag_ = debug_flag;
}
#endif
/**
   get core type.\n
   *******************************************************************************
   [Tx] return\n
   *******************************************************************************
   @return TYPE  core type [TX] return
 */
TYPE MIXIPGW::CoreTx::GetType(void){
    return(TYPE::TX);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreTx::GetObjectName(void){
    return("CoreTx");
}
/**
   connect TX Core to specific ring\n
   *******************************************************************************
   Tx = [FROM] direction = software ring\n
   throw exception when configuring unknown ring type.\n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTx::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){
        ring_tx_.push_back((struct rte_ring*)ring);
        printf("worker -> [tx] ring [%p:%s]\n", ring ,((struct rte_ring*)ring)->name);
#ifdef __TEST_MODE__
    }else if (order == ORDER::TO){
        ring_test_to_ = (struct rte_ring*)ring;
        printf(">>test mode<< input ring [%p:%s]\n", ring ,((struct rte_ring*)ring)->name);
#endif
    }else{
        rte_exit(EXIT_FAILURE, "enqueue ring cannot be set.(%u)\n", order);
    }
    // count of worker rings connected to self.
    SetN(KEY::CNT_WORKER, (unsigned)ring_tx_.size());

    return(0);
}
/**
   TX Core  : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 batch process, forward bulk+dequeued packets from worker core\n
    to NIC-Tx queue as send to network.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTx::Cycle(void* arg, uint64_t* cnt){
    auto wk_cnt = GetN(KEY::CNT_WORKER);
    // bulk read directly from worker ring to buffer for NIC-output.
    for (auto n = 0; n < wk_cnt; n ++) {
        auto curpos = output_cnt_;
        auto nburst = rte_ring_sc_dequeue_burst(ring_tx_[n], (void **) &output_[curpos], burst_fm_, NULL);
        if (unlikely(nburst == -ENOENT)) {
            continue;
        }
        if (unlikely(nburst == 0)){
            continue;
        }
        curpos += nburst;

        // continue process when burst size is exceeded.
        if (unlikely(curpos < burst_to_)) {
            output_cnt_ = curpos;
            continue;
        }
#ifdef __TEST_MODE__
        if (debug_flag_&0x0001){
            for(auto k = 0; k < curpos; k++){
                // down stream packets from internet -> swap ip address
                auto ip = rte_pktmbuf_mtod_offset(output_[k], struct ipv4_hdr*, sizeof(ether_hdr));
                PGW_LOG(RTE_LOG_DEBUG, "swap ip dst: %08x /src: %08x\n", ip->dst_addr, ip->src_addr);
               // swap ip
                auto t_ipaddr = ip->dst_addr;
                ip->dst_addr = ip->src_addr;
                ip->src_addr = t_ipaddr;
                ip->hdr_checksum = 0;
                ip->hdr_checksum = _wrapsum(_checksum(ip, 20, 0));
                PGW_LOG(RTE_LOG_DEBUG, "swapped ip dst: %08x /src: %08x\n", ip->dst_addr, ip->src_addr);
            }
        }
        nburst = rte_ring_sp_enqueue_bulk(ring_test_to_, (void **) output_, (uint16_t) curpos, NULL);
#else
        nburst = rte_eth_tx_burst(port_, 0, output_, (uint16_t) curpos);
#endif
        if (unlikely(nburst < curpos)) {
            for (auto k = nburst; k < curpos; k ++) {
                rte_pktmbuf_free(output_[k]);
            }
            counter_.Inc(0, curpos, 0, 0);
        }
        output_cnt_ = 0;
    }
    // every xxx K times(depends on environment), flush burst buffer.
    if (flush_delay_ && (unlikely((*cnt) >= flush_delay_))) {
        BurstFlushTx(arg);
        (*cnt) = 0;
    }
    return(0);
}
/**
   transfer remained packets in burst buffer to NIC-Tx.\n
   *******************************************************************************
   time out burst send packet to nic tx ring\n
   *******************************************************************************
   @param[in]     arg  application instance address
 */
void MIXIPGW::CoreTx::BurstFlushTx(void* arg){
    auto lcore  = rte_lcore_id();
    //
    if (output_cnt_ > 0){
#ifdef __TEST_MODE__
        if (debug_flag_&0x0001){
            for(auto k = 0; k < output_cnt_; k++){
                // down stream packet from internet -> swap ip address
                auto ip = rte_pktmbuf_mtod_offset(output_[k], struct ipv4_hdr*, sizeof(ether_hdr));

                PGW_LOG(RTE_LOG_DEBUG, "swap ip dst: %08x /src: %08x\n", ip->dst_addr, ip->src_addr);

                // swap ip
                auto t_ipaddr = ip->dst_addr;
                ip->dst_addr = ip->src_addr;
                ip->src_addr = t_ipaddr;
                ip->hdr_checksum = 0;
                ip->hdr_checksum = _wrapsum(_checksum(ip, 20, 0));

                PGW_LOG(RTE_LOG_DEBUG, "swapped ip dst: %08x /src: %08x\n", ip->dst_addr, ip->src_addr);
            }
        }
        auto ret = rte_ring_sp_enqueue_bulk(ring_test_to_, (void **) output_, (uint16_t) output_cnt_, NULL);
#else
        auto ret = rte_eth_tx_burst(port_, 0, output_, (uint16_t) output_cnt_);
#endif
        if (unlikely(ret < output_cnt_)) {
            for (auto k = ret; k < output_cnt_; k ++) {
                rte_pktmbuf_free(output_[k]);
            }
            counter_.Inc(0, output_cnt_, 0, 0);
        }
    }
    output_cnt_ = 0;
    // numa--off
#ifdef __DEBUG__
    rte_pause();
    pthread_yield();
#endif
}
