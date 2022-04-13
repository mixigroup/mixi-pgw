//
// Created by mixi on 2017/04/28.
//

#ifndef MIXIPGW_TOOLS_MOD_SGW_DEF_H
#define MIXIPGW_TOOLS_MOD_SGW_DEF_H

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/misc.hpp"
#include "lib/buffer.hpp"
#include "lib/process.hpp"
#include "lib/interface/arch_interface.hpp"
#include "lib/const/link.h"
#include "lib/const/counter.h"
#include "lib/const/bfd.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib_db/mysql.hpp"
#include "lib_srv/server.hpp"
#include "lib_srv/bfd.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <stdexcept>
#include <memory>
#include <functional>
#include <vector>
#include <string>

#ifndef __APPLE__
#include <ncurses.h>
#else
#include <ncurses.h>
#endif



using namespace MIXIPGW_TOOLS;


#define IMSI_BASE   (310149010002000)
#define MSISDN_BASE (819010005000)
#define THRED_CNT   (4)
#define RTHRED_CNT  (2)
#define LEVEL_MAX   (18)


typedef enum SGWSIM_{
    DC = 0,
    BFD_RECV,
    GTPC_RECV,
    GTPU_RECV,
    GTPU_SEND,
    GTPC_SEND_CREATE_SESSION,
    GTPC_SEND_DELETE_SESSION,
    GTPC_SEND_MODIFY_BEARER,
    GTPC_SEND_DELETE_BEARER,
    GTPC_SEND_RESUME_NOTIFICATION,
    GTPC_SEND_SUSPEND_NOTIFICATION,
    GTPU_SEND_ECHO,
    GTPC_SEND_ECHO,
    MAX
}SGWSIM;


class HostPort{
public:
    HostPort():host_(""),mcast_(""),port_(0){}
    HostPort(const std::string& host,const std::string& mcast, unsigned int port):host_(host), mcast_(mcast), port_(port){}
    HostPort(const HostPort& cp){
        host_ = cp.host_;
        port_ = cp.port_;
        mcast_= cp.mcast_;
    }
public:
    std::string host_;
    std::string mcast_;
    unsigned int port_;
};

class SgwSim{
public:
    enum{
        RET, TYPE, LEVEL,
        MAX
    };
public:
    SgwSim(const char*);
    virtual ~SgwSim(){ }
public:
    void BfdRecv(void*);
    void GtpcRecv(void*);
    void GtpuRecv(void*);
    void GtpuSend(void*);
    void SendGtpuData(void*,void*,void*);
    //
    void GtpcSendCreateSession(void*);
    void SendCreateSession(void*,void*,void*);
    //
    void GtpcSendEcho(void*);
    void SendEchoC(void*,void*,void*);
    void GtpuSendEcho(void*);
    void SendEchoU(void*,void*,void*);
    //
    void GtpcSendDeleteSession(void*);
    void SendDeleteSession(void*,void*,void*);
    //
    void GtpcSendModifyBearer(void*);
    void SendModifyBearer(void*,void*,void*);
    //
    void GtpcSendDeleteBearer(void*);
    void SendDeleteBearer(void*,void*,void*);
    //
    void GtpcSendSuspendNotification(void*);
    void SendSuspendNotification(void*,void*,void*);
    //
    void GtpcSendResumeNotification(void*);
    void SendResumeNotification(void*,void*,void*);
public:
    int Input(const char* );
    void Init(int);
    uint64_t WaitForInterval(struct timeval *, struct timeval *, int );
    std::tuple<int,int,int> PrintMenu(int,int,const std::string text[]);
private:
    SgwSim(){ }
private:
    std::string cfg_;
    HostPort    pgw_gtpc_;
    HostPort    pgw_gtpu_;
    HostPort    sgw_emuc_;
    HostPort    sgw_emuu_;
    HostPort    sgw_database_;
    HostPort    sgw_bfd_;
    HostPort    sgw_sndrt_;
    //
    uint16_t    dbport_;
    std::string dbhost_;
    std::string dbuser_;
    std::string dbpswd_;
    std::string dbinst_;
};

