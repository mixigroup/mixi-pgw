/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_counter_ingress_worker.cc
    @brief      counter ingress worker core implemented
*******************************************************************************
	counter - ingress : \n
     \n
*******************************************************************************
    @date       created(02/may/2018)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 02/may/2018 
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;

/**
   counter ingress worker core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     lcore  core id
   @param[in]     type   0=ingress/1=egress
 */
MIXIPGW::CoreCounterIngressWorker::CoreCounterIngressWorker(COREID lcore):CoreCounterWorker(lcore, COUNTER_INGRESS){
}
/**
   counter ingress - logger worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::CoreCounterIngressWorker::~CoreCounterIngressWorker(){
}

TYPE MIXIPGW::CoreCounterIngressWorker::GetType(void){
    return(TYPE::COUNTER);
}

/**
   counter ingress - logger worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterIngressWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;
    auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_, NULL);
    if (unlikely(nburst == -ENOENT)) {
        return(0);
    }
    if (unlikely(nburst == 0)){
        if (likely(((*cnt)>=flush_delay_))) {
            ForwardTransfer(arg);
            BurstFlushPassthrough(arg);
            (*cnt) = 0;
        }
        return(0);
    }
    // prefetch first.
    for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
    }
    // count every packets , while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
        auto udat = (mbuf_userdat_ptr)&(input_[n]->udata64);

        // low speed + color!=GREEN => drop
        if (udat->mode == POLICER_MODE_LOW && udat->color != POLICER_COLOR_GREEN){
            rte_pktmbuf_free(input_[n]);
        }else if (udat->mode == POLICER_MODE_3G && udat->color == POLICER_COLOR_RED){
            rte_pktmbuf_free(input_[n]);
        }else{
            if ((udat->flag & MBFU_FLAG_PDU)){
                Counting(udat->teid, udat->size, udat->mode);
            }
            SendOrBufferPassThrough(arg, input_[n]);
        }
    }
    // remained.
    for (; n < nburst; n++) {
        auto udat = (mbuf_userdat_ptr)&(input_[n]->udata64);
        // low speed + color!=GREEN => drop
        if (udat->mode == POLICER_MODE_LOW && udat->color != POLICER_COLOR_GREEN){
            rte_pktmbuf_free(input_[n]);
        }else if (udat->mode == POLICER_MODE_3G && udat->color == POLICER_COLOR_RED){
            rte_pktmbuf_free(input_[n]);
        }else{
            if ((udat->flag & MBFU_FLAG_PDU)){
                Counting(udat->teid, udat->size, udat->mode);
            }
            SendOrBufferPassThrough(arg, input_[n]);
        }
    }
    // every xxx K times(depends on environment), transfer counter aggregate values.
    if (likely(((*cnt)>=flush_delay_))) {
        ForwardTransfer(arg);
        BurstFlushPassthrough(arg);
        (*cnt) = 0;
    }
    return(0);
}
