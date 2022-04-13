//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_GTPU_ECHO_HPP
#define MIXIPGW_TOOLS_GTPU_ECHO_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // gtp-u tunnel process sub(keepalive)
  class GtpuEcho: public FilterIf{
  public:
      virtual int OnFilter(PktInterface*,int*);
  }; // class GtpuEcho
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_GTPU_ECHO_HPP
