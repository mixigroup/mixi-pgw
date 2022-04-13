#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>

// 
int main(int argc, char* argv[]){
    FILE *fp;
    int  irqn;
    char line[2048],smpa[256];

    // part of nic interface name
    if (argc != 2){
        fprintf(stderr, "<thispg> <interface>\n");
        exit(1);
    }
    printf("start proc %s\n",basename(argv[0]));
    fp = fopen("/proc/interrupts","r");
    if (fp == NULL){
        fprintf(stderr, "cannot open /proc/interrupts : %s", strerror(errno));
        exit(1);
    }
    memset(line,0,sizeof(line));
    //
    while(fgets(line, sizeof(line)-1,fp)!=NULL){
        char *quenm = NULL;
        // judgement nic. 
        if ((quenm = strstr(line, argv[1]))!= NULL){
            if (sscanf(line, "%d:", &irqn) == 1){
                FILE* fpsmp;
                // check those smp_affinity of irq. 
                snprintf(smpa, sizeof(smpa)-1,"/proc/irq/%d/smp_affinity",irqn);

                if ((fpsmp = fopen(smpa,"w+")) != NULL){
                    char lowbf[32] = {0}, highbf[32]={0};
                    if (fscanf(fpsmp, "%[^,]%s", highbf, lowbf) == 2){
                        unsigned cpu_l = strtoul(highbf,NULL,16);
                        unsigned cpu_h = strtoul(&lowbf[1], NULL,16);
                        char bf1[64],bf2[64],quen[32];
                        bf1[0] = bf2[0] = quen[0] = 0;
                        printf("current smpaffinity %s (%x-%x)\n", quenm, cpu_l,cpu_h);

                        if (sscanf(quenm, "%[^-]-%[^-]-%s[^\n]",bf1,bf2,quen) == 3){
                            char cpubf[64] = {0};
                            if (atoi(quen) >= 32){
                                snprintf(cpubf,sizeof(cpubf)-1,"%06x,00000000", (1<<(atoi(quen)-32)));
                            }else{
                                snprintf(cpubf,sizeof(cpubf)-1,"000000,%08x", (1<<(atoi(quen))));
                            }
                            printf("%d: %s old:%u,%u/ new cpu => %d..%s\n", irqn, quenm, cpu_l,cpu_h, atoi(quen), cpubf);
                            fputs(cpubf, fpsmp);
                        }
                    }
                    fclose(fpsmp);
                }
            }
        }
        memset(line,0,sizeof(line));
    }
    fclose(fp);

    return(0);
}

