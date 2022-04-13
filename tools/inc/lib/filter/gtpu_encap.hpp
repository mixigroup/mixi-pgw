//
// Created by mixi on 2017/04/26.
//

#ifndef MIXIPGW_TOOLS_GTPU_ENCAP_HPP
#define MIXIPGW_TOOLS_GTPU_ENCAP_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // gtp-u tunnel process/encap - mark and drop
  class TranslatorEncapEnforcer: public FilterIf{
  public:
      virtual int OnInitFilter(void);
      virtual int OnFilter(PktInterface*,int*);
      virtual int OnUnInitFilter(void);
  protected:
      struct encap_header*  encap_;
      uint32_t recalc_counter_;
  }; // class TranslatorEncapEnforcer

  // gtp-u tunnel process/encap - only mark
  class TranslatorEncapMarker: public TranslatorEncapEnforcer{
  public:
      virtual int OnFilter(PktInterface*,int*);
  }; // class TranslatorEncapMarker



}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_GTPU_ENCAP_HPP
