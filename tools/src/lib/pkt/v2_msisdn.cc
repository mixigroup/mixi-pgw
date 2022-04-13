//
// Created by mixi on 2017/01/12.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Msisdn::Msisdn(){
      memset(&msisdn_,0,sizeof(msisdn_));
      set(0);
  }
  Msisdn::Msisdn(void* src,size_t len){
      attach(src,len);
  }
  Msisdn::Msisdn(uint64_t digits){
      memset(&msisdn_,0,sizeof(msisdn_));
      set(digits);
  }
  Msisdn::~Msisdn(){ }
  void Msisdn::set(uint64_t digits){
      char bf[32] = {0};
      snprintf(bf,sizeof(bf)-1, FMT_LLU , digits);
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Msisdn::set(%s)", bf);
      }
      //
      if (digits != 0 && strlen(bf)==GTPC_MSISDN_LEN*2){
          for(int n = 0;n < GTPC_MSISDN_LEN;n++){
              if (Module::VERBOSE() > PCAPLEVEL_PARSE){
                  Logger::LOGINF("Msisdn::set[%d](%u,%u)", n, (bf[n*2+0] - '0'), (bf[n*2+1] - '0'));
              }
              msisdn_.digits[n].low = (bf[n*2+0] - '0');
              msisdn_.digits[n].high = (bf[n*2+1] - '0');
          }
      }
      msisdn_.head.type = type();
      msisdn_.head.length = htons(GTPC_MSISDN_LEN);
  }
  int Msisdn::type(void) const{
      return(GTPC_TYPE_MSISDN);
  }
  void* Msisdn::ref(void) const{
      return((void*)&msisdn_);
  }
  size_t Msisdn::len(void) const{
      return(sizeof(gtpc_msisdn_t));
  }
  int Msisdn::attach(void* src,size_t len){
      gtpc_msisdn_ptr p = (gtpc_msisdn_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Msisdn::attach. (%d)..%d", len, sizeof(gtpc_msisdn_t));
      }
      if (len != sizeof(gtpc_msisdn_t)){
          Logger::LOGERR("Msisdn::attach failed.(%d, %d)", len, sizeof(gtpc_msisdn_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == GTPC_MSISDN_LEN){
          memcpy(&msisdn_, p, sizeof(gtpc_msisdn_t));
          return(RETOK);
      }
      Logger::LOGERR("Msisdn::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS
