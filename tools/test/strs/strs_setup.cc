#include "strs.h"
#include <net/ethernet.h>

void start_thread(system_arg_ptr arg){
    int n;
    thread_arg_ptr  t;
    static const int SKIP_CPUCORE = 1;
    //
    for (n = 0; n < THREAD_NUM(); n++) {
        char nmbf[128] = {0};
        const char *func = NULL;
        t = ref_thread_arg(n);
        t->fd = -1;
        //
        if (arg->func == FUNC_SEND){
            func = "/T";
        }else if (arg->func == FUNC_RECV_SEND){
            func = "";
        }else{
            func = "/R";
        }
        snprintf(nmbf,sizeof(nmbf)-1,"%s-%d%s", arg->ifnm,(arg->affinity+(n*SKIP_CPUCORE)),func);
        struct nmreq base_nmd;
        bzero(&base_nmd, sizeof(base_nmd));
        t->nmd = nm_open(nmbf,  &base_nmd, 0, NULL);
        if (t->nmd == NULL) {
            ERR("Unable to open %s: %s", nmbf, strerror(errno));
            continue;
        }
        LOG("success open %s", nmbf);
        t->fd = t->nmd->fd;
        t->used = 1;
        t->id = n;
        if (arg->affinity >= 0) {
            t->affinity = (arg->affinity +(n*SKIP_CPUCORE));
        } else {
            t->affinity = -1;
        }
        t->vlanid = arg->vlanid;
        t->seq_rand = arg->seq_rand;
        t->ip_range = arg->ip_range;
        t->port_range = arg->port_range;
    }
    // wait for phi - reset
    sleep(PHIRESET_WAITSEC);
    //
    for (n = 0; n < THREAD_NUM(); n++) {
        t = ref_thread_arg(n);
        t->used = 0;
        // only send or receive
        if (arg->func == FUNC_RECV){
            t->operate = strs_recv;
        }else if (arg->func == FUNC_RECV_SEND){
            t->operate = strs_recv_send;
        }else{
            t->operate = strs_send;
        }
        if (pthread_create(&t->thread, NULL, t->operate, t) != -1) {
            t->used = 1;
            LOG("success create thread %d", n);
        }else{
            ERR("Unable to create thread %d: %s", n, strerror(errno));
        }
    }
}
void wait_thread(system_arg_ptr arg){
    int n;
    thread_arg_ptr  t;
    //
    for (n = 0; n < THREAD_NUM(); n++) {
        t = ref_thread_arg(n);
        if (t->used){ pthread_join(t->thread, NULL); }
        LOG("nm_close(%d)",n);
        nm_close(t->nmd);
        t->nmd = NULL;
    }
}
uint64_t wait_for_next_report(struct timeval *prev, struct timeval *cur, int report_interval){
    struct timeval delta;
    delta.tv_sec = report_interval/1000;
    delta.tv_usec = (report_interval%1000)*1000;
    if (select(0, NULL, NULL, NULL, &delta) < 0 && errno != EINTR) {
        perror("select");
        abort();
    }
    gettimeofday(cur, NULL);
    timersub(cur, prev, &delta);
    return delta.tv_sec* 1000000 + delta.tv_usec;
}



