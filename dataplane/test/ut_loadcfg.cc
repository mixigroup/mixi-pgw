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
#include "app.hpp"
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

class MockCore: public CoreInterface{
public:
    MockCore():CoreInterface(1,2,3){ }
    virtual ~MockCore(){}
public:
    virtual TXT   GetObjectName(void){ return("MockCore"); }
    virtual TYPE  GetType(void){ return(TYPE::PGW_INGRESS); }
    virtual RETCD SetRingAddr(ORDER, void*){ return(0); }
    virtual RETCD Cycle(void*,uint64_t*){ return(0); }
};

static App* __app_ = NULL;
static MockCore* __mock_ = NULL;
//
TEST(AppReLoadConfig, Init){
    MIXIPGW::PGW_LOG_LEVEL = 99;
    char f[128] = {0};
    const char *v[] = {
        "loglevel=7",
        "burst_size_rx_read=11",
        "burst_size_rx_enq=22",
        "burst_size_rx_deq=33",
        "burst_size_tx_enq=44",
        "burst_size_tx_deq=55",
        "burst_size_tx_write=66",
        "flush_counter=777",
        "flush_encap=888",
        "flush_greterm=999",
        "flush_pgw_ie=111",
        "flush_rx=222",
        "flush_rx_dist=333",
        "flush_tx=444",
        NULL,
    };
    snprintf(f, sizeof(f)-1,"/tmp/%u.cnf", getpid());
    auto fp = fopen(f, "w+");
    if (fp){
        for(auto n = 0;v[n] != NULL;n++){
            fwrite(v[n], strlen(v[n]), 1, fp);
            fwrite("\n", strlen("\n"), 1, fp);
        }
        fclose(fp);
    }
};
//
TEST(AppReLoadConfig, Check){
    __app_->cores_[0] = __mock_;
    __app_->LoadConfig();
    __app_->ApplyConfig();
    //
    EXPECT_EQ(MIXIPGW::PGW_LOG_LEVEL, 7);
    EXPECT_EQ(MIXIPGW::FLUSH_COUNTER, 777);
    EXPECT_EQ(MIXIPGW::FLUSH_ENCAP, 888);
    EXPECT_EQ(MIXIPGW::FLUSH_GRETERM, 999);
    EXPECT_EQ(MIXIPGW::FLUSH_PGW_IE, 111);
    EXPECT_EQ(MIXIPGW::FLUSH_RX, 222);
    EXPECT_EQ(MIXIPGW::FLUSH_RX_DIST, 333);
    EXPECT_EQ(MIXIPGW::FLUSH_TX, 444);
    //
    EXPECT_EQ(__mock_->GetN(KEY::BURST_TO), 22);
}
//
TEST(AppReLoadConfig, UnInit){
    delete __app_;
    __app_ = NULL;
    delete __mock_;
    __mock_ = NULL;
}
//
int main(int argc, char* argv[]){
    __app_ = App::Init(argc, argv);
    __mock_ = new MockCore();
    //
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}

