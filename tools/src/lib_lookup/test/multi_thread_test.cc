//
// Created by mixi on 2017/04/24.
//


#include "gtest/gtest.h"

#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/const/link.h"
#include "lib/misc.hpp"
#include "lib/logger.hpp"
#include "mixipgw_tools_def.h"
#include <boost/thread/thread.hpp>

using namespace MIXIPGW_TOOLS;

static inline unsigned long long msec(void){
    struct timeval	tv;
    gettimeofday(&tv,NULL);
    return((((uint64_t)tv.tv_sec * 1000000) + ((uint64_t)tv.tv_usec)));
}



static inline const char *Norm2(char *buf, double val, const char *fmt){
    const char *units[] = { "", "K", "M", "G", "T" };
    u_int i;
    for (i = 0; val >=1000 && i < sizeof(units)/sizeof(char *) - 1; i++)
        val /= 1000;
    sprintf(buf, fmt, val, units[i]);
    return(buf);
}
static inline const char *Norm(char *buf, double val){
    return Norm2(buf, val, "%.3f %s");
}


DualBufferedLookupTable<link_t> *s_tbl_ = NULL;
boost::mutex                    s_mutex_;
boost::thread_group             s_threads_;

TEST(MultiThread, Init){
    Misc::SetAffinity(15);

    s_tbl_ = DualBufferedLookupTable<link_t>::Create();
    EXPECT_NE((void*)s_tbl_, (void*)NULL);
}
//
TEST(MultiThread, Write){
    uint64_t counter = 0;
    srand(time(NULL));
    uint32_t    randidx[65536] = {0,};
    for(uint64_t n = 0;n<65536;n++){
        randidx[n] = rand()%65536;
    }
    //
    auto st = msec();
    s_threads_.create_thread([&]{
        Misc::SetAffinity(22);

        for(int c = 0;c < 1000;c++){
            for(uint64_t n = 0;n<65536;n++){
                link_t itm;
                bzero(&itm, sizeof(itm));
                if (c == 0){
                    itm.sgw_teid_u = n+1;
                    itm.sgw_teid_c = n+2;
                    itm.sgw_ipv4 = n+3;
                    itm.pgw_ipv4 = n+4;
                    EXPECT_EQ(s_tbl_->Add((__be32)(200+n),&itm, 0), RETOK);
                }else{
                    itm.sgw_teid_u = randidx[n]+1;
                    itm.sgw_teid_c = randidx[n]+2;
                    itm.sgw_ipv4 = randidx[n]+3;
                    itm.pgw_ipv4 = randidx[n]+4;
                    EXPECT_EQ(s_tbl_->Add((__be32)(200+randidx[n]),&itm, 0), RETOK);
                }
                //
                counter++;
            }
        }
    });
    s_threads_.join_all();
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "### write ###\n %s pps\n#############\n", Norm(bf, (double)counter*1000000.0e0/(double)(et - st)));
    fprintf(stderr, "### write ###\n %s count\n%u us\n#############\n", Norm(bf, (double)counter), (et - st));
}
//
TEST(MultiThread, Read){
    uint64_t counter = 0,err_counter = 0;
    srand(time(NULL));
    uint32_t    randidx[65536] = {0,};
    for(uint64_t n = 0;n<65536;n++){
        randidx[n] = rand()%65536;
    }

    Misc::SetAffinity(15);
    //
    auto st = msec();
    // at main thread
    for(int c = 0;c < 100;c++){
        for(uint64_t n = 0;n<65536;n++){
            auto it = s_tbl_->Find(200+randidx[n],0);
            EXPECT_EQ(it==s_tbl_->End(), false);
            if (it != s_tbl_->End()){
                EXPECT_EQ(it->sgw_teid_u, (randidx[n]+1));
                EXPECT_EQ(it->sgw_teid_c, (randidx[n]+2));
                EXPECT_EQ(it->sgw_ipv4, (randidx[n]+3));
                EXPECT_EQ(it->pgw_ipv4, (randidx[n]+4));
                counter++;
            }else{
                fprintf(stderr, "not found?(%u)\n", 200+randidx[n]);
                err_counter++;
            }
        }
    }
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "###  read ###\n %s pps\n#############\n", Norm(bf, (double)counter*1000000.0e0/(double)(et - st)));
    fprintf(stderr, "###  err  ###\n %s pps\n#############\n", Norm(bf, (double)err_counter*1000000.0e0/(double)(et - st)));
}

TEST(MultiThread, ReadWrite){
    uint64_t counter = 0,rcounter = 0;
    unsigned long long r_st = 0,r_et = 0;
    int done = 0;
    //
    s_threads_.create_thread([&]{
        srand(time(NULL));
        uint32_t    randidx[65536] = {0,};
        for(uint64_t n = 0;n<65536;n++){
            randidx[n] = rand()%65536;
        }
        
        Misc::SetAffinity(22);
        for(int c = 0;c < 10;c++){
            for(uint64_t n = 0;n<65536;n+=(c>0?17:1)){
                link_t itm;
                bzero(&itm, sizeof(itm));
                itm.sgw_teid_u = randidx[n]+2;
                itm.sgw_teid_c = randidx[n]+3;
                itm.sgw_ipv4 = randidx[n]+4;
                itm.pgw_ipv4 = randidx[n]+5;
                //
                EXPECT_EQ(s_tbl_->Add((__be32)(200+randidx[n]),&itm, 0), RETOK);
                counter++;
                if (c > 0){ usleep(10); }
            }
            done = 1;
        }
        done=2;
    });

    s_threads_.create_thread([&]{

        Misc::SetAffinity(15);
        while(1){
            if (done == 0){ usleep(100); continue; }
            break;
        }
        srand(time(NULL));
        uint32_t    randidx[65536] = {0,};
        for(uint64_t n = 0;n<65536;n++){
            randidx[n] = rand()%65536;
        }

        r_st = msec();
        // at main thread
        for(int c = 0;c < 100;c++){
            for(uint64_t n = 0;n<65536;n++){
                auto it = s_tbl_->Find(200+randidx[n],0);
                EXPECT_EQ(it == s_tbl_->End(), false);
                if (it != s_tbl_->End()){
                    EXPECT_EQ(it->sgw_teid_u, (randidx[n]+2));
                    EXPECT_EQ(it->sgw_teid_c, (randidx[n]+3));
                    EXPECT_EQ(it->sgw_ipv4,   (randidx[n]+4));
                    EXPECT_EQ(it->pgw_ipv4,   (randidx[n]+5));
                    rcounter++;
                }
                if (done == 2){ break; }
            }
        }
        r_et = msec();
    });

    s_threads_.join_all();
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "### read/write ###\n %s pps\n#############\n", Norm(bf, (double)rcounter*1000000.0e0/(double)(r_et - r_st)));
}


TEST(MultiThread, Finish){
    if (s_tbl_ != NULL){ delete s_tbl_; }
    s_tbl_ = NULL;
}
