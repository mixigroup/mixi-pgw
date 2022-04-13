//
// Created by mixi on 2017/06/14.
//

#include "../mixipgw_mod_radius.hpp"

using namespace MIXIPGW_TOOLS;


void RadiusServer::ReqAccount(evutil_socket_t sock, IE& ie, RadiusClient* cli){
    radius_link_t   lnk;
    bool            isok = false;
    Logger::LOGINF("RadiusServer::ReqAccount");


    // get Calling-Station-Id(string), Nas-Ip-Address(string) by elements.
    auto it  = ie.find(RADIUS_CONTENT_TYPE_CALLING_STATION_ID);
    auto itn = ie.find(RADIUS_CONTENT_TYPE_NAS_IP_ADDRESS);
    if (it != ie.end() && itn != ie.end()){
        bzero(&lnk, sizeof(lnk));
        lnk.key = strtoull((it->second).c_str(),NULL,10);
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
        cli->ResponseWithCode(RADIUS_CODE_ACCOUNTING_RESPONSE);
    }
}

