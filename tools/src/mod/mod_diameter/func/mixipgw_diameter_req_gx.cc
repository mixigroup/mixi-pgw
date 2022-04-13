//
// Created by mixi on 2017/06/22.
//

#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"

using namespace MIXIPGW_TOOLS;

int Diameter::ReqGxInitial(MIXIPGW_TOOLS::diameter_header_ptr h, evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    char        rbf[4096] = {0};
    auto        dh = (diameter_header_ptr)rbf;
    auto sessid = ie[DIAMETER_AVPCODE_SESSION_ID];
    auto imsi   = ie[DIAMETER_AVPCODE_SUBSCRIPTION_ID_DATA];
    auto authid = ie[DIAMETER_AVPCODE_AUTH_APPLICATION_ID];
    auto cc_type= ie[DIAMETER_AVPCODE_CC_REQUEST_TYPE];
    auto cc_num = ie[DIAMETER_AVPCODE_CC_REQUEST_NUMBER];

    Logger::LOGINF("Diameter::ReqGxInitial");

    // find on link table.
    diameter_link_t lnk;
    bzero(&lnk, sizeof(lnk));
    lnk.key = strtoull(imsi.c_str(),NULL,10);;
    if (!FindTable(&lnk)){
        Logger::LOGERR("not found. diameter contract table.(%s)", imsi.c_str());
        return(RETERR);
    }
    // add ession id, IMSI number to link table
    session_[sessid] = imsi;
    //
    memcpy(dh, h, sizeof(*dh));
    //
    dh->version     = DIAMETER_VERSION;
    dh->f.flag.r    = DIAMETER_OFF;
    dh->f.flag.p    = DIAMETER_ON;
    dh->f.flag.e    = DIAMETER_OFF;

    auto payload    = (char*)(dh+1);
    uint32_t len    = sizeof(*dh);
    auto lenb       = len;

    // CCA(Initial) required
    // session id , origin host, origin realm
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_SESSION_ID, sessid);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_RESULT_CODE, DIAMETER_SUCCESS);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_HOST, origin_host_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_REALM, origin_realm_);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_AUTH_APPLICATION_ID, atoi(authid.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_REQUEST_TYPE, atoi(cc_type.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_REQUEST_NUMBER, atoi(cc_num.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_SESSION_FAILOVER, FAILOVER_SUPPORTED); //
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_ONLINE , DIAMETER_VENDOR_ID_3GPP, DISABLE_ONLINE,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_OFFLINE, DIAMETER_VENDOR_ID_3GPP, DISABLE_ONLINE,1);

    
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, SUCCESSFUL_RESOURCE_ALLOCATION,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, USAGE_REPORT_10,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, USAGE_REPORT,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, REALLOCATION_OF_CREDIT,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, OUT_OF_CREDIT,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_BEARER_CONTROL_MODE, DIAMETER_VENDOR_ID_3GPP, UE_NW,1);
    //
    uint32_t revalidation_tm = DIAMETER_TIME_OFFSET + uint32_t(time(NULL)); 
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_REVALIDATION_TIME, DIAMETER_VENDOR_ID_3GPP, 0, 1);

    // supported functions
    auto lenbf      = len;
    auto feature    = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_SUPPORTED_FEATURES, DIAMETER_VENDOR_ID_3GPP,1);
    auto avphf      = std::get<0>(feature);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_VENDOR_ID, DIAMETER_VENDOR_ID_3GPP);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_FEATURE_LIST_ID, DIAMETER_VENDOR_ID_3GPP, FEATURE_LIST_ID, 0);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_FEATURE_LIST, DIAMETER_VENDOR_ID_3GPP, FEATURE_FLAG, 0);
    avphf->len      = BE24R(len - lenbf); 
    
    // monitoring
    auto lenbkm     = len;
    auto monitor    = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_USAGE_MONITORING_INFORMATION, DIAMETER_VENDOR_ID_3GPP,1);
    auto avphm      = std::get<0>(monitor);

    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_MONITORING_KEY, DIAMETER_VENDOR_ID_3GPP, PCC_RULE_MONITOR_ID,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_USAGE_MONITORING_LEVEL, DIAMETER_VENDOR_ID_3GPP, 1/*SESSION_LEVEL*/,1);
    
    // service traffic
    auto lenbkg = len;
    auto grant  = COPY_AVP_GROUP(&payload, &len, DIAMETER_AVPCODE_GRANTED_SERVICE_UNIT);
    auto avphg  = std::get<0>(grant);
    //
    COPY_AVP_UINT64(&payload, &len, DIAMETER_AVPCODE_CC_INPUT_OCTETS, DIAMETER_GRANTED_SERVICE_UNIT_SIZE);
    COPY_AVP_UINT64(&payload, &len, DIAMETER_AVPCODE_CC_OUTPUT_OCTETS, DIAMETER_GRANTED_SERVICE_UNIT_SIZE);
    COPY_AVP_UINT64(&payload, &len, DIAMETER_AVPCODE_CC_TOTAL_OCTETS, (DIAMETER_GRANTED_SERVICE_UNIT_SIZE<<1));
    avphg->len      = BE24R(len - lenbkg);
    avphm->len      = BE24R(len - lenbkm);
    // QOS information.
    auto lenbkqs    = len;
    auto qosinf     = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_QOS_INFORMATION, DIAMETER_VENDOR_ID_3GPP,1);
    auto avphqs     = std::get<0>(qosinf);
    //
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_APN_AGGREGATE_MAX_BITRATE_DL, DIAMETER_VENDOR_ID_3GPP, lnk.ingress, 1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_APN_AGGREGATE_MAX_BITRATE_UL, DIAMETER_VENDOR_ID_3GPP, lnk.egress, 1);
    //
    avphqs->len     = BE24R(len - lenbkqs);
    // default QOS
    auto lenbkq     = len;
    auto qos        = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_DEFAULT_EPS_BEARER_QOS, DIAMETER_VENDOR_ID_3GPP, 0);
    auto avphq      = std::get<0>(qos);
    // QCI
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_QOS_CLASS_IDENTIFIER, DIAMETER_VENDOR_ID_3GPP, QCI_9,1);

    auto lenbkqc    = len;
    auto qosc       = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_ALLOCATION_RETENTION_PRIORITY, DIAMETER_VENDOR_ID_3GPP,1);
    auto avphqc     = std::get<0>(qosc);
    //
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_PRIORITY_LEVEL, DIAMETER_VENDOR_ID_3GPP, PRIORITY_1,1);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_PRE_EMPTION_CAPABILITY, DIAMETER_VENDOR_ID_3GPP, PRE_EMPTION_CAPABILITY_ENABLED,0);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_PRE_EMPTION_VULNERABILITY, DIAMETER_VENDOR_ID_3GPP, PRE_EMPTION_VULNERABILITY_ENABLED,0);

    avphqc->len     = BE24R(len - lenbkqc);
    avphq->len      = BE24R(len - lenbkq);

    // charging rules
    auto lenbkc     = len;
    auto charging   = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_CHARGING_RULE_INSTALL, DIAMETER_VENDOR_ID_3GPP,1);
    auto avphc      = std::get<0>(charging);
    //
    COPY_AVP_STRINGV(&payload, &len, DIAMETER_AVPCODE_CHARGING_RULE_BASE_NAME, DIAMETER_VENDOR_ID_3GPP, lnk.policy[0]=='H'?"HIGH-QOS":"LOW-QOS");
    COPY_AVP_STRINGV(&payload, &len, DIAMETER_AVPCODE_CHARGING_RULE_NAME, DIAMETER_VENDOR_ID_3GPP, lnk.policy);
    COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_RESOURCE_ALLOCATION_NOTIFICATION, DIAMETER_VENDOR_ID_3GPP, ENABLE_NOTIFICATION ,1);
    avphc->len      = BE24R(len - lenbkc);
    //
    dh->len         = BE24R(len);
    //
    cli->Send(dh, len);

    return(RETOK);
}
int Diameter::ReqGxUpdate(MIXIPGW_TOOLS::diameter_header_ptr h, evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    char rbf[4096] = {0};
    auto dh = (diameter_header_ptr)rbf;
    auto sessid       = ie[DIAMETER_AVPCODE_SESSION_ID];
    auto imsi         = ie[DIAMETER_AVPCODE_SUBSCRIPTION_ID_DATA];
    auto authid       = ie[DIAMETER_AVPCODE_AUTH_APPLICATION_ID];
    auto cc_type      = ie[DIAMETER_AVPCODE_CC_REQUEST_TYPE];
    auto cc_num       = ie[DIAMETER_AVPCODE_CC_REQUEST_NUMBER];
    auto used_ingress = ie[DIAMETER_AVPCODE_CC_INPUT_OCTETS];
    auto used_egress  = ie[DIAMETER_AVPCODE_CC_OUTPUT_OCTETS];

    Logger::LOGINF("Diameter::ReqGxUpdate(%s : %s,%s)", imsi.c_str(), used_ingress.c_str(), used_egress.c_str());
    // find item by imsi number on link table
    diameter_link_t lnk;
    bzero(&lnk, sizeof(lnk));
    lnk.key = strtoull(imsi.c_str(),NULL,10);;
    if (!FindTable(&lnk)){
        Logger::LOGERR("not found. diameter contract table.(%s)", imsi.c_str());
        return(RETERR);
    }
    // Gx Update event
    // insert logdata into log_diameter_gy
    snprintf(rbf, sizeof(rbf) - 1,"INSERT INTO log_diameter_gy(imsi,used_s5_bytes,used_sgi_bytes,reporter)VALUES(%s,%s,%s,'%s')",
             imsi.c_str(),
             used_ingress.c_str(),
             used_egress.c_str(),
             lnk.policy);
    Logger::LOGINF(" Gx CCR:update.(%s)", rbf);
    //
    if (cli->mysql_->Query(rbf) != RETOK){
        Logger::LOGERR("failed. query(%s)",rbf);
//      return(RETERR);
    }
    //
    bzero(rbf, sizeof(rbf));
    memcpy(dh, h, sizeof(*dh));
    dh->version     = DIAMETER_VERSION;
    dh->f.flag.r    = DIAMETER_OFF;
    dh->f.flag.p    = DIAMETER_ON;
    dh->f.flag.e    = DIAMETER_OFF;

    auto payload    = (char*)(dh+1);
    uint32_t len    = sizeof(*dh);
    auto lenb       = len;
    auto res_ok_only= used_ingress.empty()?false:true;

    // CCA(Update) required
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_SESSION_ID, sessid);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_RESULT_CODE, DIAMETER_SUCCESS);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_HOST, origin_host_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_REALM, origin_realm_);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_AUTH_APPLICATION_ID, atoi(authid.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_REQUEST_TYPE, atoi(cc_type.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_REQUEST_NUMBER, atoi(cc_num.c_str()));

    if (res_ok_only){
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, SUCCESSFUL_RESOURCE_ALLOCATION,1);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, USAGE_REPORT_10,1);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, USAGE_REPORT,1);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, REALLOCATION_OF_CREDIT,1);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_EVENT_TRRIGER, DIAMETER_VENDOR_ID_3GPP, OUT_OF_CREDIT,1);
        // monitoring
        auto lenbkm     = len;
        auto monitor    = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_USAGE_MONITORING_INFORMATION, DIAMETER_VENDOR_ID_3GPP,1);
        auto avphm      = std::get<0>(monitor);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_MONITORING_KEY, DIAMETER_VENDOR_ID_3GPP, lnk.mkey, 1);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_USAGE_MONITORING_LEVEL, DIAMETER_VENDOR_ID_3GPP, 1/*SESSION_LEVEL*/,1);
        // service traffic
        auto lenbkg = len;
        auto grant  = COPY_AVP_GROUP(&payload, &len, DIAMETER_AVPCODE_GRANTED_SERVICE_UNIT);
        auto avphg  = std::get<0>(grant);
        //
        COPY_AVP_UINT64(&payload, &len, DIAMETER_AVPCODE_CC_INPUT_OCTETS, DIAMETER_GRANTED_SERVICE_UNIT_SIZE);
        COPY_AVP_UINT64(&payload, &len, DIAMETER_AVPCODE_CC_OUTPUT_OCTETS, DIAMETER_GRANTED_SERVICE_UNIT_SIZE);
        COPY_AVP_UINT64(&payload, &len, DIAMETER_AVPCODE_CC_TOTAL_OCTETS, DIAMETER_GRANTED_SERVICE_UNIT_SIZE<<1);
        avphg->len      = BE24R(len - lenbkg);
        avphm->len      = BE24R(len - lenbkm);

        // QOS information.
        auto lenbkqs    = len;
        auto qosinf     = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_QOS_INFORMATION, DIAMETER_VENDOR_ID_3GPP,1);
        auto avphqs     = std::get<0>(qosinf);
        //
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_APN_AGGREGATE_MAX_BITRATE_DL, DIAMETER_VENDOR_ID_3GPP, lnk.ingress, 1);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_APN_AGGREGATE_MAX_BITRATE_UL, DIAMETER_VENDOR_ID_3GPP, lnk.egress, 1);
        //
        avphqs->len     = BE24R(len - lenbkqs);

        // FIXME:notify only when changes occur.
        // charging rules
        auto lenbkc     = len;
        auto charging   = COPY_AVP_GROUPV(&payload, &len, DIAMETER_AVPCODE_CHARGING_RULE_INSTALL, DIAMETER_VENDOR_ID_3GPP,1);
        auto avphc      = std::get<0>(charging);
        //
        COPY_AVP_STRINGV(&payload, &len, DIAMETER_AVPCODE_CHARGING_RULE_BASE_NAME, DIAMETER_VENDOR_ID_3GPP, lnk.policy[0]=='H'?"HIGH-QOS":"LOW-QOS");
        COPY_AVP_STRINGV(&payload, &len, DIAMETER_AVPCODE_CHARGING_RULE_NAME, DIAMETER_VENDOR_ID_3GPP, lnk.policy);
        COPY_AVP_UINT32V(&payload, &len, DIAMETER_AVPCODE_RESOURCE_ALLOCATION_NOTIFICATION, DIAMETER_VENDOR_ID_3GPP, ENABLE_NOTIFICATION ,1);
        avphc->len      = BE24R(len - lenbkc);
    }
    dh->len         = BE24R(len);
    //
    cli->Send(dh, len);
    return(RETOK);
}
int Diameter::ReqGxTerminate(MIXIPGW_TOOLS::diameter_header_ptr h, evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    auto sessid = ie[DIAMETER_AVPCODE_SESSION_ID];
    char        rbf[4096] = {0};
    auto        dh = (diameter_header_ptr)rbf;
    auto authid = ie[DIAMETER_AVPCODE_AUTH_APPLICATION_ID];
    auto cc_type= ie[DIAMETER_AVPCODE_CC_REQUEST_TYPE];
    auto cc_num = ie[DIAMETER_AVPCODE_CC_REQUEST_NUMBER];
    Logger::LOGINF("Diameter::ReqGxTerminate");
    // remove link  by session id , IMSI number
    session_.erase(sessid);

    memcpy(dh, h, sizeof(*dh));
    //
    dh->version     = DIAMETER_VERSION;
    dh->f.flag.r    = DIAMETER_OFF;
    dh->f.flag.p    = DIAMETER_ON;
    dh->f.flag.e    = DIAMETER_OFF;

    auto payload    = (char*)(dh+1);
    uint32_t len    = sizeof(*dh);
    auto lenb       = len;

    // CCA(Terminate) required
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_SESSION_ID, sessid);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_RESULT_CODE, DIAMETER_SUCCESS);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_HOST, origin_host_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_REALM, origin_realm_);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_AUTH_APPLICATION_ID, atoi(authid.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_REQUEST_TYPE, atoi(cc_type.c_str()));
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_CC_REQUEST_NUMBER, atoi(cc_num.c_str()));
    
    dh->len        = BE24R(len);
    cli->Send(dh, len);
    //
    return(RETOK);
}
int Diameter::ReqGxEvent(MIXIPGW_TOOLS::diameter_header_ptr dh, evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    auto sessid = ie[DIAMETER_AVPCODE_SESSION_ID];
    Logger::LOGINF("Diameter::ReqGxEvent. not implemented");
    //
    cli->Response(dh, sessid, DIAMETER_COMMAND_UNSUPPORTED);
    return(RETOK);
}

