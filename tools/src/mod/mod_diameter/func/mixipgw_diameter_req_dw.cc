//
// Created by mixi on 2017/06/27.
//


#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"

using namespace MIXIPGW_TOOLS;

int Diameter::ReqDw(MIXIPGW_TOOLS::diameter_header_ptr h,evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    char rbf[1024] = {0};
    auto dh = (diameter_header_ptr)rbf;

    auto sessid = ie[DIAMETER_AVPCODE_SESSION_ID];
    auto cc_type = atoi(ie[DIAMETER_AVPCODE_CC_REQUEST_TYPE].c_str());
    //
    Logger::LOGINF("Diameter::ReqDw(%s:%d)", sessid.c_str(), cc_type);

    //
    memcpy(dh, h, sizeof(*dh));
    //
    dh->version     = DIAMETER_VERSION;
    dh->f.flag.r    = DIAMETER_OFF;
    dh->f.flag.p    = DIAMETER_OFF;
    dh->f.flag.e    = DIAMETER_OFF;

    auto payload    = (char*)(dh+1);
    uint32_t len    = sizeof(*dh);
    auto lenb       = len;
    // DWA
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_RESULT_CODE, DIAMETER_SUCCESS);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_HOST, origin_host_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_REALM, origin_realm_);
    //
    dh->len         = BE24R(sizeof(*dh) + (len - lenb));
    //
    cli->Send(dh, len);
    return(RETOK);
}

