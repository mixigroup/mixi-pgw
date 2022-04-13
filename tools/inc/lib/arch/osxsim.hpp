//
// Created by mixi on 2017/04/26.
//

#ifndef MIXIPGW_TOOLS_OSXSIM_HPP
#define MIXIPGW_TOOLS_OSXSIM_HPP

#include "lib/interface/arch_interface.hpp"

namespace MIXIPGW_TOOLS{
  // osx simulation
  class Osxsim:public ArchIf{
  public:
      Osxsim(ProcessParameter*);
      virtual ~Osxsim();
  public:
      //
      virtual int Open(const char*,int);
      virtual int Poll(int,PktInterface*);
      virtual int Send(int,PktInterface*);
  }; // class Osxsim

}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_OSXSIM_HPP
