/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_item_func_v1.c
    @brief      gtpc-v1packet   item 
*******************************************************************************
*******************************************************************************
    @date       created(22/mar/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 22/mar/2018 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"
#include "gtpc_ext.h"

/**
  parse callback:gtpc-v1\n
 *******************************************************************************
 gtpc-v1 parse : gtpc-v1\n
 *******************************************************************************
 @parma[in]   type      item type
 @param[in]   data      item datafirst address
 @param[in]   datalen   item length(exclude header)
 @param[in]   cs        callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_gtpcv1_parse_root(U8 type, U8* data, U16 datalen, gtpc_parse_state_ptr cs) {
    size_t  offset,loop_counter;
    if(!data || !datalen || !cs) {
        return (ERR);
    }
    cs->flag_v1 |= HAVE_V1;

    if (type == GTPC_V1_IE_CAUSE) {
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_CAUSE/%u:%u)\n", datalen, type);
        if (datalen != GTPCV1_IE_SIMPLE_HEADER_LEN(gtpc_cause_v1_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_CAUSE/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_cause_v1_t));
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_CAUSE;
        cs->cause_v1 = ((gtpc_cause_v1_ptr)data)->cause;
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_CAUSE/%u)\n", cs->cause_v1);
    }else if (type == GTPC_V1_IE_TEID_CTRL_PLANE){
        if (datalen != GTPCV1_IE_SIMPLE_HEADER_LEN(gtpc_teid_v1_t)) {
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_TEID_CTRL_PLANE/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_teid_v1_t));
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_TEID_CTRL;
        memcpy(&cs->c_teid.teid_grekey, &((gtpc_teid_v1_ptr)data)->teid, datalen);
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_TEID_CTRL_PLANE/%u)\n", cs->c_teid.teid_grekey);
    }else if (type == GTPC_V1_IE_TEID_DATA_I){
        if (datalen != GTPCV1_IE_SIMPLE_HEADER_LEN(gtpc_teid_v1_t)) {
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_TEID_DATA_I/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_teid_v1_t));
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_TEID_DATA;
        memcpy(&cs->u_teid.teid_grekey, &((gtpc_teid_v1_ptr)data)->teid, datalen);
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_TEID_DATA_I/%u)\n", cs->u_teid.teid_grekey);
    }else if (type == GTPC_V1_IE_IMSI){
        if (datalen != GTPCV1_IE_SIMPLE_HEADER_LEN(gtpc_imsi_v1_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_IMSI/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_imsi_v1_t));
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_IMSI;
        memcpy(&cs->imsi.digits, ((gtpc_imsi_v1_ptr)data)->digits, GTPC_IMSI_LEN);
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_IMSI/%llu)\n", gtpc_digits_get(cs->imsi.digits, GTPC_IMSI_LEN));

    }else if (type == GTPC_V1_IE_MSISDN){
        if (datalen != GTPCV1_IE_VARIABLE_HEADER_LEN(gtpc_msisdn_v1_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_MSISDN/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_msisdn_v1_t));
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_MSISDN;
        memcpy(&cs->msisdn.digits, ((gtpc_msisdn_v1_ptr)data)->digits, GTPC_MSISDN_LEN);
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_MSISDN/%llu)\n", gtpc_digits_get(cs->msisdn.digits, GTPC_MSISDN_LEN));

    }else if (type == GTPC_V1_IE_GSN_ADDRESS) {
        if(datalen != GTPCV1_IE_VARIABLE_HEADER_LEN(gtpc_gsn_address_v1_t)) {
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_GSN_ADDRESS/%u - %u)\n", datalen,
                    (unsigned) sizeof(gtpc_gsn_address_v1_t));
            return (OK);
        }
        if(cs->flag_v1 & HAVE_V1_IPADR_CTRL) {
            cs->flag_v1 |= HAVE_V1_IPADR_DATA;
            memcpy(cs->u_teid.blk, ((gtpc_gsn_address_v1_ptr)data)->adrs, datalen);
        } else {
            cs->flag_v1 |= HAVE_V1_IPADR_CTRL;
            memcpy(cs->c_teid.blk, ((gtpc_gsn_address_v1_ptr)data)->adrs, datalen);
        }
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_GSN_ADDRESS/%u:%u)\n", cs->flag_v1 & HAVE_V1_IPADR_CTRL,
                cs->flag_v1 & HAVE_V1_IPADR_DATA);
    }else if (type == GTPC_V1_IE_QOS){
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_QOS/%u:%u)\n", datalen, type);
        if (datalen < sizeof(cs->qos)){
            bzero(cs->qos,sizeof(cs->qos));
            memcpy(cs->qos, ((gtpc_qos_v1_ptr)data)->payload, datalen);
            cs->qos_len = datalen;
            cs->flag_v1 |= HAVE_V1_QOS;
        }
    }else if (type == GTPC_V1_IE_NSAPI){
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_NSAPI/%u:%u)\n", datalen, type);
        if (datalen != GTPCV1_IE_SIMPLE_HEADER_LEN(gtpc_nsapi_v1_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_V1_IE_NSAPI/%u - %u)\n", datalen, (unsigned)sizeof(gtpc_nsapi_v1_t));
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_NSAPI;
        cs->nsapi = ((gtpc_nsapi_v1_ptr)data)->nsapi_value;
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_NSAPI/%u)\n", cs->nsapi);
    }else if (type == GTPC_V1_IE_PCO){
        PGW_LOG(PGW_LOG_LEVEL_DBG, "(GTPC_V1_IE_PCO/%u:%u)\n", datalen, type);
        if (datalen >= sizeof(gtpc_pco_t)){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. struct size.(GTPC_TYPE_PCO)\n");
            return(OK);
        }
        cs->flag_v1 |= HAVE_V1_PCO;
        memcpy(&cs->pco.pco_head, &((gtpc_pco_v1_ptr)data)->pco_head, datalen);

        PGW_LOG(PGW_LOG_LEVEL_INF, "(GTPC_V1_IE_PCO - out/%u)\n", datalen);
        // pco request is exists
        for(loop_counter = 0, offset = 0;offset < (datalen -1) && loop_counter < 255;loop_counter++){
            U16 type  = 0;
            U8  pilen = cs->pco.pco[offset + 2];
            memcpy(&type, &cs->pco.pco[offset], sizeof(type));


            PGW_LOG(PGW_LOG_LEVEL_INF, "(GTPC_V1_IE_PCO - intloop/%u:%08x)\n", ntohs(type), cs->flag_v1);
            // supported only content length is 0
            switch(ntohs(type)){
                case GTPC_PCO_DNS:              cs->flag_v1 |= HAVE_PCO_DNS_IPV4; break;
                case GTPC_PCO_DNS6:             cs->flag_v1 |= HAVE_PCO_DNS_IPV6; break;
                case GTPC_PCO_IPV4_LINK_MTU:    cs->flag_v1 |= HAVE_PCO_IPV4_LINK_MTU; break;
                case GTPC_PCO_NONIP_LINK_MTU:   cs->flag_v1 |= HAVE_PCO_NONIP_LINK_MTU; break;
                case GTPC_PCO_IPCP:
                    if (gtpc_iterate_pco_ipcp_item((gtpc_pco_ipcp_cfg_ptr)&cs->pco.pco[offset+3], cs) != OK){
                        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. iterate pco ipcp.(GTPC_TYPE_PCO)\n");
                        return(OK);
                    }
                    break;
            }
            offset += (U16)(sizeof(type) + 1 + pilen);
        }
    }else{
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. not implemented.(%u - %u)\n", datalen, (unsigned)sizeof(gtpc_imsi_v1_t));
        return(ERR);
    }
    return(OK);
}
