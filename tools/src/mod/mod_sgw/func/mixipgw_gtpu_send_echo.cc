//
// Created by mixi on 2017/04/28.
//
#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/const/gtpu.h"

using namespace MIXIPGW_TOOLS;

void SgwSim::GtpuSendEcho(void* arg){
    auto tp     = (ThreadPrm*)arg;
    auto lhost  = sgw_emuu_.host_;
    auto lport  = sgw_emuu_.port_;
    char sbuf[2048] = {0};
    Logger::LOGINF("SgwSim::GtpuSendEcho(this: %p/type:%d)", tp, tp->type_);
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
        // keep sending GTPU Echo Request
        //  every delay interval.
        SendEchoU(tp, &client, sbuf);
        sleep(1);
    }
}


void SgwSim::SendEchoU(void* arg, void* client, void* sbuf){
    auto tp   = (ThreadPrm*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto host   = pgw_gtpu_.host_;
    auto port   = pgw_gtpu_.port_;
    size_t rlen = 0;
    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);

    if (tp->current_++ > 0xffff){
        tp->current_ = 0;
    }
    auto echo = (gtpu_header_ptr)sbuf;
    bzero(echo, sizeof(*echo));
    echo->type = GTPU_ECHO_REQ;
    echo->length = htons(sizeof(*echo) - 4);
    echo->tid    = 3;
    echo->u.v1_flags.sequence = 1;
    echo->u.v1_flags.version = GTPU_VERSION_1;
    echo->u.v1_flags.proto   = GTPU_PROTO_GTP;
    rlen = sizeof(*echo)+4;

    // to control plane.
    auto r = pcli->send_to(boost::asio::buffer(sbuf, rlen), remote_endp);
    if (r != rlen){
        throw std::runtime_error("failed.send_to(udp socket)");
    }
    tp->AddPacketPerType(1);
}

