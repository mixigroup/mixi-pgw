//
// Created by mixi on 2017/01/10.
//

#ifndef MIXIPGW_TOOLS_V2_ECHO_H
#define MIXIPGW_TOOLS_V2_ECHO_H

#include "gtpc.hpp"
#include "lib/const/gtpc.h"

#define  GTPCITEM_COMMON_FUNCTIONS(t)    \
    public:\
        t();\
        t(void*,size_t);\
        virtual ~t();\
    public:\
        t& operator()(){ return(*this); }\
    private:\
        t(const t&){}\
    public:\
        virtual int type(void) const;\
        virtual size_t len(void) const;\
        virtual void* ref(void) const;\
        virtual int attach(void*,size_t);


namespace MIXIPGW_TOOLS{
  // ----------
  class Recovery:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Recovery)
      Recovery(uint8_t);
  public:
      void set(uint8_t);
  protected:
      gtpc_recovery_t recovery_;
  }; // class Recovery

  // ----------
  class Apn:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Apn)
      Apn(const char*);
  public:
      void set(const char*);
      void set(void*,size_t);
      std::string name(void);
  protected:
      gtpc_apn_t apn_;
  }; // class Apn

  // ----------
  class Imsi:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Imsi)
      Imsi(uint64_t);
  public:
      void set(void*,size_t);
      void set(uint64_t);
      virtual std::string digits(void){
          return(GtpcPkt::digits(imsi_.digits, GTPC_IMSI_LEN,GTPC_DIGITS_MASKON));
      }
  protected:
      gtpc_imsi_t imsi_;
  }; // class Imsi

  // ----------
  class Ambr:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Ambr)
      Ambr(uint32_t,uint32_t);
  public:
      void set(uint32_t,uint32_t);
      uint32_t uplink(void);
      uint32_t downlink(void);
  protected:
      gtpc_ambr_t ambr_;
  }; // class Ambr

  // ----------
  class Msisdn:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Msisdn)
      Msisdn(uint64_t);
  public:
      void set(uint64_t);
      virtual std::string digits(void){
          return(GtpcPkt::digits(msisdn_.digits, GTPC_MSISDN_LEN));
      }
  protected:
      gtpc_msisdn_t msisdn_;
  }; // class Msisdn

  // ----------
  class Cause:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Cause)
      Cause(uint8_t);
  public:
      void set(void*,size_t);
  protected:
      gtpc_cause_t cause_;
  }; // class Cause

  // ----------
  class Mei:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Mei)
      Mei(uint64_t);
  public:
      void set(void*,size_t);
      void set(uint64_t);
      virtual std::string digits(void){
          return(GtpcPkt::digits(mei_.digits, GTPC_MEI_LEN, GTPC_DIGITS_MASKON));
      }
  protected:
      gtpc_mei_t mei_;
  }; // class Mei

  // ----------
  class Ebi:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Ebi)
      Ebi(uint8_t,void*,size_t);
      Ebi(uint8_t,uint8_t);
  public:
      void set(void*,size_t);
  protected:
      gtpc_ebi_t ebi_;
  }; // class Ebi

  // ----------
  class Indication:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Indication)
      Indication(uint32_t);
  public:
      void set(void*,size_t);
  protected:
      gtpc_indication_flags_t indication_;
  }; // class Indication

  // ----------
  class Pco:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Pco)
      Pco(const char*);
  public:
      void set(void*,size_t);
  protected:
      gtpc_pco_t pco_;
  }; // class Pco

  // ----------
  class Paa:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Paa)
      Paa(uint8_t,void*,size_t);
  public:
      void set(uint8_t,void*,size_t);
  protected:
      gtpc_paa_t paa_;
  }; // class Paa

  // ----------
  class Bqos:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Bqos)
      Bqos(uint8_t,uint8_t,uint64_t,uint64_t,uint64_t,uint64_t);
  public:
      enum MODE{
          UPLINK,
          DOWNLINK,
          G_UPLINK,
          G_DOWNLINK,
          QCI,
      };
  public:
      void set(uint8_t,uint8_t,uint64_t,uint64_t,uint64_t,uint64_t);
      uint64_t  get(MODE);
  protected:
      gtpc_bearer_qos_t bqos_;
  }; // class Bqos

  // ----------
  class Rat:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Rat)
      Rat(uint8_t);
  public:
      void set(uint8_t);
  protected:
      gtpc_rat_t rat_;
  }; // class Rat

  // ----------
  class ServingNetwork:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(ServingNetwork)
      ServingNetwork(gtpc_numberdigit_ptr);
  public:
      void set(void*,size_t);
  protected:
      gtpc_serving_network_t servingnetwork_;
  }; // class ServingNetwork

  // ----------
  class Uli:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Uli)
      Uli(gtpc_uli_cgi_ptr, gtpc_uli_sai_ptr, gtpc_uli_rai_ptr, gtpc_uli_tai_ptr, gtpc_uli_ecgi_ptr, gtpc_uli_lai_ptr);
  public:
      void set(gtpc_uli_cgi_ptr, gtpc_uli_sai_ptr, gtpc_uli_rai_ptr, gtpc_uli_tai_ptr, gtpc_uli_ecgi_ptr, gtpc_uli_lai_ptr);
  protected:
      gtpc_uli_t uli_;
  }; // class Uli

  // ----------
  class Fteid:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Fteid)
      Fteid(uint8_t,uint8_t,uint32_t,uint8_t*,uint8_t*);
  public:
      void set(uint8_t,uint8_t,uint32_t,uint8_t*,uint8_t*);
      uint32_t teid(void);
      std::string ipv(void);
  protected:
      gtpc_f_teid_t fteid_;
  }; // class Fteid

  // ----------
  class BearerContext:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(BearerContext)
  public:
      void append(const GtpcItem&);
      void append(GtpcItem*);
      void iterate(iterate_gtpc,void*);
      GtpcPkt* child(void);
  protected:
      void calc(void);
  protected:
      uint8_t               payload_[ETHER_MAX_LEN];
      size_t                offset_;
      GtpcPkt*              child_;
  }; // class BearerContext

  // ----------
  class ChargingId:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(ChargingId)
      ChargingId(uint32_t);
  public:
      void set(uint32_t);
  protected:
      gtpc_charging_id_t chargingid_;
  }; // class ChargingId

  // ----------
  class Pdn:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Pdn)
      Pdn(uint8_t);
  public:
      void set(uint8_t);
  protected:
      gtpc_pdn_type_t pdn_;
  }; // class Pdn

  // ----------
  class ApnRestriction:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(ApnRestriction)
      ApnRestriction(uint8_t);
  public:
      void set(uint8_t);
  protected:
      gtpc_apn_restriction_t apn_;
  }; // class ApnRestriction

  // ----------
  class SelectionMode:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(SelectionMode)
      SelectionMode(uint8_t);
  public:
      void set(uint8_t);
  protected:
      gtpc_selection_mode_t sel_;
  }; // class SelectionMode

  // ----------
  class Any:public GtpcItem{
  GTPCITEM_COMMON_FUNCTIONS(Any)
  protected:
      size_t    length_;
      uint8_t   payload_[ETHER_MAX_LEN];
  }; // class Any
}; // namespace MIXIPGW_TOOLS

#undef  GTPCITEM_COMMON
#undef  GTPCITEM_ORVERRIDE_FUNC

#endif //MIXIPGW_TOOLS_V2_ECHO_H