class ThreadPrm{
public:
    ThreadPrm(int cpuid, SGWSIM type,int offset, int limit):
              cpuid_(cpuid),level_(0),delay_(0),type_(type),stat_(0),offset_(offset),limit_(limit),current_(0),pkts_(0),bytes_(0),checked_(0),batch_(0),pkts_per_type_(0){
        bzero(&tid_,sizeof(tid_));
        bzero(pkts_calc_, sizeof(pkts_calc_));
        pthread_mutex_init(&mtx_, NULL);
    }
    virtual ~ThreadPrm(){
        Module::ABORT_INCR();
        pthread_join(tid_,NULL);
    }
    void Start(std::function<void*(void*)> func){
        func_ = func;
        pthread_create(&tid_, NULL, ThreadPrm::ThreadFunc, this);
    }
    void SetLevel(int type, int level){
        if (type_ == type){
            level_ = level;
            batch_ = 0;
            if (level_ > 9 && level_ < LEVEL_MAX){
                switch(level_){
                    case 10:    delay_ = 256; break;
                    case 11:    delay_ = 192; break;
                    case 12:    delay_ = 128; break;
                    case 13:    delay_ = 32; break;
                    case 14:    delay_ = 8; break;
                    case 15:    delay_ = 4; batch_ = 2; break;
                    case 16:    delay_ = 4; batch_ = 4; break;
                    case 17:    delay_ = 1; batch_ = 0; break;
                }
            }else if (level_ > (LEVEL_MAX-1)){
                delay_ = (uint64_t)-1;
            }else{
                delay_ = (1000 << (9-level_));
            }
            Logger::LOGINF("this[set:%p]LEVEL:%u/Delay:%u/Batch:%u", this, level_, delay_, batch_);
        }
    }
    void SetStat(int stat){
        stat_ = stat;
    }
    void AddPacketCalc(SGWSIM t, uint64_t a){
        pthread_mutex_lock(&mtx_);
        pkts_calc_[t] += a;
        pthread_mutex_unlock(&mtx_);
    }
    void AddPacketPerType(uint64_t a){
        pthread_mutex_lock(&mtx_);
        pkts_per_type_ += a;
        pthread_mutex_unlock(&mtx_);
    }
    void AddPacket(uint64_t a){
        pthread_mutex_lock(&mtx_);
        pkts_ += a;
        pthread_mutex_unlock(&mtx_);
    }
    void AddBytes(uint64_t a){
        pthread_mutex_lock(&mtx_);
        bytes_ += a;
        pthread_mutex_unlock(&mtx_);
    }
    uint64_t GetPacket(void){
        uint64_t val = 0;
        pthread_mutex_lock(&mtx_);
        val = pkts_;
        pthread_mutex_unlock(&mtx_);
        return(val);
    }
    uint64_t GetBytes(void){
        uint64_t val = 0;
        pthread_mutex_lock(&mtx_);
        val = bytes_;
        pthread_mutex_unlock(&mtx_);
        return(val);
    }
    uint64_t GetPacketPerType(void){
        uint64_t val = 0;
        pthread_mutex_lock(&mtx_);
        val = pkts_per_type_;
        pthread_mutex_unlock(&mtx_);
        return(val);
    }
    uint64_t GetPacketCalc(SGWSIM t){
        uint64_t val = 0;
        pthread_mutex_lock(&mtx_);
        val = pkts_calc_[t];
        pthread_mutex_unlock(&mtx_);
        return(val);
    }

    uint64_t GetChecked(void){ return(checked_); }
    uint64_t Batch(void){ return(batch_); }
    uint64_t Delay(void){ return(delay_==0?1000000:delay_); }
    void DelayNext(void){
        if (stat_){
            if (Delay() == (uint64_t)-1){
                ;; // busy loop.
            }else{
                usleep(Delay());
            }
        }else{
            // delay 1000 ms, until execution flag is tuned ON.
            sleep(1);
        }
    }
private:
    static void* ThreadFunc(void* u){
        auto p = (ThreadPrm*)u;
        return(p->func_(p));
    }
public:
    int         cpuid_;
    int         level_;
    int         stat_;
    int         offset_;
    int         limit_;
    int         current_;
    SGWSIM      type_;
    uint64_t    delay_;
    uint64_t    batch_;
    uint64_t    checked_;
    pthread_t   tid_;
    pthread_mutex_t mtx_;
    int         fd_;
    std::function<void*(void*)>  func_;
private:
    uint64_t    pkts_;
    uint64_t    bytes_;
    uint64_t    pkts_per_type_;
    uint64_t    pkts_calc_[SGWSIM::MAX];
private:
    ThreadPrm(){}
};
#define __MULTICAST__
#undef __MULTICAST__
// sgw (gtpc) server event.
class SgwGtpcServer:public Event{
public:
    SgwGtpcServer(const char*,const char*,int,void*);
    virtual ~SgwGtpcServer();
public:
    virtual int on_hook_gtpc(ConnInterface*,void*,size_t);
    virtual const char* server_mcastif(void);
private:
    std::string maddr_;
    uint64_t    pkts_;
    uint64_t    bytes_;
    uint64_t    checked_;
    uint64_t    counter_;
}; // class SgwGtpcServer

// sgw (gtpu) server event.
class SgwGtpuServer:public Event{
public:
    SgwGtpuServer(const char*,const char*,int,void*);
    virtual ~SgwGtpuServer();
public:
    virtual int on_hook_gtpc(ConnInterface*,void*,size_t);
    virtual const char* server_mcastif(void);
private:
    std::string maddr_;
    uint64_t    pkts_;
    uint64_t    bytes_;
    uint64_t    checked_;
    uint64_t    counter_;
}; // class SgwGtpuServer



// bfd server event.
class BfdServer:public Event{
public:
    BfdServer(const char*,const char*,int,void*);
    virtual ~BfdServer();
public:
    virtual int on_hook_gtpc(ConnInterface*,void*,size_t);
    virtual const char* server_mcastif(void);
private:
    std::string maddr_;
}; // class BfdServer


#endif //MIXIPGW_TOOLS_MOD_SGW_DEF_H
