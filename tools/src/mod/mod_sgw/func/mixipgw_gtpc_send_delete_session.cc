//
// Created by mixi on 2017/05/30.
//
#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"


using namespace MIXIPGW_TOOLS;

void SgwSim::GtpcSendDeleteSession(void* arg){
    auto tp     = (ThreadPrm*)arg;
    auto lhost  = sgw_emuc_.host_;
    auto lport  = sgw_emuc_.port_;
    char sbuf[2048] = {0};
    Logger::LOGINF("SgwSim::GtpcSendDeleteSession(this: %p/type:%d)", tp, tp->type_);
    // assign CPU resource
    Misc::SetAffinity(tp->cpuid_);
    Logger::LOGINF("SgwSim::GtpcSendDeleteSession(%p,%d)", (void*)pthread_self(), tp->cpuid_);

    // prepare bsd-socket
    boost::asio::io_service         ioservice;
    boost::asio::ip::udp::socket    client(ioservice);
    boost::asio::ip::udp::endpoint  local_endp(boost::asio::ip::address::from_string(lhost), lport);
    // bind local port
    // ip@ttl, gtpc local port config, multiple bind.
    client.open(local_endp.protocol());
    client.set_option(boost::asio::ip::udp::socket::send_buffer_size(4*1024*1024));
    client.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    client.set_option(boost::asio::ip::unicast::hops(255));
    client.bind(local_endp);
    // start position
    tp->current_ = tp->offset_;
    //
    while(!Module::ABORT()){
        SendDeleteSession(tp, &client, sbuf);
        sleep(1);
    }
}


void SgwSim::SendDeleteSession(void* arg, void* client, void* sbuf){
    auto tp   = (ThreadPrm*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto lhost  = sgw_emuc_.host_;
    auto lhostu = sgw_emuu_.host_;
    auto host   = pgw_gtpc_.host_;
    auto port   = pgw_gtpc_.port_;
    size_t rlen = 0;
    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);
    // generate DeleteSessionRequest from offset, limit, current.
    if ((tp->current_++) >= (tp->offset_ + tp->limit_ -1)){
        tp->current_ = tp->offset_;
    }

    // delete session request.
    GtpcPkt req(GTPC_DELETE_SESSION_REQ);
    // LBI
    req.append(Ebi(GTPC_INSTANCE_ORDER_0,(uint8_t)((tp->current_+1)&0xF)));
    auto rbuf = (gtpc_header_ptr)req.ref();
    rlen = req.len();

    rbuf->t.teid = htonl(tp->current_);
    rbuf->q.seqno = tp->current_;
    memcpy(sbuf, rbuf, rlen);
    // to control plane.
    auto r = pcli->send_to(boost::asio::buffer(sbuf, rlen), remote_endp);
    if (r != rlen){
        throw std::runtime_error("failed.send_to(udp socket)");
    }
    tp->AddPacketPerType(1);
}

