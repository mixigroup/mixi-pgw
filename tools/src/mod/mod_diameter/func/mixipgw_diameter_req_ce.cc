//
// Created by mixi on 2017/06/29.
//

#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"

using namespace MIXIPGW_TOOLS;

int Diameter::ReqCe(MIXIPGW_TOOLS::diameter_header_ptr h,evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    char rbf[1024] = {0};
    auto dh = (diameter_header_ptr)rbf;
    auto vendorid       = ntohl(atoi(ie[DIAMETER_AVPCODE_VENDOR_ID].c_str()));
    auto product_name   = ie[DIAMETER_AVPCODE_PRODUCT_NAME];
    //
    Logger::LOGINF("Diameter::ReqCe(%d:%s)", vendorid, product_name.c_str());
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
    // CEA
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_RESULT_CODE, DIAMETER_SUCCESS);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_HOST, origin_host_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_REALM, origin_realm_);
    COPY_AVP_IPV4(&payload, &len, DIAMETER_AVPCODE_HOST_IP_ADDRESS, host_ip_address_);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_ACCT_APPLICATION_ID, DIAMETER_APPID_BASE_ACCOUNTING);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_VENDOR_ID, vendor_id_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_PRODUCT_NAME, product_name_);
    // Gx-3gpp 
    auto vendor = COPY_AVP_GROUP(&payload, &len, DIAMETER_AVPCODE_VENDOR_SPECIFIC_APPLICATION_ID);
    auto avphv  = std::get<0>(vendor);
    auto lenbkv = std::get<1>(vendor);
    //
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_VENDOR_ID, DIAMETER_VENDOR_ID_3GPP); 
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_AUTH_APPLICATION_ID, DIAMETER_APPID_3GPP_GX_29212); 
    avphv->len      = BE24R(sizeof(*avphv) + (len - lenbkv));
    // firmaware rev
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_FIRMWARE_REVISION, 1);
    //
    dh->len         = BE24R(sizeof(*dh) + (len - lenb));

    Logger::LOGINF("Diameter::ReqCe(%d:%d)", len, lenb);
    //
    cli->Send(dh, len);
    return(RETOK);
}
