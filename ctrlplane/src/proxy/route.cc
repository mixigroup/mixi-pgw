/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       reoute.cc
    @brief      routing [externl/internal/session cache]
*******************************************************************************
*******************************************************************************
    @date       created(12/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 12/apr/2018 
      -# Initial Version
******************************************************************************/
#include "proxy.hpp"
#include "route.hpp"


/**
   route :  constructor \n
   *******************************************************************************
   internal \n
   *******************************************************************************
 */
RouteInt::RouteInt(){
    host_ = "";
    cnt_ = 0;
    port_ = 0;
}
/**
   route : init-constructor\n
   *******************************************************************************
   internal \n
   *******************************************************************************
   @param[in]  host   host , IP address 
   @param[in]  cnt    count
   @param[in]  port   port number 
 */
RouteInt::RouteInt(const char* host, U64 cnt, int port){
    host_ = host;
    cnt_ = cnt;
    port_ = port;
}
/**
   route : copy constructor \n
   *******************************************************************************
   internal \n
   *******************************************************************************
   @param[in]  cp   source instance
 */
RouteInt::RouteInt(const RouteInt& cp){
    host_ = cp.host_;
    cnt_ = cp.cnt_;
    port_ = cp.port_;
}
/**
   route : destructor\n
   *******************************************************************************
   internal \n
   *******************************************************************************
 */
RouteInt::~RouteInt(){
}
/**
   host名\n
   *******************************************************************************
   internal\n
   *******************************************************************************
   @return const char* host名
*/
const char* RouteInt::Host(void){
    return(host_.c_str());
}
/**
   counter\n
   *******************************************************************************
   internal \n
   *******************************************************************************
   @return U64 counter value
*/
U64 RouteInt::Count(void){
    return(cnt_);
}
/**
   port number \n
   *******************************************************************************
   internal\n
   *******************************************************************************
   @return int port number 
*/
int RouteInt::Port(void){
    return(port_);
}

/**
   route :  constructor \n
   *******************************************************************************
   external \n
   *******************************************************************************
 */
RouteExt::RouteExt(){
    bzero(&host_, sizeof(host_));
    cnt_ = 0;
    sock_ = -1;
}
/**
   route : init-constructor\n
   *******************************************************************************
   external \n
   *******************************************************************************
   @param[in]  host   host , IP address 
   @param[in]  cnt    count
   @param[in]  sock   socket descriptor
 */
RouteExt::RouteExt(const SOCKADDRIN* host, U64 cnt, int sock){
    host_ = (*host);
    cnt_ = cnt;
    sock_ = sock;
}
/**
   route : copy constructor \n
   *******************************************************************************
   external \n
   *******************************************************************************
   @param[in]  cp   source instance
 */
RouteExt::RouteExt(const RouteExt& cp){
    host_ = cp.host_;
    cnt_ = cp.cnt_;
    sock_ = cp.sock_;
}
/**
   route : destructor\n
   *******************************************************************************
   external \n
   *******************************************************************************
 */
RouteExt::~RouteExt(){
}
/**
   host name\n
   *******************************************************************************
   external \n
   *******************************************************************************
   @return const char* host name
*/
SOCKADDRIN* RouteExt::Host(void){ return(&host_); }
/**
   counter\n
   *******************************************************************************
   external \n
   *******************************************************************************
   @return U64 counter value
*/
U64 RouteExt::Count(void){ return(cnt_); }
/**
   socket descriptor\n
   *******************************************************************************
   external \n
   *******************************************************************************
   @return int socket
*/
int RouteExt::Sock(void){ return(sock_); }


/**
   session cache :  constructor \n
   *******************************************************************************
   cache \n
   *******************************************************************************
 */
RouteSession::RouteSession(){
    bzero(&host_, sizeof(host_));
    imsi_ = 0;
    seqno_ = 0;
    teid_ = 0;
    created_ = 0;
}
/**
   session cache : init-constructor\n
   *******************************************************************************
   cache \n
   *******************************************************************************
   @param[in]  imsi   IMSI number
   @param[in]  seqno  sequence  number 
   @param[in]  teid   tunnel  identifier
   @param[in]  host   host address 
 */
RouteSession::RouteSession(const U64 imsi, const U32 seqno, const U32 teid, const SOCKADDRIN* host){
    host_ = (*host);
    imsi_ = imsi;
    seqno_ = seqno;
    teid_ = teid;
    created_ = (U32)time(NULL);
}
/**
   session cache : copy constructor \n
   *******************************************************************************
   cache \n
   *******************************************************************************
   @param[in]  cp   source instance
 */
RouteSession::RouteSession(const RouteSession& cp){
    host_ = cp.host_;
    imsi_ = cp.imsi_;
    seqno_ = cp.seqno_;
    teid_ = cp.teid_;
    created_ = cp.created_;
}
/**
   session cache : destructor\n
   *******************************************************************************
   cache \n
   *******************************************************************************
 */
RouteSession::~RouteSession(){
}

/**
   host name\n
   *******************************************************************************
   cache \n
   *******************************************************************************
   @return const char* host name
*/
SOCKADDRIN* RouteSession::Host(void){ return(&host_); }

/**
   created_at : epoch\n
   *******************************************************************************
   cache \n
   *******************************************************************************
   @return U32 created_at : epoch
*/
U32 RouteSession::Created(void){ return(created_); }
