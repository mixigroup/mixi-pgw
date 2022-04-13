//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_CTRLPLANE_DEF_H
#define MIXIPGW_TOOLS_CTRLPLANE_DEF_H

#include "mixipgw_tools_def.h"

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
#define GTPC_PCO_IPCP                           htons(0x8021)
#define GTPC_PCO_DNS                            htons(0x000d)

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

#define DEBUG_CTRL_UDP_PORT                     55667


namespace MIXIPGW_TOOLS{
  typedef struct gtpc_header {    /* According to 3GPP TS 29.274v11.5.0. */
      union _c{
          struct _v2_flags{
              __u8 spare:3;
              __u8 teid:1;
              __u8 piggy:1;
              __u8 version:3;
          }v2_flags;
          __u8 flags;
      }c;
      __u8	type;
      __be16	length;
      union _t{
          __be32	teid;
          struct _sq{
              __u32 seqno:24;
              __u32 padd:8;
          }sq;
      }t;
      union _q{
          __be32    seqno;
          struct _sq{
              __u32 seqno:24;
              __u32 padd:8;
          }sq;
      }q;
  } __attribute__ ((packed)) gtpc_header_t,*gtpc_header_ptr;

  typedef struct gtpc_inst{
      __u8 instance:4;
      __u8 spare:4;
  }__attribute__ ((packed)) gtpc_inst_t,*gtpc_inst_ptr;

  typedef struct gtpc_numberdigit{
      __u8 low:4;
      __u8 high:4;
  }__attribute__ ((packed)) gtpc_numberdigit_t,*gtpc_numberdigit_ptr;

  typedef struct gtpc_comm_header{
      __u8    type;
      __be16  length;
      gtpc_inst_t         inst;
  }__attribute__ ((packed)) gtpc_comm_header_t,*gtpc_comm_header_ptr;

  // IMSI
  typedef struct gtpc_imsi{
      gtpc_comm_header_t  head;   // IMSI_LEN
      gtpc_numberdigit_t  digits[GTPC_IMSI_LEN];
  }__attribute__ ((packed)) gtpc_imsi_t,*gtpc_imsi_ptr;

  // CAUSE
  typedef struct gtpc_cause{
      gtpc_comm_header_t  head;   // 6 - 10
      __u8                cause;
      union _c {
          struct _bit{
              __u8 spare:5;
              __u8 pce:1;
              __u8 bce:1;
              __u8 cs:1;
          }bit;
          __u8 flags;
      }c;
      __u8                offending_ie;
      __be16              length;
      gtpc_inst_t         inst;
  }__attribute__ ((packed)) gtpc_cause_t,*gtpc_cause_ptr;

  // RECOVERY(Restart Counter)
  typedef struct gtpc_recovery{
      gtpc_comm_header_t  head;   // 1
      __u8                recovery_restart_counter;
  }__attribute__ ((packed)) gtpc_recovery_t,*gtpc_recovery_ptr;

  // APN(Access Point Name)
  typedef struct gtpc_apn{
      gtpc_comm_header_t  head;   // over 25 octet
      __u8                apn[GTPC_APN_LEN];
  }__attribute__ ((packed)) gtpc_apn_t,*gtpc_apn_ptr;

  typedef struct gtpc_ambr{
      gtpc_comm_header_t  head;   // 12
      __u32               uplink;
      __u32               downlink;
  }__attribute__ ((packed)) gtpc_ambr_t,*gtpc_ambr_ptr;

  // EBI(EPS Bearer ID)
  typedef struct gtpc_ebi{
      gtpc_comm_header_t  head;   // 1
      gtpc_numberdigit_t  ebi;
  }__attribute__ ((packed)) gtpc_ebi_t,*gtpc_ebi_ptr;

  // MEI
  typedef struct gtpc_mei{
      gtpc_comm_header_t  head;   // MEI_LEN
      gtpc_numberdigit_t  digits[GTPC_MEI_LEN];
  }__attribute__ ((packed)) gtpc_mei_t,*gtpc_mei_ptr;

