/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc.h
    @brief      mixi_pgw_ctrl_plane gtpc defined
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
******************************************************************************/
#ifndef MIXI_PGW_GTPC_H
#define MIXI_PGW_GTPC_H

#include "pgw_def.h"

#define GTPC_PORT                               2123

// defined for gtpc-v2
#define GTPC_VERSION_0                          (0x00)
#define GTPC_VERSION_1                          (0x01)
#define GTPC_VERSION_2                          (0x02)

#define GTPC_PIGGY_OFF                          0
#define GTPC_PIGGY_ON                           1

#define GTPC_TEID_OFF                           0
#define GTPC_TEID_ON                            1

// Cause
#define GTPC_CAUSE_REQ_REJECTED                 0
#define GTPC_CAUSE_REQ_ACCEPTED                 1

#define GTPC_UNKNOWN                            0
#define GTPC_ECHO_REQ                           1
#define GTPC_ECHO_RES                           2
#define GTPC_VERSION_NOT_SUPPORTED_INDICATION   3
#define GTPC_CREATE_SESSION_REQ                 32
#define GTPC_CREATE_SESSION_RES                 33
#define GTPC_MODIFY_BEARER_REQ                  34
#define GTPC_MODIFY_BEARER_RES                  35
#define GTPC_DELETE_SESSION_REQ                 36
#define GTPC_DELETE_SESSION_RES                 37
#define GTPC_DELETE_BEARER_REQ                  99
#define GTPC_DELETE_BEARER_RES                  100
#define GTPC_RESUME_NOTIFICATION                164
#define GTPC_RESUME_ACK                         165
#define GTPC_SUSPEND_NOTIFICATION               162
#define GTPC_SUSPEND_ACK                        163

#define GTPC_TYPE_IMSI                          1
#define GTPC_TYPE_CAUSE                         2
#define GTPC_TYPE_RECOVERY                      3
#define GTPC_TYPE_APN                           71
#define GTPC_TYPE_AMBR                          72
#define GTPC_TYPE_EBI                           73
#define GTPC_TYPE_MEI                           75
#define GTPC_TYPE_MSISDN                        76
#define GTPC_TYPE_INDICATION                    77
#define GTPC_TYPE_PCO                           78
#define GTPC_TYPE_PAA                           79
#define GTPC_TYPE_BEARER_QOS                    80
#define GTPC_TYPE_RAT_TYPE                      82
#define GTPC_TYPE_SERVING_NETWORK               83
#define GTPC_TYPE_ULI                           86
#define GTPC_TYPE_F_TEID                        87
#define GTPC_TYPE_BEARER_CTX                    93
#define GTPC_TYPE_CHARGING_ID                   94
#define GTPC_TYPE_PDN_TYPE                      99
#define GTPC_TYPE_APN_RESTRICTION               127
#define GTPC_TYPE_SELECTION_MODE                128
#define GTPC_TYPE_PRIVATE_EXTENSION             255


#define GTPC_CAUSE_REQUEST_ACCEPTED             16
#define GTPC_CAUSE_REQUEST_ACCEPTED_PARTIALLY   17
#define GTPC_CAUSE_REQUEST_DENIED_APN_ACCESS    93

#define GTPC_CONTEXT_NOT_FOUND                  64
#define GTPC_INVALID_MESSAGE_FORMAT             65


#define GTPC_RAT_TYPE_UTRAN                     1
#define GTPC_RAT_TYPE_EUTRAN                    6

// IMSI(International Mobile Subscriber Identity)
#define GTPC_IMSI_LEN                           8
#define GTPC_APN_LEN                            32
#define GTPC_DIGITS_MASKON                      1
#define GTPC_DIGITS_MASKOFF                     0

#define GTPC_PDN_IPV4                           1
#define GTPC_PDN_IPV6                           2
#define GTPC_PDN_IPV4V6                         3


// AMBR(Aggregate Maximum Bit Rate)
#define GTPC_AMBR_BIT_RATE_64                   0x00000040
#define GTPC_AMBR_BIT_RATE_128                  0x00000080
#define GTPC_AMBR_BIT_RATE_384                  0x00000180
#define GTPC_AMBR_BIT_RATE_1024                 0x00000400
#define GTPC_AMBR_BIT_RATE_12500                0x00003004

// MEI(Mobile Equipment Identity)
#define GTPC_MEI_LEN                            8
// MSISDN
#define GTPC_MSISDN_LEN                         6
#ifndef ETHER_MAX_LEN
#define ETHER_MAX_LEN                           (1518)
#endif
#define GTPC_ACTIVATE_PDP_CONTEXT_REQ           (0x41)
#define GTPC_PCO_LCP                            htons(0xc021)
#define GTPC_PCO_PAP                            htons(0xc023)
#define GTPC_PCO_CHAP                           htons(0xc223)
#define GTPC_PCO_IPCP                           (0x8021)
#define GTPC_PCO_DNS6                           (0x0003)
#define GTPC_PCO_DNS                            (0x000d)
#define GTPC_PCO_IPV4_LINK_MTU                  (0x0010)
#define GTPC_PCO_NONIP_LINK_MTU                 (0x0015)
#define GTPC_PCO_EXTENSION_ON                   (1)
#define GTPC_PCO_IPCP_CFG_REQ                   (1)

#define GTPC_PCO_IPCP_CFG_PRIMARY_DNS           (0x81)
#define GTPC_PCO_IPCP_CFG_SECONDARY_DNS         (0x83)


#define GTPC_PCO_DNS_LEN                        (0x04)
#define GTPC_PCO_IPCP_LEN                       (0x10)
#define GTPC_PCO_LEN                            (253)
#define GTPC_PAA_ADRS_LEN                       (21)

// FTEID
#define GTPC_FTEIDIF_S5S8_SGW_GTPU              4
#define GTPC_FTEIDIF_S5S8_PGW_GTPU              5
#define GTPC_FTEIDIF_S5S8_SGW_GTPC              6
#define GTPC_FTEIDIF_S5S8_PGW_GTPC              7
#define GTPC_FTEIDIF_S11_MME                    10
#define GTPC_FTEIDIF_S11S4_SGW                  11

// INSTANCE
#define GTPC_INSTANCE_ORDER_0                   0
#define GTPC_INSTANCE_ORDER_1                   1
#define GTPC_INSTANCE_ORDER_2                   2
#define GTPC_PAA_PDNTYPE_IPV4                   1
#define GTPC_PAA_PDNTYPE_IPV6                   2
#define GTPC_PAA_PDNTYPE_BOTH                   3
#define GTPC_APN_RESTRICTION                    0

