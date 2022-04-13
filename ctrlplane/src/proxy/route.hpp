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


#ifndef XPGW_ROUTE_HPP
#define XPGW_ROUTE_HPP
#include "proxy_def.hpp"


/** *****************************************************
 * @brief
 * Route Internal\n
 * \n
 */
class RouteInt{
public:
    RouteInt();
    RouteInt(const char* ,U64, int);
    RouteInt(const RouteInt&);
    virtual ~RouteInt();
public:
    const char* Host(void);
    int Port(void);
    U64 Count(void);
private:
    SSTR    host_;
    int     port_;
    U64     cnt_;
};

/** *****************************************************
 * @brief
 * Route External\n
 * \n
 */
class RouteExt{
public:
    RouteExt();
    RouteExt(const SOCKADDRIN* ,U64, int);
    RouteExt(const RouteExt&);
    virtual ~RouteExt();
public:
    SOCKADDRIN* Host(void);
    U64 Count(void);
    int Sock(void);
private:
    SOCKADDRIN host_;
    U64     cnt_;
    int     sock_;
};


/** *****************************************************
 * @brief
 * Route Session Cache\n
 * \n
 */
class RouteSession{
public:
    RouteSession();
    RouteSession(const U64, const U32, const U32, const SOCKADDRIN* );
    RouteSession(const RouteSession&);
    virtual ~RouteSession();
public:
    SOCKADDRIN* Host(void);
    U32 Created(void);
private:
    SOCKADDRIN host_;
    U64     imsi_;
    U32     seqno_;
    U32     teid_;
    U32     created_;
};



/*! @name mixi_pgw_ctrl_plane_proxy typedef */
/* @{ */
typedef std::vector<RouteInt>       ROUTEINTS;       /*!< internal route list */
typedef std::map<GROUP, ROUTEINTS>  GROUP_ROUTES;    /*!< map group/route internal list */
typedef std::map<U32, RouteExt>     SGW_ROUTES;      /*!< map ipv4/route external*/
typedef std::map<U64, RouteSession> SESSIONS;        /*!< map imsi/session */
/* @} */


#endif //XPGW_ROUTE_HPP
