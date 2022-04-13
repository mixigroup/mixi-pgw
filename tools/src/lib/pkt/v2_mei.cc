//
// Created by mixi on 2017/01/12.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Mei::Mei(){
      memset(&mei_,0,sizeof(mei_));
      set(NULL,0);
  }
  Mei::Mei(void* src,size_t len){
      attach(src,len);
  }
  Mei::Mei(uint64_t digits){
      memset(&mei_,0,sizeof(mei_));
      set(digits);
  }
  Mei::~Mei(){ }
  void Mei::set(uint64_t digits){
      char bf[32] = {0};
      snprintf(bf,sizeof(bf)-1, FMT_LLU , digits);
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Mei::set(%s)", bf);
      }
      //
      if (digits != 0 && strlen(bf)==(GTPC_MEI_LEN*2-1)){
          for(int n = 0;n < GTPC_MEI_LEN;n++){
              mei_.digits[n].low = (bf[n*2+0] - '0');
              if (n == (GTPC_IMSI_LEN-1)){
                  if (Module::VERBOSE() > PCAPLEVEL_PARSE){
                      Logger::LOGINF("Mei::set[%d](%u,-)", n, (bf[n*2+0] - '0'));
                  }
                  mei_.digits[n].high = 0xf;
              }else{
                  if (Module::VERBOSE() > PCAPLEVEL_PARSE){
                      Logger::LOGINF("Mei::set[%d](%u,%u)", n, (bf[n*2+0] - '0'), (bf[n*2+1] - '0'));
                  }
                  mei_.digits[n].high = (bf[n*2+1] - '0');
              }
          }
      }
      mei_.head.type = type();
      mei_.head.length = htons(GTPC_MEI_LEN);

  }
  void Mei::set(void* mei, size_t len){
      if (mei != NULL && len == GTPC_MEI_LEN){
          memcpy(mei_.digits, mei, GTPC_MEI_LEN);
      }
      mei_.head.type = type();
      mei_.head.length = htons(GTPC_MEI_LEN);
  }
  int Mei::type(void) const{
      return(GTPC_TYPE_MEI);
  }
  void* Mei::ref(void) const{
      return((void*)&mei_);
  }
  size_t Mei::len(void) const{
      return(sizeof(gtpc_mei_t));
  }
  int Mei::attach(void* src,size_t len){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Mei::attach. (%d)..%d", len, sizeof(gtpc_mei_t));
      }
      gtpc_mei_ptr p = (gtpc_mei_ptr)src;
      if (len != sizeof(gtpc_mei_t)){
          Logger::LOGERR("Imsi::attach failed.(%d, %d)", len, sizeof(gtpc_mei_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == GTPC_MEI_LEN){
          memcpy(&mei_, p, sizeof(gtpc_mei_t));
          return(RETOK);
      }
      Logger::LOGERR("Mei::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS
