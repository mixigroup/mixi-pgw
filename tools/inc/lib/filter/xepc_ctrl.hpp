//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_EPC_CTRL_HPP
#define MIXIPGW_TOOLS_EPC_CTRL_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // EpcCtrl ( mixipgw ) , do not require initialize, cleanup
  class EpcCtrl: public FilterIf{
  public:
      virtual int OnFilter(PktInterface*,int*);
  }; // class EpcCtrl
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_EPC_CTRL_HPP