  // MSISDN
  typedef struct gtpc_msisdn{
      gtpc_comm_header_t  head;   // MSISDN_LEN
      gtpc_numberdigit_t  digits[GTPC_MSISDN_LEN];
  }__attribute__ ((packed)) gtpc_msisdn_t,*gtpc_msisdn_ptr;

  // INDICATION FLAGS
  typedef struct gtpc_indication_flags{
      gtpc_comm_header_t  head;   // 4
      union _c {
          struct _bit{
              __u32 sgwci:1; __u32 israi:1; __u32 isrsi:1; __u32 oi:1;
              __u32 dfi:1;   __u32 hi:1;    __u32 dtf:1;   __u32 daf:1;
              //
              __u32 msv:1;   __u32 si:1;    __u32 pt:1;    __u32 p:1;
              __u32 crsi:1;  __u32 cfsi:1;  __u32 uimsi:1; __u32 sqci:1;
              //
              __u32 ccrsi:1; __u32 israu:1; __u32 mbmdt:1; __u32 s4af:1;
              __u32 s6af:1;  __u32 srni:1;  __u32 pbic:1;  __u32 retloc:1;
              //
              __u32 cpsr:1;  __u32 clii:1;  __u32 spare:6;
          }bit;
          __u32 flags;
      }c;
      __be16       padd;
  }__attribute__ ((packed)) gtpc_indication_flags_t,*gtpc_indication_flags_ptr;

  // PCO(Protocol Configuration Options)
  typedef struct gtpc_pco{
      gtpc_comm_header_t  head;
      __u8                pco[GTPC_PCO_LEN];    // 3gpp. ts 24.008 10.5.6.3 Protocol configuration options.
  }__attribute__ ((packed)) gtpc_pco_t,*gtpc_pco_ptr;
  typedef struct gtpc_pco_option_header{
      union _c {
          struct _bit{
              __u8 prot:3;
              __u8 spare:4;
              __u8 exit:1;
          }bit;
          __u8 flags;
      }c;
  }__attribute__ ((packed)) gtpc_pco_option_header_t,*gtpc_pco_option_header_ptr;
  typedef struct gtpc_pco_option_item{
      __be16      protocol_or_container_id;
      __u8        length;
      __u8        contents[GTPC_PCO_IPCP_LEN];
  }__attribute__ ((packed)) gtpc_pco_option_item_t,*gtpc_pco_option_item_ptr;
  // deined from RFC1661,RFC3232



  // PAA(PDN Address Allocation)
  typedef struct gtpc_paa{
      gtpc_comm_header_t  head;
      union _c {
          struct _bit{
              __u8 pdn_type:3;
              __u8 spare:5;
          }bit;
          __u8 flags;
      }c;
      __u8                paa[GTPC_PAA_ADRS_LEN];   // 3gpp. ts 29.274 8.14-1:PDN Address Allocation.
  }__attribute__ ((packed)) gtpc_paa_t,*gtpc_paa_ptr;


  // Bearer QOS(Bearer Quality of Service)
  typedef struct gtpc_bearer_qos{
      gtpc_comm_header_t  head;
      union _c {
          struct _bit{
              __u8 spare0:1;
              __u8 pci:1;
              __u8 pl:4;
              __u8 spare1:1;
              __u8 pvi:1;
          }bit;
          __u8 flags;
      }c;
      __u8        qci;
      __u8        rate[20];
  }__attribute__ ((packed)) gtpc_bearer_qos_t,*gtpc_bearer_qos_ptr;

  // RAT TYPE
  typedef struct gtpc_rat{
      gtpc_comm_header_t  head;   // 1
      __u8                rat_type;
  }__attribute__ ((packed)) gtpc_rat_t,*gtpc_rat_ptr;

