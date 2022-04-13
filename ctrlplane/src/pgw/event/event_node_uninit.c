/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       event_node_uninit.c
    @brief      cleanpu node
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
  event : cleanup node\n
 *******************************************************************************
  cleanup it every node\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_node_uninit(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    sgw_peers_ptr peers = (sgw_peers_ptr)pnode->peers_table;
    PGW_LOG(PGW_LOG_LEVEL_INF, "event_node_uninit..(%p : %p/%u)\n", pnode, (void*)pthread_self(), pnode->index);
    //
    if (pnode->index == PGW_NODE_TIMER){
        if (pnode->node_opt){
            timernode_ext_ptr tnep = (timernode_ext_ptr)pnode->node_opt;
            if (tnep->stmt){
                DBPROVIDER_STMT_CLOSE(tnep->stmt);
            }
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
#ifndef SINGLE_CREATE_SESSION
    }else if (pnode->index == PGW_NODE_GTPC_CREATE_SESSION_REQ ||
              pnode->index == PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0){
#else
        }else if (pnode->index == PGW_NODE_GTPC_CREATE_SESSION_REQ){
#endif
        if (pnode->node_opt){
            create_session_node_ext_ptr tnep = (create_session_node_ext_ptr)pnode->node_opt;
            if (tnep->stmt){
                DBPROVIDER_STMT_CLOSE(tnep->stmt);
            }
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
    }else if (pnode->index == PGW_NODE_GTPC_MODIFY_BEARER_REQ){
        if (pnode->node_opt){
            modify_bearer_node_ext_ptr tnep = (modify_bearer_node_ext_ptr)pnode->node_opt;
            if (tnep->stmt){
                DBPROVIDER_STMT_CLOSE(tnep->stmt);
            }
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
    }else if (pnode->index == PGW_NODE_GTPC_OTHER_REQ){
        if (pnode->node_opt){
            delete_session_node_ext_ptr tnep = (delete_session_node_ext_ptr)pnode->node_opt;
            if (tnep->stmt){
                DBPROVIDER_STMT_CLOSE(tnep->stmt);
            }
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
    }else if (pnode->index == PGW_NODE_GTPU_OTHER_REQ){
        if (pnode->node_opt){
            gtpu_other_node_ext_ptr tnep = (gtpu_other_node_ext_ptr)pnode->node_opt;
            if (tnep->stmt){
                DBPROVIDER_STMT_CLOSE(tnep->stmt);
            }
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;

    }else if (pnode->index == PGW_NODE_TX){
        if (pnode->node_opt){
            txnode_ext_ptr tnep = (txnode_ext_ptr)pnode->node_opt;
            close(tnep->sock_gtpc);
            close(tnep->sock_gtpu);
            //
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
    }
    if (peers){
        pgw_free_sgw_peers(&peers);
    }

    return(ERR);
}
