/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       config.cc
    @brief      config
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
#include "config.hpp"

/**
   config  : init-constructor\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     cfgfile       config file path
 */
Config::Config(const char* cfgfile){
    ASSERT(cfgfile);
    if (cfgfile){
        loadConfig(cfgfile);
    }
}
/**
   config  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
Config::~Config() { }

/**
   get value by key\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     key   key string
   @return int  result:0xffffffff=error
 */
int Config::GetInt(const char* key){
    int	ret = -1;
    auto itr = keyvalues_.find(key);
    if (itr != keyvalues_.end()){
        ret = atoi(itr->second.c_str());
    }
    return((int)ret);
}

/**
   get string value by key\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     key   key string
   @return const char* result:NULL=error
 */
const char* Config::GetText(const char* key) {
    auto itr = keyvalues_.find(key);
    if (itr == keyvalues_.end()){
        return(NULL);
    }
    return(itr->second.c_str());
}
/**
   config load\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     cfgfile  config file path
   @return int 0: success , 0!=:error
 */
int	Config::loadConfig(const char* cfgfile){
    char*	token = NULL;
    char*	delim = (char*)" =:\n";
    char	line[512] = {0};
    //
    ASSERT(cfgfile);
    //
    auto fp = fopen(cfgfile,"r");
    ASSERT(fp);
    if (fp){
        while(fgets(line,sizeof(line) - 1,fp) != NULL){
            char	key[128] = {0};
            char	val[128] = {0};
            if ((token = strtok(line, delim)) != NULL){
                strncpy(key, token, MIN(sizeof(key) - 1, strlen(token)));
                if(token != NULL){
                    token = strtok(NULL, delim);
                    if (token != NULL){
                        strncpy(val, token, MIN(sizeof(val) - 1, strlen(token)));
                        // comment(exclude [#])
                        if (key[0] != '#'){
                            keyvalues_[key] = val;
                        }
                    }
                }
            }
            memset(line,0,sizeof(line));
        }
        fclose(fp);
    }
    return(0);
}
