/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy_utils.cc
    @brief      gtpc proxy utils
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

int Proxy::quit_ = OK;
int Proxy::reload_ = OK;

/**
  usage\n
 *******************************************************************************
  print usage\n
 *******************************************************************************
 */
void Proxy::Usage(const char* name, const char* fmt, ...){
    char msg[512] = {0};
    va_list ap;
    printf("============\n");
    printf("%s\n", name);
    printf("------------\n");
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg)-1, fmt, ap);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n============\n");
    printf("\t-c <config file>\n");
}

/**
  option parse \n
 *******************************************************************************
 + \n
 *******************************************************************************
 @param[in]     argc   count of arguments.
 @param[in]     argv   arguments
 @param[in/out] cfg    config file path buffer
 @param[in]     cfglen cfg buffer length
 */
void Proxy::ReadArgs(int argc, char *argv[], char* cfg, int cfglen){
    int ch;
    // default log level.
    PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;

    // read arguments , bind ip, NIC, server_id
    while ( (ch = getopt(argc, argv, "c:l:h")) != -1) {
        switch(ch){
            case 'l':       /* log level */
                if (optarg != NULL){
                    PGW_LOG_LEVEL = atoi(optarg);
                    if (PGW_LOG_LEVEL < 0 || PGW_LOG_LEVEL > PGW_LOG_LEVEL_MAX){
                        PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
                    }
                }
                break;
            case 'c':       /* config */
                if (optarg != NULL){
                    strncpy(cfg, optarg, MIN(cfglen, strlen(optarg)));
                }
                break;
            case 'h':       /* help */
                Proxy::Usage(argv[0], "help");
                break;
            default: /* unknown option */
                Proxy::Usage(argv[0], "Option %c is not defined.", ch);
                break;
        }
    }
}
/**
  initialize\n
 *******************************************************************************
  \n
 *******************************************************************************
 @param[in]     cfg       config file path
 @return        0/ok , !=0/error
 */
int Proxy::Init(const char* cfg){
    DatabaseConfig  dbcfg(cfg, "");




    return(OK);
}

/**
  is quit\n
 *******************************************************************************
  \n
 *******************************************************************************
 @return        0/continue , !=0/quit
 */
int  Proxy::Quit(void){
    return(Proxy::quit_);
}
/**
  create socket\n
 *******************************************************************************
  \n
 *******************************************************************************
 @param[in]     ifnm       interface  name 
 @param[in]     ip         IP address 
 @param[in]     port       port number 
 @return        0/ok , !=0/error
 */
int Proxy::CreateSocket(const char* ifnm, const char* ip, int port){
    auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ASSERT(sock >= 0);

    PGW_LOG(PGW_LOG_LEVEL_INF, "udp - socket. bind (%s/%s/%d)\n", ifnm, ip , port);
#ifndef __linux__
#ifdef SO_BINDTODEVICE
    PGW_LOG(PGW_LOG_LEVEL_ERR, "linux__ \n");
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, ifnm, strlen(ifnm)+1);
#endif
#endif
    if (set_nonblock(sock)) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. socket unblocking(%d: %s)\n", errno, strerror(errno));
        return(-1);
    }
    //
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, (char*)ip, &addr.sin_addr.s_addr);
    //
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. bind (%d: %s)\n", errno, strerror(errno));
        return(-1);
    }
    return(sock);
}
/**
  user signal\n
 *******************************************************************************
  \n
 *******************************************************************************
 @param[in]     signo  signal number 
 @return        0/ok , !=0/error
 */
void Proxy::Signal(int signo){
    Proxy::reload_++;
    PGW_LOG(PGW_LOG_LEVEL_INF, "signal_user(%d)\n", Proxy::reload_);
}

