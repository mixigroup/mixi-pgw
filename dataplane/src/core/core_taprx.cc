/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_taprx.cc
    @brief      Tap - Rx Core Implement
*******************************************************************************
	forward or duplicate packets received from tap interface\n
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

#ifndef __APPLE__
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/sockios.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#else
#include <libgen.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <sys/sys_domain.h>
#include <sys/ioctl.h>
#include <net/if_utun.h>
#include <sys/kern_control.h>
#endif

#ifndef TUNNEL_TUNTAP_DEVICE
#define TUNNEL_TUNTAP_DEVICE               "/dev/net/tun"
#endif

using namespace MIXIPGW;

/**
   TapRx Core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     ifnm   interface  name 
   @param[in]     lcore  core id
 */
MIXIPGW::CoreTapRx::CoreTapRx(TXT ifnm,COREID lcore,CoreMempool* mpl):CoreInterface(0, 0, lcore), mpl_(mpl){
    egress_ring_count_ = ingress_ring_count_ = 0;
    bzero(tap_nm_, sizeof(tap_nm_));
    strncpy(tap_nm_, ifnm, MIN(strlen(ifnm), sizeof(tap_nm_)-1));
    ring_test_from_ = NULL;
    for(auto n = 0;n < TAPDUPLICTE_MAX;n++){
        ring_tap_ingress_[n] = ring_tap_egress_[n] = NULL;
    }
#ifdef __TEST_MODE__
    tap_fd_ = -1;
#else
    // preparing tap
    if ((tap_fd_ = CoreTapBase::UtapOpen(ifnm)) < 0){
        rte_exit(EXIT_FAILURE, "cannot open utun(%d/%s)\n", errno, strerror(errno));
    }
#endif
    PGW_LOG(RTE_LOG_INFO, "CoreTapRx::CoreTapRx /lcore: %u /ifnm: %s/ CoreMempool: %p - %p/ this: %p/ thread: %p\n", lcore, ifnm, mpl, mpl_, this, (void*)pthread_self());
}
MIXIPGW::CoreTapRx::~CoreTapRx(){
    PGW_LOG(RTE_LOG_INFO, "CoreTapRx::~CoreTapRx / lcore: %u /CoreMempool:%p/ this: %p/ thread: %p\n", lcore_, mpl_, this, (void*)pthread_self());

    if (tap_fd_ != -1){
        close(tap_fd_);
    }
    tap_fd_ = -1;
}

/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  Core Type [TAPRX] return
 */
