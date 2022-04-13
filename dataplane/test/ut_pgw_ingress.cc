#include "ut_counter_inc.hpp"

using namespace MIXIPGW;

// + test_virtual
// + test_ingeress
// + test_egress
// need above 3 functions.
#define TEST_PACKET_SIZE    (554)
#define PAGE_USER_SIZE      (16)
#define INTERNAL_BURST_SIZE (PAGE_USER_SIZE/128)
#define TOTAL_USER_SIZE     (PAGE_USER_SIZE*16)
#define TEST_TID            (0x0AB5dead)

static int __g_halt__  = 0;

#define USE_VIRTUAL_CYCLE

#include "ut_counter_entry.cc"

//
void test_ingress(void) {
    auto mbuf = rte_pktmbuf_alloc(__pktmbuf_pool);
    if (unlikely(!mbuf)) {
        rte_exit(EXIT_FAILURE, "invalid .rte_pktmbuf_alloc(alloc)(%p)\n", __pktmbuf_pool);
    }
    auto eh = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
    auto ip = (struct ipv4_hdr*)(eh+1);
    auto udp= (struct udp_hdr*)(ip+1);
    auto gtp= (struct gtpu_hdr*)(udp+1);
    auto payload = (char*)(gtp + 1);
    bzero(eh, TEST_PACKET_SIZE);
    //
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    ether_addr_copy(&__src_mac, &eh->s_addr);
    ether_addr_copy(&__dst_mac, &eh->d_addr);

    __counter_s++;
    // simulate SGW -> PGW direction packet
    // ip version 4
    ip->version_ihl     = (IP_VERSION | 0x05);
    ip->type_of_service = 0;
    ip->packet_id       = 0;
    ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ip->time_to_live    = IP_DEFTTL;
    ip->next_proto_id   = IPPROTO_UDP;
    ip->hdr_checksum    = 0;
    ip->src_addr        = (uint32_t)__counter_s+1;
    ip->dst_addr        = (uint32_t)__counter_s+2;
    ip->total_length    = rte_cpu_to_be_16(TEST_PACKET_SIZE - sizeof(*eh));
    ip->hdr_checksum    = 0;
    //
    udp->dst_port       = htons(2152);
    //
    gtp->tid            = TEST_TID;
    gtp->length         = rte_cpu_to_be_16(TEST_PACKET_SIZE - sizeof(*eh) - sizeof(*ip) - sizeof(*gtp));
    //
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
    // flush buffer
    auto cycle_counter = __counter_s;
    if (cycle_counter%PAGE_USER_SIZE == 0 && cycle_counter > 0){
        printf("cycle_.. flush: %u\n", (uint32_t)cycle_counter);
        cycle_counter = 100000;
    }
    if (__ingress.target_->Cycle(NULL, &cycle_counter) != 0){
        rte_exit(EXIT_FAILURE, "failed. Cycle(test_ingress)(%p - %p)\n", __ingress.target_, __ingress.ring_);
    }
    usleep(10000);
}


void test_egress(void) {
    struct rte_mbuf *pkt;
    if (!__egress.ring_){
        rte_exit(EXIT_FAILURE, "missing egress ring..enough map.\n");
        return;
    }
    auto ret = rte_ring_sc_dequeue(__egress.ring_, (void**)&pkt);
    if (ret == 0){
        printf("succeeded .dequeue (%u)\n", (unsigned)__counter_r);
        __counter_r++;

        auto ip = rte_pktmbuf_mtod_offset(pkt, struct ipv4_hdr*, sizeof(ether_hdr));

        EXPECT_EQ(ip->src_addr, 0x0a190001);
        EXPECT_EQ(ip->dst_addr, 0x0a04c001);
        //
        if (__counter_r > 256){
            printf("goto finish..\n");
            for(auto n = 0;n < 100;n++){
                rte_atomic32_inc(&__stop);
                usleep(10000);
            }
        }
    }
    usleep(10000);
}

void test_virtual(void){
    // binlog thread
    uint64_t cycle_counter = 0;
    if (__egress.target_->VirtualCycle(NULL, &cycle_counter) != 0){
        rte_exit(EXIT_FAILURE, "test_virtual\n");
    }
}
TEST(PgwIngress, PgwIngress){
    unsigned lcore;
    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    //
    __ingress.target_ = new MIXIPGW::CorePgwIngressWorker(1,uri.c_str(),11801,10,0x0a04c001,0x0a190001,5,0x0B);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::FROM, __ingress.ring_);
    __ingress.target_->SetRingAddr(MIXIPGW::ORDER::TO, __egress.ring_);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_FROM, 1);
    __ingress.target_->SetN(MIXIPGW::KEY::BURST_TO, 1);
    __ingress.target_->SetN(MIXIPGW::KEY::FLUSH_DELAY, 100000);
    //
    __egress.target_ = __ingress.target_;
    pthread_t   th;
    pthread_create(&th, NULL, [](void* arg)->void*{
        static uint64_t egress_counter = 0;
        char bf[128] = {0};
        //
        while(!__g_halt__){
            usleep(10000);
            egress_counter++;
            if ((egress_counter%16)==0){
                fprintf(stderr, "test_egress.(%u) \n", (unsigned)egress_counter);
                Mysql   con;
                std::string sql = "INSERT INTO tunnel (imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,pgw_gtpu_ipv)VALUES";
                sql += "(1234567890,234567890,'1.1.1.1',";
                sprintf(bf, "%u,", htonl(TEST_TID));
                sql += bf;
                sql += bf;
                sql += "'2.2.2.2',1,'3.3.3.3') ON DUPLICATE KEY UPDATE pgw_teid = ";
                sql += bf;
                sql += "active = 1,updated_at=NOW(),pgw_gtpu_ipv=VALUES(`pgw_gtpu_ipv`)";
                //
                if (con.Query(sql.c_str()) != 0){
                    fprintf(stderr, "query..... \n");
                    rte_exit(EXIT_FAILURE, "query .\n");
                }
                // fprintf(stderr, "inserted. (%s)\n", sql.c_str());
            }
        }
        return(NULL);
    }, NULL);

    __ingress.target_->BeforeCycle(NULL);
    rte_eal_mp_remote_launch(test_loop, NULL, CALL_MASTER);

    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return;
        }
    }
    __g_halt__ = 1;
    pthread_join(th, NULL);
    return;
}

