/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_counter_worker.cc
    @brief      counter worker core implemented
*******************************************************************************
	counter : \n
    send aggregated data (counting is primary aggregation) every 1 sec.\n
    enqueue via dpdk software ring : notify\n
    multi-stage design is necessary for dpdk to run at high speeds such as 10Gbps/14.8Mpps.\n
    any connecting interface must be designed for high speed implmentaion
        without exception so as not to become bottleneck.\n
*******************************************************************************
    @date       created(10/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 10/oct/2017
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;

static inline uint32_t BE24(uint32_t len){
    uint32_t ptr = len;
    uint32_t ret = ((uint8_t)(((char*)&ptr)[0]&0xff) << 16) +
        ((uint8_t)(((char*)&ptr)[1]&0xff) << 8) +
        ((uint8_t)(((char*)&ptr)[2]&0xff));
    return(ret);
}


/**
   counter worker core interface  :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     lcore  core id
   @param[in]     type   0=ingress/1=egress
 */
MIXIPGW::CoreCounterWorker::CoreCounterWorker(COREID lcore, unsigned type)
        :CoreInterface(0, 0, lcore), mpl_(NULL), cur_(0), type_(type){
    forward_cycle_ = forward_count_ =
    virtual_cycle_ = virtual_count_ =
    counting_cycle_= counting_count_ =
    traffic_bytes_ = 0;
    ring_pass_through_ = NULL;
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u /CoreMempool: %p/ type: %u\n", lcore, mpl_, type);
}
/**
   counter - logger worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::CoreCounterWorker::~CoreCounterWorker(){
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CoreCounterWorker::GetType(void){
    return(TYPE::WORKER);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreCounterWorker::GetObjectName(void){
    return("CoreCounterWorker");
}

/**
   set key value.\n
   *******************************************************************************
   set typical 64 bit pointer  key - value\n
   *******************************************************************************
   @param[in]     key key
   @param[in]     p   value
 */
void MIXIPGW::CoreCounterWorker::SetP(KEY key,void* p){
    if (key == KEY::OPT){
        mpl_ = (CoreMempool*)p;
    }
    return(CoreInterface::SetP(key, p));
}

/**
   counter - logger , connect woker core  to specific ring \n
   *******************************************************************************
   counter - logger in worker core  , [FROM/TO] both direction are software ring\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterWorker::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){              // from = Rx -> Worker between Indicates ring 
        ring_fm_ = (struct rte_ring*)ring;
    }else if (order == ORDER::TO) {         // to   = Counter -> next worker
        ring_to_ = (struct rte_ring *) ring;
    }else if (order == ORDER::EXTEND){      // extend = Counter -> PassThrough
        ring_pass_through_ = (struct rte_ring *) ring;
    }else{
        rte_exit(EXIT_FAILURE, "unknown ring order.(%u)\n", order);
    }
    return(0);
}
/**
   counter - logger worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 patch process, bulk + dequeued packets parse one by one, and counting\n
   release mbuf immediately after counting\n
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterWorker::Cycle(void* arg, uint64_t* cnt){
    auto start_time = rte_rdtsc();
    int n = 0;
    auto min_len = (sizeof(ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr) + sizeof(struct ipv4_hdr));
    //
    auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_, NULL);
    if (unlikely(nburst == -ENOENT)) {
        return(0);
    }
    if (unlikely(nburst == 0)){
        if (unlikely(((*cnt)%flush_delay_)==0)) {
            ForwardTransfer(arg);
        }
        return(0);
    }
    virtual_count_ += nburst;
    // prefetch first.
    for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
    }
    // count every packets , while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));

        // ipv4
        if (RTE_ETH_IS_IPV4_HDR(input_[n]->packet_type)) {
            auto ipv4 = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr*, sizeof(struct ether_hdr));
            if (ipv4->next_proto_id == IPPROTO_UDP){
                auto udp = rte_pktmbuf_mtod_offset(input_[n], struct udp_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
                if (udp->dst_port == htons(GTPU_PORT)){
                    auto gtpu = rte_pktmbuf_mtod_offset(input_[n], struct gtpu_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
                    auto used = rte_cpu_to_be_16(gtpu->length);
                    auto ofset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
                    auto inip4 = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);
                    auto ipkey_dst = inip4->dst_addr;
                    auto ipkey_src = inip4->src_addr;
                    if ((inip4->version_ihl&0xf0) != 0x40){
                        auto inip6 = rte_pktmbuf_mtod_offset(input_[n], struct ipv6_hdr *,ofset);
                        rte_memcpy(&ipkey_dst, &inip6->dst_addr[4], 4);
                        rte_memcpy(&ipkey_src, &inip6->src_addr[4], 4);
                    }
                    if (gtpu->u.v1_flags.version == GTPU_VERSION_1 && gtpu->type==0xFF){
                        if (type_ == COUNTER_INGRESS){
                            Counting(ntohl(ipkey_src), used, 0);
                        }else{
                            Counting(ntohl(ipkey_dst), used, 0);
                        }
                    }
                }
            }
        }
        rte_pktmbuf_free(input_[n]);
    }
    // remained.
    for (; n < nburst; n++) {
        // ipv4
        if (RTE_ETH_IS_IPV4_HDR(input_[n]->packet_type)) {
            auto ipv4 = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr*, sizeof(struct ether_hdr));
            if (ipv4->next_proto_id == IPPROTO_UDP){
                auto udp = rte_pktmbuf_mtod_offset(input_[n], struct udp_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
                if (udp->dst_port == htons(GTPU_PORT)){
                    auto gtpu = rte_pktmbuf_mtod_offset(input_[n], struct gtpu_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
                    auto used = rte_cpu_to_be_16(gtpu->length);
                    auto ofset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
                    auto inip4 = rte_pktmbuf_mtod_offset(input_[n], struct ipv4_hdr *,ofset);
                    auto ipkey_dst = inip4->dst_addr;
                    auto ipkey_src = inip4->src_addr;
                    if ((inip4->version_ihl&0xf0) != 0x40){
                        auto inip6 = rte_pktmbuf_mtod_offset(input_[n], struct ipv6_hdr *,ofset);
                        rte_memcpy(&ipkey_dst, &inip6->dst_addr[4], 4);
                        rte_memcpy(&ipkey_src, &inip6->src_addr[4], 4);
                    }
                    if (gtpu->u.v1_flags.version == GTPU_VERSION_1 && gtpu->type==0xFF){
                        if (type_ == COUNTER_INGRESS){
                            Counting(ntohl(ipkey_src), used, 0);
                        }else{
                            Counting(ntohl(ipkey_dst), used, 0);
                        }
                    }
                }
            }
        }
        rte_pktmbuf_free(input_[n]);
    }
    // every xxx K times(depends on environment), transfer counter aggregate values.
    if (unlikely(((*cnt)%flush_delay_)==0)) {
        ForwardTransfer(arg);
    }
    virtual_cycle_ += (rte_rdtsc() - start_time);

    return(0);
}
/**
   primary aggregation :  event packet notify to transfer\n
   *******************************************************************************
   distribute 16 maps, bufferring aggregated values\n
   *******************************************************************************
   @param[in]     key  aggregated key(ipv4 or ipv6[4-7])
   @param[in]     used used traffic
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterWorker::Counting(uint32_t key, uint16_t used, uint32_t mode){
    auto start_time = rte_rdtsc();
    // primary aggregate map by simply modulo.
    auto idx = (key%COUNT_MAP_SZ);
    auto it  = map_[idx].find(key);
    auto val = (uint32_t)used;
    if (it != map_[idx].end()){
        if (mode == POLICER_MODE_LOW){     (it->second).used_low += val;
        }else if (mode == POLICER_MODE_3G){(it->second).used_3g += val;
        }else{                             (it->second).used += val;
        }		
    }else{
        counting_item_t itm;
        bzero(&itm, sizeof(itm));
        itm.key  = time(NULL);
        if (mode == POLICER_MODE_LOW){     itm.used_low = val;
        }else if (mode == POLICER_MODE_3G){itm.used_3g = val;
        }else{                             itm.used = val;
        }		
        map_[idx][key] = itm;
    }
    counting_cycle_ += (rte_rdtsc() - start_time);
    counting_count_ ++;
    traffic_bytes_  += (uint64_t)val;

    return(0);
}
/**
   forwarding aggregated values to transfer in bulk.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg
 */
void MIXIPGW::CoreCounterWorker::ForwardTransfer(void* arg){
    auto start_time = rte_rdtsc();
    auto mpool = mpl_->Ref(rte_lcore_id());
    size_t  counter,dcounter;
    int     n,m,map_tail,ringid;
    auto curtm = time(NULL);

    // temporary array with current postion as start-position.
    unsigned sort[COUNT_MAP_SZ] = {0,};

    // setup address to sorted array from beginning.
    /*
     * example,prev     map_[0] -> map_[1] -> map_[2] ... cur_ = 3 , when send completed.
     *         current  map_[3] -> map_[4] -> map_[5] ... cur_ = 6
     *         next     map_[6] -> map_[7] -> map_[8] ... cur_ = 9
     *
     */
    for(m = 0,n = cur_;n < COUNT_MAP_SZ;n++,m++){ sort[m] = n; }
    for(n = 0;n < cur_;n++,m++){ sort[m] = n; }

    // iterate from current -> tail,
    // packing data into ring, and send
    dcounter = 0;
    counter = 0;
    ringid = 0;
    // allocate first ring
    auto pring = rte_pktmbuf_alloc(mpool);
    if (unlikely(!pring)) {
        rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(alloc)(%p : begin)\n", mpool);
    }
    auto counting   = rte_pktmbuf_mtod(pring, counting_ptr);
    auto item       = &counting->items[0];
    auto skip_next  = false;
    counting->type  = type_;
    counting->len   = BE24(sizeof(counting_t));
    pring->pkt_len  = pring->data_len = sizeof(counting_t);
    //
    for(n = 0;n < COUNT_MAP_SZ && !skip_next;n++){
//      printf("map loop [%u -> %u]\n",n, sort[n]);
        for(auto it = map_[sort[n]].begin();it != map_[sort[n]].end();){
            if ((it->second).used > THRESHOLD_TRANSFER || ((it->second).key < (curtm - 60))){
                item[counter].key  = (it->first);
                item[counter].used = (it->second).used;
                item[counter].used_3g = (it->second).used_3g;
                item[counter].used_low = (it->second).used_low;

                counter++;
                dcounter++;
                counting->count    = counter;
                //
                if (counter >= TRANSFER_RING_BURST){
                    // ring is full -> transfer
                    SendOrBuffer(arg, pring);
                    counter = 0;
                    ringid ++;
                    // use next burst timing.
                    if (ringid >= TRANSFER_RING_SZ){
                        skip_next = true;
                        break;
                    }
                    // allocate next ring
                    pring = rte_pktmbuf_alloc(mpool);
                    if (unlikely(!pring)) {
                        rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(alloc)(%p : %u)\n", mpool, ringid);
                    }

                    counting = rte_pktmbuf_mtod(pring, counting_ptr);
                    counting->type = type_;
                    counting->len = BE24(sizeof(counting_t));
                    item = &counting->items[0];
                    pring->pkt_len = pring->data_len = sizeof(counting_t);
                }
                it = map_[sort[n]].erase(it);
                //(it->second) = 0;
                //++it;
                if (dcounter >= DCOUNTER_LIMIT){
                    skip_next = true;
                    break;
                }
            }else{
                ++it;
            }
            // tail of map-index
            map_tail = (n+1);
        }
    }
    // remaining => send
    if (counter > 0){
        SendOrBuffer(arg, pring);
    }
    // empty operations
    if (dcounter == 0){
        rte_pktmbuf_free(pring);
    }
    forward_cycle_ += (rte_rdtsc() - start_time);
    forward_count_ += (uint64_t)dcounter;

    // advance current map position
    if (map_tail == COUNT_MAP_SZ){
        cur_ = 0;
    }else{
        cur_ += map_tail;
        if (cur_ >= COUNT_MAP_SZ){
            cur_ = (cur_ % COUNT_MAP_SZ);
        }
    }
    //
    BurstFlush(arg);
//  printf("finished  : map [all : %u/ cur : %u/ tail: %u / next : %u]\n", COUNT_MAP_SZ, prev_cur, map_tail, cur_);
}

/**
   bulk enqueue to worker core.\n
   *******************************************************************************
   burst enqueue to ring of worker core.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     mbuf mbuf address transferred to next Core.  address 
 */
void MIXIPGW::CoreCounterWorker::SendOrBufferPassThrough(void* arg, struct rte_mbuf* mbuf){
    auto curpos = output_cnt_pass_through_;
    if (!ring_pass_through_){
        return;
    }
    // stores into buffer without sending out immediately(< buset size)
    output_pass_through_[curpos ++] = mbuf;
    if (likely(curpos < burst_to_)) {
        output_cnt_pass_through_ = curpos;
        return;
    }
    // queuing immediately(>= buset size)
    auto ret = rte_ring_sp_enqueue_bulk(ring_pass_through_, (void **) output_pass_through_, burst_to_, NULL);
    // drop (not enough buffer)
    if (unlikely(ret == 0)) {
        for (auto k = 0; k < burst_to_; k ++) {
            rte_pktmbuf_free(output_pass_through_[k]);
        }
        counter_.Inc(0, burst_to_, 0, 0);
    }
    output_cnt_pass_through_ = 0;
}
/**
   send out remaining packets to worker ring\n
   *******************************************************************************
   time out burst enqueue\n
   *******************************************************************************
   @param[in]     arg  application instance address
 */
void MIXIPGW::CoreCounterWorker::BurstFlushPassthrough(void* arg){
    if (unlikely(output_cnt_pass_through_==0)){
        return;
    }
    if (!ring_pass_through_){
        return;
    }
    // transfer packet to worker
    auto ret = rte_ring_sp_enqueue_bulk(ring_pass_through_, (void **) output_pass_through_, output_cnt_pass_through_, NULL);
    if (unlikely(ret == 0)) {
        for (auto k = 0; k < output_cnt_pass_through_; k ++) {
            rte_pktmbuf_free(output_pass_through_[k]);
        }
        counter_.Inc(0, (uint64_t)output_cnt_pass_through_, 0, 0);
    }
    output_cnt_pass_through_ = 0;
}
