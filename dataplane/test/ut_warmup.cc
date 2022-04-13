#include "ut_counter_inc.hpp"
#include "mixi_pgw_data_plane_def.hpp"

using namespace MIXIPGW;
static struct rte_ring*     __ring_taprx_test_mode_0_ = NULL;
static struct rte_ring*     __ring_taprx_test_mode_1_ = NULL;
static struct rte_ring*     __ring_taprx_test_mode_2_ = NULL;
static struct rte_ring*     __ring_taprx_test_mode_3_ = NULL;

static struct rte_ring*     __ring_rxd_test_mode_0_ = NULL;
static struct rte_ring*     __ring_rxd_test_mode_1_ = NULL;
static struct rte_ring*     __ring_rxd_test_mode_2_ = NULL;

static struct rte_ring*     __ring_pgw_test_mode_0_ = NULL;
static struct rte_ring*     __ring_pgw_test_mode_1_ = NULL;
static struct rte_ring*     __ring_pgw_test_mode_2_ = NULL;
static struct rte_ring*     __ring_pgw_test_mode_3_ = NULL;
static struct rte_ring*     __ring_pgw_test_mode_4_ = NULL;
static struct rte_ring*     __ring_pgw_test_mode_5_ = NULL;

static struct rte_ring*     __ring_rx_dist_test_mode_0_ = NULL;

static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CoreTapRx* __target_tap = NULL;
static MIXIPGW::CorePgwIngressDistributorWorker* __target_pgw_in = NULL;
static MIXIPGW::CorePgwEgressDistributorWorker* __target_pgw_e = NULL;
static MIXIPGW::CoreRxDistributor* __target_rx_dist = NULL;

namespace MIXIPGW{
  int PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1)};
};

#include "ut_misc_inline.cc"

class UtWarmupMemPool:public CoreMempool{
public:
    UtWarmupMemPool(){}
    //
    virtual struct rte_mempool* Ref(COREID coreid){
        return(pl_);
    }
public:
    struct rte_mempool* pl_;
};

static bool __halt_ = false;
static UtWarmupMemPool  __mmppll;

std::string __uri = "mysql://root:develop@127.0.0.1:3306";

/*
static unsigned char __CREATE_SESSION_PKTS[] = {
        0x48,0x21,0x00,0xeb,0x00,0x15,0x41,0x5a,0x29,0x34,0xe6,0x00,0x02,0x00,0x02,0x00,
        0x10,0x00,0x57,0x00,0x09,0x01,0x87,0x0a,0x0a,0x0a,0x01,0x6e,0x2c,0xb5,0x12,0x4f,
        0x00,0x12,0x00,0x02,0x40,0x24,0x01,0xf1,0x00,0x00,0x0a,0x0a,0x01,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x01,0x7f,0x00,0x01,0x00,0x00,0x48,0x00,0x08,0x00,0x00,0x41,
        0x89,0x37,0x00,0x41,0x89,0x37,0x4e,0x00,0x2c,0x00,0x00,0x00,0x03,0x10,0x20,0x01,
        0x48,0x60,0x48,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x00,0x03,
        0x10,0x20,0x01,0x48,0x60,0x48,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,
        0x44,0x00,0x10,0x02,0x05,0x96,0x5d,0x00,0x50,0x00,0x02,0x00,0x02,0x00,0x10,0x00,
        0x49,0x00,0x01,0x00,0x06,0x57,0x00,0x09,0x02,0x85,0x0a,0x0a,0x0a,0x01,0x6e,0x2c,
        0xb5,0xa0,0x4f,0x00,0x12,0x00,0x02,0x40,0x24,0x01,0xf1,0x00,0x00,0x0a,0x0a,0x01,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x50,0x00,0x16,0x00,0x04,0x09,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x5e,0x00,0x04,0x00,0x0a,0x0a,0x0a,0x01,0x03,0x00,0x01,0x00,0x01,0xff,
        0x00,0x1c,0x00,0xc7,0x69,
        0x00,0x00,
        0xaf,0xbe,0xad,0xde,
        0x6e,0x2c,0xb5,0xa0,
        0x40,0xbd,0x08,0x92,
        0x00,0x0a,0x0a,0x01,
        0x01,0x0a,0x0a,0x0a,
        0x00,0x00,0x00,0x00,
};
 */