  // SERVING NETWORK
  typedef struct gtpc_serving_network{
      gtpc_comm_header_t  head;       // 3
      gtpc_numberdigit_t  digits[3];
  }__attribute__ ((packed)) gtpc_serving_network_t,*gtpc_serving_network_ptr;


  // ULI(User Location Information)
  typedef struct gtpc_uli_cgi{
      gtpc_numberdigit_t      digits[3];
      __be16                  location_area_code;
      __be16                  cell_identity;
  }__attribute__ ((packed)) gtpc_uli_cgi_t,*gtpc_uli_cgi_ptr;
  typedef struct gtpc_uli_sai{
      gtpc_numberdigit_t      digits[3];
      __be16                  location_area_code;
      __be16                  service_area_code;
  }__attribute__ ((packed)) gtpc_uli_sai_t,*gtpc_uli_sai_ptr;
  typedef struct gtpc_uli_rai{
      gtpc_numberdigit_t      digits[3];
      __be16                  location_area_code;
      __be16                  routing_area_code;
  }__attribute__ ((packed)) gtpc_uli_rai_t,*gtpc_uli_rai_ptr;
  typedef struct gtpc_uli_tai{
      gtpc_numberdigit_t      digits[3];
      __be16                  tracking_area_code;
  }__attribute__ ((packed)) gtpc_uli_tai_t,*gtpc_uli_tai_ptr;
  typedef struct gtpc_uli_ecgi{
      gtpc_numberdigit_t  digits[3];
      gtpc_inst_t         eci;
      __u8                eutran_cell_identifier[3];
  }__attribute__ ((packed)) gtpc_uli_ecgi_t,*gtpc_uli_ecgi_ptr;
  typedef struct gtpc_uli_lai{
      gtpc_numberdigit_t      digits[3];
      __be16                  location_area_code;
  }__attribute__ ((packed)) gtpc_uli_lai_t,*gtpc_uli_lai_ptr;

  //
  typedef struct gtpc_uli{
      gtpc_comm_header_t  head;   // over 12
      union _c {
          struct _bit{
              __u8 cgi:1;
              __u8 sai:1;
              __u8 rai:1;
              __u8 tai:1;
              __u8 ecgi:1;
              __u8 lai:1;
              __u8 spare:2;
          }bit;
          __u8 flags;
      }c;
      uint8_t          blk[38];    // rai or tai and ecgi
  }__attribute__ ((packed)) gtpc_uli_t,*gtpc_uli_ptr;


  // F-TEID(Fully Qualified TEID)
  typedef struct gtpc_f_teid{
      gtpc_comm_header_t  head;   // over 13
      union _c {
          struct _bit{
              __u8 iftype:5;
              __u8 spare:1;
              __u8 v6:1;
              __u8 v4:1;
          }bit;
          __u8 flags;
      }c;
      __be32              teid_grekey;
      __u8                blk[16];
  }__attribute__ ((packed)) gtpc_f_teid_t,*gtpc_f_teid_ptr;

  // Charging ID
  typedef struct gtpc_charging_id{
      gtpc_comm_header_t  head;       // 4
      __be32              charging_id;
  }__attribute__ ((packed)) gtpc_charging_id_t,*gtpc_charging_id_ptr;

  // PDN Type
  typedef struct gtpc_pdn_type{
      gtpc_comm_header_t  head;       // 1gtpc_ebi_t
      union _c {
          struct _bit{
              __u8 pdn_type:3;
              __u8 spare:5;
          }bit;
          __u8 flags;
      }c;
  }__attribute__ ((packed)) gtpc_pdn_type_t,*gtpc_pdn_type_ptr;

  // APN Restriction
  typedef struct gtpc_apn_restriction{
      gtpc_comm_header_t  head;       // 1
      __u8                restriction_type;
  }__attribute__ ((packed)) gtpc_apn_restriction_t,*gtpc_apn_restriction_ptr;

