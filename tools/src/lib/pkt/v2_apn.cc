//
// Created by mixi on 2017/01/12.
//
#include "mixipgw_tools_def.h"
#include "lib/packet/gtpc_items.hpp"

using namespace MIXIPGW_TOOLS;
//
namespace MIXIPGW_TOOLS{
  Apn::Apn(){
      memset(&apn_,0,sizeof(apn_));
      set(NULL,0);
  }
  Apn::Apn(void* src,size_t len){
      attach(src,len);
  }
  Apn::Apn(const char* apn){
      memset(&apn_,0,sizeof(apn_));
      set(apn);
  }
  Apn::~Apn(){ }
  void Apn::set(const char* apn){
      std::vector<std::string> splitted;
      std::vector<std::string>::iterator itr;
      uint8_t ttl = 0;
      //
      set(NULL,0);
      //
      if (apn != NULL){
          if (GtpcPkt::StrSplit(apn, ".",splitted) == RETOK){
              for(itr = splitted.begin();itr != splitted.end();++itr){
                  uint8_t len = (uint8_t)(*itr).length();
                  //
                  if ((ttl+len) >= GTPC_APN_LEN){
                      set(NULL, 0);
                      return;
                  }
                  apn_.apn[ttl+0] = len;
                  memcpy(&apn_.apn[ttl+1], (*itr).c_str(), len);
                  ttl += (len+1);
              }
              apn_.head.length = htons(ttl);
              apn_.head.type = type();
              return;
          }
      }
  }
  void Apn::set(void* apn, size_t len){
      if (apn != NULL){
          memcpy(apn_.apn, apn ,MIN(len, GTPC_APN_LEN));
          apn_.head.length = htons(MIN(len, GTPC_APN_LEN));
      }else{
          apn_.head.length = 0;
      }
      apn_.head.type = type();
  }
  std::string Apn::name(void){
      std::string   ret;
      uint8_t       len = 0,n = 0;
      char*         ptr;
      while(n < MIN(GTPC_APN_LEN, ntohs(apn_.head.length))){
          len = apn_.apn[n];
          std::string   bf((char*)&(apn_.apn[n+1]), (size_t)len);
          if (n != 0){
              ret += ".";
          }
          ret += bf;
          n += (len+1);
      };
      return(ret);
  }

  int Apn::type(void) const{
      return(GTPC_TYPE_APN);
  }
  void* Apn::ref(void) const{
      return((void*)&apn_);
  }
  size_t Apn::len(void) const{
      return(ntohs(apn_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Apn::attach(void* src,size_t len){
      gtpc_apn_ptr p = (gtpc_apn_ptr)src;
      if (len <= sizeof(gtpc_comm_header_t)){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) > 0 && ntohs(p->head.length) <= GTPC_APN_LEN){
          memcpy(&apn_, p, ntohs(p->head.length) + sizeof(gtpc_comm_header_t));
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS