//
// Created by mixi on 2017/04/26.
//
#include "mixipgw_tools_def.h"
#include "lib/arch/osxsim.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"

using namespace MIXIPGW_TOOLS;


namespace MIXIPGW_TOOLS{
  // ---------------------------
  // osx simulation Implemented.
  Osxsim::Osxsim(ProcessParameter* param){
      param_  = param;
  }
  Osxsim::~Osxsim(){ }

  int Osxsim::Open(const char* ifnm,int cpuid){
      Logger::LOGINF("Osxsim::Open.");
      return(RETOK);
  }
  int Osxsim::Poll(int flag, PktInterface* pkt){
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Osxsim::Poll.");
      }
      usleep(1000000);
      return(RETOK);
  }
  int Osxsim::Send(int flag,PktInterface* pkt){
      Logger::LOGINF("Osxsim::Send.");
      return(RETOK);
  }
};// namespace MIXIPGW_TOOLS
