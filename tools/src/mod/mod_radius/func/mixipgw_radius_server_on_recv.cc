//
// Created by mixi on 2017/06/13.
//
#include "../mixipgw_mod_radius.hpp"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>


using namespace MIXIPGW_TOOLS;

#ifndef __TEST__
#define INLINE  static inline
#else
#define INLINE
#endif



// basic validate.
INLINE bool ValidateRadius(RadiusServer* inst, ssize_t rlen ,radius_header_ptr h,struct sockaddr_in& sa, unsigned int salen,struct sockaddr_in& ssa, unsigned int ssalen, RadiusClient** cli){
    // supported request 
    if (h->code != RADIUS_CODE_ACCESS_REQUEST && h->code != RADIUS_CODE_ACCOUNTING_REQUEST){
        Logger::LOGERR("invalid code(%u/%u)", h->code, ntohs(h->len));
        return(false);
    }
    // length: 20 <-> 4096 bytes.
    if (ntohs(h->len) < RADIUS_BUFFER_SIZE_MIN || ntohs(h->len) > RADIUS_BUFFER_SIZE_MAX){
        Logger::LOGERR("invalid length(%u != %u)", rlen, ntohs(h->len));
        return(false);
    }
    if (rlen != ntohs(h->len)){
        Logger::LOGERR("invalid header length(%u != %u)", rlen, ntohs(h->len));
        return(false);
    }
    auto attr  = (unsigned char*)&h[1];
    auto tlen  = (ntohs(h->len) - sizeof(*h));
    // depth == 1 validate packet length
    for(auto n = 0;n < tlen;){
        auto len  = *(attr + n + 1);
        if ((len + n) > tlen){
            Logger::LOGERR("calculate(%u != %u/%u)", tlen, len,n);
            return(false);
        }
        n += len;
    }
    // Identifier is incremented number for each client
    //  grouped by ip.src.addr and udp.src.port.
    std::map<ULONGLONG,radius_traffic_t>::iterator it;
    auto rid = MAKE_ULL(ssa.sin_port, ssa.sin_addr.s_addr);
    if ((it = inst->radius_identifier_.find(rid)) != inst->radius_identifier_.end()){
        (*cli) = (it->second).cli;
        // 重複request を除去
        if ((*cli)->GetSeq() == h->identifier){
            Logger::LOGERR("Duplicate Request.(%u %u %u)",ssa.sin_port, ssa.sin_addr.s_addr, h->identifier);
            return(false);
        }
        (it->second).time = (uint64_t)time(NULL);
        (it->second).seq  = (it->second).seq>(uint8_t)0xfe?(uint8_t)0:(it->second).seq+(uint8_t)1;
        (*cli)->SetSeq(h->identifier);
        (*cli)->SetAuth(h->auth);
        //
        return(true);
    }
    //
    (*cli) = new RadiusClient(&sa, salen, (struct sockaddr*)&ssa, ssalen, h->identifier, inst->sock_, inst->radius_secret_);
    (*cli)->SetSeq(h->identifier);
    (*cli)->SetAuth(h->auth);
    inst->radius_identifier_[rid] = radius_traffic_t{ (uint64_t)time(NULL), (*cli), h->identifier};

    return(true);
}
// copy mandatory parameters in radius packet to map.
INLINE bool ParsePacketToVector(radius_header_ptr h, std::map<uint32_t,std::string>& mpkt){
    auto    attr  = (unsigned char*)(h+1);
    char    bf[128] = {0};
    bool    ok = false,log=false;
    unsigned tmp = 0;
    unsigned short stmp = 0;

    for(auto n = 0;n < (ntohs(h->len) - sizeof(*h));){
        auto type = *(attr + n + 0);
        auto len  = *(attr + n + 1);
        log = false;
        //
        if (type == RADIUS_CONTENT_TYPE_CALLING_STATION_ID && len > 3){
            mpkt[type] = std::string((char*)attr + n + 2, len);
            ok = true;
            log = true;
        }else if ((type == RADIUS_CONTENT_TYPE_NAS_IP_ADDRESS && len == 6) ||
                  (type == RADIUS_CONTENT_TYPE_FRAMED_IP_ADDRESS && len == 6)){
            memcpy(&tmp, attr + n + 2, 4);
            snprintf(bf,sizeof(bf)-1,"%u", tmp);
            mpkt[type] = bf;
            ok = true;
            log = true;
        }else if (type == RADIUS_CONTENT_TYPE_ACCT_STATUS_TYPE && len == 6){
            memcpy(&tmp, attr + n + 2, 4);
            snprintf(bf,sizeof(bf)-1,"%u", tmp);
            mpkt[type] = bf;
            ok = true;
            log = true;
        }else if (type == RADIUS_CONTENT_TYPE_FRAMED_IPV6_PREFIX){
            memcpy(&stmp, attr + n + 2, 2);
            auto prefix = ntohs(stmp);
            std::string ipv6 = "";
            for(auto m = (n+4);m < len; m+=2){
                memcpy(&stmp, attr + m, 2);
                snprintf(bf,sizeof(bf)-1,"%s%04x",(m==(n+4)?"":":"), ntohs(stmp));
                ipv6 += bf;
            }
            snprintf(bf,sizeof(bf)-1,"%s/%u", ipv6.c_str(), prefix);
            mpkt[type] = bf;
        }else if (type == RADIUS_CONTENT_TYPE_VENDOR){
            memcpy(&stmp, attr + n + 2, 2);
            auto offset = (stmp == 0?6:4);
            auto vtype  = *(attr + n + offset);
            auto vlen   = *(attr + n + offset + 1);
            // PDN
            if (vtype == RADIUS_CONTENT_TYPE_VENDOR_3GPP_PDP_TYPE && vlen == 6){
                memcpy(&tmp, attr + n + offset + 2, 4);
                snprintf(bf,sizeof(bf)-1,"%u", tmp);
                mpkt[RADIUS_MAKE_VENDOR_CODE(vtype)] = bf;
            }
        }
//      if (log){ Logger::LOGINF("ParsePacketToVector(%u : %s)", type, mpkt[type].c_str()); }
        n += (int)len;
        if (!len){ break; }
    }
    return(ok);
}


