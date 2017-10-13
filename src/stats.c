// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 ****************************************************************************
 * stats.c
 *      stats handling functions.
 *
 * (C) 2016 by Alexey V. Lesovsky (lesovsky <at> gmail.com)
 * 
 ****************************************************************************
 */
#include <linux/ethtool.h>
#include "include/stats.h"

/*
 ****************************************************************************
 * Allocate memory for cpu and mem statistics structs.
 ****************************************************************************
 */
void init_stats(struct cpu_s *st_cpu[], struct mem_s **st_mem_short)
{
    unsigned int i;
    /* Allocate structures for CPUs "all" and 0 */
    for (i = 0; i < 2; i++) {
        if ((st_cpu[i] = (struct cpu_s *) malloc(STATS_CPU_SIZE * 2)) == NULL) {
            mreport(true, msg_fatal, "FATAL: malloc for cpu stats failed.\n");
        }
        memset(st_cpu[i], 0, STATS_CPU_SIZE * 2);
    }

    /* Allocate structures for memory */
    if ((*st_mem_short = (struct mem_s *) malloc(STATS_MEM_SIZE)) == NULL) {
        mreport(true, msg_fatal, "FATAL: malloc for memory stats failed.\n");
    }
    memset(*st_mem_short, 0, STATS_MEM_SIZE);
}

/*
 ****************************************************************************
 * Allocate memory for IO statistics structs.
 ****************************************************************************
 */
void init_iostat(struct tab_s * tabs[], int index)
{
    int start, end, i, j;
    
    /* init structs for all tabs if index < 0, otherwise only for a particular tab */
    index < 0 ? (start = 0, end = MAX_TABS) : (start = index, end = index + 1);
    
    /* go through the tabs */
    for (i = start; i < end; i++) {
        /* go through the iostats array entries */
        for (j = 0; j < tabs[i]->sys_special.bdev; j++) {
            if ((tabs[i]->curr_iostat[j] = (struct iodata_s *) malloc(STATS_IODATA_SIZE)) == NULL ||
                (tabs[i]->prev_iostat[j] = (struct iodata_s *) malloc(STATS_IODATA_SIZE)) == NULL) {
                mreport(true, msg_fatal, "FATAL: malloc for iostat failed.\n");
            }
            memset(tabs[i]->curr_iostat[j], 0, STATS_IODATA_SIZE);
            memset(tabs[i]->prev_iostat[j], 0, STATS_IODATA_SIZE);
        }
    }
}

/*
 ****************************************************************************
 * Free memory consumed by IO statistics structs.
 ****************************************************************************
 */
void free_iostat(struct tab_s * tabs[], int index)
{
    int start, end, i, j;

    /* init structs for all tabs if index < 0, otherwise only for particular tab */
    index < 0 ? (start = 0, end = MAX_TABS) : (start = index, end = index + 1);

    /* go through the tabs */
    for (i = start; i < end; i++) {
        /* go through the iostats array entries */
        for (j = 0; j < tabs[i]->sys_special.bdev; j++) {
            free(tabs[i]->curr_iostat[j]);
            free(tabs[i]->prev_iostat[j]);
        }
    }
}

/*
 ****************************************************************************
 * Allocate memory for network interfaces statistics structs.
 ****************************************************************************
 */
void init_ifstat(struct tab_s * tabs[], int index)
{
    int start, end, i, j;
    
    /* init structs for all tabs if index < 0, otherwise only for a particular tab */
    index < 0 ? (start = 0, end = MAX_TABS) : (start = index, end = index + 1);
    
    /* go through the tabs */
    for (i = start; i < end; i++) {
        /* go through the iostats array entries */
        for (j = 0; j < tabs[i]->sys_special.idev; j++) {
            if ((tabs[i]->curr_ifstat[j] = (struct ifdata_s *) malloc(STATS_IFDATA_SIZE)) == NULL ||
                (tabs[i]->prev_ifstat[j] = (struct ifdata_s *) malloc(STATS_IFDATA_SIZE)) == NULL) {
                mreport(true, msg_fatal, "FATAL: malloc for ifstat failed.\n");
            }
            memset(tabs[i]->curr_ifstat[j], 0, STATS_IFDATA_SIZE);
            memset(tabs[i]->prev_ifstat[j], 0, STATS_IFDATA_SIZE);
            
            /* initialize interfaces with unknown speed and duplex */
            tabs[i]->curr_ifstat[j]->speed = -1;
            tabs[i]->curr_ifstat[j]->duplex = DUPLEX_UNKNOWN;
        }
    }
}

/*
 ****************************************************************************
 * Free memory consumed by network interfaces statistics structs.
 ****************************************************************************
 */
void free_ifstat(struct tab_s * tabs[], int index)
{
    int start, end, i, j;

    /* init structs for all tabs if index < 0, otherwise only for particular tab */
    index < 0 ? (start = 0, end = MAX_TABS) : (start = index, end = index + 1);

    /* go through the tabs */
    for (i = start; i < end; i++) {
        /* go through the iostats array entries */
        for (j = 0; j < tabs[i]->sys_special.idev; j++) {
            free(tabs[i]->curr_ifstat[j]);
            free(tabs[i]->prev_ifstat[j]);
        }
    }
}

/*
 ****************************************************************************
 * Get system clock resolution.
 ****************************************************************************
 */
