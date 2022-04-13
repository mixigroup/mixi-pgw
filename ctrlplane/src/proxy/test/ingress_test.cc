#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "gtest/gtest.h"

#include <typeinfo>
#include <string>
#include <map>
#include "mysql.h"


#include "config.hpp"
#include "database_config.hpp"
#include "general_config.hpp"
#include "database_connection.hpp"
#include "routes.hpp"
#include "proxy.hpp"

#define TESTCFG  ("/tmp/logic_test.cfg")

// instance
pthread_mutex_t __mysql_mutex;
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
int PGW_RECOVERY_COUNT = 0;
static Routes* __proute = NULL;

enum STATUS{
    STATUS_DC = 0,
    STATUS_INLOOP,
    STATUS_FIN,
    STATUS_MAX
};

static int __status = STATUS_DC;

//
class CustomEnvironment :public ::testing::Environment {
public:
    virtual ~CustomEnvironment() {}
    virtual void SetUp() { }
    virtual void TearDown() { }
};


int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
/*
ifconfig lo0 alias 127.0.0.2
ifconfig lo0 alias 127.0.0.3
ifconfig lo0 alias 127.0.0.4
ifconfig lo0 alias 127.0.0.5
ifconfig lo0 alias 127.0.0.6
 */
//
TEST(Ingress, Start){
    openlog("test", LOG_PERROR|LOG_PID,LOG_LOCAL2);
    PGW_LOG(PGW_LOG_LEVEL_INF, " >> start %s(%p)\n", "Ingress", (void*)pthread_self());
}
//
TEST(Ingress, ConfigMake){
    auto cfg = TESTCFG;
    auto fp = fopen(cfg, "w+");
    EXPECT_EQ(fp!=NULL,true);
    if (fp){
        auto p = "\n"\
                    "HOST_MM=localhost\n"\
                    "PORT_MM=3306\n"\
                    "USER_MM=mixipgw\n"\
                    "PSWD_MM=password\n"\
                    "INST_MM=mixipgw\n"\
                    "EXT_IF=lo\n"\
                    "EXT_IP=127.0.0.2\n"\
                    "EXT_PORT=2123\n"\
                    "INT_IF=lo\n"\
                    "INT_IP=127.0.0.3\n"\
                    "INT_PORT=2123\n"\
                    "STS_IF=lo\n"\
                    "STS_IP=127.0.0.3\n"\
                    "STS_PORT=10161";
        auto r = fwrite(p, strlen(p),1, fp);
        EXPECT_EQ(r>0, true);
        fclose(fp);
    }
}

