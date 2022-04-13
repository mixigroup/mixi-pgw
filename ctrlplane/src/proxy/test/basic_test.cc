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

#define TESTCFG  ("/tmp/proxy_test.cfg")

// instance
pthread_mutex_t __mysql_mutex;
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
int PGW_RECOVERY_COUNT = 0;
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
//
TEST(ProxyTest, Start){
    openlog("test", LOG_PERROR|LOG_PID,LOG_LOCAL2);
    PGW_LOG(PGW_LOG_LEVEL_INF, " >> start %s(%p)\n", "BasicTest", (void*)pthread_self());
}
//
TEST(ProxyTest, ConfigMake){
    auto cfg = TESTCFG;
    auto fp = fopen(cfg, "w+");
    EXPECT_EQ(fp!=NULL,true);
    if (fp){
        auto p = "\n"\
                 "HOST=localhost\n"\
                 "PORT=1234\n"\
                 "USER=user\n"\
                 "PSWD=pswd\n"\
                 "INST=inst\n"\
                 "HOST_N=host_n\n"\
                 "USER_N=user_n\n"\
                 "PSWD_N=pswd_n\n"\
                 "PORT_N=3214\n"\
                 "INST_N=inst_n\n";
        auto r = fwrite(p, strlen(p),1, fp);
        EXPECT_EQ(r>0,true);
        fclose(fp);
    }
}
TEST(ProxyTest, ConfigRead){
    auto cfg = new Config(TESTCFG);
    EXPECT_EQ(cfg!=NULL,true);
    if (cfg){
        auto nval = cfg->GetInt("PORT");
        EXPECT_EQ(nval, 1234);
        nval = cfg->GetInt("PORT_N");
        EXPECT_EQ(nval, 3214);

        auto sval = cfg->GetText("HOST");
        EXPECT_EQ(strcmp(sval, "localhost"), int(0));
        sval = cfg->GetText("USER");
        EXPECT_EQ(strcmp(sval, "user"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "user"));

        sval = cfg->GetText("PSWD");
        EXPECT_EQ(strcmp(sval, "pswd"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "pswd"));

        sval = cfg->GetText("INST");
        EXPECT_EQ(strcmp(sval, "inst"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "inst"));

        sval = cfg->GetText("HOST_N");
        EXPECT_EQ(strcmp(sval, "host_n"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "host_n"));

        sval = cfg->GetText("USER_N");
        EXPECT_EQ(strcmp(sval, "user_n"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "user_n"));

        sval = cfg->GetText("PSWD_N");
        EXPECT_EQ(strcmp(sval, "pswd_n"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "pswd_n"));

        sval = cfg->GetText("INST_N");
        EXPECT_EQ(strcmp(sval, "inst_n"), int(0));
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s , %d\n", sval , strcmp(sval, "inst_n"));
        //
        delete cfg;
    }
}
TEST(ProxyTest, DatabaseConfigRead){
    auto cfg = new DatabaseConfig(TESTCFG, "");
    EXPECT_EQ(cfg!=NULL,true);
    if (cfg){
        EXPECT_EQ(strcmp(cfg->Host(), "localhost"), int(0));
        EXPECT_EQ(strcmp(cfg->User(), "user"), int(0));
        EXPECT_EQ(strcmp(cfg->Pswd(), "pswd"), int(0));
        EXPECT_EQ(strcmp(cfg->Inst(), "inst"), int(0));
        EXPECT_EQ(cfg->Port(), int(1234));
        //
        delete cfg;
    }
    cfg = new DatabaseConfig(TESTCFG, "_N");
    EXPECT_EQ(cfg!=NULL,true);
    if (cfg){
        EXPECT_EQ(strcmp(cfg->Host(), "host_n"), int(0));
        EXPECT_EQ(strcmp(cfg->User(), "user_n"), int(0));
        EXPECT_EQ(strcmp(cfg->Pswd(), "pswd_n"), int(0));
        EXPECT_EQ(strcmp(cfg->Inst(), "inst_n"), int(0));
        EXPECT_EQ(cfg->Port(), int(3214));
        //
        delete cfg;
    }
}
TEST(ProxyTest, ConfigReMake){
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
                    "EXT_IP=127.0.0.1\n"\
                    "EXT_PORT=12123\n"\
                    "INT_IF=lo\n"\
                    "INT_IP=127.0.0.1\n"\
                    "INT_PORT=32123\n"\
                    "STS_IF=lo\n"\
                    "STS_IP=127.0.0.1\n"\
                    "STS_PORT=10161";
        auto r = fwrite(p, strlen(p),1, fp);
        EXPECT_EQ(r>0, true);
        fclose(fp);
    }
}