static unsigned char __CREATE_SESSION_PKTS[] = {
        0x48, 0x21, 0x00, 0xb9, 0x09, 0x82, 0x24, 0xfc, // 8
        0x24, 0x00, 0x15, 0x00, 0x02, 0x00, 0x02, 0x00, // 16
        0x10, 0x00, 0x57, 0x00, 0x09, 0x01, 0x87, 0x8a, // 24
        0x0a, 0x0a, 0xa3, 0x6e, 0x2c, 0xb5, 0x12, 0x4f, // 32
        0x00, 0x05, 0x00, 0x01, 0x0a, 0x0a, 0x0a, 0xa3, // 40
        0x7f, 0x00, 0x01, 0x00, 0x00, 0x48, 0x00, 0x08, // 48
        0x00, 0x00, 0x41, 0x89, 0x37, 0x00, 0x41, 0x89, // 56
        0x37, 0x4e, 0x00, 0x14, 0x00, 0x80, 0x00, 0x0d, // 64
        0x04, 0x08, 0x08, 0x08, 0x08, 0x00, 0x0d, 0x04, // 72
        0x08, 0x08, 0x04, 0x04, 0x00, 0x10, 0x02, 0x05, // 80
        0xdc, 0x5d, 0x00, 0x43, 0x00, 0x02, 0x00, 0x02, // 88
        0x00, 0x10, 0x00, 0x49, 0x00, 0x01, 0x00, 0x08, // 96
        0x57, 0x00, 0x09, 0x02, 0x85, 0x8a, 0x0a, 0x0a, // 104
        0xa3, 0x6e, 0x2c, 0xb5, 0xa0, 0x4f, 0x00, 0x05, // 112
        0x00, 0x01, 0x0a, 0x0a, 0x0a, 0xa3, 0x50, 0x00, // 120
        0x16, 0x00, 0x04, 0x09, 0x00, 0x00, 0x00, 0x00, // 128
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 136
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 144
        0x5e, 0x00, 0x04, 0x00, 0x8a, 0x0a, 0x0a, 0xa3, // 152
        0x03, 0x00, 0x01, 0x00, 0x01, 0xff, 0x00, 0x1c,
        0x00, 0xc7, 0x69,

        0x00, 0x00,
        0xaf, 0xbe, 0xad,0xde,

        0x6e, 0x2c, 0xb5, 0xa0,// SGW_GTPU_IPV4,    /*!< sgw gtpu ipv4 */
        0x1f, 0x62, 0x62, 0x1b,// SGW_GTPU_TEID,        /*!< sgw gtpu teid */
        0x00, 0x00, 0x03, 0x47,// UE_IPV4,              /*!< ue ipv4 */
        0x47, 0x03, 0x00, 0x50,// UE_TEID,              /*!< ue teid *//
	    0x01, 0x01, 0x01, 0x01
};

static unsigned char __SGW_GTPU_IPV4[]  = {0x6e,0x2c,0xb5,0xa0};
static unsigned char __SGW_GTPU_TEID[]  = {0x1f,0x62,0x62,0x1b};
static unsigned char __UE_IPV4[]        = {0x00,0x00,0x03,0x47};
static unsigned char __UE_TEID[]        = {0x47,0x03,0x00,0x50};

TEST(Warmup, InitXX){
    uint64_t count = 0;
    for(auto n = 0;n < 10;n++){
        auto mpl = new UtWarmupMemPool();
        auto tmp_trgt = new MIXIPGW::CoreTapRx("tap_00", 0, mpl);
        EXPECT_EQ(tmp_trgt->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_taprx_test_mode_0_), 0);
        EXPECT_EQ(tmp_trgt->SetRingAddr(MIXIPGW::ORDER::TO_00,    __ring_taprx_test_mode_1_), 0);
        EXPECT_EQ(tmp_trgt->SetRingAddr(MIXIPGW::ORDER::TO_01,    __ring_taprx_test_mode_3_), 0);
        EXPECT_EQ(tmp_trgt->SetRingAddr(MIXIPGW::ORDER::FROM,     __ring_taprx_test_mode_2_), 0);
        tmp_trgt->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        tmp_trgt->SetN(MIXIPGW::KEY::BURST_TO, 1);
        tmp_trgt->SetP(KEY::OPT, mpl);
        EXPECT_EQ(tmp_trgt->Cycle(NULL, &count), 0);
        //
        delete tmp_trgt;
        delete mpl;
    }
}
TEST(Warmup, InitXX00){
    uint64_t count = 0;
    for(auto n = 0;n < 10;n++){
        auto trgt = new MIXIPGW::CoreRxDistributor(0, 0, 0);
        EXPECT_EQ(trgt!=NULL, true);
        if (trgt){
            EXPECT_EQ(trgt->GetType(), TYPE::RX);
            EXPECT_EQ(trgt->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_rxd_test_mode_0_), 0);
            EXPECT_EQ(trgt->SetRingAddr(MIXIPGW::ORDER::EXTEND,   __ring_rxd_test_mode_1_), 0);
            EXPECT_EQ(trgt->SetRingAddr(MIXIPGW::ORDER::FROM,     __ring_rxd_test_mode_2_), 0);
            trgt->SetN(MIXIPGW::KEY::BURST_FROM, 1);
            trgt->SetN(MIXIPGW::KEY::BURST_TO, 1);
            EXPECT_EQ(trgt->Cycle(NULL, &count), 0);
            delete trgt;
        }
    }
}

