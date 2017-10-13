/*
 ****************************************************************************
 * stats.h
 *      stats handling definitions and macros.
 *
 * (C) 2016 by Alexey V. Lesovsky (lesovsky <at> gmail.com)
 * 
 ****************************************************************************
 */
#ifndef __STATS_H__
#define __STATS_H__

#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <time.h>
#include "pgf.h"
#include "common.h"

#define LOADAVG_FILE            "/proc/loadavg"
#define STAT_FILE               "/proc/stat"
#define UPTIME_FILE             "/proc/uptime"
#define MEMINFO_FILE            "/proc/meminfo"
#define DISKSTATS_FILE          "/proc/diskstats"
#define NETDEV_FILE             "/proc/net/dev"

#define MAXDEV_IN_FILE      64          /* max devices per stats files */
#define DEFAULT_HZ          100         /* default clock ticks */

/*
 * Macros used to display statistics values.
 * NB: Define SP_VALUE() to normalize to %;
 */
#define SP_VALUE(m,n,p) (((double) ((n) - (m))) / (p) * 100)
#define S_VALUE(m,n,p,q) (((double) ((n) - (m))) / (p) * q)

/* struct which used for cpu statistic */
struct cpu_s {
    unsigned long long cpu_user;
    unsigned long long cpu_nice;
    unsigned long long cpu_sys;
    unsigned long long cpu_idle;
    unsigned long long cpu_iowait;
    unsigned long long cpu_steal;
    unsigned long long cpu_hardirq;
    unsigned long long cpu_softirq;
    unsigned long long cpu_guest;
    unsigned long long cpu_guest_nice;
};

#define STATS_CPU_SIZE (sizeof(struct cpu_s))

/* struct which used for memory statistics */
struct mem_s {
    unsigned long long mem_total;
    unsigned long long mem_free;
    unsigned long long mem_used;
    unsigned long long swap_total;
    unsigned long long swap_free;
    unsigned long long swap_used;
    unsigned long long cached;
    unsigned long long buffers;
    unsigned long long dirty;
    unsigned long long writeback;
    unsigned long long slab;
};

#define STATS_MEM_SIZE (sizeof(struct mem_s))

/* struct which used for io statistics */
struct iodata_s
{
    int major;
    int minor;
    char devname[S_BUF_LEN];
    unsigned long r_completed;          /* reads completed successfully */
    unsigned long r_merged;             /* reads merged */
    unsigned long r_sectors;            /* sectors read */
    unsigned long r_spent;              /* time spent reading (ms) */
    unsigned long w_completed;          /* writes completed */
    unsigned long w_merged;             /* writes merged */
    unsigned long w_sectors;            /* sectors written */
    unsigned long w_spent;              /* time spent writing (ms) */
    unsigned long io_in_progress;       /* I/Os currently in progress */
    unsigned long t_spent;              /* time spent doing I/Os (ms) */
    unsigned long t_weighted;           /* weighted time spent doing I/Os (ms) */
    double arqsz;                       /* average request size */
    double await;                       /* latency */
    double util;                        /* device utilization */
};
#define STATS_IODATA_SIZE (sizeof(struct iodata_s))
#define BLKDEV     1

/* This may be defined by <linux/ethtool.h> */
#ifndef DUPLEX_UNKNOWN
#define DUPLEX_UNKNOWN          0xff
#endif /* DUPLEX_UNKNOWN */

/* struct for network interface's data (settings and stats) */
struct ifdata_s
{
    char ifname[IF_NAMESIZE + 1];
    long speed;
    int duplex;
    unsigned long rbytes;
    unsigned long rpackets;
    unsigned long ierr;
    unsigned long wbytes;
    unsigned long wpackets;
    unsigned long oerr;
    unsigned long coll;
    unsigned long sat;
};
#define STATS_IFDATA_SIZE (sizeof(struct ifdata_s))
#define NETDEV     2

/* init/free stuff */
void init_stats(struct cpu_s *st_cpu[], struct mem_s **st_mem_short);
void init_iostat(struct tab_s * tabs[], int index);
void init_ifstat(struct tab_s * tabs[], int index);
void free_iostat(struct tab_s * tabs[], int index);
void free_ifstat(struct tab_s * tabs[], int index);

/* load average stats */
float * get_local_loadavg();
float * get_remote_loadavg(PGconn * conn);

/* cpu stat functions */
void read_local_cpu_stat(struct cpu_s *st_cpu, unsigned int nbr,
        unsigned long long *uptime, unsigned long long *uptime0);
void read_remote_cpu_stat(struct cpu_s *st_cpu, unsigned int nbr,
        unsigned long long *uptime, unsigned long long *uptime0, PGconn * conn);
void write_cpu_stat_raw(WINDOW * window, struct cpu_s *st_cpu[],
        unsigned int curr, unsigned long long itv);

/* mem/swap stat functions */
void read_mem_stat(struct mem_s *st_mem_short);
void read_remote_mem_stat(struct mem_s *st_mem_short, PGconn * conn);
void write_mem_stat(WINDOW * window, struct mem_s *st_mem_short);

/* iostat functions */
void replace_iostat(struct iodata_s * curr[], struct iodata_s * prev[], int bdev);
void read_local_diskstats(WINDOW * window, struct iodata_s * curr[], int bdev, bool * repaint);
void read_remote_diskstats(WINDOW * window, struct iodata_s * curr[], int bdev, PGconn * conn, bool * repaint);
void write_iostat(WINDOW * window, struct iodata_s * curr[], struct iodata_s * prev[], 
        int bdev, unsigned long long itv, int sys_hz);

/* nicstat functions */
void get_speed_duplex(struct ifdata_s * ifdata, bool conn_local, PGconn * conn);
void replace_ifdata(struct ifdata_s *curr[], struct ifdata_s *prev[], int idev);
void read_local_netdev(WINDOW * window, struct ifdata_s *curr[], bool * repaint);
void read_remote_netdev(WINDOW * window, struct ifdata_s *curr[], int idev, PGconn * conn, bool * repaint);
void write_nicstats(WINDOW * window, struct ifdata_s *curr[], struct ifdata_s *prev[], int idev, unsigned long long itv, int sys_hz);

/* others */
void read_local_uptime(unsigned long long *uptime, struct tab_s * tab);
void read_remote_uptime(unsigned long long *uptime, struct tab_s * tab, PGconn * conn);
double ll_sp_value(unsigned long long value1, unsigned long long value2, unsigned long long itv);
unsigned long long get_interval(unsigned long long prev_uptime, unsigned long long curr_uptime);
int count_devices(int type, bool conn_local, PGconn * conn);
void get_time(char * strtime);

#endif /* __STATS_H__ */