void get_HZ(struct tab_s * tab, PGconn * conn)
{
    long ticks;
    static char errmsg[ERRSIZE];
    PGresult * res;
    
    if (tab->conn_local) {
        if ((ticks = sysconf(_SC_CLK_TCK)) == -1)
            mreport(false, msg_error, "ERROR: sysconf failure.\n");        
    } else {
        if ((res = do_query(conn, PG_SYS_GET_CLK_QUERY, errmsg)) != NULL) {
            ticks = atol(PQgetvalue(res, 0, 0));
            PQclear(res);
        } else {
            ticks = DEFAULT_HZ;             /* can't get ticks, use defaults */
        }
    }
           
    tab->sys_special.sys_hz = (unsigned int) ticks;
}

/*
 ****************************************************************************
 * Count specified devices, block devices or network interfaces.
 ****************************************************************************
 */
int count_devices(int type, bool conn_local, PGconn * conn)
{
    int ndev = 0;
    FILE * fp;
    char ch;
    static char statfile[PATH_MAX], query[QUERY_MAXLEN], errmsg[ERRSIZE];
    PGresult * res;

    switch (type) {
        case BLKDEV:
            snprintf(statfile, PATH_MAX, "%s", DISKSTATS_FILE);
            snprintf(query, QUERY_MAXLEN, "%s", PG_SYS_PROC_BDEV_CNT_QUERY);
        break;
        case NETDEV:
            snprintf(statfile, PATH_MAX, "%s", NETDEV_FILE);
            snprintf(query, QUERY_MAXLEN, "%s", PG_SYS_PROC_IFDEV_CNT_QUERY);
        break;
    }

    if (conn_local) {
        if ((fp = fopen(statfile, "r")) == NULL) {
            return 0;              /* can't get stats */
        }
        /* count number of lines */
        while (!feof(fp)) {
            ch = fgetc(fp);
            if (ch == '\n')
                ndev++;
        }
        fclose(fp);
    } else {
        if ((res = do_query(conn, query, errmsg)) != NULL && PQntuples(res) > 0) {
            ndev = atoi(PQgetvalue(res, 0, 0));
            PQclear(res);
        } else {
            return 0;              /* can't get stats */
        }
    }

    return ndev;
}

/*
 ****************************************************************************
 * Read /proc/loadavg and return load average values.
 ****************************************************************************
 */
float * get_local_loadavg()
{
    static float la[3];
    FILE *fp;

    if ((fp = fopen(LOADAVG_FILE, "r")) != NULL) {
        if ((fscanf(fp, "%f %f %f", &la[0], &la[1], &la[2])) != 3)
            la[0] = la[1] = la[2] = 0;            /* something goes wrong */
        fclose(fp);
    } else {
        la[0] = la[1] = la[2] = 0;                /* can't read statfile */
    }

    return la;
}

/*
 ****************************************************************************
 * Read remote /proc/loadavg via sql and return load average values.
 ****************************************************************************
 */
float * get_remote_loadavg(PGconn * conn)
{
    static char errmsg[ERRSIZE];
    static float la[3];
    PGresult * res;

    if ((res = do_query(conn, PG_SYS_PROC_LOADAVG_QUERY, errmsg)) != NULL
        && PQntuples(res) > 0) {
        la[0] = atof(PQgetvalue(res, 0, 0));
        la[1] = atof(PQgetvalue(res, 0, 1));
        la[2] = atof(PQgetvalue(res, 0, 2));
        PQclear(res);
    } else {
        la[0] = la[1] = la[2] = 0;                /* can't read statfile */
    }
    
    return la;
}

/*
 ****************************************************************************
 * Workaround for CPU counters read from /proc/stat: Dyn-tick kernels
 * have a race issue that can make those counters go backward.
 ****************************************************************************
 */
double ll_sp_value(unsigned long long value1, unsigned long long value2,
                unsigned long long itv)
{
    if (value2 < value1)
        return (double) 0;
    else
        return SP_VALUE(value1, value2, itv);
}

/*
 ****************************************************************************
 * Read machine uptime independently of the number of processors.
 ****************************************************************************
 */
void read_local_uptime(unsigned long long *uptime, struct tab_s * tab)
{
    FILE *fp;
    unsigned long up_sec, up_cent;
    int sys_hz = tab->sys_special.sys_hz;

    if ((fp = fopen(UPTIME_FILE, "r")) != NULL) {
        if ((fscanf(fp, "%lu.%lu", &up_sec, &up_cent)) != 2) {
	    fclose(fp);
	    return;
    	}
    } else
        return;

    *uptime = (unsigned long long) up_sec * sys_hz + (unsigned long long) up_cent * sys_hz / 100;
    fclose(fp);
}

/*
 ****************************************************************************
 * Read machine uptime using sql interface.
 ****************************************************************************
 */
void read_remote_uptime(unsigned long long *uptime, struct tab_s * tab, PGconn * conn)
{
    static char errmsg[ERRSIZE];
    double up_full, up_sec, up_cent;
    int sys_hz = tab->sys_special.sys_hz;
    PGresult * res;

    if ((res = do_query(conn, PG_SYS_PROC_UPTIME_QUERY, errmsg)) != NULL
        && PQntuples(res) > 0) {
        up_full = atof(PQgetvalue(res, 0, 0));
        PQclear(res);
    } else
        return;                /* can't read statfile */
    
    up_cent = modf(up_full, &up_sec);
    up_cent = up_cent * 100;
    *uptime = (unsigned long long) up_sec * sys_hz + (unsigned long long) up_cent * sys_hz / 100;
}

/*
 ****************************************************************************
 * Read cpu statistics from /proc/stat. Also calculate uptime if 
 * read_*_uptime() function return NULL.
 *
 * IN:
 * @st_cpu          Struct where stat will be saved.
 * @nbr             Total number of CPU (including cpu "all").
 *
 * OUT:
 * @st_cpu          Struct with statistics.
 * @uptime          Machine uptime multiplied by the number of processors.
 * @uptime0         Machine uptime. Filled only if previously set to zero.
 ****************************************************************************
 */