TEST(Warmup, Init){
    uint64_t count = 0;
    __mmppll.pl_ = __pktmbuf_pool_;
    __target_tap = new MIXIPGW::CoreTapRx("tap_00", 0, &__mmppll);
    EXPECT_EQ(__target_tap!=NULL,true);
    if (__target_tap){
        EXPECT_EQ(__target_tap->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_taprx_test_mode_0_), 0);
        EXPECT_EQ(__target_tap->SetRingAddr(MIXIPGW::ORDER::TO_00,    __ring_taprx_test_mode_1_), 0);
        EXPECT_EQ(__target_tap->SetRingAddr(MIXIPGW::ORDER::TO_01,    __ring_taprx_test_mode_3_), 0);
        EXPECT_EQ(__target_tap->SetRingAddr(MIXIPGW::ORDER::FROM,     __ring_taprx_test_mode_2_), 0);
        __target_tap->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target_tap->SetN(MIXIPGW::KEY::BURST_TO, 1);
        __target_tap->SetP(KEY::OPT, &__mmppll);
    }
    EXPECT_EQ(__target_tap->Cycle(NULL, &count), 0);
    //
    __target_pgw_in = new MIXIPGW::CorePgwIngressDistributorWorker(1,__uri.c_str(),1801,0,0xac180401,0xac180401,0,0);
    EXPECT_EQ(__target_pgw_in!=NULL,true);
    if (__target_pgw_in){
        EXPECT_EQ(__target_pgw_in->SetRingAddr(MIXIPGW::ORDER::FROM,     __ring_pgw_test_mode_0_), 0);
        EXPECT_EQ(__target_pgw_in->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_pgw_test_mode_1_), 0);
        EXPECT_EQ(__target_pgw_in->SetRingAddr(MIXIPGW::ORDER::EXTEND,   __ring_pgw_test_mode_2_), 0);
        EXPECT_EQ(__target_pgw_in->SetRingAddr(MIXIPGW::ORDER::FROM_00,  __ring_taprx_test_mode_1_), 0);
        __target_pgw_in->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target_pgw_in->SetN(MIXIPGW::KEY::BURST_TO, 1);
    }
    __target_pgw_in->BeforeCycle(NULL);
    EXPECT_EQ(__target_pgw_in->Cycle(NULL, &count), 0);

    __target_pgw_e = new MIXIPGW::CorePgwEgressDistributorWorker(2,__uri.c_str(),1805,0,0xac180401,0xac180401,0,0);
    EXPECT_EQ(__target_pgw_e!=NULL,true);
    if (__target_pgw_e){
        EXPECT_EQ(__target_pgw_e->SetRingAddr(MIXIPGW::ORDER::FROM,     __ring_pgw_test_mode_3_), 0);
        EXPECT_EQ(__target_pgw_e->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_pgw_test_mode_4_), 0);
        EXPECT_EQ(__target_pgw_e->SetRingAddr(MIXIPGW::ORDER::EXTEND,   __ring_pgw_test_mode_5_), 0);
        EXPECT_EQ(__target_pgw_e->SetRingAddr(MIXIPGW::ORDER::FROM_00,  __ring_taprx_test_mode_3_), 0);
        __target_pgw_e->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target_pgw_e->SetN(MIXIPGW::KEY::BURST_TO, 1);
    }
    __target_pgw_e->BeforeCycle(NULL);
    EXPECT_EQ(__target_pgw_e->Cycle(NULL, &count), 0);

    __target_rx_dist = new MIXIPGW::CoreRxDistributor(0,0,0);
    EXPECT_EQ(__target_rx_dist!=NULL,true);
    if (__target_rx_dist){
        EXPECT_EQ(__target_rx_dist->SetRingAddr(MIXIPGW::ORDER::FROM,     __ring_rx_dist_test_mode_0_), 0);
        EXPECT_EQ(__target_rx_dist->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_pgw_test_mode_0_), 0);
        EXPECT_EQ(__target_rx_dist->SetRingAddr(MIXIPGW::ORDER::TO,       __ring_pgw_test_mode_3_), 0);
        __target_rx_dist->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target_rx_dist->SetN(MIXIPGW::KEY::BURST_TO, 1);
    }
    EXPECT_EQ(__target_rx_dist->Cycle(NULL, &count), 0);
}
TEST(Warmup, CreateSession){
    uint64_t count = 0;

    auto mbuf = rte_pktmbuf_alloc(__pktmbuf_pool_);
    if (unlikely(!__pktmbuf_pool_)) {
        rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(alloc)(%p)\n", __pktmbuf_pool_);
    }
    auto eh = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
    auto ip = (struct ipv4_hdr*)(eh+1);
    auto udp= (struct udp_hdr*)(ip+1);
    auto gtp= (struct gtpu_hdr*)(udp+1);
    auto payload = (char*)(gtp + 1);
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    memcpy(gtp, __CREATE_SESSION_PKTS, sizeof(__CREATE_SESSION_PKTS));

    // ip version 4
    ip->version_ihl     = (IP_VERSION | 0x05);
    ip->type_of_service = 0;
    ip->packet_id       = 0;
    ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ip->time_to_live    = IP_DEFTTL;
    ip->next_proto_id   = IPPROTO_UDP;
    ip->hdr_checksum    = 0;
    ip->src_addr        = 0xdeadc0de;
    ip->dst_addr        = 0xdeadc0de;
    ip->total_length    = rte_cpu_to_be_16(sizeof(__CREATE_SESSION_PKTS)+20+8);
    ip->hdr_checksum    = 0;
    // Control Packet
    udp->dst_port       = htons(2123);
    gtp->tid            = (ip->dst_addr);
    gtp->length         = rte_cpu_to_be_16(sizeof(__CREATE_SESSION_PKTS) - 8);
    //
    mbuf->pkt_len = mbuf->data_len = (sizeof(__CREATE_SESSION_PKTS)+20+8+14);
    mbuf->nb_segs = 1;
    mbuf->next = NULL;
    mbuf->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
    //
    auto ret = rte_ring_sp_enqueue(__ring_taprx_test_mode_2_, mbuf);
    EXPECT_EQ(ret, 0);

    if (unlikely(ret == -ENOBUFS)) {
        rte_exit(EXIT_FAILURE, "Error no buffer.\n");
        return;
    }else if (ret < 0){
        rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
    }
    __mmppll.pl_ = __pktmbuf_pool_;

    int i;
    for(i=0;i<14000001;i++){
        EXPECT_EQ(__target_tap->Cycle(NULL, &count), 0);
        EXPECT_EQ(__target_pgw_in->Cycle(NULL, &count), 0);
        EXPECT_EQ(__target_pgw_e->Cycle(NULL, &count), 0);
    }


    //
    // send user packet ....
    //

    auto mbuf_in = rte_pktmbuf_alloc(__pktmbuf_pool_);
    auto mbuf_e = rte_pktmbuf_alloc(__pktmbuf_pool_);
    
    #define         PKTSIZE     (96)
    #define         TEID        (0x50000347)
    #define         DSTADDR     (0x00000347)
    #define         SRCADDR     (0x00000347)

    set_packet_gtpu(mbuf_in, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, TEID, PKTSIZE);
    set_packet_gtpu(mbuf_e, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 50999, 50999, TEID, PKTSIZE);

    ret = rte_ring_sp_enqueue(__ring_rx_dist_test_mode_0_, mbuf_in);
    EXPECT_EQ(ret, 0);

    if (unlikely(ret == -ENOBUFS)) {
        rte_exit(EXIT_FAILURE, "Error no buffer.\n");
        return;
    }else if (ret < 0){
        rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
    }

    ret = rte_ring_sp_enqueue(__ring_rx_dist_test_mode_0_, mbuf_e);
    EXPECT_EQ(ret, 0);

    if (unlikely(ret == -ENOBUFS)) {
        rte_exit(EXIT_FAILURE, "Error no buffer.\n");
        return;
    }else if (ret < 0){
        rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
    } 
    
    EXPECT_EQ(__target_rx_dist->Cycle(NULL, &count), 0);
    EXPECT_EQ(__target_rx_dist->Cycle(NULL, &count), 0);
    EXPECT_EQ(__target_pgw_in->Cycle(NULL, &count), 0);
    EXPECT_EQ(__target_pgw_e->Cycle(NULL, &count), 0);
}

