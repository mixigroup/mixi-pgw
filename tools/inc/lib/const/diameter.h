//
// Created by mixi on 2017/06/22.
//

#ifndef MIXIPGW_TOOLS_DIAMETER_H
#define MIXIPGW_TOOLS_DIAMETER_H
/*

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Version    |                 Message Length                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Command Flags |                  Command Code                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Application-ID                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      Hop-by-Hop Identifier                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      End-to-End Identifier                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  AVPs ...
+-+-+-+-+-+-+-+-+-+-+-+-+-
*/
/*
 * https://www.iana.org/assignments/aaa-parameters/aaa-parameters.xhtml
 */

// ---------------------
//Application IDs
#define DIAMETER_APPID_COMMON_MESSAGE                   (0)         // Diameter common message,[RFC6733]
#define DIAMETER_APPID_BASE_ACCOUNTING                  (3)         // Diameter Base Accounting,[RFC6733]
#define DIAMETER_APPID_CREDIT_CONTROL                   (4)         // Diameter Credit Control,[RFC4006]

#define DIAMETER_APPID_3GPP_GX_29210                    (16777224)  // 3GPP Gx,[3GPP TS 29.210][Kimmo_Kymalainen]
#define DIAMETER_APPID_3GPP_GX_OVER_GY                  (16777225)  // 3GPP Gx over Gy,[3GPP TS 29.210][Kimmo_Kymalainen]
#define DIAMETER_APPID_3GPP_GX_29212                    (16777238)  // 3GPP Gx,[3GPP TS 29.212][Kimmo_Kymalainen]
// ---------------------
//Command Codes
#define DIAMETER_CMDCODE_CER_CEA                        (257)       // CER / CEA,[RFC6733]
#define DIAMETER_CMDCODE_CCR_CCA                        (272)       // CCR / CCA,[RFC4006]
#define DIAMETER_CMDCODE_DWR_DWA                        (280)       // DWR / DWA,[RFC6733]
// ---------------------
//AVP Code
#define DIAMETER_AVPCODE_HOST_IP_ADDRESS                (257)       // Host-IP-Address,[RFC6733]
#define DIAMETER_AVPCODE_AUTH_APPLICATION_ID            (258)       // Auth-Application-Id,[RFC6733]
#define DIAMETER_AVPCODE_ACCT_APPLICATION_ID            (259)       // Acct-Application-Id
#define DIAMETER_AVPCODE_VENDOR_SPECIFIC_APPLICATION_ID (260)       // Vendor-Specific-Application-Id

#define DIAMETER_AVPCODE_SESSION_ID                     (263)       // Session-Id,[RFC6733]
#define DIAMETER_AVPCODE_ORIGIN_HOST                    (264)       // Origin-Host,[RFC6733]
#define DIAMETER_AVPCODE_VENDOR_ID                      (266)       // Vendor-Id,[RFC6733]
#define DIAMETER_AVPCODE_FIRMWARE_REVISION              (267)       // Firmware-Revision[RFC6733]
#define DIAMETER_AVPCODE_RESULT_CODE                    (268)       // Result-Code,[RFC6733]
#define DIAMETER_AVPCODE_PRODUCT_NAME                   (269)       // Product-Name,[RFC6733]


#define DIAMETER_AVPCODE_DESTINATION_REALM              (283)       // Destination-Realm,[RFC6733]

#define DIAMETER_AVPCODE_ORIGIN_REALM                   (296)       // Origin-Realm,[RFC6733]

#define DIAMETER_AVPCODE_CC_INPUT_OCTETS                (412)       // CC-Input-Octets,[RFC4006]
#define DIAMETER_AVPCODE_CC_MONEY                       (413)       // CC-Money,[RFC4006]
#define DIAMETER_AVPCODE_CC_OUTPUT_OCTETS               (414)       // CC-Output-Octets,[RFC4006]

#define DIAMETER_AVPCODE_CC_REQUEST_NUMBER              (415)       // CC-Request-Number,[RFC4006]
#define DIAMETER_AVPCODE_CC_REQUEST_TYPE                (416)       // CC-Request-Type,[RFC4006]
#define DIAMETER_AVPCODE_CC_SESSION_FAILOVER            (418)

#define DIAMETER_AVPCODE_CC_TOTAL_OCTETS                (421)       // CC-Total-Octets,[RFC4006]


#define DIAMETER_AVPCODE_GRANTED_SERVICE_UNIT           (431)       // Granted-Service-Unit,[RFC4006]


#define DIAMETER_AVPCODE_SUBSCRIPTION_ID                (443)       // Subscription-Id,[RFC4006]
#define DIAMETER_AVPCODE_SUBSCRIPTION_ID_DATA           (444)       // Subscription-Id-Data,[RFC4006]
#define DIAMETER_AVPCODE_USED_SERVICE_UNIT              (446)       // Used-Service-Unit,[RFC4006]

