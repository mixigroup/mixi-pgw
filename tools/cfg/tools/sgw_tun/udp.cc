#include "sgw_tun_def.h"
#include "sgw_tun_ext.h"

#include "lib/const/gtpu.h"

//
void* udp_run(void* arg){
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    struct timeval tv{1,0};
    assert(p);
    p->gtpc_state = GTPC_STATE_DC;
    p->udp_fd = udp_open(p);
    p->udpc_fd= udpc_open(p);
    assert(p->udp_fd >= 0);
    assert(p->udpc_fd >= 0);

    printf(">>udp loop(%d/%p/%d/%s/%d)\n", getpid(), (void*)pthread_self(), p->udp_fd, p->local_addr,p->local_port);
    //
    while(!QUIT()){
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(p->udp_fd, &rfds);
        FD_SET(p->udpc_fd, &rfds);
        auto maxfd = MAX(p->udp_fd, p->udpc_fd);
        auto r = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (r<0){
            if (errno == EINTR){ continue; }
            printf("err(%s/%d)\n", strerror(errno), errno);
            break;
        }else if (r == 0){
            if (p->gtpc_state == GTPC_STATE_DC){
                gtpc_create_session_req(p);
            }else if (p->gtpc_state == GTPC_STATE_CREATE_SESS_RES){
                gtpc_modify_bearer_req(p);
            }
            continue; // timeout
        }
        // gtpu
        if (FD_ISSET(p->udp_fd, &rfds)) {
            char bf[ETHER_MAX_LEN];
            auto l = read(p->udp_fd,(void*)&bf[0],(ETHER_MAX_LEN-0));
            if (l > 0) {
                auto gh = (MIXIPGW_TOOLS::gtpu_header_ptr)bf;
                auto ofset = sizeof(*gh) + (gh->u.flags&GTPU_FLAGMASK?4:0);
                auto s = write(p->tun_fd, &bf[ofset], (l-ofset));
                if (s > 0) {
                    p->size_udp_to_tun += s;
                } else {
                    fprintf(stderr, "failed. write(%d:%s)\n", errno, strerror(errno));
                }
            }
        }
        // gtpc
        if (FD_ISSET(p->udpc_fd, &rfds)){
            char bf[ETHER_MAX_LEN];
            auto l = read(p->udpc_fd,(void*)&bf,ETHER_MAX_LEN);
            if (l > 0) {
                gtpc_parse_res(bf, l, p);
            } 
        }
    }
    return(NULL);
}

int udp_open(void* arg) {
    int fd = -1;
    int on = 1;
    struct sockaddr_in addr;
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    // local
    inet_pton(AF_INET, p->local_addr, &addr.sin_addr.s_addr);
    addr.sin_port = htons(p->local_port);
    addr.sin_family = AF_INET;
    //
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    assert(fd>=0);
    assert(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))>=0);
    assert(bind(fd, (struct sockaddr *)&addr, sizeof(addr))!=-1);
    assert(fcntl(fd, F_SETFL, O_NONBLOCK)!=-1);
//  assert(connect(fd, (struct sockaddr *)&addr_p, sizeof(addr_p))!=-1);
    //
    return(fd);
}

int udpc_open(void* arg) {
    int fd = -1;
    int on = 1;
    struct sockaddr_in addr, peer;
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    // local
    inet_pton(AF_INET, p->local_addr, &addr.sin_addr.s_addr);
    addr.sin_port = htons(GTPC_PORT);
    addr.sin_family = AF_INET;
    //
    bzero(&peer, sizeof(peer));
    peer.sin_family = AF_INET;
    assert(inet_pton(AF_INET, p->peer_addr_dst, &peer.sin_addr.s_addr));
    peer.sin_port = htons(GTPC_PORT);
    peer.sin_family = AF_INET;
    //
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    assert(fd>=0);
    assert(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))>=0);
    assert(bind(fd, (struct sockaddr *)&addr, sizeof(addr))!=-1);
    assert(fcntl(fd, F_SETFL, O_NONBLOCK)!=-1);
    assert(connect(fd, (struct sockaddr *)&peer, sizeof(peer))!=-1);
    //
    return(fd);
}