void read_local_cpu_stat(struct cpu_s *st_cpu, unsigned int nbr,
                            unsigned long long *uptime, unsigned long long *uptime0)
{
    FILE *fp;
    struct cpu_s *st_cpu_i;
    struct cpu_s sc;
    char line[XXXL_BUF_LEN];
    unsigned int proc_nb;

    if ((fp = fopen(STAT_FILE, "r")) == NULL) {
        /* zeroing stats if stats read failed */
        memset(st_cpu, 0, STATS_CPU_SIZE);
        return;
    }

    while ( (fgets(line, sizeof(line), fp)) != NULL ) {
        if (!strncmp(line, "cpu ", 4)) {
            memset(st_cpu, 0, STATS_CPU_SIZE);
            sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                            &st_cpu->cpu_user,      &st_cpu->cpu_nice,
                            &st_cpu->cpu_sys,       &st_cpu->cpu_idle,
                            &st_cpu->cpu_iowait,    &st_cpu->cpu_hardirq,
                            &st_cpu->cpu_softirq,   &st_cpu->cpu_steal,
                            &st_cpu->cpu_guest,     &st_cpu->cpu_guest_nice);
                            *uptime = st_cpu->cpu_user + st_cpu->cpu_nice +
                                st_cpu->cpu_sys + st_cpu->cpu_idle +
                                st_cpu->cpu_iowait + st_cpu->cpu_steal +
                                st_cpu->cpu_hardirq + st_cpu->cpu_softirq +
                                st_cpu->cpu_guest + st_cpu->cpu_guest_nice;
        } else if (!strncmp(line, "cpu", 3)) {
            if (nbr > 1) {
                memset(&sc, 0, STATS_CPU_SIZE);
                sscanf(line + 3, "%u %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                                &proc_nb,           &sc.cpu_user,
                                &sc.cpu_nice,       &sc.cpu_sys,
                                &sc.cpu_idle,       &sc.cpu_iowait,
                                &sc.cpu_hardirq,    &sc.cpu_softirq,
                                &sc.cpu_steal,      &sc.cpu_guest,
                                &sc.cpu_guest_nice);

                                if (proc_nb < (nbr - 1)) {
                                    st_cpu_i = st_cpu + proc_nb + 1;
                                    *st_cpu_i = sc;
                                }

                                if (!proc_nb && !*uptime0) {
                                    *uptime0 = sc.cpu_user + sc.cpu_nice   +
                                    sc.cpu_sys     + sc.cpu_idle   +
                                    sc.cpu_iowait  + sc.cpu_steal  +
                                    sc.cpu_hardirq + sc.cpu_softirq;
                                    printf("read_cpu_stat: uptime0 = %llu\n", *uptime0);
                                }
            }
        }
    }
    fclose(fp);
}

/*
 ****************************************************************************
 * Read cpu statistics from /proc/stat using sql interface. 
 * Also calculate uptime if read_*_uptime() function return NULL.
 ****************************************************************************
 */
void read_remote_cpu_stat(struct cpu_s *st_cpu, unsigned int nbr,
                            unsigned long long *uptime, unsigned long long *uptime0, PGconn * conn)
{
    struct cpu_s *st_cpu_i;
    struct cpu_s sc;
    unsigned int proc_nb;
    static char errmsg[ERRSIZE];
    PGresult * res_cpu_total;
    PGresult * res_cpu_part;

    if ((res_cpu_total = do_query(conn, PG_SYS_PROC_TOTAL_CPU_STAT_QUERY, errmsg)) != NULL
        && PQntuples(res_cpu_total) > 0) {
        memset(st_cpu, 0, STATS_CPU_SIZE);
        /* fill st_cpu */
        st_cpu->cpu_user = atoll(PQgetvalue(res_cpu_total,0,1));
        st_cpu->cpu_nice = atoll(PQgetvalue(res_cpu_total,0,2));
        st_cpu->cpu_sys = atoll(PQgetvalue(res_cpu_total,0,3));
        st_cpu->cpu_idle = atoll(PQgetvalue(res_cpu_total,0,4));
        st_cpu->cpu_iowait = atoll(PQgetvalue(res_cpu_total,0,5));
        st_cpu->cpu_hardirq = atoll(PQgetvalue(res_cpu_total,0,6));
        st_cpu->cpu_softirq = atoll(PQgetvalue(res_cpu_total,0,7));
        st_cpu->cpu_steal = atoll(PQgetvalue(res_cpu_total,0,8));
        st_cpu->cpu_guest = atoll(PQgetvalue(res_cpu_total,0,9));
        st_cpu->cpu_guest_nice = atoll(PQgetvalue(res_cpu_total,0,10));
        *uptime = st_cpu->cpu_user + st_cpu->cpu_nice +
                  st_cpu->cpu_sys + st_cpu->cpu_idle +
                  st_cpu->cpu_iowait + st_cpu->cpu_steal +
                  st_cpu->cpu_hardirq + st_cpu->cpu_softirq +
                  st_cpu->cpu_guest + st_cpu->cpu_guest_nice;
        PQclear(res_cpu_total);
    } else if ((res_cpu_part = do_query(conn, PG_SYS_PROC_PART_CPU_STAT_QUERY, errmsg)) != NULL
        && PQntuples(res_cpu_part) > 0) {
        if (nbr > 1) {
            memset(&sc, 0, STATS_CPU_SIZE);
            /* fill st_cpu */
            proc_nb = atoi(PQgetvalue(res_cpu_total,0,0));
            st_cpu->cpu_user = atoll(PQgetvalue(res_cpu_total,0,2));
            st_cpu->cpu_nice = atoll(PQgetvalue(res_cpu_total,0,3));
            st_cpu->cpu_sys = atoll(PQgetvalue(res_cpu_total,0,4));
            st_cpu->cpu_idle = atoll(PQgetvalue(res_cpu_total,0,5));
            st_cpu->cpu_iowait = atoll(PQgetvalue(res_cpu_total,0,6));
            st_cpu->cpu_hardirq = atoll(PQgetvalue(res_cpu_total,0,7));
            st_cpu->cpu_softirq = atoll(PQgetvalue(res_cpu_total,0,8));
            st_cpu->cpu_steal = atoll(PQgetvalue(res_cpu_total,0,9));
            st_cpu->cpu_guest = atoll(PQgetvalue(res_cpu_total,0,10));
            st_cpu->cpu_guest_nice = atoll(PQgetvalue(res_cpu_total,0,11));
            
            if (proc_nb < (nbr - 1)) {
                st_cpu_i = st_cpu + proc_nb + 1;
                *st_cpu_i = sc;
            }

            if (!proc_nb && !*uptime0) {
                *uptime0 = sc.cpu_user + sc.cpu_nice   +
                        sc.cpu_sys     + sc.cpu_idle   +
                        sc.cpu_iowait  + sc.cpu_steal  +
                        sc.cpu_hardirq + sc.cpu_softirq;
                printf("read_cpu_stat: uptime0 = %llu\n", *uptime0);
            }
        }
        PQclear(res_cpu_part);
    }
}

