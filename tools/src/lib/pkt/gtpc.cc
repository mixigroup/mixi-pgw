//
// Created by mixi on 2017/01/11.
//

#include "mixipgw_tools_def.h"
#include "lib/buffer.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"
#include <string>
#include <assert.h>
#include <stdexcept>

#define ETHERPACKET_PAGE_SIZE   (2048)
using namespace MIXIPGW_TOOLS;
//
namespace MIXIPGW_TOOLS{
  static size_t headerlen(int);
  static bool issupported(__u8);
  //
  void GtpcPkt::dump_impl(const char* prefix, const void* src, int len){
      char* bf = (char*)src;
      std::string log;
      char        logbf[32];
      if (Module::VERBOSE() <= PCAPLEVEL_PARSE){
          return;
      }
      //
      log = "\n";
      log += (prefix==NULL?"":prefix);
      log += "\n";
      for(int n = 0;n < len; n++){
          if (n%4==0){ log += " "; }
          if (n%32==0){ log += "\n"; }
          snprintf(logbf, sizeof(logbf) -1, "%02x", (__u8)bf[n]);
          log += logbf;
      }
      log += "\n";
      if (prefix!=NULL){
          Logger::LOGINF("%s", log.c_str());
      }else{
          printf("\n%s\n", log.c_str());
      }
  }
  //
  int GtpcItem::type(void* src,size_t len){
      gtpc_comm_header_ptr p = (gtpc_comm_header_ptr)src;
      if (len < sizeof(gtpc_comm_header_t)){
          Logger::LOGERR("invalid length/GtpcItem::type[%d < %d]", len, sizeof(gtpc_comm_header_t));
          return(0);
      }
      if (p->length == 0){
          Logger::LOGERR("invalid header length/GtpcItem::type[%d]", p->length);
          return(0);
      }
      return(p->type);
  }
  std::string GtpcItem::digits(void){
      return("");
  }
  void GtpcItem::dump(const char* prefix){
      GtpcPkt::dump_impl(prefix, ref(), len());
  }
  GtpcPkt::GtpcPkt(int type):buffer_(NULL),pktlen_(sizeof(gtpc_header_t)),header_(NULL),type_(type){
      if ((buffer_ = new Buffer(ETHERPACKET_PAGE_SIZE)) == NULL){
          assert(!"invalid address from new operation"); _exit(0);
      }
      header_ = (gtpc_header_ptr)buffer_->Ref(NULL);
  }
  GtpcPkt::GtpcPkt():buffer_(NULL),pktlen_(sizeof(gtpc_header_t)),header_(NULL),type_(0){}
  GtpcPkt::~GtpcPkt(){
      GTPCITEMITR itr;
      if (buffer_ != NULL){
          delete buffer_;
      }
      buffer_ = NULL;
      for(itr = items_.begin();itr != items_.end();++itr){
          delete (*itr);
      }
      items_.clear();
  }
  int GtpcPkt::append(GtpcItem& item){
      return(append(&item));
  }
  int GtpcPkt::append(const GtpcItem& item){
      return(append(&item));
  }
  int GtpcPkt::append(const GtpcItem* item){
      Buffer* tmpbf;
      if (item == NULL){
          return(RETERR);
      }
      if ((pktlen_ + item->len()) > ETHERPACKET_PAGE_SIZE){
          Logger::LOGERR("capability over total size.(%u : %u)", pktlen_, item->len());
          GtpcPkt::dump_impl(NULL,item->ref(), MIN(8,item->len()));
          return(RETERR);
      }
      if ((tmpbf = new Buffer(item->ref(), item->len())) == NULL){
          Logger::LOGERR("new error (%p : %u)", item->ref(), item->len());
          return(RETERR);
      }
      items_.push_back(tmpbf);
      pktlen_ = len();
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("GtpcPkt::append(%p : %u : %u : %d)", item->ref(), item->len(), pktlen_, items_.size());
      }
      return(RETOK);
  }
  size_t GtpcPkt::len(void){
      GTPCITEMITR itr;
      size_t gtpclen = headerlen(type_);
      for(itr = items_.begin();itr != items_.end(); ++itr){
          gtpclen += (*itr)->Len();
      }
      return(gtpclen);
  }
  void GtpcPkt::dump(void){
      dump_impl("gtpc-packet", ref(), len());
  }
  std::string GtpcPkt::digits(gtpc_numberdigit_ptr pdigits,size_t len, __u8 mask){
      std::string ret;
      char bf[32] = {0};
      for(size_t n = 0; n < len; n++){
          if ((n+1)==len && mask){
              snprintf(bf, sizeof(bf) -1, "%d", pdigits[n].low);
          }else{
              snprintf(bf, sizeof(bf) -1, "%d%d", pdigits[n].low, pdigits[n].high);
          }
          ret += bf;
      }
      return(ret);
  }

