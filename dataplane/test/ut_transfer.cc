#include "ut_counter_inc.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace MIXIPGW;

#define USE_VIRTUAL_CYCLE

#include "ut_counter_entry.cc"
static uint64_t start_time = 0;
static uint64_t end_time = 0;

static int server_socket_ = -1;
static int client_socket_ = -1;

#define TEST_PACKET_SIZE    (1024)
#define LIMIT_USER_SIZE     (8*1024)

//
void test_ingress(void) {
    if (!__ingress.ring_){
        rte_exit(EXIT_FAILURE, "invalid .ring pointer.(transfer)(%p)\n", __ingress.ring_);
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
            rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(transfer)(%p)\n", __pktmbuf_pool);
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
//      printf("input counter (thread : %p -> %u)\n", (void*)pthread_self(), (unsigned)__counter_s);
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
    usleep(TEST_DELAY/10000);
}

void test_egress(void) {
    fd_set	readfd;
    struct timeval  to;

    to.tv_sec  = 0;
    to.tv_usec = 10000;
    FD_ZERO(&readfd);
    FD_SET(client_socket_,&readfd);
    auto ret = select(client_socket_ + 1,&readfd,NULL,NULL,&to);
    if(ret > 0){
        if(FD_ISSET(client_socket_,&readfd)){
            char rbf[sizeof(counting_t)] = {0};
            auto rsz = recv(client_socket_, rbf, sizeof(rbf), 0);
            auto pc = (counting_ptr)rbf;
            EXPECT_EQ((unsigned)rsz, sizeof(rbf));
            EXPECT_EQ(pc->count, TRANSFER_RING_BURST);
#if 1
            EXPECT_EQ(pc->reserved, (uint32_t)__counter_r);
            if (pc->reserved != (uint32_t)__counter_r){
                auto dbg_flag = (uint64_t)-1;
                __egress.target_->Cycle(NULL, &dbg_flag);

                rte_exit(EXIT_FAILURE, "?? drop ??: %u  %u\n", (uint32_t)pc->reserved, (uint32_t)__counter_r);
            }
#endif
            if (pc->count == TRANSFER_RING_BURST){
                for(auto n = 0;n < TRANSFER_RING_BURST;n++){
                    EXPECT_EQ(pc->items[n].key, (uint32_t)(pc->reserved*TRANSFER_RING_BURST+n));
                    EXPECT_EQ(pc->items[n].used,(uint32_t)(pc->items[n].key+1));
                }
            }else{
                rte_exit(EXIT_FAILURE, "malformed packet(tcp): %u\n", pc->count);
            }
            __counter_r++;
            if (__counter_r%8192==0){
                printf("output counter(tcp) (%u)\n", (unsigned)__counter_r);
            }
            if (__counter_r >= LIMIT_USER_SIZE){
                rte_atomic32_inc(&__stop);  // stop testing
            }
        }
    }
}

void test_virtual(void){
    uint64_t cycle_counter = 0;
    if (__egress.target_->VirtualCycle(NULL, &cycle_counter) != 0){
        rte_exit(EXIT_FAILURE, "transfer worker(virtual Cycle)\n");
    }
}

TEST(Transfer, Transfer){
    struct sockaddr_in  srv_addr;
    int on = 1;
    unsigned lcore;
    if ((server_socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        rte_exit(EXIT_FAILURE, "create socket(tcp): %s\n", strerror(errno));
    }
    srv_addr.sin_family    = AF_INET;
    srv_addr.sin_port      = htons(2345);
    inet_pton(AF_INET, "127.0.0.1" ,&srv_addr.sin_addr.s_addr);

    if (bind(server_socket_, (struct sockaddr*)&srv_addr, sizeof(srv_addr))) {
        rte_exit(EXIT_FAILURE, "bind..[src] %s\n", strerror(errno));
    }
    on = 1;
    if (setsockopt(server_socket_, SOL_TCP, TCP_NODELAY, &on, sizeof(on)) < 0){
        rte_exit(EXIT_FAILURE, "nagle off.. %s\n", strerror(errno));
    }
    on = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        rte_exit(EXIT_FAILURE, "reuse addr.. %s\n", strerror(errno));
    }
    if (listen(server_socket_, 1024) < 0){
        rte_exit(EXIT_FAILURE, "listen .. %s\n", strerror(errno));
    }
    pthread_t th;
    pthread_create(&th, NULL, [](void* arg)->void*{
        auto ssock = (int*)arg;
        for(auto f_stop = rte_atomic32_read(&__stop);!f_stop;rte_atomic32_read(&__stop)){
            auto csock = accept((*ssock), NULL, NULL);
            if (client_socket_ != -1){
                close(client_socket_);
            }
            client_socket_ = csock;
        }
        return((void*)NULL);
    }, &server_socket_);

    __ingress.target_ = new MIXIPGW::CoreTransferWorker(0,"127.0.0.1",1234,"127.0.0.1",2345,0);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __ingress.ring_);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::TO,   __egress.ring_);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_FROM, 32);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_TO, 32);
    __ingress.target_->SetN(MIXIPGW::KEY::FLUSH_DELAY, 100000);
    __egress.target_ = __ingress.target_;
    rte_eal_mp_remote_launch(test_loop, NULL, CALL_MASTER);

    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return;
        }
    }
    return;
}

