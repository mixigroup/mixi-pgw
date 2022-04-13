/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_transfer_worker.cc
    @brief      transfer worker core implemented
*******************************************************************************
	transfer : \n
    receive aggregated traffic values from counter worker around every 1 sec,\n
    notify bufferred received packet every 128 records to diameter server(tcp).\n
*******************************************************************************
    @date       created(15/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 15/oct/2017 
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace MIXIPGW;


/**
   transfer worker core interface  :  constructor \n
   *******************************************************************************
   transfer\n
   *******************************************************************************
   @param[in]     lcore  core id
   @param[in]     srcip  source ip
   @param[in]     srcport   diameter src port
   @param[in]     dstip  destination ip
   @param[in]     dstport   diameter dst port
   @param[in]     cpuid  host stack sending thread cpu/core
 */
MIXIPGW::CoreTransferWorker::CoreTransferWorker(COREID lcore, TXT srcip, unsigned srcport, TXT dstip, unsigned dstport,unsigned cpuid)
        :CoreInterface(0, 0, lcore),quit_(0),sock_(-1),cpuid_(cpuid){
    transfer_bytes_ =  drop_bytes_ = transfer_count_ = 0;
    struct sockaddr_in  src_addr, dst_addr;
    int on = 1;
    // preparing socket
    if ((sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        rte_exit(EXIT_FAILURE, "create socket(tcp): %s\n", strerror(errno));
    }
    src_addr.sin_family    = AF_INET;
    src_addr.sin_port      = htons(srcport);
    inet_pton(AF_INET, srcip ,&src_addr.sin_addr.s_addr);

    dst_addr.sin_family    = AF_INET;
    dst_addr.sin_port      = htons(dstport);
    inet_pton(AF_INET, dstip ,&dst_addr.sin_addr.s_addr);
    //
    if (bind(sock_, (struct sockaddr*)&src_addr, sizeof(src_addr))) {
        rte_exit(EXIT_FAILURE, "bind..[src] %s\n", strerror(errno));
    }
    on = 1;
    if (setsockopt(sock_, SOL_TCP, TCP_NODELAY, &on, sizeof(on)) < 0){
        rte_exit(EXIT_FAILURE, "nagle off.. %s\n", strerror(errno));
    }
    on = 1;
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        rte_exit(EXIT_FAILURE, "reuse addr.. %s\n", strerror(errno));
    }
    on = 8192; // send in every 1 packet
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, &on, sizeof(on)) < 0){
        rte_exit(EXIT_FAILURE, "send buffer.. %s\n", strerror(errno));
    }
    if (connect(sock_, (struct sockaddr *)&dst_addr, sizeof(dst_addr)) < 0){
            rte_exit(EXIT_FAILURE, "connect .. %s\n", strerror(errno));
    }
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/srcip: %s/srcport: %u/dstip: %s/dstport: %u/cpuid: %u\n", lcore, srcip, srcport, dstip, dstport, cpuid);
}
/**
   transfer worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CoreTransferWorker::~CoreTransferWorker(){
    quit_ = 1;
    close(sock_);
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CoreTransferWorker::GetType(void){
    return(TYPE::WORKER_TRANSFER);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreTransferWorker::GetObjectName(void){
    return("CoreTransferWorker");
}
/**
  obtain value find by key.\n
 *******************************************************************************
 override, call CoreInterface::GetN ,except KEY::OPT\n
 *******************************************************************************
 @param[in]     key key
 @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CoreTransferWorker::GetN(KEY t){
    if (t == KEY::OPT){
        return(cpuid_);
    }
    return(CoreInterface::GetN(t));
}

/**
   transfer - transfer , connect woker core  to specific ring \n
   *******************************************************************************
   transfer in worker core  , [TO]direction=internal specific software ring\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTransferWorker::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){              // from = counter -> transfer between Indicates ring 
        ring_fm_ = (struct rte_ring*)ring;
    }else if (order == ORDER::TO){          // to   = internal -> to host stack
        ring_to_ = (struct rte_ring*)ring;
    }else{
        rte_exit(EXIT_FAILURE, "unknown ring order.(%u)\n", order);
    }
    return(0);
}
/**
   transfer worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 batch process, set bulk+dequeue packets to host stack buffer\n
   release mbuf after send completed.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTransferWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;

    // debug output
    if (cnt && (*cnt) && ((*cnt) == (uint64_t)-1 || (*cnt)%(512*1024*1024)==0)){
        PGW_LOG(RTE_LOG_INFO, "performance(%p)\n"
		"core transfer total : %lu/ drop : %lu bytes/ transfer : %lu bytes \n",
               (void*)pthread_self(), transfer_count_, drop_bytes_, transfer_bytes_);
               transfer_bytes_ =  drop_bytes_ = transfer_count_ = 0;
        return(0);
    }
    /* input buffer (used main cycle processing) = input_ */
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
    // enqueue to host stack thread while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
        SendOrBuffer(arg, input_[n]);
    }
    // remained.
    for (; n < nburst; n++) {
        SendOrBuffer(arg, input_[n]);
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
RETCD MIXIPGW::CoreTransferWorker::VirtualCycle(void* arg,uint64_t* cnt){
    // dequeue from internal-connected ring
    /* input buffer (used internal ring) = input_int_ */
    auto nburst = rte_ring_sc_dequeue_burst(ring_to_, (void**)input_int_, burst_to_, NULL);

    // host stack processing is bottleneck,
    // so no compile hints are needed.
    if (nburst == -ENOENT || nburst == 0) {
        return(0);
    }
    printf(">> to send(%u)\n", nburst);
    for (auto n = 0; n < nburst; n++) {
        auto payload = rte_pktmbuf_mtod(input_int_[n], void*);
        auto payload_len = rte_pktmbuf_pkt_len(input_int_[n]);
        if (Send(payload, payload_len) < 0){
            PGW_LOG(RTE_LOG_CRIT, "send failed.\n");
            drop_bytes_ += payload_len;
        }else{
            transfer_bytes_ += payload_len;
        }
        transfer_count_ ++;
        rte_pktmbuf_free(input_int_[n]);
    }
    return(0);
}
/**
   send on tcp socket.\n
   *******************************************************************************
   transfer data to diameter server.\n
   *******************************************************************************
   @param[in]     data payload
   @param[in]     len  payload length
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTransferWorker::Send(void* data, unsigned len){
    int		ret      = 0;
    int		allsend  = 0;
    int		sendlen  = 0;
    int		errored  = 0;
    int		retrycnt = 0;
    char*	senddata = (char*)data;
    fd_set	sendfd;
    struct timeval  timeout;
 
//  printf(">> Send(%p: %u)\n", (void*)pthread_self(), len);
    while(1){
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100000;
        FD_ZERO(&sendfd);
        FD_SET(sock_,&sendfd);
        ret = select(sock_ + 1,NULL,&sendfd,NULL,&timeout);
        //retry count is over.
        if (retrycnt ++ > 10){
            errored = 1;
            break;
        }
        if (ret == -1){			//select failed.
            if (errno == EINTR){	continue;	}
            errored = 1;
            break;
        }else if(ret > 0){		//sended. any bytes data..
            if(FD_ISSET(sock_,&sendfd)){
                sendlen = send(sock_,(senddata + allsend),(len - allsend),0);
                if (sendlen < 0){
                    // terminate process when tcp connection
                    //  with diameter server was done.
                    rte_exit(EXIT_FAILURE, "failed. socket send: %s\n", strerror(errno));
                    break;
                }
                allsend += sendlen;
                if (allsend >= len){	break;	}
            }
        }else{
            continue;
        }
    }
    return(errored==0?allsend:-1);
}