#define ATTACH_CALLBACK(t,i,c,u)  {t ti((*i)->Ref(NULL), (*i)->Len()); if (callback(&ti, udata) != RETOK){ return; } }

  void GtpcPkt::iterate(iterate_gtpc callback, void* udata){
      GTPCITEMITR itr;
      if (callback != NULL){
          for(itr = items_.begin();itr != items_.end(); ++itr){
              int type = GtpcItem::type((*itr)->Ref(NULL), (*itr)->Len());
              char bf[32] = {0};
              //
              snprintf(bf, sizeof(bf)-1,"GtpcPkt::iterate(%d)", type);
              dump_impl(bf, (*itr)->Ref(NULL), (*itr)->Len());
              switch(type){
                  case GTPC_TYPE_RECOVERY:ATTACH_CALLBACK(Recovery, itr, callback, udata); break;
                  case GTPC_TYPE_APN:     ATTACH_CALLBACK(Apn, itr, callback, udata); break;
                  case GTPC_TYPE_IMSI:    ATTACH_CALLBACK(Imsi, itr, callback, udata); break;
                  case GTPC_TYPE_AMBR:    ATTACH_CALLBACK(Ambr, itr, callback, udata); break;
                  case GTPC_TYPE_MSISDN:  ATTACH_CALLBACK(Msisdn, itr, callback, udata); break;
                  case GTPC_TYPE_CAUSE:   ATTACH_CALLBACK(Cause, itr, callback, udata); break;
                  case GTPC_TYPE_MEI:     ATTACH_CALLBACK(Mei, itr, callback, udata); break;
                  case GTPC_TYPE_EBI:     ATTACH_CALLBACK(Ebi, itr, callback, udata); break;
                  case GTPC_TYPE_INDICATION:ATTACH_CALLBACK(Indication, itr, callback, udata); break;
                  case GTPC_TYPE_PCO:     ATTACH_CALLBACK(Pco, itr, callback, udata); break;
                  case GTPC_TYPE_PAA:     ATTACH_CALLBACK(Paa, itr, callback, udata); break;
                  case GTPC_TYPE_BEARER_QOS:ATTACH_CALLBACK(Bqos, itr, callback, udata); break;
                  case GTPC_TYPE_RAT_TYPE:ATTACH_CALLBACK(Rat, itr, callback, udata); break;
                  case GTPC_TYPE_SERVING_NETWORK:ATTACH_CALLBACK(ServingNetwork, itr, callback, udata); break;
                  case GTPC_TYPE_ULI:     ATTACH_CALLBACK(Uli, itr, callback, udata); break;
                  case GTPC_TYPE_F_TEID:  ATTACH_CALLBACK(Fteid, itr, callback, udata); break;
                  case GTPC_TYPE_BEARER_CTX:ATTACH_CALLBACK(BearerContext, itr, callback, udata); break;
                  case GTPC_TYPE_CHARGING_ID:ATTACH_CALLBACK(ChargingId, itr, callback, udata); break;
                  case GTPC_TYPE_PDN_TYPE:ATTACH_CALLBACK(Pdn, itr, callback, udata); break;
                  case GTPC_TYPE_APN_RESTRICTION:ATTACH_CALLBACK(ApnRestriction, itr, callback, udata); break;
                  case GTPC_TYPE_SELECTION_MODE:ATTACH_CALLBACK(SelectionMode, itr, callback, udata); break;
                  default:
//                    Logger::LOGERR("not implemented. (%d)GtpcPkt::iterate.", type);
                      ATTACH_CALLBACK(Any, itr, callback, udata);
                      break;
              }

          }
      }
  }
  void* GtpcPkt::ref(void){
      // fix gtpc header length.
      GTPCITEMITR itr;
      size_t offset = headerlen(type_);
      if (offset == sizeof(gtpc_header_t)){
          header_->length = htons(len() - 4);
      }else{
          header_->length = htons(len() - 8);
      }

      header_->type = type_;
      header_->c.v2_flags.version = GTPC_VERSION_2;
      header_->c.v2_flags.piggy = GTPC_PIGGY_OFF;
      header_->c.v2_flags.spare = 0;
      //
      switch(type_){
          case GTPC_ECHO_REQ:
          case GTPC_ECHO_RES:
              header_->c.v2_flags.teid = GTPC_TEID_OFF;
              header_->t.sq.seqno = 0;
              break;
          default:
              header_->c.v2_flags.teid = GTPC_TEID_ON;
              header_->t.teid = 0;
              break;
      }
      for(itr = items_.begin();itr != items_.end(); ++itr){
          memcpy(((char*)header_) + offset, (*itr)->Ref(NULL), (*itr)->Len());
          offset += (*itr)->Len();
      }
      return(header_);
  }
