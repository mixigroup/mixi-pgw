#include "ut_counter_inc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>

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

#define         PKTSIZE     (96)
#define         TEID        (0xdeadbeaf)
#define         DSTADDR     (0x11223344)
#define         SRCADDR     (0x99887766)

using namespace MIXIPGW;

static struct rte_ring*     __ring_before_ = NULL;
static struct rte_ring*     __ring_after_ = NULL;
static struct rte_mempool*  __pktmbuf_pool_ = NULL;
static MIXIPGW::CoreRxDistributor* __target = NULL;
static int enq_cnt = 0;
static int deq_cnt = 0;
static int enq_miss = 0;
static int deq_miss = 0;
static int cyc_miss = 0;
static clock_t start = clock();
static clock_t end;
static char qloop = 0;
static char flag = 0;

#define TEST_NUMBER_OF_PACKETS 1000000
#define TEST_BURST_SIZE 4
#define TEST_CYCLE_TIMES 200

static void *cycler(void *param){
    uint64_t count = 0;
    while(qloop != 1) {
        for (int i = 0; i < TEST_CYCLE_TIMES; i++) __target->Cycle(NULL, &count);
        cyc_miss++;
        usleep(1);
    }
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    printf("CYC CPU TIME : %4ld%03ldms \n", ts.tv_sec, ts.tv_nsec / 1000000);
}

static void *packet_enqueuer(void *param){
    while (qloop != 1) {
        auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
        set_packet_gtpu(pkt, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);
        auto ret = rte_ring_sp_enqueue(__ring_before_, pkt);
        if (ret == 0){
            enq_cnt ++;
        } else {
            rte_pktmbuf_free(pkt);
            enq_miss++;
            usleep(1);
        }
    }
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    printf("ENQ CPU TIME : %4ld%03ldms \n", ts.tv_sec, ts.tv_nsec / 1000000);

}

static void *packet_dequeuer(void *param) {
    Buf input[DEFAULT_MBUF_ARRAY_SIZE];
    while (deq_cnt < TEST_NUMBER_OF_PACKETS) {
        auto nburst = rte_ring_sc_dequeue_burst(__ring_after_, (void **) input, 64, NULL);
        if (nburst >= 1) {
            if(unlikely(flag == 0)) {
                start = clock();
                flag = 1;
            }
            deq_cnt += nburst;
            for (int s = 0; s < nburst; s++) {
                rte_pktmbuf_free(input[s]);
            }
        }
        else{
            deq_miss++;
            usleep(1);
        }
    }
    end = clock();
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    printf("DEQ CPU TIME : %4ld%03ldms \n", ts.tv_sec, ts.tv_nsec / 1000000);
}

TEST(RxDistributor, Init){
    __target = new MIXIPGW::CoreRxDistributor(0,0,0);
    EXPECT_EQ(__target!=NULL,true);
    if (__target){
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::FROM,   __ring_before_), 0);
        EXPECT_EQ(__target->SetRingAddr(MIXIPGW::ORDER::TO,     __ring_after_), 0);

        __target->SetN(MIXIPGW::KEY::BURST_FROM, TEST_BURST_SIZE);
        __target->SetN(MIXIPGW::KEY::BURST_TO, TEST_BURST_SIZE);
    }
}

TEST(RxDistributor, ThroughPut){

    pthread_t enq;
    pthread_t cycle;
    pthread_t deq;

    flag = 0;
    qloop = 0;
    deq_cnt = 0;

    pthread_create(&cycle, NULL, cycler, NULL);
    pthread_create(&deq, NULL, packet_dequeuer, NULL);
    pthread_create(&enq, NULL, packet_enqueuer, NULL);

    pthread_join(deq, NULL);
    qloop = 1;
    pthread_join(cycle,NULL);
    pthread_join(enq, NULL);

    auto pkt = rte_pktmbuf_alloc(__pktmbuf_pool_);
    set_packet_gtpu(pkt, 0xde, 0xad, htonl(SRCADDR), htonl(DSTADDR), 2152, 2152, htonl(TEID), PKTSIZE);
    auto len = rte_pktmbuf_pkt_len(pkt);
    printf("packet size: %d byte\n",len);
    rte_pktmbuf_free(pkt);

    printf("enq/deq : %d/%d\n",enq_cnt,deq_cnt);
    printf("enq func_call / cyc_func_call / deq func_call : %d/%d/%d (times)\n",enq_miss,cyc_miss,deq_miss);

    auto through_time = (double) (end - start) / CLOCKS_PER_SEC * 1000;
    printf("%d packet throughtime: %fms\n",deq_cnt, through_time);
    printf("throughput: %3.3f Gbps ",deq_cnt/through_time*1000*len*8/1000000000);
    for(int l=0;l<(int)(deq_cnt/through_time*1000*len*8/500000000);l++){printf(">>");}
    printf("\n");
}

int main(int argc, char* argv[]){
    MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {99,};
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
