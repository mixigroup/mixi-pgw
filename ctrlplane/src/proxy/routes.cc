/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       reoutes.cc
    @brief      routing container
*******************************************************************************
*******************************************************************************
    @date       created(10/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 10/apr/2018 
      -# Initial Version
******************************************************************************/
#include "proxy.hpp"
#include "routes.hpp"
#include "database_connection.hpp"
#include "general_config.hpp"

int Routes::init_ = 0;
pthread_mutex_t Routes::lock_;
/**
   route : init-constructor\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]  cfg    local databaseconfig 
   @param[in]  gencfg general config 
 */
Routes::Routes(DatabaseConfig* cfg, DatabaseConfig* cfg_n, GeneralConfig* gencfg){
    ASSERT(cfg);
    ASSERT(cfg_n);
    ASSERT(gencfg);
    //
    cfg_ = cfg;
    cfg_n_ = cfg_n;
    gencfg_ = gencfg;
    intsock_ = extsock_ = stssock_ = -1;
    pthread_mutex_init(&Routes::lock_, NULL);
    //
    pthread_mutex_lock(&Routes::lock_);
    Routes::init_++;
    pthread_mutex_unlock(&Routes::lock_);
    uptime_ = (U32)time(NULL);
}
/**
   route : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
Routes::~Routes() {
    if (intsock_ != -1){ close(intsock_); }
    if (extsock_ != -1){ close(extsock_); }
    if (stssock_ != -1){ close(stssock_); }
    if (con_){
        delete con_;
    }
}

/**
   initialize\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return int  0:success , 0!=error
 */
int Routes::Init(){
    ASSERT(cfg_);
    pthread_mutex_lock(&Routes::lock_);
    Routes::init_++;
    pthread_mutex_unlock(&Routes::lock_);
    ASSERT(Routes::init_==2);
    //
    con_ = new DatabaseConnection(cfg_);
    con_n_ = new DatabaseConnection(cfg_n_);
    ASSERT(con_);
    ASSERT(con_n_);

    // cache routing.
    Cache();

    // generate sockets.
    extsock_ = Proxy::CreateSocket(gencfg_->ExtIf(), gencfg_->ExtIp(), gencfg_->ExtPort());
    intsock_ = Proxy::CreateSocket(gencfg_->IntIf(), gencfg_->IntIp(), gencfg_->IntPort());
    stssock_ = Proxy::CreateSocket(gencfg_->StatsIf(), gencfg_->StatsIp(), gencfg_->StatsPort());
    return(OK);
}
/**
   KeepAlive\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return int  0:success , 0!=error
 */
int Routes::KeepAlive(void){
    ASSERT(con_);
    ASSERT(con_n_);
    // for mysql interactive timeout.
    con_->Ping();
    con_n_->Ping();

    return(OK);
}

/**
    find internal-route by group\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]      gid     group  identifier
   @param[in/out]  routes route list
   @return int  0:success , 0!=error
 */
int Routes::FindInternlRouteByGroupFromCache(GROUP gid, ROUTEINTS& routes){
    auto it = grouproutes_.find(GROUP(gid));
    if (it == grouproutes_.end()){
        return(ERR);
    }
    if ((it->second).empty()){
        return(ERR);
    }
    routes = (it->second);
    return(OK);
}
/**
   find group by IMSI\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]      imsi   IMSI  number 
   @param[in/out]  gdi     group  identifier
   @return int  0:success , 0!=error
 */
int Routes::FindGroupByImsi(U64 imsi, GROUP& gid){
    char bf[32] = {0};
    SSTR sql = "SELECT `pgw_group_id` FROM subscribers WHERE imsi = ";
    ASSERT(con_n_);
    snprintf(bf, sizeof(bf)-1,"%llu", imsi);
    sql += bf;
    int fgid = -1;
    //
    con_n_->Select(sql.c_str(), [](MYSQL_ROW row, U32 coln, void* p){
        if (coln != 1){ return(ERR); }
        (*((int*)p)) = atoi(row[0]);
        return(OK);
    }, &fgid);
    gid = GROUP(fgid);
    return(fgid==-1?ERR:OK);
}

/**
   find session by sequence  number + tunnel  identifier\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]      seqno   sequence  number 
   @param[in]      teid    tunnel  identifier
   @param[in/out]  sess    session
   @return int  0:success , 0!=error
 */
int Routes::FindSessionBySeqnoTeid(const U32 seqno,const U32 teid, RouteSession& sess){
    auto key = ((((U64)seqno)<<32)|((U64)teid));
    auto it  = session_.find(key);

    PGW_LOG(PGW_LOG_LEVEL_INF, "FindSessionBySeqnoTeid(%08x/%08x)\n", seqno, teid);

    if (it == session_.end()){
        return(ERR);
    }
    sess = (it->second);
    return(OK);
}



/**
   out-side socket\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return int  socket descriptor
 */
int Routes::ExtSock(void){ return(extsock_); }

/**
   in-side socket\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return int  socket descriptor
 */
int Routes::IntSock(void){ return(intsock_); }

/**
   stats socket\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return int  socket descriptor
 */
int Routes::StatsSock(void){ return(stssock_); }

/**
   get stats counter values\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   ip     ip address  : 0xFFFFFFFF=all
   @param[in]   group   group id : 0xFFFF=all
   @return U64 statistics , default(U64)-1
 */
U64 Routes::StatCount(PSTAT_COUNT e, U32 ip, U16 group){
#define ADDCOUNT(e,i,c,s) {\
    switch(e){\
        case PSTAT_BYTES_REQUEST:       c += i.req_bytes_; break;\
        case PSTAT_BYTES_RESPONSE:      c += i.res_bytes_; break;\
        case PSTAT_COUNT_REQUEST:       c += i.req_cnt_; break;\
        case PSTAT_COUNT_RESPONSE:      c += i.res_cnt_; break;\
        case PSTAT_COUNT_DELAY:         c += i.delay_; break;\
        case PSTAT_REJECTED_REQUEST:    c += i.req_reject_cnt_; break;\
        case PSTAT_REJECTED_RESPONSE:   c += i.res_reject_cnt_; break;\
        case PSTAT_COUNT_SESSION:       c += s.size(); break;\
        default: break;\
    }}
    switch(e){
        case PSTAT_PID:             return((U64)getpid());
        case PSTAT_UPTIME:          return((U64)uptime_);
        case PSTAT_TIME:            return((U64)time(NULL));
        case PSTAT_VERSION:         return((U64)PSTAT_DVERSION);
        //
        default:{
            U64 count = 0;
            for(auto it = stats_.begin();it != stats_.end();++it){
                auto k_ip = (U32)(((it->first)>>32)&0xFFFFFFFF);
                auto k_group = (U16)(((it->first)>>16)&0xFFFF);
                //
                if (ip!=((U32)-1) && group!=((U16)-1)){
                    // ip and group aggregate
                    if (ip == k_ip && group == k_group){
                        ADDCOUNT(e, (it->second), count, session_);
                    }
                }else if (ip!=((U32)-1) && group==((U16)-1)){
                    // ip aggregate
                    if (ip == k_ip){
                        ADDCOUNT(e, (it->second), count, session_);
                    }
                }else{
                    // total aggregate
                    ADDCOUNT(e, (it->second), count, session_);
                }
            }
            return(count);
            }
            break;
    }
    return((U64)0);
}

