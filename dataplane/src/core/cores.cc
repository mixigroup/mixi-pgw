/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       cores.cc
    @brief      core common interface implement
*******************************************************************************
	\n
	\n
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
#include "../../inc/static_conf.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;

/**
   Core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     port   portid
   @param[in]     queue  queue id
   @param[in]     lcore  core id
 */
MIXIPGW::CoreInterface::CoreInterface(PORTID port, QUEUEID queue, COREID lcore)
    :port_(port), queue_(queue), lcore_(lcore), burst_fm_(0), burst_to_(0), cnt_pgw_in_(0), cnt_pgw_eg_(0), socket_(0){
#if ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21 
    #warning arch type .15-21.
#else
     IsAvailableCore(lcore);
#endif
    bzero(&output_cnt_, sizeof(output_cnt_));
    bzero(output_, sizeof(output_));
    bzero(input_, sizeof(input_));
    cnt_rx_ = cnt_tx_ = cnt_wk_ = cnt_pgw_in_ = cnt_pgw_eg_ = burst_fm_ = burst_to_ = 0;
    ring_fm_ = ring_to_ = NULL;
}
/**
   is available core\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     lcore  core id
   @return RETCD  0==available(okay) , 0!=error
 */
RETCD MIXIPGW::CoreInterface::IsAvailableCore(COREID lcore){
    // is available core
    if (rte_lcore_is_enabled(lcore) == 0){
        rte_exit(EXIT_FAILURE, "cannot use lcore(%u)\n", lcore);
    }
    return(0);
}
/**
   execute on defferent virtual core except main core.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreInterface::VirtualCycle(void* arg,uint64_t* cnt){
    return(0);
}

/**
   called before cycle loop at once.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreInterface::BeforeCycle(void* arg){
    return(0);
}
/**
   called after cycle loop at once.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreInterface::AfterCycle(void* arg){
    return(0);
}

/**
   packet drop between last function call and current function call.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @parma[in/out]   obj   counter return value 
   @return bool true : dropped/ false : didnot dropped
 */
bool MIXIPGW::CoreInterface::HasDropped(CoreCounter* obj){
    auto ret = counter_.HasDropped();
    (*obj) = counter_;
    counter_.Clear();
    return(ret);
}

/**
   obtain value find by key.\n
   *******************************************************************************
   get 32 bits unsigned integer value. key - value\n
   *******************************************************************************
   @param[in]     key   key
   @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CoreInterface::GetN(KEY key){
    switch(key){
        case KEY::CNT_RX:       return(cnt_rx_);
        case KEY::CNT_TX:       return(cnt_tx_);
        case KEY::CNT_WORKER:   return(cnt_wk_);
        case KEY::BURST_FROM:   return(burst_fm_);
        case KEY::BURST_TO:     return(burst_to_);
        case KEY::LCORE:        return(lcore_);
        case KEY::PORT:         return(port_);
        case KEY::QUEUE:        return(queue_);
        case KEY::CNT_PGW_INGRESS:return(cnt_pgw_in_);
        case KEY::CNT_PGW_EGRESS:return(cnt_pgw_eg_);
        case KEY::SOCKET:       return(socket_);
    }
    return((unsigned)-1);
}
/**
   set key value.\n
   *******************************************************************************
   set 32 bits unsigned integer value  key - value\n
   *******************************************************************************
   @param[in]     key key
   @param[in]     v   value
 */
void MIXIPGW::CoreInterface::SetN(KEY key,VALUE v){
    PGW_LOG(RTE_LOG_INFO, "SetN: %s(%u)[%u = %u](%p)\n", GetObjectName(), GetN(KEY::LCORE), key, v, (void*)pthread_self());
    switch(key){
        case KEY::CNT_RX:    cnt_rx_ = v;   break;
        case KEY::CNT_TX:    cnt_tx_ = v;   break;
        case KEY::CNT_WORKER:cnt_wk_ = v;   break;
        case KEY::BURST_FROM:burst_fm_ = v; break;
        case KEY::BURST_TO:  burst_to_ = v; break;
        case KEY::CNT_PGW_INGRESS:cnt_pgw_in_ = v; break;
        case KEY::CNT_PGW_EGRESS:cnt_pgw_eg_ = v; break;
        case KEY::FLUSH_DELAY:flush_delay_ = v; break;
        case KEY::SOCKET:    socket_= v; break;
    }
}
/**
   set key value.\n
   *******************************************************************************
   set typical 64 bit pointer  key - value\n
   *******************************************************************************
   @param[in]     key key
   @param[in]     p   value
 */
void MIXIPGW::CoreInterface::SetP(KEY key,void* p){
    PGW_LOG(RTE_LOG_INFO, "CoreInterface::SetP: %p/ this: %p\n", p, this);

}

/**
   destination side ring:(To): bulk enqueue to core\n
   *******************************************************************************
   burst enqueue packets into Tx ring.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     mbuf mbuf address transferred to next Core.  address 
 */
void MIXIPGW::CoreInterface::SendOrBuffer(void* arg,  struct rte_mbuf* mbuf){
    auto curpos = output_cnt_;

    // stores into buffer without sending out immediately(< buset size)
    output_[curpos ++] = mbuf;
    if (likely(curpos < burst_to_)) {
        output_cnt_ = curpos;
        return;
    }
    // queuing immediately(>= buset size)
    auto ret = rte_ring_sp_enqueue_bulk(ring_to_, (void **) output_, burst_to_, NULL);
    // drop (not enough buffer)
    if (unlikely(ret == 0)) {
        for (auto k = 0; k < burst_to_; k ++) {
            rte_pktmbuf_free(output_[k]);
        }
        counter_.Inc(0, burst_to_, 0, 0);
    }else if (unlikely(ret < 0)){
        counter_.Inc(0, burst_to_, 0, 0);
    }
    output_cnt_ = 0;
}

/**
   transfer remained packets in burst buffer to destination-side ring.\n
   *******************************************************************************
   time out burst enqueue\n
   *******************************************************************************
   @param[in]     arg  application instance address
 */
void MIXIPGW::CoreInterface::BurstFlush(void* arg){
    auto lcore  = rte_lcore_id();
    //
    if (output_cnt_ > 0){
        auto ret = rte_ring_sp_enqueue_bulk(ring_to_, (void **) output_, output_cnt_, NULL);
        if (unlikely(ret != output_cnt_)) {
            for (auto k = ret; k < output_cnt_; k ++) {
                rte_pktmbuf_free(output_[k]);
            }
            counter_.Inc(0, output_cnt_, 0, 0);
        }else if (unlikely(ret < 0)){
            counter_.Inc(0, output_cnt_, 0, 0);
        }
    }
    output_cnt_ = 0;
}