void open_netmap(system_arg_ptr arg){
    struct nmreq base_nmd;
    bzero(&base_nmd, sizeof(base_nmd));
    char bf[128] = {0};
    snprintf(bf,sizeof(bf)-1,"%s", arg->ifnm);
    arg->nmd = nm_open(bf, &base_nmd, 0, NULL);
    if (arg->nmd == NULL){
        D("Unable to open...  %s: %s", bf, strerror(errno));
        usage();
    }
    LOG("mapped %dKB at %p", arg->nmd->req.nr_memsize>>10, arg->nmd->mem);
    LOG("nthreads %d, has tx:%d/rx:%d queues", THREAD_NUM(), arg->nmd->req.nr_tx_rings, arg->nmd->req.nr_rx_rings);
    logging_netmap(arg);
    nm_close(arg->nmd);
}
void logging_netmap(system_arg_ptr arg){
    struct netmap_if *nifp = arg->nmd->nifp;
    struct nmreq *req = &arg->nmd->req;
    int n;

//  LOG("nifp at offset %d, %d tx %d rx region %d", req->nr_offset, req->nr_tx_rings, req->nr_rx_rings, req->nr_arg2);
    //
    for (n = 0; n <= req->nr_tx_rings; n++) {
        struct netmap_ring *ring = NETMAP_TXRING(nifp, n);
        LOG("   TX%d at 0x%p slots %d", n, (void *)((char *)ring - (char *)nifp), ring->num_slots);
    }
    for (n = 0; n <= req->nr_rx_rings; n++) {
        struct netmap_ring *ring = NETMAP_RXRING(nifp, n);
        LOG("   RX%d at 0x%p slots %d", n, (void *)((char *)ring - (char *)nifp), ring->num_slots);
    }
}
//
int setaffinity(pthread_t me, int i){
    cpuset_t cpumask;
    if (i == -1) { return(0); }
    CPU_ZERO(&cpumask);
    CPU_SET(i, &cpumask);
    if (pthread_setaffinity_np(me, sizeof(cpuset_t), &cpumask) != 0) {
        ERR("Unable to set affinity: %s", strerror(errno));
        return(1);
    }
    return(0);
}
void init_globals(int argc, char **argv, system_arg_ptr arg){
    struct sigaction sa;
    struct timespec tx_period;
    struct ether_addr *ea,dstmac,srcmac;
    int ch,n,tx_rate = 0,burst = 0,psize = 0, tnum = 0;
    //
    bzero(arg, sizeof(*arg));
    for(n = 0;n < THREAD_NUM();n++){
        if (ref_thread_arg(n)!=NULL){
            bzero(ref_thread_arg(n), sizeof(thread_arg_t));
        }
    }
    //
    while ((ch = getopt(argc, argv, "f:i:c:b:r:l:t:D:S:v:qR:Q:")) != -1) {
        if (ch == 'f'){
            if (!strcmp("tx", optarg)){ // send
                arg->func = FUNC_SEND;
            }else if (!strcmp("rtx", optarg)){
                arg->func = FUNC_RECV_SEND;
            }else{                      // recv
                arg->func = FUNC_RECV;
            }
        }else if (ch == 'i' && optarg){
            strncpy(arg->ifnm, optarg, sizeof(arg->ifnm)-1);
        }else if (ch == 'c' && optarg){
            arg->affinity = atoi(optarg);
        }else if (ch == 'r' && optarg){
            tx_rate = atoi(optarg);
        }else if (ch == 'b' && optarg){
            burst = atoi(optarg);
        }else if (ch == 'l' && optarg){
            psize = atoi(optarg);
        }else if (ch == 't' && optarg){
            tnum  = atoi(optarg);
        }else if (ch == 'v' && optarg){
            arg->vlanid = atoi(optarg);
        }else if (ch == 'R' && optarg){
            arg->ip_range = atoi(optarg);
        }else if (ch == 'q'){
            arg->seq_rand ++;
        }else if (ch == 'Q' && optarg){
            arg->port_range = atoi(optarg);
        }else if (ch == 'D' && optarg){
            if ((ea = ether_aton(optarg))!=NULL){
                memcpy(&dstmac, ea, sizeof(dstmac));
            }
        }else if (ch == 'S' && optarg){
            if ((ea = ether_aton(optarg))!=NULL){
                memcpy(&srcmac, ea, sizeof(srcmac));
            }
        }
    }
    if (arg->func == FUNC_SEND){
        if (!tx_rate || !burst || !psize || psize > MAX_BODYSIZE || !tnum || !*(char*)&srcmac || !*(char*)&dstmac){
            ERR("tx_rate or burst or packet size or thread num is invalid");
            usage();
        }

        // pkt-gen.c : second rating > count of burst
        // packet size , determine size of single transmission(==burst size) base on transmission rate.
//      if (burst > (tx_rate/300)){ burst = (tx_rate/300); }
        tx_period.tv_nsec   = ((uint64_t)1000000000 * (uint64_t)burst) / (uint64_t) tx_rate;
        LOG("period(%d)\n", (int)tx_period.tv_nsec);
        tx_period.tv_sec    = tx_period.tv_nsec / 1000000000;
        tx_period.tv_nsec   = tx_period.tv_nsec % 1000000000;

        // vlanid
        if (arg->vlanid <= 0 || arg->vlanid >= 4096){
            arg->vlanid = 10;
        }
        // ip address range
        LOG("ip_range(%d)\n", arg->ip_range);
        if (arg->ip_range <= 0 || arg->ip_range >= 65535){
            arg->ip_range = 65535;
        }
        // port range
        LOG("port_range(%d)\n", arg->port_range);
        if (arg->port_range <= 0 || arg->port_range >= 32768){
            arg->port_range = 0;
        }
    }else{
        if (!tnum){
            ERR("thread num is invalid");
            usage();
        }
    }
    // fixed, count of thread
    SET_THREAD_NUM((unsigned) tnum);
    // set to thread structure.
    for(n = 0;n < THREAD_NUM();n++){
        if (ref_thread_arg(n)!=NULL){
            pkt_ptr pkt = &ref_thread_arg(n)->pkt;
            gtpu_header_ptr gtpuh = (gtpu_header_ptr)pkt->body; 
            ref_thread_arg(n)->tx_rate   = tx_rate;
            ref_thread_arg(n)->burst     = burst;
            pkt->size  = psize;
            memcpy(&ref_thread_arg(n)->dst_mac,   &dstmac, ETHER_ADDR_LEN);
            memcpy(&ref_thread_arg(n)->src_mac,   &srcmac, ETHER_ADDR_LEN);
            memcpy(&ref_thread_arg(n)->tx_period, &tx_period, sizeof(tx_period));
            // 198.18.00.100 - 
            // set ether packet
            pkt->eh.ether_type = htons(ETHERTYPE_VLAN);
            pkt->vlanid = htons(arg->vlanid);
#ifdef __IPV6__
            pkt->etype = htons(ETHERTYPE_IPV6);
#else
            pkt->etype = htons(ETHERTYPE_IP);
#endif
            memcpy(&pkt->eh.ether_dhost, &dstmac, ETHER_ADDR_LEN);
            memcpy(&pkt->eh.ether_shost, &srcmac, ETHER_ADDR_LEN);
#ifdef __IPV6__

            pkt->ip.ip6_plen    = htons(psize-14-4-40);
            pkt->ip.ip6_hops    = IPV6_DEFHLIM;
            pkt->ip.ip6_nxt     = IPPROTO_UDP;
            pkt->ip.ip6_flow    = htonl(TEST_ECN_STRS<<20);
            pkt->ip.ip6_vfc     = IPV6_VERSION;
            uint8_t ip6src[16] = {
                0x00,0x00,0x00,0x00,
                0x0a,0x1e,0x48,0xe9,
                0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,
            };
            uint8_t ip6dst[16] = {
                0x00,0x00,0x00,0x00,
                0xac,0x10,0x0a,0x0a,
                0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,
            };
            memcpy(pkt->ip.ip6_src.s6_addr, ip6src, sizeof(ip6src));
            memcpy(pkt->ip.ip6_dst.s6_addr, ip6dst, sizeof(ip6dst));
            pkt->udp.uh_ulen  = htons(psize-14-4-40);
            gtpuh->length = htons(psize-14-4-40-8-8);
#else
            pkt->ip.ip_v   = IPVERSION;
            pkt->ip.ip_hl  = 5;
            pkt->ip.ip_tos = TEST_ECN_STRS;/*IPTOS_LOWDELAY;*/
            pkt->ip.ip_off = htons(IP_DF);
            pkt->ip.ip_ttl = 64;
            pkt->ip.ip_dst.s_addr = htonl(0x0a1e48e9);
            pkt->ip.ip_src.s_addr = htonl(0xac100a0a);
            pkt->ip.ip_p   = IPPROTO_UDP;
            pkt->ip.ip_sum = 0;
            pkt->ip.ip_len = htons(psize-14-4);
            pkt->ip.ip_sum = wrapsum(checksum(&pkt->ip, sizeof(struct ip), 0));
            pkt->udp.uh_ulen  = htons(psize-14-4-20);
            gtpuh->length = htons(psize-14-4-20-8-8);
#endif
            // udp 
            pkt->udp.uh_sport = htons(2152);
            pkt->udp.uh_dport = htons(2152);

            pkt->udp.uh_sum = 0;
            // gtpu
            gtpuh->u.v1_flags.version = 1;

            LOG("rate: %u/burst: %u/size: %u/dmac: %02x:%02x:%02x:%02x:%02x:%02x",
               ref_thread_arg(n)->tx_rate,ref_thread_arg(n)->burst,pkt->size,
               (char)((char*)&dstmac)[0]&0xff,(char)((char*)&dstmac)[1]&0xff,
               (char)((char*)&dstmac)[2]&0xff,(char)((char*)&dstmac)[3]&0xff,
               (char)((char*)&dstmac)[4]&0xff,(char)((char*)&dstmac)[5]&0xff);
        }
    }
    //
    sa.sa_handler = sigint_h;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        ERR("failed to install ^C handler: %s", strerror(errno));
    }
}