/*
 ****************************************************************************
 * Compute time interval.
 ****************************************************************************
 */
unsigned long long get_interval(unsigned long long prev_uptime,
                                        unsigned long long curr_uptime)
{
    unsigned long long itv;
    
    /* first run prev_uptime=0 so displaying stats since system startup */
    itv = curr_uptime - prev_uptime;

    if (!itv) {     /* Paranoia checking */
        itv = 1;
    }

    return itv;
}

/*
 ****************************************************************************
 * Display cpu statistics in specified window.
 ****************************************************************************
 */
void write_cpu_stat_raw(WINDOW * window, struct cpu_s *st_cpu[],
                unsigned int curr, unsigned long long itv)
{
    wprintw(window, 
            "    %%cpu: %4.1f us, %4.1f sy, %4.1f ni, %4.1f id, %4.1f wa, %4.1f hi, %4.1f si, %4.1f st\n",
            ll_sp_value(st_cpu[!curr]->cpu_user, st_cpu[curr]->cpu_user, itv),
            ll_sp_value(st_cpu[!curr]->cpu_sys + st_cpu[!curr]->cpu_softirq + st_cpu[!curr]->cpu_hardirq,
            st_cpu[curr]->cpu_sys + st_cpu[curr]->cpu_softirq + st_cpu[curr]->cpu_hardirq, itv),
            ll_sp_value(st_cpu[!curr]->cpu_nice, st_cpu[curr]->cpu_nice, itv),
            (st_cpu[curr]->cpu_idle < st_cpu[!curr]->cpu_idle) ?
            0.0 :
            ll_sp_value(st_cpu[!curr]->cpu_idle, st_cpu[curr]->cpu_idle, itv),
            ll_sp_value(st_cpu[!curr]->cpu_iowait, st_cpu[curr]->cpu_iowait, itv),
            ll_sp_value(st_cpu[!curr]->cpu_hardirq, st_cpu[curr]->cpu_hardirq, itv),
            ll_sp_value(st_cpu[!curr]->cpu_softirq, st_cpu[curr]->cpu_softirq, itv),
            ll_sp_value(st_cpu[!curr]->cpu_steal, st_cpu[curr]->cpu_steal, itv));
    wrefresh(window);
}

/*
 ****************************************************************************
 * Read /proc/meminfo and save results into struct.
 ****************************************************************************
 */
void read_mem_stat(struct mem_s *st_mem_short)
{
    FILE *mem_fp;
    char buffer[XXXL_BUF_LEN];
    char key[M_BUF_LEN];
    unsigned long long value;
    
    if ((mem_fp = fopen(MEMINFO_FILE, "r")) != NULL) {
        while (fgets(buffer, XXXL_BUF_LEN, mem_fp) != NULL) {
            sscanf(buffer, "%s %llu", key, &value);
            if (!strcmp(key,"MemTotal:"))
                st_mem_short->mem_total = value / 1024;
            else if (!strcmp(key,"MemFree:"))
                st_mem_short->mem_free = value / 1024;
            else if (!strcmp(key,"SwapTotal:"))
                st_mem_short->swap_total = value / 1024;
            else if (!strcmp(key,"SwapFree:"))
                st_mem_short->swap_free = value / 1024;
            else if (!strcmp(key,"Cached:"))
                st_mem_short->cached = value / 1024;
            else if (!strcmp(key,"Dirty:"))
                st_mem_short->dirty = value / 1024;
            else if (!strcmp(key,"Writeback:"))
                st_mem_short->writeback = value / 1024;
            else if (!strcmp(key,"Buffers:"))
                st_mem_short->buffers = value / 1024;
            else if (!strcmp(key,"Slab:"))
                st_mem_short->slab = value / 1024;
        }
        st_mem_short->mem_used = st_mem_short->mem_total - st_mem_short->mem_free
            - st_mem_short->cached - st_mem_short->buffers - st_mem_short->slab;
        st_mem_short->swap_used = st_mem_short->swap_total - st_mem_short->swap_free;

        fclose(mem_fp);
    } else {
        /* failed to read /proc/meminfo, zeroing stats */
        st_mem_short->mem_total = st_mem_short->mem_free = st_mem_short->mem_used = 0;
        st_mem_short->cached = st_mem_short->buffers = st_mem_short->slab = 0;
        st_mem_short->swap_total = st_mem_short->swap_free = st_mem_short->swap_used = 0;
        st_mem_short->dirty = st_mem_short->writeback = 0;
    }
}

