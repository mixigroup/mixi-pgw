/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       send.c
    @brief      injection packet
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 07/nov/2017 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"

#define PGWSEND_RETRY_CNT       (10)
#define PGWSEND_TOMS            (100000)


/**
 inject packet.\n
 *******************************************************************************
 send packet\n
 *******************************************************************************
 @param[in]     pinst     instance
 @parma[in]     saddr     source address
 @parma[in]     caddr     destination address
 @param[in]     caddrlen  length
 @param[in]     packet    packet 
 @param[in]     length    packet length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_send(handle_ptr pinst, const struct sockaddr_in* saddr, const struct sockaddr_in* caddr, const ssize_t caddrlen, const PTR packet, U32 length){
    server_ptr  pserver = NULL;
    INT         ret = 0;
    INT         sendlen = 0;
    INT	        allsend = 0;
    INT	        errored = 0;
    INT         retrycnt = 0;
    INT         len = (INT)length;
    char*	    senddata = (char*)packet;
    fd_set	    fdset;
    struct timeval  tm;
    //
    if (!pinst){
        return(ERR);
    }
    // send from gtpc interface or gtpu interface
    if (pinst->server_gtpc->addr.sin_addr.s_addr == saddr->sin_addr.s_addr &&
        pinst->server_gtpc->addr.sin_port == saddr->sin_port){
        pserver = pinst->server_gtpc;
    }else if (pinst->server_gtpu->addr.sin_addr.s_addr == saddr->sin_addr.s_addr &&
        pinst->server_gtpu->addr.sin_port == saddr->sin_port){
        pserver = pinst->server_gtpu;
    }
    if (pserver == NULL){
        return(NOTFOUND);
    }

    while(1){
        tm.tv_sec  = 0;
        tm.tv_usec = PGWSEND_TOMS;
        FD_ZERO(&fdset);
        FD_SET(pserver->sock,&fdset);
        ret = select(pserver->sock + 1,NULL,&fdset,NULL,&tm);
        //retry count is over.
        if (retrycnt ++ > PGWSEND_RETRY_CNT){
            errored = 1;
            break;
        }
        if (ret == -1){
            if (errno == EINTR){
            	continue;
            }
            errored = 1;
            break;
        }else if(ret > 0){		//sended. any bytes data..
            if(FD_ISSET(pserver->sock, &fdset)){
                sendlen = sendto(pserver->sock, (senddata + allsend), (len - allsend), 0, (struct sockaddr*)caddr, caddrlen);
                if (sendlen < 0){
                    errored = 1;
                    break;
                }
                allsend += sendlen;
                if (allsend >= len){	break;	}
            }
        }else{
            break;
        }
    }
    //it's errored.
    if (errored){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_send(%d: %s)\n", errno, strerror(errno));
        return(ERR);
    }
    return(OK);
}



/**
 inject packet.\n
 *******************************************************************************
 send packet from specific socket\n
 *******************************************************************************
 @param[in]     sock      socket descriptor
 @parma[in]     caddr     destination address
 @param[in]     caddrlen  destination address length
 @param[in]     packet    packet 
 @param[in]     length    packet length
 @return RETCD  0==success, 0!=error
*/
RETCD pgw_send_sock(int sock, const struct sockaddr_in* caddr, const ssize_t caddrlen, const PTR packet, U32 length){
    INT         ret = 0;
    INT         sendlen = 0;
    INT	        allsend = 0;
    INT	        errored = 0;
    INT         retrycnt = 0;
    INT         len = (INT)length;
    char*	    senddata = (char*)packet;
    fd_set	    fdset;
    struct timeval  tm;


    while(1){
        tm.tv_sec  = 0;
        tm.tv_usec = PGWSEND_TOMS;
        FD_ZERO(&fdset);
        FD_SET(sock,&fdset);
        ret = select(sock + 1,NULL,&fdset,NULL,&tm);
        //retry count is over.
        if (retrycnt ++ > PGWSEND_RETRY_CNT){
            errored = 1;
            break;
        }
        if (ret == -1){
            if (errno == EINTR){
                continue;
            }
            errored = 1;
            break;
        }else if(ret > 0){		//sended. any bytes data..
            if(FD_ISSET(sock, &fdset)){
                sendlen = sendto(sock, (senddata + allsend), (len - allsend), 0, (struct sockaddr*)caddr, caddrlen);
                if (sendlen < 0){
                    errored = 1;
                    break;
                }
                allsend += sendlen;
                if (allsend >= len){	break;	}
            }
        }else{
            break;
        }
    }
    //it's errored.
    if (errored){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_send(%d: %s)\n", errno, strerror(errno));
        return(ERR);
    }
    return(OK);
}