#define DIAMETER_AVPCODE_SUBSCRIPTION_ID_TYPE           (450)       // Subscription-Id-Type,[RFC4006]
#define DIAMETER_AVPCODE_MULTIPLE_SERVICES_CREDIT_CONTROL (456)     // Multiple-Services-Credit-Control,[RFC4006]


#define DIAMETER_AVPCODE_SERVICE_CONTEXT_ID             (461)       // Service-Context-Id,[RFC4006]
#define DIAMETER_AVPCODE_SUPPORTED_FEATURES             (628)
#define DIAMETER_AVPCODE_FEATURE_LIST_ID                (629)
#define DIAMETER_AVPCODE_FEATURE_LIST                   (630)
#define DIAMETER_AVPCODE_VOLUME_QUOTA_THRESHOLD         (869)       // Volume-Quota-Threshold

//
#define DIAMETER_AVPCODE_CHARGING_RULE_INSTALL          (1001)      // grouped.
#define DIAMETER_AVPCODE_CHARGING_RULE_REMOVE           (1002)      // grouped.
#define DIAMETER_AVPCODE_CHARGING_RULE_BASE_NAME        (1004)      // string
#define DIAMETER_AVPCODE_CHARGING_RULE_NAME             (1005)      // string


#define DIAMETER_AVPCODE_EVENT_TRRIGER                  (1006)      //
#define DIAMETER_AVPCODE_ONLINE                         (1009)
#define DIAMETER_AVPCODE_OFFLINE                        (1008)
#define DIAMETER_AVPCODE_QOS_INFORMATION                (1016)
#define DIAMETER_AVPCODE_BEARER_CONTROL_MODE            (1023)
#define DIAMETER_AVPCODE_QOS_CLASS_IDENTIFIER           (1028)
#define DIAMETER_AVPCODE_ALLOCATION_RETENTION_PRIORITY  (1034)
#define DIAMETER_AVPCODE_APN_AGGREGATE_MAX_BITRATE_DL   (1040)
#define DIAMETER_AVPCODE_APN_AGGREGATE_MAX_BITRATE_UL   (1041)
#define DIAMETER_AVPCODE_REVALIDATION_TIME              (1042)

#define DIAMETER_AVPCODE_PRIORITY_LEVEL                 (1046)
#define DIAMETER_AVPCODE_PRE_EMPTION_CAPABILITY         (1047)
#define DIAMETER_AVPCODE_PRE_EMPTION_VULNERABILITY      (1048)
#define DIAMETER_AVPCODE_DEFAULT_EPS_BEARER_QOS         (1049)      // Default Eps Bearer QOS(grouped)
#define DIAMETER_AVPCODE_RESOURCE_ALLOCATION_NOTIFICATION (1063)    //
#define DIAMETER_AVPCODE_MONITORING_KEY                 (1066)      // OctetString,VendorID:10415,Value Length : 0 .. *
#define DIAMETER_AVPCODE_USAGE_MONITORING_INFORMATION   (1067)      // Grouped
#define DIAMETER_AVPCODE_USAGE_MONITORING_LEVEL         (1068)      // Enumerated, VendorID:10415

//
#define HAS_SESSION_ID(x)                               (x|=(1<<31))
#define HAS_ORIGIN_HOST(x)                              (x|=(1<<30))
#define HAS_ORIGIN_REALM(x)                             (x|=(1<<29))
#define HAS_DESTINATION_REALM(x)                        (x|=(1<<28))
#define HAS_AUTH_APPLICATION_ID(x)                      (x|=(1<<27))
#define HAS_SERVICE_CONTEXT_ID(x)                       (x|=(1<<26))
#define HAS_CC_REQUEST_TYPE(x)                          (x|=(1<<25))
#define HAS_CC_REQUEST_NUMBER(x)                        (x|=(1<<24))
#define HAS_SUBSCRIPTION_ID(x)                          (x|=(1<<23))
#define HAS_SUBSCRIPTION_ID_TYPE(x)                     (x|=(1<<22))
#define HAS_SUBSCRIPTION_ID_DATA(x)                     (x|=(1<<21))
#define HAS_CC_INPUT_OCTETS(x)                          (x|=(1<<20))
#define HAS_CC_OUTPUT_OCTETS(x)                         (x|=(1<<19))
#define HAS_CC_TOTAL_OCTETS(x)                          (x|=(1<<18))
#define HAS_DONTCARE(x)                                 void(0)
#define IS_REQUIRED_CC(x)                               ((x&0xfbe00000)==0xfbe00000)
#define IS_REQUIRED_DW(x)                               ((x&0x60000000)==0x60000000)

