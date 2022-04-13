//
// Created by mixi on 2017/04/21.
//

#include "gtest/gtest.h"

#include "lib/module.hpp"
#include "mixipgw_tools_def.h"
#include <boost/thread/thread.hpp>

TEST(Module, Instanciate){
    for(int n = 0; n < 10000;n++){
        EXPECT_EQ(MIXIPGW_TOOLS::Module::Init(NULL), RETOK);
        EXPECT_EQ(MIXIPGW_TOOLS::Module::Uninit(NULL), RETOK);
    }
}

TEST(Module, GetSet){
    EXPECT_EQ(MIXIPGW_TOOLS::Module::Init(NULL), RETOK);
    EXPECT_EQ(MIXIPGW_TOOLS::Module::ABORT(),0);
    for(int n = 0;n < 5871;n++){
        MIXIPGW_TOOLS::Module::ABORT_INCR();
    }
    EXPECT_EQ(MIXIPGW_TOOLS::Module::ABORT(),5871);
    MIXIPGW_TOOLS::Module::ABORT_CLR();
    EXPECT_EQ(MIXIPGW_TOOLS::Module::ABORT(),0);

    EXPECT_EQ(MIXIPGW_TOOLS::Module::Uninit(NULL), RETOK);
}
TEST(Verbose, GetSet){
    EXPECT_EQ(MIXIPGW_TOOLS::Module::Init(NULL), RETOK);
    EXPECT_EQ(MIXIPGW_TOOLS::Module::VERBOSE(),0);
    for(int n = 0;n < 5871;n++){
        MIXIPGW_TOOLS::Module::VERBOSE_INCR();
    }
    EXPECT_EQ(MIXIPGW_TOOLS::Module::VERBOSE(),5871);
    MIXIPGW_TOOLS::Module::VERBOSE_CLR();
    EXPECT_EQ(MIXIPGW_TOOLS::Module::VERBOSE(),0);

    EXPECT_EQ(MIXIPGW_TOOLS::Module::Uninit(NULL), RETOK);
}

TEST(Verbose, Thread){
    EXPECT_EQ(MIXIPGW_TOOLS::Module::Init(NULL), RETOK);
    EXPECT_EQ(MIXIPGW_TOOLS::Module::VERBOSE(),0);
    for(int n = 0;n < 5871;n++){
        MIXIPGW_TOOLS::Module::VERBOSE_INCR();
    }
    boost::thread_group th;
    th.create_thread([]{
        EXPECT_EQ(MIXIPGW_TOOLS::Module::VERBOSE(),5871);
        MIXIPGW_TOOLS::Module::VERBOSE_CLR();
    });
    th.join_all();

    EXPECT_EQ(MIXIPGW_TOOLS::Module::VERBOSE(),0);

    EXPECT_EQ(MIXIPGW_TOOLS::Module::Uninit(NULL), RETOK);
}

