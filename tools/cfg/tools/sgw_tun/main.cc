#include "sgw_tun_def.h"
#include "sgw_tun_ext.h"

static sgw_tun_container_t container;
static int quit = 0;
//
void sigint_h(int sig) {
    QUIT_INCR();
}
int QUIT_INCR(void){
    return(quit++);
}
int QUIT(void){
    return(quit);
}

//
int main(int argc, char* argv[]) {
    //
    if (argc != 9){
        assert(!"Usage: [this program] <dev> <udpip> <udpport> <teid> <remotehost_dst> <imsi/ex:40101234567890> <msisdn/ex:818012345678> <ueipv> " );
    }
    bzero(&container, sizeof(container));
    strncpy(container.device_name, argv[1], MIN(strlen(argv[1]), sizeof(container.device_name)));
    strncpy(container.local_addr, argv[2], MIN(strlen(argv[2]), sizeof(container.local_addr)));
    container.local_port = (unsigned short)atoi(argv[3]);
    container.teid = (unsigned short)atoi(argv[4]);
    strncpy(container.peer_addr_dst, argv[5], MIN(strlen(argv[5]), sizeof(container.peer_addr_dst)));

    container.imsi = strtoull(argv[6], NULL, 10);
    container.msisdn = strtoull(argv[7], NULL, 10);
    strncpy(container.ueipv, argv[8], MIN(strlen(argv[8]), sizeof(container.ueipv)));

    // start thread.
    if (pthread_create(&container.tun_thread, NULL, utun_run, &container)) {
        assert(!"pthread_create..(run tun loop.)");
    }
    if (pthread_create(&container.udp_thread, NULL, udp_run, &container)) {
        assert(!"pthread_create..(run udp loop.)");
    }
    // main loop.
    sleep(1);

    printf(">>>started. sgw emu.(%d/%d)\n", container.tun_fd, container.udp_fd);
    //

    signal(SIGINT, sigint_h);

    // wait sig
    printf("\n+----------+----------+\n");
    while(!QUIT()){
        sleep(3);
        printf("| %08x | %08x |", (unsigned int)container.size_tun_to_udp, (unsigned int)container.size_udp_to_tun);
        printf("\n+----------+----------+\n");
    }
    pthread_join(container.tun_thread, NULL);
    pthread_join(container.udp_thread, NULL);

    // cleanup
    close(container.tun_fd);
    close(container.udp_fd);
    //
    printf("<<< finish. sgw emu\n");
    return 0;
}
//




