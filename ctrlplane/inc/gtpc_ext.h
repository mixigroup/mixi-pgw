/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_ext.h
    @brief      mixi_pgw_ctrl_plane gtpc access, c funciton define, common header
*******************************************************************************
*******************************************************************************
    @date       created(08/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 08/nov/2017 
      -# Initial Version
    - 22/mar/2018 
      -# fix gtpc-v1

******************************************************************************/
#ifndef MIXI_PGW_GTPC_EXT_H
#define MIXI_PGW_GTPC_EXT_H

#include "gtpc_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 gtp packet, generate from payload\n
 *******************************************************************************
 allocate packet from payload(gtp[c/u])\n
 *******************************************************************************
 @param[in]   ppkt  packet 
 @param[in]   type  packet type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_alloc_packet_from_payload(gtp_packet_ptr* ppkt, U8* payload, U16 len);

/**
 gtp packet, generate \n
 *******************************************************************************
 packet allocate(gtp[c/u])\n
 *******************************************************************************
 @param[in]   ppkt  packet 
 @param[in]   type  packet type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_alloc_packet(gtp_packet_ptr* ppkt, U8 type);

/**
 gtpv1packet , generate\n
 *******************************************************************************
 packet allocate(gtpv1)\n
 *******************************************************************************
 @param[in]   ppkt  packet 
 @param[in]   type  packet type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_v1_alloc_packet(gtp_packet_ptr* ppkt, U8 type);

/**
 gtp packet, release \n
 *******************************************************************************
 packet release(gtp[c/u])\n
 *******************************************************************************
 @param[in]    ppkt  packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_free_packet(gtp_packet_ptr* ppkt);

/**
 gtp packet, copy address \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   dpkt  dest packet 
 @param[in]   spkt  source packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_copy_packet_addr(gtp_packet_ptr dpkt, gtp_packet_ptr spkt);

/**
 gtp packet -> convert to typical packet\n
 *******************************************************************************
 gtp packet(gtp_packet_ptr) size = ETHER_FRAME\n
 \n
 *******************************************************************************
 @param[in]   pkt  packet 
 @return packet_ptr typical packet 
*/
packet_ptr gtpc_convert_to_packet(gtp_packet_ptr pkt);


/**
 gtp packet , memory status\n
 *******************************************************************************
 get memory status \n
 *******************************************************************************
 @return U64  0==valid,0!=count of invalid
*/
U64 gtpc_memory_status(void);

/**
 gtp packet , print out memory status\n
 *******************************************************************************
 \n
 *******************************************************************************
 @return RETCD  0==success, 0!=error
*/
RETCD gtpc_memory_print(void);



/**
 gtp packet, print out hex\n
 *******************************************************************************
 print out hex memory status\n
 *******************************************************************************
 @param[in]    pkt  packet 
*/
void gtpc_packet_print(gtp_packet_ptr);


/**
 gtp packet : add item\n
 *******************************************************************************
 connect to packet (gtp[c/u] generic)\n
 *******************************************************************************
 @param[in]     pkt   packet 
 @param[in]     type  item type
 @param[in]     data  item data
 @param[in]     len   item length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_append_item(gtp_packet_ptr pkt, U8 type, const U8* data, U16 len);

/**
 gtpc-v1packet, add item \n
 *******************************************************************************
 connect to packet(gtpv1, generic)\n
 *******************************************************************************
 @param[in]     pkt   packet 
 @param[in]     type  item type
 @param[in]     data  item data
 @param[in]     len   item length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_v1_append_item(gtp_packet_ptr pkt, U8 type, const U8* data, U16 len);


/**
 gtpcpacket  : header access \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
gtpc_header_ptr gtpc_header_(gtp_packet_ptr pkt);

/**
 gtpcpacket  : header access \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
gtpc_header_ptr gtpc_header(packet_ptr pkt);

/**
 gtpcpacket  : validate header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_validate_header_(gtp_packet_ptr pkt);


/**
 gtpcpacket  : validate header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_validate_header(packet_ptr pkt);

/**
 gtpc-v1packet  : header access \n
 *******************************************************************************
 gtp-c version 1\n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return RETCD  0==success,0!=error
*/
gtpc_v1_header_ptr gtpc_v1_header(packet_ptr pkt);

/**
 gtpc-v1packet  : access header \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    pkt  packet 
 @return gtpc_header_ptr  gtpc-v2 packet address,NULL==error
*/
gtpc_v1_header_ptr gtpc_v1_header_(gtp_packet_ptr pkt);

/**
 iterate items .callback  \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   type    item type
 @param[in]   data    item first address
 @param[in]   datalen item data length
 @param[in]   arg     callback parameter
 @return RETCD  0==success(continue iterate) , 0!=error(pause iterate)
*/
typedef RETCD (*iterate_func)(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr arg);
/**
 gtp packet : item iteration\n
 *******************************************************************************
 gtpc item iteration\n
 *******************************************************************************
 @param[in]     pkt       packet 
 @param[in]     callback  iterate callback
 @param[in]     arg       callback parameter
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_iterate_item(gtp_packet_ptr pkt, iterate_func callback, gtpc_parse_state_ptr arg);

/**
 parse callback\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   type      item type
 @param[in]   data      item datafirst address
 @param[in]   datalen   item length
 @param[in]   cs        callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_gtpc_parse_root(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr cs);

/**
 parse callback\n
 *******************************************************************************
 parse callback under Bearer Context\n
 *******************************************************************************
 @parma[in]   type      item type
 @param[in]   data      item datafirst address
 @param[in]   datalen   item length
 @param[in]   cs        callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_gtpc_parse_bearer_context(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr cs);

/**
 gtpcpacket  : digits \n
 *******************************************************************************
 get digits from array\n
 *******************************************************************************
 @param[in]   digits  array of digits
 @param[in]   len     number of array
 @return U64 (U64)-1!=success,(U64)(-1)==error
*/
U64 gtpc_digits_get(gtpc_numberdigit_ptr digits, U16 len);

/**
 gtpcpacket  : digits \n
 *******************************************************************************
 set digits\n
 *******************************************************************************
 @param[in/out] digits  array of digits
 @param[in]     len     number of array
 @param[in]     digit   digit value
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_digits_set(gtpc_numberdigit_ptr digits, U16 len, U64 digit);




/**
 gtpcpacket  : IMSI \n
 *******************************************************************************
 setup imsi struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   digits  digits
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_imsi_set(gtpc_imsi_ptr item, U64 digits);

/**
 gtpcpacket  : MSISDN \n
 *******************************************************************************
 setup msisdn struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   digits  digits
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_msisdn_set(gtpc_msisdn_ptr item, U64 digits);

/**
 gtpcpacket  : MSISDN len\n
 *******************************************************************************
 get gtpc_msisdn validate length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_msisdn_length(gtpc_msisdn_ptr item);


/**
 gtpcpacket  : MEI \n
 *******************************************************************************
 setup mei struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   digits  digits
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_mei_set(gtpc_mei_ptr item, U64 digits);



/**
 gtpcpacket  : Recovery\n
 *******************************************************************************
 setup recovery struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   restart_counter counter of restart
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_recovery_set(gtpc_recovery_ptr item, U8 restart_counter);

/**
 gtpcpacket  : Recovery len\n
 *******************************************************************************
 get gtpc_recovery valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_recovery_length(gtpc_recovery_ptr item);

/**
 gtpcpacket  : Cause\n
 *******************************************************************************
 setup cause struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   cause   cause code
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_cause_set(gtpc_cause_ptr item, U8 cause);

/**
 gtpcpacket  : Cause len\n
 *******************************************************************************
 get Cause valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_cause_length(gtpc_cause_ptr item);

/**
 gtpcpacket  : Fteid\n
 *******************************************************************************
 setup fteid struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   inst    instance code
 @param[in]   iftype  interface type
 @param[in]   teid    tunnel id
 @param[in]   ipv4    v4 ip address
 @param[in]   ipv6    v6 ip address
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_f_teid_set(gtpc_f_teid_ptr item, U8 inst, U8 iftype, U32 teid, U8* ipv4, U8* ipv6);

/**
 gtpcpacket  : Fteid len\n
 *******************************************************************************
 get fteid valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_f_teid_length(gtpc_f_teid_ptr item);



/**
 gtpcpacket  : Ambr\n
 *******************************************************************************
 setup ambr struct\n
 *******************************************************************************
 @param[in]   item      item pointer
 @param[in]   uplink    uplink band width
 @param[in]   downlink  downlink band width
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_ambr_set(gtpc_ambr_ptr item, U32 uplink, U32 downlink);

/**
 gtpcpacket  : Ambr len\n
 *******************************************************************************
 get gtpc_ambr valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_ambr_length(gtpc_ambr_ptr item);

/**
 gtpcpacket  : Chargingid\n
 *******************************************************************************
 setup charging_id struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   cid     charging record specific id
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_charging_id_set(gtpc_charging_id_ptr item, U32 cid);

/**
 gtpcpacket  : Charging Id len\n
 *******************************************************************************
 get gtpc_charging_id_ptr valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_charging_id_length(gtpc_charging_id_ptr item);

/**
 gtpcpacket  : Ebi\n
 *******************************************************************************
 set ebi struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   inst    instance
 @param[in]   ebi     ebi identifier
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_ebi_set(gtpc_ebi_ptr item, U8 inst, U8 ebi);


/**
 gtpcpacket  : Ebi len\n
 *******************************************************************************
 get Ebi valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_ebi_length(gtpc_ebi_ptr item);

/**
 gtpcpacket  : Pco\n
 *******************************************************************************
 setup pco struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   pco     data
 @param[in]   len     data length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_pco_set(gtpc_pco_ptr item, U8* pco, U8 len);

/**
 gtpcpacket  : Pco len\n
 *******************************************************************************
 get gtpc_pco valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_pco_length(gtpc_pco_ptr item);

/**
 gtpcpacket  : Paa\n
 *******************************************************************************
 setup paa struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   pdntype type 
 @param[in]   paa     data
 @param[in]   len     data length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_paa_set(gtpc_paa_ptr item,U8 pdntype, U8* paa, U8 len);
/**
 gtpcpacket  : Paa len\n
 *******************************************************************************
 get paa valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_paa_length(gtpc_paa_ptr item);

/**
 gtpcpacket  : ApnRestriction\n
 *******************************************************************************
 set apn_restriction struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   type    type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_apn_restriction_set(gtpc_apn_restriction_ptr item, U8 type);

/**
 gtpcpacket  : ApnRestriction len\n
 *******************************************************************************
 get gtpc_apn_restriction valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_apn_restriction_length(gtpc_apn_restriction_ptr item);


/**
 gtpcpacket  : Bearer Qos\n
 *******************************************************************************
 setup gtpc_bearer_qos struct\n
 *******************************************************************************
 @param[in]   item                item pointer
 @param[in]   flags               flags
 @param[in]   qci                 quality label of traffic
 @param[in]   max_uplink          limit uplink band width
 @param[in]   max_downlink        limit downlink band width
 @param[in]   guaranteed_uplink   guaranteed uplink band width
 @param[in]   guaranteed_downlink guaranteed downlink band width
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_gtpc_bearer_qos_set(gtpc_bearer_qos_ptr item, U8 flags, U8 qci,U64 max_uplink,U64 max_downlink,U64 guaranteed_uplink,U64 guaranteed_downlink);

/**
 gtpcpacket  : Bearer Qos\n
 *******************************************************************************
 get bearer_qos valid data length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_bearer_qos_length(gtpc_bearer_qos_ptr item);

/**
 gtpcpacket  : Bearer Context\n
 *******************************************************************************
 initizlie bearer_ctx struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_bearer_ctx_set(gtpc_bearer_ctx_ptr item);


/**
 gtpcpacket  : Bearer Context\n
 *******************************************************************************
 setup bearer_ctx nested struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   child   child item data
 @param[in]   len     data length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_bearer_ctx_add_child(gtpc_bearer_ctx_ptr item, U8* child, U16 len);

/**
 gtpcpacket  : Bearer Context\n
 *******************************************************************************
 get bearer_ctx data length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_bearer_ctx_length(gtpc_bearer_ctx_ptr item);



/**
 gtp packet : bearer_ctx item iteration\n
 *******************************************************************************
 bearer_ctx item iteration\n
 *******************************************************************************
 @param[in]     pkt       packet 
 @param[in]     callback  iteration callback
 @param[in]     arg       callback parameter
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_iterate_bearer_ctx_item(gtpc_bearer_ctx_ptr item, iterate_func callback, void* arg);




/**
 gtpcpacket  : Private Extension\n
 *******************************************************************************
 setup gtpc_private_extension struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_private_extension_set(gtpc_private_extension_ptr item, gtpc_parse_state_ptr cs);


/**
 gtpcpacket  : pco_ipcp parse item \n
 *******************************************************************************
 pco_ipcp parse item\n
 *******************************************************************************
 @param[in]     pkt       packet 
 @param[in/out] cs        callback , user data
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_iterate_pco_ipcp_item(gtpc_pco_ipcp_cfg_ptr item, gtpc_parse_state_ptr cs);


/**
  parse callback, gtpc-v1\n
 *******************************************************************************
 gtpc-v1 parse : gtpc-v1\n
 *******************************************************************************
 @parma[in]   type      item type
 @param[in]   data      item datafirst address
 @param[in]   datalen   item length
 @param[in]   cs        callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_gtpcv1_parse_root(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr cs);

/**
  gtpc-v1: message table \n
 *******************************************************************************
 gtpc-v1 messge table: gtpc-v1\n
 *******************************************************************************
 @return messge_in_gtpc_v1_ptr message table of gtpc-v1, NULL==error
 */
const messge_in_gtpc_v1_ptr gtpcv1_message_table(void);

/**
 gtpcpacket  : header length \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    type type 
 @return size_t  header length
*/
static inline size_t gtpc_type_headerlen(int type){
    // echo(req/res) <= without teid
    // others        <= with teid
    size_t gtpclen = sizeof(gtpc_header_t);
    //
    switch(type){
        case GTPC_ECHO_REQ:
        case GTPC_ECHO_RES:
            gtpclen -= sizeof(uint32_t);
            break;
    }
    return(gtpclen);
}
/**
 gtpcpacket  , is supported\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    type type 
 @return bool  true==supported , false==not supported
*/
static inline bool gtpc_type_issupported(U8 type){
    switch(type){
        case GTPC_ECHO_REQ:
        case GTPC_ECHO_RES:
        case GTPC_VERSION_NOT_SUPPORTED_INDICATION:
        case GTPC_CREATE_SESSION_REQ:
        case GTPC_CREATE_SESSION_RES:
        case GTPC_MODIFY_BEARER_REQ:
        case GTPC_MODIFY_BEARER_RES:
        case GTPC_DELETE_SESSION_REQ:
        case GTPC_DELETE_SESSION_RES:
        case GTPC_DELETE_BEARER_REQ:
        case GTPC_DELETE_BEARER_RES:
        case GTPC_SUSPEND_NOTIFICATION:
        case GTPC_RESUME_NOTIFICATION:
            break;
        default:
            return(false);
    }
    return(true);
}



#ifdef __cplusplus
}
#endif


#endif //MIXI_PGW_GTPC_EXT_H
