#include "strs.h"

unsigned GLOBAL_THREAD_NUM;
//
thread_arg_ptr     GLOBAL_THREAD_ARG_00;
thread_arg_ptr     GLOBAL_THREAD_ARG_01;
thread_arg_ptr     GLOBAL_THREAD_ARG_02;
thread_arg_ptr     GLOBAL_THREAD_ARG_03;
thread_arg_ptr     GLOBAL_THREAD_ARG_04;
thread_arg_ptr     GLOBAL_THREAD_ARG_05;
thread_arg_ptr     GLOBAL_THREAD_ARG_06;
thread_arg_ptr     GLOBAL_THREAD_ARG_07;

thread_arg_ptr     GLOBAL_THREAD_ARG_08;
thread_arg_ptr     GLOBAL_THREAD_ARG_09;
thread_arg_ptr     GLOBAL_THREAD_ARG_10;
thread_arg_ptr     GLOBAL_THREAD_ARG_11;
thread_arg_ptr     GLOBAL_THREAD_ARG_12;
thread_arg_ptr     GLOBAL_THREAD_ARG_13;
thread_arg_ptr     GLOBAL_THREAD_ARG_14;
thread_arg_ptr     GLOBAL_THREAD_ARG_15;
// global (ptrlocal) variables
static system_arg_t GLOBAL_ARG;



// --------------
// main entory
// --------------
int main(int argc, char **argv){
    LOG("start storess.");

    GLOBAL_THREAD_ARG_00 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_01 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_02 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_03 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_04 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_05 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_06 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_07 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_08 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_09 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_10 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_11 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_12 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_13 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_14 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));
    GLOBAL_THREAD_ARG_15 = (thread_arg_ptr)malloc(sizeof(thread_arg_t));

    //
    init_globals(argc, argv, &GLOBAL_ARG);
    init_randidx();
    open_netmap(&GLOBAL_ARG);
    // [4] start threads
    start_thread(&GLOBAL_ARG);
    // start observer thread
    if (pthread_create(&GLOBAL_ARG.observer_thread, NULL, strs_observer,&GLOBAL_ARG)<0){
        ERR("faied. pthread create..");
        exit(0);
    } 
    // wait for all threads.
    wait_thread(&GLOBAL_ARG);
    LOG("complete storess.");

    return(0);
}
void usage(void){
    const char *cmd = "strs";
    fprintf(stderr,
            "Usage:\n"
            "%s arguments\n"
            "\t-i interface interface name(ex.netmap:eth4-0)\n"
            "\t-f function  tx/rx/rtx\n"
            "\t-r tx_rate\n"
            "\t-b burst size\n"
            "\t-c affinity start idx\n"
            "\t-t thread num\n"
            "\t-D dstmac\n"
            "\t-S srcmac\n"
            "\t-l packet length\n"
            "\t-v vlanid\n"
            "\t-q sequencial access/!= random(default)\n"
            "\t-R ip range\n"
            "",
            cmd);
    exit(0);
}
unsigned THREAD_NUM(void){
    return(GLOBAL_THREAD_NUM);
}
void SET_THREAD_NUM(unsigned tnum){
    GLOBAL_THREAD_NUM = tnum;
}
thread_arg_ptr ref_thread_arg(int id){
    switch(id){
    case 0: return(GLOBAL_THREAD_ARG_00);
    case 1: return(GLOBAL_THREAD_ARG_01);
    case 2: return(GLOBAL_THREAD_ARG_02);
    case 3: return(GLOBAL_THREAD_ARG_03);
    case 4: return(GLOBAL_THREAD_ARG_04);
    case 5: return(GLOBAL_THREAD_ARG_05);
    case 6: return(GLOBAL_THREAD_ARG_06);
    case 7: return(GLOBAL_THREAD_ARG_07);

    case 8: return(GLOBAL_THREAD_ARG_08);
    case 9: return(GLOBAL_THREAD_ARG_09);
    case 10:return(GLOBAL_THREAD_ARG_10);
    case 11:return(GLOBAL_THREAD_ARG_11);
    case 12:return(GLOBAL_THREAD_ARG_12);
    case 13:return(GLOBAL_THREAD_ARG_13);
    case 14:return(GLOBAL_THREAD_ARG_14);
    case 15:return(GLOBAL_THREAD_ARG_15);
    default: return(NULL);
    }
}
void sigint_h(int sig) {
    int n;
    UNUSED(sig);
    LOG("received control-C on thread %p", (void *)pthread_self());
    for (n = 0; n < THREAD_NUM(); n++) {
        if (ref_thread_arg(n)!=NULL){
            ref_thread_arg(n)->cancel=1;
        }
    }
}
