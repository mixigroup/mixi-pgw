/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_item_func.c
    @brief      gtpcpacket   item 
*******************************************************************************
*******************************************************************************
    @date       created(29/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 29/nov/2017 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"
#include "gtpc_ext.h"
/**
 gtp packet :  item iteration \n
 *******************************************************************************
 gtpc item iteration\n
 *******************************************************************************
 @param[in]     pkt       packet 
 @param[in]     callback  iterate callback
 @param[in]     arg       callback parameter
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_iterate_item(gtp_packet_ptr pkt, iterate_func callback, gtpc_parse_state_ptr arg){
    gtp_item_ptr p1,p2;
    //
    if (!pkt || !callback){
        return(ERR);
    }
    p1 = TAILQ_FIRST(&pkt->items);
    while(p1 != NULL){
        p2 = TAILQ_NEXT(p1, link);
        //  item callback
        if (callback(p1->type, (U8*)&(pkt->packet[p1->offset]), p1->length, arg) != OK){
            return(ERR);
        }
        p1 = p2;
    }

    return(OK);
}

/**
  parse callback\n
 *******************************************************************************
 parse since root \n
 *******************************************************************************
 @parma[in]   type      item type
 @param[in]   data      item datafirst address
 @param[in]   datalen   item length
 @param[in]   cs        callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_gtpc_parse_root(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr cs){
    size_t  offset,loop_counter,len;
    if (type == GTPC_TYPE_MSISDN){
        if (datalen > sizeof(gtpc_msisdn_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_MSISDN/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_msisdn_t));
            return(OK);
        }
        cs->flag |= HAVE_MSISDN;
        memcpy(&cs->msisdn, data, datalen);
    }else if (type == GTPC_TYPE_IMSI){
        if (datalen > sizeof(gtpc_imsi_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_IMSI:%u - %u)\n", datalen, (unsigned)sizeof(gtpc_imsi_t));
            return(OK);
        }
        cs->flag |= HAVE_IMSI;
        memcpy(&cs->imsi, data, datalen);
    }else if (type == GTPC_TYPE_RECOVERY){
        if (datalen != sizeof(gtpc_recovery_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_RECOVERY)\n");
            return(OK);
        }
        cs->flag |= HAVE_RECOVERY;
        memcpy(&cs->recovery, data, datalen);
    }else if (type == GTPC_TYPE_CAUSE){
        if (datalen != sizeof(gtpc_cause_t) && datalen != sizeof(gtpc_cause_t) - 4){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_CAUSE:%u - %u)\n", datalen , (unsigned)sizeof(gtpc_cause_t));
            return(OK);
        }
        cs->flag |= HAVE_CAUSE;
        memcpy(&cs->cause, data, datalen);
    }else if (type == GTPC_TYPE_MEI){
        if (datalen > sizeof(gtpc_mei_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_MEI)\n");
            return(OK);
        }
        cs->flag |= HAVE_MEI;
        memcpy(&cs->mei, data, datalen);
    }else if (type == GTPC_TYPE_F_TEID){
        if (datalen != sizeof(gtpc_f_teid_t) && datalen != sizeof(gtpc_f_teid_t) - 12){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_F_TEID)\n");
            return(OK);
        }
        cs->flag |= HAVE_FTEID_C;
        memcpy(&cs->c_teid, data, datalen);
    }else if (type == GTPC_TYPE_AMBR){
        if (datalen != sizeof(gtpc_ambr_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_AMBR)\n");
            return(OK);
        }
        cs->flag |= HAVE_AMBR_R;
        memcpy(&cs->ambr_r, data, datalen);
    }else if (type == GTPC_TYPE_CHARGING_ID){
        if (datalen != sizeof(gtpc_charging_id_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_CHARGING_ID)\n");
            return(OK);
        }
        cs->flag |= HAVE_CHRGID;
        memcpy(&cs->charg, data, datalen);
    }else if (type == GTPC_TYPE_EBI){
        if (datalen != sizeof(gtpc_ebi_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_EBI)\n");
            return(OK);
        }
        cs->flag |= HAVE_EBI;
        memcpy(&cs->ebi_r, data, datalen);
    }else if (type == GTPC_TYPE_RAT_TYPE){
        if (datalen != sizeof(gtpc_rat_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_RAT_TYPE)\n");
            return(OK);
        }
        cs->flag |= HAVE_RAT;
        memcpy(&cs->rat, data, datalen);
    }else if (type == GTPC_TYPE_PCO){
        if (datalen >= sizeof(gtpc_pco_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_PCO)\n");
            return(OK);
        }
        cs->flag |= HAVE_PCO;
        memcpy(&cs->pco, data, datalen);

        // PCO request is exists
        len = ntohs(cs->pco.head.length) -2;
        for(loop_counter = 0, offset = 0;offset < len && loop_counter < 255;loop_counter++){
            U16 type  = 0;
            U8  pilen = cs->pco.pco[offset + 2];
            memcpy(&type, &cs->pco.pco[offset], sizeof(type));

            // supported only content length is 0.
            // when response parse, item length >0 is available(during testing)
            if (pilen == 0 || ((cs->flag & HAVE_RES_ORDER) == HAVE_RES_ORDER)) {
                switch(ntohs(type)){
                    case GTPC_PCO_DNS:              cs->flag |= HAVE_PCO_DNS_IPV4; break;
                    case GTPC_PCO_DNS6:             cs->flag |= HAVE_PCO_DNS_IPV6; break;
                    case GTPC_PCO_IPV4_LINK_MTU:    cs->flag |= HAVE_PCO_IPV4_LINK_MTU; break;
                    case GTPC_PCO_NONIP_LINK_MTU:   cs->flag |= HAVE_PCO_NONIP_LINK_MTU; break;
                }
            }
            // IPCP content length must be > 0.
            if (ntohs(type)==GTPC_PCO_IPCP){
                if (gtpc_iterate_pco_ipcp_item((gtpc_pco_ipcp_cfg_ptr)&cs->pco.pco[offset+3], cs) != OK){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. iterate pco ipcp.(GTPC_TYPE_PCO)\n");
                    return(OK);
                }
            }
            offset += (U16)(sizeof(type) + 1 + pilen);
        }
    }else if (type == GTPC_TYPE_PAA){
        if (datalen > sizeof(gtpc_paa_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_PAA)\n");
            return(OK);
        }
        cs->flag |= HAVE_PAA;
        memcpy(&cs->paa, data, datalen);
    }else if (type == GTPC_TYPE_APN_RESTRICTION){
        if (datalen != sizeof(gtpc_apn_restriction_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_APN_RESTRICTION)\n");
            return(OK);
        }
        cs->flag |= HAVE_APN_R;
        memcpy(&cs->apn_r, data, datalen);
    }else if (type == GTPC_TYPE_BEARER_QOS){
        if (datalen != sizeof(gtpc_bearer_qos_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_BEARER_QOS)\n");
            return(OK);
        }
        cs->flag |= HAVE_BQOS_R;
        memcpy(&cs->bqos_r, data, datalen);
    }else if (type == GTPC_TYPE_PDN_TYPE){
        if (datalen != sizeof(gtpc_pdn_type_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_PDN)\n");
            return(OK);
        }
        cs->flag |= HAVE_PDN;
        memcpy(&cs->pdn, data, datalen);

    }else if (type == GTPC_TYPE_BEARER_CTX){
        cs->flag |= HAVE_BCTX;
        return(gtpc_iterate_bearer_ctx_item((gtpc_bearer_ctx_ptr)data, on_gtpc_parse_bearer_context, cs));
    }
    return(OK);
}
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
RETCD on_gtpc_parse_bearer_context(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr cs){
    if (type == GTPC_TYPE_F_TEID){
        if (datalen != sizeof(gtpc_f_teid_t) && datalen != sizeof(gtpc_f_teid_t) - 12){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_F_TEID - Bearer Context)\n");
            return(OK);
        }
        cs->flag |= HAVE_FTEID_U;
        memcpy(&cs->u_teid, data, datalen);
    }else if (type == GTPC_TYPE_BEARER_QOS){
        if (datalen != sizeof(gtpc_bearer_qos_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_BEARER_QOS - Bearer Context)\n");
            return(OK);
        }
        cs->flag |= HAVE_BQOS;
        memcpy(&cs->bqos, data, datalen);
    }else if (type == GTPC_TYPE_EBI){
        if (datalen != sizeof(gtpc_ebi_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_EBI - Bearer Context)\n");
            return(OK);
        }
        cs->flag |= HAVE_EBI_B;
        memcpy(&cs->ebi_b, data, datalen);
    }else if (type == GTPC_TYPE_AMBR){
        if (datalen != sizeof(gtpc_ambr_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_AMBR - Bearer Context)\n");
            return(OK);
        }
        cs->flag |= HAVE_AMBR;
        memcpy(&cs->ambr, data, datalen);
    }
    return(OK);
}


/**
 gtp packet :  add item\n
 *******************************************************************************
 connect to packet(gtp[c/u] generic)\n
 *******************************************************************************
 @param[in]     pkt   packet 
 @param[in]     type  item type
 @param[in]     data  item data
 @param[in]     len   item length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_append_item(gtp_packet_ptr pkt, U8 type, const U8* data, U16 len){
    gtp_item_ptr    pi;
    gtpc_header_ptr gtpch;

    if (!pkt || !data || !len){
        return(ERR);
    }
    // packet buffer size capability
    if ((pkt->offset + len) >= pkt->length){
        return(ERR);
    }
    if (!(pi = (gtp_item_ptr)malloc(sizeof(gtp_item_t)))){
        return(ERR);
    }
    //
    pi->type    = type;
    pi->length  = len;
    pi->offset  = pkt->offset;
    // add to list
    TAILQ_INSERT_TAIL(&pkt->items, pi, link);
    // increment count of list items
    pkt->count ++;

    // copy payload.
    memcpy(&pkt->packet[pkt->offset], data, len);
    pkt->offset += len;

    // update grpc header
    gtpch = (gtpc_header_ptr)pkt->packet;
    gtpch->length = htons(ntohs(gtpch->length) + len);

    return(OK);
}

/**
 gtpc-v1packet  :  item add \n
 *******************************************************************************
 connect item to packet(gtpv1, generic)\n
 *******************************************************************************
 @param[in]     pkt   packet 
 @param[in]     type  item type
 @param[in]     data  item data
 @param[in]     len   item length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_v1_append_item(gtp_packet_ptr pkt, U8 type, const U8* data, U16 len){
    gtp_item_ptr    pi;
    gtpc_v1_header_ptr gtpch;

    if (!pkt || !data || !len){
        return(ERR);
    }
    // packet buffer size capability
    if ((pkt->offset + len) >= pkt->length){
        return(ERR);
    }
    if (!(pi = (gtp_item_ptr)malloc(sizeof(gtp_item_t)))){
        return(ERR);
    }
    //
    pi->type    = type;
    pi->length  = len;
    pi->offset  = pkt->offset;
    // add to list
    TAILQ_INSERT_TAIL(&pkt->items, pi, link);
    // increment count of list items.
    pkt->count ++;

    // copy payload.
    memcpy(&pkt->packet[pkt->offset], data, len);
    pkt->offset += len;

    // update gtpc header
    gtpch = (gtpc_v1_header_ptr)pkt->packet;
    gtpch->length = htons(ntohs(gtpch->length) + len);

    return(OK);
}
/**
 gtpcpacket  : digits \n
 *******************************************************************************
 get digits from array\n
 *******************************************************************************
 @param[in]   digits  array of digits
 @parma[in]   len     number of array
 @return U64 (U64)-1!=success , (U64)(-1)==error
*/
U64 gtpc_digits_get(gtpc_numberdigit_ptr digits, U16 len){
    char bf[128] = {0};
    char tbf[128] = {0};
    U64 ret = -1;
    for(size_t n = 0; n < len; n++){
        if (digits[n].high == 0xf && digits[n].low == 0xf){
            continue;
        }else if (digits[n].high == 0xf){
            snprintf(bf, sizeof(bf) -1, "%s%d", tbf, digits[n].low);
        }else if (digits[n].low==0xf){
            snprintf(bf, sizeof(bf) -1, "%s%d", tbf, digits[n].high);
        }else{
            snprintf(bf, sizeof(bf) -1, "%s%d%d", tbf, digits[n].low, digits[n].high);
        }
        memcpy(tbf, bf, sizeof(tbf) - 1);
    }
    ret = strtoull(bf, NULL, 10);
    if (errno == ERANGE){
        return((U64)-1);
    }
    return(ret);
}


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
RETCD gtpc_digits_set(gtpc_numberdigit_ptr digits, U16 len, U64 digit){
    int n;
    char bf[32] = {0};
    snprintf(bf,sizeof(bf)-1, "%llu" , digit);
    memset(digits, 0xff, sizeof(gtpc_numberdigit_t) * len);
    //
    if (digits != 0 && strlen(bf) <= (len*2)){
        for(n = 0;n < strlen(bf);n+=2){
            digits[n==0?0:n>>1].low   = (bf[n+0] - '0');
            if ((n+1) < strlen(bf)){
                digits[n==0?0:n>>1].high  = (bf[n+1] - '0');
            }
        }
    }else{
        return(ERR);
    }
    return(OK);
}


/**
 gtpcpacket  : IMSI \n
 *******************************************************************************
 setup imsi struct\n
 *******************************************************************************
 @param[in]   item    item pointer 
 @param[in]   digits  digits value
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_imsi_set(gtpc_imsi_ptr item, U64 digits){
    bzero(item, sizeof(*item));
    if (gtpc_digits_set(item->digits, GTPC_IMSI_LEN, digits) != OK){
        return(ERR);
    }
    item->head.type     = GTPC_TYPE_IMSI;
    item->head.length   = htons(GTPC_IMSI_LEN);
    //
    return(OK);
}

/**
 gtpcpacket  : Recovery\n
 *******************************************************************************
 setup recovery struct\n
 *******************************************************************************
 @param[in]   item     item pointer 
 @param[in]   restart_counter counter of re-start
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_recovery_set(gtpc_recovery_ptr item, U8 restart_counter){
    bzero(item, sizeof(*item));
    item->head.type = GTPC_TYPE_RECOVERY;
    item->head.length = htons(1);
    item->recovery_restart_counter = restart_counter;
    return(OK);
}
/**
 gtpcpacket  : Recovery len\n
 *******************************************************************************
 get gtpc_recovery valid length\n
 *******************************************************************************
 @param[in]   item     item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_recovery_length(gtpc_recovery_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}

/**
 gtpcpacket  : Cause\n
 *******************************************************************************
 setup cause struct(supported only 6 octet)\n
 *******************************************************************************
 @param[in]   item     item pointer
 @param[in]   cause   cause code
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_cause_set(gtpc_cause_ptr item, U8 cause){
    bzero(item, sizeof(*item));
    item->head.type = GTPC_TYPE_CAUSE;
    item->head.length = htons(2);
    item->cause = cause;
    return(OK);
}


/**
 gtpcpacket  : Cause len\n
 *******************************************************************************
 get Cause valid length\n
 *******************************************************************************
 @param[in]   item     item pointer 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_cause_length(gtpc_cause_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}

/**
 gtpcpacket  : MSISDN \n
 *******************************************************************************
 setup msisdn struct\n
 *******************************************************************************
 @param[in]   item    item pointer 
 @param[in]   digits  digits
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_msisdn_set(gtpc_msisdn_ptr item, U64 digits){
    bzero(item, sizeof(*item));
    if (gtpc_digits_set(item->digits, GTPC_MSISDN_LEN, digits) != OK){
        return(ERR);
    }
    item->head.type     = GTPC_TYPE_MSISDN;
    item->head.length   = htons(GTPC_MSISDN_LEN);
    return(OK);
}

/**
 gtpcpacket  : MSISDN len\n
 *******************************************************************************
 get gtpc_msisdn valid length\n
 *******************************************************************************
 @param[in]   item     item pointer
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_msisdn_length(gtpc_msisdn_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}


/**
 gtpcpacket  : MEI \n
 *******************************************************************************
 setup mei struct\n
 *******************************************************************************
 @param[in]   item    item pointer 
 @param[in]   digits  digits
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_mei_set(gtpc_mei_ptr item, U64 digits){
    bzero(item, sizeof(*item));
    if (gtpc_digits_set(item->digits, GTPC_MEI_LEN, digits) != OK){
        return(ERR);
    }
    item->head.type     = GTPC_TYPE_MEI;
    item->head.length   = htons(GTPC_MEI_LEN);
    return(OK);
}

/**
 gtpcpacket  : Fteid\n
 *******************************************************************************
 setup fteid struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   inst    instance code
 @param[in]   iftype  interface type
 @param[in]   teid    tunnel identifier
 @param[in]   ipv4    ip address  : v4
 @param[in]   ipv6    ip address  : v6
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_f_teid_set(gtpc_f_teid_ptr item, U8 inst, U8 iftype, U32 teid, U8* ipv4, U8* ipv6){
    bzero(item, sizeof(*item));

    if (!ipv4 && !ipv6){
        return(ERR);
    }
    item->head.type = GTPC_TYPE_F_TEID;
    item->head.inst.instance = inst;
    item->teid_grekey = htonl(teid);
    item->bit.iftype = iftype;
    //
    if (ipv4 != NULL){
        item->bit.v4 = 1;
        memcpy(item->blk, ipv4, 4);
        item->head.length = htons(9);
    }else{
        item->bit.v6 = 1;
        memcpy(item->blk, ipv6, 16);
        item->head.length = htons(21);
    }
    return(OK);
}

/**
 gtpcpacket  : Fteid len\n
 *******************************************************************************
 get fteid valid length\n
 *******************************************************************************
 @param[in]   item    item pointer
 @return RETCD  0==success,0!=error
*/
U16 gtpc_f_teid_length(gtpc_f_teid_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}




/**
 gtpcpacket  : Ambr\n
 *******************************************************************************
 setup ambr struct\n
 *******************************************************************************
 @param[in]   item       item pointer
 @param[in]   uplink    uplink band width
 @param[in]   downlink  downlink band width
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_ambr_set(gtpc_ambr_ptr item, U32 uplink, U32 downlink){
    bzero(item, sizeof(*item));
    item->uplink    = htonl(uplink);
    item->downlink  = htonl(downlink);
    item->head.type = GTPC_TYPE_AMBR;
    item->head.length = htons(8);
    return(OK);
}
/**
 gtpcpacket  : Ambr len\n
 *******************************************************************************
 get gtpc_ambr valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_ambr_length(gtpc_ambr_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}

/**
 gtpcpacket  : Chargingid\n
 *******************************************************************************
 setup charging_id struct\n
 *******************************************************************************
 @param[in]   item     item pointer
 @param[in]   cid     chargin record specific id
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_charging_id_set(gtpc_charging_id_ptr item, U32 cid){
    bzero(item, sizeof(*item));
    item->charging_id   = htonl(cid);
    item->head.type = GTPC_TYPE_CHARGING_ID;
    item->head.length = htons(4);
    return(OK);
}
/**
 gtpcpacket  : Charging Id len\n
 *******************************************************************************
 get gtpc_charging_id_ptr valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_charging_id_length(gtpc_charging_id_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}
/**
 gtpcpacket  : Ebi\n
 *******************************************************************************
 setup ebi struct\n
 *******************************************************************************
 @param[in]   item    item pointer
 @param[in]   inst    instance
 @param[in]   ebi     ebi identifier
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_ebi_set(gtpc_ebi_ptr item, U8 inst, U8 ebi){
    bzero(item, sizeof(*item));
    item->head.inst.instance = inst;
    item->head.type = GTPC_TYPE_EBI;
    item->head.length = htons(1);
    item->ebi.low = ebi;
    return(OK);
}

/**
 gtpcpacket  : Ebi len\n
 *******************************************************************************
 get Ebi valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_ebi_length(gtpc_ebi_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}



/**
 gtpcpacket  : Pco\n
 *******************************************************************************
 setup pco struct\n
 *******************************************************************************
 @param[in]   item     item pointer
 @param[in]   pco     data
 @param[in]   len     data length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_pco_set(gtpc_pco_ptr item, U8* pco, U8 len){
    bzero(item->pco, sizeof(item->pco));
    if (len >= sizeof(item->pco)){
        return(ERR);
    }
    item->head.type = GTPC_TYPE_PCO;
    item->head.length = htons(len);
    memcpy(item->pco, pco, len);
    return(OK);
}

/**
 gtpcpacket  : Pco len\n
 *******************************************************************************
 get gtpc_pco valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_pco_length(gtpc_pco_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}

/**
 gtpcpacket  : Paa\n
 *******************************************************************************
 setup paa struct\n
 *******************************************************************************
 @param[in]   item     item pointer
 @param[in]   pdntype type
 @param[in]   paa     paa data
 @param[in]   len     data length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_paa_set(gtpc_paa_ptr item, U8 pdntype, U8* paa, U8 len){
    bzero(item, sizeof(*item));
    if (len > sizeof(item->paa)){
        return(ERR);
    }
    item->head.type = GTPC_TYPE_PAA;
    item->head.length = htons(len + 1);
    item->bit.pdn_type = pdntype;
    memcpy(item->paa, paa, len);
    return(OK);
}

/**
 gtpcpacket  : Paa len\n
 *******************************************************************************
 get paa valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_paa_length(gtpc_paa_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}


/**
 gtpcpacket  : ApnRestriction\n
 *******************************************************************************
 setup apn_restriction struct\n
 *******************************************************************************
 @param[in]   item     item pointer
 @param[in]   type    type 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_apn_restriction_set(gtpc_apn_restriction_ptr item, U8 type){
    bzero(item, sizeof(*item));
    item->head.type = GTPC_TYPE_APN_RESTRICTION;
    item->head.length = htons(1);
    item->restriction_type = type;
    return(OK);
}

/**
 gtpcpacket  : ApnRestriction len\n
 *******************************************************************************
 get gtpc_apn_restriction valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_apn_restriction_length(gtpc_apn_restriction_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}

/**
 gtpcpacket  : Bearer Qos\n
 *******************************************************************************
 setup gtpc_bearer_qos struct\n
 *******************************************************************************
 @param[in]   item                 item pointer
 @param[in]   flags               flags
 @param[in]   qci                 quality label of traffic
 @param[in]   max_uplink          limit uplink band width
 @param[in]   max_downlink        limit downlink band width
 @param[in]   guaranteed_uplink   guaranteed uplink band width
 @param[in]   guaranteed_downlink guaranteed downlink band width
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_gtpc_bearer_qos_set(gtpc_bearer_qos_ptr item, U8 flags, U8 qci,U64 max_uplink,U64 max_downlink,U64 guaranteed_uplink,U64 guaranteed_downlink){
    bzero(item, sizeof(*item));
    //
    memcpy(&item->bit, &flags, 1);
    item->qci = qci;
    max_uplink = XHTONLL(max_uplink);
    max_downlink = XHTONLL(max_downlink);
    guaranteed_uplink = XHTONLL(guaranteed_uplink);
    guaranteed_downlink = XHTONLL(guaranteed_downlink);
    //
    memcpy(&item->rate[0],&((char*)&max_uplink)[3],5);
    memcpy(&item->rate[5],&((char*)&max_downlink)[3],5);
    memcpy(&item->rate[10],&((char*)&guaranteed_uplink)[3],5);
    memcpy(&item->rate[15],&((char*)&guaranteed_downlink)[3],5);

    item->head.length = htons(sizeof(*item) - sizeof(gtpc_comm_header_t));
    item->head.type = GTPC_TYPE_BEARER_QOS;
    return(OK);
}

/**
 gtpcpacket  : Bearer Qos\n
 *******************************************************************************
 get bearer_qos valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_bearer_qos_length(gtpc_bearer_qos_ptr item){
    return(ntohs(item->head.length) + sizeof(item->head));
}



/**
 gtpcpacket  : Bearer Context\n
 *******************************************************************************
 setup bearer_ctx struct\n
 *******************************************************************************
 @param[in]   item     item 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_bearer_ctx_set(gtpc_bearer_ctx_ptr item){
    bzero(item, sizeof(*item));
    item->head.length = htons(0);
    item->head.type = GTPC_TYPE_BEARER_CTX;
    item->offset = 0;
    return(OK);
}
/**
 gtpcpacket  : Bearer Context\n
 *******************************************************************************
 get bearer_ctx valid length\n
 *******************************************************************************
 @param[in]   item     item 
 @return U16  data length,(U16)-1==error
*/
U16 gtpc_bearer_ctx_length(gtpc_bearer_ctx_ptr item){
    return(item->offset + sizeof(item->head));
}



/**
 gtpcpacket  : Bearer Context\n
 *******************************************************************************
 setup bearer_ctx nested struct\n
 *******************************************************************************
 @param[in]   item     item pointer
 @param[in]   child   nested(child) item data
 @param[in]   len     item data length
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_bearer_ctx_add_child(gtpc_bearer_ctx_ptr item, U8* child, U16 len){
    if ((item->offset + len) >= sizeof(item->child)){
        return(ERR);
    }
    item->head.length = htons(ntohs(item->head.length) + len);
    memcpy(&item->child[item->offset], child, len);
    item->offset += len;
    return(OK);
}


/**
 gtp packet : bearer_ctx  item iteration \n
 *******************************************************************************
 bearer_ctx item iteration\n
 *******************************************************************************
 @param[in]     pkt       packet 
 @param[in]     callback  iterate callback
 @param[in]     arg       callback arguments
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_iterate_bearer_ctx_item(gtpc_bearer_ctx_ptr item, iterate_func callback, void* arg){
    U16 cur = 0, cnt = 0, len = ntohs(item->head.length);

    // start parse
    for(cnt = 0; cur < len && cnt < GTPC_CAPABILITY_ITEM_COUNT; cnt++){
        gtpc_comm_header_ptr pitm = (gtpc_comm_header_ptr)(&item->child[cur]);
        uint16_t hlen = ntohs(pitm->length) + sizeof(gtpc_comm_header_t);
        //
        if (callback(pitm->type, (U8*)pitm, hlen, arg) != OK){
            break;
        }
        cur += hlen;
        //
        if (cur > len){
            // case in malformed item.
            PGW_LOG(PGW_LOG_LEVEL_ERR, "malformed ...(%d, %d, %d)GtpcPkt::attach", cur, len, hlen);
            return(ERR);
        }else if (cur == len){
            return(OK);
        }
    }
    return(ERR);
}


/**
 gtpcpacket  : Private Extension\n
 *******************************************************************************
 setup gtpc_private_extension struct\n
 *******************************************************************************
 @param[in]   item     item 
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_private_extension_set(gtpc_private_extension_ptr item, gtpc_parse_state_ptr cs){
    bzero(item, sizeof(*item));
    item->head.type = GTPC_TYPE_PRIVATE_EXTENSION;
    item->head.length = htons(sizeof(*item) - sizeof(gtpc_comm_header_t));
    item->magic = GTPC_PRIVATE_EXT_MAGIC;
    item->venderid = htons(GTPC_PRIVATE_EXT_VENDERID);
    item->value[SGW_GTPU_TEID] = cs->u_teid.teid_grekey;
    item->value[UE_TEID] = cs->pgw_teid;
    memcpy(&item->value[UE_IPV4], &cs->ue_ipv_n, sizeof(U32));
    memcpy(&item->value[SGW_GTPU_IPV4], &cs->u_teid.blk[0], sizeof(U32));
    memcpy(&item->value[PGW_GTPU_IPV4], &cs->pgw_u_ipv_n, sizeof(U32));

    return(OK);
}

/**
 gtpcpacket  : pco_ipcp  item iteration \n
 *******************************************************************************
 pco_ipcp item iteration\n
 *******************************************************************************
 @param[in]     pkt       packet 
 @param[in/out] cs        callback , user data
 @return RETCD  0==success,0!=error
*/
RETCD gtpc_iterate_pco_ipcp_item(gtpc_pco_ipcp_cfg_ptr item, gtpc_parse_state_ptr cs){
    //
    if (htons(item->length) != sizeof(gtpc_pco_ipcp_cfg_t)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "malformed ...(%d, %d)gtpc_iterate_pco_ipcp_item", item->code, htons(item->length));
        return(ERR);
    }
    //
    if (item->opt0.type == GTPC_PCO_IPCP_CFG_PRIMARY_DNS){
        cs->flag |= HAVE_PCO_IPCP_P_DNS;
        cs->flag_v1 |= HAVE_PCO_IPCP_P_DNS;
    }else if (item->opt0.type == GTPC_PCO_IPCP_CFG_SECONDARY_DNS){
        cs->flag |= HAVE_PCO_IPCP_S_DNS;
        cs->flag_v1 |= HAVE_PCO_IPCP_S_DNS;
    }
    //
    if (item->opt1.type == GTPC_PCO_IPCP_CFG_PRIMARY_DNS){
        cs->flag |= HAVE_PCO_IPCP_P_DNS;
        cs->flag_v1 |= HAVE_PCO_IPCP_P_DNS;
    }else if (item->opt1.type == GTPC_PCO_IPCP_CFG_SECONDARY_DNS){
        cs->flag |= HAVE_PCO_IPCP_S_DNS;
        cs->flag_v1 |= HAVE_PCO_IPCP_S_DNS;
    }
    return(OK);
}
