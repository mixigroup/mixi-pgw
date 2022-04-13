//
// Created by mixi on 2017/04/28.
//

#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>

SgwGtpcServer::SgwGtpcServer(const char* bindif,const char* mcastif,int port, void* tp){
    Logger::LOGINF("SgwGtpcServer");
    tp_ = tp;
    port_ = port;
    bindif_ = bindif?bindif:"";
    maddr_  = mcastif?mcastif:"";
    pkts_   = 0;
    bytes_  = 0;
    counter_= 0;
}
SgwGtpcServer::~SgwGtpcServer(){}

const char* SgwGtpcServer::server_mcastif(void){
#ifdef __MULTICAST__
    return(maddr_.c_str());
#else
    return(NULL);
#endif
}


int SgwGtpcServer::on_hook_gtpc(ConnInterface* pcon,void* pkt,size_t len){
    ThreadPrm* tp = (ThreadPrm*)tp_;

    // gtpu :echo
    if (((gtpc_header_ptr)pkt)->type == GTPC_ECHO_RES){
        tp->AddPacketCalc(SGWSIM::GTPC_SEND_ECHO,1);
        return(RETOK);
//  }else if (((gtpc_header_ptr)pkt)->type == GTPC_CREATE_SESSION_RES){
//      tp->AddPacketCalc(SGWSIM::GTPC_SEND_CREATE_SESSION,1);
    }else if (((gtpc_header_ptr)pkt)->type == GTPC_DELETE_SESSION_RES){
        tp->AddPacketCalc(SGWSIM::GTPC_SEND_DELETE_SESSION,1);
        return(RETOK);
    }else if (((gtpc_header_ptr)pkt)->type == GTPC_DELETE_BEARER_RES){
        tp->AddPacketCalc(SGWSIM::GTPC_SEND_DELETE_BEARER,1);
        return(RETOK);
    }else if (((gtpc_header_ptr)pkt)->type == GTPC_MODIFY_BEARER_RES){
        tp->AddPacketCalc(SGWSIM::GTPC_SEND_MODIFY_BEARER,1);
        return(RETOK);
    }else if (((gtpc_header_ptr)pkt)->type == GTPC_RESUME_ACK){
        tp->AddPacketCalc(SGWSIM::GTPC_SEND_RESUME_NOTIFICATION,1);
        return(RETOK);
    }else if (((gtpc_header_ptr)pkt)->type == GTPC_SUSPEND_ACK){
        tp->AddPacketCalc(SGWSIM::GTPC_SEND_SUSPEND_NOTIFICATION,1);
        return(RETOK);
    }
    std::auto_ptr<GtpcPkt> req(GtpcPkt::attach(pkt, len));
    if (req.get() == NULL){
        Logger::LOGERR("SgwGtpcServer::on_hook_gtpc(%p %u)", pkt, len);
        return(RETERR);
    }
    // items in gtpc packet.
    std::auto_ptr<Fteid>            fteid(req.get()->find<Fteid>());
    std::auto_ptr<BearerContext>    bctx(req.get()->find<BearerContext>());
    if (!fteid.get() || !bctx.get()){
        checked_ += 1;
        return(RETERR);
    }
    GtpcPkt* pbctx = bctx.get()->child();
    std::auto_ptr<Fteid>            fteidu(pbctx->find<Fteid>());
    if (!fteidu.get()){
        checked_ += 1;
        return(RETERR);
    }
    auto gtpc_fteid = fteid.get()->teid();
    auto gtpu_fteid = fteidu.get()->teid();

    if (gtpc_fteid != gtpu_fteid){
        checked_ += 1;
    }
    pkts_  += 1;
    bytes_ += (uint64_t)len+(14+4+20+8/* ether + vlan + ip + udp */);
    if (counter_ ++ > 100){
        tp->AddPacket(pkts_);
        tp->AddBytes(bytes_);
        // copy up to wide area vriables every 100 counting.
        counter_ = 0;
        pkts_ = 0;
        bytes_ = 0;
    }
    //
    return(RETOK);
}


void SgwSim::GtpcRecv(void* arg){
    auto tp = (ThreadPrm*)arg;
    auto lhost  = sgw_emuc_.host_;
    auto lmcast = sgw_emuc_.mcast_;
    auto lport  = sgw_emuc_.port_ + tp->offset_;

    std::auto_ptr<boost::asio::io_service> ios(new boost::asio::io_service());
    boost::thread_group th;
    //

    // Sgw Emu : start BSD-Socket server
    {   boost::asio::io_service::work work(*ios.get());
        th.create_thread([&ios,lhost,lmcast,lport,&tp]{
            Misc::SetAffinity(tp->cpuid_);
            // force halt.....
            // GTPC recieve function is initialized in last,
            // allowing both GTPU/GTPC to receive.
            sleep(1);
            SgwGtpcServer   ev(lhost.c_str(),lmcast.c_str(),lport, tp);
            Server          srv(*ios.get(), &ev);
            Logger::LOGINF("SgwSim::GtpcRecv..(%s:%s:%d:%d).", lhost.c_str(), lmcast.c_str(),lport, tp->cpuid_);
            ios.get()->run();
        });
        while(!Module::ABORT()){ sleep(1); }
        ios.get()->stop();
        th.join_all();
        Logger::LOGINF("thread end(GtpcRecv)");
    }
}
