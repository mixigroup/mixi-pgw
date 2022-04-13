/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy.cc
    @brief      gtpc proxy
*******************************************************************************
*******************************************************************************
    @date       created(09/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/apr/2018 
      -# Initial Version
******************************************************************************/
#include "proxy.hpp"
#include "database_config.hpp"
#include "general_config.hpp"
#include "routes.hpp"

pthread_mutex_t __mysql_mutex;
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
int PGW_RECOVERY_COUNT = GTPC_RECOVERY_1;



/**
  gtpc proxy : main entry.\n
 *******************************************************************************
 + \n
 *******************************************************************************
 @param[in]     argc   count of arguments.
 @param[in]     argv   arguments
 @return        0/success , !=0/error
 */
int main(int argc, char *argv[]){
    char        cfgf[128] = {0};

    signal(SIGUSR2, Proxy::Signal);
    // initialize
    openlog(argv[0], LOG_PERROR|LOG_PID,LOG_LOCAL2);
    PGW_LOG(PGW_LOG_LEVEL_INF, " >> start %s(%p)\n", argv[0], (void*)pthread_self());

    // read options
    Proxy::ReadArgs(argc, argv, cfgf, sizeof(cfgf)-1);
    if (!cfgf[0]){
        Proxy::Usage(argv[0], "invalid args.");
        return(ERR);
    }
    //
    mysql_thread_init();
    // Database config[local/network server]
    DatabaseConfig  local_dbcfg(cfgf, "");
    DatabaseConfig  nwsv_dbcfg(cfgf,  "_N");
    // general config 
    GeneralConfig   gencfg(cfgf);
    // routing
    Routes          routes(&local_dbcfg, &nwsv_dbcfg, &gencfg);
    routes.Init();
    //
    auto event_base_ext = event_base_new();
    auto event_base_int = event_base_new();
    auto event_base_sts = event_base_new();
    auto recv_event_ext = event_new(event_base_ext, routes.ExtSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveExt, &routes);
    auto recv_event_int = event_new(event_base_int, routes.IntSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveInt, &routes);
    auto recv_event_sts = event_new(event_base_sts, routes.StatsSock(), EV_READ|EV_PERSIST, Proxy::OnRecieveStats, &routes);
    auto timeout_event_ext = event_new(event_base_ext, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, &routes);
    auto timeout_event_int = event_new(event_base_int, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, &routes);
    auto timeout_event_sts = event_new(event_base_sts, -1, EV_TIMEOUT|EV_PERSIST, Proxy::OnTimeOut, &routes);
    //
    struct timeval tm;
    tm.tv_sec = (TIMEOUTMS / 1000);
    tm.tv_usec = (TIMEOUTMS % 1000) * 1000;
    if (event_add(recv_event_ext, NULL) || event_add(timeout_event_ext, &tm) ||
        event_add(recv_event_int, NULL) || event_add(timeout_event_int, &tm) ||
        event_add(recv_event_sts, NULL) || event_add(timeout_event_sts, &tm)) {
        pgw_panic("event_add failed(%d: %s)\n", errno, strerror(errno));
    }
    // out-side/in-side/statsevent loop
    while(!Proxy::Quit()){
        event_base_loop(event_base_ext, EVLOOP_ONCE);
        event_base_loop(event_base_int, EVLOOP_ONCE);
        event_base_loop(event_base_sts, EVLOOP_ONCE);
    }
    //
    closelog();
    return(OK);
}
