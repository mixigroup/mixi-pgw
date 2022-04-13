//
// Created by mixi on 2017/04/28.
//
#include "../mixipgw_mod_sgw_def.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>

template<typename T> static inline void ini_get(boost::property_tree::ptree& pt, const char* key, T* v, const T& def){
    boost::optional<T> tv = pt.get_optional<T>(key);
    (*v) = def;
    if (tv){ (*v) = tv.get(); }
}


SgwSim::SgwSim(const char* cfg){
    std::string host,mcast;
    uint16_t    port;
    //
    if (Misc::IsExists(cfg) == RETOK){
        boost::property_tree::ptree pt;
        cfg_ = cfg;

        read_ini(cfg_.c_str(), pt);
        //
        ini_get<uint16_t>   (pt, "CPLANE.PORT", &port, 0);
        ini_get<std::string>(pt, "CPLANE.HOST", &host, "");
        pgw_gtpc_ = HostPort(host, "", port);
        //
        ini_get<uint16_t>   (pt, "DATAPLANE.PORT", &port, 0);
        ini_get<std::string>(pt, "DATAPLANE.HOST", &host, "");
        pgw_gtpu_ = HostPort(host, "", port);
        //
        ini_get<uint16_t>   (pt, "SGWEMU.PORT", &port, 0);
        ini_get<std::string>(pt, "SGWEMU.HOST", &host, "");
        ini_get<std::string>(pt, "SGWEMU.MCAST",&mcast,"");
        sgw_emuc_ = HostPort(host, mcast, port);
        //
        ini_get<uint16_t>   (pt, "SGWEMU.UPORT", &port, 0);
        ini_get<std::string>(pt, "SGWEMU.UHOST", &host, "");
        ini_get<std::string>(pt, "SGWEMU.UMCAST",&mcast,"");
        sgw_emuu_ = HostPort(host, mcast, port);
        //
        ini_get<uint16_t>   (pt, "SGWEMU.DBPORT", &port, 0);
        ini_get<std::string>(pt, "SGWEMU.DBHOST", &host, "");
        sgw_database_ = HostPort(host, "", port);
        //
        ini_get<uint16_t>   (pt, "SGWEMU.BFDPORT", &port, 0);
        ini_get<std::string>(pt, "SGWEMU.BFDHOST", &host, "");
        ini_get<std::string>(pt, "SGWEMU.BFDMCAST",&mcast,"");
        sgw_bfd_ = HostPort(host, mcast, port);
        //
        ini_get<uint16_t>   (pt, "SGWEMU.SNDRTPORT", &port, 0);
        ini_get<std::string>(pt, "SGWEMU.SNDRTHOST", &host, "");
        ini_get<std::string>(pt, "SGWEMU.SNDRTMCAST",&mcast,"");
        sgw_sndrt_ = HostPort(host, mcast, port);

        ini_get<uint16_t>   (pt, "DATABASE.PORT", &dbport_, 0);
        ini_get<std::string>(pt, "DATABASE.HOST", &dbhost_, "");
        ini_get<std::string>(pt, "DATABASE.USER", &dbuser_, "");
        ini_get<std::string>(pt, "DATABASE.PSWD", &dbpswd_, "");
        ini_get<std::string>(pt, "DATABASE.INST", &dbinst_, "");
    }else{
        throw std::runtime_error("missing config(SgwSim::SgwSim)");
    }
}


// initialize contract data. active = 0
void SgwSim::Init(int num){
    std::string sql = "INSERT INTO tunnel(`imsi`,`msisdn`,`ueipv4`,`pgw_teid`,`pgw_gtpc_ipv`,`pgw_gtpu_ipv`,`dns`,`ebi`, ";
    sql += "`sgw_gtpc_teid`, `sgw_gtpc_ipv`, `sgw_gtpu_teid`, `sgw_gtpu_ipv`,";
    sql += "`policy`,`active`,`bitrate_s5`,`bitrate_sgi`,`qci`,`qos`,`teid_mask`,`priority`) ";
    sql += " VALUES ";

    MysqlCfg  cfg(dbhost_,dbuser_,dbpswd_,dbinst_);
    Mysql  con(&cfg);

    if (con.Query("TRUNCATE TABLE tunnel") != RETOK){
        throw std::runtime_error("failed. setup database(truncate)");
    }
    //
    auto nn = 0ULL;

    for(auto n = 0; n < ((num / 1000));n++){
        fprintf(stderr, "%d / %d\n", n,  ((num / 1000)));
        std::string values = sql;
        for(auto m = 0;m < 1000;m++){
            uint64_t imsi = IMSI_BASE;
            uint64_t msisdn = MSISDN_BASE;
            char bf[256] = {0};
            //
            nn++;
            auto calc = (n*1000) + m;
            //
            values += m==0?"(":",(";
            snprintf(bf, sizeof(bf)-1,
                FMT_LLU "," FMT_LLU ",'11.12.13.14'," FMT_LLU ",'127.0.0.13','127.0.0.13','00000d0408080808',0,0,'',0,'','',0,128,128,0,10,0,0",
                     (imsi + calc), (msisdn + calc), (uint64_t)calc);
            values += bf;
            values += ")";
        }
        if (con.Query(values.c_str()) != RETOK){
            fprintf(stderr, "%s\n", values.c_str());
            throw std::runtime_error("failed. setup database..(insert)");
        }
    }
}
