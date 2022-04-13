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

#include "gtest/gtest.h"

#include "accumulator.hpp"

using namespace MIXIPGW;

#define TEST_DIR    ("/tmp/test")

TEST(Init_UnInit, test_00){
    Accumulator*   accm = NULL;
    EXPECT_EQ(Accumulator::Init(ACCM::SERVER, TEST_DIR, &accm), 0);
    EXPECT_NE((void*)accm, (void*)NULL);
    if (accm){
        EXPECT_EQ(Accumulator::UnInit(&accm), 0);
    }
    accm = NULL;
}

TEST(Init_UnInit, test_01){
    Accumulator*   accm = NULL;
    EXPECT_EQ(Accumulator::Init(ACCM::SERVER, TEST_DIR, &accm), 0);
    EXPECT_NE((void*)accm, (void*)NULL);
    //
    EXPECT_EQ(accm->UnSerialize(TEST_DIR), 0);

    if (accm){
        EXPECT_EQ(Accumulator::UnInit(&accm), 0);
    }
    accm = NULL;
}