TEST(Ingress, RouteInit){
    auto cfg = new DatabaseConfig(TESTCFG, "_MM");
    auto cfgn = new DatabaseConfig(TESTCFG, "_MM");
    DatabaseConnection  cn(cfg);
    GeneralConfig   gencfg(TESTCFG);

    EXPECT_EQ(cn.Execute("TRUNCATE TABLE proxy"), OK);
    EXPECT_EQ(cn.Execute("INSERT INTO proxy(`pgw_group_id`,`dst_ip`)VALUES"\
                         "(100,'127.0.0.4'),"\
                         "(101,'127.0.0.5'),"\
                         "(102,'127.0.0.6')"\
                         ""), OK);

    EXPECT_EQ(cn.Execute("TRUNCATE TABLE subscribers"), OK);
    EXPECT_EQ(cn.Execute("INSERT INTO subscribers (msisdn,initial_ip,active_ip,imsi,pgw_group_id)"\
                            "VALUES"\
                            "(1,1,1,240679077777777,100),"\
                            "(2,2,2,240679087777777,101),"\
                            "(3,3,3,240679097777777,102)"), OK);
    __proute = new Routes(cfg, cfgn, &gencfg);
    EXPECT_EQ(__proute!=NULL,true);
    // initialize routing
    __proute->Init();
}
static void _wait_status(int stat){
    while(true){
        if (__status == stat){
            break;
        }
        usleep(100000);
    }
}
static int _check_stats(void){
    struct sockaddr_in  caddr;
    U8 rbuf[PCKTLEN_UDP];
    const U32 seqno = 123456;

    auto sh = (proxy_stats_header_ptr)rbuf;
    auto sr = (proxy_stats_request_data_ptr)&rbuf[sizeof(*sh)];
    //
    bzero(&caddr, sizeof(caddr));
    bzero(rbuf, sizeof(rbuf));
    //
    sh->magic = PSTAT_MAGIC;
    sh->type = PSTAT_REQUEST;
    sh->callbackdata = seqno;
    sh->extlength = 0;
    sh->length = htons(sizeof(*sr));
    sr->param.type = 0;
    sr->param.groupid = (U32)-1;
    sr->param.ip = (U32)-1;
    //
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(10161);
    inet_pton(AF_INET, "127.0.0.3", &caddr.sin_addr.s_addr);
    auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), sh, sizeof(*sh)+sizeof(*sr));
    EXPECT_EQ(ret, 0);
    usleep(100000);
    ret = recvfrom(sock, rbuf, sizeof(*sh) + sizeof(proxy_stats_result_counter_t), 0, NULL, NULL);
    //
    close(sock);
    auto st = (proxy_stats_result_counter_ptr)&rbuf[sizeof(*sh) + (sh->extlength << 2)];
    EXPECT_EQ(ret, sizeof(*sh)+sizeof(*st));
    EXPECT_EQ(sh->magic, PSTAT_MAGIC);
    EXPECT_EQ(sh->callbackdata, seqno);
    EXPECT_EQ((sh->type&0xF0), PSTAT_RESULT);
    for(auto n = 0;n < (PSTAT_COUNT_SESSION+1);n++){
        PGW_LOG(PGW_LOG_LEVEL_INF, "count[%u] => %llu", n, ntohll(st->val[n]));
    }
    U64 req_bytes = 0;
    U64 res_bytes = 0;
    {
        #include "pkts/create_session.c"
        req_bytes += (sizeof(tpkt)*9);
    }
    {
        #include "pkts/echo_req.c"
        req_bytes += sizeof(tpkt);
        res_bytes += sizeof(tpkt);
    }
    EXPECT_EQ(ntohll(st->val[PSTAT_BYTES_REQUEST]), req_bytes);
    EXPECT_EQ(ntohll(st->val[PSTAT_BYTES_RESPONSE]), res_bytes);
    EXPECT_EQ(ntohll(st->val[PSTAT_COUNT_REQUEST]), 10);
    EXPECT_EQ(ntohll(st->val[PSTAT_COUNT_RESPONSE]), 1);
    EXPECT_EQ(ntohll(st->val[PSTAT_COUNT_SESSION]), 2);

    // testing...
    sleep(2);
    __proute->StatExpire(1);
    //
    sh->magic = PSTAT_MAGIC;
    sh->type = PSTAT_REQUEST;
    sh->callbackdata = seqno;
    sh->extlength = 0;
    sh->length = htons(sizeof(*sr));
    sr->param.type = 0;
    sr->param.groupid = (U32)-1;
    sr->param.ip = (U32)-1;
    //
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(10161);
    inet_pton(AF_INET, "127.0.0.3", &caddr.sin_addr.s_addr);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), sh, sizeof(*sh)+sizeof(*sr));
    EXPECT_EQ(ret, 0);
    usleep(100000);
    ret = recvfrom(sock, rbuf, sizeof(*sh) + sizeof(proxy_stats_result_counter_t), 0, NULL, NULL);
    //
    close(sock);
    st = (proxy_stats_result_counter_ptr)&rbuf[sizeof(*sh) + (sh->extlength << 2)];
    // all sessions must be deleted due to time out.
    EXPECT_EQ(ntohll(st->val[PSTAT_COUNT_SESSION]), 0);
    //
    return(0);
}
TEST(Ingress, StartSendThread){
    pthread_t t;
    pthread_create(&t, NULL, [](void* arg){
        _wait_status(STATUS_INLOOP);
        //
        for(auto n = 0;n < 9;n++){
            #include "pkts/create_session.c"
            PGW_LOG(PGW_LOG_LEVEL_INF, "... %04d(%p)\n", n, (void*)pthread_self());
            // should transfer 3 packets to PGW nodes
            switch((n%3)){
                case 0: tpkt[19] = 0x70; break;
                case 1: tpkt[19] = 0x80; break;
                case 2: tpkt[19] = 0x90; break;
            }
            struct sockaddr_in  caddr;
            memset(&caddr, 0, sizeof(caddr));
            caddr.sin_family = AF_INET;
            caddr.sin_port = htons(GTPC_PORT);
            inet_pton(AF_INET, "127.0.0.2", &caddr.sin_addr.s_addr);

            auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
            usleep(100000);
            close(sock);
            EXPECT_EQ(ret, 0);
        }
        // evaluate outer-Proxy ECHO packet
        #include "pkts/echo_req.c"
        struct sockaddr_in  caddr;
        memset(&caddr, 0, sizeof(caddr));
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(GTPC_PORT);
        inet_pton(AF_INET, "127.0.0.2", &caddr.sin_addr.s_addr);
        auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
        char rbuf[2048] = {0};
        usleep(100000);
        ret = recvfrom(sock, rbuf, sizeof(tpkt), 0, NULL, NULL);
        close(sock);
        EXPECT_EQ(ret, sizeof(tpkt));
        EXPECT_EQ(rbuf[1], 0x02);
        EXPECT_EQ(rbuf[sizeof(tpkt)-1], 0x00);

        // evaluate internal-Proxy ECHO packet
        memset(&caddr, 0, sizeof(caddr));
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(GTPC_PORT);
        inet_pton(AF_INET, "127.0.0.3", &caddr.sin_addr.s_addr);
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
        usleep(100000);
        ret = recvfrom(sock, rbuf, sizeof(tpkt), 0, NULL, NULL);
        close(sock);
        EXPECT_EQ(ret, sizeof(tpkt));
        EXPECT_EQ(rbuf[1], 0x02);
        EXPECT_EQ(rbuf[sizeof(tpkt)-1], 0x00);

        // check counting status.
        EXPECT_EQ(_check_stats(), 0);

        __status = STATUS_FIN;
        return((void*)NULL);
    }, NULL);
}

