#include "ut_counter_inc.hpp"
#include "../inc/app.hpp"

using namespace MIXIPGW;
static bool __halt_ = false;
static unsigned __ring_cnt_ = 0;

static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CoreTapRx* __target_tap = NULL;
static MIXIPGW::CorePgwIngressDistributorWorker* __target_pgw_in = NULL;

static Ring                                 __rings_;
static CoreRxDistributor*                   __rxdist_0 = NULL;
static CorePgwIngressDistributorWorker*     __pgw_in = NULL;
static CoreTx*                              __tx_0 = NULL;
//
static CoreRx*                              __rx_0 = NULL;
static CoreGretermWorker*                   __greterm = NULL;
static CoreTx*                              __tx_1 = NULL;
//
static CoreRx*                              __rx_1 = NULL;
static CoreEncapWorker*                     __encap = NULL;
static CoreTx*                              __tx_2 = NULL;
//
static CoreRxDistributor*                   __rxdist_1 = NULL;
static CorePgwEgressDistributorWorker*      __pgw_e = NULL;
static CoreTx*                              __tx_3 = NULL;

static App*                                 __app_ = NULL;
static Cores                                __cores_;
#define MAX_CYCLE_COUNTERS                  (32)
static uint64_t                             __cycle_counters_[MAX_CYCLE_COUNTERS];

#include "ut_misc_inline.cc"

