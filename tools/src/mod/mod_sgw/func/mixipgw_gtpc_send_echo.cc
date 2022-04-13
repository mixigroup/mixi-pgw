//
// Created by mixi on 2017/04/28.
//
#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/packet/gtpc.hpp"

using namespace MIXIPGW_TOOLS;

void SgwSim::GtpcSendEcho(void* arg){
    static int counter = 0;
    auto tp     = (ThreadPrm*)arg;
    auto lhost  = sgw_emuc_.host_;
    auto lport  = sgw_emuc_.port_;
    char sbuf[2048] = {0};
    counter ++;
    Logger::LOGINF("SgwSim::GtpcSendEcho(this: %p/type:%d/counter:%d)", tp, tp->type_,counter);
    // assign CPU resource
    Misc::SetAffinity(tp->cpuid_);
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
    //
    while(!Module::ABORT()){
        // gtpc echo every 1 sec.
        SendEchoC(tp, &client, sbuf);
        sleep(2);
    }
}


void SgwSim::SendEchoC(void* arg, void* client, void* sbuf){
    auto tp   = (ThreadPrm*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto lhost  = sgw_emuc_.host_;
    auto lhostu = sgw_emuc_.host_;
    auto host   = pgw_gtpc_.host_;
    auto port   = pgw_gtpc_.port_;
    size_t rlen = 0;
    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);

    if (tp->current_++ > 0xffff){
        tp->current_ = 0;
    }
    
    auto echo = (gtpc_header_ptr)sbuf;
    bzero(echo, sizeof(*echo));
    echo->type = GTPC_ECHO_REQ;
    echo->q.seqno = tp->current_;
    echo->length = htons(sizeof(*echo) - 8);
    echo->c.v2_flags.version = GTPC_VERSION_2;
    echo->c.v2_flags.piggy = GTPC_PIGGY_OFF;
    echo->c.v2_flags.spare = 0;
    echo->c.v2_flags.teid = GTPC_TEID_OFF;
    echo->t.sq.seqno = tp->current_;
    rlen = sizeof(*echo) - 4;

    // to control plane.
    auto r = pcli->send_to(boost::asio::buffer(sbuf, rlen), remote_endp);
    if (r != rlen){
        throw std::runtime_error("failed.send_to(udp socket)");
    }
    tp->AddPacketPerType(1);
}