/*
 ****************************************************************************
 * Read remote /proc/meminfo via sql and save results into struct.
 ****************************************************************************
 */
void read_remote_mem_stat(struct mem_s *st_mem_short, PGconn * conn)
{
    static char errmsg[ERRSIZE];
    char * tmp;                     /* for strtoull() */
    PGresult * res;

    if ((res = do_query(conn, PG_SYS_PROC_MEMINFO_QUERY, errmsg)) != NULL
        && PQntuples(res) > 0) {
        if (!strcmp(PQgetvalue(res,0,0),"Buffers:"))
            st_mem_short->buffers = strtoull(PQgetvalue(res,0,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,1,0),"Cached:"))
            st_mem_short->cached = strtoull(PQgetvalue(res,1,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,2,0),"Dirty:"))
            st_mem_short->dirty = strtoull(PQgetvalue(res,2,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,3,0),"MemFree:"))
            st_mem_short->mem_free = strtoull(PQgetvalue(res,3,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,4,0),"MemTotal:"))
            st_mem_short->mem_total = strtoull(PQgetvalue(res,4,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,5,0),"Slab:"))
            st_mem_short->slab = strtoull(PQgetvalue(res,5,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,6,0),"SwapFree:"))
            st_mem_short->swap_free = strtoull(PQgetvalue(res,6,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,7,0),"SwapTotal:"))
            st_mem_short->swap_total = strtoull(PQgetvalue(res,7,1), &tmp, 10) / 1024;
        if (!strcmp(PQgetvalue(res,8,0),"Writeback:"))
            st_mem_short->writeback = strtoull(PQgetvalue(res,8,1), &tmp, 10) / 1024;
        
        st_mem_short->mem_used = st_mem_short->mem_total - st_mem_short->mem_free
            - st_mem_short->cached - st_mem_short->buffers - st_mem_short->slab;
        st_mem_short->swap_used = st_mem_short->swap_total - st_mem_short->swap_free;
        
        PQclear(res);
    } else {
        /* can't read stats */
        st_mem_short->mem_total = st_mem_short->mem_free = st_mem_short->mem_used = 0;
        st_mem_short->cached = st_mem_short->buffers = st_mem_short->slab = 0;
        st_mem_short->swap_total = st_mem_short->swap_free = st_mem_short->swap_used = 0;
        st_mem_short->dirty = st_mem_short->writeback = 0;
    }
}

/*
 ****************************************************************************
 * Print content of mem struct to the ncurses window
 ****************************************************************************
 */
void write_mem_stat(WINDOW * window, struct mem_s *st_mem_short)
{
    wprintw(window, " MiB mem: %6llu total, %6llu free, %6llu used, %8llu buff/cached\n",
            st_mem_short->mem_total, st_mem_short->mem_free, st_mem_short->mem_used,
            st_mem_short->cached + st_mem_short->buffers + st_mem_short->slab);
    wprintw(window, "MiB swap: %6llu total, %6llu free, %6llu used, %6llu/%llu dirty/writeback\n",
            st_mem_short->swap_total, st_mem_short->swap_free, st_mem_short->swap_used,
            st_mem_short->dirty, st_mem_short->writeback);
}

/*
 ****************************************************************************
 * Save current io statistics snapshot.
 ****************************************************************************
 */
void replace_iostat(struct iodata_s *curr[], struct iodata_s *prev[], int bdev)
{
    int i;
    for (i = 0; i < bdev; i++) {
        prev[i]->r_completed = curr[i]->r_completed;
        prev[i]->r_merged = curr[i]->r_merged;
        prev[i]->r_sectors = curr[i]->r_sectors;
        prev[i]->r_spent = curr[i]->r_spent;
        prev[i]->w_completed = curr[i]->w_completed;
        prev[i]->w_merged = curr[i]->w_merged;
        prev[i]->w_sectors = curr[i]->w_sectors;
        prev[i]->w_spent = curr[i]->w_spent;
        prev[i]->io_in_progress = curr[i]->io_in_progress;
        prev[i]->t_spent = curr[i]->t_spent;
        prev[i]->t_weighted = curr[i]->t_weighted;
        prev[i]->arqsz = curr[i]->arqsz;
        prev[i]->await = curr[i]->await;
        prev[i]->util = curr[i]->util;
    }
}

/*
 ****************************************************************************
 * Get interface speed and duplex settings.
 * Interfaces with unknown speed and duplex will shown without %util values.
 ****************************************************************************
 */
void get_speed_duplex(struct ifdata_s * ifdata, bool conn_local, PGconn * conn)
{
    struct ifreq ifr;
    struct ethtool_cmd edata;
    int status, sock;
    char query[QUERY_MAXLEN], errmsg[ERRSIZE];
    PGresult * res;

    if (conn_local) {
        sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            return;
        }

        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifdata->ifname);
        ifr.ifr_data = (void *) &edata;
        edata.cmd = ETHTOOL_GSET;
        status = ioctl(sock, SIOCETHTOOL, &ifr);
        close(sock);
    
        if (status < 0) {
            return;
        }
        ifdata->speed = edata.speed * 1000000;
        ifdata->duplex = edata.duplex;
    } else {
        snprintf(query, QUERY_MAXLEN, "%s('%s')", PG_SYS_ETHTOOL_LINK_QUERY, ifdata->ifname);
        if ((res = do_query(conn, query, errmsg)) != NULL) {
            ifdata->speed = atoi(PQgetvalue(res, 0, 1)) * 1000000;
            ifdata->duplex = atoi(PQgetvalue(res, 0, 2));
            PQclear(res);
        }
    }
}

