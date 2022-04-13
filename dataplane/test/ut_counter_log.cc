#include "ut_counter_inc.hpp"

using namespace MIXIPGW;

#define USE_VIRTUAL_CYCLE

#include "ut_counter_entry.cc"

#define TEST_PACKET_SIZE    (1024)

#ifdef __VIRTUAL__
#define LIMIT_USER_SIZE     (8*1024)
#else
#define LIMIT_USER_SIZE     (16*1024)
#endif

//
void test_ingress(void) {
    if (!__ingress.ring_){
        rte_exit(EXIT_FAILURE, "invalid .ring pointer.(counter_log)(%p)\n", __ingress.ring_);
        return;
    }
    if (__counter_s >= LIMIT_USER_SIZE){
        __counter_s++;
        auto cycle_counter = (uint64_t)100000;
        if (__ingress.target_->Cycle(NULL, &cycle_counter) != 0){
            rte_exit(EXIT_FAILURE, "failed. Cycle(over limits.)(%p - %p)\n", __ingress.target_, __ingress.ring_);
        }
    }else{
        auto mbuf = rte_pktmbuf_alloc(__pktmbuf_pool);
        if (unlikely(!mbuf)) {
            rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(counter_log)(%p)\n", __pktmbuf_pool);
        }

        auto cnt = rte_pktmbuf_mtod(mbuf, counting_ptr);
        //
        bzero(cnt, sizeof(counting_t));

        cnt->count = TRANSFER_RING_BURST;
        cnt->reserved = (uint32_t)__counter_s;
        for(auto n = 0;n < cnt->count;n++){
            cnt->items[n].key  = (uint32_t)(cnt->reserved*TRANSFER_RING_BURST+n);
            cnt->items[n].used = (uint32_t)(cnt->items[n].key+1);
        }

        __counter_s++;
        if (__counter_s%8192==0){
            printf("input counter (%u)\n", (unsigned)__counter_s);
        }

        mbuf->pkt_len = mbuf->data_len = sizeof(counting_t);
        mbuf->nb_segs = 1;
        mbuf->next = NULL;
        //
        auto ret = rte_ring_sp_enqueue(__ingress.ring_, mbuf);
        if (unlikely(ret == -ENOBUFS)) {
            rte_exit(EXIT_FAILURE, "Error no buffer.\n");
            return;
        }else if (ret < 0){
            rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
        }
        auto cycle_counter = __counter_s;
        if (__ingress.target_->Cycle(NULL, &cycle_counter) != 0){
            rte_exit(EXIT_FAILURE, "failed. Cycle(test_ingress)(%p - %p)\n", __ingress.target_, __ingress.ring_);
        }
    }
    //
    if (__counter_s>LIMIT_USER_SIZE && __counter_s%LIMIT_USER_SIZE==0){
        usleep(TEST_DELAY);
        auto dbg_flag = (uint64_t)-1;
        __ingress.target_->Cycle(NULL, &dbg_flag);
        rte_atomic32_inc(&__stop);  // stop testing.
    }
    rte_pause();
    sched_yield();
#ifdef __VIRTUAL__
usleep(TEST_DELAY/10);
#else
usleep(1000*1000/250);
#endif

}

void test_egress(void) {
    rte_pause();
    sched_yield();
    usleep(TEST_DELAY);
}

void test_virtual(void){

    static int __is_first_ = 0;
    if (__is_first_ == 0){
        __egress.target_->BeforeCycle(NULL);
        __is_first_ = 1;
    }else{
        uint64_t cycle_counter = 0;
        if (__egress.target_->VirtualCycle(NULL, &cycle_counter) != 0){
            rte_exit(EXIT_FAILURE, "counter_log worker(virtual Cycle)\n");
        }
        rte_pause();
        sched_yield();
#ifdef __VIRTUAL__
        usleep(TEST_DELAY/10000);
#endif
    }
}

TEST(CounterLog, CounterLog){
    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    unsigned lcore;
    Mysql   db;

    EXPECT_EQ(db.Query("TRUNCATE TABLE traffic_log"), 0);

    __ingress.target_ = new MIXIPGW::CoreCounterLogWorker(0,uri.c_str());
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __ingress.ring_);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::TO,   __egress.ring_);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_FROM, 128);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_TO, 128);
    __ingress.target_->SetN(MIXIPGW::KEY::FLUSH_DELAY, 100000);
    __egress.target_ = __ingress.target_;

    rte_eal_mp_remote_launch(test_loop, NULL, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return;
        }
    }
    EXPECT_EQ(db.Query("SELECT COUNT(1) FROM traffic_log"),0);

    auto res = mysql_store_result(db.dbhandle_);
    EXPECT_EQ(res!=NULL,true);
    auto rown = mysql_num_rows(res);
    EXPECT_EQ(rown, 1);
    if (rown == 1){
        auto row = mysql_fetch_row(res);
        EXPECT_EQ(row!=NULL,true);
        EXPECT_EQ(atoi(row[0]), int(LIMIT_USER_SIZE*TRANSFER_RING_BURST));
    }
    mysql_free_result(res);

    return;
}