#define GTPC_BQOS_FLAG                          0x04
#define GTPC_BQOS_QCI                           9
#define GTPC_BQOS_GUARANTEED_S5                 128
#define GTPC_BQOS_GUARANTEED_SGI                128
#define GTPC_RECOVERY_1                         1
#define GTPC_CAPABILITY_ITEM_COUNT             (255)

#define GTPC_PRIVATE_EXT_SIZE                  (32)        /*!< internal use gtpc item size */
#define GTPC_PRIVATE_EXT_MAGIC                 (0xdeadbeaf)/*!< internal use gtpc item magic code */
#define GTPC_PRIVATE_EXT_VENDERID              (00000)     /*!< private mib number */


#define GTPC_V1_PT_GTP                         (1)
#define GTPC_V1_SEQNUM_YES                     (1)
#define GTPC_V1_NPDU_NO                        (0)
#define GTPC_V1_EXTENSION_NO                   (0)
#define GTPC_V1_CAUSE_ACCEPTED                 (0x80)
#define GTPC_V1_CAUSE_VERSION_NOT_SUPPORTED    (0xc6)
#define GTPC_V1_CAUSE_NO_RESOURCE_AVAILABLE    (0xC7)
#define GTPC_V1_GSNADDR_LEN                    (4)
#define GTPC_V1_EUA_ORGANIZATION_IETF          (1)
#define GTPC_V1_EUA_NUMBER_IP                  (0x21)
#define GTPC_V1_QOS_PAYLOAD_SZ                 (256)
/*! @enum GTPC_V1_MSG
  @brief
  GTPC_V1_MSG \n
*/
enum GTPC_V1_MSG{
    GTPC_V1_MSG_DC = 0,
    GTPC_V1_MSG_ECHO_REQ = 1,
    GTPC_V1_MSG_ECHO_RES,
    GTPC_V1_MSG_VERSION_NOT_SUPPORTED,
    GTPC_V1_MSG_NODE_ALIVE_REQ,
    GTPC_V1_MSG_NODE_ALIVE_RES,
    GTPC_V1_MSG_REDIRECTION_REQ,
    GTPC_V1_MSG_REDIRECTION_RES,
    GTPC_V1_MSG_CREATE_PDP_CONTEXT_REQ = 16,
    GTPC_V1_MSG_CREATE_PDP_CONTEXT_RES,
    GTPC_V1_MSG_UPDATE_PDP_CONTEXT_REQ,
    GTPC_V1_MSG_UPDATE_PDP_CONTEXT_RES,
    GTPC_V1_MSG_DELETE_PDP_CONTEXT_REQ,
    GTPC_V1_MSG_DELETE_PDP_CONTEXT_RES,
    GTPC_V1_MSG_INITIATE_PDP_CONTEXT_ACTIVATION_REQ,
    GTPC_V1_MSG_INITIATE_PDP_CONTEXT_ACTIVATION_RES,
    GTPC_V1_MSG_PDU_NOTIFICATION_REQ = 27,
    GTPC_V1_MSG_PDU_NOTIFICATION_RES,
    GTPC_V1_MSG_PDU_NOTIFICATION_REJECT_REQ,
    GTPC_V1_MSG_PDU_NOTIFICATION_REJECT_RES,
    GTPC_V1_MSG_MAX
};

/*! @enum GTPC_V1_IE
  @brief
  GTPC_V1_IE \n
*/
enum GTPC_V1_IE{
    GTPC_V1_IE_CAUSE = 1,
    GTPC_V1_IE_IMSI,
    GTPC_V1_IE_RAI,
    GTPC_V1_IE_TLLI,
    GTPC_V1_IE_P_TMSI,
    GTPC_V1_IE_REORDERING_REQ = 8,
    GTPC_V1_IE_AUTH_TRIPLET,
    GTPC_V1_IE_MAP_CAUSE = 11,
    GTPC_V1_IE_P_TMSI_SIGN,
    GTPC_V1_IE_MS_VALIDATED,
    GTPC_V1_IE_RECOVERY,
    GTPC_V1_IE_SELECTION_MODE,
    GTPC_V1_IE_TEID_DATA_I,
    GTPC_V1_IE_TEID_CTRL_PLANE,
    GTPC_V1_IE_TEID_DATA_II,
    GTPC_V1_IE_TEARDOWN_IND,
    GTPC_V1_IE_NSAPI,
    GTPC_V1_IE_RANAP_CAUSE,
    GTPC_V1_IE_RAB_CONTEXT,
    GTPC_V1_IE_RADIO_PRIORITY_SMS,
    GTPC_V1_IE_RADIO_PRIORITY,
    GTPC_V1_IE_PACKET_FLOW_ID,
    GTPC_V1_IE_CHARGING_CHARACTERISTICS,
    GTPC_V1_IE_TRACE_REFERENCE,
    GTPC_V1_IE_TRACE_TYPE,
    GTPC_V1_IE_MSNOT_REACHABLE_REASON,
    GTPC_V1_IE_CHARGING_ID = 127,
    GTPC_V1_IE_END_USER_ADDRESS,
    GTPC_V1_IE_MM_CONTEXT,
    GTPC_V1_IE_PDP_CONTEXT,
    GTPC_V1_IE_ACCESS_POINT_NAME,
    GTPC_V1_IE_PCO,
    GTPC_V1_IE_GSN_ADDRESS,
    GTPC_V1_IE_MSISDN,
    GTPC_V1_IE_QOS,
    GTPC_V1_IE_AUTH_Q,
    GTPC_V1_IE_TRAFFIC_FLOW_TEMPLATE,
    GTPC_V1_IE_TARGET_ID,
    GTPC_V1_IE_UTRAN,
    GTPC_V1_IE_RAB_SETUP,
    GTPC_V1_IE_EXTENSION_HEADER_TYPE_LIST,
    GTPC_V1_IE_TRIGGER_ID,
    GTPC_V1_IE_OMC_ID,
    GTPC_V1_IE_RAN_TRANSPORT_CONTAINER,
    GTPC_V1_IE_PDP_CONTEXT_PRIOR,
    GTPC_V1_IE_ADDITIONAL_RAB_SETUP,
    GTPC_V1_IE_SGSN_NUMBER,
    GTPC_V1_IE_COMMON_FLAGS,
    GTPC_V1_IE_APN_RESTRICTION,
    GTPC_V1_IE_RADIO_LCS,
    GTPC_V1_IE_RAT_TYPE,
    GTPC_V1_IE_USER_LOCATION_INFO,
    GTPC_V1_IE_MS_TIME_ZONE,
    GTPC_V1_IE_IMEI,
    GTPC_V1_IE_CAMEL_CHARGING,
    GTPC_V1_IE_MBMS_UE_CONTEXT,
    GTPC_V1_IE_TMGI,
    GTPC_V1_IE_RIM_ROUTING_ADDRESS,
    GTPC_V1_IE_MBMS_PCO,
    GTPC_V1_IE_MBMS_SA,
    GTPC_V1_IE_SOURCE_RNC_PDCP,
    GTPC_V1_IE_ADDITIONAL_TRACE_INFO,
    GTPC_V1_IE_HOP_COUNTER,
    GTPC_V1_IE_SELECTED_PLMN_ID,
    GTPC_V1_IE_MBMS_SESSION_ID,
    GTPC_V1_IE_MBMS_2G3G_ID,
    GTPC_V1_IE_ENHANCED_NSAPI,
    GTPC_V1_IE_MBMS_SESSION_DURATION,
    GTPC_V1_IE_ADDITIONAL_MBMS_TRACE_INFO,
    GTPC_V1_IE_MBMS_SESSION_REPETITION_NUMBER,
    GTPC_V1_IE_MBMS_TIME_TO_DATA_TRANSFER,
    GTPC_V1_IE_PS_HANDOVER_REQ,
    GTPC_V1_IE_BSS,
    GTPC_V1_IE_CELL_ID,
    GTPC_V1_IE_PDU_NUMBERS,
    GTPC_V1_IE_BSSGP_CAUSE,
    GTPC_V1_IE_REQUIRED_MBMS_BEARER_CAP,
    GTPC_V1_IE_RIM_ROUTING_ADDRESS_DISC,
    GTPC_V1_IE_PFCS,
    GTPC_V1_IE_PS_HANDOVER_XID,
    GTPC_V1_IE_MS_INFO_CHANGE,
    GTPC_V1_IE_DIRECT_TUNNEL_FLAGS,
    GTPC_V1_IE_CORRELATION_ID,
    GTPC_V1_IE_BEARER_CTRL_MODE,
    GTPC_V1_IE_CHARGING_GATEWAY_ADDRESS = 251,
    GTPC_V1_IE_PRIVATE_EXTENSION = 255,
    GTPC_V1_IE_MAX
};

