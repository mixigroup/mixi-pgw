//
// Created by mixi on 2017/04/28.
//
#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"


using namespace MIXIPGW_TOOLS;

void SgwSim::GtpcSendCreateSession(void* arg){
    static int counter = 0;
    auto tp     = (ThreadPrm*)arg;
    auto lhost  = sgw_emuc_.host_;
    auto lport  = sgw_emuc_.port_;
    char sbuf[2048] = {0};
    uint64_t tlv[6] = {0};
    counter ++;
    Logger::LOGINF("SgwSim::GtpcSend(this: %p/type:%d/counter:%d)", tp, tp->type_,counter);
    // separated port used for scaleout.
    lport += (counter%2);
    // setup cpu resource
    Misc::SetAffinity(tp->cpuid_);
    Logger::LOGINF("SgwSim::GtpcSend(%p,%d)", (void*)pthread_self(), tp->cpuid_);

    // prepare bsd-socket
    boost::asio::io_service         ioservice;
    boost::asio::ip::udp::socket    client(ioservice);
    boost::asio::ip::udp::endpoint  local_endp(boost::asio::ip::address::from_string(lhost), lport);
    // bind local port
    // ip@ttl,gtpc local - port config, multiple bind
    client.open(local_endp.protocol());
    client.set_option(boost::asio::ip::udp::socket::send_buffer_size(4*1024*1024));
    client.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    client.set_option(boost::asio::ip::unicast::hops(255));
    client.bind(local_endp);
    // start position.
    tp->current_ = tp->offset_;
    tlv[3] = tp->limit_;
    tlv[4] = tp->current_;
    tlv[5] = tp->offset_;
    //
    while(!Module::ABORT()){
        // keep sending CreateSessionRequest
        //  from multiple threads every delay interval.
        auto batch = tp->Batch();
        if (batch > 1){
            for(auto n = 0;n < batch;n++){
                SendCreateSession(tlv, &client, sbuf);
            }
        }else{
            SendCreateSession(tlv, &client, sbuf);
        }
        tp->DelayNext();
        if (tlv[2]++>100){
            tp->AddPacket(tlv[0]);
            tp->AddBytes(tlv[1]);
             tlv[0] = tlv[1] = tlv[2] = 0;
        }
    }
}


void SgwSim::SendCreateSession(void* arg, void* client, void* sbuf){
    auto tlv   = (uint64_t*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto lhost  = sgw_emuc_.host_;
    auto lhostu = sgw_emuu_.host_;
    auto host   = pgw_gtpc_.host_;
    auto port   = pgw_gtpc_.port_;
    unsigned char ipv4[4] = {0};
    unsigned int  ipv4len = sizeof(ipv4);
    size_t rlen = 0;
    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);
    // 14 defferent bitrates
#define BITRATE_CNT (14)
#define MULTIPLY_K(x) (x<<10)
    uint64_t    bitrates[BITRATE_CNT] = {
            MULTIPLY_K(2),
            MULTIPLY_K(4),
            MULTIPLY_K(8),
            MULTIPLY_K(16),
            MULTIPLY_K(32),
            MULTIPLY_K(64),
            MULTIPLY_K(128),
            MULTIPLY_K(256),
            MULTIPLY_K(512),
            MULTIPLY_K(1024),
            MULTIPLY_K(2048),
            MULTIPLY_K(4096),
            MULTIPLY_K(8192),
            MULTIPLY_K(16384),
    };
    // generate CreateSessionRequestPacket from offset, limit, current
    if ((tlv[4]++) >= (tlv[5] + tlv[3]-1)){
        tlv[4] = tlv[5];
    }
    Misc::GetIpv4(lhost.c_str(), (uint8_t*)ipv4, &ipv4len);

    // create session request.
    GtpcPkt req(GTPC_CREATE_SESSION_REQ);
    req.append(Imsi(IMSI_BASE+tlv[4]));
    req.append(Msisdn(MSISDN_BASE+tlv[4]));
    req.append(Rat(GTPC_RAT_TYPE_UTRAN));
    req.append(Fteid(GTPC_INSTANCE_ORDER_0,GTPC_FTEIDIF_S5S8_SGW_GTPC,tlv[4],(uint8_t*)ipv4,NULL));
    req.append(Apn("test.mixipgw.com"));

    Misc::GetIpv4(lhostu.c_str(), (uint8_t*)ipv4, &ipv4len);
    // BearerContext
    BearerContext   req_bctx;
    req_bctx.append(Ebi(GTPC_INSTANCE_ORDER_0,(uint8_t)((tlv[4]+1)&0xF)));
    req_bctx.append(Fteid(GTPC_INSTANCE_ORDER_0,GTPC_FTEIDIF_S5S8_SGW_GTPU,tlv[4]+1,(uint8_t*)ipv4,NULL));
    auto bid = (tlv[4]%BITRATE_CNT);
    req_bctx.append(Bqos(0,0xe,bitrates[bid],bitrates[bid],bitrates[bid],bitrates[bid]));
    req.append(req_bctx);
    //
    req.append(Ambr((uint32_t)bitrates[bid],(uint32_t)bitrates[bid]));
    //
    auto rbuf = (gtpc_header_ptr)req.ref();
    rlen = req.len();

    rbuf->t.teid = tlv[4];
    rbuf->q.seqno = tlv[4];
    memcpy(sbuf, req.ref(), rlen);
    // to control plane.
    auto r = pcli->send_to(boost::asio::buffer(sbuf, rlen), remote_endp);
    if (r != rlen){
        throw std::runtime_error("failed.send_to(udp socket)");
    }
    tlv[0] += 1;
    tlv[1] += (uint64_t)rlen+(14+20+8/* ether + ip + udp */);
}