TEST(PerfAdjust, InitApp){

    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

    // PGW - Ingress
    __rxdist_0  = new CoreRxDistributor(0, 0, 0);
    __pgw_in    = new CorePgwIngressDistributorWorker(0,uri.c_str(),12004,1,PGW_TUNNEL_IP,PGW_TUNNEL_IP,0,0);
    __tx_0      = new CoreTx(0, 0, 0);
    // tunnel - Ingress
    __rx_0      = new CoreRx(1, 0, 0);
    __greterm   = new CoreGretermWorker(0,0);
    __tx_1      = new CoreTx(0, 0, 0, 1);   // swap IP address -> down stream packet 
    // tunnel - Egress
    __rx_1      = new CoreRx(1, 0, 0);
    __encap     = new CoreEncapWorker(0,PGW_TUNNEL_IP);
    __tx_2      = new CoreTx(0, 0, 0);
    // PGW - Egress
    __rxdist_1  = new CoreRxDistributor(0, 0, 0);
    __pgw_e     = new CorePgwEgressDistributorWorker(0,uri.c_str(),12005,0,PGW_TUNNEL_IP,PGW_TUNNEL_IP,0,0);
    __tx_3      = new CoreTx(0, 0, 0);
    //
    __cores_[0] = __rxdist_0;
    __cores_[1] = __pgw_in;
    __cores_[2] = __tx_0;
    __cores_[3] = __rx_0;
    __cores_[4] = __greterm;
    __cores_[5] = __tx_1;
    __cores_[6] = __rx_1;
    __cores_[7] = __encap;
    __cores_[8] = __tx_2;
    __cores_[9] = __rxdist_1;
    __cores_[10]= __pgw_e;
    __cores_[11]= __tx_3;
    //
    EXPECT_EQ(__app_!=NULL,true);
    uint32_t ci = 0;
    for(auto it = __cores_.begin();it != __cores_.end();++it,ci++){
        __app_->cores_[ci] = (it->second);
    }
    __app_->Commit(NULL);
    __app_->dburi_ = uri;

}
TEST(PerfAdjust, Connect){
    char nm[128] = {0};
    for(auto n = 0;n < 24;n++){
        snprintf(nm, sizeof(nm)-1,"ring_%u", n);
        __rings_.push_back(rte_ring_create(nm, 512, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ));
    }
    // PGW-In
    __rxdist_0->SetRingAddr(ORDER::FROM, __rings_[0]);
    __rxdist_0->SetRingAddr(ORDER::TO, __rings_[1]);
    __rxdist_0->SetRingAddr(ORDER::TO, __rings_[20]);   // not used at Egress side
    __pgw_in->SetRingAddr(ORDER::FROM, __rings_[1]);
    __pgw_in->SetRingAddr(ORDER::TO, __rings_[2]);
    __tx_0->SetRingAddr(ORDER::FROM, __rings_[2]);
    __tx_0->SetRingAddr(ORDER::TO, __rings_[3]);
    // Tunnel-In
    __rx_0->SetRingAddr(ORDER::FROM, __rings_[3]);
    __rx_0->SetRingAddr(ORDER::TO, __rings_[4]);
    __greterm->SetRingAddr(ORDER::FROM, __rings_[4]);
    __greterm->SetRingAddr(ORDER::TO, __rings_[5]);
    __tx_1->SetRingAddr(ORDER::FROM, __rings_[5]);
    __tx_1->SetRingAddr(ORDER::TO, __rings_[6]);
    // Tunnel-Eg
    __rx_1->SetRingAddr(ORDER::FROM, __rings_[6]);
    __rx_1->SetRingAddr(ORDER::TO, __rings_[7]);
    __encap->SetRingAddr(ORDER::FROM, __rings_[7]);
    __encap->SetRingAddr(ORDER::TO, __rings_[8]);
    __tx_2->SetRingAddr(ORDER::FROM, __rings_[8]);
    __tx_2->SetRingAddr(ORDER::TO, __rings_[9]);
    // PGW-Eg
    __rxdist_1->SetRingAddr(ORDER::FROM, __rings_[9]);
    __rxdist_1->SetRingAddr(ORDER::TO, __rings_[21]);   // not used Ingress side
    __rxdist_1->SetRingAddr(ORDER::TO, __rings_[10]);
    __pgw_e->SetRingAddr(ORDER::FROM, __rings_[10]);
    __pgw_e->SetRingAddr(ORDER::TO, __rings_[11]);
    __tx_3->SetRingAddr(ORDER::FROM, __rings_[11]);
    __tx_3->SetRingAddr(ORDER::TO, __rings_[12]);
    //
    pthread_t th;
    pthread_create(&th, NULL, [](void* arg)->void*{
        uint64_t count = 0;
        for(auto it = __cores_.begin();it != __cores_.end();++it){
            (it->second)->BeforeCycle(__app_);
        }
        //
        for(;!__halt_;count++){
            for(auto it = __cores_.begin();it != __cores_.end();++it){
                (it->second)->VirtualCycle(__app_, &count);
            }
            usleep(10000);
        }
        return((void*)NULL);
    }, NULL);

}

#define         PKTSIZE     (234)
#define         TEID        (0xdeadbeaf)
#define         DSTADDR     (0x11223344)
#define         SRCADDR     (0x99887766)

static void EntryPacket(int num){
    for(auto n = 0;n < num;n++){
        auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
        EXPECT_EQ(pkt!=NULL,true);
        if (!pkt){ return; };
        //
        set_packet_gtpu(pkt, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);
        auto ret = rte_ring_sp_enqueue(__rings_[0], pkt);
        EXPECT_EQ(ret,0);
    }
}

