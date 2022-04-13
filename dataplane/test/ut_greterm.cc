#include "test_def.hpp"

#define TUNNEL_FIXED_SRCIP  (0x0a0b0c0d)

typedef struct utparam{
    MIXIPGW::CoreGretermWorker*  target_;
    struct rte_ring*    ring_;
}utparam_t,*utparam_ptr __rte_cache_aligned;

int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1)};

static rte_atomic32_t       __stop = RTE_ATOMIC32_INIT(0);
static struct rte_mempool*  __pktmbuf_pool = NULL;
static utparam_t            __ingress;
static utparam_t            __egress;
struct ether_addr           __src_mac;
struct ether_addr           __dst_mac;
static uint64_t             __counter_s = 0;
static uint64_t             __counter_r = 0;

// local functions
static void ut_ingress(void);
static void ut_egress(void);
static void signal_handler(int s){ rte_atomic32_inc(&__stop); }

static void ut_ingress(void) {
    if (!__ingress.ring_){
        rte_exit(EXIT_FAILURE, "invalid .ring pointer.(ut_ingress)(%p)\n", __ingress.ring_);
        return;
    }
    printf("send to ring.[%p:%s] ..\n",__ingress.ring_, __ingress.ring_->name);

    // generate buffers.
    auto mbuf = rte_pktmbuf_alloc(__pktmbuf_pool);
    if (unlikely(!mbuf)) {
        rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(alloc)(%p)\n", __pktmbuf_pool);
    }
    auto eh = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
    auto ip = (struct ipv4_hdr*)(eh+1);
    auto gre= (uint32_t*)(((char*)(ip+1)) + 12/*  ip option */);
    auto ipi= (struct ipv4_hdr*)(gre+1);
    auto payload = (char*)(ipi + 1);
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    ether_addr_copy(&__src_mac, &eh->s_addr);
    ether_addr_copy(&__dst_mac, &eh->d_addr);
    __counter_s++;
    //
    ip->version_ihl     = (IP_VERSION | 0x08);
    ip->type_of_service = 0;
    ip->packet_id       = 0;
    ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ip->time_to_live    = IP_DEFTTL;
    ip->next_proto_id   = IPPROTO_GRE;
    ip->hdr_checksum    = 0;
    ip->src_addr        = (uint32_t)__counter_s;
    ip->dst_addr        = (uint32_t)(__counter_s+1);
    ip->total_length    = rte_cpu_to_be_16(TEST_BUFFER_M_SZ - sizeof(*eh));
    ip->hdr_checksum    = 0;
    //

    ipi->version_ihl     = (IP_VERSION | 0x05);
    ipi->type_of_service = 0;
    ipi->packet_id       = 0;
    ipi->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ipi->time_to_live    = IP_DEFTTL;
    ipi->next_proto_id   = IPPROTO_UDP;
    ipi->hdr_checksum    = 0;
    ipi->src_addr        = (uint32_t)__counter_s+2;
    ipi->dst_addr        = (uint32_t)(__counter_s+3);
    ipi->total_length    = rte_cpu_to_be_16(TEST_BUFFER_M_SZ - sizeof(*eh) - 28);
    ipi->hdr_checksum    = 0;
        
    memcpy(payload, &__counter_s, sizeof(__counter_s));

    mbuf->pkt_len = mbuf->data_len = TEST_BUFFER_M_SZ;
    mbuf->nb_segs = 1;
    mbuf->next = NULL;
    mbuf->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_TUNNEL_GRE);
    //
    auto ret = rte_ring_sp_enqueue(__ingress.ring_, mbuf);
    if (unlikely(ret == -ENOBUFS)) {
        printf("no buffer.\n");
        return;
    }else if (ret < 0){
        rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
    }
    uint64_t cycle = 0;
    if (__ingress.target_->Cycle(NULL, &cycle) != 0){
        rte_exit(EXIT_FAILURE, "failed. Cycle(drv_request_target)(%p - %p)\n", __ingress.target_, __ingress.ring_);
    }
}
static void ut_exchanged(struct rte_mbuf* pkt){
    // packet size reduce ip (32) + gre
    assert(pkt->pkt_len==(TEST_BUFFER_M_SZ-36));
    assert(pkt->data_len==(TEST_BUFFER_M_SZ-36));

    // ip address out-side, inner ip header before decapsulate.
    auto ip = rte_pktmbuf_mtod_offset(pkt, struct ipv4_hdr*, sizeof(ether_hdr));
    assert(ip->src_addr==(uint32_t)(__counter_r + 2));
    assert(ip->dst_addr==(uint32_t)(__counter_r + 3));
    assert(ip->next_proto_id==IPPROTO_UDP);
    assert(ip->version_ihl==(IP_VERSION | 0x05));

    // sequence number : == payload
    auto payload = rte_pktmbuf_mtod_offset(pkt, uint32_t*, 34);
    printf("egress checked(%u : %u).\n", (unsigned)__counter_r,(unsigned)*payload);
    //
    if (__counter_r > TEST_COUNT){

        printf("\n>>>>>>>>>>\n[gre terminate Worker]\n\tSucceeded. test completed.\n>>>>>>>>>\n");
        rte_atomic32_inc(&__stop);  // stop testing.
    }
}
static void ut_egress(void) {
    struct rte_mbuf *pkt = NULL;
    if (!__egress.ring_){
        rte_exit(EXIT_FAILURE, "missing egress ring...\n");
        return;
    }
    auto ret = rte_ring_sc_dequeue(__egress.ring_, (void**)&pkt);
    if (ret == 0){
        if (RTE_ETH_IS_IPV4_HDR(pkt->packet_type)) {
            __counter_r++;
            ut_exchanged(pkt);
        }else if (RTE_ETH_IS_IPV6_HDR(pkt->packet_type)) {
            rte_exit(EXIT_FAILURE, "not implemented ipv6...(ut_egress)\n");
        }else{
            rte_exit(EXIT_FAILURE, "not implemented not ipv4...(ut_egress)\n");
        }
    }
}

static int main_loop(__rte_unused void *arg){
    int32_t f_stop;
    const unsigned lcore_id = rte_lcore_id();
    //
    if (lcore_id == COREID_INGRESS) {
        while (1) {
            f_stop = rte_atomic32_read(&__stop);
            if (f_stop){ break; }
            ut_ingress();
            usleep(TEST_DELAY);
        }
    } else if (lcore_id == COREID_EGRESS) {
        while (1) {
            f_stop = rte_atomic32_read(&__stop);
            if (f_stop){ break; }
            ut_egress();
            usleep(TEST_DELAY/10);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    //
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    __pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id());
    if (__pktmbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }
    rte_eth_macaddr_get(0, &__src_mac);
    rte_eth_macaddr_get(1, &__dst_mac);

    __ingress.ring_ = rte_ring_create("ingress_ring", 1024, rte_lcore_to_socket_id(COREID_INGRESS), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __egress.ring_ = rte_ring_create("egress_ring",  1024, rte_lcore_to_socket_id(COREID_EGRESS), RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (!__ingress.ring_ || !__egress.ring_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    __ingress.target_ = new MIXIPGW::CoreGretermWorker(0,TUNNEL_FIXED_SRCIP);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __ingress.ring_);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::TO,   __egress.ring_);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __egress.target_ = __ingress.target_;
    //
    rte_eal_mp_remote_launch(main_loop, NULL, CALL_MASTER);
    unsigned lcore;
    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return(-1);
        }
    }
    return(0);
}
