//
// Created by mixi on 2017/06/13.
//
#include "../mixipgw_mod_diameter.hpp"
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
Diameter::Diameter(const char* cfg){
    boost::property_tree::ptree pt;
    read_ini(cfg, pt);
    //
    ini_get<std::string>(pt, "DATABASE.PORT", &dbport_, "");
    ini_get<std::string>(pt, "DATABASE.HOST", &dbhost_, "");
    ini_get<std::string>(pt, "DATABASE.USER", &dbuser_, "");
    ini_get<std::string>(pt, "DATABASE.PSWD", &dbpswd_, "");
    ini_get<std::string>(pt, "DATABASE.INST", &dbinst_, "");
    ini_get<int>        (pt, "DATABASE.SERVERID", &dbserverid_, 0);
    //
    ini_get<int>        (pt, "DIAMETER.PORT", &port_, 0);
    ini_get<int>        (pt, "DIAMETER.CPUID",&cpuid_, 0);
    ini_get<std::string>(pt, "DIAMETER.LISTENIF", &listen_interface_, "");
    ini_get<std::string>(pt, "DIAMETER.ORIGIN_HOST", &origin_host_, "");
    ini_get<std::string>(pt, "DIAMETER.ORIGIN_REALM", &origin_realm_, "");
    ini_get<std::string>(pt, "DIAMETER.HOST_IP_ADDR", &host_ip_address_, "");
    ini_get<std::string>(pt, "DIAMETER.PRODUCT_NAME", &product_name_, "");
    ini_get<int>        (pt, "DIAMETER.VENDOR_ID",&vendor_id_, 0);
    ini_get<std::string>(pt, "DIAMETER.BINLOG", &diameter_binlog_, "");
    ini_get<int>        (pt, "DIAMETER.BINLOGPOS", &diameter_binlog_position_, 4);

//
    Logger::LOGINF("diameter service(%s[%d]%d)",listen_interface_.c_str(), port_, cpuid_);

    mysqlconfig_ = new MysqlCfg(dbhost_, dbuser_,dbpswd_,dbinst_);
    //
    event_base_     = NULL;
    recv_event_     = NULL;
    timeout_event_  = NULL;
    accept_sock_    = -1;
    client_counter_ = 0;
    monitoring_key_ = "";
    bzero(event_base_cli_,sizeof(event_base_cli_));
}
// destructor.
Diameter::~Diameter(){
    Stop();
    //
    if (event_base_){ event_base_free(event_base_); }
    event_base_ = NULL;
    if (recv_event_){ event_free(recv_event_); }
    recv_event_ = NULL;
    if (timeout_event_){ event_free(timeout_event_); }
    timeout_event_ = NULL;
    //
    close(accept_sock_);
    accept_sock_ = -1;
    if (mysqlconfig_){ delete mysqlconfig_; }
}


