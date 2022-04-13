//
// Created by mixi on 2017/04/28.
//
#include "../mixipgw_mod_sgw_def.hpp"
#include <iostream>


uint64_t SgwSim::WaitForInterval(struct timeval *prev, struct timeval *cur, int report_interval){
    struct timeval delta;
    delta.tv_sec = report_interval/1000;
    delta.tv_usec = (report_interval%1000)*1000;
    if (select(0, NULL, NULL, NULL, &delta) < 0 && errno != EINTR) {
        perror("select");
        abort();
    }
    gettimeofday(cur, NULL);
    timersub(cur, prev, &delta);
    return delta.tv_sec* 1000000 + delta.tv_usec;
}


int SgwSim::Input(const char* msg){
    fprintf(stdout, "%s", msg);
    int c = 0;
    fscanf(stdin,"%d", &c);
    return(c);
}
//
std::tuple<int, int, int> SgwSim::PrintMenu(int cplvl, int dplvl, const std::string text[]){
    int type = 0, key = 0;
    std::string menu = " \n\
+==============================================================+\n\
| sgw traffic emulator (mixipgw)         %24s|\n\
+==============================================================+\n\
| performance counter                                          |\n\
+------------+-------------------------------------------------+\n\
| send(ctrl) | %48s|\n\
+------------+-------------------------------------------------+\n\
| send(data) | %48s|\n\
+------------+-------------------------------------------------+\n\
| rcv (ctrl) | %48s|\n\
+------------+-------------------------------------------------+\n\
| rcv (data) | %48s|\n\
+------------+-------------------------------------------------+\n\
| echo(c)    | %48s|\n\
+------------+-------------------------------------------------+\n\
| echo(u)    | %48s|\n\
+------------+-------------------------------------------------+\n\
| delses     | %48s|\n\
+------------+-------------------------------------------------+\n\
| modbearer  | %48s|\n\
+------------+-------------------------------------------------+\n\
| delbearer  | %48s|\n\
+------------+-------------------------------------------------+\n\
| suspend    | %48s|\n\
+------------+-------------------------------------------------+\n\
| resume     | %48s|\n\
+==============================================================+\n\
| configuration                                                |\n\
+------------+-----+--------------------------------+----+-----+\n\
| pgw (gtpc) | host|%-32s|port|%5u|\n\
+------------+-----+--------------------------------+----+-----+\n\
| pgw (gtpu) | host|%-32s|port|%5u|\n\
+------------+-----+--------------------------------+----+-----+\n\
| sgw (ctrl) | host|%-32s|port|%5u|\n\
+------------+-----+--------------------------------+----+-----+\n\
| sgw (data) | host|%-32s|port|%5u|\n\
+==============================================================+\n\
| current level                                                |\n\
+--------------------+-----------------------------------------+\n\
| 0123456789ABCDEFGHI|create session level(0 < I high)         |\n\
+--------------------+                                         |\n\
| %19s|                                         |\n\
+--------------------+-----------------------------------------+\n\
| 0123456789ABCDEFGHI|gtpu data level(0 < I high)              |\n\
+--------------------+                                         |\n\
| %19s|                                         |\n\
+==============================================================+\n\
| accept operation                                             |\n\
+---+---+------------------------------------------------------+\n\
|   | 8 |      8 : create session level up                     |\n\
+---+---+---+  2 : create session level down                   |\n\
| 4 |   | 6 |                                                  |\n\
+---+---+---+                                                  |\n\
|   | 2 |      6 : gtpu data level up                          |\n\
+   +---+      4 : gtpu data level down                        |\n\
|                                                              |\n\
+------------+-------------------------------------------------+\n\
| q          | stop this program.                              |\n\
+------------+-------------------------------------------------+\n\
| z          | create session level to zero.                   |\n\
+------------+-------------------------------------------------+\n\
\n\
\n\
$$ ";
    char lvlbf[LEVEL_MAX+2] = {0},lvlbf2[LEVEL_MAX+2] = {0};
    char bf[4096] = {0};
    for(auto n = 0;n < (LEVEL_MAX+1);n++){
        lvlbf[n]  = (n == cplvl?'^':' ');
        lvlbf2[n] = (n == dplvl?'^':' ');
    }
    snprintf(bf, sizeof(bf)-1,menu.c_str(),
             __DATE__,
             text[SGWSIM::GTPC_SEND_CREATE_SESSION].c_str(),
             text[SGWSIM::GTPU_SEND].c_str(),
             text[SGWSIM::GTPC_RECV].c_str(),
             text[SGWSIM::GTPU_RECV].c_str(),
             text[SGWSIM::GTPC_SEND_ECHO].c_str(),
             text[SGWSIM::GTPU_SEND_ECHO].c_str(),
             text[SGWSIM::GTPC_SEND_DELETE_SESSION].c_str(),
             text[SGWSIM::GTPC_SEND_MODIFY_BEARER].c_str(),
             text[SGWSIM::GTPC_SEND_DELETE_BEARER].c_str(),
             text[SGWSIM::GTPC_SEND_RESUME_NOTIFICATION].c_str(),
             text[SGWSIM::GTPC_SEND_SUSPEND_NOTIFICATION].c_str(),
             pgw_gtpc_.host_.c_str(),pgw_gtpc_.port_,
             pgw_gtpu_.host_.c_str(),pgw_gtpu_.port_,
             sgw_emuc_.host_.c_str(),sgw_emuc_.port_,
             sgw_emuu_.host_.c_str(),sgw_emuu_.port_,
             lvlbf, lvlbf2);
    auto k    = Input(bf);
    //
    if (k == 8){
        cplvl++;
        key = MIN(cplvl,LEVEL_MAX);
        type = SGWSIM::GTPC_SEND_CREATE_SESSION;
    }else if (k =='z'){
        cplvl = 0;
        key = 0;
        type = SGWSIM::GTPC_SEND_CREATE_SESSION;
    }else if (k == 2){
        cplvl--;
        key = MAX(cplvl,0);
        type = SGWSIM::GTPC_SEND_CREATE_SESSION;
    }else if (k == 6){
        dplvl++;
        key = MIN(dplvl,LEVEL_MAX);
        type = SGWSIM::GTPU_SEND;
    }else if (k == 4){
        dplvl--;
        key = MAX(dplvl,0);
        type = SGWSIM::GTPU_SEND;
    }else if (k == 'q'){
        fprintf(stderr, "quit\n");
        return(std::tuple<int,int,int>(RETERR,0,0));
    }
    return(std::tuple<int,int,int>(RETOK, type, key));
}
