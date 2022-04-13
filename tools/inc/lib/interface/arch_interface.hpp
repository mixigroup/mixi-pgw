//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_ARCH_HPP
#define MIXIPGW_TOOLS_ARCH_HPP
//
namespace MIXIPGW_TOOLS{
  class ProcessParameter;
  class PktInterface;

  // event handler
  class ArchIfEvent{
  public:
      virtual int OnPacketType(PktInterface*, int, int) = 0;
  }; // class ArchIfEvent


  // architecture interface
  // netmap /dpdk etc.
  class ArchIf{
  public:
      enum SendType{ DC = 0,  SELF,  BRIDGE, MAX };
  public:
      static int Init(ArchIf**,ProcessParameter*);
      static int UnInit(ArchIf**);
      static int GetPacketType(PktInterface*, int, int);
  public:
      virtual int Open(const char*,int) = 0;
      virtual int Poll(int,PktInterface*) = 0;
      virtual int Send(int,PktInterface*) = 0;
      virtual int EventPacketType(ArchIfEvent*);
  protected:
      ArchIfEvent*      event_;
      ProcessParameter* param_;
      void*             handle_;
      unsigned          filter_level_;
      unsigned          module_flag_;
      unsigned short    udp_ctrl_port_;
  }; // class ArchIf
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_ARCH_HPP
