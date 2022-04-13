//
// Created by development.team on 2017/06/27.
//

#ifndef MIXIPGW_TOOLS_MOD_DIAMETER_INL
#define MIXIPGW_TOOLS_MOD_DIAMETER_INL

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/misc.hpp"
#include "lib/buffer.hpp"
#include "lib/process.hpp"
#include "lib/module.hpp"
#include "lib/const/diameter.h"
#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <stdexcept>
#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <map>



static inline uint32_t BE24R(uint32_t n){
    uint32_t ptr = n;
    uint32_t ret = ((uint8_t)(((char*)&ptr)[0]&0xff) << 16) +
                   ((uint8_t)(((char*)&ptr)[1]&0xff) << 8) +
                   ((uint8_t)(((char*)&ptr)[2]&0xff));
    return(ret);
}

static inline uint32_t BE24(uint32_t len){
    uint32_t ptr = len;
    uint32_t ret = ((uint8_t)(((char*)&ptr)[0]&0xff) << 16) +
                   ((uint8_t)(((char*)&ptr)[1]&0xff) << 8) +
                   ((uint8_t)(((char*)&ptr)[2]&0xff));
    return(ret);
}
static inline void COPY_AVP_STRING(char** payload, uint32_t* len, uint32_t code, const std::string& str){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    auto tmplen     = strlen(str.c_str())+sizeof(*avph);
    // string
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_OFF;
    avph->f.flag.m  = DIAMETER_ON;
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R(tmplen);
    //
    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), str.c_str(), strlen(str.c_str()));
    (*payload)      += strlen(str.c_str());
    (*len)          += strlen(str.c_str());
    //
    (*payload)      += (PADD4(tmplen) - tmplen);
    (*len)          += (PADD4(tmplen) - tmplen);
}
static inline void COPY_AVP_IPV4(char** payload, uint32_t* len, uint32_t code, const std::string& ip){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    uint8_t ipt[6]  = {0};
    unsigned iptlen = sizeof(ipt);
    auto tmplen     = sizeof(ipt)+sizeof(*avph);
   
    //
    ipt[1] = 1;
    MIXIPGW_TOOLS::Misc::GetIpv4(ip.c_str(), &ipt[2], &iptlen);

    // string
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_OFF;
    avph->f.flag.m  = DIAMETER_ON;
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R(tmplen);
    //
    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), ipt, sizeof(ipt));
    (*payload)      += sizeof(ipt);
    (*len)          += sizeof(ipt);
    //
    (*payload)      += (PADD4(tmplen) - tmplen);
    (*len)          += (PADD4(tmplen) - tmplen);
}
static inline void COPY_AVP_STRINGV(char** payload, uint32_t* len, uint32_t code, uint32_t vendor, const std::string& str){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    auto vendor_cnv = htonl(vendor);
    auto tmplen     = strlen(str.c_str())+sizeof(*avph)+sizeof(vendor_cnv);
    // string
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_ON;
    avph->f.flag.m  = DIAMETER_ON;
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R(strlen(str.c_str())+sizeof(*avph)+sizeof(vendor_cnv));

    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), &vendor_cnv, sizeof(vendor_cnv));
    //
    (*payload)      += sizeof(vendor_cnv);
    (*len)          += sizeof(vendor_cnv);
    memcpy((*payload), str.c_str(), strlen(str.c_str()));
    (*payload)      += strlen(str.c_str());
    (*len)          += strlen(str.c_str());
    //
    (*payload)      += (PADD4(tmplen) - tmplen);
    (*len)          += (PADD4(tmplen) - tmplen);
}

static inline void COPY_AVP_UINT32(char** payload, uint32_t* len, uint32_t code, const uint32_t& num){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    // 32bit
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_OFF;
    avph->f.flag.m  = DIAMETER_ON;
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R((sizeof(num)+sizeof(*avph)));
    auto num_cnv    = htonl(num);
    //
    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), &num_cnv, sizeof(num));
    (*payload)      += sizeof(num);
    (*len)          += sizeof(num);
}

