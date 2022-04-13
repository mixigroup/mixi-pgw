//
// Created by mixi on 2017/04/24.
//


#include "gtest/gtest.h"

#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/logger.hpp"
#include "mixipgw_tools_def.h"
#include "lib/const/policer.h"
#include <boost/thread/thread.hpp>

using namespace MIXIPGW_TOOLS;

//
TEST(DualBufferedLookupTable, Instanciate){
    for(int n = 0;n < 64;n++){
        DualBufferedLookupTable<policer_t> *tbl = DualBufferedLookupTable<policer_t>::Create();
        EXPECT_NE((void*)tbl, (void*)NULL);

        if (tbl != NULL){
            delete tbl;
        }
    }
}
TEST(DualBufferedLookupTable, Singleton){
    DualBufferedLookupTable<policer_t> *pt = NULL;
    for(int n = 0;n < 64;n++){
        DualBufferedLookupTable<policer_t> *tbl = DualBufferedLookupTable<policer_t>::Init();
        EXPECT_NE((void*)tbl, (void*)NULL);
        if (pt != NULL){
            EXPECT_EQ((void*)pt, (void*)tbl);
        }
        pt = tbl;
    }
}


TEST(DualBufferedLookupTable, AddFind){
    DualBufferedLookupTable<policer_t> *tbl = DualBufferedLookupTable<policer_t>::Init();
    EXPECT_NE((void*)tbl, (void*)NULL);
    fprintf(stdout, ".. %p\n", tbl);
#define TEST_RANGE  1000
    if (tbl != NULL){
        policer_t itm;
        // add
        for(int n = 0;n < TEST_RANGE;n++){
            itm.commit_rate = n+1;
            itm.commit_burst_size = n+2;
            itm.commit_information_rate = n+3;
            itm.excess_rate = n+4;
            itm.excess_burst_size = n+5;
            itm.excess_information_rate = n+6;
            //
            EXPECT_EQ(tbl->Add(n+100, &itm, 0), RETOK);
        }

        // find
        for(int n = 0;n < TEST_RANGE;n++){
            auto it = tbl->Find(n+100,0);
            EXPECT_EQ(it==tbl->End(), false);
            if (it != tbl->End()){
                EXPECT_EQ(it->commit_rate, (n+1));
                EXPECT_EQ(it->commit_burst_size, (n+2));
                EXPECT_EQ(it->commit_information_rate, (n+3));
                EXPECT_EQ(it->excess_rate, (n+4));
                EXPECT_EQ(it->excess_burst_size, (n+5));
                EXPECT_EQ(it->excess_information_rate, (n+6));
            }
        }

        delete tbl;
    }
}

