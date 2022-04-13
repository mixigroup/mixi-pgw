//
// Created by mixi on 2017/04/28.
//

#include "../mixipgw_mod_sgw_def.hpp"
#include "lib/const/gtpu.h"


void SgwSim::GtpuSend(void* arg){
    auto tp = (ThreadPrm*)arg;
    auto lhost  = sgw_sndrt_.host_;
    auto lport  = sgw_sndrt_.port_;
    char sbuf[2048] = {0};
    uint64_t tlv[6] = {0};

//  Logger::LOGINF("SgwSim::GtpuSend");
    // assign CPU resource
    Misc::SetAffinity(tp->cpuid_);
    Logger::LOGINF("SgwSim::GtpuSend(%p,%d/%s:%u)", (void*)pthread_self(), tp->cpuid_, lhost.c_str(), lport);

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
    tlv[3] = tp->limit_;
    tlv[4] = tp->current_;
    tlv[5] = tp->offset_;

    while(!Module::ABORT()){
        auto batch = tp->Batch();
        if (batch > 1){
            for(auto n = 0;n < batch;n++){
                SendGtpuData(tlv, &client, sbuf);
            }
        }else{
            SendGtpuData(tlv, &client, sbuf);
        }
        tp->DelayNext();
        if (tlv[2]++>100){
            tp->AddPacket(tlv[0]);
            tp->AddBytes(tlv[1]);
            tlv[0] = tlv[1] = tlv[2] = 0;
        }
    }
}
//
void SgwSim::SendGtpuData(void* arg, void* client, void* sbuf){
    auto tlv = (uint64_t*)arg;
    auto pcli = (boost::asio::ip::udp::socket*)client;
    auto host   = pgw_gtpu_.host_;
    auto port   = pgw_gtpu_.port_;
    unsigned char ipv4[4] = {0};
    unsigned int  ipv4len = sizeof(ipv4);
    size_t rlen = 0;
    static uint16_t counter = 0;
    // destination : host , port
    boost::asio::ip::udp::endpoint  remote_endp(boost::asio::ip::address::from_string(host), port);
    // generate ICMP over GTPU Packet from offset, limit, current.
    if ((tlv[4]++) >= (tlv[5] + tlv[3] -1)){
        tlv[4] = tlv[5];
    }
    uint16_t USER_DATA_LEN = 128;
    // IMIX : 58.33/33.33/8.33 => 7:4:1
 
    auto imix = tlv[4]%12;
    if (imix == 2){
        USER_DATA_LEN = 1400;
    }else if (imix == 1 || imix == 3 || imix == 5 || imix == 7){
        USER_DATA_LEN = 570;
    }else{
        USER_DATA_LEN = 0;
    }

    if (0/*(tp->current_ % 89)==0*/){
        // about 1%, gtpu-echo
        // TODO: still .skip..
        return;
    }else{
        auto u = (gtpu_header_ptr)sbuf;
        bzero(u, sizeof(*u));
        u->type = GTPU_G_PDU;
        u->tid  = htonl(tlv[4]);
        u->length = htons(sizeof(struct icmp)+sizeof(struct ip)+4+USER_DATA_LEN);
        u->u.v1_flags.sequence = GTPU_SEQ_1;
        u->u.v1_flags.version = GTPU_VERSION_1;
        u->u.v1_flags.proto = GTPU_PROTO_GTP;

        auto seq    = ((uint32_t*)(u+1));
        (*seq)      = tlv[4];
        auto ip     = ((struct ip*)(seq+1));
        auto icmp   = ((struct icmp*)(ip+1));
        if (counter >= 0x7fff){ counter = 0; }
        // internal ip/icmp
        bzero(ip, sizeof(*ip));
        ip->ip_v    = IPVERSION;
        ip->ip_hl   = 5;
      //ip->ip_tos  = IPTOS_LOWDELAY;
        ip->ip_tos  = 0;
    //  ip->ip_off  = htons(IP_DF);
        ip->ip_off  = 0;
        ip->ip_id   = htons(0x0278);
        ip->ip_ttl  = 0xff;
        ip->ip_len  = htons(sizeof(struct ip)+sizeof(struct icmp)+USER_DATA_LEN);
        ip->ip_dst.s_addr = htonl(0xac140202);
        ip->ip_src.s_addr = htonl(0xdeadbeaf);
        ip->ip_p    = IPPROTO_ICMP;
        ip->ip_sum  = 0;
        auto ipsum  = Misc::Checksum(ip, sizeof(struct ip), 0);
        ip->ip_sum  = Misc::Wrapsum(ipsum);

        //
        bzero(icmp, sizeof(*icmp));
        icmp->icmp_type  = ICMP_ECHO;
        icmp->icmp_ip.ip_dst.s_addr=htonl(u->tid);
        icmp->icmp_ip.ip_src.s_addr=u->tid;
     // icmp->icmp_ip.ip_dst.s_addr=htonl(123);
        icmp->icmp_cksum = 0;
        icmp->icmp_cksum = Misc::Wrapsum(Misc::Checksum(icmp, sizeof(*icmp), 0));
     // icmp->icmp_cksum = 0x57b0; 
        //
        rlen = sizeof(struct icmp)+sizeof(struct ip)+sizeof(gtpu_header_t)+4+USER_DATA_LEN;
    }
    // to data plane.(P-GW)
    auto r = pcli->send_to(boost::asio::buffer(sbuf, rlen), remote_endp);
    if (r != rlen){
        throw std::runtime_error("failed.send_to(udp socket)");
    }
    tlv[0] += 1;
    tlv[1] += (uint64_t)rlen+(14+20+8/* ether + ip + udp */); 
}
