//
// Created by mixi on 2017/06/01.
//

#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"


using namespace MIXIPGW_TOOLS;

void SgwSim::GtpcSendModifyBearer(void* arg){
    auto tp     = (ThreadPrm*)arg;
    auto lhost  = sgw_emuc_.host_;
    auto lport  = sgw_emuc_.port_;
    char sbuf[2048] = {0};
    Logger::LOGINF("SgwSim::GtpcSendModifyBearer(this: %p/type:%d)", tp, tp->type_);
    // assign CPU resource
    Misc::SetAffinity(tp->cpuid_);
    Logger::LOGINF("SgwSim::GtpcSendModifyBearer(%p,%d)", (void*)pthread_self(), tp->cpuid_);

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
        SendModifyBearer(tp, &client, sbuf);
        sleep(1);
    }
}
void SgwSim::SendModifyBearer(void* arg, void* client, void* sbuf){
    auto tp   = (ThreadPrm*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto host   = pgw_gtpc_.host_;
    auto port   = pgw_gtpc_.port_;
    size_t rlen = 0;
    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);
    // generate MOdifyBearerRequest from offset, limit, current.
    if ((tp->current_++) >= (tp->offset_ + tp->limit_ -1)){
        tp->current_ = tp->offset_;
    }
    // modify bearer request.
    GtpcPkt req(GTPC_MODIFY_BEARER_REQ);

    gtpc_numberdigit_t  digits[3] = {{4,4},{0,0},{1,0}};
    uint8_t ipv[4] = {0xde,0xad,0xbe,0xef};
    uint8_t     ebi_8(1);

    // Serving Network
    req.append(ServingNetwork(digits));
    // Rat Type
    req.append(Rat(GTPC_RAT_TYPE_UTRAN));
    // Sender F-TEID for Control Plane
    req.append(Fteid(GTPC_INSTANCE_ORDER_1, GTPC_FTEIDIF_S5S8_PGW_GTPC,htonl(tp->current_+1),ipv, NULL));
    // BearerContext
    BearerContext   req_bctx;
    req_bctx.append(Ebi(GTPC_INSTANCE_ORDER_0,&ebi_8,sizeof(ebi_8)));
    req_bctx.append(Fteid(GTPC_INSTANCE_ORDER_2, GTPC_FTEIDIF_S5S8_PGW_GTPU,htonl(tp->current_+2),ipv, NULL));
    req.append(req_bctx);
    // Recovery
    req.append(Recovery(GTPC_RECOVERY_1));
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