/*! @name manage of parse state */
/* @{ */
static const U32    HAVE_MSISDN = (1<<31);  /*!< has msisdn */
static const U32    HAVE_FTEID_C= (1<<30);  /*!< has f_teid*/
static const U32    HAVE_FTEID_U= (1<<29);  /*!< has f_teid under Bearer Context */
static const U32    HAVE_AMBR   = (1<<28);  /*!< has ambr under Bearer Context */
static const U32    HAVE_BQOS   = (1<<27);  /*!< has Qos under Bearer Context */
static const U32    HAVE_RECORD = (1<<26);  /*!< lookuped msisdn */
static const U32    HAVE_IMSI   = (1<<25);  /*!< has imsi */
static const U32    HAVE_RECOVERY= (1<<24);  /*!< has recovery */
static const U32    HAVE_CAUSE  = (1<<23);   /*!< has cause */
static const U32    HAVE_MEI    = (1<<22);   /*!< has mei */
static const U32    HAVE_CHRGID = (1<<21);   /*!< has chargingid  */
static const U32    HAVE_EBI    = (1<<20);   /*!< has ebi  */
static const U32    HAVE_PCO    = (1<<19);   /*!< has pco  */
static const U32    HAVE_PAA    = (1<<18);   /*!< has paa  */
static const U32    HAVE_APN_R  = (1<<17);   /*!< has apn restriction  */
static const U32    HAVE_BQOS_R = (1<<16);   /*!< has bearer qos under root */
static const U32    HAVE_BCTX   = (1<<15);   /*!< has Bearer Context  */
static const U32    HAVE_EBI_B  = (1<<14);   /*!< has Bearer Ebi under Bearer Context */
static const U32    HAVE_AMBR_R = (1<<13);   /*!< has ambr under root */
static const U32    HAVE_PDN    = (1<<12);   /*!< has pdn  */
static const U32    HAVE_RAT    = (1<<11);   /*!< has rat type  */

//
static const U32    HAVE_V1             = (1<< 1); /*!< gtpcv1 */
static const U32    HAVE_V1_MSISDN      = (1<<30); /*!< gtpcv1: msisdn */
static const U32    HAVE_V1_TEID_DATA   = (1<<29); /*!< gtpcv1: data plane-teid */
static const U32    HAVE_V1_TEID_CTRL   = (1<<28); /*!< gtpcv1: control plane-teid */
static const U32    HAVE_V1_IPADR_DATA  = (1<<27); /*!< gtpcv1: data plane-ipaddres */
static const U32    HAVE_V1_IPADR_CTRL  = (1<<26); /*!< gtpcv1: control plane-ipaddres */
static const U32    HAVE_V1_IMSI        = (1<<25); /*!< gtpcv1: imsi */
static const U32    HAVE_V1_QOS         = (1<<24); /*!< gtpcv1: qos */
static const U32    HAVE_V1_PCO         = (1<<23); /*!< gtpcv1: pco */
static const U32    HAVE_V1_NSAPI       = (1<<22); /*!< gtpcv1: nsapi */
static const U32    HAVE_V1_CAUSE       = (1<<21); /*!< gtpcv1: cause */

//
static const U32    HAVE_PCO_IPCP_P_DNS     = (1<<10);  /*!< pco:ipcp primary dns */
static const U32    HAVE_PCO_IPCP_S_DNS     = (1<<9);   /*!< pco:ipcp secondry dns */
static const U32    HAVE_PCO_IPV4_LINK_MTU  = (1<<8);   /*!< pco:ipv4 link mtu request */
static const U32    HAVE_PCO_NONIP_LINK_MTU = (1<<7);   /*!< pco:non-ip link mtu request */
static const U32    HAVE_PCO_DNS_IPV4       = (1<<6);   /*!< pco:dns server ipv4 request */
static const U32    HAVE_PCO_DNS_IPV6       = (1<<5);   /*!< pco:dns server ipv6 request */

static const U32    HAVE_RES_ORDER          = (1<<0);   /*!< Debug Use. */

#ifdef __clang__
static const U32    INCLUDED_ALL= (HAVE_MSISDN|HAVE_FTEID_C|HAVE_FTEID_U|HAVE_AMBR_R|HAVE_BQOS);
static const U32    PCO_ALL= (HAVE_PCO_IPV4_LINK_MTU|HAVE_PCO_NONIP_LINK_MTU|HAVE_PCO_DNS_IPV6|HAVE_PCO_DNS_IPV4);
static const U32    HAVE_PCO_DNS_IPV64 = (HAVE_PCO_DNS_IPV4|HAVE_PCO_DNS_IPV6);
static const U32    REQUIRED_CREATE_PDP = (HAVE_V1|HAVE_V1_MSISDN|HAVE_V1_TEID_DATA|HAVE_V1_TEID_CTRL|HAVE_V1_IPADR_DATA|HAVE_V1_IPADR_CTRL|HAVE_V1_IMSI);
static const U32    REQUIRED_UPDATE_PDP = (HAVE_V1|HAVE_V1_TEID_DATA|HAVE_V1_IPADR_DATA|HAVE_V1_IPADR_CTRL);
static const U32    REQUIRED_DELETE_PDP = (HAVE_V1|HAVE_V1_NSAPI);

