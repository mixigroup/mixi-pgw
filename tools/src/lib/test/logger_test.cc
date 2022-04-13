//
// Created by mixi on 2017/04/21.
//

#include "gtest/gtest.h"

#include "lib/logger.hpp"
#include "mixipgw_tools_def.h"
#include <boost/thread/thread.hpp>

TEST(Logger, Instanciate){
    for(int n = 0; n < 100;n++){
        EXPECT_EQ(MIXIPGW_TOOLS::Logger::Init("",NULL,NULL), RETOK);
        EXPECT_EQ(MIXIPGW_TOOLS::Logger::Uninit(NULL), RETOK);
    }
}

TEST(Logger, MultiThreadLogging){
    EXPECT_EQ(MIXIPGW_TOOLS::Logger::Init("",NULL,NULL), RETOK);

    boost::thread_group th;
    th.create_thread([]{
        for(int n = 0;n < 9;n++){
            MIXIPGW_TOOLS::Logger::LOGERR("child thread (%d)",n);
            MIXIPGW_TOOLS::Logger::LOGERR("child thread (%d)",n);
        }
    });
    for(int n = 0;n < 5;n ++){
        MIXIPGW_TOOLS::Logger::LOGERR("test - logging (%d)",n);
        MIXIPGW_TOOLS::Logger::LOGWRN("test - logging (%d)",n);
        MIXIPGW_TOOLS::Logger::LOGINF("test - logging (%d)",n);
        MIXIPGW_TOOLS::Logger::LOGDBG("test - logging (%d)",n);
        MIXIPGW_TOOLS::Logger::LOGDMP("test - dbg","123456",6);
    }
    th.join_all();

    //
    MIXIPGW_TOOLS::Logger::START_PCAP();
    for(int n = 0;n < 11;n++){
        std::string msg("p");
        for(int m = 0;m < (n+1);m++){
            msg += ".";
        }
        MIXIPGW_TOOLS::Logger::APPEND_PCAP(0,msg.c_str(),msg.length());
    }
    MIXIPGW_TOOLS::Logger::CLOSE_PCAP();

    EXPECT_EQ(MIXIPGW_TOOLS::Logger::Uninit(NULL), RETOK);
}