typedef int (*OnRecvMmsg)(int sock, char* bf, int bflen, struct sockaddr_in& ssa, unsigned int ssalen,struct sockaddr_in& sa, unsigned int salen, RadiusServer* inst);

INLINE int RECVMMSG(int sock, OnRecvMmsg callback, RadiusServer* inst){
#ifdef __APPLE__
    char rbuf[RADIUS_BUFFER_SIZE_MAX];
    struct sockaddr_in ssa;
    unsigned int slen = sizeof(ssa);

    // read header(do not proceed)
    auto rlen = recvfrom(sock, rbuf, 4, MSG_PEEK, (struct sockaddr*)&ssa, &slen);
    // packet drop when length < 4.
    if (rlen != 4){
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        Logger::LOGERR("invalid read header(%d)", rlen);
        return(RETERR);
    }
    auto rh = (radius_header_ptr)rbuf;
    if (ntohs(rh->len) < RADIUS_BUFFER_SIZE_MIN || ntohs(rh->len) > RADIUS_BUFFER_SIZE_MAX){
        recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
        Logger::LOGERR("invalid header length(%d)", ntohs(rh->len));
        return(RETERR);
    }
    // read the remained payload.
    rlen = recvfrom(sock, rbuf, ntohs(rh->len), 0, (struct sockaddr*)&ssa, &slen);
    // radius packet 以外はスキップ
    if (rlen < RADIUS_BUFFER_SIZE_MIN || rlen != ntohs(rh->len)){
        Logger::LOGERR("not enough len(%d != %u)", rlen, ntohs(rh->len));
        return(RETERR);
    }
    struct sockaddr_in sa;
    socklen_t       salen = sizeof(sa);
    //
    if (getsockname(sock, (struct sockaddr *)&sa, &salen) < 0){
        Logger::LOGERR("getsockname(%d)", rlen);
        return(RETERR);
    }
    return(callback(sock, rbuf, rlen, ssa, slen, sa, salen, inst));
#else
    #define VLEN (16)
    struct mmsghdr msgs[VLEN];
    struct iovec iovecs[VLEN];
    struct sockaddr_in addrs[VLEN];

    char bufs[VLEN][RADIUS_BUFFER_SIZE_MAX];
    bzero(msgs, sizeof(msgs));
    for(auto n = 0;n < VLEN;n++){
        iovecs[n].iov_base         = bufs[n];
        iovecs[n].iov_len          = RADIUS_BUFFER_SIZE_MAX;
        msgs[n].msg_hdr.msg_iov    = &iovecs[n];
        msgs[n].msg_hdr.msg_iovlen = 1;
        msgs[n].msg_hdr.msg_name   = &addrs[n];
        msgs[n].msg_hdr.msg_namelen= sizeof(struct sockaddr_in);
    }
    auto rlen = recvmmsg(sock, msgs, VLEN, MSG_DONTWAIT, NULL);
    struct sockaddr_in sa;
    socklen_t       salen = sizeof(sa);
    //
    if (getsockname(sock, (struct sockaddr *)&sa, &salen) < 0){
        Logger::LOGERR("getsockname(%d)", rlen);
        return(RETERR);
    }
    auto ret = RETOK;
    int n,m;
    for(n=m=0;n < rlen;){
        // validate header length and total packet length
        auto rh = (radius_header_ptr)&bufs[n][m];
        if (ntohs(rh->len) < RADIUS_BUFFER_SIZE_MIN || ntohs(rh->len) > RADIUS_BUFFER_SIZE_MAX || ntohs(rh->len) > msgs[n].msg_len){
            Logger::LOGERR("invalid header length(%d/ %u)", ntohs(rh->len), msgs[n].msg_len);
            n++;
            m=0;
            continue;
        }
        ret |= callback(sock, &bufs[n][m], ntohs(rh->len), addrs[n], msgs[n].msg_hdr.msg_namelen, sa, salen, inst);
        if (ntohs(rh->len) < msgs[n].msg_len){
            m += msgs[n].msg_len;
        }else{
            m = 0;
            n++;
        }
    }
    return(ret);
#endif
}

