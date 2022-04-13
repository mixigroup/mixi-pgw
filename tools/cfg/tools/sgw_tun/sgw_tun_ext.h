#ifndef SGW_EMU_SGW_EMU_EXT_H
#define SGW_EMU_SGW_EMU_EXT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
int utun_open(void*);
int udp_open(void*);
int udpc_open(void*);
// 
int gtpc_create_session_req(void*);
int gtpc_modify_bearer_req(void*);
int gtpc_parse_res(char*,size_t,void*);

//
void* utun_run(void*);
void* udp_run(void*);

int QUIT_INCR(void);
int QUIT(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SGW_EMU_SGW_EMU_EXT_H




