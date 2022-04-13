#include "sgw_tun_def.h"
#include "sgw_tun_ext.h"
//
#include "lib/misc.hpp"
#include "lib/packet/gtpc.hpp"
#include "lib/packet/gtpc_items.hpp"
#include <iostream>
#include <memory>

using namespace MIXIPGW_TOOLS;

int gtpc_create_session_req(void* arg){
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    unsigned char ipv4[4] = {0};
    unsigned int  ipv4len = sizeof(ipv4);

    if (p->gtpc_state != GTPC_STATE_DC){ return(-1); }

    printf("create session..\n");

    Misc::GetIpv4(p->local_addr, (uint8_t*)ipv4, &ipv4len);
    //
    GtpcPkt req(GTPC_CREATE_SESSION_REQ);
    req.append(Imsi(p->imsi));
    req.append(Msisdn(p->msisdn));
    req.append(Rat(GTPC_RAT_TYPE_EUTRAN));
    req.append(Fteid(GTPC_INSTANCE_ORDER_0,GTPC_FTEIDIF_S5S8_SGW_GTPC,p->teid,(uint8_t*)ipv4,NULL));
    req.append(Apn(CREATE_SESSION_APN));
    gtpc_numberdigit_t num[3] = {{4,4},{0,0},{0,1}};
    req.append(ServingNetwork(num));
    req.append(Pdn(1));
    req.append(SelectionMode(2));
    __u8 paa[4] = {0x0a, 0x80, 0x00, 0x03};
    req.append(Paa(0x01, paa, sizeof(paa)));

    // BearerContext
    BearerContext   req_bctx;
    req_bctx.append(Ebi(GTPC_INSTANCE_ORDER_0,(uint8_t)(CREATE_SESSION_EBI)));
    req_bctx.append(Fteid(GTPC_INSTANCE_ORDER_2,GTPC_FTEIDIF_S5S8_SGW_GTPU,p->teid+1,(uint8_t*)ipv4,NULL));
    req_bctx.append(Bqos(0,9,0,0,0,0));
    req.append(req_bctx);
    //
    req.append(Ambr(CREATE_SESSION_AMBR_UP,CREATE_SESSION_AMBR_DOWN));
    //
    auto rbuf = (gtpc_header_ptr)req.ref();
    auto rlen = req.len();

    rbuf->t.teid = 0;
    rbuf->q.seqno = 0;
    //
    auto s = sendto(p->udpc_fd, req.ref(), rlen, 0, NULL, 0);
    if (s != rlen){
        fprintf(stderr, "failed.sendto (%d:%s)", errno, strerror(errno));
    }

    // next state.
    p->gtpc_state = GTPC_STATE_CREATE_SESS_REQ;

    return(0);
}


int gtpc_modify_bearer_req(void* arg){
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    if (p->gtpc_state != GTPC_STATE_CREATE_SESS_RES){ return(-1); }

    // 
    printf("modify session..\n");
    //
        


    // next state.
    p->gtpc_state = GTPC_STATE_MODIFY_BEARER_REQ;

    return(0);
}

int gtpc_parse_res(char* pkt,size_t len,void* arg){
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    printf("parse gtpc(%d - %d)..\n", (int)len, p->gtpc_state);

    if (p->gtpc_state == GTPC_STATE_CREATE_SESS_REQ){       
        std::auto_ptr<GtpcPkt> req(GtpcPkt::attach(pkt, len));
        if (req.get() == NULL){
            printf("invalid packet format..\n");
            return(-1);
        }
        std::auto_ptr<BearerContext> bctx(req.get()->find<BearerContext>());
        if (!bctx.get()){
           // this is echo req
           return(-1);
        }
        GtpcPkt* pbctx = bctx.get()->child();
        std::auto_ptr<Fteid> fteidu(pbctx->find<Fteid>());
        //
        if (!fteidu.get()){
            return(-1);
        }
        // gtpu teid
        p->sgwfteid = (unsigned int)fteidu.get()->teid();
        p->sgwfteid = htonl(p->sgwfteid);
        // gtpu ip
        strncpy(p->sgwgtpuipv, fteidu.get()->ipv().c_str(), MIN(strlen(fteidu.get()->ipv().c_str()), sizeof(p->sgwgtpuipv)-1));

        printf("sgw gtpu teid(%u - %s)\n",p->sgwfteid , p->sgwgtpuipv);
        struct sockaddr_in peer;
        bzero(&peer, sizeof(peer));
        peer.sin_family = AF_INET;
        assert(inet_pton(AF_INET, p->sgwgtpuipv, &peer.sin_addr.s_addr));
        peer.sin_port = htons(GTPU_PORT);
        peer.sin_family = AF_INET;

        memcpy(&p->pgwpeer, &peer, sizeof(peer));
        // next state.
        p->gtpc_state = GTPC_STATE_CREATE_SESS_RES;
    }else if (p->gtpc_state == GTPC_STATE_MODIFY_BEARER_REQ){
    }

    return(0);
}

