/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_taptx.cc
    @brief      Tap Tx Core Implement
*******************************************************************************
	receive packets from multiple sending rings and forward those to tap interface.\n
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

/**
   TapTx Core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     ifnm   interface  name 
   @param[in]     lcore  core id
 */
MIXIPGW::CoreTapTx::CoreTapTx(TXT ifnm,COREID lcore,CoreMempool* mpl, int tap_fd)
    :CoreInterface(0, 0, lcore),mpl_(mpl), cur_cycle_(rte_rdtsc()),cur_count_(0){
    //
    tap_fd_ = tap_fd;
    egress_ring_count_ = ingress_ring_count_ = 0;
    for(auto n = 0;n < TAPDUPLICTE_MAX;n++){
        ring_tap_ingress_[n] = ring_tap_egress_[n] = NULL;
    }
    PGW_LOG(RTE_LOG_INFO, "CoreTapTx::CoreTapTx / lcore: %u /ifnm: %s/ CoreMempool: %p\n", lcore, ifnm, mpl);
}
MIXIPGW::CoreTapTx::~CoreTapTx(){
    PGW_LOG(RTE_LOG_INFO, "CoreTapTx::~CoreTapTx / lcore: %u /CoreMempool:%p/ this: %p/ thread: %p\n", lcore_, mpl_, this, (void*)pthread_self());
}

/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  Core Type [TAPTX] return
 */
TYPE MIXIPGW::CoreTapTx::GetType(void){
    return(TYPE::TAPTX);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreTapTx::GetObjectName(void){
    return("CoreTapTx");
}

/**
   Tap TX Core : virtual cycle : 1 cycle\n
   *******************************************************************************
     - forwarding packet from multiple software rings to tap interface.\n
    \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTapTx::Cycle(void* arg, uint64_t* cnt){
    int n,l, nburst;
    uint64_t    t_cycle;
    const uint64_t drain_tsc = (rte_get_tsc_hz() + 1000000 - 1) / 1000000 * 1000000;

    // read packet from worker rings, process as continuously as possible in 1 cycle.
    for (n = 0; n < ingress_ring_count_; n ++) {
        nburst = rte_ring_sc_dequeue_burst(ring_tap_ingress_[n], (void **) input_int_, burst_fm_, NULL);
        if (unlikely(nburst == -ENOENT)) {
            continue;
        }
        if (unlikely(nburst == 0)){
            continue;
        }
        if (cur_count_ > MAXPACKET_PER_SECOND){
            for(l = 0;l < nburst;l++){
                rte_pktmbuf_free(input_int_[l]);
            }
            // tap interface has limit of about 1 Gbps, 
            // direct logging is possible.
            PGW_LOG(RTE_LOG_CRIT, "drop over Capability .[CoreTapTx::Cycle](%llu)\n", cur_count_);
            continue;
        }
        for(l = 0;l < nburst;l++){
            // write side of tap device is always asynchronous processing,
            // select(2) always returns success (on bsd system)
            struct rte_mbuf *m = input_int_[l];
            auto ret = write(tap_fd_,
                            rte_pktmbuf_mtod(m, void*),
                            rte_pktmbuf_data_len(m));
            if (likely(ret > 0)){
                counter_.Inc(0, 0, ret, 1);
                cur_count_ ++;
            }else if (ret < 0){
                counter_.Inc(rte_pktmbuf_data_len(m), 1, 0, 0);
            }
            rte_pktmbuf_free(m);
        }
    }
    // initialize counting around every 1 sec.
    t_cycle = rte_rdtsc();
    if ((cur_cycle_ + drain_tsc) < t_cycle){
        cur_count_ = 0;
        cur_cycle_ = t_cycle;
    }
    return(0);
}

/**
   set key value.\n
   *******************************************************************************
   set typical 64 bit pointer  key - value\n
   *******************************************************************************
   @param[in]     key key
   @param[in]     p   value
 */
void MIXIPGW::CoreTapTx::SetP(KEY key,void* p){
    if (key == KEY::OPT){
        mpl_ = (CoreMempool*)p;
        PGW_LOG(RTE_LOG_INFO, "CoreTapTx::SetP - CoreMempool: %p/ this: %p/ thread: %p\n", mpl_, this, (void*)pthread_self());
    }else{
        CoreInterface::SetP(key, p);
    }
}

/**
   CoreTapTx connect core to specific ring.\n
   *******************************************************************************
   \n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTapTx::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::TO){
        ring_to_ = (struct rte_ring*)ring;
        printf("[CoreTapTx] -> tx [%p:%s] / thread: %p\n", ring_to_ ,ring_to_->name, (void*)pthread_self());
    }else if (order == ORDER::FROM_00){
        if (ingress_ring_count_ < TAPDUPLICTE_MAX){
            ring_tap_ingress_[ingress_ring_count_] = (struct rte_ring*)ring;
            ingress_ring_count_++;
        }
        printf("[CoreTapTx] -> ingress ring [%p:%s:%u]/ thread: %p\n", ring ,((struct rte_ring*)ring)->name, ingress_ring_count_, (void*)pthread_self());
    }else if (order == ORDER::TO_00){
        if (egress_ring_count_ < TAPDUPLICTE_MAX){
            ring_tap_egress_[egress_ring_count_] = (struct rte_ring*)ring;
            egress_ring_count_++;
        }
        printf("[CoreTapTx] -> egress ring [%p:%s:%u]/ thread: %p\n", ring ,((struct rte_ring*)ring)->name,egress_ring_count_, (void*)pthread_self());
    }else{
        rte_exit(EXIT_FAILURE, "invalid ring order.. cannot be set.(%u)\n", order);
    }
    return(0);
}

