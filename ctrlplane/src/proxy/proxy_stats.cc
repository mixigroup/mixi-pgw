/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy_stats.cc
    @brief      gtpc proxy stats
*******************************************************************************
*******************************************************************************
    @date       created(24/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 24/apr/2018 
      -# Initial Version
******************************************************************************/
#include "proxy.hpp"
#include "proxy_stats.hpp"


/**
    proxy status  : init-constructor\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]  ip        ip address 
   @param[in]  group      group  identifier
   @param[in]  req_cnt   request count
   @param[in]  res_cnt   response count
   @param[in]  req_bytes requeset length
   @param[in]  res_bytes response length
   @param[in]  delay     delay
 */
ProxyStat::ProxyStat(U32 ip, U16 group, U64 req_cnt, U64 res_cnt, U64 delay, U64 req_bytes, U64 res_bytes){
    ip_ = ip;
    group_ = group;
    req_cnt_ = req_cnt;
    res_cnt_ = res_cnt;
    req_bytes_ = req_bytes;
    res_bytes_ = res_bytes;
    delay_ = delay;
    req_reject_bytes_ = 0;
    res_reject_bytes_ = 0;
    req_reject_cnt_ = 0;
    res_reject_cnt_ = 0;
    created_ = time(NULL);
}



/**
    proxy status  : copy constructor \n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]  cp       source instance
 */
ProxyStat::ProxyStat(const ProxyStat& cp){
    ip_ = cp.ip_;
    group_ = cp.group_;
    req_cnt_ = cp.req_cnt_;
    res_cnt_ = cp.res_cnt_;
    delay_ = cp.delay_;
    req_bytes_ = cp.req_bytes_;
    res_bytes_ = cp.res_bytes_;
    req_reject_bytes_ = cp.req_reject_bytes_;
    res_reject_bytes_ = cp.res_reject_bytes_;
    req_reject_cnt_ = cp.req_reject_cnt_;
    res_reject_cnt_ = cp.res_reject_cnt_;
    created_ = cp.created_;
}
/**
    proxy status  : default constructor \n
   *******************************************************************************
   default constructor\n
   *******************************************************************************
 */
ProxyStat::ProxyStat():ip_(0), group_(0), req_cnt_(0), res_cnt_(0), delay_(0),
                        req_bytes_(0), res_bytes_(0), req_reject_bytes_(0), res_reject_bytes_(0),
                        req_reject_cnt_(0), res_reject_cnt_(0), created_(time(NULL)){

}

/**
    proxy status  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
ProxyStat::~ProxyStat(){
}
/**
    proxy status  : init statistics\n
   *******************************************************************************
   \n
   *******************************************************************************
 */
void ProxyStat::Clear(void){
    req_bytes_ = res_bytes_ = req_cnt_ = res_cnt_ = delay_ =
        req_reject_bytes_ = res_reject_bytes_ = req_reject_cnt_ = res_reject_cnt_ = 0;
    created_ = time(NULL);
}
/**
    proxy status  : statistics accumulate\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]  req_bytes  request count
   @param[in]  res_bytes  response count
   @param[in]  delay      delay
 */
void ProxyStat::Add(U64 req_bytes, U64 res_bytes, U64 delay){
    req_bytes_ += req_bytes;
    res_bytes_ += res_bytes;
    if (req_bytes){ req_cnt_ ++; }
    if (res_bytes){ res_cnt_ ++; }
    delay_ += delay;
    created_ = time(NULL);
}

/**
    proxy status  : statistics accumulate\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]  req_reject_bytes  request  : reject bytes
   @param[in]  res_reject_bytes  response  : reject bytes
 */
void ProxyStat::Reject(U64 req_reject_bytes, U64 res_reject_bytes){
    req_reject_bytes_ += req_reject_bytes;
    if (req_reject_bytes){
        req_reject_cnt_ ++;
    }
    res_reject_bytes_ += res_reject_bytes;
    if (res_reject_bytes_){
        res_reject_cnt_ ++;
    }
    created_ = time(NULL);
}
