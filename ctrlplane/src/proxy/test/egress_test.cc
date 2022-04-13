
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
TEST(Egress, Start){
    openlog("test", LOG_PERROR|LOG_PID,LOG_LOCAL2);
    PGW_LOG(PGW_LOG_LEVEL_INF, " >> start %s(%p)\n", "Egress", (void*)pthread_self());
}
//
TEST(Egress, ConfigMake){
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
                    "INT_PORT=2123";
        auto r = fwrite(p, strlen(p),1, fp);
        EXPECT_EQ(r>0, true);
        fclose(fp);
    }
}

TEST(Egress, RouteInit){
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

// ====================

// 1st , send CreateSessionRequest to 127.0.0.2 , register sequence number and TEID into MAP
// 2nd , send corresponding CreateSessionResponse to 127.0.0.3
TEST(Egress, SGWThread){
    pthread_t t;
    pthread_create(&t, NULL, [](void* arg){
        _wait_status(STATUS_INLOOP); // wait for.
        #include "pkts/create_session.c"
        PGW_LOG(PGW_LOG_LEVEL_INF, "... %04d(%p)\n", 1, (void*)pthread_self());
        // =====
        // CreateSessionRequest
        struct sockaddr_in  caddr;
        memset(&caddr, 0, sizeof(caddr));
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(GTPC_PORT);
        inet_pton(AF_INET, "127.0.0.2", &caddr.sin_addr.s_addr);

        // PGW
        auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(GTPC_PORT);
        inet_pton(AF_INET, "127.0.0.7", &addr.sin_addr.s_addr);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));

        auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), real_req, sizeof(real_req));
        usleep(100000);
        EXPECT_EQ(ret, 0);
        // after that, evaluate returned CreateSessionResponse
        // sent to SGW(127.0.0.7) in correctly
        while(__status != STATUS_FIN){
            char buf[2048] = {0};
            auto ret = recv(sock, buf, sizeof(buf), 0);
            EXPECT_EQ(ret, sizeof(real_res));
            usleep(100000);
        }
        close(sock);
        return((void*)NULL);
    }, NULL);
}

TEST(Egress, PGWThread){
    pthread_t t;
    pthread_create(&t, NULL, [](void* arg){
        _wait_status(STATUS_INLOOP); // wait
        #include "pkts/create_session.c"
        PGW_LOG(PGW_LOG_LEVEL_INF, "... %04d(%p)\n", 1, (void*)pthread_self());
        // =====
        // CreateSessionResponse
        struct sockaddr_in  caddr;
        memset(&caddr, 0, sizeof(caddr));
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(GTPC_PORT);
        inet_pton(AF_INET, "127.0.0.3", &caddr.sin_addr.s_addr);
        auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), real_res, sizeof(real_res));
        usleep(100000);
        close(sock);
        EXPECT_EQ(ret, 0);
        // =====
        usleep(100000); // TODO CHECK
        __status = STATUS_FIN;
        return((void*)NULL);
    }, NULL);
}


// =====================

TEST(Egress, MainLoop){
    U64 counter = 0;
    auto event_base_ext = event_base_new();
    auto event_base_int = event_base_new();
    auto recv_event_ext = event_new(event_base_ext, __proute->ExtSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveExt, __proute);
    auto recv_event_int = event_new(event_base_int, __proute->IntSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveInt, __proute);
    auto timeout_event_ext = event_new(event_base_ext, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, __proute);
    auto timeout_event_int = event_new(event_base_int, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, __proute);
    //
    struct timeval tm;
    tm.tv_sec = (TIMEOUTMS / 1000);
    tm.tv_usec = (TIMEOUTMS % 1000) * 1000;
    if (event_add(recv_event_ext, NULL) || event_add(timeout_event_ext, &tm) ||
        event_add(recv_event_int, NULL) || event_add(timeout_event_int, &tm)) {
        pgw_panic("event_add failed(%d: %s)\n", errno, strerror(errno));
    }
    while(!Proxy::Quit()){
        event_base_loop(event_base_ext, EVLOOP_ONCE);
        event_base_loop(event_base_int, EVLOOP_ONCE);
        if (__status == STATUS_DC){
            __status = STATUS_INLOOP;
        }else if (__status == STATUS_FIN){
            break;
        }
        usleep(100000);
    }
}

TEST(Egress, End){
    if (__proute){
        delete __proute;
    }
    __proute = NULL;
    closelog();
}

