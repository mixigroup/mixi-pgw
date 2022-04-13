//
// Created by mixi on 2017/05/17.
//
#include "../mixipgw_mod_sgw_def.hpp"
#define SESSIONMGR_DELAY	(1000000)

#include <boost/thread/thread.hpp>
 
BfdServer::BfdServer(const char* bindif,const char* mcastif,int port, void* tp){
    tp_ = tp;
    port_ = port;
    bindif_ = bindif?bindif:"";
    maddr_  = mcastif?mcastif:"";
}
BfdServer::~BfdServer(){}

const char* BfdServer::server_mcastif(void){
#ifdef __MULTICAST__
    return(maddr_.c_str());
#else
    return(NULL);
#endif
}
int BfdServer::on_hook_gtpc(ConnInterface* pcon,void* pkt,size_t len){
    bfd_ptr bfd =  (bfd_ptr)pkt;
    BfdSessionManager* psess = (BfdSessionManager*)tp_;
    uint8_t ipv[16] = {0};
    unsigned  ipv_len(16);
    std::string host,lhost;
    uint16_t    port,lport;
    pcon->address(host,port,lhost,lport);
    //
    if (Misc::GetIpv(host.c_str(),ipv, &ipv_len) != RETIPV4){
        return(RETERR);
    }
    //
    return(psess->on_bfd_recieve(htonl(*(uint32_t*)&ipv), port_,pkt , len, this));
}
void SgwSim::BfdRecv(void* arg){
    auto tp = (ThreadPrm*)arg;
    auto lhost  = sgw_bfd_.host_;
    auto lmcast = sgw_bfd_.mcast_;
    auto lport  = sgw_bfd_.port_;
    std::auto_ptr<boost::asio::io_service> ios(new boost::asio::io_service());
    boost::thread_group th;
    Logger::LOGINF("SgwSim::BfdRecv.(%s:%s:%d).", lhost.c_str(), lmcast.c_str(),lport);

    {   boost::asio::io_service::work work(*ios.get());
        th.create_thread([&ios,lhost,lmcast, lport, &tp]{
            BfdSessionManager  sess(1000000);
            BfdServer   ev(lhost.c_str(),lmcast.c_str(),lport, &sess);
            Server      srv(*ios.get(), &ev);
            ios.get()->run();
        });
    }

    while(!Module::ABORT()){ sleep(1); }
    ios.get()->stop();
    th.join_all();
}

