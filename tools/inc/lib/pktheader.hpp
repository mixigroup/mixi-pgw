//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_PKTHEADER_HPP
#define MIXIPGW_TOOLS_PKTHEADER_HPP
#include "lib/interface/pkt_interface.hpp"

namespace MIXIPGW_TOOLS{
  // store offset of parsed packet header.
  // this interface allows each module to access payload directory from offset values.
  // Only functions to hold offset.
  class PktHeader: public PktInterface{
  public:
      PktHeader(char*,int);
      ~PktHeader();
  public:
      virtual void* Header(int);
      virtual int   Length(void);
      virtual void  Length(int);
      virtual void  Type(int);
      virtual int   Type(void);
      virtual int   LengthVlan(void);
      virtual int   LengthIp(void);
  private:
      char* data_;
      int   len_;
      int   len_vlan_;
      int   len_ipv_;
      int   offset_[PktInterface::MAX];
      int   type_;
  private:
      PktHeader(){}   // witout copy
  }; // class PktHeader
}; // namespace MIXIPGW_TOOLS
#endif //MIXIPGW_TOOLS_PKTHEADER_HPP