TEST(Warmup, Check){
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];
    uint32_t    ueteid,ueipv4,sgw_gtpu_teid,sgw_gtpu_ipv4;
    lookup_t    foundi;
    //
    memcpy(&ueteid, __UE_TEID, sizeof(ueteid));
    memcpy(&ueipv4, __UE_IPV4, sizeof(ueipv4));
    memcpy(&sgw_gtpu_teid, __SGW_GTPU_TEID, sizeof(sgw_gtpu_teid));
    memcpy(&sgw_gtpu_ipv4, __SGW_GTPU_IPV4, sizeof(sgw_gtpu_ipv4));
    bzero(&foundi, sizeof(foundi));

    EXPECT_EQ(__target_pgw_in->Find(ueteid, &foundi), 0);
    EXPECT_EQ(foundi.ue_teid, ueteid);
    EXPECT_EQ(foundi.ue_ipv4, ueipv4);
    EXPECT_EQ(foundi.sgw_gtpu_teid, sgw_gtpu_teid);
    EXPECT_EQ(foundi.sgw_gtpu_ipv4, sgw_gtpu_ipv4);

    EXPECT_EQ(__target_pgw_e->Find(ueipv4, &foundi), 0);
    EXPECT_EQ(foundi.ue_teid, ueteid);
    EXPECT_EQ(foundi.ue_ipv4, ueipv4);
    EXPECT_EQ(foundi.sgw_gtpu_teid, sgw_gtpu_teid);
    EXPECT_EQ(foundi.sgw_gtpu_ipv4, sgw_gtpu_ipv4);
    auto nburst = rte_ring_sc_dequeue_burst(__ring_pgw_test_mode_1_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst, 1);
    nburst = rte_ring_sc_dequeue_burst(__ring_pgw_test_mode_4_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst, 1);
}

