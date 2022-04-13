//
// Created by mixi on 2017/04/21.
//

#include <unistd.h>
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "mixipgw_tools_def.h"
#include "log4cpp/RemoteSyslogAppender.hh"
#include "log4cpp/SyslogAppender.hh"
#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/Priority.hh"

#include <string>
#include <fstream>

#define TMPPATH ("/tmp/")
#define STRPTR(x)   (*((std::string*)x))
#define FILEPTR(x)  (FILE*)(x)

#define PRINTFNC()  printf("%s:%d\n",__FILE__,__LINE__)

namespace MIXIPGW_TOOLS{
  static pthread_mutex_t static_mtx;
  Logger* Logger::pthis_ = NULL;

  static inline bool file_is_appendable(const std::string& path) {
      std::ofstream check_appendable_file;
      check_appendable_file.open(path.c_str(), std::ios::app);
      if (check_appendable_file.is_open()) {
          // all fine, just close file
          check_appendable_file.close();

          return true;
      } else {
          return false;
      }
  }
  static inline std::string DEPTH(int d){
      std::string depth("");
      for(int n = 0;n < d;n++){
      //  depth += "\t";
      }
      return(depth);
  }
  int  Logger::Init(const char* mod, const char* dir,Logger** ppmod){

      pthread_mutex_lock(&static_mtx);
      if (!pthis_){
          pthis_ = new Logger(mod, dir);
      }
      pthread_mutex_unlock(&static_mtx);
      if (ppmod != NULL){
          (*ppmod) = pthis_;
      }
      // level
      if (Module::VERBOSE() > PCAPLEVEL_MIN){
          START_PCAP();
      }
      return(RETOK);
  }
  int  Logger::Uninit(Logger** ppmod){
      if (Module::VERBOSE() > PCAPLEVEL_MIN){
          CLOSE_PCAP();
      }
      if (pthis_ != NULL){
          delete pthis_;
      }
      if (ppmod != NULL){
          (*ppmod) = NULL;
      }
      pthis_ = NULL;
      return(RETOK);
  }
  Logger::Logger(const char* mod, const char* dir):
        priority_(log4cpp::Priority::DEBUG),depth_(0),path_(new std::string()),
        pcapf_(NULL),pcappath_(new std::string()),pcapcounter_(0){
      assert(path_);
      assert(pcappath_);
      if (dir != NULL){
          STRPTR(path_) = dir;
      }else{
          STRPTR(path_) = "./";
      }
      if (!file_is_appendable(STRPTR(path_) + "/" + mod + ".log")) {
          std::cerr << "Can't open log file " << STRPTR(path_) << "/" << mod << ".log" << " for writing! Please check file and folder permissions" << std::endl;
          exit(EXIT_FAILURE);
      }
      log4cpp::PatternLayout* layout = new log4cpp::PatternLayout();
      layout->setConversionPattern("%d [%p] %m%n");

      log4cpp::Appender* appender = new log4cpp::SyslogAppender("default", mod);
      appender->setLayout(layout);

      log4cpp::Category::getRoot().setPriority(priority_);
      log4cpp::Category::getRoot().addAppender(appender);

      log4cpp::Category::getRoot() << log4cpp::Priority::INFO << "Logger initialized!";
  }
  Logger::~Logger(){
      if (pcapf_){
          fclose((FILE*)pcapf_);
      }
      pcapf_ = NULL;
      if (path_){
          delete ((std::string*)path_);
      }
      if (pcappath_){
          delete ((std::string*)pcappath_);
      }
      path_ = NULL;
  }
  void Logger::LOGDEPTH(int depth){
      if (pthis_){
          pthis_->depth_ += depth;
      }
  }
  void Logger::LOGERR(const char* fmt,...){
      char     bf[1024] = {0};
      char     bf2[1024 + 32] = {0};
      va_list  ap;
      va_start(ap, fmt);
      vsnprintf(bf,sizeof(bf)-1,fmt, ap);
      snprintf(bf2, sizeof(bf2)-1,"[ERR][pid: %08x : tid: %p]%s%s", getpid(), (void*)pthread_self(), DEPTH(pthis_==NULL?0:pthis_->depth_).c_str(), bf);
      va_end(ap);
#ifdef __APPLE__
      fprintf(stderr, "%s\n", bf2);
#endif
#ifndef __NO_LOG__
      log4cpp::Category::getRoot().error(bf2);
#endif
  }
  void Logger::LOGINF(const char* fmt,...){
      char     bf[1024] = {0};
      char     bf2[1024 + 32] = {0};
      va_list  ap;
      va_start(ap, fmt);
      vsnprintf(bf,sizeof(bf)-1,fmt, ap);
      snprintf(bf2, sizeof(bf2)-1,"[INF][pid: %08x : tid: %p]%s%s", getpid(), (void*)pthread_self(), DEPTH(pthis_==NULL?0:pthis_->depth_).c_str(), bf);
#ifndef __NO_LOG__
      log4cpp::Category::getRoot().info(bf2);
#endif
      va_end(ap);
#ifdef __APPLE__
      printf("%s\n", bf2);
#endif
  }
  void Logger::LOGDBG(const char* fmt,...){
#ifndef __NO_LOG__
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          char     bf[1024] = {0};
          va_list  ap;
          va_start(ap, fmt);
          vsnprintf(bf,sizeof(bf)-1,fmt, ap);
          log4cpp::Category::getRoot().debug(bf);
          va_end(ap);
      }
#endif
  }
  void Logger::LOGWRN(const char* fmt,...){
      char     bf[1024] = {0};
      char     bf2[1024 + 32] = {0};
      va_list  ap;
      va_start(ap, fmt);
      vsnprintf(bf,sizeof(bf)-1,fmt, ap);
      snprintf(bf2, sizeof(bf2)-1,"[WRN][pid: %08x : tid: %p]%s%s", getpid(), (void*)pthread_self(), DEPTH(pthis_==NULL?0:pthis_->depth_).c_str(), bf);
#ifndef __NO_LOG__
      log4cpp::Category::getRoot().warn(bf2);
#endif
      va_end(ap);
  }
  void Logger::LOGDMP(const char* prifix, const char* data,unsigned len){
#ifndef __NO_LOG__
      std::string logmsg;
      char bf[1024] = {0};
      size_t n;
      //
      snprintf(bf,sizeof(bf)-1,"[DMP][pid: %08x : tid: %p]%s ", getpid(), (void*)pthread_self(), prifix);
      logmsg = bf;
      for(n = 0;n < len;n++) {
          if (n == 0){
              logmsg += "\n";
          }
          bf[0] = '\0';
          snprintf(bf,sizeof(bf)-1,"%02X", ((unsigned char)data[n]));
          logmsg += bf;
          if ((n+1)%8 == 0){
              logmsg += " ";
          }else if ((n+1)%16==0){
              logmsg += "\n";
          }
      }
      log4cpp::Category::getRoot().info(logmsg.c_str());
#endif
  }
  static inline std::string pcapf(const std::string& dir){
      char pid[32] = {0};
      sprintf(pid, "%d", (unsigned)getpid());
      return(dir + pid + ".pcap");
  }
  void Logger::START_PCAP(void){
      if (pthis_){
          std::string path = STRPTR(pthis_->pcappath_);
          unlink(pcapf(STRPTR(pthis_->pcappath_)).c_str());
          if (!file_is_appendable(pcapf(STRPTR(pthis_->pcappath_)))) {
              throw std::runtime_error("cannot append. pcap file: " + pcapf(STRPTR(pthis_->pcappath_)));
          }
          pthis_->pcapf_ = fopen(pcapf(STRPTR(pthis_->pcappath_)).c_str(),"a+");
      }
  }
    void Logger::APPEND_PCAP(int flag, const char* data,unsigned len){
        pcap_fheader_t  fhead;
        pcap_pkthdr_t   pkth;
        //
        if (pthis_ == NULL){ return; }
        if (!pthis_->pcapf_ || (len + sizeof(pkth)) > ETHER_MAX_LEN){ return; }
        // file header if need.
        if (!pthis_->pcapcounter_++){
            memset(&fhead,0,sizeof(fhead));
            fhead.magic = htonl(0xd4c3b2a1);
            fhead.major = 2;
            fhead.minor = 4;
            fhead.snap  = htonl(0x00000400);
            fhead.ltype = htonl(0x01000000);
            //
            if (fwrite(&fhead, 1, sizeof(fhead), FILEPTR(pthis_->pcapf_)) == 0){
                std::string emsg = "fwrite: file header: ("; emsg += strerror(errno); emsg += ")";
                throw std::runtime_error(emsg.c_str());
            }
        }
        //
        pkth.tm_sec  = (uint32_t)(pthis_->pcapcounter_>>32);
        pkth.tm_usec = (uint32_t)(pthis_->pcapcounter_&0xffffffff);
        pkth.pcaplen = (uint32_t)(len);
        pkth.len     = (uint32_t)(len);
        if (fwrite(&pkth, 1, sizeof(pkth), FILEPTR(pthis_->pcapf_)) == 0){
            std::string emsg = "fwrite: packet header: ("; emsg += strerror(errno); emsg += ")";
            throw std::runtime_error(emsg.c_str());
        }
        // user payload
        if (fwrite(data, 1, len, FILEPTR(pthis_->pcapf_)) == 0){
            std::string emsg = "fwrite: packet payload: ("; emsg += strerror(errno); emsg += ")";
            throw std::runtime_error(emsg.c_str());
        }
        // flush
        // TBD: need update interval?
        fflush(FILEPTR(pthis_->pcapf_));
    }
    void Logger::CLOSE_PCAP(void){
        if (pthis_ == NULL){ return; }
        if (pthis_->pcapf_ != NULL){
            fclose(FILEPTR(pthis_->pcapf_));
        }
        pthis_->pcapf_ = NULL;
    }
}; // namespace MIXIPGW_TOOLS