#else
#define INCLUDED_ALL (HAVE_MSISDN|HAVE_FTEID_C|HAVE_FTEID_U|HAVE_AMBR_R|HAVE_BQOS)
#define PCO_ALL (HAVE_PCO_IPV4_LINK_MTU|HAVE_PCO_NONIP_LINK_MTU|HAVE_PCO_DNS_IPV6|HAVE_PCO_DNS_IPV4)
#define HAVE_PCO_DNS_IPV64 (HAVE_PCO_DNS_IPV4|HAVE_PCO_DNS_IPV6)
#define REQUIRED_CREATE_PDP (HAVE_V1|HAVE_V1_MSISDN|HAVE_V1_TEID_DATA|HAVE_V1_TEID_CTRL|HAVE_V1_IPADR_DATA|HAVE_V1_IPADR_CTRL|HAVE_V1_IMSI)
#define REQUIRED_UPDATE_PDP (HAVE_V1|HAVE_V1_TEID_DATA|HAVE_V1_IPADR_DATA|HAVE_V1_IPADR_CTRL)
#define REQUIRED_DELETE_PDP (HAVE_V1|HAVE_V1_NSAPI)
#endif

static const U8     PCO_DNS_IPV64_VAL[52] = {
    // Primary IPV6 DNS 2001:4860:4860::8888
    0x00,   0x03,
    0x10,   0x20, 0x01, 0x48, 0x60,   0x48, 0x60, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x88, 0x88,
    // Secondary IPV6 DNS 2001:4860:4860::8844
    0x00,   0x03,
    0x10,   0x20, 0x01, 0x48, 0x60,   0x48, 0x60, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x88, 0x44,
    // Primary IPV4 DNS
    0x00,   0x0d,
    0x04,   0x08, 0x08, 0x08, 0x08, //8.8.8.8
    // Secondary IPV4 DNS
    0x00,   0x0d,
    0x04,   0x08, 0x08, 0x04, 0x04, //8.8.4.4
};
static const U8     PCO_DNS_IPV6_VAL[38] = {
    // Primary IPV6 DNS 2001:4860:4860::8888
    0x00,   0x03,
    0x10,   0x20, 0x01, 0x48, 0x60,   0x48, 0x60, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x88, 0x88,
    // Secondary IPV6 DNS 2001:4860:4860::8844
    0x00,   0x03,
    0x10,   0x20, 0x01, 0x48, 0x60,   0x48, 0x60, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x88, 0x44,
};
static const U8     PCO_DNS_IPV4_VAL[14] = {
    // Primary IPV4 DNS
    0x00,   0x0d,
    0x04,   0x08, 0x08, 0x08, 0x08, //8.8.8.8
    // Secondary IPV4 DNS
    0x00,   0x0d,
    0x04,   0x08, 0x08, 0x04, 0x04, //8.8.4.4
};
static const U8     PCO_MTU_IPV4_VAL[5] = {
    0x00,   0x10,
    0x02,   0x05, 0xb8,     // 1464 - mtu
};
static const U8     PCO_MTU_NONIP_VAL[5] = {
    0x00,   0x15,
    0x02,   0x05, 0xb8,     // 1464 - mtu
};
static const U8     PCO_DNS_IPCP_IPV4_VAL[19] = {
    0x80,   0x21,   0x10,
    0x03,   0x00,   0x00,   0x10,
    // Primary IPV4 DNS
    0x81,   0x06,
    0x08, 0x08, 0x08, 0x08, //8.8.8.8
    // Secondary IPV4 DNS
    0x83,   0x06,
    0x08, 0x08, 0x04, 0x04, //8.8.4.4
};



/* @} */


/*! @enum PEID
  @brief
  gtpc private extension index\n
  1 index=32bit\n
*/
enum GTPC_PEID{
    SGW_GTPU_IPV4 = 0,    /*!< sgw gtpu ipv4 */
    SGW_GTPU_TEID,        /*!< sgw gtpu teid */
    UE_IPV4,              /*!< ue ipv4 */
    UE_TEID,              /*!< ue teid */
    PGW_GTPU_IPV4,        /*!< pgw gtpu ipv4 */
    GTPC_PEID_MAX = 5     /*!< index max value,5x4=20+4(common header)+venderid(2)+padding(2)+4(magic)=32 */
};

#ifdef __clang__
#pragma pack(1)
#endif

/*! @struct gtp_item
    @brief
    gtp_packet, gtp item offset\n
    \n
*/
typedef struct gtp_item {
    TAILQ_ENTRY(gtp_item) link;
    U16     type;
    U16     length;
    U16     offset;
}ATTRIBUTE_PACKED gtp_item_t,*gtp_item_ptr;


/*! @struct gtp_packet
    @brief
    gtp packet, generated(gtp[c/u])\n
    \n
*/
typedef struct gtp_packet{
    TAILQ_HEAD(items, gtp_item)  items;
    U16     count;
    U16     length;
    U16     offset;
    U8*     packet;
    // copy address from any packet
    struct sockaddr_in  saddr;
    ssize_t             saddrlen;
    struct sockaddr_in  caddr;
    ssize_t             caddrlen;
}ATTRIBUTE_PACKED gtp_packet_t,*gtp_packet_ptr;


/*! @struct gtpc_header
    @brief
    gtpc header\n
    version 2\n
*/
typedef struct gtpc_header {    /* According to 3GPP TS 29.274v11.5.0. */
    struct _v2_flags{
        U8 spare:3;
        U8 teid:1;
        U8 piggy:1;
        U8 version:3;
    }v2_flags;
    U8	type;
    U16	length;
    union _t{
        U32	teid;
        struct _sq{
            U32 seqno:24;
            U32 padd:8;
        }sq;
    }t;
    union _q{
        U32    seqno;
        struct _sq_t{
            U32 seqno:24;
            U32 padd:8;
        }sq_t;
    }q;
} ATTRIBUTE_PACKED gtpc_header_t,*gtpc_header_ptr;

/*! @struct gtpc_v1_header
    @brief
    gtpc header\n
    version 1\n
*/
typedef struct gtpc_v1_header {
    union _f{
        U8  flags;
        struct _gtpc_v1_flags{
            U8  npdu:1;
            U8  sequence:1;
            U8  extension:1;
            U8  reserve:1;
            U8  proto:1;
            U8  version:3;
        }gtpc_v1_flags;
    }f;
    U8      type;
    U16     length;
    U32     tid;
} __attribute__ ((packed)) gtpc_v1_header_t,*gtpc_v1_header_ptr;