static void ExitPacket(int num, int flush, int burst_rx_read,int burst_size_rx_enq,int burst_size_rx_deq,
                       int burst_size_tx_enq,int burst_size_tx_deq, int burst_size_tx_write){
    Buf         input[DEFAULT_MBUF_ARRAY_SIZE];
    uint64_t    count = 0;
    char        sql[1024] = {0};
    bzero(__cycle_counters_, sizeof(__cycle_counters_));

    {   Mysql   con;
        EXPECT_EQ(con.Query("DELETE FROM config"), 0);
        EXPECT_EQ(con.Query("INSERT INTO config(`key`,`val`) VALUES('loglevel','8')"), 0);
#define FMT_QUERY(s, c, k, f)  snprintf(s,sizeof(s)-1,"INSERT INTO config(`key`,`val`) VALUES('%s','%d')", k, f); EXPECT_EQ(c.Query(s), 0);
        FMT_QUERY(sql, con, "flush_counter", flush);
        FMT_QUERY(sql, con, "flush_encap", flush);
        FMT_QUERY(sql, con, "flush_greterm", flush);
        FMT_QUERY(sql, con, "flush_pgw_ie", flush);
        FMT_QUERY(sql, con, "flush_rx", flush);
        FMT_QUERY(sql, con, "flush_rx_dist", flush);
        FMT_QUERY(sql, con, "flush_tx", flush);
        FMT_QUERY(sql, con, "burst_size_rx_read", burst_rx_read);
        FMT_QUERY(sql, con, "burst_size_rx_enq", burst_size_rx_enq);
        FMT_QUERY(sql, con, "burst_size_rx_deq", burst_size_rx_deq);
        FMT_QUERY(sql, con, "burst_size_tx_enq", burst_size_tx_enq);
        FMT_QUERY(sql, con, "burst_size_tx_deq", burst_size_tx_deq);
        FMT_QUERY(sql, con, "burst_size_tx_write", burst_size_tx_write);
        //
        EXPECT_EQ(con.Query("DELETE FROM tunnel"), 0);
        EXPECT_EQ(insert_tunnel(htonl(TEID), htonl(SRCADDR)&0xffffff00), 0);
    }
    __app_->LoadConfig();
    __app_->SwapConfig();
    // wait for binlog events.
    sleep(2);
    uint32_t    recv_counter = 0;

    for(;;__cycle_counters_[(MAX_CYCLE_COUNTERS-1)]++){
        for(auto it = __cores_.begin();it != __cores_.end();++it){
            (it->second)->Cycle(__app_, &__cycle_counters_[it->first]);
            __cycle_counters_[it->first]++;
        }
        auto nburst = rte_ring_sc_dequeue_burst(__rings_[12], (void **) input, 32, NULL);
        if (nburst > 0){
            recv_counter += nburst;
            if (recv_counter >= num){
                break;
            }
        }
    }
    PGW_LOG(RTE_LOG_ERR, "finished [%10llu]\n", __cycle_counters_[(MAX_CYCLE_COUNTERS-1)]);
}

TEST(PerfAdjust, Packet100K_1){
    EntryPacket(1);
    ExitPacket(1, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet100K_10){
    EntryPacket(10);
    ExitPacket(10, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet100K_20){
    EntryPacket(20);
    ExitPacket(20, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet100K_40){
    EntryPacket(40);
    ExitPacket(40, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet100K_80){
    EntryPacket(80);
    ExitPacket(80, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet100K_100){
    EntryPacket(100);
    ExitPacket(100, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet100K_400){
    EntryPacket(400);
    ExitPacket(400, 100000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet40K_400){
    EntryPacket(400);
    ExitPacket(400, 40000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet40K_200){
    EntryPacket(200);
    ExitPacket(200, 40000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet20K_400){
    EntryPacket(400);
    ExitPacket(400, 20000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}
TEST(PerfAdjust, Packet20K_200){
    EntryPacket(200);
    ExitPacket(200, 20000, NIC_RXBURST, WORKER_RX_ENQ_BURST, WORKER_RX_DEQ_BURST, WORKER_TX_ENQ_BURST, WORKER_TX_DEQ_BURST, NIC_TXBURST);
}

TEST(PerfAdjust, Packet20K_400_B128){
    // total burst => 128
    EntryPacket(400);
    ExitPacket(400, 20000, NIC_RXBURST, WORKER_RX_ENQ_BURST, 128, 128, 128, NIC_TXBURST);
}


int main(int argc, char* argv[]){
    __app_ = MIXIPGW::App::Init(argc, argv);
    if ((__pktmbuf_pool_ = rte_pktmbuf_pool_create("mbuf_pool_warmup", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}

