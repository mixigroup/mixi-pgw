#include "strs.h"
//
void *strs_observer(void *data){
    system_arg_ptr sysarg = (system_arg_ptr)data;
    thread_arg_ptr thread = NULL;
    int n;
    observe_t prev,cur,diff;
    uint64_t pps;
    
    LOG("start.strs_observer(%d)", THREAD_NUM());
    bzero(&prev,sizeof(prev));
    bzero(&cur,sizeof(cur));
    bzero(&diff,sizeof(diff));
    gettimeofday(&prev.tm, NULL);
    // observer thread also continues until end of first thread.
    while(!ref_thread_arg(0)->cancel){
        uint64_t pps, usec;
        char b1[40],b2[40],b3[40],b4[70],b5[70];
        // reporting interval(msec)
        usec = wait_for_next_report(&prev.tm, &cur.tm, 1000);
        if (usec < 10000){
            continue;
        }
        cur.pkts=cur.bytes=cur.events=cur.drop=cur.min_space = 0;
        // aggregate in child threads.
        for(n = 0;n < THREAD_NUM();n++){
            if (ref_thread_arg(n)!=NULL){
                cur.pkts  += ref_thread_arg(n)->pkts;
                cur.bytes += ref_thread_arg(n)->bytes;
                cur.drop  += ref_thread_arg(n)->recverr;
            }
        }
        diff.pkts  = (cur.pkts - prev.pkts);
        diff.bytes = (cur.bytes - prev.bytes);
        diff.drop  = (cur.drop  - prev.drop);
        // report
        pps = (diff.pkts*1000000+usec/2)/usec; 
        LOG("%spps %s(%spkts %sbps in %llu usec ecn %s)",
            norm(b1, pps), b5,
            norm(b2, (double)diff.pkts),
            norm(b3, (double)diff.bytes*8),
            (unsigned long long)usec,
            norm(b4, (double)diff.drop)
            );
        if (diff.drop != 0){
          LOG("%s:err/%lf",norm(b1, (double)diff.drop), (double)diff.drop);
        }
        memcpy(&prev, &cur, sizeof(cur));
    }

    return(NULL);
}
