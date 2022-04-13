#include "strs.h"
FILE* pcapf_ = NULL;
//
void *strs_send(void *data){
    uint64_t n,sent,cursent,will_send,ttl_sent,ttl_counter;
    thread_arg_ptr  t = (thread_arg_ptr)data;
    thread_arg_t    tcpy;
    int ret;

    struct netmap_ring *txring = NULL;
    struct timespec nexttime = { 0, 0};
    memcpy(&tcpy, t, sizeof(tcpy));
   
    struct pollfd pfd = { .fd = tcpy.fd, .events = POLLOUT };
    struct pkt *pkt = &tcpy.pkt;
    struct netmap_if *nifp = t->nmd->nifp;
    pcapf_ = start_pcap(NULL); 

    // assign CPU to threads
    LOG("start.strs_send, fd %d affinity %d burst  %d", tcpy.fd, tcpy.affinity, tcpy.burst);
    if (setaffinity(tcpy.thread, tcpy.affinity)){
        ERR("failed. setaffinity.");
        goto quit;
    }
    if (!tcpy.burst){
        ERR("failed. burst config.");
        goto quit;
    }
    // store start time.
    clock_gettime(CLOCK_REALTIME_PRECISE, &tcpy.tic);

    sent = 0;
    will_send = 0;
    ttl_sent = 0;
    ttl_counter = 0;
    tcpy.pkts = tcpy.bytes = 0;
    //
    while (!tcpy.cancel && (tcpy.total_send == 0 || sent < tcpy.total_send)) {
        if (will_send <= 0) {
            will_send = tcpy.burst;
            nexttime = timespec_add(nexttime, tcpy.tx_period);
            wait_time(nexttime);
        }
#if 0
        if (ioctl(pfd.fd, NIOCTXSYNC, NULL) < 0) {
            ERR("ioctl error on queue %d: %s", t->id, strerror(errno));
            goto quit;
        }
#endif
        if ((ret = poll(&pfd, 1,200))<=0){
            ERR("poll error or timeout queue %d: %d: %s", ret, tcpy.id, strerror(errno));
            t->cancel = 1;
        }
        if (!(pfd.revents & POLLOUT)){
            LOG("cannot send. need wait..");
            continue;
        }
        // 
        for (n = t->nmd->first_tx_ring; n <= t->nmd->last_tx_ring; n++) {
            uint64_t limit = MIN(tcpy.burst,will_send);
            txring = NETMAP_TXRING(nifp, n);
            if (nm_ring_empty(txring)){
                continue;
            }
            cursent = send_packets(&tcpy, txring, pkt, pkt->size, limit);
//          usleep(1000000);
            //
            sent += cursent;
            tcpy.pkts  = sent;
            tcpy.bytes = (sent*pkt->size);
            will_send -= MIN(will_send, cursent);
            if (will_send <= 0){
                break;
            }
        }
        t->pkts = tcpy.pkts;
        t->bytes = tcpy.bytes;
        tcpy.cancel = t->cancel; 
#if 0
        ttl_sent += (uint64_t)tcpy.pkts;
        ttl_counter++;
        // 16T packets done when 16T packets transmission is complete.
        // if (ttl_counter > (uint64_t)(1024ULL*1024ULL*1024ULL*1024ULL*16ULL)){
        if (ttl_counter > (uint64_t)(1024ULL)){
            LOG("finished. ttl_sent:%llu/pkts:%llu/bytes:%llu  count(%llu)", ttl_sent, tcpy.pkts, tcpy.bytes, ttl_counter);
            break;
        }
#endif
    }
    LOG("flush..");
    // flush for tx queue.
    ret = 0;
    for (n = tcpy.nmd->first_tx_ring; n <= tcpy.nmd->last_tx_ring; n++) {
        txring = NETMAP_TXRING(nifp, n);
        while (!tcpy.cancel && nm_tx_pending(txring)) {
            LOG("pending tx tail %d head %d on ring %p cancel:%d", txring->tail, txring->head, (void*)pthread_self(),tcpy.cancel);
            ioctl(pfd.fd, NIOCTXSYNC, NULL);
            usleep(1);
        }
    }
    clock_gettime(CLOCK_REALTIME_PRECISE, &tcpy.toc);
    LOG("send complete..(%d)", tcpy.id + 1);
    stop_pcap(pcapf_);
quit:
    return(NULL);
}
