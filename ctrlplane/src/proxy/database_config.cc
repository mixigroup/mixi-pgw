/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       database_config.cc
    @brief      database config wrapper
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
#include "config.hpp"

static const char* CFG_KEY_HOST = "HOST";
static const char* CFG_KEY_USER = "USER";
static const char* CFG_KEY_PSWD = "PSWD";
static const char* CFG_KEY_INST = "INST";
static const char* CFG_KEY_PORT = "PORT";


/**
   databaseconfig  : init-constructor\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     cfgfile       config file path
   @param[in]     suffix        suffix
 */
DatabaseConfig::DatabaseConfig(const char* cfgfile, const char* suffix) {
    config_ = new Config(cfgfile);
    ASSERT(config_);
    ASSERT(suffix);
    suffix_ = suffix;
}
/**
   databaseconfig  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
DatabaseConfig::~DatabaseConfig(){
    if (config_){ delete config_; }
}
/**
   port number \n
   *******************************************************************************
   Key = PORT + suffix\n
   *******************************************************************************
   @return int  port number / 0xffffffff=error
 */
const int DatabaseConfig::Port(void){
    ASSERT(config_);
    std::string key(CFG_KEY_PORT);
    key += suffix_;
    return(config_->GetInt(key.c_str()));
}
/**
   host name \n
   *******************************************************************************
   Key = HOST + suffix\n
   *******************************************************************************
   @return const char* host name / NULL=error
 */
const char* DatabaseConfig::Host(void){
    ASSERT(config_);
    std::string key(CFG_KEY_HOST);
    key += suffix_;
    return(config_->GetText(key.c_str()));
}
/**
   user\n
   *******************************************************************************
   Key = USER + suffix\n
   *******************************************************************************
   @return const char* user/ NULL=error
 */
const char* DatabaseConfig::User(void){
    std::string key(CFG_KEY_USER);
    key += suffix_;
    return(config_->GetText(key.c_str()));
}
/**
   password\n
   *******************************************************************************
   Key = PSWD + suffix\n
   *******************************************************************************
   @return const char* password/ NULL=error
 */
const char* DatabaseConfig::Pswd(void){
    std::string key(CFG_KEY_PSWD);
    key += suffix_;
    return(config_->GetText(key.c_str()));
}
/**
   instance\n
   *******************************************************************************
   Key = INST + suffix\n
   *******************************************************************************
   @return const char* instance/ NULL=error
 */
const char* DatabaseConfig::Inst(void){
    std::string key(CFG_KEY_INST);
    key += suffix_;
    return(config_->GetText(key.c_str()));
}