#define ATTACH_INSERT(p,l,t,k)  {t itm; if (itm.attach(p,l) != RETOK){ return(RETERR); } else { k->append(itm);} }

  int GtpcPkt::attach_impl(GtpcPkt* pinst, void* payload,size_t hlen){
      gtpc_comm_header_ptr pitm = (gtpc_comm_header_ptr)payload;
      char bf[32] = {0};

      snprintf(bf, sizeof(bf)-1,"attach[%d]-%zu", pitm->type, hlen);
      GtpcPkt::dump_impl(bf, payload, hlen);

      //
      switch(pitm->type){
          case GTPC_TYPE_RECOVERY:    ATTACH_INSERT(payload, hlen, Recovery, pinst); break;
          case GTPC_TYPE_APN:         ATTACH_INSERT(payload, hlen, Apn, pinst); break;
          case GTPC_TYPE_IMSI:        ATTACH_INSERT(payload, hlen, Imsi, pinst); break;
          case GTPC_TYPE_AMBR:        ATTACH_INSERT(payload, hlen, Ambr, pinst); break;
          case GTPC_TYPE_MSISDN:      ATTACH_INSERT(payload, hlen, Msisdn, pinst); break;
          case GTPC_TYPE_CAUSE:       ATTACH_INSERT(payload, hlen, Cause, pinst); break;
          case GTPC_TYPE_MEI:         ATTACH_INSERT(payload, hlen, Mei, pinst); break;
          case GTPC_TYPE_EBI:         ATTACH_INSERT(payload, hlen, Ebi, pinst); break;
          case GTPC_TYPE_INDICATION:  ATTACH_INSERT(payload, hlen, Indication, pinst); break;
          case GTPC_TYPE_PCO:         ATTACH_INSERT(payload, hlen, Pco, pinst); break;
          case GTPC_TYPE_PAA:         ATTACH_INSERT(payload, hlen, Paa, pinst); break;
          case GTPC_TYPE_BEARER_QOS:  ATTACH_INSERT(payload, hlen, Bqos, pinst); break;
          case GTPC_TYPE_RAT_TYPE:    ATTACH_INSERT(payload, hlen, Rat, pinst); break;
          case GTPC_TYPE_SERVING_NETWORK:ATTACH_INSERT(payload, hlen, ServingNetwork, pinst); break;
          case GTPC_TYPE_ULI:         ATTACH_INSERT(payload, hlen, Uli, pinst); break;
          case GTPC_TYPE_F_TEID:      ATTACH_INSERT(payload, hlen, Fteid, pinst); break;
          case GTPC_TYPE_BEARER_CTX:  ATTACH_INSERT(payload, hlen, BearerContext, pinst); break;
          case GTPC_TYPE_CHARGING_ID: ATTACH_INSERT(payload, hlen, ChargingId, pinst); break;
          case GTPC_TYPE_PDN_TYPE:    ATTACH_INSERT(payload, hlen, Pdn, pinst); break;
          case GTPC_TYPE_APN_RESTRICTION:ATTACH_INSERT(payload, hlen, ApnRestriction, pinst); break;
          case GTPC_TYPE_SELECTION_MODE:ATTACH_INSERT(payload, hlen, SelectionMode, pinst); break;
          default:
//            Logger::LOGERR("not implemented. (%d) GtpcPkt::attach_impl.", pitm->type);
              ATTACH_INSERT(payload, hlen, Any, pinst);
              break;
      }
      return(RETOK);
  }
  GtpcPkt* GtpcPkt::attach(void* src,size_t len){
      gtpc_header_ptr gtp = (gtpc_header_ptr)src;
      //
      if (ntohs(gtp->length) < sizeof(gtpc_comm_header_t)){
          Logger::LOGERR("invalid header length.(%d)", ntohs(gtp->length));
          return(NULL);
      }
      if (gtp->c.v2_flags.version != GTPC_VERSION_2 || gtp->c.v2_flags.piggy != GTPC_PIGGY_OFF){
          Logger::LOGERR("unsupported intercace(%u,%u)",gtp->c.v2_flags.version,  gtp->c.v2_flags.piggy);
          return(NULL);
      }
      if (!issupported(gtp->type)){
          Logger::LOGERR("unsupported type(%d)", gtp->type);
          return(NULL);
      }
      size_t offset = headerlen(gtp->type);
      if (offset > len){
          Logger::LOGERR("invalid length(%d, %d, %d)", offset, len , ntohs(gtp->length));
          return(NULL);
      }
      //
      GtpcPkt* pinst = new GtpcPkt(gtp->type);
      if (!pinst){ throw std::runtime_error("failed. malloc.)"); }
      for(int cnt = 0; offset < len && cnt < 256; cnt ++){
          gtpc_comm_header_ptr pitm = (gtpc_comm_header_ptr)(((char*)src) + offset);
          void* payload = ((char*)src) + offset;
          uint16_t hlen = ntohs(pitm->length) + sizeof(gtpc_comm_header_t);
          //
          if (GtpcPkt::attach_impl(pinst, payload, hlen) != RETOK){
              Logger::LOGERR("skip packet item.(%u : %u)", offset, hlen);
          }
          offset += hlen;
          //
          if (offset > len){
              // case in malformed item.
              Logger::LOGERR("malformed ...(%d, %d, %d)GtpcPkt::attach", offset, len, hlen);
              delete pinst;
              return(NULL);
          }else if (offset == len){
              return(pinst);
          }
      }
      Logger::LOGERR("malformed ...(%d, %d)GtpcPkt::attach", offset, len);
      delete pinst;
      return(NULL);
  }
  int GtpcPkt::StrSplit(const char* src, const char* sep,std::vector<std::string>& splitted ){
      char*		finded = NULL;
      char*		tmpsep = NULL;
      char*		tmpsrc = NULL;
      char*		current = NULL;
      uint32_t	seplen;
      uint32_t	srclen;
      uint32_t	busyloop_counter = 0;
      std::string tmpstr("");
      //
      if (!src || !sep)		{ return(RETERR); }
      if (!strlen(src))		{ return(RETERR); }
      if (!strlen(sep))		{ return(RETERR); }
      //
      seplen	= strlen(sep);
      srclen	= strlen(src);
      //
      tmpsep	= (char*)malloc(seplen + 1);
      memset(tmpsep,0,(seplen + 1));
      memcpy(tmpsep,sep,seplen);
      //
      tmpsrc	= (char*)malloc(srclen + 1);
      memset(tmpsrc,0,(srclen + 1));
      memcpy(tmpsrc,src,srclen);
      //
      current	= tmpsrc;
      //
      while(true){
          // find separator
          if ((finded = strstr(current,tmpsep)) == NULL){
              splitted.push_back(current);
              break;
          }
          // first char is separator.
          if (finded == current){
              finded += seplen;
          }else{
              tmpstr.assign(current,(finded - current));
              splitted.push_back(tmpstr);
          }
          current += (finded - current);
          // end of string
          if (current >= (tmpsrc + srclen)){
              break;
          }
          busyloop_counter ++;
          if (busyloop_counter > 1000){
              return(RETERR);
          }
      }
      free(tmpsrc);
      free(tmpsep);
      //
      return(RETOK);
  }

  bool issupported(__u8 type){
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
              Logger::LOGERR("not supported.type(%d)", type);
              return(false);
      }
      return(true);
  }
  size_t headerlen(int type){
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
}; // namespace MIXIPGW_TOOLS

