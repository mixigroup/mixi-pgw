//
// Created by mixi on 2017/04/21.
//
#include "mixipgw_tools_def.h"
#include "lib/module.hpp"

namespace MIXIPGW_TOOLS{
  static pthread_mutex_t static_mtx;
  Module* Module::pthis_ = NULL;

  int  Module::Init(Module** ppmod){
      pthread_mutex_lock(&static_mtx);
      if (!pthis_){
          pthis_ = new Module();
      }
      pthread_mutex_unlock(&static_mtx);

      if (ppmod != NULL){
          (*ppmod) = pthis_;
      }
      return(RETOK);
  }
  int  Module::Uninit(Module** ppmod){
      if (pthis_ != NULL){
          delete pthis_;
      }
      if (ppmod != NULL){
          (*ppmod) = NULL;
      }
      pthis_ = NULL;
      return(RETOK);
  }
  Module::Module():abort_(0),verbose_(0){}
  Module::~Module(){}

  int  Module::ABORT(void){
      if (pthis_){
          return(pthis_->abort_);
      }
      return(0);
  }
  void Module::ABORT_INCR(void){
      if (pthis_){
          pthis_->abort_ ++;
      }
  }
  void Module::ABORT_CLR(void){
      if (pthis_){
          pthis_->abort_ = 0;
      }
  }
  int  Module::VERBOSE(void){
      if (pthis_){
          return(pthis_->verbose_);
      }
      return(0);
  }
  void Module::VERBOSE_INCR(void){
      if (pthis_){
          pthis_->verbose_++;
      }
  }
  void Module::VERBOSE_CLR(void){
      if (pthis_){
          pthis_->verbose_ = 0;
      }
  }
}; // namespace MIXIPGW_TOOLS