#define GTPC_V1_NSE_MASK    (0x07)

/*! @struct gtpc_inst
    @brief
    gtpc item, instance\n
    \n
*/
typedef struct gtpc_inst{
    U8 instance:4;
    U8 spare:4;
}ATTRIBUTE_PACKED gtpc_inst_t,*gtpc_inst_ptr;

/*! @struct gtpc_numberdigit
    @brief
    gtpc item, numeric\n
    \n
*/
typedef struct gtpc_numberdigit{
    U8 low:4;
    U8 high:4;
}ATTRIBUTE_PACKED gtpc_numberdigit_t,*gtpc_numberdigit_ptr;

/*! @struct gtpc_comm_header
    @brief
    gtpc item, common\n
    \n
*/
typedef struct gtpc_comm_header{
    U8    type;
    U16  length;
    gtpc_inst_t         inst;
}ATTRIBUTE_PACKED gtpc_comm_header_t,*gtpc_comm_header_ptr;

/*! @struct gtpc_imsi
    @brief
    gtpc item, IMSI\n
    \n
*/
typedef struct gtpc_imsi{
    gtpc_comm_header_t  head;   // IMSI_LEN
    gtpc_numberdigit_t  digits[GTPC_IMSI_LEN];
}ATTRIBUTE_PACKED gtpc_imsi_t,*gtpc_imsi_ptr;

/*! @struct gtpc_cause
    @brief
    gtpc item, CAUSE\n
    \n
*/
typedef struct gtpc_cause_bit{
    U8 spare:5;
    U8 pce:1;
    U8 bce:1;
    U8 cs:1;
}ATTRIBUTE_PACKED gtpc_cause_bit_t,*gtpc_cause_bit_ptr;

typedef struct gtpc_cause{
    gtpc_comm_header_t  head;   // 6 or 10
    U8                  cause;
    gtpc_cause_bit_t    bit;
    U8                  offending_ie;
    U16                 length;
    gtpc_inst_t         inst;
}ATTRIBUTE_PACKED gtpc_cause_t,*gtpc_cause_ptr;

/*! @struct gtpc_recovery
    @brief
    gtpc item, RECOVERY(Restart Counter)\n
    \n
*/
typedef struct gtpc_recovery{
    gtpc_comm_header_t  head;   // 1
    U8                recovery_restart_counter;
}ATTRIBUTE_PACKED gtpc_recovery_t,*gtpc_recovery_ptr;

/*! @struct gtpc_apn
    @brief
    grpc item, APN(Access Point Name)\n
    \n
*/
typedef struct gtpc_apn{
    gtpc_comm_header_t  head;   // over 25 octet
    U8                apn[GTPC_APN_LEN];
}ATTRIBUTE_PACKED gtpc_apn_t,*gtpc_apn_ptr;

/*! @struct gtpc_ambr
    @brief
    grpc item, AMBR\n
    \n
*/
typedef struct gtpc_ambr{
    gtpc_comm_header_t  head;   // 12
    U32               uplink;
    U32               downlink;
}ATTRIBUTE_PACKED gtpc_ambr_t,*gtpc_ambr_ptr;

/*! @struct gtpc_ebi
    @brief
    grpc item, EBI(EPS Bearer ID)\n
    \n
*/
typedef struct gtpc_ebi{
    gtpc_comm_header_t  head;   // 1
    gtpc_numberdigit_t  ebi;
}ATTRIBUTE_PACKED gtpc_ebi_t,*gtpc_ebi_ptr;

/*! @struct gtpc_mei
    @brief
    grpc item, MEI\n
    \n
*/
typedef struct gtpc_mei{
    gtpc_comm_header_t  head;   // MEI_LEN
    gtpc_numberdigit_t  digits[GTPC_MEI_LEN];
}ATTRIBUTE_PACKED gtpc_mei_t,*gtpc_mei_ptr;

/*! @struct gtpc_msisdn
    @brief
    grpc item, MSISDN\n
    \n
*/
typedef struct gtpc_msisdn{
    gtpc_comm_header_t  head;   // MSISDN_LEN
    gtpc_numberdigit_t  digits[GTPC_MSISDN_LEN];
}ATTRIBUTE_PACKED gtpc_msisdn_t,*gtpc_msisdn_ptr;

/*! @struct gtpc_indication_flags
    @brief
    grpc item, INDICATION FLAGS\n
    \n
*/
typedef struct gtpc_indication_flags_bit{
    U32 sgwci:1; U32 israi:1; U32 isrsi:1; U32 oi:1;
    U32 dfi:1;   U32 hi:1;    U32 dtf:1;   U32 daf:1;
    //
    U32 msv:1;   U32 si:1;    U32 pt:1;    U32 p:1;
    U32 crsi:1;  U32 cfsi:1;  U32 uimsi:1; U32 sqci:1;
    //
    U32 ccrsi:1; U32 israu:1; U32 mbmdt:1; U32 s4af:1;
    U32 s6af:1;  U32 srni:1;  U32 pbic:1;  U32 retloc:1;
    //
    U32 cpsr:1;  U32 clii:1;  U32 spare:6;
}ATTRIBUTE_PACKED gtpc_indication_flags_bit_t,*gtpc_indication_flags_bit_ptr;


typedef struct gtpc_indication_flags{
    gtpc_comm_header_t  head;   // 4
    gtpc_indication_flags_bit_t bit;
    U16       padd;
}ATTRIBUTE_PACKED gtpc_indication_flags_t,*gtpc_indication_flags_ptr;

/*! @struct gtpc_pco_option_header
    @brief
    grpc item, PCO - option\n
    \n
*/
typedef struct gtpc_pco_option_header{
    U8 prot:3;
    U8 spare:4;
    U8 extension:1;
}ATTRIBUTE_PACKED gtpc_pco_option_header_t,*gtpc_pco_option_header_ptr;

/*! @struct gtpc_pco
    @brief
    grpc item, PCO(Protocol Configuration Options)\n
    \n
*/
typedef struct gtpc_pco{
    gtpc_comm_header_t  head;
    gtpc_pco_option_header_t pco_head;
    U8                pco[GTPC_PCO_LEN];    // 3gpp. ts 24.008 10.5.6.3 Protocol configuration options.
}ATTRIBUTE_PACKED gtpc_pco_t,*gtpc_pco_ptr;


/*! @struct gtpc_pco_option_item
    @brief
    grpc item, PCO - option item\n
    deined from RFC1661,RFC3232 \n
*/
typedef struct gtpc_pco_option_item{
    U16       protocol_or_container_id;
    U8        length;
    U8        contents[GTPC_PCO_IPCP_LEN];
}ATTRIBUTE_PACKED gtpc_pco_option_item_t,*gtpc_pco_option_item_ptr;

