//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_BUFFER_HPP
#define MIXIPGW_TOOLS_BUFFER_HPP

namespace MIXIPGW_TOOLS{
  // packet buffers for queue.
  // management leak memory.
  // checking Buffer::MemoryTest() function
  // used control plane(does not require ultra-low latency)
  class Buffer{
  public:
      Buffer(unsigned);
      Buffer(const void*,unsigned);
      Buffer(const void*,unsigned,const char*,unsigned short);
      Buffer(const void*,unsigned,const char*,unsigned short,const char*,unsigned short);
      virtual ~Buffer();
  public:
      void* Ref(unsigned*);
      unsigned short Len(void);
      unsigned short Port(void);
      const char*    Host(void);
      unsigned short Lport(void);
      const char*    Lhost(void);
      //
      static int   Stat(unsigned*);
      static void* AllocateBuffer(unsigned);
      static void  FreeBuffer(void*);
      static int   MemoryTest(unsigned*);
  private:
      Buffer(){}
      unsigned  len_;
      char     *host_,*lhost_;
      unsigned short port_,lport_;
      void *    buffer_;
  }; // class Buffer
};// namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_BUFFER_HPP