TYPE MIXIPGW::CoreTapRx::GetType(void){
    return(TYPE::TAPRX);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreTapRx::GetObjectName(void){
    return("CoreTapRx");
}
/**
  called before cycle loop at once.\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     arg  application instance address
 @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTapRx::BeforeCycle(void* arg){
    if (egress_ring_count_ != TAPDUPLICTE_MAX){
        rte_panic("core tap Rx num of ring.%u\n", egress_ring_count_); 
    }
    return(0);
}

/**
   Tap RX Core : virtual cycle : 1 cycle\n
   *******************************************************************************
     - read packet from tap Interface\n
     - forwarding packet to nic:tx\n
     - case gtpc .\n
       - forward/duplicate packets to ingress[n] or egress[n] core\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTapRx::Cycle(void* arg, uint64_t* cnt){
    uint32_t index,gtpctype;
    struct rte_mempool *mpool = NULL;
    struct rte_mbuf *m = NULL;
    struct private_extension_hdr*   pehdr;
    const uint32_t baselen = (sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) +
                                sizeof(struct udp_hdr) + sizeof(struct gtpc_hdr));
#ifdef __TEST_MODE__
    // from testing input ring 
    int ret = rte_ring_sc_dequeue_burst(ring_test_from_, (void**)input_, 1, NULL);
    if (ret != 1){
        return(0);
    }
    m = input_[0];
    ret = (int)m->pkt_len;
    auto rbf = rte_pktmbuf_mtod(m, char*);
    mpool = mpl_->Ref(0);
#else
    struct timeval tv = {0,10000};
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(tap_fd_, &rfd);
    auto ret = select(tap_fd_+1, &rfd, NULL, NULL, &tv);
    if (ret <= 0){
        return(-1);
    }
    char rbf[MAX_PACKET_SZ] = {0};
    ret = read(tap_fd_, rbf, sizeof(rbf));
    if (unlikely(ret <= 0)){
        return(-1);
    }
    mpool = mpl_->Ref(rte_lcore_id());
    m = rte_pktmbuf_alloc(mpool);
    if (unlikely(m == NULL)){
        rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc.MIXIPGW::CoreTapRx::Cycle(%p)\n", mpool);
    }
    rte_memcpy(rte_pktmbuf_mtod(m, char*), rbf, ret);
    m->nb_segs = 1;
    m->next = NULL;
    m->pkt_len = (uint16_t)ret;
    m->data_len = (uint16_t)ret;
    m->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
#endif
    //
    gtpctype = is_gtpc(m);
    if (gtpctype == GTPC_CREATE_SESSION_RES || gtpctype == GTPC_MODIFY_BEARER_RES){
        // case gtpc@Create Session Response or Modify Bearer Response
        //      forward/duplicate packets to ingress[n] , egress[n].
        // data required for warm-up
        //  from Create Session Request
        //      - sgw gtpu - destination ip address 
        //      - sgw gtpu - teid
        //  from Create Session Respoinse
        //      - bearer context/fteid
        //          - ip address assigned to user 
        //          - teid assigned to user
        //
        // Control Plane stores data required for warm-ups in gtpc@type==0xFF
        // private extension region of CreateSession/ModifyBearer Response.
        //
        // extension data is stored at tail of gtpc packet as private extension==0xFF
        //
        if (ret > (GTPC_PRIVATE_EXT_SIZE + baselen)){
            // referenced to last private extension.
            pehdr = rte_pktmbuf_mtod_offset(m, struct private_extension_hdr*, ret - GTPC_PRIVATE_EXT_SIZE);
            if (ntohs(pehdr->head.length) == (GTPC_PRIVATE_EXT_SIZE - sizeof(gtpc_comm_header_t))){
                if (pehdr->magic == GTPC_PRIVATE_EXT_MAGIC && pehdr->venderid == htons(GTPC_PRIVATE_EXT_VENDERID)){
                    // valid private extension header
                    // --
                    // forward or duplicate packet to ingress/egress worker for warm-up.
                    for(auto n = 0;n < egress_ring_count_;n++){
                        auto mi = rte_pktmbuf_alloc(mpool);
                        rte_memcpy(rte_pktmbuf_mtod(mi, char*), rbf, m->pkt_len);
                        mi->nb_segs = 1;
                        mi->next = NULL;
                        mi->pkt_len = m->pkt_len;
                        mi->data_len = m->pkt_len;
                        mi->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
                        if (unlikely(mi == NULL)){
                            rte_exit(EXIT_FAILURE, "rte_pktmbuf_clone.(%p)\n", mpool);
                        }
                        if ((ret = rte_ring_sp_enqueue(ring_tap_egress_[n], mi)) != 0){
                            PGW_LOG(RTE_LOG_ERR, "egress duplicate.(%u).\n", n);
                            rte_pktmbuf_free(mi);
                        }
                    }
                    for(auto n = 0;n < ingress_ring_count_;n++){
                        auto mi = rte_pktmbuf_alloc(mpool);
                        rte_memcpy(rte_pktmbuf_mtod(mi, char*), rbf, m->pkt_len);
                        mi->nb_segs = 1;
                        mi->next = NULL;
                        mi->pkt_len = m->pkt_len;
                        mi->data_len = m->pkt_len;
                        mi->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
                        if (unlikely(mi == NULL)){
                            rte_exit(EXIT_FAILURE, "rte_pktmbuf_clone.(%p)\n", mpool);
                        }
                        if ((ret = rte_ring_sp_enqueue(ring_tap_ingress_[n], mi)) != 0){
                            PGW_LOG(RTE_LOG_ERR, "ingress duplicate.(%u).\n", n);
                            rte_pktmbuf_free(mi);
                        }
                    }
                }else{
                    PGW_LOG(RTE_LOG_ERR, "invalid magic or vendor id(%u / %u).\n", pehdr->magic, pehdr->venderid);
                }
            }
        }
    }
    ret = rte_ring_sp_enqueue(ring_to_, m);
    if (unlikely(ret == -ENOBUFS)) {
        rte_pktmbuf_free(m);
        // tap rx is very slow compared to dpdk , direct logging is possible.
        PGW_LOG(RTE_LOG_CRIT, "Error no buffer.[CoreTapRx::Cycle]\n");
        return(-1);
    }else if (ret < 0){
        rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
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
void MIXIPGW::CoreTapRx::SetP(KEY key,void* p){
    if (key == KEY::OPT){
        mpl_ = (CoreMempool*)p;
        PGW_LOG(RTE_LOG_INFO, "CoreTapRx::SetP - CoreMempool: %p/ this: %p/ thread: %p\n", mpl_, this, (void*)pthread_self());
    }else{
        CoreInterface::SetP(key, p);
    }
}

/**
   obtain value find by key.\n
   *******************************************************************************
   get typical 32bit unsigned digits  key - value\n
   *******************************************************************************
   @param[in]     key   key
   @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CoreTapRx::GetN(KEY key){
    if(key == KEY::OPT){
        return((VALUE)tap_fd_);
    }
    return(CoreInterface::GetN(key));
}

/**
   connect Tap Base Core to specific ring\n
   *******************************************************************************
   \n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTapRx::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::TO){
        ring_to_ = (struct rte_ring*)ring;
        printf("[CoreTapRx] -> tx [%p:%s] / thread: %p\n", ring_to_ ,ring_to_->name, (void*)pthread_self());
    }else if (order == ORDER::TO_00){
        if (ingress_ring_count_ < TAPDUPLICTE_MAX){
            ring_tap_ingress_[ingress_ring_count_] = (struct rte_ring*)ring;
            ingress_ring_count_++;
        }
        printf("[CoreTapRx] -> ingress ring [%p:%s:%u]/ thread: %p\n", ring ,((struct rte_ring*)ring)->name, ingress_ring_count_, (void*)pthread_self());
    }else if (order == ORDER::TO_01){
        if (egress_ring_count_ < TAPDUPLICTE_MAX){
            ring_tap_egress_[egress_ring_count_] = (struct rte_ring*)ring;
            egress_ring_count_++;
        }
        printf("[CoreTapRx] -> egress ring [%p:%s:%u]/ thread: %p\n", ring ,((struct rte_ring*)ring)->name,egress_ring_count_, (void*)pthread_self());
    }else if (order == ORDER::FROM){
        ring_test_from_ = (struct rte_ring*)ring;
        printf(">>CoreTapRx<< input ring [%p:%s]/ thread: %p\n", ring ,((struct rte_ring*)ring)->name, (void*)pthread_self());
    }else{
        rte_exit(EXIT_FAILURE, "invalid ring order.. cannot be set.(%u)\n", order);
    }
    return(0);
}
