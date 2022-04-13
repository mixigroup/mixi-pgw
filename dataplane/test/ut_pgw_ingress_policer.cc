#include "ut_counter_inc.hpp"

using namespace MIXIPGW;
static struct rte_ring*     __ring_rx_in_ = NULL;
static struct rte_ring*     __ring_err_ind_ = NULL;
static struct rte_ring*     __ring_tx_out_ = NULL;
static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CorePgwIngressTunnelWorker* __target = NULL;
static bool __halt_ = false;

int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1)};

#include "ut_misc_inline.cc"
#include "pkts/pkts.cc"

#define __YELLOW_TEST__ (10)
#define __PKTSIZE__     (POLICER_CIR_DEFAULT*__YELLOW_TEST__)

static const uint8_t   SRCMAC  = 0xff;
static const uint8_t   DSTMAC  = 0x7f;
static const uint32_t  SRCADDR = 0xdeadbeaf;
static const uint32_t  DSTADDR = 0xdeadc0de;
static const uint16_t  SRCPORT = 2152;
static const uint16_t  DSTPORT = 2152;
static const uint32_t  TEID    = 0x99887766;


TEST(Policer, Init){
    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    __target = new MIXIPGW::CorePgwIngressTunnelWorker(0,uri.c_str(),13001,1,0x11111111,0x22222222,0x02,0x03);
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM,   __ring_rx_in_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::EXTEND, __ring_err_ind_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_tx_out_), 0);

        __target->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target->SetN(MIXIPGW::KEY::BURST_TO, 1);
        __target->SetN(MIXIPGW::KEY::POLICER_CIR, POLICER_CIR_DEFAULT);
        __target->SetN(MIXIPGW::KEY::POLICER_CBS, POLICER_CBS_DEFAULT);
        __target->SetN(MIXIPGW::KEY::POLICER_EIR, POLICER_EIR_DEFAULT);
        __target->SetN(MIXIPGW::KEY::POLICER_EBS, POLICER_EBS_DEFAULT);
        __target->BeforeCycle(NULL);
    }
}
enum __PTYPE{
    PTYPE_GTPU,
    PTYPE_DNS_Q00,
    PTYPE_DNS_Q01,
    PTYPE_MAX
};

static int alloc_cycle(Buf *input, int c, int ptype = PTYPE_GTPU){
    uint64_t        count = 0;
    //
    for(int n = 0;n < c;n++){
        auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
        EXPECT_EQ(pkt!=NULL,true);
        if (!pkt){ return(-1); };
        if (ptype == PTYPE_GTPU) {
            set_packet_gtpu(pkt, SRCMAC, DSTMAC, htonl(SRCADDR), htonl(DSTADDR), SRCPORT, DSTPORT, htonl(TEID),
                            __PKTSIZE__);
        }else if (ptype == PTYPE_DNS_Q00){
            #include "pkts/dns.cc"
            set_packet_udpip(pkt, SRCMAC, DSTMAC, htonl(SRCADDR), htonl(DSTADDR), SRCPORT, DSTPORT, sizeof(__dns_query_8888));
            auto p = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
            memcpy(p, __dns_query_8888, sizeof(__dns_query_8888));
        }
        //
        auto ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
        EXPECT_EQ(ret,0);
        EXPECT_EQ(__target->Cycle(NULL, &count), 0);
        //
        auto nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) &input[n], 32, NULL);
        EXPECT_EQ(nburst,1);
        if (nburst != 1){
            return(-1);
        }
        auto udat = (mbuf_userdat_ptr)&(input[n]->udata64);

        printf("alloc_cycle(%u/%u/[%s])\n", n,c,
            udat->color == POLICER_COLOR_RED?"red":
            udat->color == POLICER_COLOR_YELLOW?"yel":"grn");
    }
    return(0);
}

TEST(Policer, Test){
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];
    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }

    EXPECT_EQ(truncate_tunnel(), 0);
    EXPECT_EQ(insert_tunnel(htonl(TEID), htonl(SRCADDR)), 0);

    pthread_t th;
    pthread_create(&th, NULL, [](void* arg)->void*{
        uint64_t count = 0;
        for(;!__halt_;){
            __target->VirtualCycle(NULL, &count);
        }
        return((void*)NULL);
    }, NULL);
    sleep(1);
    __halt_ = true;
    //
    auto ret = alloc_cycle(&input[0],(__YELLOW_TEST__+1));
    if (ret == 0){
        auto current  = rte_pktmbuf_mtod_offset(input[__YELLOW_TEST__], char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[__YELLOW_TEST__]);
        auto udat = (mbuf_userdat_ptr)&(input[__YELLOW_TEST__]->udata64);
        EXPECT_EQ(udat->color, POLICER_COLOR_YELLOW);
        EXPECT_EQ(udat->mode, POLICER_MODE_3G);
        //
        auto ll = (sizeof(struct gtpu_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));
        EXPECT_EQ(current_len, (__PKTSIZE__ - ll));
        //
        for(int n = 0;n < (__YELLOW_TEST__+1);n++){
            rte_pktmbuf_free(input[n]);
        }
    }else{
        return;
    }
    ret = alloc_cycle(&input[0], __YELLOW_TEST__);
    if (ret == 0){
        auto current  = rte_pktmbuf_mtod_offset(input[__YELLOW_TEST__-1], char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[__YELLOW_TEST__-1]);
        auto udat = (mbuf_userdat_ptr)&(input[__YELLOW_TEST__-1]->udata64);
        EXPECT_EQ(udat->color, POLICER_COLOR_RED);
        EXPECT_EQ(udat->mode, POLICER_MODE_3G);
        //
        for(int n = 0;n < __YELLOW_TEST__;n++){
            rte_pktmbuf_free(input[n]);
        }
    }else{
        return;
    }
    // replacement size
    __target->SetN(MIXIPGW::KEY::POLICER_EIR, POLICER_EIR_DEFAULT*6);
    {   Mysql   mmmm;
        mmmm.Query("UPDATE tunnel SET updated_at = NOW();");
    }

    __halt_ = false;
    pthread_create(&th, NULL, [](void* arg)->void*{
        uint64_t count = 0;
        for(;!__halt_;){
            __target->VirtualCycle(NULL, &count);
        }
        return((void*)NULL);
    }, NULL);
    sleep(1);
    __halt_ = true;
    //
    sleep(1);
    ret = alloc_cycle(&input[0], 1);
    if (ret == 0){
        auto udat = (mbuf_userdat_ptr)&(input[0]->udata64);
        printf("policer color. %u - %s\n", 0, udat->color==POLICER_COLOR_GREEN?"green": "?");
        EXPECT_EQ(udat->color, POLICER_COLOR_GREEN);
        //
        rte_pktmbuf_free(input[0]);
    }else{
        return;
    }
    alloc_cycle(&input[0], __YELLOW_TEST__);
}

int main(int argc, char* argv[]){
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    if ((__pktmbuf_pool_ = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }
    __ring_rx_in_   = rte_ring_create("rx_in", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_err_ind_ = rte_ring_create("err_ind", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_tx_out_  = rte_ring_create("tx_out", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    //
    if (!__ring_rx_in_|| !__ring_err_ind_ || !__ring_tx_out_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