int Diameter::ReqGx(MIXIPGW_TOOLS::diameter_header_ptr dh, evutil_socket_t sock, IE& ie, void* p){
    auto cli = (DiameterClient*)p;
    int ret = RETERR;
    auto sessid = ie[DIAMETER_AVPCODE_SESSION_ID];
    auto cc_type = atoi(ie[DIAMETER_AVPCODE_CC_REQUEST_TYPE].c_str());
    //
    if (cc_type == INITIAL_REQUEST){
        ret = ReqGxInitial(dh, sock, ie, p);
    }else if (cc_type == UPDATE_REQUEST){
        ret = ReqGxUpdate(dh, sock, ie, p);
    }else if (cc_type == TERMINATE_REQUEST){
        ret = ReqGxTerminate(dh, sock, ie, p);
    }else if (cc_type == EVENT_REQUEST){
        ret = ReqGxEvent(dh, sock, ie, p);
    }else{
        Logger::LOGERR("not implemented.(%u)", cc_type);
    }
    //
    if (ret == RETERR){
        cli->Response(dh, sessid, DIAMETER_AVP_UNSUPPORTED);
    }else if (ret == RETWRN){
        cli->Response(dh, sessid, DIAMETER_AUTHENTICATION_REJECTED);
    }
    return(RETOK);
}

