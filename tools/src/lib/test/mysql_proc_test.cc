//
// Created by mixi on 2017/05/29.
//

#include "gtest/gtest.h"

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/misc.hpp"
#include "lib/module.hpp"
#include "lib_db/mysql.hpp"
using namespace MIXIPGW_TOOLS;

#define TBL_TUNNEL "\
CREATE TABLE `tunnel` (\
`id` bigint(20) NOT NULL AUTO_INCREMENT,\
`imsi` bigint(20) NOT NULL DEFAULT 0,\
`msisdn` bigint(20) NOT NULL DEFAULT 0,\
`ueipv4` varchar(64) NOT NULL DEFAULT '',\
`pgw_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Send Create Session Response:F-TEID',\
`pgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Create Session Request Recieved Control plane Ipaddress',\
`pgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'P-GW gtpu ipv4',\
`dns` varchar(256) NOT NULL DEFAULT 0,\
`ebi` bigint(20) NOT NULL DEFAULT 0,\
`sgw_gtpc_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Recieved by Create Session Request:F-TEID.',\
`sgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Recieved by Create Session Request:F-TEID.',\
`sgw_gtpu_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Recieved by Create Session Request:BEARERCTX:F-TEID.',\
`sgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Recieved by Create Session Request:BEARERCTX:F-TEID.',\
`policy` varchar(64) NOT NULL DEFAULT '',\
`active` bigint(20) NOT NULL DEFAULT 0 COMMENT 'active = 0/inactive != 0',\
`bitrate_s5` bigint(20) NOT NULL DEFAULT 0,\
`bitrate_sgi` bigint(20) NOT NULL DEFAULT 0,\
`qci` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Recieved by Create Session Request: Qci.',\
`qos` bigint(20) NOT NULL DEFAULT 0,\
`teid_mask` bigint(20) NOT NULL DEFAULT 0,\
`priority` bigint(20) NOT NULL DEFAULT 0,\
`updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
PRIMARY KEY (`id`),\
UNIQUE KEY `pgw_teid` (`pgw_teid`),\
UNIQUE KEY `msisdn` (`msisdn`),\
KEY `teid_mask` (`teid_mask`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1;"


#define TBL_LOG_GY "\
CREATE TABLE `log_gy` (\
`id` bigint(20) NOT NULL AUTO_INCREMENT,\
`teid` bigint(20) NOT NULL,\
`ueipv4` bigint(20) NOT NULL DEFAULT 0,\
`reporter` varchar(64) NOT NULL DEFAULT '',\
`used_s5_bytes` bigint(20) NOT NULL DEFAULT 0,\
`used_sgi_bytes` bigint(20) NOT NULL DEFAULT 0,\
`created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
PRIMARY KEY (`id`),\
KEY `teid` (`teid`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1;"

#define TBL_CONTRACT "\
CREATE TABLE `contract` (\
`id` bigint(20) NOT NULL AUTO_INCREMENT,\
`teid` bigint(20) NOT NULL,\
`max_traffic_month` bigint(20) NOT NULL DEFAULT 0,\
`base_traffic_s5` bigint(20) NOT NULL DEFAULT 0,\
`base_traffic_sgi` bigint(20) NOT NULL DEFAULT 0,\
`guaranteed_min_traffic_s5` bigint(20) NOT NULL DEFAULT 0,\
`guaranteed_min_traffic_sgi` bigint(20) NOT NULL DEFAULT 0,\
`updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
PRIMARY KEY (`id`),\
KEY `teid` (`teid`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1;"

#define PRC_LOG2TNL "\
CREATE PROCEDURE log_gy_to_tunnel(IN __teid_s BIGINT, IN __teid_e BIGINT)\n\
BEGIN\n\
  DECLARE _teid BIGINT;\n\
  DECLARE _used_s5_bytes BIGINT;\n\
  DECLARE _used_sgi_bytes BIGINT;\n\
  DECLARE _max_traffic_month BIGINT;\n\
  DECLARE _guaranteed_min_traffic_s5 BIGINT;\n\
  DECLARE _guaranteed_min_traffic_sgi BIGINT;\n\
  DECLARE _base_traffic_s5 BIGINT;\n\
  DECLARE _base_traffic_sgi BIGINT;\n\
\n\
  DECLARE _done INT DEFAULT 0;\n\
  DECLARE _cur_log CURSOR FOR\n\
    SELECT teid,MAX(used_s5_bytes),MAX(used_sgi_bytes)\n\
    FROM log_gy_work\n\
    WHERE teid BETWEEN __teid_s AND __teid_e\n\
    GROUP BY teid;\n\
  DECLARE CONTINUE HANDLER FOR NOT FOUND SET _done = 1;\n\
\n\
  DROP TABLE IF EXISTS log_gy_work;\n\
  CREATE TABLE log_gy_work LIKE log_gy;\n\
  INSERT INTO log_gy_work SELECT * FROM log_gy WHERE teid BETWEEN __teid_s AND __teid_e;\n\
  DELETE FROM log_gy WHERE id IN (SELECT id FROM log_gy_work);\n\
\n\
  OPEN _cur_log;\n\
\
  __FETCH: LOOP\n\
    FETCH _cur_log INTO _teid,_used_s5_bytes, _used_sgi_bytes;\n\
    IF _done = 1 THEN\n\
      LEAVE __FETCH;\n\
    END IF;\n\
    IF EXISTS(SELECT 1 FROM contract WHERE teid = _teid) THEN\n\
      SELECT guaranteed_min_traffic_s5,guaranteed_min_traffic_sgi,base_traffic_s5,base_traffic_sgi,max_traffic_month\n\
      INTO _guaranteed_min_traffic_s5,_guaranteed_min_traffic_sgi,_base_traffic_s5,_base_traffic_sgi,_max_traffic_month\n\
      FROM contract\n\
      WHERE teid = _teid;\n\
\
      IF (_max_traffic_month < _used_s5_bytes OR _max_traffic_month < _used_sgi_bytes) THEN\n\
        UPDATE tunnel SET bitrate_s5 = _guaranteed_min_traffic_s5,bitrate_sgi = _guaranteed_min_traffic_sgi WHERE pgw_teid = _teid;\n\
      ELSE\n\
        UPDATE tunnel SET bitrate_s5 = _base_traffic_s5,bitrate_sgi = _base_traffic_sgi WHERE pgw_teid = _teid;\n\
      END IF;\n\
    END IF;\n\
  END LOOP;\n\
  CLOSE _cur_log;\n\
END;"


TEST(MysqlProc, LogGyToTunnel){
    char bf[32] = {0};
    std::string csql = "";
    Mysql::Init();
    Module::Init(NULL);
    EXPECT_EQ(MIXIPGW_TOOLS::Logger::Init("",NULL,NULL), RETOK);

    mysql_thread_init();


    snprintf(bf,sizeof(bf)-1,"LogGyTest_%d",getpid());
#ifdef __APPLE__
    MysqlCfg  cfg(NULL,NULL,NULL,NULL,0,0);
#else
    MysqlCfg  cfg("127.0.0.1",NULL,NULL,NULL,0,0);
#endif
    Mysql  con(&cfg);
    MysqlCfg::Prefix(bf);
    char sql[1024] = {0};

    snprintf(sql,sizeof(sql)-1,"DROP DATABASE IF EXISTS %s",bf);
    EXPECT_EQ(con.Query(sql),RETOK);
    snprintf(sql,sizeof(sql)-1,"CREATE DATABASE %s",bf);
    EXPECT_EQ(con.Query(sql),RETOK);
    snprintf(sql,sizeof(sql)-1,"USE %s",bf);
    EXPECT_EQ(con.Query(sql),RETOK);

    // prepare tables
    EXPECT_EQ(con.Query(TBL_TUNNEL),RETOK);
    EXPECT_EQ(con.Query(TBL_LOG_GY),RETOK);
    EXPECT_EQ(con.Query(TBL_CONTRACT),RETOK);
    // prepare procedures.
    EXPECT_EQ(con.Query("DROP PROCEDURE IF EXISTS log_gy_to_tunnel"),RETOK);
    EXPECT_EQ(con.Query(PRC_LOG2TNL),RETOK);

    // set data for procedure testing. プロシージャを評価するためのデータ投入

    // contract(teid:1234,10GB/monthly , generic bit rate : 10Mbps , limitation bit rate : 128Kbps
    csql  = "INSERT INTO contract(";
    csql += "teid,max_traffic_month,base_traffic_s5,base_traffic_sgi,guaranteed_min_traffic_s5,guaranteed_min_traffic_sgi";
    csql += ")VALUES(";
    csql += "1234,10737418240,1310720,1310720,16384,16384";
    csql += ")";
    EXPECT_EQ(con.Query(csql.c_str()),RETOK);

    // traffic log
    EXPECT_EQ(con.Query("INSERT INTO log_gy (teid,used_s5_bytes,used_sgi_bytes)VALUES(1234,111111,1111)"),RETOK);
    EXPECT_EQ(con.Query("INSERT INTO log_gy (teid,used_s5_bytes,used_sgi_bytes)VALUES(1234,222222,2222)"),RETOK);
    // tunnel
    csql  = "INSERT INTO tunnel(";
    csql += "imsi,msisdn,pgw_teid,bitrate_s5,bitrate_sgi";
    csql += ")VALUES(";
    csql += "4409012341234,12341234,1234,987654321,987654321";
    csql += ")";

    EXPECT_EQ(con.Query(csql.c_str()), RETOK);
    EXPECT_EQ(con.Query("CALL log_gy_to_tunnel(1234,1234)"), RETOK);

    MysqlStmt   stmt(&con, "SELECT * FROM tunnel");
    std::vector<RECORDS> rec;


    // monthly limit not reached.
    rec.clear();
    auto ret = stmt.Execute([](uint64_t c,const RECORDS* r, void* u)->int { ((std::vector<RECORDS>*)u)->push_back(*r); return(RETOK); }, &rec, NULL);
    EXPECT_EQ(ret, RETOK);
    EXPECT_EQ(rec.size(), 1);
    if (rec.size() == 1){
        // no overflow -> basic value 10Mbps.
        EXPECT_EQ(rec[0]["bitrate_s5"].Dec(), 1310720);
        EXPECT_EQ(rec[0]["bitrate_sgi"].Dec(), 1310720);
    }
    // insert logs reaching monthly limit.
    // Exactly 10GB -> no overflow
    EXPECT_EQ(con.Query("INSERT INTO log_gy (teid,used_s5_bytes,used_sgi_bytes)VALUES(1234,10737418240,0)"),RETOK);
    EXPECT_EQ(con.Query("CALL log_gy_to_tunnel(1,1234)"), RETOK);
    // monthly limit not reached
    rec.clear();
    ret = stmt.Execute([](uint64_t c,const RECORDS* r, void* u)->int { ((std::vector<RECORDS>*)u)->push_back(*r); return(RETOK); }, &rec, NULL);
    EXPECT_EQ(ret, RETOK);
    EXPECT_EQ(rec.size(), 1);
    if (rec.size() == 1){
        // no overflow -> basic value 10Mbps.
        EXPECT_EQ(rec[0]["bitrate_s5"].Dec(), 1310720);
        EXPECT_EQ(rec[0]["bitrate_sgi"].Dec(), 1310720);
    }
    // communication by 1 byte will exceed 10 GBytes
    EXPECT_EQ(con.Query("INSERT INTO log_gy (teid,used_s5_bytes,used_sgi_bytes)VALUES(1234,10737418241,0)"),RETOK);
    EXPECT_EQ(con.Query("CALL log_gy_to_tunnel(1234,65535)"), RETOK);
    // Reach monthly limit.
    rec.clear();
    ret = stmt.Execute([](uint64_t c,const RECORDS* r, void* u)->int { ((std::vector<RECORDS>*)u)->push_back(*r); return(RETOK); }, &rec, NULL);
    EXPECT_EQ(ret, RETOK);
    EXPECT_EQ(rec.size(), 1);
    if (rec.size() == 1){
        // overflow -> limiting value 128 Kbps.
        EXPECT_EQ(rec[0]["bitrate_s5"].Dec(), 16384);
        EXPECT_EQ(rec[0]["bitrate_sgi"].Dec(), 16384);
    }
    // cleanup
    snprintf(sql,sizeof(sql)-1,"DROP DATABASE IF EXISTS %s",bf);
    EXPECT_EQ(con.Query(sql),RETOK);
}
