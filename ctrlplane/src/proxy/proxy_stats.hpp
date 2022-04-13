/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy_stats.hpp
    @brief      mixi_pgw_ctrl_plane_proxy / proxy status container
*******************************************************************************
*******************************************************************************
    @date       created(24/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 NetworkTeam. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 24/apr/2018 
      -# Initial Version
******************************************************************************/

#ifndef XPGW_PROXY_STATS_HPP
#define XPGW_PROXY_STATS_HPP
#include "proxy_def.hpp"

/** *****************************************************
 * @brief
 * proxy status\n
 * \n
 */
class ProxyStat{
public:
    ProxyStat();
    ProxyStat(U32, U16, U64, U64, U64, U64, U64);
    ProxyStat(const ProxyStat&);
    virtual ~ProxyStat();
public:
    void Clear(void);
    void Add(U64, U64, U64);
    void Reject(U64, U64);
public:
    U32    ip_;
    U16    group_;
    U64    req_bytes_;
    U64    req_cnt_;
    U64    res_bytes_;
    U64    res_cnt_;
    U64    delay_;
    U64    req_reject_bytes_;
    U64    res_reject_bytes_;
    U64    req_reject_cnt_;
    U64    res_reject_cnt_;
    U32    created_;
};
typedef std::map<U64, ProxyStat> PROXYSTATS;        /*!< map status of ip,group */
// U64 : MAKE_PSTATKEY((U32)ip, (U16)group, (U16)padd)
#define MAKE_PSTATKEY(a,b,c)    ((((U64)a)<<32)|(((U64)b&0xFFFF)<<16)|(((U64)c&0xFFFF)))



#endif //XPGW_PROXY_STATS_HPP