/*! @struct gtpc_pco_ipcp_cfg_option
    @brief
    grpc item, PCO - IPCP configuration option\n
*/
typedef struct gtpc_pco_ipcp_cfg_option{
    U8        type;
    U8        length;
    U32       value;
}ATTRIBUTE_PACKED gtpc_pco_ipcp_cfg_option_t,*gtpc_pco_ipcp_cfg_option_ptr;


/*! @struct gtpc_pco_ipcp
    @brief
    grpc item, PCO - IPCP\n
    deined from RFC1661,RFC3232 \n
*/
typedef struct gtpc_pco_ipcp_cfg{
    U8        code;
    U8        identifier;
    U16       length;
    gtpc_pco_ipcp_cfg_option_t  opt0;
    gtpc_pco_ipcp_cfg_option_t  opt1;
}ATTRIBUTE_PACKED gtpc_pco_ipcp_cfg_t,*gtpc_pco_ipcp_cfg_ptr;


/*! @struct gtpc_paa
    @brief
    grpc item, PAA(PDN Address Allocation)\n
    \n
*/
typedef struct gtpc_paa_bit{
    U8 pdn_type:3;
    U8 spare:5;
}ATTRIBUTE_PACKED gtpc_paa_bit_t,*gtpc_paa_bit_ptr;

typedef struct gtpc_paa{
    gtpc_comm_header_t  head;
    gtpc_paa_bit_t      bit;
    U8                paa[GTPC_PAA_ADRS_LEN];   // 3gpp. ts 29.274 8.14-1:PDN Address Allocation.
}ATTRIBUTE_PACKED gtpc_paa_t,*gtpc_paa_ptr;


/*! @struct gtpc_bearer_qos
    @brief
    grpc item, Bearer QOS(Bearer Quality of Service)\n
    \n
*/
typedef struct gtpc_bearer_qos_bit{
    U8 spare0:1;
    U8 pci:1;
    U8 pl:4;
    U8 spare1:1;
    U8 pvi:1;
}ATTRIBUTE_PACKED gtpc_bearer_qos_bit_t,*gtpc_bearer_qos_bit_ptr;


typedef struct gtpc_bearer_qos{
    gtpc_comm_header_t  head;
    gtpc_bearer_qos_bit_t   bit;
    U8        qci;
    U8        rate[20];
}ATTRIBUTE_PACKED gtpc_bearer_qos_t,*gtpc_bearer_qos_ptr;

/*! @struct gtpc_rat
    @brief
    grpc item, RAT TYPE\n
    \n
*/
typedef struct gtpc_rat{
    gtpc_comm_header_t  head;   // 1
    U8                rat_type;
}ATTRIBUTE_PACKED gtpc_rat_t,*gtpc_rat_ptr;

/*! @struct gtpc_serving_network
    @brief
    grpc item, SERVING NETWORK\n
    \n
*/
typedef struct gtpc_serving_network{
    gtpc_comm_header_t  head;       // 3
    gtpc_numberdigit_t  digits[3];
}ATTRIBUTE_PACKED gtpc_serving_network_t,*gtpc_serving_network_ptr;


/*! @struct gtpc_uli_cgi
    @brief
    grpc item, ULI(User Location Information)\n
    \n
*/
typedef struct gtpc_uli_cgi{
    gtpc_numberdigit_t      digits[3];
    U16                  location_area_code;
    U16                  cell_identity;
}ATTRIBUTE_PACKED gtpc_uli_cgi_t,*gtpc_uli_cgi_ptr;

/*! @struct gtpc_uli_sai
    @brief
    grpc item, ULI(User Location Information)\n
    \n
*/
typedef struct gtpc_uli_sai{
    gtpc_numberdigit_t      digits[3];
    U16                  location_area_code;
    U16                  service_area_code;
}ATTRIBUTE_PACKED gtpc_uli_sai_t,*gtpc_uli_sai_ptr;

/*! @struct gtpc_uli_rai
    @brief
    grpc item, ULI - RAI\n
    \n
*/
typedef struct gtpc_uli_rai{
    gtpc_numberdigit_t      digits[3];
    U16                  location_area_code;
    U16                  routing_area_code;
}ATTRIBUTE_PACKED gtpc_uli_rai_t,*gtpc_uli_rai_ptr;

/*! @struct gtpc_uli_tai
    @brief
    grpc item, ULI - TAI\n
    \n
*/
typedef struct gtpc_uli_tai{
    gtpc_numberdigit_t      digits[3];
    U16                  tracking_area_code;
}ATTRIBUTE_PACKED gtpc_uli_tai_t,*gtpc_uli_tai_ptr;

/*! @struct gtpc_uli_ecgi
    @brief
    gtpc item, ULI - ECGI\n
    \n
*/
typedef struct gtpc_uli_ecgi{
    gtpc_numberdigit_t  digits[3];
    gtpc_inst_t         eci;
    U8                eutran_cell_identifier[3];
}ATTRIBUTE_PACKED gtpc_uli_ecgi_t,*gtpc_uli_ecgi_ptr;

/*! @struct gtpc_uli_lai
    @brief
    gtpc item, ULI -  LAI\n
    \n
*/
typedef struct gtpc_uli_lai{
    gtpc_numberdigit_t      digits[3];
    U16                  location_area_code;
}ATTRIBUTE_PACKED gtpc_uli_lai_t,*gtpc_uli_lai_ptr;

/*! @struct gtpc_uli
    @brief
    gtpc item, ULI\n
    \n
*/
typedef struct gtpc_uli_bit{
    U8 cgi:1;
    U8 sai:1;
    U8 rai:1;
    U8 tai:1;
    U8 ecgi:1;
    U8 lai:1;
    U8 spare:2;
}ATTRIBUTE_PACKED gtpc_uli_bit_t,*gtpc_uli_bit_ptr;

typedef struct gtpc_uli{
    gtpc_comm_header_t  head;   // over 12
    gtpc_uli_bit_t      bit;
    uint8_t          blk[38];   // rai or tai and ecgi
}ATTRIBUTE_PACKED gtpc_uli_t,*gtpc_uli_ptr;


/*! @struct gtpc_f_teid
    @brief
    gtpc item, F-TEID(Fully Qualified TEID)\n
    \n
*/
typedef struct gtpc_f_teid_bit{
    U8 iftype:5;
    U8 spare:1;
    U8 v6:1;
    U8 v4:1;
}ATTRIBUTE_PACKED gtpc_f_teid_bit_t,*gtpc_f_teid_bit_ptr;