TEST(Warmup, UnInit){
    EXPECT_EQ(__target_tap!=NULL,true);
    if (__target_tap){
        delete __target_tap;
    }
    __target_tap = NULL;

    EXPECT_EQ(__target_pgw_in!=NULL,true);
    if (__target_pgw_in){
        delete __target_pgw_in;
    }
    __target_pgw_in = NULL;

    EXPECT_EQ(__target_pgw_e!=NULL,true);
    if (__target_pgw_e){
        delete __target_pgw_e;
    }
    __target_pgw_e = NULL;

    EXPECT_EQ(__target_rx_dist!=NULL,true);
    if (__target_rx_dist){
        delete __target_rx_dist;
    }
    __target_rx_dist = NULL;
}

int main(int argc, char* argv[]){
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    if ((__pktmbuf_pool_ = rte_pktmbuf_pool_create("mbuf_pool_warmup", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }
    // ng
    __ring_taprx_test_mode_0_ = rte_ring_create("taprx_test_mode_0", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_taprx_test_mode_1_ = rte_ring_create("taprx_test_mode_1", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_taprx_test_mode_2_ = rte_ring_create("taprx_test_mode_2", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_taprx_test_mode_3_ = rte_ring_create("taprx_test_mode_3", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

    // ok
    __ring_rxd_test_mode_0_   = rte_ring_create("rxd_test_mode_0", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_rxd_test_mode_1_   = rte_ring_create("rxd_test_mode_1", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_rxd_test_mode_2_   = rte_ring_create("rxd_test_mode_2", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

    // pgw ingress
    __ring_pgw_test_mode_0_   = rte_ring_create("pgw_test_mode_0", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_pgw_test_mode_1_   = rte_ring_create("pgw_test_mode_1", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_pgw_test_mode_2_   = rte_ring_create("pgw_test_mode_2", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_pgw_test_mode_3_   = rte_ring_create("pgw_test_mode_3", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_pgw_test_mode_4_   = rte_ring_create("pgw_test_mode_4", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_pgw_test_mode_5_   = rte_ring_create("pgw_test_mode_5", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    //
    __ring_rx_dist_test_mode_0_ = rte_ring_create("rx_dist_test_mode_0", 32, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    //
    if (!__ring_taprx_test_mode_0_ || !__ring_taprx_test_mode_1_ || !__ring_taprx_test_mode_2_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
