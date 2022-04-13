//
// Created by mixi on 2017/04/28.
//


#include "gtest/gtest.h"

#include "mixipgw_tools_def.h"
#include "lib/process.hpp"
#include <boost/thread/thread.hpp>

using namespace MIXIPGW_TOOLS;


class Good{
public:
    Good():Good(0,100){}
    Good(int min,int max):min_(min),max_(max){ }
public:
    int min_;
    int max_;
};

class NoGood{
public:
    NoGood(){
        NoGood(0,100);
    }
    NoGood(int min,int max):min_(min),max_(max){ }
public:
    int min_;
    int max_;
};

class Cpp1{
public:
    Cpp1():h_(0),c_{}{}
    ~Cpp1(){}
public:
    void Print(void){ fprintf(stdout, "hoge Cpp1.\n"); }
private:
    char c_0[3];
    int h_;
    char c_[118];
};


TEST(Basic, Compilter){
    Cpp1    cpp1;
    fprintf(stdout, "Cpp1: %u\n", sizeof(cpp1));
    cpp1.Print();
    EXPECT_EQ(sizeof(cpp1),128);
}

TEST(Basic, ClassConstract){
    ProcessParameter p;
    ProcessParameter np(NULL);
    //
    for(auto i = (int)ProcessParameter::TXT_CLASS; i < (int)ProcessParameter::TXT_MAX;i++){
        EXPECT_EQ(strcmp(p.Get(ProcessParameter::TXT_(i)), np.Get(ProcessParameter::TXT_(i))),0);
    }
}
TEST(Basic, Nesting){
    Good    h0;
    EXPECT_EQ(h0.min_,0);
    EXPECT_EQ(h0.max_,100);
    // ----
    Good    h1(123,234);
    EXPECT_EQ(h1.min_,123);
    EXPECT_EQ(h1.max_,234);
    // ---- good
    NoGood  n0(0,100);
    EXPECT_EQ(n0.min_,0);
    EXPECT_EQ(n0.max_,100);
    // ----
    NoGood  n1;
    EXPECT_NE(n1.min_,0);
    EXPECT_NE(n1.max_,100);
}
