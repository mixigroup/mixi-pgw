//
// Created by mixi on 2017/05/19.
//


#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/const/gtpu.h"


SgwGtpuServer::SgwGtpuServer(const char* bindif,const char* mcastif,int port, void* tp){
    Logger::LOGINF("SgwGtpuServer");
    tp_ = tp;
    port_ = port;
    bindif_ = bindif?bindif:"";
    maddr_  = mcastif?mcastif:"";
    pkts_   = 0;
    bytes_  = 0;
    counter_= 0;
}
SgwGtpuServer::~SgwGtpuServer(){}
const char* SgwGtpuServer::server_mcastif(void){
#ifdef __MULTICAST__
    return(maddr_.c_str());
#else
    return(NULL);
#endif
}

int SgwGtpuServer::on_hook_gtpc(ConnInterface* pcon,void* pkt,size_t len){
    ThreadPrm* tp = (ThreadPrm*)tp_;
//  Logger::LOGINF("SgwGtpuServer::on_hook_gtpc(%u)",(unsigned)len);
    pkts_  += 1;
    bytes_ += (uint64_t)len+(14+4+20+8/* ether + vlan + ip + udp */);

    // gtpu :echo
    if (((gtpu_header_ptr)pkt)->type == GTPU_ECHO_RES){
        tp->AddPacketCalc(SGWSIM::GTPU_SEND_ECHO,1);
    }
    if (counter_++>100){
        tp->AddPacket(pkts_);
        tp->AddBytes(bytes_);
        // copy up to wide area variables every 100 counting.
        counter_ = 0;
        pkts_ = 0;
        bytes_ = 0;
    }
    //
    return(RETOK);
}


void SgwSim::GtpuRecv(void* arg){
    auto tp = (ThreadPrm*)arg;
    auto lhost  = sgw_emuu_.host_;
    auto lmcast = sgw_emuu_.mcast_;
    auto lport  = sgw_emuu_.port_;
    std::auto_ptr<boost::asio::io_service> ios(new boost::asio::io_service());
    boost::thread_group th;

    {   boost::asio::io_service::work work(*ios.get());
        th.create_thread([&ios,lhost,lmcast,lport,&tp]{
            Misc::SetAffinity(tp->cpuid_);
            sleep(1);
            SgwGtpuServer   ev(lhost.c_str(),lmcast.c_str(),lport, tp);
            Server          srv(*ios.get(), &ev);
            Logger::LOGINF("SgwSim::GtpuRecv..(%s:%s:%d).", lhost.c_str(),lmcast.c_str(),lport);
            ios.get()->run();
        });
        while(!Module::ABORT()){ sleep(1); }
        ios.get()->stop();
        th.join_all();
        Logger::LOGINF("thread end(GtpuRecv)");
    }
}