  // Selection Mode
  typedef struct gtpc_selection_mode{
      gtpc_comm_header_t  head;       // 1
      union _c {
          struct _bit{
              __u8 spare:6;
              __u8 select_mode:2;
          }bit;
          __u8 flags;
      }c;
  }__attribute__ ((packed)) gtpc_selection_mode_t,*gtpc_selection_mode_ptr;

  // Bearer Context
  typedef struct gtpc_bearer_ctx{
      gtpc_comm_header_t  head;
      gtpc_ebi_t          ebi;
      gtpc_f_teid_t       tft;
      gtpc_bearer_qos_t   qos;
  }__attribute__ ((packed)) gtpc_bearer_ctx_t,*gtpc_bearer_ctx_ptr;
  // Bearer Context to be modified
  typedef struct gtpc_bearer_ctx_to_be_modified{
      gtpc_comm_header_t  head;
      gtpc_ebi_t          ebi;
      gtpc_cause_t        cause;
      gtpc_charging_id_t  charge;
  }__attribute__ ((packed)) gtpc_bearer_ctx_to_be_modified_t,*gtpc_bearer_ctx_to_be_modified_ptr;
  //
  typedef struct gtpc_echo_request{
      gtpc_recovery_t    recovery;
  }__attribute__ ((packed)) gtpc_echo_request_t,*gtpc_echo_request_ptr;
  //
  typedef struct gtpc_echo_response{
      gtpc_recovery_t recovery;
      gtpc_cause_t    cause;
  }__attribute__ ((packed)) gtpc_echo_response_t,*gtpc_echo_response_ptr;
  //
  typedef struct gtpc_create_session{
      gtpc_imsi_ptr               imsi;
      gtpc_msisdn_ptr             msisdn;
      gtpc_mei_ptr                mei;
      gtpc_uli_ptr                uli;
      gtpc_serving_network_ptr    serving_network;
      gtpc_rat_ptr                rat;
      gtpc_indication_flags_ptr   indication_flags;
      gtpc_f_teid_ptr             f_teid;
      gtpc_apn_ptr                apn;
      gtpc_selection_mode_ptr     selection_mode;
      gtpc_pdn_type_ptr           pdn_type;
      gtpc_paa_ptr                paa;
      gtpc_apn_restriction_ptr    apn_restriction;
      gtpc_ambr_ptr               ambr;
      gtpc_pco_ptr                pco;
      gtpc_bearer_ctx_ptr         bearer_ctx_created;
      gtpc_recovery_ptr           recovery;
      gtpc_ebi_ptr                ebi;
  }__attribute__ ((packed)) gtpc_create_session_t,*gtpc_create_session_ptr;
  //
  typedef struct gtpc_modify_bearer{
      gtpc_serving_network_ptr    serving_network;
      gtpc_rat_ptr                rat;
      gtpc_f_teid_ptr             f_teid;
      gtpc_bearer_ctx_ptr         bearer_ctx_created;
      gtpc_recovery_ptr           recovery;
  }__attribute__ ((packed)) gtpc_modify_bearer_t,*gtpc_modify_bearer_ptr;
  // parser container.
  typedef struct gtpc_parse_container{
      gtpc_cause_ptr              cause;
      gtpc_msisdn_ptr             msisdn;
      gtpc_f_teid_ptr             fteid;
      gtpc_paa_ptr                paa;
      gtpc_apn_restriction_ptr    apn_restriction;
      gtpc_ambr_ptr               ambr;
      gtpc_pco_ptr                pco;
      gtpc_bearer_ctx_ptr         bearer_ctx;
      gtpc_bearer_ctx_to_be_modified_ptr  bearer_modified_ctx;
      gtpc_recovery_ptr           recovery;
      gtpc_ebi_ptr                ebi;
  }gtpc_parse_container_t,*gtpc_parse_container_ptr;
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_CTRLPLANE_DEF_H
