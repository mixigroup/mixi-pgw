//
// Created by mixi on 2017/04/27.
//
#include "gtest/gtest.h"

#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"
#include "lib/const/link.h"

using namespace MIXIPGW_TOOLS;
DualBufferedLookupTable<link_t> *tbl = NULL;

TEST(Basic, Test00){
    tbl = DualBufferedLookupTable<link_t>::Create();
    EXPECT_NE((void*)tbl, (void*)NULL);

    union _teidcnv cnv;
    cnv.pgw_teid.gcnt = 12345;
    cnv.pgw_teid.gid  = 0;
    // notify bit on
    tbl->NotifyChange(cnv.pgw_teid_uc);

    uint64_t    bmp64 = 0;
    // check notify bit
    for(int n=0;n < (MAX_STRAGE_PER_GROUP>>6);n++){
        if ((bmp64 = tbl->FindNoticeBmp64((__be16)n<<6)) != 0){
            fprintf(stderr, "bmp64 = (%llu)\n", bmp64);
            // notify detected -> clear status 64bit boundary region.
            tbl->ClearNoticeBmp64(n<<6);
            //
            for(int m = 0;m < 64;m++){
                if (bmp64&(((uint64_t)1)<<m)){
                    // teid with notify
                    union _teidcnv cnv;
                    cnv.pgw_teid.gcnt = (__be32)((n<<6)+m);
                    cnv.pgw_teid.gid  = 0;
                    //
                    EXPECT_EQ((__be32)cnv.pgw_teid.gcnt,(__be32)12345);
                }
            }
            break;
        }
    }
    EXPECT_NE(bmp64,0);
    // validate, notification bits are cleared.
    bmp64 = 0;
    for(int n=0;n < (MAX_STRAGE_PER_GROUP>>6);n++){
        if ((bmp64 = tbl->FindNoticeBmp64((__be16)n<<6)) != 0){
            FAIL() << "why exist??";
        }
    }
    EXPECT_EQ(bmp64,0);
}
//
