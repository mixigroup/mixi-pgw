//
// Created by mixi on 2017/01/12.
//

#define TUNNEL_TBLCREATE   "CREATE TABLE `tunnel` (\
  `id` bigint(20) NOT NULL AUTO_INCREMENT,\
  `imsi` bigint(20) NOT NULL DEFAULT 0,\
  `msisdn` bigint(20) NOT NULL DEFAULT 0,\
  `ueipv4` varchar(64) NOT NULL DEFAULT '',\
  `pgw_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Send Create Session Response:F-TEID',\
  `pgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Create Session Request Recieved Control plane Ipaddress',\
  `pgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'P-GW gtpu ipv4', \
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
  `qos` bigint(20) NOT NULL DEFAULT 0,\
  `teid_mask` bigint(20) NOT NULL DEFAULT 0,\
  `priority` bigint(20) NOT NULL DEFAULT 0,\
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
  PRIMARY KEY (`id`),\
  UNIQUE KEY `pgw_teid` (`pgw_teid`),\
  UNIQUE KEY `msisdn` (`msisdn`),\
  KEY `teid_mask` (`teid_mask`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1"

#define LOGGY_TBLCREATE "CREATE TABLE `log_gy` (\
  `id` bigint(20) NOT NULL AUTO_INCREMENT,\
  `teid` bigint(20) NOT NULL,\
  `ueipv4` bigint(20) NOT NULL,\
  `reporter` varchar(64) NOT NULL DEFAULT '',\
  `used_s5_bytes` bigint(20) NOT NULL DEFAULT 0,\
  `used_sgi_bytes` bigint(20) NOT NULL DEFAULT 0,\
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
  PRIMARY KEY (`id`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1"

#define TUNNEL_GROUPTBLCREATE "CREATE TABLE `tunnel_group` (\
  `id` bigint(20) NOT NULL AUTO_INCREMENT,\
  `name` varchar(64) NOT NULL DEFAULT '' COMMENT 'arbitor(counter), arbitor(translater)',\
  `module_id` bigint(20) NOT NULL DEFAULT 0 COMMENT 'index within entire system',\
  `arbitor_ctrl_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'arbitor(counter), arbitor(translater) control packet recieve ip',\
  `arbitor_ctrl_port` bigint(20) NOT NULL DEFAULT 0 COMMENT 'arbitor(counter), arbitor(translater) control packet recieve udp port',\
  `member` varchar(64) NOT NULL DEFAULT '' COMMENT 'counter, translater(enc+qos/dec)',\
  `member_idx` bigint(20) NOT NULL DEFAULT 0 COMMENT 'index within group ',\
  `member_teid_mask` bigint(20) NOT NULL DEFAULT 0 COMMENT 'prefix mask of teid',\
  `member_ipv4_mask` bigint(20) NOT NULL DEFAULT 0 COMMENT 'prefix mask of ipv4',\
  `member_teid_assigned` bigint(20) NOT NULL DEFAULT 0,\
  `member_teid_capacity` bigint(20) NOT NULL DEFAULT 0,\
  `member_notify_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'ip address of notify to childprocess',\
  `member_notify_port` bigint(20) NOT NULL DEFAULT 0 COMMENT 'port num of notify to childprocess',\
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
  PRIMARY KEY (`id`),\
  KEY `module_id` (`module_id`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1"


#define TUNNEL_DEFTBLCREATE "CREATE TABLE `tunnel_default` (\
  `id` bigint(20) NOT NULL AUTO_INCREMENT,\
  `dns` varchar(256) NOT NULL DEFAULT 0,\
  `ebi` bigint(20) NOT NULL DEFAULT 0,\
  `policy` varchar(64) NOT NULL DEFAULT '',\
  `active` bigint(20) NOT NULL DEFAULT 0 COMMENT 'active = 0/inactive != 0',\
  `bitrate_s5` bigint(20) NOT NULL DEFAULT 0,\
  `bitrate_sgi` bigint(20) NOT NULL DEFAULT 0,\
  `qos` bigint(20) NOT NULL DEFAULT 0,\
  `priority` bigint(20) NOT NULL DEFAULT 0,\
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
  PRIMARY KEY (`id`)\
) ENGINE=InnoDB DEFAULT CHARSET=latin1"



#define INSSQL  "INSERT INTO tunnel(pgw_teid,imsi,msisdn,ueipv4,policy,qos,bitrate_s5,bitrate_sgi,sgw_gtpu_ipv,teid_mask,active,pgw_gtpc_ipv,sgw_gtpc_ipv)VALUES(?,?,?,?,?,?,?,?,?,?,1,'127.0.0.123','170.120.20.1')"
#define UPDSQL  "UPDATE tunnel SET active=1 WHERE msisdn=?"
#define INSSQL_TUNNEL_GROUP "INSERT INTO tunnel_group( \
                        `name`,`module_id`,`arbitor_ctrl_ipv`,`arbitor_ctrl_port`, \
                        `member`,`member_idx`,`member_teid_mask`,`member_ipv4_mask`, \
                        `member_teid_assigned`,`member_teid_capacity`)VALUES(?,?,?,?,?,?,?,?,?,?)"


//
class TestEnv : public ::testing::Environment {
protected:
    virtual void SetUp() {
        char bf[32] = {0};
        snprintf(bf,sizeof(bf)-1,"%d",getpid());
        MysqlCfg::prefix(bf);
        printf("tear up.(%d) MysqlCfg::prefix:[%s]\n", getpid(),bf);
        //
        InitializeTestDb();
    };
    virtual void TearDown() {
        printf("tear down.(%d)\n", getpid());
        UninitializeTestDb();
    };
private:
    void InitializeTestDb(void){
        MysqlCfg  cfg("localhost","root",NULL,NULL,0,0);
        Mysql     con(&cfg);

        std::string dbname = "dbtest";
        std::string sql;
        dbname += MysqlCfg::prefix();

        con.state();
        sql = "DROP DATABASE IF EXISTS " + dbname;
        con.query(sql.c_str());
        sql = "CREATE DATABASE " + dbname;
        con.query(sql.c_str());
        sql = "USE " + dbname;
        con.query(sql.c_str());
        con.query(TUNNEL_TBLCREATE);
        con.query(LOGGY_TBLCREATE);
        con.query(TUNNEL_GROUPTBLCREATE);
        con.query(TUNNEL_DEFTBLCREATE);

        InsertOcsRecord();
        InsertOcsRecord2();
        InsertSysRecord();
    }
    void UninitializeTestDb(void){
        MysqlCfg  cfg("localhost","root",NULL,NULL,0,0);
        Mysql     con(&cfg);

        std::string dbname = "dbtest";
        std::string sql;
        dbname += MysqlCfg::prefix();

        con.state();
        sql = "DROP DATABASE IF EXISTS " + dbname;
        con.query(sql.c_str());
    }

    void InsertOcsRecord(void) {
        MysqlCfg  cfg("localhost","root",NULL,"dbtest",0,0);
        Mysql     con(&cfg);
        MysqlBind bind(INSSQL);
        ///
        bind.bind(1L);
        bind.bind(2L);
        bind.bind(30L);
        bind.bind(3L);
        bind.bind("high");
        bind.bind(13L);
        bind.bind(111L);
        bind.bind(222L);
        bind.bind("192.168.123.123");
        bind.bind(1L);

        MysqlStmt stmt(&con, &bind);
        stmt.execute(NULL,NULL,NULL);
    }
    void InsertOcsRecord2(void){
        MysqlCfg  cfg("localhost","root",NULL,"dbtest",0,0);
        Mysql     con(&cfg);
        MysqlBind bind(INSSQL);
        bind.bind(7L);
        bind.bind(8L);
        bind.bind(90L);
        bind.bind(9L);
        bind.bind("def");
        bind.bind(14L);
        bind.bind(444L);
        bind.bind(555L);
        bind.bind("185.125.32.2");
        bind.bind(7L);

        MysqlStmt stmt(&con, &bind);
        stmt.execute(NULL,NULL,NULL);
    }
    void InsertSysRecord(void){
        MysqlCfg  cfg("localhost","root",NULL,"dbtest",0,0);
        Mysql     con(&cfg);
        std::string sql = "INSERT INTO tunnel_group(`name`,`module_id`,`arbitor_ctrl_ipv`,`arbitor_ctrl_port`,`member`,`member_idx`, \
                            `member_teid_mask`,`member_ipv4_mask`,`member_teid_assigned`,`member_teid_capacity`)VALUES";
        sql += " ('testenv',1,'10.10.10.10',8888,'counter_ingress',50101, 100000,168428552,0,16384)";
        sql += ",('testenv',2,'10.10.10.11',8889,'counter_ingress',50102, 200000,168428552,0,16384)";
        sql += ",('testenv',3,'10.10.10.12',8890,'counter_ingress',50103, 400000,168428552,0,16384)";
        sql += ",('testenv',4,'10.10.10.13',8891,'counter_ingress',50104, 500000,168428552,0,16384)";
        sql += ",('testenv',5,'10.10.10.14',8892,'counter_ingress',50105, 600000,168428552,0,16384)";
        sql += ",('testenv',6,'10.10.10.15',8893,'counter_ingress',50106, 700000,168428552,0,16384)";
        sql += ",('testenv',7,'10.10.10.16',8894,'counter_ingress',50107, 800000,168428552,0,16384)";
        sql += ",('testenv',8,'10.10.10.17',8895,'counter_ingress',50108, 900000,168428552,0,16384)";
        sql += ",('testenv',9,'10.10.10.18',8896,'counter_ingress',50109,1000000,168428552,0,16384)";
        con.query(sql.c_str());
    }

};
int is_dbinit_type_ = 0;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::Environment* _test_env = ::testing::AddGlobalTestEnvironment(new TestEnv);

    process_handle_t    process;
    memset(&process, 0, sizeof(process));

    init_gtpu_header(&process);

    mysql_library_init(argc, argv, NULL);
    return RUN_ALL_TESTS();
}

