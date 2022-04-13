/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy.hpp
    @brief      mixi_pgw_ctrl_plane_proxy c++ header
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

#ifndef XPGW_PROXY_HH
#define XPGW_PROXY_HH
#include "proxy_def.hpp"


/** *****************************************************
 * @brief
 * Proxy - Utils\n
 * \n
 */
class Proxy{
public:
    static void ReadArgs(int argc, char *argv[], char* cfg, int cfglen);
    static void Usage(const char* name, const char* fmt, ...);
    static void OnTimeOut(evutil_socket_t sock, short what, void* arg);
    static void OnRecieveExt(evutil_socket_t sock, short what, void* arg);
    static void OnRecieveInt(evutil_socket_t sock, short what, void* arg);
    static void OnRecieveStats(evutil_socket_t sock, short what, void* arg);
    static int  Quit(void);
    static int  Init(const char*);
    static int  CreateSocket(const char*, const char*, int);
    static void Signal(int);
private:
    static int quit_;
    static int reload_;
private:
    Proxy(){}
    ~Proxy(){}
};


//

#endif //XPGW_PROXY_HH