static inline void COPY_AVP_UINT32V(char** payload, uint32_t* len, uint32_t code, uint32_t vendor, const uint32_t& num, const uint32_t m){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    // 32bit + vendor
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_ON;
    avph->f.flag.m  = (m?DIAMETER_ON:0);
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R((sizeof(num)+sizeof(*avph) + 4));
    auto num_cnv    = htonl(num);
    auto vendor_cnv = htonl(vendor);
    //
    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), &vendor_cnv, sizeof(vendor_cnv));

    (*payload)      += sizeof(vendor_cnv);
    (*len)          += sizeof(vendor_cnv);
    memcpy((*payload), &num_cnv, sizeof(num));
    (*payload)      += sizeof(num);
    (*len)          += sizeof(num);
}

static inline void COPY_AVP_UINT64V(char** payload, uint32_t* len, uint32_t code, uint32_t vendor, const uint64_t& num){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    // 32bit + vendor
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_ON;
    avph->f.flag.m  = DIAMETER_ON;
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R((sizeof(num)+sizeof(*avph) + 4));
    auto num_cnv    = htonll(num);
    auto vendor_cnv = htonl(vendor);
    //
    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), &vendor_cnv, sizeof(vendor_cnv));

    (*payload)      += sizeof(vendor_cnv);
    (*len)          += sizeof(vendor_cnv);
    memcpy((*payload), &num_cnv, sizeof(num));
    (*payload)      += sizeof(num);
    (*len)          += sizeof(num);
}
static inline void COPY_AVP_UINT64(char** payload, uint32_t* len, uint32_t code, const uint64_t& num){
    auto avph       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    // 32bit + vendor
    avph->code      = htonl(code);
    avph->f.flag.v  = DIAMETER_OFF;
    avph->f.flag.m  = DIAMETER_ON;
    avph->f.flag.p  = DIAMETER_OFF;
    avph->len       = BE24R((sizeof(num)+sizeof(*avph)));
    auto num_cnv    = htonll(num);
    //
    (*payload)      += sizeof(*avph);
    (*len)          += sizeof(*avph);
    memcpy((*payload), &num_cnv, sizeof(num));
    (*payload)      += sizeof(num);
    (*len)          += sizeof(num);
}
static inline std::tuple<MIXIPGW_TOOLS::diameter_avp_header_ptr,uint32_t> COPY_AVP_GROUPV(char** payload, uint32_t* len, uint32_t code, uint32_t vendor, const uint32_t m){
    auto avphc      = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    avphc->code     = htonl(code);
    avphc->f.flag.v = DIAMETER_ON;
    avphc->f.flag.m = (m?DIAMETER_ON:0);
    avphc->f.flag.p = DIAMETER_OFF;
    (*payload)     += sizeof(*avphc);
    *((uint32_t*)(*payload)) = htonl(vendor);
    (*payload)     += sizeof(uint32_t);/* vendor id */
    (*len)         += sizeof(*avphc) + sizeof(uint32_t);
    //
    return(std::tuple<MIXIPGW_TOOLS::diameter_avp_header_ptr,uint32_t>(avphc, (*len)));
}
static inline std::tuple<MIXIPGW_TOOLS::diameter_avp_header_ptr,uint32_t> COPY_AVP_GROUP(char** payload, uint32_t* len, uint32_t code){
    auto avphc      = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(*payload);
    avphc->code     = htonl(code);
    avphc->f.flag.v = DIAMETER_OFF;
    avphc->f.flag.m = DIAMETER_ON;
    avphc->f.flag.p = DIAMETER_OFF;
    (*payload)     += sizeof(*avphc);
    (*len)         += sizeof(*avphc);
    //
    return(std::tuple<MIXIPGW_TOOLS::diameter_avp_header_ptr,uint32_t>(avphc, (*len)));
}


/**
 * https://supportforums.cisco.com/ja/document/12384186#Gx_Diameter
 *
 * CISCO: implementation similar to PGW
 *
 */