void *Diameter::ClientLoop(void * arg){
    auto ev = (struct event_base*)arg;

    Logger::LOGINF("Diameter::ClientLoop(%p)", ev);

    // client loop.
    while(!Module::ABORT()){
        auto r = event_base_loop(ev, EVLOOP_ONCE);
        if (r == 0){
            // success.
        }else if (r < 0){
            // errored.
            Logger::LOGERR("failed.event_base_loop(%p:%d)", ev, r);
        }else{
            // no events.
            usleep(10);
        }
    }
    return(NULL);
}
void *Diameter::RunLoop(void *arg){
    Diameter* inst = (Diameter*)arg;

    inst->InitTable();

    Logger::LOGINF("Diameter::RunLoop[accept](%p)", inst);
    // main loop.
    while(!Module::ABORT()){
        auto r = event_base_loop(inst->event_base_, EVLOOP_ONCE);
        if (r == 0){
            // success.
        }else if (r < 0){
            // errored.
            Logger::LOGERR("failed.event_base_loop(%p:%d)", inst->event_base_, r);
        }else{
            // no events.
            usleep(10);
        }

    }
    return(NULL);
}
// start .
void Diameter::Start(){
    // accept
    if (pthread_create(&threadid_, NULL, Diameter::RunLoop, this)) {
        throw std::runtime_error("pthread_create..(run tcp server accept loop.)");
	}
	// for binlog
    if (pthread_create(&threadid_db_, NULL, Diameter::BinLogLoop, this)) {
        throw std::runtime_error("pthread_create..(run binlog loop.)");
    }
    // tcp client.
    for(auto n = 0;n < DIAMETER_CLIENT_THREAD;n++){
        // receive thread
        if (!(event_base_cli_[n] = event_base_new())){
            throw std::runtime_error("malloc event base client..");
        }
        if (pthread_create(&threadid_cli_[n], NULL, Diameter::ClientLoop, event_base_cli_[n])) {
            throw std::runtime_error("pthread_create..(client loop.)");
        }
    }
    // queue
    for(auto n = 0;n < DIAMETER_QUEUE_COUNT;n++){
        queue_loop_container_[n].id_ = n;
        queue_loop_container_[n].ptr_ = this;
        // init mutex object
        pthread_mutex_init(&diameter_queue_mtx_[n], NULL);
        if (pthread_create(&threadid_queue_[n], NULL, Diameter::QueueLoop, &queue_loop_container_[n])) {
            throw std::runtime_error("pthread_create..(queue loop.)");
        }
    }
}
// stop
void Diameter::Stop(){
    Module::ABORT_INCR();
    if (event_base_){
        event_base_loopexit(event_base_, NULL);
    }
    pthread_join(threadid_, NULL);
    pthread_join(threadid_db_, NULL);
    //
    for(auto n = 0;n < DIAMETER_CLIENT_THREAD;n++){
        if (event_base_cli_[n]){
            event_base_loopexit(event_base_cli_[n], NULL);
        }
        pthread_join(threadid_cli_[n], NULL);
    }
    for(auto n = 0;n < DIAMETER_QUEUE_COUNT;n++){
        pthread_join(threadid_queue_[n], NULL);
    }
}
void Diameter::InitTable(){
    int on = 1;
    Misc::SetAffinity(cpuid_);

    pthread_mutex_init(&diameter_link_mtx_,NULL);
    timeout_counter_ = 0;
    accept_sock_ = -1;
    bzero(&addr_, sizeof(addr_));
    addrlen_ = sizeof(addr_);
#ifdef	EVTHREAD_USE_PTHREADS_IMPLEMENTED
    evthread_use_pthreads();
#endif
    if ((accept_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        throw std::runtime_error("socket..");
    }
    if (setsockopt(accept_sock_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        throw std::runtime_error("SO_REUSEADDR..");
    }

    addr_.sin_family    = AF_INET;
    addr_.sin_port      = htons(port_);
    inet_pton(AF_INET, listen_interface_.c_str() ,&addr_.sin_addr.s_addr);
    //
    if (bind(accept_sock_, (struct sockaddr*)&addr_, sizeof(addr_))) {
        throw std::runtime_error("bind..");
    }
    auto flags = fcntl(accept_sock_,F_GETFL,0);
    if (flags < 0){
        throw std::runtime_error("fcntl(F_GETFL)..");
    }
    fcntl(accept_sock_,F_SETFL,flags|O_NONBLOCK);
    on = 1;
    if (setsockopt(accept_sock_, SOL_TCP, TCP_NODELAY, &on, sizeof(on)) < 0){
        throw std::runtime_error("nagle off..");
    }
    on = (1024*1024*32);
    if (setsockopt(accept_sock_, SOL_SOCKET, SO_RCVBUF, &on, sizeof(on)) < 0){
        throw std::runtime_error("recieve buffer size..");
    }
    on = (1024*1024*32);
    if (setsockopt(accept_sock_, SOL_SOCKET, SO_SNDBUF, &on, sizeof(on)) < 0){
        throw std::runtime_error("send buffer size..");
    }

     if (listen(accept_sock_, DIAMETER_LISTEN_Q) < 0){
         throw std::runtime_error("listen..");
     }
    //
    if (!(event_base_ = event_base_new())){
        throw std::runtime_error("malloc event base..");
    }
    recv_event_     = event_new(event_base_, accept_sock_, EV_READ|EV_PERSIST, Diameter::OnAccept, this);
    timeout_event_  = event_new(event_base_, -1, EV_TIMEOUT|EV_PERSIST, Diameter::OnTimeOut, this);
    if (!recv_event_ || !timeout_event_){
        throw std::runtime_error("malloc event..");
    }
    struct timeval now; // 1sec timer.
    now.tv_sec  = 1;
    now.tv_usec = 0;
    // event in tcp accept loop at main thread.
    if (event_add(recv_event_, NULL) || event_add(timeout_event_, &now)) {
        throw std::runtime_error("event_add..");
    }
}
// diameter table operation
void Diameter::OperateTable(MIXIPGW_TOOLS::diameter_link_ptr lnk){
    const char *operate = "unknown";
    pthread_mutex_lock(&diameter_link_mtx_);
    //
    if (lnk->stat.type == LINKMAPTYPE_UP){
        auto it = diameter_link_.find(lnk->key);
        if (it != diameter_link_.end()){
            operate = "update";
            (it->second)[lnk->nasipv] = (*lnk);
        }else{
            operate = "insert";
            std::map<ULONGLONG, MIXIPGW_TOOLS::diameter_link_t>  tmp;
            tmp[lnk->nasipv] = (*lnk);
            diameter_link_[lnk->key] = tmp;
        }
    }else{
        operate = "delete";
        diameter_link_[lnk->key].erase(lnk->nasipv);
    }
    pthread_mutex_unlock(&diameter_link_mtx_);
    //
    Logger::LOGINF("%s (key : " FMT_LLU "/ipv: %u/nas: %u/ipv6: %08x:%08x:%08x:%08x)%s %u/%u[%u]",
                   operate,
                   lnk->key, lnk->ipv4, lnk->nasipv,
                   lnk->ipv6[0], lnk->ipv6[1],lnk->ipv6[2],lnk->ipv6[3],
                   lnk->policy, lnk->ingress, lnk->egress, lnk->threshold);
}
// find diameter table
bool Diameter::FindTable(MIXIPGW_TOOLS::diameter_link_ptr lnk){
    pthread_mutex_lock(&diameter_link_mtx_);
    auto it = diameter_link_.find(lnk->key);
    if (it == diameter_link_.end()){
        pthread_mutex_unlock(&diameter_link_mtx_);
        return(false);
    }
    if ((it->second).find(lnk->nasipv) == (it->second).end()){
        pthread_mutex_unlock(&diameter_link_mtx_);
        return(false);
    }
    (*lnk) = (it->second)[lnk->nasipv];
    pthread_mutex_unlock(&diameter_link_mtx_);
    return(true);
}

