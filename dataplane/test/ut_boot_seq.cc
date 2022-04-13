#include "ut_counter_inc.hpp"

using namespace XDPDK;
static bool __halt_ = false;

int XDPDK::PGW_LOG_LEVEL = (RTE_LOG_DEBUG+1);
uint32_t    XDPDK::FLUSH_COUNTER = _FLUSH_WORKER_;
uint32_t    XDPDK::FLUSH_ENCAP   = _FLUSH_TX_;
uint32_t    XDPDK::FLUSH_GRETERM = _FLUSH_TX_;
uint32_t    XDPDK::FLUSH_PGW_IE  = _FLUSH_TX_;
uint32_t    XDPDK::FLUSH_RX      = _FLUSH_RX_;
uint32_t    XDPDK::FLUSH_RX_DIST = _FLUSH_RX_;
uint32_t    XDPDK::FLUSH_TX      = _FLUSH_TX_;

static XDPDK::CorePgwIngressDistributorWorker* __trgt = NULL;

#include "ut_misc_inline.cc"

TEST(PgwBootSeq, Init){
    std::string uri = "mysql://root:develop@127.0.0.1:3306";
    lookup_t itm;
    bzero(&itm, sizeof(itm));

    EXPECT_EQ(truncate_tunnel(), 0);
    auto trgt = new XDPDK::CorePgwIngressDistributorWorker(0,uri.c_str(),13001,1,0x11111111,0x22222222,0x02,0x03);
    EXPECT_EQ(trgt!=NULL,true);
    if (trgt){
        EXPECT_EQ(trgt->Find(htonl(0x12345678), &itm), -1);
        delete trgt;
    }

    EXPECT_EQ(insert_tunnel(htonl(0x12345678), htonl(0x12345678)), 0);
    trgt = new XDPDK::CorePgwIngressDistributorWorker(0,uri.c_str(),13001,1,0x11111111,0x22222222,0x02,0x03);
    EXPECT_EQ(trgt!=NULL,true);
    if (trgt){
        EXPECT_EQ(trgt->Find(htonl(0x12345678), &itm), 0);
        delete trgt;
    }
    
    EXPECT_EQ(truncate_tunnel(), 0);
    trgt = new XDPDK::CorePgwIngressDistributorWorker(0,uri.c_str(),13001,1,0x11111111,0x22222222,0x02,0x03);
    EXPECT_EQ(trgt!=NULL,true);
    if (trgt){
        EXPECT_EQ(insert_tunnel(htonl(0x12345678), htonl(0x12345678)), 0);
        
        pthread_t th;
        __trgt = trgt;
        pthread_create(&th, NULL, [](void* arg)->void*{
            uint64_t count = 0;
            for(;!__halt_;){
                __trgt->VirtualCycle(NULL, &count);
                usleep(100000);
            }
            return((void*)NULL);
        }, NULL);
        sleep(1);
        __halt_ = true;
        sleep(1);
        //
        EXPECT_EQ(trgt->Find(htonl(0x12345678), &itm), 0);
    }
}

int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