// Result-Code AVP Values (code 268) - Success
#define DIAMETER_SUCCESS                                (2001)
// Result-Code AVP Values (code 268) - Protocol Errors
#define DIAMETER_COMMAND_UNSUPPORTED                    (3001)
#define DIAMETER_COMMAND_INVALIDARGS                    (3002)
// Result-Code AVP Values (code 268) - Transient Failures
#define DIAMETER_AUTHENTICATION_REJECTED                (4001)
// Result-Code AVP Values (code 268) - Permanent Failure
#define DIAMETER_AVP_UNSUPPORTED                        (5001)

//
#define DIAMETER_ON                                     (1)
#define DIAMETER_OFF                                    (0)
#define DIAMETER_VERSION                                (1)
#define DIAMETER_VERSION_COUNTER_INGRESS                (0xfe)
#define DIAMETER_VERSION_COUNTER_EGRESS                 (0xff)
#define XDIAMETER_PAYLOAD_MAX                           (4096)

#define END_USER_E164                                   (0)
#define END_USER_IMSI                                   (1)

#define INITIAL_REQUEST                                 (1)
#define UPDATE_REQUEST                                  (2)
#define TERMINATE_REQUEST                               (3)
#define EVENT_REQUEST                                   (4)
#define COUNTER_REQUEST                                 (5)

#define SGSN_CHANGE                                     (0)
#define QOS_CHANGE                                      (1)
#define RAT_CHANGE                                      (2)
#define PLMN_CHANGE                                     (4)
#define OUT_OF_CREDIT                                   (15)
#define REALLOCATION_OF_CREDIT                          (16)
#define REVALIDATION_TIMEOUT                            (17)
#define DEFAULT_EPS_BEARER_QOS_CHANGE                   (21)
#define SUCCESSFUL_RESOURCE_ALLOCATION                  (22)
#define USAGE_REPORT_10                                 (26)
#define USAGE_REPORT                                    (33)
#define DISABLE_ONLINE                                  (0)
#define DIAMETER_VENDOR_ID_3GPP                         (10415)
#define SESSION_LEVEL                                   (0)
#define PCC_RULE_MONITOR_ID                             (1)
#define QCI_9                                           (9)
#define PRIORITY_1                                      (1)
#define PRE_EMPTION_CAPABILITY_ENABLED                  (0)
#define PRE_EMPTION_VULNERABILITY_ENABLED               (0)
#define UE_NW                                           (2)
#define FEATURE_LIST_ID                                 (1)
#define FEATURE_FLAG                                    (0x0000000B)
#define FAILOVER_SUPPORTED                              (1)
#define ENABLE_NOTIFICATION                             (0)
#define CC_TOTAL_OCTETS_THREATHOULD                     (1024*1024)
#define CC_OUTPUT_OCTETS_THREATHOULD                    (1024*512)
#define CC_INPUT_OCTETS_THREATHOULD                     (1024*512)
#define DIAMETER_TIME_OFFSET                            (2208988800)


namespace MIXIPGW_TOOLS{
  typedef struct diameter_header{
      uint32_t  version:8;
      uint32_t  len:24;
      union _f {
          struct _flag{
              u_int8_t rsvd:4;
              u_int8_t t:1;
              u_int8_t e:1;
              u_int8_t p:1;
              u_int8_t r:1;
          }flag;
          uint8_t flags;
      }f;
      uint32_t  code:24;
      uint32_t  appid;
      uint32_t  popid;
      uint32_t  eeid;
  }__attribute__ ((packed)) diameter_header_t,*diameter_header_ptr;


  typedef struct diameter_avp_header{
      uint32_t  code;
      union _f {
          struct _flag{
              u_int8_t rsvd:5;
              u_int8_t p:1;
              u_int8_t m:1;
              u_int8_t v:1;
          }flag;
          uint8_t flags;
      }f;
      uint32_t  len:24;
  }__attribute__ ((packed)) diameter_avp_header_t,*diameter_avp_header_ptr;


  typedef struct diameter_link {
      uint64_t  key;
      uint64_t  mkey;
      __be32    ipv4;
      __be32    ipv6[4];
      __be32    nasipv;
      char      policy[16];
      __be32    ingress;
      __be32    egress;
      __be32    threshold;
      //
      struct _stat{
          __be32 valid :1;
          __be32 type  :8;
          __be32 active:1;
          __be32 expire:1;
          __be32 ipv:3;     // 1=ipv4,2=ipv6,3=both
          __be32 padd :18;
      }stat;
  }__attribute__ ((packed)) diameter_link_t,*diameter_link_ptr;
  //
  static_assert(sizeof(diameter_header_t)==20, "need 20 octets");
  static_assert(sizeof(diameter_avp_header_t)==8, "need 8 octets");

}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_DIAMETER_H
