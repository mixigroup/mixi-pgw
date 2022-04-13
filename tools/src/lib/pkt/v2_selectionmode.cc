//
// Created by mixi on 2017/01/16.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  SelectionMode::SelectionMode(){
      memset(&sel_,0,sizeof(sel_));
      set(0);
  }
  SelectionMode::SelectionMode(void* src,size_t len){
      attach(src,len);
  }
  SelectionMode::SelectionMode(uint8_t selection_mode){
      memset(&sel_,0,sizeof(sel_));
      set(selection_mode);
  }
  SelectionMode::~SelectionMode(){ }
  void SelectionMode::set(uint8_t selection_mode){
      sel_.head.type = type();
      sel_.head.length = htons(1);
      sel_.c.bit.select_mode = selection_mode;
  }
  int SelectionMode::type(void) const{
      return(GTPC_TYPE_SELECTION_MODE);
  }
  void* SelectionMode::ref(void) const{
      return((void*)&sel_);
  }
  size_t SelectionMode::len(void) const{
      return(sizeof(gtpc_selection_mode_t));
  }
  int SelectionMode::attach(void* src,size_t len){
      gtpc_selection_mode_ptr p = (gtpc_selection_mode_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("SelectionMode::attach. (%d)..%d", len, sizeof(gtpc_selection_mode_t));
      }
      if (len != 5){
          return(RETERR);
      }
      if (p->head.type == type() && ntohs(p->head.length) == 1){
          memcpy(&sel_, p, len);
          return(RETOK);
      }
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS