#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_malloc.h>
#include <rte_kni.h>
#include <rte_ip.h>
#include <rte_arp.h>

#include <assert.h>
#include <math.h>

#include "gtest/gtest.h"

#include "core.hpp"
#include "mixi_pgw_data_plane_def.hpp"
#include "test_def.hpp"
#include "ut_misc_inline_nodb.cc"
#include "pkts/pkts.cc"
#define TUNNEL_FIXED_SRCIP  (0x0a0b0c0d)

using namespace MIXIPGW;

class CustomEnvironment :public ::testing::Environment {
public:
    virtual ~CustomEnvironment() {}
    virtual void SetUp() {
        PGW_LOG(RTE_LOG_INFO, "setup.\n");
    }
    virtual void TearDown() {
        PGW_LOG(RTE_LOG_INFO, "tear down.\n");
    }
};

static Ring                                 __rings_;

static CoreRx*                              __rx_1 = NULL;
static CoreGretermWorker*                   __greterm = NULL;
static CoreTx*                              __tx_1 = NULL;

static struct rte_mempool*  __pktmbuf_pool_ = NULL;

TEST(TunnelServerIngress, Init){
    __rx_1      = new CoreRx(1, 0, 0,TUNNEL_EGRESS);
    __greterm   = new CoreGretermWorker(0,PGW_TUNNEL_IP);
    __tx_1      = new CoreTx(0, 0, 0);
    __rx_1->SetRingAddr(ORDER::FROM, __rings_[0]);
    __rx_1->SetRingAddr(ORDER::TO, __rings_[1]);
    __greterm->SetRingAddr(ORDER::FROM, __rings_[1]);
    __greterm->SetRingAddr(ORDER::TO, __rings_[2]);
    __tx_1->SetRingAddr(ORDER::FROM, __rings_[2]);
    __tx_1->SetRingAddr(ORDER::TO, __rings_[3]);

    __rx_1->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __rx_1->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __greterm->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __greterm->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __tx_1->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __tx_1->SetN(MIXIPGW::KEY::BURST_TO, 1);
}

TEST(TunnelServerIngress,ALLTest){
    const uint8_t   SRCMAC  = 0xfe;
    const uint8_t   DSTMAC  = 0x23;
    const uint32_t  SRCADDR = 0xdeadbeaf;
    const uint32_t  DSTADDR = 0xdeadc0de;

    uint64_t count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    printf("-- IPv4 TEST --\n");

    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    auto compare = rte_pktmbuf_alloc(__pktmbuf_pool_);

    #define         PKTSIZE     (234)
    #define         TEID        (0xdeadbeaf)
    #define         DSTADDR     (0x11223344)
    #define         SRCADDR     (0x99887766)

    set_packet_gtpu(pkt, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);
    set_packet_gtpu(compare, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);

    rte_ring_sp_enqueue(__rings_[0], pkt);
    __rx_1->Cycle(NULL, &count);
    __greterm->Cycle(NULL, &count);
    __tx_1->Cycle(NULL, &count);
    auto nburst = rte_ring_sc_dequeue_burst(__rings_[3], (void **) input, 32, NULL);

    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[0]);
        auto prev_len = rte_pktmbuf_data_len(compare);

        EXPECT_EQ(current_len + 4 + 12 /*gre + ip option*/ + sizeof(ipv4_hdr), prev_len);
        // validate decapsulate.

        EXPECT_EQ(
            memcmp(
                current+sizeof(ether_hdr),
                prev+ + 4 + 12 /*gre + ip option*/ + sizeof(ipv4_hdr) + sizeof(ether_hdr),
                current_len - sizeof(ether_hdr)
            ),
            0
        );
        // equal payload
    }
    rte_pktmbuf_free(pkt);
    rte_pktmbuf_free(compare);

    printf("-- VLAN DROP TEST --\n");

    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    set_packet_temp(pkt,PTEMP_ICMP_IPV4_VLAN);
    set_packet_temp(compare,PTEMP_ICMP_IPV4_VLAN);
    rte_ring_sp_enqueue(__rings_[0], pkt);
    __rx_1->Cycle(NULL, &count);
    __greterm->Cycle(NULL, &count);
    __tx_1->Cycle(NULL, &count);
    nburst = rte_ring_sc_dequeue_burst(__rings_[3], (void **) input, 32, NULL);

    EXPECT_EQ(nburst!=1,true);

    rte_pktmbuf_free(pkt);
    rte_pktmbuf_free(compare);

    printf("-- OSPF TEST --\n");

    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    set_packet_temp(pkt,PTEMP_OSPF_IPV4);
    set_packet_temp(compare,PTEMP_OSPF_IPV4);
    rte_ring_sp_enqueue(__rings_[0], pkt);
    __rx_1->Cycle(NULL, &count);
    __greterm->Cycle(NULL, &count);
    __tx_1->Cycle(NULL, &count);
    nburst = rte_ring_sc_dequeue_burst(__rings_[3], (void **) input, 32, NULL);

    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[0]);
        auto prev_len = rte_pktmbuf_data_len(compare);
        EXPECT_EQ(prev_len, current_len);
        // must be equal  packet length
        EXPECT_EQ(memcmp(current,prev,current_len),0);
        // and payload.
    }

    rte_pktmbuf_free(pkt);
    rte_pktmbuf_free(compare);

    printf("-- BFD TEST --\n");

    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    set_packet_temp(pkt,PTEMP_BFD_UDP_IPV4);
    set_packet_temp(compare,PTEMP_BFD_UDP_IPV4);
    rte_ring_sp_enqueue(__rings_[0], pkt);
    __rx_1->Cycle(NULL, &count);
    __greterm->Cycle(NULL, &count);
    __tx_1->Cycle(NULL, &count);
    nburst = rte_ring_sc_dequeue_burst(__rings_[3], (void **) input, 32, NULL);

    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[0]);
        auto prev_len = rte_pktmbuf_data_len(compare);
        EXPECT_EQ(prev_len, current_len);
        // must be equal  packet length
        EXPECT_EQ(memcmp(current,prev,current_len),0);
        // and payload.
    }
    rte_pktmbuf_free(pkt);
    rte_pktmbuf_free(compare);

    printf("-- BGP TEST --\n");

    pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    set_packet_temp(pkt,PTEMP_BGP_TCP_IPV4);
    set_packet_temp(compare,PTEMP_BGP_TCP_IPV4);
    rte_ring_sp_enqueue(__rings_[0], pkt);
    __rx_1->Cycle(NULL, &count);
    __greterm->Cycle(NULL, &count);
    __tx_1->Cycle(NULL, &count);
    nburst = rte_ring_sc_dequeue_burst(__rings_[3], (void **) input, 32, NULL);

    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[0]);
        auto prev_len = rte_pktmbuf_data_len(compare);
        EXPECT_EQ(prev_len, current_len);
        // must be equal  packet length
        EXPECT_EQ(memcmp(current,prev,current_len),0);
        // and payload
    }
    rte_pktmbuf_free(pkt);
    rte_pktmbuf_free(compare);
}


int main(int argc, char* argv[]){
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    if ((__pktmbuf_pool_ = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }

    char nm[128] = {0};
    for(auto n = 0;n < 4;n++){
        snprintf(nm, sizeof(nm)-1,"ring_%u", n);
        __rings_.push_back(rte_ring_create(nm, 512, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ));
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
