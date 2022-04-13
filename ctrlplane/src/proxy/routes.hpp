/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       route.hpp
    @brief      mixi_pgw_ctrl_plane_proxy / general config header
*******************************************************************************
*******************************************************************************
    @date       created(09/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 NetworkTeam. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/apr/2018 
      -# Initial Version
******************************************************************************/


#ifndef XPGW_ROUTES_HPP
#define XPGW_ROUTES_HPP
#include "proxy_def.hpp"
#include "route.hpp"
#include "proxy_stats.hpp"


class DatabaseConfig;
class GeneralConfig;
class DatabaseConnection;
class DbCon;
/** *****************************************************
 * @brief
 * Routes\n
 * \n
 */
class Routes{
public:
    Routes(DatabaseConfig*,DatabaseConfig*,GeneralConfig*);
    virtual ~Routes();
public:
    int Init(void);
    int KeepAlive(void);
    int ExtSock(void);
    int IntSock(void);
    int StatsSock(void);
    int Cache(void);
    U64 StatCount(PSTAT_COUNT, U32, U16);
    void StatAdd(U32, U16, U64, U64, U64);
    void StatReject(U32, U16, U64, U64);
    void StatExpire(int);

public:
    int SetExtRoute(const SOCKADDRIN*);
    int SetSession(const U64,const U32,const U32,const SOCKADDRIN*);
public:
    int FindInternlRouteByGroupFromCache(GROUP, ROUTEINTS&);
    int FindGroupByImsi(U64,GROUP&);
    int FindGroupByAddr(const INADDR*, GROUP&);
    int FindSessionBySeqnoTeid(const U32,const U32, RouteSession&);
private:
    DatabaseConfig* cfg_;
    DatabaseConfig* cfg_n_;
    DatabaseConnection* con_;
    DatabaseConnection* con_n_;
    GeneralConfig*  gencfg_;
    IMSI_GROUP      imsigroup_;
    GROUP_ROUTES    grouproutes_;
    SGW_ROUTES      sgw_routes_;
    SESSIONS        session_;
    PROXYSTATS      stats_;
    int             extsock_;
    int             intsock_;
    int             stssock_;
    U32             uptime_;
private:
    static int      init_;
    static pthread_mutex_t lock_;
};


#endif //XPGW_ROUTES_HPP