typedef struct gtpc_f_teid{
    gtpc_comm_header_t  head;   // over 13 or 25
    gtpc_f_teid_bit_t   bit;
    U32              teid_grekey;
    U8                blk[16];
}ATTRIBUTE_PACKED gtpc_f_teid_t,*gtpc_f_teid_ptr;

/*! @struct gtpc_charging_id
    @brief
    gtpc item, Charging ID\n
    \n
*/
typedef struct gtpc_charging_id{
    gtpc_comm_header_t  head;       // 4
    U32              charging_id;
}ATTRIBUTE_PACKED gtpc_charging_id_t,*gtpc_charging_id_ptr;

/*! @struct gtpc_pdn_type
    @brief
    gtpc item, PDN Type\n
    \n
*/
typedef struct gtpc_pdn_type_bit{
    U8 pdn_type:3;
    U8 spare:5;
}ATTRIBUTE_PACKED gtpc_pdn_type_bit_t,*gtpc_pdn_type_bit_ptr;

typedef struct gtpc_pdn_type{
    gtpc_comm_header_t  head;       // 1gtpc_ebi_t
    gtpc_pdn_type_bit_t bit;
}ATTRIBUTE_PACKED gtpc_pdn_type_t,*gtpc_pdn_type_ptr;

/*! @struct gtpc_apn_restriction
    @brief
    gtpc item, APN Restriction\n
    \n
*/
typedef struct gtpc_apn_restriction{
    gtpc_comm_header_t  head;       // 1
    U8                  restriction_type;
}ATTRIBUTE_PACKED gtpc_apn_restriction_t,*gtpc_apn_restriction_ptr;

/*! @struct gtpc_selection_mode
    @brief
    gtpc item, Selection Mode\n
    \n
*/
typedef struct gtpc_selection_mode_bit{
    U8 spare:6;
    U8 select_mode:2;
}ATTRIBUTE_PACKED gtpc_selection_mode_bit_t,*gtpc_selection_mode_bit_ptr;

typedef struct gtpc_selection_mode{
    gtpc_comm_header_t  head;       // 1
    gtpc_selection_mode_bit_t   bit;
}ATTRIBUTE_PACKED gtpc_selection_mode_t,*gtpc_selection_mode_ptr;

/*! @struct gtpc_bearer_ctx
    @brief
    gtpc item, Bearer Context\n
    \n
*/
typedef struct gtpc_bearer_ctx{
    gtpc_comm_header_t  head;
    U8      child[256];
    U16     offset;
}ATTRIBUTE_PACKED gtpc_bearer_ctx_t,*gtpc_bearer_ctx_ptr;

/*! @struct gtpc_bearer_ctx_to_be_modified
    @brief
    gtpc item, Bearer Context to be modified\n
    \n
*/
typedef struct gtpc_bearer_ctx_to_be_modified{
    gtpc_comm_header_t  head;
    U8      child[256];
    U16     offset;
}ATTRIBUTE_PACKED gtpc_bearer_ctx_to_be_modified_t,*gtpc_bearer_ctx_to_be_modified_ptr;

/*! @struct gtpc_echo_request
    @brief
    gtpc item, \n
    \n
*/
typedef struct gtpc_echo_request{
    gtpc_recovery_t    recovery;
}ATTRIBUTE_PACKED gtpc_echo_request_t,*gtpc_echo_request_ptr;

/*! @struct gtpc_echo_response
    @brief
    grpc item, \n
    \n
*/
typedef struct gtpc_echo_response{
    gtpc_recovery_t recovery;
    gtpc_cause_t    cause;
}ATTRIBUTE_PACKED gtpc_echo_response_t,*gtpc_echo_response_ptr;

/*! @struct private_extension_hdr
    @brief
    user extend item defined\n
    total=32 bytes fixed\n
*/
typedef struct gtpc_private_extension{
    gtpc_comm_header_t  head;       /*!< common header */
    U16                 venderid;   /*!< vendor id */
    U16                 padd;       /*!< padding */
    U32                 magic;      /*!< magic */
    U32                 value[GTPC_PEID_MAX]; /*!< values */
}ATTRIBUTE_PACKED gtpc_private_extension_t,*gtpc_private_extension_ptr;


/*! @struct gtpc_imsi_v1
    @brief
    gtpv1-citem, IMSI\n
    \n
*/
typedef struct gtpc_imsi_v1{
    U8  type;
    gtpc_numberdigit_t  digits[GTPC_IMSI_LEN];
}ATTRIBUTE_PACKED gtpc_imsi_v1_t,*gtpc_imsi_v1_ptr;

/*! @struct gtpc_msisdn_v1
    @brief
    gtpv1-citem, MSISDN\n
    \n
*/
typedef struct gtpc_msisdn_v1{
    U8  type;
    U16 len;
    gtpc_numberdigit_t  indicator;
    gtpc_numberdigit_t  digits[GTPC_MSISDN_LEN];
}ATTRIBUTE_PACKED gtpc_msisdn_v1_t,*gtpc_msisdn_v1_ptr;

/*! @struct gtpc_cause_v1
    @brief
    gtpv1-citem, CAUSE\n
    \n
*/
typedef struct gtpc_cause_v1{
    U8  type;
    U8  cause;
}ATTRIBUTE_PACKED gtpc_cause_v1_t,*gtpc_cause_v1_ptr;

/*! @struct gtpc_teid_v1
    @brief
    gtpv1-citem, Tunnel Identifier\n
    \n
*/
typedef struct gtpc_gsn_address_v1{
    U8  type;
    U16 len;
    U8  adrs[GTPC_V1_GSNADDR_LEN];
}ATTRIBUTE_PACKED gtpc_gsn_address_v1_t,*gtpc_gsn_address_v1_ptr;

/*! @struct gtpc_teid_v1
    @brief
    gtpv1-citem, Tunnel Identifier\n
    \n
*/
typedef struct gtpc_teid_v1{
    U8  type;
    U32 teid;
}ATTRIBUTE_PACKED gtpc_teid_v1_t,*gtpc_teid_v1_ptr;


/*! @struct gtpc_reordering_v1
    @brief
    gtpv1-citem, Reorder\n
    \n
*/
typedef struct gtpc_reordering_v1{
    U8  type;
    U8  value;
}ATTRIBUTE_PACKED gtpc_reordering_v1_t,*gtpc_reordering_v1_ptr;

/*! @struct gtpc_recovery_v1
    @brief
    gtpv1-citem, Recovery\n
    \n
*/
typedef struct gtpc_recovery_v1{
    U8  type;
    U8  counter;
}ATTRIBUTE_PACKED gtpc_recovery_v1_t,*gtpc_recovery_v1_ptr;

