#include "ut_counter_inc.hpp"

using namespace MIXIPGW;
static struct rte_ring*     __ring_rx_in_ = NULL;
static struct rte_ring*     __ring_err_ind_ = NULL;
static struct rte_ring*     __ring_tx_out_ = NULL;
static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CorePgwEgressTunnelWorker* __target = NULL;
static bool __halt_ = false;

int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1)};

#include "ut_misc_inline.cc"
#include "pkts/pkts.cc"

#define __YELLOW_TEST__ (10)
#define __PKTSIZE__     (POLICER_CIR_DEFAULT*__YELLOW_TEST__)

static const uint8_t   SRCMAC  = 0xff;
static const uint8_t   DSTMAC  = 0x7f;
static const uint32_t  SRCADDR = 0x0a0b0b0a;
static const uint32_t  DSTADDR = 0x0a0b0b0a;
static const uint16_t  SRCPORT = 2152;
static const uint16_t  DSTPORT = 2152;
static const uint32_t  TEID    = 0x99887766;


TEST(Policer, Init){
    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    __target = new MIXIPGW::CorePgwEgressTunnelWorker(0,uri.c_str(),13001,1,0x11111111,0x22222222,0x02,0x03);
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

static int alloc_cycle(Buf *input, int c){
    uint64_t        count = 0;
    //
    for(int n = 0;n < c;n++){
        auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
        EXPECT_EQ(pkt!=NULL,true);
        if (!pkt){ return(-1); };

        set_packet_temp(pkt,PTEMP_TCP_IPV4);
        set_packet_udpip(pkt,SRCMAC, DSTMAC, htonl(SRCADDR), htonl(DSTADDR), SRCPORT, DSTPORT, __PKTSIZE__);
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

    // regist tunnel data to database
    EXPECT_EQ(truncate_tunnel(), 0);
    EXPECT_EQ(insert_tunnel(htonl(TEID), DSTADDR&0xFFFFFF00), 0);
    // wait for binlog events
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
    auto ret = alloc_cycle(&input[0],(__YELLOW_TEST__));
    if (ret == 0){
        // exists tunnel id  -> gtpu -> decapped
        auto current  = rte_pktmbuf_mtod_offset(input[__YELLOW_TEST__-1], char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[__YELLOW_TEST__-1]);
        auto udat = (mbuf_userdat_ptr)&(input[__YELLOW_TEST__-1]->udata64);
        // need color yellow
        EXPECT_EQ(udat->color, POLICER_COLOR_YELLOW);
        // adoption value of mode from database
        EXPECT_EQ(udat->mode, POLICER_MODE_3G);
        //
        // decap size <= x - (gtpu + ip + udp)
        EXPECT_EQ(current_len, (__PKTSIZE__+36));
        //
        for(int n = 0;n < (__YELLOW_TEST__-1);n++){
            rte_pktmbuf_free(input[n]);
        }
    }else{
        return;
    }
    ret = alloc_cycle(&input[0], __YELLOW_TEST__-1);
    if (ret == 0){
        // exists tunnel id , gtpu -> decapped
        auto current  = rte_pktmbuf_mtod_offset(input[__YELLOW_TEST__-2], char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[__YELLOW_TEST__-2]);
        auto udat = (mbuf_userdat_ptr)&(input[__YELLOW_TEST__-2]->udata64);
        // color red
        EXPECT_EQ(udat->color, POLICER_COLOR_RED);
        // adoption value of mode from database
        EXPECT_EQ(udat->mode, POLICER_MODE_3G);
        //
        for(int n = 0;n < __YELLOW_TEST__;n++){
            rte_pktmbuf_free(input[n]);
        }
    }else{
        return;
    }
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
