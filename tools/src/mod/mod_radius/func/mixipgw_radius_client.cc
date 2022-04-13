//
// Created by mixi on 2017/06/13.
//

#include "../mixipgw_mod_radius.hpp"


using namespace MIXIPGW_TOOLS;

#ifdef __TEST__
extern void __testcallback__(void* pkt,unsigned len);
#endif

#define RADIUS_SECRET_MAX   (64)

// constructor.destructor
RadiusClient::RadiusClient(struct sockaddr_in* addr_in,unsigned int addr_in_len, struct sockaddr* addr, unsigned int addrlen, uint8_t seq,int sock, const std::string& radius_secret){
    //
    bzero(auth_, sizeof(auth_));
    radius_secret_ = radius_secret;
    ctx_ = (MD5_CTX*)malloc(sizeof(*ctx_));
    if (addr){ memcpy(&addr_, addr, sizeof(addr_)); }
    addrlen_ = addrlen;
    if (addr_in){ memcpy(&addr_in_, addr_in, sizeof(addr_in_)); }
    addr_in_len_ = addr_in_len;
    seq_ = seq;
    //
    client_sock_ = sock;

    if (radius_secret_.length() >= RADIUS_SECRET_MAX){
        throw std::runtime_error("too long radius secret string.");
    }

#if 0
    int on = 1;
    if ((client_sock_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0){ return; }
    if (setsockopt(client_sock_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) { return; }
    if (bind(client_sock_, (struct sockaddr *)&addr_in_, addr_in_len_) < 0){
        Logger::LOGERR("bind : %s", strerror(errno));
        return;
    }
    if (connect(client_sock_, (struct sockaddr *)&addr_, addrlen_) < 0){ return; }
#endif
}
RadiusClient::~RadiusClient(){
#if 0
    close(client_sock_);
#endif
    if (ctx_){ free(ctx_); }
    ctx_ = NULL;
}
// allow
void RadiusClient::ResponseAcceptFrameIp(radius_link_t& lnk, uint32_t flag){
    char    wbf[sizeof(radius_header_t)+6+20+RADIUS_SECRET_MAX] = {0};
    //
    auto trgtlen = sizeof(radius_header_t);
    auto payload = sizeof(radius_header_t);
    auto rh = (radius_header_ptr)wbf;
    auto hav6 = (lnk.ipv6[0] || lnk.ipv6[1] || lnk.ipv6[2] || lnk.ipv6[3]);
    // ipv4 or ipv6 or both
    if (flag==RADIUS_3GPP_VENDOR_CODE_PDP_IPV6 && hav6){
        trgtlen += 20;
    }else if (flag==RADIUS_3GPP_VENDOR_CODE_PDP_IPV4V6 && hav6){
        trgtlen += 20;
        trgtlen += 6;
    }else{
        trgtlen += 6;
    }
    rh->code = RADIUS_CODE_ACCESS_ACCEPT;
    rh->len  = htons(trgtlen);
    rh->identifier = seq_;
    memcpy(rh->auth, auth_, sizeof(auth_));
    if (flag==RADIUS_3GPP_VENDOR_CODE_PDP_IPV6 && hav6){
        wbf[payload + 0] = RADIUS_CONTENT_TYPE_FRAMED_IPV6_PREFIX;
        wbf[payload + 1] = 20;
        wbf[payload + 2] =  0;  // reserved.
        wbf[payload + 3] = 64;  // prefix len
        memcpy(&wbf[payload + 4], &lnk.ipv6, 16);
        payload += 20;
    }else if (flag==RADIUS_3GPP_VENDOR_CODE_PDP_IPV4V6 && hav6){
        wbf[payload + 0] = RADIUS_CONTENT_TYPE_FRAMED_IPV6_PREFIX;
        wbf[payload + 1] = 20;
        wbf[payload + 2] =  0;  // reserved.
        wbf[payload + 3] = 64;  // prefix len
        memcpy(&wbf[payload + 4], &lnk.ipv6, 16);
        payload += 20;
        wbf[payload + 0] = RADIUS_CONTENT_TYPE_FRAMED_IP_ADDRESS;
        wbf[payload + 1] = 6;
        memcpy(&wbf[payload + 2], &lnk.ipv4, 4);
        payload += 6;
    }else{
        wbf[payload + 0] = RADIUS_CONTENT_TYPE_FRAMED_IP_ADDRESS;
        wbf[payload + 1] = 6;
        memcpy(&wbf[payload + 2], &lnk.ipv4, 4);
        payload += 6;
    }
    memcpy(&wbf[payload], radius_secret_.c_str(),radius_secret_.length());
    //
    MD5_Init(ctx_);
    MD5_Update(ctx_,(unsigned char*)wbf,trgtlen);
    MD5_Update(ctx_, (unsigned char*)&wbf[trgtlen],radius_secret_.length());
    MD5_Final(rh->auth, ctx_);
    //
    Response(wbf, trgtlen);
}
// deny
void RadiusClient::ResponseWithCode(uint8_t code){
    char    wbf[sizeof(radius_header_t)+RADIUS_SECRET_MAX] = {0};
    auto trgtlen = sizeof(radius_header_t);
    auto rh = (radius_header_ptr)wbf;
    rh->code = code;
    rh->len  = htons(sizeof(*rh));
    rh->identifier = seq_;
    memcpy(rh->auth, auth_, sizeof(auth_));
    memcpy(&wbf[sizeof(radius_header_t)], radius_secret_.c_str(),radius_secret_.length());
    //
    MD5_Init(ctx_);
    MD5_Update(ctx_,(unsigned char*)wbf,trgtlen);
    MD5_Update(ctx_, (unsigned char*)&wbf[trgtlen],radius_secret_.length());
    MD5_Final(rh->auth, ctx_);
    //
    Response(wbf, trgtlen);
}

// response 
void RadiusClient::Response(void* pkt,unsigned len){
    auto ret = sendto(client_sock_, pkt, len, 0, &addr_,(socklen_t)addrlen_);
    if (ret != len){
        Logger::LOGERR("RadiusClient::Response(%u!=%u)", ret, len);
    }
#ifdef __TEST__
    __testcallback__(pkt, len);
#endif
}


