typedef struct utparam{
    MIXIPGW::CoreInterface*  target_;
    struct rte_ring*    ring_;
}utparam_t,*utparam_ptr __rte_cache_aligned;

static struct rte_mempool*  __pktmbuf_pool = NULL;
static rte_atomic32_t       __stop = RTE_ATOMIC32_INIT(0);
static utparam_t            __ingress;
static utparam_t            __egress;
struct ether_addr           __src_mac;
struct ether_addr           __dst_mac;
static uint64_t             __counter_s = 0;
static uint64_t             __counter_r = 0;

static void test_ingress(void);
static void test_egress(void);

#ifdef USE_VIRTUAL_CYCLE
static utparam_t      __internal;
static void test_virtual(void);
#endif

static const uint32_t   __BASE_IP_  = 0x00000000;

namespace MIXIPGW{
  int PGW_LOG_LEVEL[PGW_LOG_CORES] = {  (RTE_LOG_DEBUG+1),
                                        (RTE_LOG_DEBUG+1),
                                        (RTE_LOG_DEBUG+1),
                                        (RTE_LOG_DEBUG+1),};
};


class MyMemPool:public CoreMempool{
    virtual struct rte_mempool* Ref(COREID coreid){
        return(__pktmbuf_pool);
    }
};
//
static int test_loop(__rte_unused void *arg){
    int32_t f_stop;
    const unsigned lcore_id = rte_lcore_id();
    //
    if (lcore_id == COREID_INGRESS) {
        printf(">>> start ingress.(%u)\n", lcore_id);
        while (1) {
            f_stop = rte_atomic32_read(&__stop);
            if (f_stop){ break; }
            test_ingress();
        }
        printf("<<< end ingress.(%u)\n", lcore_id);
    } else if (lcore_id == COREID_EGRESS) {
        printf(">>> start egress.(%u)\n", lcore_id);
        while (1) {
            f_stop = rte_atomic32_read(&__stop);
            if (f_stop){ break; }
            test_egress();
        }
        printf("<<< end egress.(%u)\n", lcore_id);
#ifdef USE_VIRTUAL_CYCLE
    } else if (lcore_id == COREID_INTERNAL) {
        printf(">>> start virtual.(%u)\n", lcore_id);
        while (1) {
            f_stop = rte_atomic32_read(&__stop);
            if (f_stop){ break; }
            test_virtual();
        }
        printf("<<< end virtual.(%u)\n", lcore_id);
#endif
    }
    return 0;
}
//
int main(int argc, char* argv[]){
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

    __ingress.ring_ = rte_ring_create("ingress_ring", 4096, rte_lcore_to_socket_id(COREID_INGRESS), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __egress.ring_  = rte_ring_create("egress_ring",  4096, rte_lcore_to_socket_id(COREID_EGRESS), RING_F_SP_ENQ | RING_F_SC_DEQ);
#ifdef USE_VIRTUAL_CYCLE
    __internal.ring_= rte_ring_create("internal_ring",  4096, rte_lcore_to_socket_id(COREID_INTERNAL), RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (!__ingress.ring_ || !__egress.ring_ || !__internal.ring_){
#else
    if (!__ingress.ring_ || !__egress.ring_){
#endif
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
