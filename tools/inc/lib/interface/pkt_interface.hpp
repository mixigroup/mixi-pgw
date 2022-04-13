//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_PKT_INTERFACE_HPP
#define MIXIPGW_TOOLS_PKT_INTERFACE_HPP

namespace MIXIPGW_TOOLS{
  class PktInterface{
  public:
      enum { DC = 0, ETHER, IP, UDP, GTPU, PAYLOAD, ARP, BFD, IP_I, ICMP, ICMP_I,  MAX };
  public:
      virtual void* Header(int) = 0;
      virtual int   Length(void) = 0;
      virtual void  Length(int) = 0;
      virtual void  Type(int) = 0;
      virtual int   Type(void) = 0;
      virtual int   LengthVlan(void) = 0;
      virtual int   LengthIp(void) = 0;
  };
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_PKT_INTERFACE_HPP
