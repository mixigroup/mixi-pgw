//
// Created by mixi on 2017/06/22.
//
#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"


using namespace MIXIPGW_TOOLS;

DiameterClient::DiameterClient(){
    bzero(&addr_in_, sizeof(addr_in_));
    addr_in_len_    = 0;
    client_sock_    = 0;
    client_id_      = 0;
    counter_        = 0;
    client_ev_      = NULL;
    inst_           = NULL;
    mysql_          = NULL;
    origin_host_    = "";
    origin_realm_   = "";
}
DiameterClient::DiameterClient(const DiameterClient& cp){
    memcpy(&addr_in_, &cp.addr_in_, sizeof(addr_in_));
    addr_in_len_    = cp.addr_in_len_;
    client_sock_    = cp.client_sock_;
    client_id_      = cp.client_id_;
    counter_        = cp.counter_;
    client_ev_      = cp.client_ev_;
    inst_           = cp.inst_;
    mysql_          = cp.mysql_;
    origin_host_    = cp.origin_host_;
    origin_realm_   = cp.origin_realm_;
}
DiameterClient::DiameterClient(int client_sock,struct sockaddr_in* addr_in,unsigned int addr_in_len){
    client_sock_    = client_sock;
    addr_in_len_    = addr_in_len;
    if (addr_in){ addr_in_ = (*addr_in); }
    client_ev_      = NULL;
    mysql_          = NULL;
    origin_host_    = "";
    origin_realm_   = "";
    client_id_      = 0;
    counter_        = 0;
}
DiameterClient::~DiameterClient(){}

void DiameterClient::Response(MIXIPGW_TOOLS::diameter_header_ptr h, const std::string& sessid,const unsigned retcd){
    char        rbf[1024] = {0};
    auto        dh = (diameter_header_ptr)rbf;
    memcpy(dh, h, sizeof(*dh));
    //
    dh->version     = DIAMETER_VERSION;
    dh->f.flag.r    = DIAMETER_OFF;
    dh->f.flag.e    = (retcd==DIAMETER_SUCCESS?DIAMETER_OFF:DIAMETER_ON);

    auto payload    = (char*)(dh+1);
    uint32_t len    = sizeof(*dh);
    // session id , origin host, origin realm
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_SESSION_ID, sessid);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_HOST, origin_host_);
    COPY_AVP_STRING(&payload, &len, DIAMETER_AVPCODE_ORIGIN_REALM, origin_realm_);
    COPY_AVP_UINT32(&payload, &len, DIAMETER_AVPCODE_RESULT_CODE, retcd);
    dh->len     = BE24R(len);
    //
    if (Send(dh, len) <= 0){
        Logger::LOGERR("failed.DiameterClient::Response(%p:%d)",dh, len);
    }
}

int DiameterClient::Send(void* data, int len){
    int		ret      = 0L;
    int		allsend  = 0L;
    int		sendlen  = 0L;
    int		errored  = RETOK;
    int		retrycnt = 0L;
    char*	senddata = (char*)data;
    fd_set	sendfd;
    struct timeval  timeout;

    while(1){
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100000;
        FD_ZERO(&sendfd);
        FD_SET(client_sock_,&sendfd);
        ret = select(client_sock_ + 1,NULL,&sendfd,NULL,&timeout);
        //retry count is over.
        if (retrycnt ++ > 10){
            errored = 1;
            break;
        }
        if (ret == -1){			//select failed.
            if (errno == EINTR){	continue;	}
            errored = 1;
            break;
        }else if(ret > 0){		//sended. any bytes data..
            if(FD_ISSET(client_sock_,&sendfd)){
                sendlen = send(client_sock_,(senddata + allsend),(len - allsend),0);
                if (sendlen < 0){
                    errored = 1;
                    break;
                }
                allsend += sendlen;
                if (allsend >= len){	break;	}
            }
        }else{
            continue;
        }
    }
    return(errored == RETOK?allsend:-1);
}
