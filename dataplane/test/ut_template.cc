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

#include <assert.h>
#include <math.h>

#include "gtest/gtest.h"

#include "core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

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
  int PGW_LOG_LEVEL = (RTE_LOG_DEBUG+1);
  uint32_t    MIXIPGW::FLUSH_COUNTER = _FLUSH_WORKER_;
  uint32_t    MIXIPGW::FLUSH_ENCAP   = _FLUSH_TX_;
  uint32_t    MIXIPGW::FLUSH_GRETERM = _FLUSH_TX_;
  uint32_t    MIXIPGW::FLUSH_PGW_IE  = _FLUSH_TX_;
  uint32_t    MIXIPGW::FLUSH_RX      = _FLUSH_RX_;
  uint32_t    MIXIPGW::FLUSH_RX_DIST = _FLUSH_RX_;
  uint32_t    MIXIPGW::FLUSH_TX      = _FLUSH_TX_;
};

TEST(UtTemplate, Hellow){
    auto a = 132;
    auto n = ((uint64_t)a*(uint64_t)a);
    //
    EXPECT_EQ(n, (uint64_t)pow(a,2.0));
}


int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}

