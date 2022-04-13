/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       general_config.cc
    @brief      general: config wrapper
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
#include "general_config.hpp"
#include "config.hpp"

/**
   general base config  : init-constructor\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     cfgfile       config file path
 */
GeneralConfig::GeneralConfig(const char* cfgfile) {
    config_ = new Config(cfgfile);
    ASSERT(config_);
}
/**
   general config  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
GeneralConfig::~GeneralConfig(){
    if (config_){ delete config_; }
}
/**
   out-side : port number \n
   *******************************************************************************
   Key = PORT \n
   *******************************************************************************
   @return int  port number / 0xffffffff=error
 */
const int GeneralConfig::ExtPort(void){
    ASSERT(config_);
    std::string key("EXT_PORT");
    auto ret = config_->GetInt(key.c_str());
    ASSERT(ret);
    return(ret);
}
/**
   out-side : interface \n
   *******************************************************************************
   Key = IF \n
   *******************************************************************************
   @return const char* interface  name / NULL=error
 */
const char* GeneralConfig::ExtIf(void){
    ASSERT(config_);
    std::string key("EXT_IF");
    auto ret = config_->GetText(key.c_str());
    ASSERT(ret);
    return(ret);
}
/**
   out-side : IP address \n
   *******************************************************************************
   Key = IP\n
   *******************************************************************************
   @return const char* IP address / NULL=error
 */
const char* GeneralConfig::ExtIp(void){
    std::string key("EXT_IP");
    auto ret = config_->GetText(key.c_str());
    ASSERT(ret);
    return(ret);
}

/**
   internal-side : port number \n
   *******************************************************************************
   Key = PORT \n
   *******************************************************************************
   @return int  port number / 0xffffffff=error
 */
const int GeneralConfig::IntPort(void){
    ASSERT(config_);
    std::string key("INT_PORT");
    auto ret = config_->GetInt(key.c_str());
    ASSERT(ret);
    return(ret);
}
/**
   internal-side : interface \n
   *******************************************************************************
   Key = IF \n
   *******************************************************************************
   @return const char* interface  name / NULL=error
 */
const char* GeneralConfig::IntIf(void){
    ASSERT(config_);
    std::string key("INT_IF");
    auto ret = config_->GetText(key.c_str());
    ASSERT(ret);
    return(ret);
}
/**
   internal-side : IP address \n
   *******************************************************************************
   Key = IP\n
   *******************************************************************************
   @return const char* IP address / NULL=error
 */
const char* GeneralConfig::IntIp(void){
    std::string key("INT_IP");
    auto ret = config_->GetText(key.c_str());
    ASSERT(ret);
    return(ret);
}


/**
   stats : port number \n
   *******************************************************************************
   Key = PORT \n
   *******************************************************************************
   @return int  port number / 0xffffffff=error
 */
const int GeneralConfig::StatsPort(void){
    ASSERT(config_);
    std::string key("STS_PORT");
    auto ret = config_->GetInt(key.c_str());
    ASSERT(ret);
    return(ret);
}
/**
   stats : interface \n
   *******************************************************************************
   Key = IF \n
   *******************************************************************************
   @return const char* interface  name / NULL=error
 */
const char* GeneralConfig::StatsIf(void){
    ASSERT(config_);
    std::string key("STS_IF");
    auto ret = config_->GetText(key.c_str());
    ASSERT(ret);
    return(ret);
}
/**
   stats : IP address \n
   *******************************************************************************
   Key = IP\n
   *******************************************************************************
   @return const char* IP address / NULL=error
 */
const char* GeneralConfig::StatsIp(void){
    std::string key("STS_IP");
    auto ret = config_->GetText(key.c_str());
    ASSERT(ret);
    return(ret);
}