TEST(Ingress, StartRecvThread){
    pthread_t t;
    pthread_create(&t, NULL, [](void* arg){
        #include "pkts/create_session.c"
        int sock[3] = {0};
        const char* addrs[3] = {"127.0.0.4","127.0.0.5","127.0.0.6"};
        //
        for(auto n = 0;n < 3;n++){
            sock[n] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            EXPECT_EQ(sock[n]>=0, true);
        }
        for(auto n = 0;n < 3;n++){
            struct sockaddr_in addr;
            bzero(&addr, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(GTPC_PORT);
            inet_pton(AF_INET, (char*)addrs[n], &addr.sin_addr.s_addr);
            EXPECT_EQ(bind(sock[n], (struct sockaddr *)&addr, sizeof(addr)), 0);
        }
        _wait_status(STATUS_INLOOP);
        //
        while(__status != STATUS_FIN){
            for(auto n = 0;n < 3;n++){
                char buf[2048] = {0};
                auto ret = recv(sock[n], buf, sizeof(buf), 0);
                EXPECT_EQ(ret, sizeof(tpkt));
                switch((n%3)){
                case 0: EXPECT_EQ((U8)buf[19],(U8)0x70); break;
                case 1: EXPECT_EQ((U8)buf[19],(U8)0x80); break;
                case 2: EXPECT_EQ((U8)buf[19],(U8)0x90); break;
                }
                usleep(100000);
            }
        }
        //
        for(auto n = 0;n < 3;n++){ close(sock[n]); }

        return((void*)NULL);
    }, NULL);
}


TEST(Ingress, MainLoop){
    U64 counter = 0;
    auto event_base_ext = event_base_new();
    auto event_base_int = event_base_new();
    auto event_base_sts = event_base_new();
    auto recv_event_ext = event_new(event_base_ext, __proute->ExtSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveExt, __proute);
    auto recv_event_int = event_new(event_base_int, __proute->IntSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveInt, __proute);
    auto recv_event_sts = event_new(event_base_sts, __proute->StatsSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveStats, __proute);
    auto timeout_event_ext = event_new(event_base_ext, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, __proute);
    auto timeout_event_int = event_new(event_base_int, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, __proute);
    auto timeout_event_sts = event_new(event_base_sts, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, __proute);
    //
    struct timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = (TIMEOUTMS % 1000) * 1000;
    if (event_add(recv_event_ext, NULL) || event_add(timeout_event_ext, &tm) ||
        event_add(recv_event_int, NULL) || event_add(timeout_event_int, &tm) ||
        event_add(recv_event_sts, NULL) || event_add(timeout_event_sts, &tm)) {
        pgw_panic("event_add failed(%d: %s)\n", errno, strerror(errno));
    }
    while(!Proxy::Quit()){
        event_base_loop(event_base_ext, EVLOOP_ONCE);
        event_base_loop(event_base_int, EVLOOP_ONCE);
        event_base_loop(event_base_sts, EVLOOP_ONCE);
        if (__status == STATUS_DC){
            __status = STATUS_INLOOP;
        }else if (__status == STATUS_FIN){
            break;
        }
        usleep(100000);
    }
}

TEST(Ingress, End){
    if (__proute){
        delete __proute;
    }
    __proute = NULL;
    closelog();
}

