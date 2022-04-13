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

namespace MIXIPGW{
  int PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1),};
};

static struct rte_ring*     __ring_before_ = NULL;
static struct rte_ring*     __ring_after_ = NULL;
static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CoreGtpu2GreWorker* __target = NULL;

TEST(Gtpu2GreWorker, Init){
    __target = new MIXIPGW::CoreGtpu2GreWorker(0,TUNNEL_FIXED_SRCIP);
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM,   __ring_before_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_after_), 0);

        __target->SetN(MIXIPGW::KEY::BURST_FROM, 1);
        __target->SetN(MIXIPGW::KEY::BURST_TO, 1);
    }
}

TEST(Gtp2GreWorker,IPv4Test){
    const uint8_t   SRCMAC  = 0xfe;
    const uint8_t   DSTMAC  = 0x23;
    const uint32_t  SRCADDR = 0xdeadbeaf;
    const uint32_t  DSTADDR = 0xdeadc0de;
    const uint16_t  SRCPORT = 9999;
    const uint16_t  DSTPORT = PGWEGRESSPORT;
    const uint16_t  PKTSIZE = 456;
    const uint32_t  TEID    = 0;
    uint64_t        count = 0;

    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    auto compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; }

    set_packet_gtpu(pkt, SRCMAC, DSTMAC, SRCADDR, DSTADDR, SRCPORT, DSTPORT, TEID, PKTSIZE);
    set_packet_gtpu(compare,SRCMAC,DSTMAC,SRCADDR,DSTADDR,SRCPORT,DSTPORT, TEID, PKTSIZE);

    auto ret = rte_ring_sp_enqueue(__ring_before_, pkt);
    EXPECT_EQ(ret,0);

    EXPECT_EQ(__target->Cycle(NULL, &count), 0);
    auto nburst = rte_ring_sc_dequeue_burst(__ring_after_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst,1);
    if (nburst == 1){
        auto current  = rte_pktmbuf_mtod_offset(input[0], char*, 0);
        auto prev = rte_pktmbuf_mtod_offset(compare, char*, 0);
        auto current_len = rte_pktmbuf_data_len(input[0]);
        auto prev_len = rte_pktmbuf_data_len(compare);

        EXPECT_EQ(current_len,prev_len);
        //must be equal  packet length

        EXPECT_EQ(memcmp(current,prev,sizeof(struct ether_hdr)),0);
        // validate ether header
        
        auto gh = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
        EXPECT_EQ(memcmp(current + gh,prev + gh,current_len - gh),0);
        // equal  payload.

        EXPECT_EQ(*(uint8_t *)(prev+sizeof(struct ether_hdr)+9),0x11);
        EXPECT_EQ(*(uint8_t *)(current+sizeof(struct ether_hdr)+9),0x2f);
        // next protocol == convert udp to gre
    }
}

TEST(Gtp2GreWorker,vlan_icmpTest){
    uint64_t count = 0;
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];

    EXPECT_EQ(__target!=NULL,true);
    if (!__target){ return; }
    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; };
    auto compare = rte_pktmbuf_alloc(__pktmbuf_pool_);
    EXPECT_EQ(compare!=NULL,true);
    if (!compare){ return; }

    set_packet_temp(pkt,PTEMP_ICMP_IPV4_VLAN);
    set_packet_temp(compare,PTEMP_ICMP_IPV4_VLAN);

    auto ret = rte_ring_sp_enqueue(__ring_before_, pkt);
    EXPECT_EQ(ret,0);

    EXPECT_EQ(__target->Cycle(NULL, &count), 0);
    auto nburst = rte_ring_sc_dequeue_burst(__ring_after_, (void **) input, 32, NULL);
    EXPECT_EQ(nburst!=1,true);

}


int main(int argc, char* argv[]){
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    if ((__pktmbuf_pool_ = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }

    __ring_before_ = rte_ring_create("before", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    __ring_after_ = rte_ring_create("after", 1024, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

    if (!__ring_before_|| !__ring_after_){
        rte_exit(EXIT_FAILURE, "rte_ring_crfeate..\n");
    } 
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}

