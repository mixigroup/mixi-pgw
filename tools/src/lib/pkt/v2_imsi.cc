//
// Created by mixi on 2017/01/12.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"

//
namespace MIXIPGW_TOOLS{
  Imsi::Imsi(){
      memset(&imsi_,0,sizeof(imsi_));
      set(0);
  }
  Imsi::Imsi(void* src,size_t len){
      attach(src,len);
  }
  Imsi::Imsi(uint64_t digits){
      memset(&imsi_,0,sizeof(imsi_));
      set(digits);
  }

  Imsi::~Imsi(){ }
  void Imsi::set(uint64_t digits){
      char bf[32] = {0};
      snprintf(bf,sizeof(bf)-1,FMT_LLU , digits);
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Imsi::set(%s)", bf);
      }
      //
      if (digits != 0 && strlen(bf)==(GTPC_IMSI_LEN*2-1)){
          for(int n = 0;n < GTPC_IMSI_LEN;n++){
              imsi_.digits[n].low = (bf[n*2+0] - '0');
              if (n == (GTPC_IMSI_LEN-1)){
                  if (Module::VERBOSE() > PCAPLEVEL_PARSE){
                      Logger::LOGINF("imsi::set[%d](%u,-)", n, (bf[n*2+0] - '0'));
                  }
                  imsi_.digits[n].high = 0xf;
              }else{
                  if (Module::VERBOSE() > PCAPLEVEL_PARSE){
                      Logger::LOGINF("imsi::set[%d](%u,%u)", n, (bf[n*2+0] - '0'), (bf[n*2+1] - '0'));
                  }
                  imsi_.digits[n].high = (bf[n*2+1] - '0');
              }
          }
      }
      imsi_.head.type = type();
      imsi_.head.length = htons(GTPC_IMSI_LEN);
  }
  void Imsi::set(void* imsi, size_t len){
      if (imsi != NULL && len == GTPC_IMSI_LEN && (uint8_t)((uint8_t)(((char*)imsi)[GTPC_IMSI_LEN-1])>>4) == (uint8_t)0x0f){
          memcpy(imsi_.digits, imsi, GTPC_IMSI_LEN);
      }
      imsi_.head.type = type();
      imsi_.head.length = htons(GTPC_IMSI_LEN);
  }
  int Imsi::type(void) const{
      return(GTPC_TYPE_IMSI);
  }
  void* Imsi::ref(void) const{
      return((void*)&imsi_);
  }
  size_t Imsi::len(void) const{
      return(sizeof(gtpc_imsi_t));
  }
  int Imsi::attach(void* src,size_t len){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Imsi::attach. (%d)..%d", len, sizeof(gtpc_imsi_t));
      }
      gtpc_imsi_ptr p = (gtpc_imsi_ptr)src;
      if (len != sizeof(gtpc_imsi_t)){
          Logger::LOGERR("Imsi::attach failed.(%d, %d)", len, sizeof(gtpc_imsi_t));
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == GTPC_IMSI_LEN){
          memcpy(&imsi_, p, sizeof(gtpc_imsi_t));
          return(RETOK);
      }
      Logger::LOGERR("Imsi::attach failed.......(%d, %d)", len, ntohs(p->head.length));
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS
