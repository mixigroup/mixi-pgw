#include "../src/pgw/pgw.h"

#include <typeinfo>
#include <string>
#include <map>
#include <mysql/mysql.h>




#ifndef __USESQLITE3_ON_TEST__

#define CREATE_TBL_KEEPALIVE    ("CREATE TABLE `keepalive` ("\
                                 "  `id` bigint(20) NOT NULL AUTO_INCREMENT,"\
                                 "  `dst_ip` varchar(64) NOT NULL COMMENT '',"\
                                 "  `dst_port` int(11) NOT NULL COMMENT '',"\
                                 "  `src_ip` varchar(64) NOT NULL COMMENT '',"\
                                 "  `src_port` int(11) NOT NULL COMMENT '',"\
                                 "  `proto` int(11) NOT NULL DEFAULT '0' COMMENT '',"\
                                 "  `stat` int(11) NOT NULL DEFAULT '0' COMMENT '',"\
                                 "  `active` int(11) NOT NULL DEFAULT '0' COMMENT '',"\
                                 "  `server_id` VARCHAR(64) NOT NULL DEFAULT 'a00000' COMMENT 'サーバー識別子',"\
                                 "  `server_type` INT NOT NULL DEFAULT 0 COMMENT 'サーバータイプ 0=master, 1=slave',"\
                                 "  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '',"\
                                 "  PRIMARY KEY (`id`),"\
                                 "  KEY `idx_stat_updated_at` (`stat`,`updated_at`)"\
                                 ") ENGINE=MyISAM DEFAULT CHARSET=latin1")

#define CREATE_TBL_TUNNEL       ("CREATE TABLE `tunnel` ("\
                                "  `id` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `imsi` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `msisdn` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `ueipv4` varchar(64) NOT NULL DEFAULT '',"\
                                "  `pgw_teid` bigint(20) NOT NULL DEFAULT '0' COMMENT 'Send Create Session Response:F-TEID',"\
                                "  `pgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Create Session Request Recieved Control plane Ipaddress',"\
                                "  `pgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'P-GW gtpu ipv4',"\
                                "  `dns` varchar(256) NOT NULL DEFAULT '0',"\
                                "  `ebi` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `sgw_gtpc_teid` bigint(20) NOT NULL DEFAULT '0' COMMENT 'Recieved by Create Session Request:F-TEID.',"\
                                "  `sgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Recieved by Create Session Request:F-TEID.',"\
                                "  `sgw_gtpu_teid` bigint(20) NOT NULL DEFAULT '0' COMMENT 'Recieved by Create Session Request:BEARERCTX:F-TEID.',"\
                                "  `sgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Recieved by Create Session Request:BEARERCTX:F-TEID.',"\
                                "  `policy` varchar(64) NOT NULL DEFAULT '',"\
                                "  `active` bigint(20) NOT NULL DEFAULT '0' COMMENT 'active = 0/inactive != 0',"\
                                "  `bitrate_s5` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `bitrate_sgi` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `qci` bigint(20) NOT NULL DEFAULT '0' COMMENT 'Recieved by Create Session Request: Qci.',"\
                                "  `qos` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `teid_mask` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `rat` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `latest_gtp_version` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `restart_counter` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"\
                                "  UNIQUE KEY `pgw_teid` (`pgw_teid`),"\
                                "  UNIQUE KEY `msisdn` (`msisdn`),"\
                                "  UNIQUE KEY `imsi` (`imsi`)"\
                                ") ENGINE=MyISAM DEFAULT CHARSET=latin1")

#define CREATE_TBL_SGW_PEER ("CREATE TABLE `sgw_peer` ("\
                                "  `ip` bigint(20) NOT NULL,"\
                                "  `counter` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `stat` bigint(20) NOT NULL DEFAULT '0',"\
                                "  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'last updated',"\
                                "  UNIQUE KEY `ip` (`ip`)"\
                                ") ENGINE=InnoDB DEFAULT CHARSET=latin1")
#else
// for sqlite3
#define CREATE_TBL_KEEPALIVE    ("DROP TABLE IF EXISTS `keepalive`; "\
                                 "CREATE TABLE `keepalive` ("\
                                 "  `id` INTEGER PRIMARY KEY,"\
                                 "  `dst_ip` TEXT,"\
                                 "  `dst_port` INTEGER,"\
                                 "  `src_ip` TEXT,"\
                                 "  `src_port` INTEGER,"\
                                 "  `proto` INTEGER,"\
                                 "  `stat` INTEGER,"\
                                 "  `active` INTEGER,"\
                                 "  `server_id` TEXT,"\
                                 "  `server_type` INTEGER,"\
                                 "  `updated_at` TIMESTAMP DEFAULT (DATETIME(CURRENT_TIMESTAMP,'localtime'))"\
                                 ")")
// "); CREATE UNIQUE INDEX `idx_stat_updated_at` ON `keepalive`(`stat`,`updated_at`)")

#define CREATE_TBL_TUNNEL       ("DROP TABLE IF EXISTS `tunnel`;"\
                                "CREATE TABLE `tunnel` ("\
                                "  `id` INTEGER,"\
                                "  `imsi` INTEGER,"\
                                "  `msisdn` INTEGER,"\
                                "  `ueipv4` TEXT,"\
                                "  `pgw_teid` INTEGER,"\
                                "  `pgw_gtpc_ipv` TEXT,"\
                                "  `pgw_gtpu_ipv` TEXT,"\
                                "  `dns` TEXT,"\
                                "  `ebi` INTEGER,"\
                                "  `sgw_gtpc_teid` INTEGER,"\
                                "  `sgw_gtpc_ipv` TEXT,"\
                                "  `sgw_gtpu_teid` INTEGER,"\
                                "  `sgw_gtpu_ipv` TEXT,"\
                                "  `policy` TEXT,"\
                                "  `active` INTEGER,"\
                                "  `bitrate_s5` INTEGER,"\
                                "  `bitrate_sgi` INTEGER,"\
                                "  `qci` INTEGER,"\
                                "  `qos` INTEGER,"\
                                "  `teid_mask` INTEGER,"\
                                "  `rat` INTEGER,"\
                                "  `latest_gtp_version` INTEGER,"\
                                "  `restart_counter` INTEGER,"\
                                "  `updated_at` TIMESTAMP DEFAULT (DATETIME(CURRENT_TIMESTAMP,'localtime'))"\
                                "); CREATE UNIQUE INDEX `pgw_teid` ON `tunnel`(`pgw_teid`); "\
                                "   CREATE UNIQUE INDEX `msisdn` ON `tunnel`(`msisdn`);"\
                                "   CREATE UNIQUE INDEX `imsi` ON `tunnel`(`imsi`);")

#define CREATE_TBL_SGW_PEER     ("DROP TABLE IF EXISTS `sgw_peer`;"\
                                "CREATE TABLE `sgw_peer` ("\
                                "  `ip` INTEGER,"\
                                "  `counter` INTEGER,"\
                                "  `stat` INTEGER,"\
                                "  `updated_at` TIMESTAMP DEFAULT (DATETIME(CURRENT_TIMESTAMP,'localtime'))"\
                                "); CREATE UNIQUE INDEX `ip` ON `sgw_peer`(`ip`);"\
                                " ")

#endif /* __USESQLITE3_ON_TEST__ */

int PGW_RECOVERY_COUNT = (GTPC_RECOVERY_1 + 1);




static char TESTDB[64] = {0};

static void setup_database(void){
    int ret;
    U32 port = 3306;
    U32 opt = 0;
    char    sql[1024] = {0};

    DBPROVIDER_APP_INIT();

    // database name is yymmddhhmmss+random(8)
    srand(time(NULL));
    snprintf(TESTDB, sizeof(TESTDB) - 1,"%u_%05u", (unsigned)time(NULL), rand() % 100000);
    // insert tunnel data for debugging.
    DBPROVIDER_THREAD_INIT();
    auto dbh = DBPROVIDER_INIT(NULL);
    ASSERT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    ASSERT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)"",
                                 port,
                                 NULL,
                                 opt)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    snprintf(sql, sizeof(sql) -1 ,"DROP DATABASE IF EXISTS %s", TESTDB);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);
    PGW_LOG(PGW_LOG_LEVEL_ERR, "(%d: %s)\n", errno, DBPROVIDER_ERROR(dbh));
    snprintf(sql, sizeof(sql) -1 ,"CREATE DATABASE %s", TESTDB);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);
    PGW_LOG(PGW_LOG_LEVEL_ERR, "(%d: %s)\n", errno, DBPROVIDER_ERROR(dbh));

    snprintf(sql, sizeof(sql)-1,"USE %s", TESTDB);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, CREATE_TBL_KEEPALIVE), 0);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, CREATE_TBL_TUNNEL), 0);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, CREATE_TBL_SGW_PEER), 0);

#ifndef __USESQLITE3_ON_TEST__
    DBPROVIDER_CLOSE(dbh);
#else
    // save to reference count.
#endif
}
static void shutdown_database(void){
    int ret;
    U32 port = 3306;
    U32 opt = 0;
    char    sql[128] = {0};

    // cleanup database for testing.
    auto dbh = DBPROVIDER_INIT(NULL);
    ASSERT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    ASSERT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)"",
                                 port,
                                 NULL,
                                 opt)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    snprintf(sql, sizeof(sql) -1 ,"DROP DATABASE IF EXISTS %s", TESTDB);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);
    DBPROVIDER_CLOSE(dbh);
#ifndef __USESQLITE3_ON_TEST__
#else
    // save to reference count.
    DBPROVIDER_CLOSE(dbh);
#endif

}


//

class CustomEnvironment :public ::testing::Environment {
public:
    virtual ~CustomEnvironment() {}
    virtual void SetUp() {
        openlog("pgw_test", LOG_PERROR|LOG_PID,LOG_LOCAL2);
        setup_database();
    }
    virtual void TearDown() {
        shutdown_database();
    }
};