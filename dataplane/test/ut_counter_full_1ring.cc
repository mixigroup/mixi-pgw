#include "ut_counter_inc.hpp"

using namespace MIXIPGW;

#include "ut_counter_entry.cc"

#define TEST01_PACKET_SIZE  (256)

//
void test_ingress(void) {
    if (!__ingress.ring_){
        rte_exit(EXIT_FAILURE, "invalid .ring pointer.(test2ring)(%p)\n", __ingress.ring_);
        return;
    }
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
    ip->src_addr        = htonl(__BASE_IP_ | (uint32_t)(__counter_s%128+1));
    ip->dst_addr        = htonl(__BASE_IP_ | (uint32_t)(__counter_s%128+2)); // aggregate at ip.dst
    ip->total_length    = rte_cpu_to_be_16(TEST01_PACKET_SIZE - sizeof(*eh));
    ip->hdr_checksum    = 0;
    //
    udp->dst_port       = htons(2152);
    //
    gtp->tid            = htonl(__counter_s%128+2);
    gtp->length         = rte_cpu_to_be_16(TEST01_PACKET_SIZE - sizeof(*eh) - sizeof(*ip) - sizeof(*gtp));
    gtp->u.v1_flags.version = GTPU_VERSION_1;
    gtp->type           = 0xFF;
    //
    ipi->version_ihl    = (IP_VERSION | 0x05);
    ipi->src_addr       = ip->src_addr;
    ipi->dst_addr       = ip->dst_addr;
    //
    memcpy(payload, &__counter_s, sizeof(__counter_s));

    mbuf->pkt_len = mbuf->data_len = TEST01_PACKET_SIZE;
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

    if (__ingress.target_->Cycle(NULL, &cycle_counter) != 0){
        rte_exit(EXIT_FAILURE, "failed. Cycle(test_00_ingress)(%p - %p)\n", __ingress.target_, __ingress.ring_);
    }
    if (__counter_s >= 256){
        sleep(1);                   // wait for dequeue
        rte_atomic32_inc(&__stop);  // stop testing.
    }
}

void test_egress(void) {
    struct rte_mbuf *pkt;
    if (!__egress.ring_){
        rte_exit(EXIT_FAILURE, "missing egress ring..test_00_egress.\n");
        return;
    }
    auto ret = rte_ring_sc_dequeue(__egress.ring_, (void**)&pkt);
    if (ret == 0){
        printf("succeeded .dequeue \n");

        auto psize = (TEST01_PACKET_SIZE - sizeof(struct ether_hdr) - sizeof(struct ipv4_hdr) - sizeof(struct gtpu_hdr));

        auto counting = rte_pktmbuf_mtod(pkt, counting_ptr);
        EXPECT_EQ(counting->count, TRANSFER_RING_BURST);

        unsigned total = 0;
        for(auto n = 0;n < MIN(TRANSFER_RING_BURST, counting->count);n++){
            total += counting->items[n].used;
        }
        EXPECT_EQ(total, psize*2*TRANSFER_RING_BURST);
        //
        for(auto n = 0;n < MIN(TRANSFER_RING_BURST, counting->count);n++){
            EXPECT_EQ(counting->items[n].used, psize*2);
            EXPECT_EQ(counting->items[n].key>=2 && counting->items[n].key < 130, true);
        }
        __counter_r++;
    }
    usleep(TEST_DELAY/10);
}


TEST(Counter, test_01){
    MyMemPool   mymempool;
    
    __ingress.target_ = new MIXIPGW::CoreCounterWorker(0,0);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __ingress.ring_);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::TO,   __egress.ring_);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __ingress.target_->SetP(MIXIPGW::KEY::OPT, (void*)&mymempool);
    __ingress.target_->SetN(MIXIPGW::KEY::FLUSH_DELAY, 256);
    __egress.target_ = __ingress.target_;
    
    rte_eal_mp_remote_launch(test_loop, NULL, CALL_MASTER);
    unsigned lcore;
    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return;
        }
    }
    return;
}

