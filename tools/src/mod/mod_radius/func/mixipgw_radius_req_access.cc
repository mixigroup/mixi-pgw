//
// Created by mixi on 2017/06/13.
//

#include "../mixipgw_mod_radius.hpp"

using namespace MIXIPGW_TOOLS;


void RadiusServer::ReqAccess(evutil_socket_t sock, IE& ie, RadiusClient* cli){
    radius_link_t   lnk;
    bool            isok = false;

    // get Calling-Station-Id(string) by elements.
    auto it = ie.find(RADIUS_CONTENT_TYPE_CALLING_STATION_ID);
    auto itn = ie.find(RADIUS_CONTENT_TYPE_NAS_IP_ADDRESS);
    auto itp = ie.find(RADIUS_MAKE_VENDOR_CODE(RADIUS_CONTENT_TYPE_VENDOR_3GPP_PDP_TYPE));
    //
    if (it != ie.end() && itn != ie.end()){
        bzero(&lnk, sizeof(lnk));
        lnk.key = strtoull((it->second).c_str(),NULL,10);;
        lnk.nasipv = strtoul((itn->second).c_str(),NULL,10);
        //
        if (FindTable(&lnk)){
            isok = true;
        }
    }
    // reject or accept
    if (!isok){
        cli->ResponseWithCode(RADIUS_CODE_ACCESS_REJECT);
    }else{
        auto pdn_type = RADIUS_3GPP_VENDOR_CODE_PDP_IPV4;
        if (itp != ie.end()){
            pdn_type = RADIUS_VENDOR_VSA_CODE(ntohl(atoi((itp->second).c_str())));
        }
        cli->ResponseAcceptFrameIp(lnk, pdn_type);
    }
}