/*
 ****************************************************************************
 * Save current nicstat snapshot.
 ****************************************************************************
 */
void replace_ifdata(struct ifdata_s *curr[], struct ifdata_s *prev[], int idev)
{
    int i;
    for (i = 0; i < idev; i++) {
        prev[i]->rbytes = curr[i]->rbytes;
        prev[i]->rpackets = curr[i]->rpackets;
        prev[i]->wbytes = curr[i]->wbytes;
        prev[i]->wpackets = curr[i]->wpackets;
        prev[i]->ierr = curr[i]->ierr;
        prev[i]->oerr = curr[i]->oerr;
        prev[i]->coll = curr[i]->coll;
        prev[i]->sat = curr[i]->sat;
    }
}

/*
 ****************************************************************************
 * Print current time.
 ****************************************************************************
 */
void get_time(char * strtime)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(strtime, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

/*
 ****************************************************************************
 * Read /proc/diskstats and save stats.
 ****************************************************************************
 */
void read_local_diskstats(WINDOW * window, struct iodata_s *curr[], int bdev, bool * repaint)
{
    FILE *fp;
    char line[L_BUF_LEN];

    unsigned int major, minor;
    char devname[S_BUF_LEN];
    unsigned long r_completed, r_merged, r_sectors, r_spent,
                  w_completed, w_merged, w_sectors, w_spent,
                  io_in_progress, t_spent, t_weighted;
    int i = 0;
    
    /*
     * If /proc/diskstats read failed, fire up repaint flag.
     * Next when subtab repainting fails, subtab will be closed.
     */
    if ((fp = fopen(DISKSTATS_FILE, "r")) == NULL) {
        wclear(window);
        wprintw(window, "Do nothing. Can't open %s", DISKSTATS_FILE);
        *repaint = true;
        return;
    }

    while ((fgets(line, sizeof(line), fp) != NULL) && (i < bdev)) {
        sscanf(line, "%u %u %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                    &major, &minor, devname,
                    &r_completed, &r_merged, &r_sectors, &r_spent,
                    &w_completed, &w_merged, &w_sectors, &w_spent,
                    &io_in_progress, &t_spent, &t_weighted);
        curr[i]->major = major;
        curr[i]->minor = minor;
        snprintf(curr[i]->devname, S_BUF_LEN, "%s", devname);
        curr[i]->r_completed = r_completed;
        curr[i]->r_merged = r_merged;
        curr[i]->r_sectors = r_sectors;
        curr[i]->r_spent = r_spent;
        curr[i]->w_completed = w_completed;
        curr[i]->w_merged = w_merged;
        curr[i]->w_sectors = w_sectors;
        curr[i]->w_spent = w_spent;
        curr[i]->io_in_progress = io_in_progress;
        curr[i]->t_spent = t_spent;
        curr[i]->t_weighted = t_weighted;
        i++;
    }
    fclose(fp);
}

/*
 ****************************************************************************
 * Read /proc/diskstats via sql and save stats.
 ****************************************************************************
 */
void read_remote_diskstats(WINDOW * window, struct iodata_s * curr[], int bdev, PGconn * conn, bool * repaint)
{
    static char errmsg[ERRSIZE];
    char * tmp;                     /* for strtoull() */
    PGresult * res;
    int i;
    
    if ((res = do_query(conn, PG_SYS_PROC_DISKSTATS_QUERY, errmsg)) != NULL
        && PQntuples(res) > 0) {
        for (i = 0; i < bdev; i++) {
            curr[i]->major = strtoul(PQgetvalue(res, i, 0), &tmp, 10);
            curr[i]->minor = strtoul(PQgetvalue(res, i, 1), &tmp, 10);
            snprintf(curr[i]->devname, S_BUF_LEN, "%s", PQgetvalue(res, i, 2));
            curr[i]->r_completed = strtoul(PQgetvalue(res, i, 3), &tmp, 10);
            curr[i]->r_merged = strtoul(PQgetvalue(res, i, 4), &tmp, 10);
            curr[i]->r_sectors = strtoul(PQgetvalue(res, i, 5), &tmp, 10);
            curr[i]->r_spent = strtoul(PQgetvalue(res, i, 6), &tmp, 10);
            curr[i]->w_completed = strtoul(PQgetvalue(res, i, 7), &tmp, 10);
            curr[i]->w_merged = strtoul(PQgetvalue(res, i, 8), &tmp, 10);
            curr[i]->w_sectors = strtoul(PQgetvalue(res, i, 9), &tmp, 10);
            curr[i]->w_spent = strtoul(PQgetvalue(res, i, 10), &tmp, 10);
            curr[i]->io_in_progress = strtoul(PQgetvalue(res, i, 11), &tmp, 10);
            curr[i]->t_spent = strtoul(PQgetvalue(res, i, 12), &tmp, 10);
            curr[i]->t_weighted = strtoul(PQgetvalue(res, i, 13), &tmp, 10);
        }
        PQclear(res);
    } else {
        /*
         * If /proc/diskstats read failed, fire up repaint flag.
         * Next when repainting subtab fails, subtab will be closed.
         */
        wclear(window);
        wprintw(window, "Do nothing. Failed to get stats.");
        *repaint = true;
        return;
    }
}

/*
 ****************************************************************************
 * Calculate IO stats and print it out.
 ****************************************************************************
 */
void write_iostat(WINDOW * window, struct iodata_s * curr[], struct iodata_s * prev[], 
        int bdev, unsigned long long itv, int sys_hz)
{
    int i = 0;
    double r_await[bdev], w_await[bdev];
    
    for (i = 0; i < bdev; i++) {
        curr[i]->util = S_VALUE(prev[i]->t_spent, curr[i]->t_spent, itv, sys_hz);
        curr[i]->await = ((curr[i]->r_completed + curr[i]->w_completed) - (prev[i]->r_completed + prev[i]->w_completed)) ?
            ((curr[i]->r_spent - prev[i]->r_spent) + (curr[i]->w_spent - prev[i]->w_spent)) /
            ((double) ((curr[i]->r_completed + curr[i]->w_completed) - (prev[i]->r_completed + prev[i]->w_completed))) : 0.0;
        curr[i]->arqsz = ((curr[i]->r_completed + curr[i]->w_completed) - (prev[i]->r_completed + prev[i]->w_completed)) ?
            ((curr[i]->r_sectors - prev[i]->r_sectors) + (curr[i]->w_sectors - prev[i]->w_sectors)) /
            ((double) ((curr[i]->r_completed + curr[i]->w_completed) - (prev[i]->r_completed + prev[i]->w_completed))) : 0.0;

        r_await[i] = (curr[i]->r_completed - prev[i]->r_completed) ?
            (curr[i]->r_spent - prev[i]->r_spent) /
            ((double) (curr[i]->r_completed - prev[i]->r_completed)) : 0.0;
        w_await[i] = (curr[i]->w_completed - prev[i]->w_completed) ?
            (curr[i]->w_spent - prev[i]->w_spent) /
            ((double) (curr[i]->w_completed - prev[i]->w_completed)) : 0.0;
    }

    /* print headers */
    wclear(window);
    wattron(window, A_BOLD);
    wprintw(window, "\nDevice:           rrqm/s  wrqm/s      r/s      w/s    rMB/s    wMB/s avgrq-sz avgqu-sz     await   r_await   w_await   %%util\n");
    wattroff(window, A_BOLD);

    /* print statistics */
    for (i = 0; i < bdev; i++) {
        /* skip devices without iops */
        if (curr[i]->r_completed == 0 && curr[i]->w_completed == 0) {
            continue;
        }
        wprintw(window, "%6s:\t\t", curr[i]->devname);
        wprintw(window, "%8.2f%8.2f",
                S_VALUE(prev[i]->r_merged, curr[i]->r_merged, itv, sys_hz),
                S_VALUE(prev[i]->w_merged, curr[i]->w_merged, itv, sys_hz));
        wprintw(window, "%9.2f%9.2f",
                S_VALUE(prev[i]->r_completed, curr[i]->r_completed, itv, sys_hz),
                S_VALUE(prev[i]->w_completed, curr[i]->w_completed, itv, sys_hz));
        wprintw(window, "%9.2f%9.2f%9.2f%9.2f",
                S_VALUE(prev[i]->r_sectors, curr[i]->r_sectors, itv, sys_hz) / 2048,
                S_VALUE(prev[i]->w_sectors, curr[i]->w_sectors, itv, sys_hz) / 2048,
                curr[i]->arqsz,
                S_VALUE(prev[i]->t_weighted, curr[i]->t_weighted, itv, sys_hz) / 1000.0);
        wprintw(window, "%10.2f%10.2f%10.2f", curr[i]->await, r_await[i], w_await[i]);
        wprintw(window, "%8.2f", curr[i]->util / 10.0);
        wprintw(window, "\n");
    }
    wrefresh(window);
}

/*
 ****************************************************************************
 * Read /proc/net/dev and save stats.
 ****************************************************************************
 */
void read_local_netdev(WINDOW * window, struct ifdata_s *curr[], bool * repaint)
{
    FILE *fp;
    unsigned int i = 0, j = 0;
    char line[L_BUF_LEN];
    char ifname[IF_NAMESIZE + 1];
    unsigned long lu[16];
    
    /*
     * If read /proc/net/dev failed, fire up repaint flag.
     * Next when subtab repainting fails, subtab will be closed.
     */
    if ((fp = fopen(NETDEV_FILE, "r")) == NULL) {
        wclear(window);
        wprintw(window, "Do nothing. Can't open %s", NETDEV_FILE);
        *repaint = true;
        return;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (j < 2) {
            j++;
            continue;       /* skip headers */
        }
        sscanf(line, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                ifname,
             /* rbps    rpps    rerrs   rdrop   rfifo   rframe  rcomp   rmcast */
                &lu[0], &lu[1], &lu[2], &lu[3], &lu[4], &lu[5], &lu[6], &lu[7],
             /* wbps    wpps    werrs    wdrop    wfifo    wcoll    wcarrier wcomp */
                &lu[8], &lu[9], &lu[10], &lu[11], &lu[12], &lu[13], &lu[14], &lu[15]);
        snprintf(curr[i]->ifname, IF_NAMESIZE + 1, "%s", ifname);
        curr[i]->rbytes = lu[0];
        curr[i]->rpackets = lu[1];
        curr[i]->wbytes = lu[8];
        curr[i]->wpackets = lu[9];
        curr[i]->ierr = lu[2];
        curr[i]->oerr = lu[10];
        curr[i]->coll = lu[13];
        curr[i]->sat = lu[2];
        curr[i]->sat += lu[3];
        curr[i]->sat += lu[11];
        curr[i]->sat += lu[12];
        curr[i]->sat += lu[13];
        curr[i]->sat += lu[14];
        i++;
    }
    fclose(fp);
}

/*
 ****************************************************************************
 * Read remote /proc/net/dev via sql and save stats.
 ****************************************************************************
 */
void read_remote_netdev(WINDOW * window, struct ifdata_s *curr[], int idev, PGconn * conn, bool * repaint)
{
    static char errmsg[ERRSIZE];
    char * tmp;                     /* for strtoull() */
    PGresult * res;
    int i;
    
    if ((res = do_query(conn, PG_SYS_PROC_NETDEV_QUERY, errmsg)) != NULL
        && PQntuples(res) > 0) {
        for (i = 0; i < idev; i++) {
            snprintf(curr[i]->ifname, IF_NAMESIZE + 1, "%s", PQgetvalue(res, i, 0));
            curr[i]->rbytes = strtoul(PQgetvalue(res, i, 2), &tmp, 10);
            curr[i]->rpackets = strtoul(PQgetvalue(res, i, 3), &tmp, 10);
            curr[i]->wbytes = strtoul(PQgetvalue(res, i, 10), &tmp, 10);
            curr[i]->wpackets = strtoul(PQgetvalue(res, i, 11), &tmp, 10);
            curr[i]->ierr = strtoul(PQgetvalue(res, i, 4), &tmp, 10);
            curr[i]->oerr = strtoul(PQgetvalue(res, i, 12), &tmp, 10);
            curr[i]->coll = strtoul(PQgetvalue(res, i, 15), &tmp, 10);
            curr[i]->sat = strtoul(PQgetvalue(res, i, 4), &tmp, 10);
            curr[i]->sat += strtoul(PQgetvalue(res, i, 5), &tmp, 10);
            curr[i]->sat += strtoul(PQgetvalue(res, i, 13), &tmp, 10);
            curr[i]->sat += strtoul(PQgetvalue(res, i, 14), &tmp, 10);
            curr[i]->sat += strtoul(PQgetvalue(res, i, 15), &tmp, 10);
            curr[i]->sat += strtoul(PQgetvalue(res, i, 16), &tmp, 10);            
        }
        PQclear(res);
    } else {
        /*
         * If /proc/diskstats read failed, fire up repaint flag.
         * Next when repainting subtab fails, subtab will be closed.
         */
        wclear(window);
        wprintw(window, "Do nothing. Failed to get remote ifstats.");
        *repaint = true;
        return;
    }
}

/*
 ****************************************************************************
 * Compute NIC stats and print it out.
 ****************************************************************************
 */
void write_nicstats(WINDOW * window, struct ifdata_s *curr[], struct ifdata_s *prev[], int idev, unsigned long long itv, int sys_hz)
{
    /* print headers */
    wclear(window);
    wattron(window, A_BOLD);
    wprintw(window, "\n    Interface:   rMbps   wMbps    rPk/s    wPk/s     rAvs     wAvs     IErr     OErr     Coll      Sat   %%rUtil   %%wUtil    %%Util\n");
    wattroff(window, A_BOLD);

    double rbps, rpps, wbps, wpps, ravs, wavs, ierr, oerr, coll, sat, rutil, wutil, util;
    int i = 0;

    for (i = 0; i < idev; i++) {
        /* skip interfaces which never seen packets */
        if (curr[i]->rpackets == 0 && curr[i]->wpackets == 0) {
           continue;
        }

        rbps = S_VALUE(prev[i]->rbytes, curr[i]->rbytes, itv, sys_hz);
        wbps = S_VALUE(prev[i]->wbytes, curr[i]->wbytes, itv, sys_hz);
        rpps = S_VALUE(prev[i]->rpackets, curr[i]->rpackets, itv, sys_hz);
        wpps = S_VALUE(prev[i]->wpackets, curr[i]->wpackets, itv, sys_hz);
        ierr = S_VALUE(prev[i]->ierr, curr[i]->ierr, itv, sys_hz);
        oerr = S_VALUE(prev[i]->oerr, curr[i]->oerr, itv, sys_hz);
        coll = S_VALUE(prev[i]->coll, curr[i]->coll, itv, sys_hz);
        sat = S_VALUE(prev[i]->sat, curr[i]->sat, itv, sys_hz);

	/* if no data about pps, zeroing averages */
        (rpps > 0) ? ( ravs = rbps / rpps ) : ( ravs = 0 );
        (wpps > 0) ? ( wavs = wbps / wpps ) : ( wavs = 0 );

        /* Calculate utilization */
        if (curr[i]->speed > 0) {
            /*
             * The following have a mysterious "800",
             * it is 100 for the % conversion, and 8 for bytes2bits.
             */
            rutil = min(rbps * 800 / curr[i]->speed, 100);
            wutil = min(wbps * 800 / curr[i]->speed, 100);
            if (curr[i]->duplex == 1) {
                /* Full duplex */
                util = max(rutil, wutil);
            } else if (curr[i]->duplex == 0) {
                /* Half Duplex */
                util = min((rbps + wbps) * 800 / curr[i]->speed, 100);
            }
        } else {
            util = rutil = wutil = 0;
        }

        /* print statistics */
        wprintw(window, "%14s", curr[i]->ifname);
        wprintw(window, "%8.2f%8.2f", rbps / 1024 / 128, wbps / 1024 / 128);
        wprintw(window, "%9.2f%9.2f", rpps, wpps);
        wprintw(window, "%9.2f%9.2f", ravs, wavs);
        wprintw(window, "%9.2f%9.2f%9.2f%9.2f", ierr, oerr, coll, sat);
        wprintw(window, "%9.2f%9.2f%9.2f", rutil, wutil, util);
        wprintw(window, "\n");
    }

    wrefresh(window);
}
