//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_BFD_HPP
#define MIXIPGW_TOOLS_BFD_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // Bfd filter (on netmap), unnecessary cleanup.
  class Bfd: public FilterIf{
  public:
      virtual int OnInitFilter(void);
      virtual int OnFilter(PktInterface*,int*);
  private:
      unsigned bfd_recv_counter_;
  }; // class Bfd
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_BFD_HPP
