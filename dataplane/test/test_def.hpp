#ifndef MIXI_PGW_DPDK_TEST_DEF_HPP_HPP
#define MIXI_PGW_DPDK_TEST_DEF_HPP_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_malloc.h>
#include <rte_kni.h>

#include "core.hpp"
#include "mixi_pgw_data_plane_def.hpp"
#include <assert.h>

#ifndef MAX_PACKET_SZ
#define MAX_PACKET_SZ           2048
#endif
#define MBUF_DATA_SZ            (MAX_PACKET_SZ + RTE_PKTMBUF_HEADROOM)
#ifdef __TEST_MODE__
#define NB_MBUF                 (8192 * 4)
#define PKT_BURST_SZ            4
#else
#define NB_MBUF                 (8192 * 16)
#define PKT_BURST_SZ            32
#endif
#define MEMPOOL_CACHE_SZ        PKT_BURST_SZ
#define TEST_DELAY              10000
#define TEST_COUNT              100

#define TEST_BUFFER_SZ          (64)
#define TEST_BUFFER_M_SZ        (128)

#define COREID_INGRESS          (0)
#define COREID_EGRESS           (COREID_INGRESS+1)
#define COREID_INTERNAL         (COREID_INGRESS+2)

#define IP_DEFTTL               64
#define IP_VERSION              0x40
#define IP_HDRLEN               0x05
#define IP_VHL_DEF (IP_VERSION | IP_HDRLEN)
#define IP_DN_FRAGMENT_FLAG     0x0040

#endif //MIXI_PGW_DPDK_TEST_DEF_HPP_HPP
