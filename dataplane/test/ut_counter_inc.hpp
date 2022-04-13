#ifndef MIXI_PGW_DPDK_UT_COUNTER_INC_HPP
#define MIXI_PGW_DPDK_UT_COUNTER_INC_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_malloc.h>
#include <rte_kni.h>
#include <rte_hexdump.h>

#include <assert.h>

#include "gtest/gtest.h"
#include "test_def.hpp"

#include <typeinfo>
#include <string>
#include <map>
#include <mysql.h>

class CustomEnvironment :public ::testing::Environment {
public:
    virtual ~CustomEnvironment() {}
    virtual void SetUp() { }
    virtual void TearDown() { }
};

class Mysql {
  public:
      Mysql(){
        static bool init = false;
        if (!init){
            mysql_library_init(0,0,0);
            init = true;
        }
        if ((state_ = Connect()) != 0){
            fprintf(stderr, "failed .Connect . (%s)\n", strerror(errno));
            exit(1);
        }
      }
      virtual ~Mysql(){ Disconnect(); }
  public:
      int Query(const char * sql){
          return (mysql_query(dbhandle_, sql));
      }
  public:
      int Connect(){
          mysql_thread_init();
          bool reconnect = 1;
          if ((dbhandle_ = mysql_init(NULL)) == NULL) {
              fprintf(stderr, "mysql_init . (%s)\n", strerror(errno));
              return (-1);
          }
          mysql_options(dbhandle_, MYSQL_OPT_RECONNECT, &reconnect);
          if (mysql_real_connect(dbhandle_,
                                "127.0.0.1",
                                "root",
                                "develop",
                                "mixipgw",
                                3306,
                                NULL,
                                0)) {
              mysql_set_character_set(dbhandle_, "utf8");
              return (0);
          }
          return (-1);
      }
      void Disconnect(void){
          if (dbhandle_ != NULL) {
              mysql_close(dbhandle_);
          }
          dbhandle_ = NULL;
      }
  public:
      MYSQL *dbhandle_;
      int state_;
  };// class Mysql

#endif //MIXI_PGW_DPDK_UT_COUNTER_INC_HPP