// basic validate.
static inline bool ValidateDiameter(ssize_t rlen ,MIXIPGW_TOOLS::diameter_header_ptr h, uint32_t* cc_type = NULL){
    auto isok = false;
    // check header
    if (h->f.flag.r != DIAMETER_ON || h->version != DIAMETER_VERSION){
        MIXIPGW_TOOLS::Logger::LOGERR("invalid flag/version(%02x/%02x)", h->f.flag.r, h->version);
        return(false);
    }
    // only Dw, Gx, Gy(CCR/CCA,DWR/DWA,CER/CEA)
    auto code   = BE24(h->code);
    auto appid  = ntohl(h->appid);
    if (code == DIAMETER_CMDCODE_CCR_CCA){
                                                             // Gy
        if (appid == DIAMETER_APPID_CREDIT_CONTROL && h->f.flag.p == DIAMETER_ON){
            isok = true;
        }else if (appid == DIAMETER_APPID_3GPP_GX_29212){    // Gx
            isok = true;
        }
    }else if (code == DIAMETER_CMDCODE_DWR_DWA){             // Device Watchdog
        if (appid == DIAMETER_APPID_COMMON_MESSAGE){
            isok = true;
        }
    }else if (code == DIAMETER_CMDCODE_CER_CEA){             // Capability Exchange
        if (appid == DIAMETER_APPID_COMMON_MESSAGE){
            isok = true;
        }
    }
    if (!isok){
        MIXIPGW_TOOLS::Logger::LOGERR("invalid code/appid(%u/%u)", h->code, h->appid);
        return(false);
    }
    // support length = 20 <-> 4096 bytes.
    auto payloadlen = BE24(h->len);
    if (payloadlen < sizeof(*h) || payloadlen > XDIAMETER_PAYLOAD_MAX){
        MIXIPGW_TOOLS::Logger::LOGERR("invalid length(%u != %u)", rlen, payloadlen);
        return(false);
    }
    if (rlen != payloadlen){
        MIXIPGW_TOOLS::Logger::LOGERR("invalid header length(%u != %u)", rlen, payloadlen);
        return(false);
    }
    auto attr  = (unsigned char*)&h[1];
    auto tlen  = (payloadlen - sizeof(*h));
    // validate in depth == 1, packet length
    for(auto n = 0;n < tlen;){
        auto avp  = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(attr + n);
        auto avplen = BE24(avp->len);
        auto avppayload = (attr + n + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avp->f.flag.v?4:0));
        //
        if (cc_type != NULL && ntohl(avp->code) == DIAMETER_AVPCODE_CC_REQUEST_TYPE){
            memcpy(cc_type, avppayload, sizeof(*cc_type));
        }
        if ((avplen + n) > tlen){
            MIXIPGW_TOOLS::Logger::LOGERR("calculate(%u != %u/%u)", tlen, avplen,n);
            return(false);
        }
        n += PADD4(avplen);
    }
    return(true);
}
// copy mandatory parameters in diameter packet to map.
static inline bool ParsePacketToVector(MIXIPGW_TOOLS::diameter_header_ptr h, IE& mpkt){
    auto    attr   = (unsigned char*)(h+1);
    auto    attrlen = BE24(h->len);
    char    bf[32] = {0};
    uint32_t ok = 0;
    uint64_t tmp64 = 0;
    unsigned tmp = 0;
    //
    for(auto n = 0;n < (attrlen - sizeof(*h));){
        auto avp        = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(attr + n);
        auto avplen     = BE24(avp->len);
        auto avppayload = (attr + n + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avp->f.flag.v?4:0));
        auto log = true;
        auto avplenv    = (avplen - (sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+avp->f.flag.v?4:0));
#define SET_DIAMETER_MAP_TXT(M,a,l,c) {mpkt[c] = std::string((char*)a, l); M(ok);}
#define SET_DIAMETER_MAP_TXTB(M,b,c) {mpkt[c] = std::string(b); M(ok);}
        auto code   = ntohl(avp->code);
        //
        if (code == DIAMETER_AVPCODE_SESSION_ID){
            SET_DIAMETER_MAP_TXT(HAS_SESSION_ID,avppayload,avplenv,code);
        }else if (code == DIAMETER_AVPCODE_AUTH_APPLICATION_ID){
            memcpy(&tmp, avppayload, sizeof(tmp));
            snprintf(bf,sizeof(bf)-1,"%u", ntohl(tmp));
            SET_DIAMETER_MAP_TXTB(HAS_AUTH_APPLICATION_ID,bf,code);
        }else if (code == DIAMETER_AVPCODE_ORIGIN_HOST){
            SET_DIAMETER_MAP_TXT(HAS_ORIGIN_HOST,avppayload,avplenv,code);
        }else if (code == DIAMETER_AVPCODE_ORIGIN_REALM){
            SET_DIAMETER_MAP_TXT(HAS_ORIGIN_REALM,avppayload,avplenv,code);
        }else if (code == DIAMETER_AVPCODE_DESTINATION_REALM){
            SET_DIAMETER_MAP_TXT(HAS_DESTINATION_REALM,avppayload,avplenv,code);
        }else if (code == DIAMETER_AVPCODE_SERVICE_CONTEXT_ID){
            SET_DIAMETER_MAP_TXT(HAS_SERVICE_CONTEXT_ID,avppayload,avplenv,code);
        }else if (code == DIAMETER_AVPCODE_VENDOR_ID){
            memcpy(&tmp, avppayload, sizeof(tmp));
            snprintf(bf,sizeof(bf)-1,"%u", ntohl(tmp));
            SET_DIAMETER_MAP_TXTB(HAS_DONTCARE,bf,code);
        }else if (code == DIAMETER_AVPCODE_PRODUCT_NAME){
            SET_DIAMETER_MAP_TXT(HAS_DONTCARE,avppayload,avplenv,code);
        }else if (code == DIAMETER_AVPCODE_CC_REQUEST_TYPE){
            memcpy(&tmp, avppayload, sizeof(tmp));
            snprintf(bf,sizeof(bf)-1,"%u", ntohl(tmp));
            SET_DIAMETER_MAP_TXTB(HAS_CC_REQUEST_TYPE,bf, code);
        }else if (code == DIAMETER_AVPCODE_CC_REQUEST_NUMBER){
            memcpy(&tmp, avppayload, sizeof(tmp));
            snprintf(bf,sizeof(bf)-1,"%u", ntohl(tmp));
            SET_DIAMETER_MAP_TXTB(HAS_CC_REQUEST_NUMBER,bf,code);
        }else if (code == DIAMETER_AVPCODE_SUBSCRIPTION_ID){
            // Subscription-Id-Type + Subscription-Id-Data
            int is_e164 = 0; 
            int m = 0,l = 0;
            for(;l < 2;l++){
                auto avpt        = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(avppayload + m);
                auto avplent     = BE24(avpt->len);
                auto avppayloadt = (avppayload + m + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avpt->f.flag.v?4:0));
                //
                if (ntohl(avpt->code) == DIAMETER_AVPCODE_SUBSCRIPTION_ID_DATA && is_e164){
                    SET_DIAMETER_MAP_TXT(HAS_SUBSCRIPTION_ID_DATA,avppayloadt,avplent,ntohl(avpt->code));
                }else if (ntohl(avpt->code) == DIAMETER_AVPCODE_SUBSCRIPTION_ID_TYPE){
                    memcpy(&tmp, avppayloadt, sizeof(tmp));
                    if (ntohl(tmp) == END_USER_E164){
                        SET_DIAMETER_MAP_TXT(HAS_SUBSCRIPTION_ID_TYPE,avppayloadt,avplent,ntohl(avp->code));
                        is_e164 = 1;
                    }
                }
                m += PADD4(avplent);
            }
            HAS_SUBSCRIPTION_ID(ok);
        }else if (code == DIAMETER_AVPCODE_MULTIPLE_SERVICES_CREDIT_CONTROL){
            // Requested-Service-Unit + Used-Service-Unit
            int m = 0,l = 0;
            for(;l < 2;l++){
                auto avpt        = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(avppayload + m);
                auto avplent     = BE24(avpt->len);
                auto avppayloadt = (avppayload + m + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avpt->f.flag.v?4:0));
                //
                if (ntohl(avpt->code) == DIAMETER_AVPCODE_USED_SERVICE_UNIT){
                    // DIAMETER_AVPCODE_CC_INPUT_OCTETS
                    int o = 0,p = 0;
                    for(;p < 3;p++){
                        auto avptd       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(avppayloadt + o);
                        auto avplentd    = BE24(avptd->len);
                        auto avppayloadtd= (avppayloadt + o + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avptd->f.flag.v?4:0));
                        //
                        if (ntohl(avptd->code) == DIAMETER_AVPCODE_CC_INPUT_OCTETS){
                            memcpy(&tmp64, avppayloadtd, sizeof(tmp64));
                            snprintf(bf,sizeof(bf)-1,FMT_LLU, ntohll(tmp64));
                            SET_DIAMETER_MAP_TXTB(HAS_CC_INPUT_OCTETS,bf,ntohl(avptd->code));
                        }else if (ntohl(avptd->code) == DIAMETER_AVPCODE_CC_OUTPUT_OCTETS){
                            memcpy(&tmp64, avppayloadtd, sizeof(tmp64));
                            snprintf(bf,sizeof(bf)-1,FMT_LLU, ntohll(tmp64));
                            SET_DIAMETER_MAP_TXTB(HAS_CC_OUTPUT_OCTETS,bf,ntohl(avptd->code));
                        }
                        o += PADD4(avplentd);
                    }
                }
                m += PADD4(avplent);
            }
        }else if (code == DIAMETER_AVPCODE_USAGE_MONITORING_INFORMATION){
            // Usage-Monitoring-Information
            // -> Monitoring-Key
            // -> Used-Service-Unit
            //    -> CC-Input-Octets
            //    -> CC-Output-Octets
            // -> Usage-Motitoring-Level
            int m = 0,l = 0;
            for(;l < 3;l++){
                auto avpt        = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(avppayload + m);
                auto avplent     = BE24(avpt->len);
                auto avppayloadt = (avppayload + m + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avpt->f.flag.v?4:0));
                //
                if (ntohl(avpt->code) == DIAMETER_AVPCODE_USED_SERVICE_UNIT){
                    int o = 0,p = 0;
                    for(;p < 3;p++){
                        auto avptd       = (MIXIPGW_TOOLS::diameter_avp_header_ptr)(avppayloadt + o);
                        auto avplentd    = BE24(avptd->len);
                        auto avppayloadtd= (avppayloadt + o + sizeof(MIXIPGW_TOOLS::diameter_avp_header_t)+(avptd->f.flag.v?4:0));
                        //
                        if (ntohl(avptd->code) == DIAMETER_AVPCODE_CC_INPUT_OCTETS){
                            memcpy(&tmp64, avppayloadtd, sizeof(tmp64));
                            snprintf(bf,sizeof(bf)-1,FMT_LLU, ntohll(tmp64));
                            SET_DIAMETER_MAP_TXTB(HAS_CC_INPUT_OCTETS,bf,ntohl(avptd->code));
                        }else if (ntohl(avptd->code) == DIAMETER_AVPCODE_CC_OUTPUT_OCTETS){
                            memcpy(&tmp64, avppayloadtd, sizeof(tmp64));
                            snprintf(bf,sizeof(bf)-1,FMT_LLU, ntohll(tmp64));
                            SET_DIAMETER_MAP_TXTB(HAS_CC_OUTPUT_OCTETS,bf,ntohl(avptd->code));
                        }
                        o += PADD4(avplentd);
                    }
                }
                m += PADD4(avplent);
            }
        }else{
            log = false;
        }
        if (log){
            /* MIXIPGW_TOOLS::Logger::LOGINF("ParsePacketToVector(%08X(%u) : %s)", code, code, mpkt[code].c_str()); */
        }
        n += PADD4(avplen);
        if (!avplen){ break; }
    }
    if (BE24(h->code) == DIAMETER_CMDCODE_CCR_CCA){
        // validation of keys required by cressit control.
        MIXIPGW_TOOLS::Logger::LOGINF("CCR required(%08x)", ok);
        return(IS_REQUIRED_CC(ok));
    }else{
        // device watchdog capability enabled if origin-host/realm is present
        return(IS_REQUIRED_DW(ok));
    }
}


#endif //MIXIPGW_TOOLS_MOD_DIAMETER_INL