/**
    proxy status  : statistics accumulate\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   ip         ip address 
   @param[in]   group       group id
   @param[in]   req_bytes  request count
   @param[in]   res_bytes  response count
   @param[in]   delay      delay
 */
void Routes::StatAdd(U32 ip, U16 group, U64 req_bytes, U64 res_bytes, U64 delay){
    auto it = stats_.find(ip);
    PGW_LOG(PGW_LOG_LEVEL_INF, "Routes::StatAdd(%08x:%08x/req:%llu/res:%llu/delay:%llu)\n", ip, group, req_bytes, res_bytes, delay);
    if (it == stats_.end()){
        stats_[ip] = ProxyStat(ip, group, req_bytes?1:0, res_bytes?1:0, delay, req_bytes, res_bytes);
    }else{
        (it->second).Add(req_bytes, res_bytes, delay);
    }
}
/**
    proxy status  : statistics accumulate\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   ip         ip address 
   @param[in]   group       group id
   @param[in]   req_reject_bytes  request  : reject bytes
   @param[in]   res_reject_bytes  response  : reject bytes
 */
void Routes::StatReject(U32 ip, U16 group, U64 req_reject_bytes, U64 res_reject_bytes){
    auto it = stats_.find(ip);
    PGW_LOG(PGW_LOG_LEVEL_INF, "Routes::StatReject(%08x:%08x/req:%llu,res:%llu)\n", ip, group, req_reject_bytes, res_reject_bytes);
    if (it == stats_.end()){
        stats_[ip] = ProxyStat(ip, group, 0, 0, 0, 0, 0);
        stats_[ip].Reject(req_reject_bytes, res_reject_bytes);
    }else{
        (it->second).Reject(req_reject_bytes, res_reject_bytes);
    }
}
/**
    proxy status  : statistics expire\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   elapsed     elapsed time
 */
void Routes::StatExpire(int elapsed){
    auto it = stats_.begin();
    auto now = (U32)time(NULL) - elapsed;
    for(;it != stats_.end();){
        if ((it->second).created_ < now) {
            stats_.erase(it++);
        }else{
            ++it;
        }
    }
}


/**
   update out-side socket\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   addr   host address 
   @return int  0:success , 0!=error
 */
int Routes::SetExtRoute(const SOCKADDRIN* addr){
    ASSERT(addr);
    sgw_routes_[(U32)addr->sin_addr.s_addr] = RouteExt(addr, 0, extsock_);
    return(OK);
}
/**
   update session cache\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   imsi   IMSI number 
   @param[in]   seqno  sequence  number 
   @param[in]   teid   SGW GTPC tunnel  identifier
   @param[in]   addr   host address 
   @return int  0:success , 0!=error
 */
int Routes::SetSession(const U64 imsi, const U32 seqno, const U32 teid, const SOCKADDRIN* addr){
    ASSERT(addr);
    auto key = ((((U64)seqno)<<32)|((U64)teid));
    //
    if (session_.find(key) == session_.end()){
        PGW_LOG(PGW_LOG_LEVEL_INF, "Routes::SetSession(%08x/%08x)\n", seqno, teid);
        session_[key] = RouteSession(imsi, seqno, teid, addr);
    }
    return(OK);
}

/**
   generate routing cache.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return int  0:success , 0!=error
 */
int Routes::Cache(void){
    ASSERT(con_);
    ASSERT(con_n_);
    //
    grouproutes_.clear();

    // cache routing
    con_->Select("SELECT `pgw_group_id`, `dst_ip` FROM proxy", [](MYSQL_ROW row, U32 coln, void* p){
        if (coln != 2){ return(ERR); }
        auto route = (Routes*)p;
        auto gid = atoi(row[0]);
        auto port  = GTPC_PORT;
        auto it = route->grouproutes_.find(GROUP(gid));
        //
        if (it != route->grouproutes_.end()){
            (it->second).push_back(RouteInt(row[1], 0, port));
        }else{
            ROUTEINTS  rts;
            rts.push_back(RouteInt(row[1], 0, port));
            route->grouproutes_[GROUP(gid)] = rts;
        }
        return(OK);
    }, this);
    return(OK);
}


