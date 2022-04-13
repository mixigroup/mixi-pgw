/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       general_config.hpp
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

#ifndef XPGW_GENERAL_CONFIG_HPP
#define XPGW_GENERAL_CONFIG_HPP


class Config;
/** *****************************************************
 * @brief
 * General Config\n
 * \n
 */
class GeneralConfig{
public:
    GeneralConfig(const char*);
    virtual ~GeneralConfig();
public:
    const int ExtPort(void);
    const char* ExtIf(void);
    const char* ExtIp(void);
    const int IntPort(void);
    const char* IntIf(void);
    const char* IntIp(void);
    const int StatsPort(void);
    const char* StatsIf(void);
    const char* StatsIp(void);
private:
    Config  *config_;
private:
    GeneralConfig(){}
};


#endif //XPGW_GENERAL_CONFIG_HPP
