#include "ut_counter_inc.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace MIXIPGW;

// + test_virtual
// + test_ingeress
// + test_egress
// needs implement 3 functions
#define USE_VIRTUAL_CYCLE

#include "ut_counter_entry.cc"
static uint64_t start_time = 0;
static uint64_t end_time = 0;

static int server_socket_ = -1;
static int client_socket_ = -1;

#define TEST_PACKET_SIZE    (456)
#define PAGE_USER_SIZE      (1024)
#define INTERNAL_BURST_SIZE (PAGE_USER_SIZE/128)
#define TOTAL_USER_SIZE     (PAGE_USER_SIZE*16)

void test_ingress(void) {
    if (__counter_s >= TOTAL_USER_SIZE){
        __counter_s++;
        auto cycle_counter = (uint64_t)100000;
        if (__ingress.target_->Cycle(NULL, &cycle_counter) != 0){
            rte_exit(EXIT_FAILURE, "failed. Cycle(over limits.)(%p - %p)\n", __ingress.target_, __ingress.ring_);
        }
    }else{
        auto mbuf = rte_pktmbuf_alloc(__pktmbuf_pool);
        if (unlikely(!mbuf)) {
            rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(alloc)(%p)\n", __pktmbuf_pool);
        }
        auto eh = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
        auto ip = (struct ipv4_hdr*)(eh+1);
        auto udp= (struct udp_hdr*)(ip+1);
        auto gtp= (struct gtpu_hdr*)(udp+1);
        auto ipi= (struct ipv4_hdr*)(gtp+1);
        auto payload = (char*)(ipi + 1);
        eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
        ether_addr_copy(&__src_mac, &eh->s_addr);
        ether_addr_copy(&__dst_mac, &eh->d_addr);

        __counter_s++;

        // ip version 4
        ip->version_ihl     = (IP_VERSION | 0x05);
        ip->type_of_service = 0;
        ip->packet_id       = 0;
        ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
        ip->time_to_live    = IP_DEFTTL;
        ip->next_proto_id   = IPPROTO_UDP;
        ip->hdr_checksum    = 0;
        ip->src_addr        = htonl(__BASE_IP_ | (uint32_t)(__counter_s%TOTAL_USER_SIZE+1));
        ip->dst_addr        = htonl(__BASE_IP_ | (uint32_t)(__counter_s%TOTAL_USER_SIZE+2)); // aggregate at ip.dst
        ip->total_length    = rte_cpu_to_be_16(TEST_PACKET_SIZE - sizeof(*eh));
        ip->hdr_checksum    = 0;
        //
        udp->dst_port       = htons(2152);
        //
        gtp->tid            = htonl(__counter_s%TOTAL_USER_SIZE+2);
        gtp->length         = rte_cpu_to_be_16(TEST_PACKET_SIZE - sizeof(*eh) - sizeof(*ip) - sizeof(*gtp));
        gtp->u.v1_flags.version = GTPU_VERSION_1;
        gtp->type           = 0xFF;
        //
        ipi->version_ihl    = (IP_VERSION | 0x05);
        ipi->src_addr       = ip->src_addr;
        ipi->dst_addr       = ip->dst_addr;
        //
        memcpy(payload, &__counter_s, sizeof(__counter_s));

        mbuf->pkt_len = mbuf->data_len = TEST_PACKET_SIZE;
        mbuf->nb_segs = 1;
        mbuf->next = NULL;
        mbuf->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
        //
        auto ret = rte_ring_sp_enqueue(__ingress.ring_, mbuf);
        if (unlikely(ret == -ENOBUFS)) {
            rte_exit(EXIT_FAILURE, "Error no buffer.\n");
            return;
        }else if (ret < 0){
            rte_exit(EXIT_FAILURE, "Error enqueue to target ring.\n");
        }
        auto cycle_counter = __counter_s;
        if (cycle_counter%PAGE_USER_SIZE == 0 && cycle_counter > 0){
            printf("cycle_.. flush: %u\n", (uint32_t)cycle_counter);
            cycle_counter = 100000;
        }
        if (__ingress.target_->Cycle(NULL, &cycle_counter) != 0){
            rte_exit(EXIT_FAILURE, "failed. Cycle(test_ingress)(%p - %p)\n", __ingress.target_, __ingress.ring_);
        }
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

            if (pc->count == TRANSFER_RING_BURST){
                for(auto n = 0;n < TRANSFER_RING_BURST;n++){
                    EXPECT_EQ(pc->items[n].used,TEST_PACKET_SIZE-(14+20+8/* ether + ip + udp */));
                }
            }else{
                rte_exit(EXIT_FAILURE, "malformed packet(tcp): %u\n", pc->count);
            }
            __counter_r++;
            if (__counter_r%INTERNAL_BURST_SIZE==0){
                printf("recieved counting tcp packet(%u - %u packets.)\n", (unsigned)__counter_r, INTERNAL_BURST_SIZE);
            }
            if (__counter_r >= 128){
                rte_atomic32_inc(&__stop);  // stop testing.
            }
        }
    }
}

void test_virtual(void){
    uint64_t cycle_counter = 0;
    // internal ring  -> dqueue -> Transfer  : trigger cycle.
    if (__egress.target_->Cycle(NULL, &cycle_counter) != 0){
        rte_exit(EXIT_FAILURE, "transfer worker(virtual Cycle)\n");
    }
    // Transfer -> dequeue -> tcp send       : trigger cycle.
    if (__egress.target_->VirtualCycle(NULL, &cycle_counter) != 0){
        rte_exit(EXIT_FAILURE, "transfer worker(virtual Cycle)\n");
    }
}

TEST(CounterTransfer, CounterTransfer){
    struct sockaddr_in  srv_addr;
    int on = 1;
    unsigned lcore;
    
    if ((server_socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        rte_exit(EXIT_FAILURE, "create socket(tcp): %s\n", strerror(errno));
    }
    srv_addr.sin_family    = AF_INET;
    srv_addr.sin_port      = htons(12345);
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
    // accept thread(supported single client)
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

    MyMemPool   mymempool;

    __ingress.target_ = new MIXIPGW::CoreCounterWorker(0,0);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __ingress.ring_);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::TO, __internal.ring_);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_TO, INTERNAL_BURST_SIZE);
    __ingress.target_->SetN(MIXIPGW::KEY::FLUSH_DELAY, 100000);
    __ingress.target_->SetP(MIXIPGW::KEY::OPT, (void*)&mymempool);

    __egress.target_ = new MIXIPGW::CoreTransferWorker(0,"127.0.0.1",1234,"127.0.0.1",12345,0);
    __egress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __internal.ring_);
    __egress.target_->SetRingAddr(MIXIPGW::ORDER::TO,   __egress.ring_);
    __egress.target_->SetN(MIXIPGW::KEY::BURST_FROM, INTERNAL_BURST_SIZE);
    __egress.target_->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __egress.target_->SetN(MIXIPGW::KEY::FLUSH_DELAY, 100000);
    
    rte_eal_mp_remote_launch(test_loop, NULL, CALL_MASTER);

    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return;
        }
    }
    return;
}