INLINE int RecvEvent(int sock, char* bf, int bflen, struct sockaddr_in& ssa, unsigned int ssalen,struct sockaddr_in& sa, unsigned int salen,RadiusServer* inst){
    RadiusClient  *cli = NULL;
    auto radiush = (radius_header_ptr)bf;
    // Radiuspacket  Validate
    if (!ValidateRadius(inst, bflen, radiush, sa, salen, ssa, ssalen, &cli)){
        Logger::LOGERR("ValidateRadiusCode(%u)", bflen);
        return(RETERR);
    }
    IE mpkt;
    // radius requires IMSI(telephone number)
    // parse  , minimize memory costs.
    if (!ParsePacketToVector(radiush, mpkt)){
        Logger::LOGERR("ParsePacketToVector");
        cli->ResponseWithCode(RADIUS_CODE_ACCESS_REJECT);
        return(RETERR);
    }
    if (radiush->code == RADIUS_CODE_ACCESS_REQUEST){
        // Access Req
        inst->ReqAccess(sock, mpkt, cli);
    }else if (radiush->code == RADIUS_CODE_ACCOUNTING_REQUEST){
        // Account Req
        inst->ReqAccount(sock, mpkt, cli);
    }
    return(0);
}

// received event. udp packet
void RadiusServer::OnRecv(evutil_socket_t sock, short what, void *arg){
    RadiusServer* inst = (RadiusServer*)arg;

    auto ret = RECVMMSG(sock, RecvEvent, inst);
    if (ret != RETOK){
        Logger::LOGERR("RECVMMSG(%d)", ret);
    }
}
