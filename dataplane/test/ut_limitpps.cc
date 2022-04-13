#include "test_def.hpp"

#define TUNNEL_FIXED_SRCIP  (0x0a0b0c0d)

int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {(RTE_LOG_DEBUG+1),};
static rte_atomic32_t       __stop = RTE_ATOMIC32_INIT(0);
static void signal_handler(int s){ rte_atomic32_inc(&__stop); }

static int main_loop(__rte_unused void *arg){
    int32_t f_stop;
    const unsigned lcore_id = rte_lcore_id();
    uint64_t    cur_cycle = rte_rdtsc();
    uint64_t    t_cycle;
    const uint64_t drain_tsc = (rte_get_tsc_hz() + 1000000 - 1) / 1000000 * 1000000;

    static uint64_t counter_ = 0;

    //
    if (lcore_id == 0) {
        while (1) {
            f_stop = rte_atomic32_read(&__stop);
            if (f_stop){ break; }

            t_cycle = rte_rdtsc();
            if ((cur_cycle + drain_tsc) < t_cycle){
                //
                printf("..(%u .. %llu .. %llu)\n", (unsigned)drain_tsc, t_cycle, cur_cycle);

                cur_cycle = t_cycle;
            }
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    auto ret = rte_eal_init(argc, argv);
    if (ret < 0){
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    }
    rte_eal_mp_remote_launch(main_loop, NULL, CALL_MASTER);
    unsigned lcore;
    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0){
            return(-1);
        }
    }
    return(0);
}
