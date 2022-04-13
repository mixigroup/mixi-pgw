#include "ut_counter_inc.hpp"

using namespace MIXIPGW;
int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1)};

static struct rte_ring*     __ring_debug_in_ = NULL;
static struct rte_ring*     __ring_egress_ = NULL;
static struct rte_ring*     __ring_ingress_ = NULL;
static struct rte_ring*     __ring_tap_tx_ = NULL;
static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CoreRxDistributor* __target = NULL;

#include "ut_misc_inline.cc"

TEST(DistributorRx, Init){
    __target = new MIXIPGW::CoreRxDistributor(1,2,0);
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM,   __ring_debug_in_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_ingress_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_egress_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::EXTEND, __ring_tap_tx_), 0);

        __target->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target->SetN(MIXIPGW::KEY::BURST_TO, 1);
    }
}
TEST(DistributorRx, TapTx){
    const uint8_t   SRCMAC  = 0xff;
    const uint8_t   DSTMAC  = 0x7f;
    const uint32_t  SRCADDR = 0xdeadbeaf;
    const uint32_t  DSTADDR = 0xdeadc0de;
    const uint16_t  SRCPORT = 6789;
    const uint16_t  DSTPORT = 8542;
    const uint16_t  PKTSIZE = 923;
    uint64_t        count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    //
    set_packet_udpip(pkt, SRCMAC, DSTMAC, SRCADDR, DSTADDR, SRCPORT, DSTPORT, PKTSIZE);
    auto compare = rte_pktmbuf_clone(pkt, __pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; }
    //
    auto ret = rte_ring_sp_enqueue(__ring_debug_in_, pkt);
    EXPECT_EQ(ret,0);

    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    auto nburst = rte_ring_sc_dequeue_burst(__ring_tap_tx_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], void*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, void*, 0);
        EXPECT_EQ(rte_pktmbuf_data_len(compare), rte_pktmbuf_data_len(input[0]));
        EXPECT_EQ(memcmp(prev, current, rte_pktmbuf_data_len(compare)), 0);
    }
}
TEST(DistributorRx, PgwIngress){
    const uint8_t   SRCMAC  = 0xfe;
    const uint8_t   DSTMAC  = 0x23;
    const uint32_t  SRCADDR = 0x6578abcd;
    const uint32_t  DSTADDR = 0x78651265;
    const uint16_t  SRCPORT = 9999;
    const uint16_t  DSTPORT = GTPU_PORT;
    const uint16_t  PKTSIZE = 234;
    const uint32_t  TEID    = 0xdeadbeaf;
    uint64_t        count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    //
    set_packet_gtpu(pkt, SRCMAC, DSTMAC, SRCADDR, DSTADDR, SRCPORT, DSTPORT, TEID, PKTSIZE);
    auto compare = rte_pktmbuf_clone(pkt, __pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; }
    //
    auto ret = rte_ring_sp_enqueue(__ring_debug_in_, pkt);
    EXPECT_EQ(ret,0);

    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    auto nburst = rte_ring_sc_dequeue_burst(__ring_ingress_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], void*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, void*, 0);
        EXPECT_EQ(rte_pktmbuf_data_len(compare), rte_pktmbuf_data_len(input[0]));
        EXPECT_EQ(memcmp(prev, current, rte_pktmbuf_data_len(compare)), 0);
    }
}

TEST(DistributorRx, PgwEgress){
    const uint8_t   SRCMAC  = 0xfd;
    const uint8_t   DSTMAC  = 0x12;
    const uint32_t  SRCADDR = 0x44556677;
    const uint32_t  DSTADDR = 0xababcdcd;
    const uint16_t  SRCPORT = 9999;
    const uint16_t  DSTPORT = PGWEGRESSPORT;
    const uint16_t  PKTSIZE = 456;
    const uint32_t  TEID    = 0x55442211;
    uint64_t        count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    //
    set_packet_gtpu(pkt, SRCMAC, DSTMAC, SRCADDR, DSTADDR, SRCPORT, DSTPORT, TEID, PKTSIZE);
    auto compare = rte_pktmbuf_clone(pkt, __pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; }
    //
    auto ret = rte_ring_sp_enqueue(__ring_debug_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    auto nburst = rte_ring_sc_dequeue_burst(__ring_egress_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], void*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, void*, 0);
        EXPECT_EQ(rte_pktmbuf_data_len(compare), rte_pktmbuf_data_len(input[0]));
        EXPECT_EQ(memcmp(prev, current, rte_pktmbuf_data_len(compare)), 0);
    }
}

TEST(DistributorRx, UnInit){
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        delete __target;
    }
    __target = NULL;
}


int main(int argc, char* argv[]){
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    if ((__pktmbuf_pool_ = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }

    __ring_debug_in_ = rte_ring_create("debug_in", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_egress_   = rte_ring_create("egress", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_ingress_  = rte_ring_create("ingress", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_tap_tx_   = rte_ring_create("taptx", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

    if (!__ring_debug_in_|| !__ring_egress_ || !__ring_ingress_ || !__ring_tap_tx_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
