//
// Created by mixi on 2017/06/01.
//


#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"


using namespace MIXIPGW_TOOLS;

void SgwSim::GtpcSendDeleteBearer(void* arg){
    auto tp     = (ThreadPrm*)arg;
    auto lhost  = sgw_emuc_.host_;
    auto lport  = sgw_emuc_.port_;
    char sbuf[2048] = {0};
    Logger::LOGINF("SgwSim::GtpcSendDeleteBearer(this: %p/type:%d)", tp, tp->type_);
    // assign CPU resource
    Misc::SetAffinity(tp->cpuid_);
    Logger::LOGINF("SgwSim::GtpcSendDeleteBearer(%p,%d)", (void*)pthread_self(), tp->cpuid_);

    // prepare bsd-socket.
    boost::asio::io_service         ioservice;
    boost::asio::ip::udp::socket    client(ioservice);
    boost::asio::ip::udp::endpoint  local_endp(boost::asio::ip::address::from_string(lhost), lport);
    // bind local port
    // ip@ttl,gtpc local port config , multiple bind.
    client.open(local_endp.protocol());
    client.set_option(boost::asio::ip::udp::socket::send_buffer_size(4*1024*1024));
    client.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    client.set_option(boost::asio::ip::unicast::hops(255));
    client.bind(local_endp);
    // start position.
    tp->current_ = tp->offset_;
    //
    while(!Module::ABORT()){
        SendDeleteBearer(tp, &client, sbuf);
        sleep(1);
    }
}
void SgwSim::SendDeleteBearer(void* arg, void* client, void* sbuf){
    auto tp   = (ThreadPrm*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto lhost  = sgw_emuc_.host_;
    auto lhostu = sgw_emuu_.host_;
    auto host   = pgw_gtpc_.host_;
    auto port   = pgw_gtpc_.port_;
    size_t rlen = 0;
    uint8_t ebi_8(1);

    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);
    // generate DeleteBearerRequest from offset, limit, current.
    if ((tp->current_++) >= (tp->offset_ + tp->limit_ -1)){
        tp->current_ = tp->offset_;
    }
    // delete bearer request.
    GtpcPkt req(GTPC_DELETE_BEARER_REQ);
    // EBI
    req.append(Ebi(GTPC_INSTANCE_ORDER_0,&ebi_8,sizeof(ebi_8)));
    // Pco
    req.append(Pco("00000d0408080808"));
    //
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
