//
// Created by mixi on 2017/06/13.
//
#include "../mixipgw_mod_radius.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>

using namespace MIXIPGW_TOOLS;

template<typename T> static inline void ini_get(boost::property_tree::ptree& pt, const char* key, T* v, const T& def){
    boost::optional<T> tv = pt.get_optional<T>(key);
    (*v) = def;
    if (tv){ (*v) = tv.get(); }
}


// constructor
RadiusServer::RadiusServer(const char* cfg){
    boost::property_tree::ptree pt;
    read_ini(cfg, pt);
    //
    ini_get<std::string>(pt, "DATABASE.PORT", &dbport_, "");
    ini_get<std::string>(pt, "DATABASE.HOST", &dbhost_, "");
    ini_get<std::string>(pt, "DATABASE.USER", &dbuser_, "");
    ini_get<std::string>(pt, "DATABASE.PSWD", &dbpswd_, "");
    ini_get<std::string>(pt, "RADIUS.SECRET", &radius_secret_, "");
    ini_get<int>        (pt, "RADIUS.CPUID",  &cpuid_, 0);
    ini_get<std::string>(pt, "RADIUS.BINLOG", &radius_binlog_, "");
    ini_get<int>        (pt, "RADIUS.BINLOGPOS", &radius_binlog_position_, 4);
}
// destructor.
RadiusServer::~RadiusServer(){
    Stop();
    //
    if (event_base_){ event_base_free(event_base_); }
    event_base_ = NULL;
    if (recv_event_){ event_free(recv_event_); }
    recv_event_ = NULL;
    if (timeout_event_){ event_free(timeout_event_); }
    timeout_event_ = NULL;
    //
    close(sock_);
    sock_ = -1;
    // release local resources.
    for(auto it = radius_identifier_.begin();it != radius_identifier_.end();++it){
        delete (it->second).cli;
    }
    radius_identifier_.clear();
}

void *RadiusServer::RunLoop(void *arg){
    RadiusServer* inst = (RadiusServer*)arg;

    inst->InitTable();

    // main loop.
    while(!Module::ABORT()){
        event_base_loop(inst->event_base_, EVLOOP_ONCE);
    }
    return(NULL);
}
// start .
void RadiusServer::Start(){
    if (pthread_create(&threadid_, NULL, RadiusServer::RunLoop, this)) {
        throw std::runtime_error("pthread_create..(run udp server loop.)");
	}
    if (pthread_create(&threadid_db_, NULL, RadiusServer::BinLogLoop, this)) {
        throw std::runtime_error("pthread_create..(run binlog loop.)");
    }
}
// stop
void RadiusServer::Stop(){
    Module::ABORT_INCR();
    if (event_base_){
        event_base_loopexit(event_base_, NULL);
    }
    pthread_join(threadid_, NULL);
    pthread_join(threadid_db_, NULL);
}
void RadiusServer::InitTable(){
    int on = 1;
    Misc::SetAffinity(cpuid_);

    pthread_mutex_init(&radius_link_mtx_,NULL);
    timeout_counter_ = 0;
    sock_ = -1;
    bzero(&addr_, sizeof(addr_));
    addrlen_ = sizeof(addr_);
#ifdef	EVTHREAD_USE_PTHREADS_IMPLEMENTED
    evthread_use_pthreads();
#endif
    if ((sock_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        throw std::runtime_error("socket..");
    }
    addr_.sin_family    = AF_INET;
    addr_.sin_port      = htons(RADIUS_UDP_PORT);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    //
    if (bind(sock_, (struct sockaddr*)&addr_, sizeof(addr_))) {
        throw std::runtime_error("bind..");
    }
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))< 0){
        throw std::runtime_error("setsockopt(SO_REUSEADDR)..");
    }
    auto flags = fcntl(sock_,F_GETFL,0);
    if (flags < 0){
        throw std::runtime_error("fcntl(F_GETFL)..");
    }
    fcntl(sock_,F_SETFL,flags|O_NONBLOCK);
    //
    if (!(event_base_ = event_base_new())){
        throw std::runtime_error("malloc event base..");
    }
    recv_event_     = event_new(event_base_, sock_, EV_READ|EV_PERSIST, RadiusServer::OnRecv, this);
    timeout_event_  = event_new(event_base_, -1, EV_TIMEOUT|EV_PERSIST, RadiusServer::OnTimeOut, this);
    if (!recv_event_ || !timeout_event_){
        throw std::runtime_error("malloc event..");
    }
    struct timeval now;
    now.tv_sec  = RADIUS_TIMECOUNTER / 1000;
    now.tv_usec = (RADIUS_TIMECOUNTER % 1000) * 1000;
    //
    if (event_add(recv_event_, NULL) || event_add(timeout_event_, &now)) {
        throw std::runtime_error("event_add..");
    }
}

// set radius table
void RadiusServer::OperateTable(MIXIPGW_TOOLS::radius_link_ptr lnk){
    pthread_mutex_lock(&radius_link_mtx_);
    //
    if (lnk->stat.type == LINKMAPTYPE_UP){
        Logger::LOGINF("update (key : " FMT_LLU "/ipv: %u/nas: %u/ipv6: %08x:%08x:%08x:%08x)",
                       lnk->key, lnk->ipv4, lnk->nasipv,
                       lnk->ipv6[0], lnk->ipv6[1],lnk->ipv6[2],lnk->ipv6[3]);
        auto it = radius_link_.find(lnk->key);
        if (it != radius_link_.end()){
            (it->second)[lnk->nasipv] = (*lnk);
        }else{
            std::map<ULONGLONG, MIXIPGW_TOOLS::radius_link_t>  tmp;
            tmp[lnk->nasipv] = (*lnk);
            radius_link_[lnk->key] = tmp;
        }
    }else{
        radius_link_[lnk->key].erase(lnk->nasipv);
    }
    pthread_mutex_unlock(&radius_link_mtx_);
}
// find radius table
bool RadiusServer::FindTable(MIXIPGW_TOOLS::radius_link_ptr lnk){
    pthread_mutex_lock(&radius_link_mtx_);
    auto it = radius_link_.find(lnk->key);
    if (it == radius_link_.end()){
        pthread_mutex_unlock(&radius_link_mtx_);
        Logger::LOGINF("not found. key " FMT_LLU " at " FMT_LLU ")", lnk->key, radius_link_.size());
        return(false);
    }
    if ((it->second).find(lnk->nasipv) == (it->second).end()){
        pthread_mutex_unlock(&radius_link_mtx_);
        Logger::LOGINF("not found. nasipv (%u)", lnk->nasipv);
        return(false);
    }
    (*lnk) = (it->second)[lnk->nasipv];
    pthread_mutex_unlock(&radius_link_mtx_);
    Logger::LOGINF("exists. (" FMT_LLU " %u)", lnk->key, lnk->nasipv);
    return(true);
}