TEST(ProxyTest, DatabaseCon){
    auto cfg = new DatabaseConfig(TESTCFG, "_MM");
    EXPECT_EQ(cfg!=NULL,true);
    if (cfg){
        PGW_LOG(PGW_LOG_LEVEL_INF, "%s/%s/%s/%s/%d\n",
            cfg->Host(),
            cfg->User(),
            cfg->Pswd(),
            cfg->Inst(),
            cfg->Port());
        DatabaseConnection  cn(cfg);

        EXPECT_EQ(cn.Execute("DROP DATABASE IF EXISTS testproxy"), OK);
        EXPECT_EQ(cn.Execute("CREATE DATABASE testproxy"), OK);
        EXPECT_EQ(cn.Execute("USE testproxy"), OK);
        EXPECT_EQ(cn.Execute("CREATE TABLE hoge(n int)"), OK);
        EXPECT_EQ(cn.Execute("INSERT INTO hoge VALUES(1),(2),(3),(4),(5),(6)"), OK);

        static int hoge = 0;

        cn.Select("SELECT * FROM hoge ORDER BY `hoge`", [](MYSQL_ROW row, U32 n, void* p){
                EXPECT_EQ(hoge, atoi(row[0]));
                hoge ++;
                return(OK);
            }, NULL);
        delete cfg;
    }
}

TEST(ProxyTest, RouteInit){
    auto cfg = new DatabaseConfig(TESTCFG, "_MM");
    auto cfgn = new DatabaseConfig(TESTCFG, "_MM");
    DatabaseConnection  cn(cfg);
    GeneralConfig   gencfg(TESTCFG);

    EXPECT_EQ(cn.Execute("TRUNCATE TABLE proxy"), OK);
    EXPECT_EQ(cn.Execute("INSERT INTO proxy(`pgw_group_id`,`dst_ip`)VALUES"\
                         "(0,'echo0'),(0,'echo1'),"\
                         "(1,'dev0'),(1,'dev1'),"\
                         "(2,'qa0'),(2,'qa1'),"\
                         "(3,'cm0'),(3,'cm1'),"\
                         "(4,'user-d0'),(4,'user-d1'),"\
                         "(5,'user-s0'),(5,'user-s1'),"\
                         "(6,'user-a0'),(6,'user-a1')"\
                         ""), OK);
    Routes  rt(cfg, cfgn, &gencfg);
    ROUTEINTS  routes;
    // initialize routing
    rt.Init();
    //
    EXPECT_EQ(rt.FindInternlRouteByGroupFromCache(GROUP(0), routes), OK);
    EXPECT_EQ(routes.size(), 2);
    if (routes.size() == 2){
        EXPECT_EQ(strcmp(routes[0].Host(), "echo0"), 0);
        EXPECT_EQ(strcmp(routes[1].Host(), "echo1"), 0);
    }
    //
    routes.clear();
    EXPECT_EQ(rt.FindInternlRouteByGroupFromCache(GROUP(1), routes), OK);
    EXPECT_EQ(routes.size(), 2);
    if (routes.size() == 2){
        EXPECT_EQ(strcmp(routes[0].Host(), "dev0"), 0);
        EXPECT_EQ(strcmp(routes[1].Host(), "dev1"), 0);
    }

    //
    routes.clear();
    EXPECT_EQ(rt.FindInternlRouteByGroupFromCache(GROUP(6), routes), OK);
    EXPECT_EQ(routes.size(), 2);
    if (routes.size() == 2){
        EXPECT_EQ(strcmp(routes[0].Host(), "user-a0"), 0);
        EXPECT_EQ(strcmp(routes[1].Host(), "user-a1"), 0);
    }
}


TEST(ProxyTest, End){
    closelog();
}

