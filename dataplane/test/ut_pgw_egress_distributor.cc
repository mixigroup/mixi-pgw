#include "ut_counter_inc.hpp"

using namespace MIXIPGW;

static struct rte_ring*     __ring_rx_in_ = NULL;
static struct rte_ring*     __ring_tap_rx_in_ = NULL;
static struct rte_ring*     __ring_tap_tx_out_ = NULL;
static struct rte_ring*     __ring_tx_out_ = NULL;
static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CorePgwEgressDistributorWorker* __target = NULL;
static bool __halt_ = false;

int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1)};

#include "ut_misc_inline.cc"

TEST(DistributorPgwEgress, Init){
    std::string uri = "mysql://root:develop@127.0.0.1:3306";

    EXPECT_EQ(truncate_tunnel(), 0);

    __target = new MIXIPGW::CorePgwEgressDistributorWorker(0,uri.c_str(),55332,1,0x11111111,0x22222222,0x02,0x03);
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM,   __ring_rx_in_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::EXTEND,     __ring_tap_tx_out_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_tx_out_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM_00, __ring_tap_rx_in_), 0);

        __target->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target->SetN(MIXIPGW::KEY::BURST_TO, 1);
        __target->BeforeCycle(NULL);
    }
}

TEST(DistributorPgwEgress, TapTx){
    const uint8_t   SRCMAC  = 0xff;
    const uint8_t   DSTMAC  = 0x7f;
    const uint32_t  SRCADDR = 0xdeadbeaf;
    const uint32_t  DSTADDR = 0xdeadc0de;
    const uint16_t  SRCPORT = 50999;
    const uint16_t  DSTPORT = 50999;
    const uint16_t  PKTSIZE = 923;
    const uint32_t  TEID    = 0x99887766;
    uint64_t        count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    //
    set_packet_gtpu(pkt, SRCMAC, DSTMAC, htonl(SRCADDR), htonl(DSTADDR), SRCPORT, DSTPORT, 0, PKTSIZE);
    //
    auto ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    auto nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    // not found ipv4 ->  error notify to tx
    auto errind_len = (sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr) + sizeof(gtpu_hdr));
    auto eind = rte_pktmbuf_mtod_offset(input[0], gtpu_err_indication_ptr, errind_len);
    auto gtph = rte_pktmbuf_mtod_offset(input[0], struct gtpu_hdr*, errind_len - sizeof(gtpu_hdr));
    //
    EXPECT_EQ(eind->teid.type, GTPU_TEIDI_TYPE);
    EXPECT_EQ(eind->teid.val, 0);
    EXPECT_EQ(eind->peer.type, GTPU_PEER_ADDRESS);
    EXPECT_EQ(eind->peer.length, htons(sizeof(eind->peer.val)));
    EXPECT_EQ(eind->peer.val, htonl(DSTADDR));
    EXPECT_EQ(gtph->type, GTPU_ERROR_INDICATION);
    EXPECT_EQ(gtph->length, htons(sizeof(gtpu_err_indication_t)));

    EXPECT_EQ(truncate_tunnel(), 0);

    // register target tunnel
    EXPECT_EQ(insert_tunnel(htonl(TEID), htonl(DSTADDR)&0xffffff00), 0);
    // wait for binlog
    pthread_t th;
    pthread_create(&th, NULL, [](void* arg)->void*{
        uint64_t count = 0;
        for(;!__halt_;){
            __target->VirtualCycle(NULL, &count);
        }
        return((void*)NULL);
    }, NULL);
    sleep(2);
    __halt_ = true;
//  pthread_join(th, NULL);
    //
    set_packet_gtpu(pkt, SRCMAC, DSTMAC, htonl(SRCADDR), htonl(DSTADDR), SRCPORT, DSTPORT, 0, PKTSIZE);
    //
    ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);
    //
    nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    // exists ipv4 -> setup teid in gtpu@teid
    gtph = rte_pktmbuf_mtod_offset(input[0], struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));
    EXPECT_EQ(gtph->tid, htonl(TEID));
}

TEST(DistributorPgwEgress, UnInit){
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

    __ring_rx_in_       = rte_ring_create("rx_in", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_tap_rx_in_   = rte_ring_create("tap_rx_in", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_tap_tx_out_  = rte_ring_create("tap_tx_out", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_tx_out_      = rte_ring_create("tx_out", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    //
    if (!__ring_rx_in_|| !__ring_tap_rx_in_ || !__ring_tap_tx_out_ || !__ring_tx_out_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
