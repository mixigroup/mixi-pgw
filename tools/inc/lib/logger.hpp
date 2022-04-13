//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_LOGGER_HPP
#define MIXIPGW_TOOLS_LOGGER_HPP
//
namespace MIXIPGW_TOOLS{
  // logger object
  // singleton instance
  // available global access.
  class Logger{
  public:
      static int  Init(const char*,const char*,Logger**);
      static int  Uninit(Logger**);
  public:
      static void LOGDEPTH(int);
      static void LOGERR(const char* fmt,...);
      static void LOGWRN(const char* fmt,...);
      static void LOGINF(const char* fmt,...);
      static void LOGDBG(const char* fmt,...);
      static void LOGDMP(const char*,const char* ,unsigned);
      // for pcap
      static void START_PCAP(void);
      static void APPEND_PCAP(int,const char*,unsigned);
      static void CLOSE_PCAP(void);
  private:
      int priority_;
      int depth_;
      void* path_;
      void* pcappath_;
      void* pcapf_;
      unsigned long long pcapcounter_;
      static Logger* pthis_;
  private:
      Logger(){}
      Logger(const char*,const char*);
      ~Logger();
  }; // class Logger
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_LOGGER_HPP
