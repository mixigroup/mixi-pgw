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

#define         PKTSIZE     (234)
#define         TEID        (0xdeadbeaf)
#define         DSTADDR     (0x11223344)
#define         SRCADDR     (0x99887766)

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
static CoreRxDistributor*                   __rxdist_0 = NULL;
static CorePgwIngressDistributorWorker*     __pgw_in = NULL;
static CoreTx*                              __tx_0 = NULL;

static struct rte_mempool*  __pktmbuf_pool_ = NULL;

TEST(PgwIngressDistributorWorker, Init){
    std::string uri = "mysql://root:develop@127.0.0.1:3306";

    __rxdist_0  = new CoreRxDistributor(0, 0, 0);
    __pgw_in    = new CorePgwIngressDistributorWorker(0,uri.c_str(),12004,1,PGW_TUNNEL_IP,PGW_TUNNEL_IP,0,0);
    __tx_0      = new CoreTx(0, 0, 0);

    __rxdist_0->SetRingAddr(ORDER::FROM, __rings_[0]);
    __rxdist_0->SetRingAddr(ORDER::TO, __rings_[1]);
    __rxdist_0->SetRingAddr(ORDER::TO, __rings_[4]);   // not use Egress-side
    __pgw_in->SetRingAddr(ORDER::FROM, __rings_[1]);
    __pgw_in->SetRingAddr(ORDER::TO, __rings_[2]);
    __tx_0->SetRingAddr(ORDER::FROM, __rings_[2]);
    __tx_0->SetRingAddr(ORDER::TO, __rings_[3]);

    __rxdist_0->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __rxdist_0->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __pgw_in->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __pgw_in->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __tx_0->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __tx_0->SetN(MIXIPGW::KEY::BURST_TO, 1);

    __pgw_in->BeforeCycle(NULL);
}

TEST(PgwIngressDistributorWorker,ALLTest){

    uint64_t count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    printf("-- IPv4 TEST --\n");

    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    auto compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    set_packet_gtpu(pkt, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);
    set_packet_gtpu(compare, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);
    rte_ring_sp_enqueue(__rings_[0], pkt);

    pthread_t th;
    pthread_create(&th, NULL, [](void* arg)->void*{
        uint64_t count = 0;
        for(;;){
            __pgw_in->VirtualCycle(NULL, &count);
        }
        return((void*)NULL);
    }, NULL);
    sleep(2);

    __rxdist_0->Cycle(NULL, &count);
    __pgw_in->Cycle(NULL, &count);
    __tx_0->Cycle(NULL, &count);
    auto nburst = rte_ring_sc_dequeue_burst(__rings_[3], (void **) input, 32, NULL);

    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[0]);
        auto prev_len = rte_pktmbuf_data_len(compare);

        EXPECT_EQ(prev_len, current_len);

        auto headerlen = sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
        EXPECT_EQ(
                memcmp(
                        prev+headerlen,
                        current+headerlen,
                        current_len - headerlen
                ),0
        );
        // equal  payload
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
    for(auto n = 0;n < 5;n++){
        snprintf(nm, sizeof(nm)-1,"ring_%u", n);
        __rings_.push_back(rte_ring_create(nm, 512, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ));
    }
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
