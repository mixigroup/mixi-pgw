//
// Created by mixi on 2017/01/15.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
#include <assert.h>
//
namespace MIXIPGW_TOOLS{
  BearerContext::BearerContext():child_(new GtpcPkt(GTPC_TYPE_BEARER_CTX)){
      assert(child_);
      memset(payload_,0,sizeof(payload_));
      calc();
  }
  BearerContext::BearerContext(void* src,size_t len):child_(new GtpcPkt(GTPC_TYPE_BEARER_CTX)){
      attach(src,len);
  }
  BearerContext::~BearerContext(){
      if (child_ != NULL){
          delete child_;
      }
      child_ = NULL;
  }
  void BearerContext::calc(void){
      gtpc_comm_header_ptr ph = (gtpc_comm_header_ptr)payload_;
      ph->type = type();
      ph->length = htons(len() - sizeof(gtpc_comm_header_t));
      offset_ = sizeof(gtpc_comm_header_t);
      //
      child_->iterate([](GtpcItem* i, void* u)->int{
          BearerContext* pthis = (BearerContext*)u;
          //
          memcpy(&pthis->payload_[pthis->offset_],i->ref(), i->len());
          pthis->offset_ += i->len();
          return(0);
      },this);
  }
  void BearerContext::append(const GtpcItem& item){
      if (child_ != NULL){
          child_->append(&item);
      }
      calc();
  }
  void BearerContext::append(GtpcItem* item){
      if (child_ != NULL){
          child_->append(item);
      }
      calc();
  }
  void BearerContext::iterate(iterate_gtpc cb,void* udata){
      if (!child_){
          return;
      }
      child_->iterate(cb, udata);
  }
  GtpcPkt* BearerContext::child(void){
      return(child_);
  }

  int BearerContext::type(void) const{
      return(GTPC_TYPE_BEARER_CTX);
  }
  void* BearerContext::ref(void) const{
      return((void*)payload_);
  }
  size_t BearerContext::len(void) const{
      size_t len = 0;
      child_->iterate([](GtpcItem* i, void* u)->int{
          char bf[32] = {0};
          *((size_t*)u) += i->len();
          snprintf(bf,sizeof(bf)-1,"BearerContext::len(%zu)", i->len());
          return(0);
      },&len);
      return(len + sizeof(gtpc_comm_header_t));
  }
  int BearerContext::attach(void* src,size_t len){
      gtpc_comm_header_ptr p = (gtpc_comm_header_ptr)src;
      int ret = RETOK;
      if (len < sizeof(gtpc_header_t)){
          Logger::LOGERR("BearerContext::attach failed.(%d, %d)", len, sizeof(gtpc_comm_header_t));
          return(RETERR);
      }
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("BearerContext::attach[:start]");
      }
      Logger::LOGDEPTH(1);
      if (p->type == type() && ntohs(p->length) > sizeof(gtpc_comm_header_t)){
          size_t cnt;
          memcpy(payload_, p , sizeof(gtpc_comm_header_t));
          for(offset_ = sizeof(gtpc_comm_header_t);offset_ < len;cnt++){
              gtpc_comm_header_ptr pitm = (gtpc_comm_header_ptr)(((char*)src) + offset_);
              void* payload = ((char*)src) + offset_;
              uint16_t hlen = ntohs(pitm->length) + sizeof(gtpc_comm_header_t);
              //
              if (GtpcPkt::attach_impl(child_, payload, hlen) != RETOK){
                  Logger::LOGERR("malformed .GtpcPkt::attach_impl..(%d, %d, %d) on BearerContext::attach", offset_, len, hlen);
                  ret = RETERR;
                  break;
              }
              //
              offset_ += hlen;
              if (offset_ > len){
                  // case in malformed item.
                  Logger::LOGERR("malformed ...(%d, %d, %d) on BearerContext::attach", offset_, len, hlen);
                  ret = RETERR;
                  break;
              }else if (offset_ == len){
                  if (Module::VERBOSE() > PCAPLEVEL_PARSE){
                      Logger::LOGINF("parse succeeded. on BearerContext::attach");
                  }
                  break;
              }
          }
          calc();
          //
          Logger::LOGDEPTH(-1);
          return(ret);
      }
      Logger::LOGERR("BearerContext::attach... failed.(%d, %d)", ntohs(p->length), p->type);
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS