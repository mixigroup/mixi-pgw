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

TEST(TunnelPgwEgress, Init){
    EXPECT_EQ(truncate_tunnel(), 0);
    
    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    
    __target = new MIXIPGW::CorePgwEgressTunnelWorker(0,uri.c_str(),13001,1,0x11111111,0x22222222,0x02,0x03);
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM,   __ring_rx_in_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::EXTEND, __ring_err_ind_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_tx_out_), 0);
        
        __target->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target->SetN(MIXIPGW::KEY::BURST_TO, 1);
        __target->BeforeCycle(NULL);
    }
}

TEST(TunnelPgwEgress, Test){
    const uint32_t  DSTADDR = 0x001f3daa;
    const uint32_t  TEID    = 0x001f3daa;

    uint64_t        count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];
    
    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    
    auto compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; };
    
    EXPECT_EQ(truncate_tunnel(), 0);

    set_packet_temp(pkt,PTEMP_TCP_IPV4);
    set_packet_temp(compare,PTEMP_TCP_IPV4);
    //
    auto ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);
    
    auto nburst = rte_ring_sc_dequeue_burst(__ring_err_ind_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    // not found tunnel id
    auto eind = rte_pktmbuf_mtod(input[0], char*);
    //
    auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
    auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
    auto current_len = rte_pktmbuf_data_len(input[0]);
    auto prev_len = rte_pktmbuf_data_len(compare);
    rte_hexdump(stdout,"in",prev,prev_len);
    rte_hexdump(stdout,"out",current,current_len);
    //
    EXPECT_EQ(memcmp(eind,rte_pktmbuf_mtod(compare,char*),rte_pktmbuf_data_len(compare)),0);
    //
    //
    EXPECT_EQ(truncate_tunnel(), 0);
    // regist tunnel data to database
    EXPECT_EQ(insert_tunnel(htonl(TEID), htonl(DSTADDR)), 0);
    // wait for binlog events arrived.
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
    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    set_packet_temp(pkt,PTEMP_TCP_IPV4);
    //
    ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);
    //
    nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    // exists tunnel id ->  gtpu
    current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
    prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
    current_len = rte_pktmbuf_data_len(input[0]);
    prev_len = rte_pktmbuf_data_len(compare);
    rte_hexdump(stdout,"in",prev,prev_len);
    rte_hexdump(stdout,"out",current,current_len);
    auto g = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
    EXPECT_EQ(memcmp(current + g, prev + sizeof(struct ether_hdr), prev_len - sizeof(struct ether_hdr) ),0);

    // unrelated packet 

    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; };

    set_packet_temp(pkt,PTEMP_OSPF_IPV4);
    set_packet_temp(compare,PTEMP_OSPF_IPV4);

    ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
    prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
    current_len = rte_pktmbuf_data_len(input[0]);
    prev_len = rte_pktmbuf_data_len(compare);
    rte_hexdump(stdout,"in",prev,prev_len);
    rte_hexdump(stdout,"out",current,current_len);
    EXPECT_EQ(memcmp(current,prev,current_len),0);

    fprintf(stderr, "...\n");
    }

TEST(TunnelPgwEgress, IPv6Test){
    __halt_ = false;
    const uint32_t  DSTADDR = 0x001f3dab;
    const uint32_t  TEID    = 0x001f3dab;

    uint64_t        count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };

    auto compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; };

    EXPECT_EQ(truncate_tunnel(), 0);

    set_packet_temp(pkt,PTEMP_6RD_IPV6_IPv4);
    set_packet_temp(compare,PTEMP_6RD_IPV6_IPv4);
    auto v4 = rte_pktmbuf_mtod_offset(pkt, struct ipv4_hdr*, sizeof(struct ether_hdr));
    v4->src_addr = htonl(SIXRD_ENCAPPER_ADDR);
    v4 = rte_pktmbuf_mtod_offset(compare, struct ipv4_hdr*, sizeof(struct ether_hdr));
    v4->src_addr = htonl(SIXRD_ENCAPPER_ADDR);
    //
    auto ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    auto nburst = rte_ring_sc_dequeue_burst(__ring_err_ind_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    // not found tunnel id
    auto eind = rte_pktmbuf_mtod(input[0], char*);
    //
    auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
    auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
    auto current_len = rte_pktmbuf_data_len(input[0]);
    auto prev_len = rte_pktmbuf_data_len(compare);
    rte_hexdump(stdout,"in",prev,prev_len);
    rte_hexdump(stdout,"out",current,current_len);
    //
    EXPECT_EQ(memcmp(eind,rte_pktmbuf_mtod(compare,char*),rte_pktmbuf_data_len(compare)),0);
    //
    //
    EXPECT_EQ(truncate_tunnel(), 0);
    // regist tunnel data to database
    EXPECT_EQ(insert_tunnel(htonl(TEID), htonl(DSTADDR)), 0);
    // wait for binlog events.
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
    sleep(2);
    //
    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    set_packet_temp(pkt,PTEMP_6RD_IPV6_IPv4);
    v4 = rte_pktmbuf_mtod_offset(pkt, struct ipv4_hdr*, sizeof(struct ether_hdr));
    v4->src_addr = htonl(SIXRD_ENCAPPER_ADDR);
    //
    ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);
    //
    nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    // exists tunnel id -> gtpu
    current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
    prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
    current_len = rte_pktmbuf_data_len(input[0]);
    prev_len = rte_pktmbuf_data_len(compare);
    rte_hexdump(stdout,"in",prev,prev_len);
    rte_hexdump(stdout,"out",current,current_len);
    auto g = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
    EXPECT_EQ(memcmp(current + g, prev + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr), current_len - g ),0);

    // unrelated packet 

    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; };

    set_packet_temp(pkt,PTEMP_TCP_IPV6);
    set_packet_temp(compare,PTEMP_TCP_IPV6);

    ret = rte_ring_sp_enqueue(__ring_rx_in_, pkt);
    EXPECT_EQ(ret,0);
    EXPECT_EQ(__target->Cycle(NULL, &count), 0);

    nburst = rte_ring_sc_dequeue_burst(__ring_tx_out_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst != 1){ return; }
    current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
    prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
    current_len = rte_pktmbuf_data_len(input[0]);
    prev_len = rte_pktmbuf_data_len(compare);
    rte_hexdump(stdout,"in",prev,prev_len);
    rte_hexdump(stdout,"out",current,current_len);
    EXPECT_EQ(memcmp(current,prev,current_len),0);

    fprintf(stderr, "...\n");
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
    __ring_err_ind_      = rte_ring_create("err_ind", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_tx_out_      = rte_ring_create("tx_out", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    //
    if (!__ring_rx_in_|| !__ring_err_ind_ || !__ring_tx_out_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