/*! @struct gtpc_end_user_address_v1
    @brief
    gtpv1-citem, Recovery\n
    \n
*/
typedef struct gtpc_end_user_address_v1{
    U8  type;
    U16 len;
    union {
        U8  flags;
        struct {
            U8  pdp_type_organization:4;
            U8  reserved:4;
        };
    };
    U8  pdp_type_number;
    U32 pdp_address;
}ATTRIBUTE_PACKED gtpc_end_user_address_v1_t,*gtpc_end_user_address_v1_ptr;

/*! @struct gtpc_qos_v1
    @brief
    gtpv1-citem, QOS\n
    \n
*/
typedef struct gtpc_qos_v1{
    U8  type;
    U16 len;
    U8  payload[GTPC_V1_QOS_PAYLOAD_SZ];
}ATTRIBUTE_PACKED gtpc_qos_v1_t,*gtpc_qos_v1_ptr;


/*! @struct gtpc_pco_v1
    @brief
    gtpv1-citem, PCO\n
    \n
*/
typedef struct gtpc_pco_v1{
    U8  type;
    U16 len;
    gtpc_pco_option_header_t pco_head;
    U8  pco[GTPC_PCO_LEN];              // 3gpp. ts 24.008 10.5.6.3 Protocol configuration options.
}ATTRIBUTE_PACKED gtpc_pco_v1_t,*gtpc_pco_v1_ptr;


/*! @struct gtpc_nsapi_v1
    @brief
    gtpv1-citem, PCO\n
    \n
*/
typedef struct gtpc_nsapi_v1{
    U8  type;
    union {
        U8  flags;
        struct {
            U8  nsapi_value:4;
            U8  reserved:4;
        };
    };
}ATTRIBUTE_PACKED gtpc_nsapi_v1_t,*gtpc_nsapi_v1_ptr;


/*! @struct gtpc_charginid_v1
    @brief
    gtpv1-citem, Charging id\n
    \n
*/
typedef struct gtpc_charginid_v1{
    U8  type;
    U32 id;
}ATTRIBUTE_PACKED gtpc_charginid_v1_t,*gtpc_charginid_v1_ptr;


/*! @struct gtpc_teardown_ind_v1
    @brief
    gtpv1-citem, TearDown Indication\n
    \n
*/
typedef struct gtpc_teardown_ind_v1{
    U8  type;
    union {
        U8  flags;
        struct {
            U8  reserved:7;
            U8  teardown_ind_value:1;
        };
    };
}ATTRIBUTE_PACKED gtpc_teardown_ind_v1_t,*gtpc_teardown_ind_v1_ptr;


/*! @struct messge_in_gtpc_v1
    @brief
    gtpv1-c message parse\n
    \n
*/
typedef struct messge_in_gtpc_v1{
    U8      type;
    int     len;
    void*   parser;
}ATTRIBUTE_PACKED messge_in_gtpc_v1_t,*messge_in_gtpc_v1_ptr;



#define GTPCV1_IE_SIMPLE_HEADER_LEN(s)      (sizeof(s)-sizeof(U8))
#define GTPCV1_IE_VARIABLE_HEADER_LEN(s)    (sizeof(s)-sizeof(U8)-sizeof(U16))

/*! @struct gtpc_parse_state
    @brief
    Create Session Request processing\n
    user conteiner\n
*/
typedef struct gtpc_parse_state{
    U32                 gtph_seqno;     /*!< number of sequence */
    U32                 gtph_teid;      /*!< tunnel identifier */
    U32                 flag;           /*!< state flags */
    U32                 flag_v1;        /*!< state flags(v1) */
    U8                  reqtype_v1;     /*!< request type on gtpcv1 */
    //
    gtpc_msisdn_t       msisdn;         /*!< msisdn number  : parse from packet */
    gtpc_f_teid_t       c_teid;         /*!< sgw gtp-c teid : .. */
    gtpc_f_teid_t       u_teid;         /*!< sgw gtp-u teid : .. */
    gtpc_ambr_t         ambr;           /*!< ambr(Bearer)   : .. */
    gtpc_bearer_qos_t   bqos;           /*!< bearer qos(bearer):.. */
    gtpc_imsi_t         imsi;           /*!< imsi number    : .. */
    gtpc_recovery_t     recovery;       /*!< recovery       : .. */
    gtpc_cause_t        cause;          /*!< cause          : .. */
    gtpc_mei_t          mei;            /*!< mei            : .. */
    gtpc_charging_id_t  charg;          /*!< charging id    : ..  */
    gtpc_ebi_t          ebi_r;          /*!< ebi            : .. */
    gtpc_pco_t          pco;            /*!< pco            : .. */
    gtpc_paa_t          paa;            /*!< paa            : .. */
    gtpc_apn_restriction_t apn_r;       /*!< apn ristriction: .. */
    gtpc_bearer_qos_t   bqos_r;         /*!< bearer qos     : .. */
    gtpc_ebi_t          ebi_b;          /*!< ebi (bearer)   : .. */
    gtpc_ambr_t         ambr_r;         /*!< ambr           : .. */
    gtpc_pdn_type_t     pdn;            /*!< pdn_type       : .. */
    gtpc_rat_t          rat;            /*!< rat type       : .. */
    char                qos[256];       /*!< qos            : .. */
    U8                  qos_len;        /*!< qos            : .. */
    U8                  nsapi;          /*!< nsapi          : .. */
    U8                  cause_v1;       /*!< cause          : .. */
    //
    char                ue_ipv[32];     /*!< ue ipaddress   : lookup from database */
    struct in_addr      ue_ipv_n;       /*!< ue ip addr -> in_addr */
    U32                 sgw_c_teid;     /*!< sgw gtp-c teid : lookup from database */
    U32                 pgw_teid;       /*!< pgw gtp-[c/u] teid : .. */
    char                pgw_u_ipv[32];  /*!< pgw gtp-u ip addr  : .. */
    struct in_addr      pgw_u_ipv_n;    /*!< pgw gtp-u ip addr -> in_addr */
    char                pgw_c_ipv[32];  /*!< pgw gtp-c ip addr  : ..*/
    struct in_addr      pgw_c_ipv_n;    /*!< pgw gtp-c ip addr -> in_addr */
    U64                 imsi_num;       /*!< imsi nubmer */
    U64                 msisdn_num_;    /*!< msisdn number */
    U64                 bitrate_s5;     /*!< bitrate s5     : .. */
    U64                 bitrate_sgi;    /*!< bitrate sgi    : .. */
    char                dns[256];       /*!< dns : .. */
    U8                  dns_len;        /*!< dns : data length */
    U8                  ebi;            /*!< ebi : .. */
}ATTRIBUTE_PACKED gtpc_parse_state_t,*gtpc_parse_state_ptr;


#ifdef __clang__
#pragma options align=reset
#endif

#endif //MIXI_PGW_GTPC_H
