/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_rx.c
    @brief      packet receive event
*******************************************************************************
*******************************************************************************
    @date       created(28/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 28/nov/2017 
      -# Initial Version
******************************************************************************/
#include "../pgw.h"

/**
  PGWevent : Rx\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   data    receive buffer
 @param[in]   datalen receive buffer長(=packet 長)
 @param[in]   saddr   server address
 @param[in]   saddrlen address length
 @param[in]   caddr   client address
 @param[in]   caddrlen address length
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_rx(handle_ptr pinst, const U8* data, const INT datalen,  struct sockaddr_in* saddr, ssize_t saddrlen, struct sockaddr_in* caddr, ssize_t caddrlen, void* ext){
    RETCD       ret=ERR,idx;
    server_ptr  pserver = NULL;
    packet_ptr  pkt = NULL;
    pserver = (server_ptr)ext;
    // not yet, all threads not been started
    if ((pinst->flags&ON)!=ON){
        return(ERR);
    }

//  PGW_LOG("event_rx..(%p : %p)\n", pserver, (void*)pthread_self());

    // receive packet(gtp[c/u]) from socket descriptor
    // callback in libevent context
    if (pgw_alloc_packet(&pkt, datalen) != OK){
        pgw_panic("failed. memory allocate.");
    }
    memcpy(pkt->data, data, datalen);
    memcpy(&pkt->saddr, saddr, sizeof(*saddr));
    memcpy(&pkt->caddr, caddr, sizeof(*caddr));
    pkt->saddrlen   = saddrlen;
    pkt->caddrlen   = caddrlen;

    // gtpu or gtpc
    if (pserver->port == GTPU1_PORT){
        if (gtpu_header(pkt)->v1_flags.version == GTPU_VERSION_1 ){
            if (gtpu_header(pkt)->type == GTPU_ECHO_REQ || gtpu_header(pkt)->type == GTPU_ECHO_RES){
                if ((ret = pgw_enq(pinst->nodes[PGW_NODE_GTPU_ECHO_REQ], INGRESS, pkt)) != OK){
                    pgw_panic("failed. enq(gtpu echo req/res).");
                }
            }else{
                if ((ret = pgw_enq(pinst->nodes[PGW_NODE_GTPU_OTHER_REQ], INGRESS, pkt)) != OK){
                    pgw_panic("failed. enq(gtpu other).");
                }
            }
        }else{
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. gtpu version\n");
        }
    }else if(pserver->port == GTPC_PORT && gtpc_header(pkt)->v2_flags.version == GTPC_VERSION_2){
        if (gtpc_header(pkt)->v2_flags.version == GTPC_VERSION_2){
            if (gtpc_header(pkt)->type == GTPC_ECHO_REQ || gtpc_header(pkt)->type == GTPC_ECHO_RES){
                if ((ret = pgw_enq(pinst->nodes[PGW_NODE_GTPC_ECHO_REQ], INGRESS, pkt)) != OK){
                    pgw_panic("failed. enq(gtpc echo req/res).");
                }
            }else if (gtpc_header(pkt)->type == GTPC_CREATE_SESSION_REQ){
                idx = PGW_NODE_GTPC_CREATE_SESSION_REQ + (gtpc_header(pkt)->q.seqno%
                                                          (PGW_NODE_GTPC_CREATE_SESSION_REQ_MAX-PGW_NODE_GTPC_CREATE_SESSION_REQ));
                if ((ret = pgw_enq(pinst->nodes[idx], INGRESS, pkt)) != OK){
                    pgw_panic("failed. enq(gtpc create session).");
                }
            }else if (gtpc_header(pkt)->type == GTPC_MODIFY_BEARER_REQ){
                if ((ret = pgw_enq(pinst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], INGRESS, pkt)) != OK){
                    pgw_panic("failed. enq(gtpc modify bearer).");
                }
            }else if (gtpc_header(pkt)->type == GTPC_DELETE_SESSION_REQ ||
                      gtpc_header(pkt)->type == GTPC_DELETE_BEARER_REQ ||
                      gtpc_header(pkt)->type == GTPC_SUSPEND_NOTIFICATION ||
                      gtpc_header(pkt)->type == GTPC_RESUME_NOTIFICATION){
                if ((ret = pgw_enq(pinst->nodes[PGW_NODE_GTPC_OTHER_REQ], INGRESS, pkt)) != OK){
                    pgw_panic("failed. enq(gtpc delete-session/delete-bearer/suspend/resume).");
                }
            }
        }else{
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. gtpc version\n");
        }
    }else if(pserver->port == GTPC_PORT && gtpc_v1_header(pkt)->f.gtpc_v1_flags.version == GTPC_VERSION_1){
        // Protocol Type:1/SequenceNumberFlag:1/N-PDU:0
        // ---
        // http://www.qtc.jp/3GPP/Specs/29274-920.pdf
        // 8.2 Usage of the GTP-C Header
        if ((gtpc_v1_header(pkt)->f.flags&GTPC_V1_NSE_MASK)){
            if ((ret = pgw_enq(pinst->nodes[PGW_NODE_GTPC_OTHER_REQ], INGRESS, pkt)) != OK){
                pgw_panic("failed. enq(gtpc version 1).");
            }
        }
    }else{
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid port.. or version(%u)\n", pserver->port);
    }
    // not applicable(exept GTPU/GTPC) packet
    if (ret != OK){
        pgw_free_packet(pkt);
    }
    return(ret);
}
