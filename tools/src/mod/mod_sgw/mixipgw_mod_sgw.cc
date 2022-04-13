//
// Created by mixi on 2017/04/28.
//

#include "mixipgw_mod_sgw_def.hpp"
/*
 * sgw sim module.
 *
 */
using namespace MIXIPGW_TOOLS;

typedef std::vector<ThreadPrm*>             TPVEC;
typedef std::vector<ThreadPrm*>::iterator   TPITR;

typedef struct observe {
    uint64_t   pkts,bytes,checked,pkts_send,pkts_recv;
    struct timeval tm;
}observe_t,*observe_ptr;

static std::string observed_text[SGWSIM::MAX];
static pthread_mutex_t observed_mutex;
static TPVEC ope_threads;
//
int main(int argc, char **argv){
    if (argc != 2){
        fprintf(stderr, "invalid arguments.(argc)\n");
        exit(0);
    }
    // init process.
    Module::Init(NULL);
    Module::VERBOSE_CLR();
    Logger::Init(basename(argv[0]),NULL,NULL);

    Logger::LOGINF("Start Sgw simulate.");
    Misc::SetModuleCfg(argv[1]);
    //
    SgwSim sgw(argv[1]);

    auto tnum = sgw.Input("number of contract level[n * 10000]?\n$$");
    tnum *= 10000;
    //
    auto s = sgw.Input("start?[0:start/0!=:quit]\n$$");
    if (s){
        fprintf(stderr, "quit (%d) \n", s);
        Module::ABORT_INCR();
    }
    //
    sgw.Init(tnum);
    auto limit = (tnum/THRED_CNT);


    for(auto n = 0;n < THRED_CNT;n ++){ ope_threads.push_back(new ThreadPrm(28+n,SGWSIM::GTPU_SEND,(n*limit)+1,limit)); }
    for(auto n = 0;n < THRED_CNT;n ++){ ope_threads.push_back(new ThreadPrm(14+n,SGWSIM::GTPC_SEND_CREATE_SESSION,(n*limit)+1,limit)); }
    for(auto n = 0;n < RTHRED_CNT;n++){ ope_threads.push_back(new ThreadPrm(45+n,SGWSIM::GTPC_RECV,n,0)); }
    ope_threads.push_back(new ThreadPrm(11,SGWSIM::GTPU_RECV,0,0));
    ope_threads.push_back(new ThreadPrm(12,SGWSIM::BFD_RECV,0,0));

    //
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPC_SEND_ECHO,0,0));
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPU_SEND_ECHO,0,0));
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPC_SEND_DELETE_SESSION,1,tnum));
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPC_SEND_MODIFY_BEARER,1,tnum));
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPC_SEND_DELETE_BEARER,1,tnum));
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPC_SEND_RESUME_NOTIFICATION,1,tnum));
    ope_threads.push_back(new ThreadPrm(4,SGWSIM::GTPC_SEND_SUSPEND_NOTIFICATION,1,tnum));
    //
    for(TPITR it = ope_threads.begin();it != ope_threads.end();++it){
        Logger::LOGINF("Start Before(%p) type[%d]", (*it),(*it)->type_);
        (*it)->Start([&sgw](void* arg){
            auto p = (ThreadPrm*)arg;
            Logger::LOGINF("Start lamda(%p) type[%d]", p, p->type_);
            switch(p->type_){
                case SGWSIM::GTPC_RECV: sgw.GtpcRecv(p); break;
                case SGWSIM::GTPU_SEND: sgw.GtpuSend(p); break;
                case SGWSIM::GTPC_SEND_CREATE_SESSION: sgw.GtpcSendCreateSession(p); break;
                case SGWSIM::GTPC_SEND_DELETE_SESSION: sgw.GtpcSendDeleteSession(p); break;
                case SGWSIM::BFD_RECV: sgw.BfdRecv(p); break;
                case SGWSIM::GTPU_RECV: sgw.GtpuRecv(p); break;
                case SGWSIM::GTPC_SEND_ECHO: sgw.GtpcSendEcho(p); break;
                case SGWSIM::GTPU_SEND_ECHO: sgw.GtpuSendEcho(p); break;
                case SGWSIM::GTPC_SEND_MODIFY_BEARER: sgw.GtpcSendModifyBearer(p); break;
                case SGWSIM::GTPC_SEND_DELETE_BEARER: sgw.GtpcSendDeleteBearer(p); break;
                case SGWSIM::GTPC_SEND_RESUME_NOTIFICATION: sgw.GtpcSendResumeNotification(p); break;
                case SGWSIM::GTPC_SEND_SUSPEND_NOTIFICATION: sgw.GtpcSendSuspendNotification(p); break;
                case SGWSIM::DC:
                default:
                    break;
            }
            return((void*)NULL);
        });
    }


    // notify start telegram to all threads.
    for(TPITR it = ope_threads.begin();it != ope_threads.end();++it){
        (*it)->SetStat(1);
    }
    // wait for thread started.
    sleep(1);
    int level_by_type[SGWSIM::MAX] = {0};
    {
        // aggregate performance statistics at thread of observer.
        ThreadPrm   observer(-1,SGWSIM::DC,0,0);
        observer.Start([&sgw](void* arg){
            observe_t prev[SGWSIM::MAX],cur[SGWSIM::MAX],diff[SGWSIM::MAX];
            //
            bzero(&prev,sizeof(prev));
            bzero(&cur,sizeof(cur));
            bzero(&diff,sizeof(diff));
            gettimeofday(&prev[2].tm, NULL);
            //
            while(!Module::ABORT()){
                auto usec = sgw.WaitForInterval(&prev[2].tm, &cur[2].tm, 1000*1);
                if (usec < 1000000*1){
                    continue;
                }
                // aggregate at child threads.
                for(auto n = 0;n < SGWSIM::MAX;n++){ cur[n].pkts=cur[n].bytes=cur[n].checked=cur[n].pkts_send=cur[n].pkts_recv=0; }
                //
                for(TPITR it = ope_threads.begin();it != ope_threads.end();++it){
#define TYPE_ADD(i,t,t2,c) { if ((*i)->type_ == t){ c[t].pkts_send += (*i)->GetPacketPerType();}else if ((*i)->type_ == t2){ c[t].pkts_recv += (*i)->GetPacketCalc(t); }}
                    // summary count of send,receive
                    TYPE_ADD(it,SGWSIM::GTPU_SEND_ECHO,SGWSIM::GTPU_RECV,cur);
                    TYPE_ADD(it,SGWSIM::GTPC_SEND_ECHO,SGWSIM::GTPC_RECV,cur);
                    TYPE_ADD(it,SGWSIM::GTPC_SEND_DELETE_SESSION,SGWSIM::GTPC_RECV,cur);
                    TYPE_ADD(it,SGWSIM::GTPC_SEND_MODIFY_BEARER,SGWSIM::GTPC_RECV,cur);
                    TYPE_ADD(it,SGWSIM::GTPC_SEND_DELETE_BEARER,SGWSIM::GTPC_RECV,cur);
                    TYPE_ADD(it,SGWSIM::GTPC_SEND_RESUME_NOTIFICATION,SGWSIM::GTPC_RECV,cur);
                    TYPE_ADD(it,SGWSIM::GTPC_SEND_SUSPEND_NOTIFICATION,SGWSIM::GTPC_RECV,cur);
                    //
                    if ((*it)->type_ < SGWSIM::MAX){
                        cur[(*it)->type_].pkts  += (*it)->GetPacket();
                        cur[(*it)->type_].bytes += (*it)->GetBytes();
                        cur[(*it)->type_].checked += (*it)->GetChecked();
                    }
                }
                for(auto n = 0;n < SGWSIM::MAX;n++){
                    diff[n].pkts  = (cur[n].pkts - prev[n].pkts);
                    diff[n].bytes = (cur[n].bytes - prev[n].bytes);
                    diff[n].checked = (cur[n].checked - prev[n].checked);
                }
                // report
                for(auto n = 0;n < SGWSIM::MAX;n++){
                    auto pps = (diff[n].pkts*1000000+usec/2)/usec;
                    char bf[1024] = {0};
                    char b1[40],b2[40],b3[40],b4[70];
                    if (n == SGWSIM::GTPC_SEND_ECHO           || n == SGWSIM::GTPU_SEND_ECHO ||
                        n == SGWSIM::GTPC_SEND_DELETE_SESSION || n == SGWSIM::GTPC_SEND_MODIFY_BEARER ||
                        n == SGWSIM::GTPC_SEND_DELETE_BEARER  || n == SGWSIM::GTPC_SEND_RESUME_NOTIFICATION ||
                        n == SGWSIM::GTPC_SEND_SUSPEND_NOTIFICATION){
                        snprintf(
                                bf,sizeof(bf)-1,
                                "send : %s/recv : %s",
                                Misc::Norm(b2, cur[n].pkts_send),
                                Misc::Norm(b3, cur[n].pkts_recv));
                    }else{
                        snprintf(
                                bf,sizeof(bf)-1,
                                "%spps %s(%s err/%sbps)",
                                Misc::Norm(b1, pps), b4,
                                Misc::Norm(b2, (double)diff[n].checked),
                                Misc::Norm(b3, (double)diff[n].bytes*8));
                    }
                    //
                    pthread_mutex_lock(&observed_mutex);
                    observed_text[n].assign(bf);
                    pthread_mutex_unlock(&observed_mutex);
                }
                //
                for(auto n = 0;n < SGWSIM::MAX;n++){ memcpy(&prev[n], &cur[n], sizeof(cur[0])); }
            }
            Logger::LOGINF("observer complete.");
            return((void*)NULL);
        });
        // ----------------------------
        // wait for quit flag
        while(!Module::ABORT()){
            std::string observed_text_copy[SGWSIM::MAX];
            // following observed_xxxx locs are not lock with traffic aggregation counter threads.
            // those are locks with buffer stringified after summarization in counter thread. 
            // --
            // No performance degradation due to aggregation processing
            pthread_mutex_lock(&observed_mutex);
            for(auto n = 0;n < SGWSIM::MAX;n++){ observed_text_copy[n] = observed_text[n]; }
            pthread_mutex_unlock(&observed_mutex);
            //
            auto ret = sgw.PrintMenu(
                    level_by_type[SGWSIM::GTPC_SEND_CREATE_SESSION],
                    level_by_type[SGWSIM::GTPU_SEND],
                    observed_text_copy);
            if (std::get<SgwSim::RET>(ret) != RETOK){ break; }

            auto type = std::get<SgwSim::TYPE>(ret);
            auto lvl  = std::get<SgwSim::LEVEL>(ret);
            //
            switch(type){
                case SGWSIM::GTPC_SEND_CREATE_SESSION:
                case SGWSIM::GTPU_SEND:
                    // notify change-level event to all threads.
                    for(TPITR it = ope_threads.begin();it != ope_threads.end();++it){
                        (*it)->SetLevel(type, lvl);
                    }
                    // save previous level.
                    level_by_type[type] = lvl;
                    break;
            }
        }
        // ----------------------------
        for(TPITR it = ope_threads.begin();it != ope_threads.end();++it){
            if ((*it)){ delete (*it); }
        }
        ope_threads.clear();
    } // scope for [ThreadPrm   observer]
    // uninitialize
    Logger::Uninit(NULL);
    Module::Uninit(NULL);
    //
    return (0);
}




