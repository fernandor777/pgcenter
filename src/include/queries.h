/*
 ****************************************************************************
 * queries.h
 *      queries definitions.
 *
 * (C) 2016 by Alexey V. Lesovsky (lesovsky <at> gmail.com)
 * 
 ****************************************************************************
 */
#ifndef __QUERIES_H__
#define __QUERIES_H__

/* Linux sys stats from /proc pseudo-filesystem */
#define DISKSTATS_VIEW  "pgcenter.sys_proc_diskstats"
#define NETDEV_VIEW     "pgcenter.sys_proc_netdev"

#define PG_SYS_GET_CLK_QUERY \
    "SELECT pgcenter.get_sys_clk_ticks()"

#define PG_SYS_PROC_LOADAVG_QUERY \
    "SELECT min1, min5, min15 FROM pgcenter.sys_proc_loadavg"

#define PG_SYS_PROC_UPTIME_QUERY \
    "SELECT seconds_total FROM pgcenter.sys_proc_uptime"

#define PG_SYS_PROC_TOTAL_CPU_STAT_QUERY \
    "SELECT * FROM pgcenter.sys_proc_stat WHERE cpu = 'cpu'"

#define PG_SYS_PROC_PART_CPU_STAT_QUERY \
    "SELECT right(cpu,-3),* FROM pgcenter.sys_proc_stat WHERE cpu ~ 'cpu[0-9]+' \
    ORDER BY right(cpu,-3)::int"
        
#define PG_SYS_PROC_MEMINFO_QUERY \
    "SELECT metric, metric_value FROM pgcenter.sys_proc_meminfo WHERE metric IN \
    ('MemTotal:','MemFree:','SwapTotal:','SwapFree:', \
    'Cached:','Dirty:','Writeback:','Buffers:','Slab:') \
    ORDER BY 1"

#define PG_SYS_PROC_BDEV_CNT_QUERY \
    "SELECT count(1) FROM pgcenter.sys_proc_diskstats"
    
#define PG_SYS_PROC_DISKSTATS_QUERY \
    "SELECT * FROM pgcenter.sys_proc_diskstats ORDER BY (maj,min)"

#define PG_SYS_ETHTOOL_LINK_QUERY \
    "SELECT * FROM pgcenter.get_netdev_link_settings"

#define PG_SYS_PROC_IFDEV_CNT_QUERY \
    "SELECT count(1) FROM pgcenter.sys_proc_netdev"

#define PG_SYS_PROC_NETDEV_QUERY \
    "SELECT left(iface,-1),* FROM pgcenter.sys_proc_netdev ORDER BY iface"

/* drop pgcenter's stats schema and all its content */
#define PG_DROP_STATS_SCHEMA_QUERY "DROP SCHEMA pgcenter CASCADE"

/* for postgresql versions before 9.6 */
#define PG_STAT_ACTIVITY_COUNT_95_QUERY \
    "WITH pgsa AS (SELECT * FROM pg_stat_activity) \
       SELECT \
         (SELECT count(*) AS total FROM pgsa), \
         (SELECT count(*) AS idle FROM pgsa WHERE state = 'idle'), \
         (SELECT count(*) AS idle_in_xact FROM pgsa WHERE state IN ('idle in transaction', 'idle in transaction (aborted)')), \
         (SELECT count(*) AS active FROM pgsa WHERE state = 'active'), \
         (SELECT count(*) AS waiting FROM pgsa WHERE waiting), \
         (SELECT count(*) AS others FROM pgsa WHERE state IN ('fastpath function call','disabled')), \
         (SELECT count(*) AS total_prepared FROM pg_prepared_xacts)"

/* for postgresql versions since 9.6 */
#define PG_STAT_ACTIVITY_COUNT_96_QUERY \
    "WITH pgsa AS (SELECT * FROM pg_stat_activity) \
       SELECT \
         (SELECT count(*) AS total FROM pgsa), \
         (SELECT count(*) AS idle FROM pgsa WHERE state = 'idle'), \
         (SELECT count(*) AS idle_in_xact FROM pgsa WHERE state IN ('idle in transaction', 'idle in transaction (aborted)')), \
         (SELECT count(*) AS active FROM pgsa WHERE state = 'active'), \
         (SELECT count(*) AS waiting FROM pgsa WHERE wait_event IS NOT NULL), \
         (SELECT count(*) AS others FROM pgsa WHERE state IN ('fastpath function call','disabled')), \
         (SELECT count(*) AS total_prepared FROM pg_prepared_xacts)"

/* for postgresql versions since 10.0 */
#define PG_STAT_ACTIVITY_COUNT_QUERY \
    "WITH pgsa AS (SELECT * FROM pg_stat_activity) \
       SELECT \
         (SELECT count(*) AS total FROM pgsa), \
         (SELECT count(*) AS idle FROM pgsa WHERE state = 'idle'), \
         (SELECT count(*) AS idle_in_xact FROM pgsa WHERE state IN ('idle in transaction', 'idle in transaction (aborted)')), \
         (SELECT count(*) AS active FROM pgsa WHERE state = 'active'), \
         (SELECT count(*) AS waiting FROM pgsa WHERE wait_event_type = 'Lock'), \
         (SELECT count(*) AS others FROM pgsa WHERE state IN ('fastpath function call','disabled')), \
         (SELECT count(*) AS total_prepared FROM pg_prepared_xacts)"

#define PG_STAT_ACTIVITY_AV_COUNT_QUERY \
    "WITH pgsa AS (SELECT * FROM pg_stat_activity) \
       SELECT \
         (SELECT count(*) AS av_workers FROM pgsa WHERE query ~* '^autovacuum:' AND pid <> pg_backend_pid()), \
         (SELECT count(*) AS av_wrap FROM pgsa WHERE query ~* '^autovacuum:.*to prevent wraparound' AND pid <> pg_backend_pid()), \
	 (SELECT count(*) AS v_manual FROM pgsa WHERE query ~* '^vacuum' AND pid <> pg_backend_pid()), \
	 (SELECT coalesce(date_trunc('seconds', max(now() - xact_start)), '00:00:00') AS av_maxtime FROM pgsa \
	 WHERE (query ~* '^autovacuum:' OR query ~* '^vacuum') AND pid <> pg_backend_pid());"

#define PG_STAT_STATEMENTS_SYS_QUERY \
        "SELECT (sum(total_time) / sum(calls))::numeric(6,3) AS avg_query, sum(calls) AS total_calls FROM pg_stat_statements"
#define PG_STAT_ACTIVITY_SYS_QUERY \
        "SELECT \
            (SELECT coalesce(date_trunc('seconds', max(now() - xact_start)), '00:00:00') \
                AS xact_maxtime FROM pg_stat_activity \
                WHERE (query !~* '^autovacuum:' AND query !~* '^vacuum') AND pid <> pg_backend_pid()), \
            (SELECT COALESCE(date_trunc('seconds', max(clock_timestamp() - prepared)), '00:00:00') \
                AS prep_maxtime FROM pg_prepared_xacts);"

/* context queries */
#define PG_STAT_DATABASE_91_QUERY \
    "SELECT \
        datname, \
        xact_commit AS commit, xact_rollback AS rollback, \
        blks_read AS reads, blks_hit AS hits, \
        tup_returned AS returned, tup_fetched AS fetched, \
        tup_inserted AS inserts, tup_updated AS updates, tup_deleted AS deletes, \
        conflicts \
    FROM pg_stat_database \
    ORDER BY datname"

#define PG_STAT_DATABASE_QUERY \
    "SELECT \
        datname, \
        xact_commit AS commit, xact_rollback AS rollback, \
        blks_read AS reads, blks_hit AS hits, \
        tup_returned AS returned, tup_fetched AS fetched, \
        tup_inserted AS inserts, tup_updated AS updates, tup_deleted AS deletes, \
        conflicts, deadlocks, \
        temp_files AS tmp_files, temp_bytes AS tmp_bytes, \
        blk_read_time AS read_t, blk_write_time AS write_t, \
        date_trunc('seconds', now() - stats_reset) as stats_age \
    FROM pg_stat_database \
    ORDER BY datname DESC"

/* Start and end number for columns used for make diff array */
#define PG_STAT_DATABASE_DIFF_MIN           1
#define PG_STAT_DATABASE_DIFF_MAX_91        10
#define PG_STAT_DATABASE_DIFF_MAX_LT        15
/* Max number of columns for specified context, can vary in different PostgreSQL versions */
#define PG_STAT_DATABASE_CMAX_91            10
#define PG_STAT_DATABASE_CMAX_LT            16

#define PG_STAT_REPLICATION_94_QUERY_P1 \
    "SELECT \
        client_addr AS client, usename AS user, application_name AS name, \
        state, sync_state AS mode, \
        (pg_xlog_location_diff("
#define PG_STAT_REPLICATION_94_QUERY_P2 ",'0/0') / 1024)::bigint as xlog, \
        (pg_xlog_location_diff("
#define PG_STAT_REPLICATION_94_QUERY_P3 \
    ",sent_location) / 1024)::bigint as pending, \
	(pg_xlog_location_diff(sent_location,write_location) / 1024)::bigint as write, \
	(pg_xlog_location_diff(write_location,flush_location) / 1024)::bigint as flush, \
	(pg_xlog_location_diff(flush_location,replay_location) / 1024)::bigint as replay, \
	(pg_xlog_location_diff("
#define PG_STAT_REPLICATION_94_QUERY_P4 \
    ",replay_location))::bigint / 1024 as total_lag \
    FROM pg_stat_replication \
    ORDER BY left(md5(client_addr::text || client_port::text), 10) DESC"

/* 9.6 query */
#define PG_STAT_REPLICATION_96_QUERY_P1 \
    "SELECT \
        client_addr AS client, usename AS user, application_name AS name, \
        state, sync_state AS mode, \
        (pg_xlog_location_diff("
#define PG_STAT_REPLICATION_96_QUERY_P2 ",'0/0') / 1024)::bigint as xlog, \
        (pg_xlog_location_diff("
#define PG_STAT_REPLICATION_96_QUERY_P3 \
    ",sent_location) / 1024)::bigint as pending, \
	(pg_xlog_location_diff(sent_location,write_location) / 1024)::bigint as write, \
	(pg_xlog_location_diff(write_location,flush_location) / 1024)::bigint as flush, \
	(pg_xlog_location_diff(flush_location,replay_location) / 1024)::bigint as replay, \
	(pg_xlog_location_diff("
#define PG_STAT_REPLICATION_96_QUERY_P4 \
    ",replay_location))::bigint / 1024 as total_lag \
    FROM pg_stat_replication \
    ORDER BY left(md5(client_addr::text || client_port::text), 10) DESC"
#define PG_STAT_REPLICATION_96_QUERY_EXT_P4 \
    ",replay_location))::bigint / 1024 as total_lag, \
    (pg_last_committed_xact()).xid::text::bigint - backend_xmin::text::bigint as xact_age, \
    date_trunc('seconds', (pg_last_committed_xact()).timestamp - pg_xact_commit_timestamp(backend_xmin)) as time_age \
    FROM pg_stat_replication \
    ORDER BY left(md5(client_addr::text || client_port::text), 10) DESC"

/* since postgresql 10 xlog functions renamed to wal functions */
#define PG_STAT_REPLICATION_QUERY_P1 \
    "SELECT \
        client_addr AS client, usename AS user, application_name AS name, \
        state, sync_state AS mode, \
        (pg_wal_lsn_diff("
#define PG_STAT_REPLICATION_QUERY_P2 ",'0/0') / 1024)::bigint as wal, \
        (pg_wal_lsn_diff("
#define PG_STAT_REPLICATION_QUERY_P3 \
    ",sent_lsn) / 1024)::bigint as pending, \
	(pg_wal_lsn_diff(sent_lsn,write_lsn) / 1024)::bigint as write, \
	(pg_wal_lsn_diff(write_lsn,flush_lsn) / 1024)::bigint as flush, \
	(pg_wal_lsn_diff(flush_lsn,replay_lsn) / 1024)::bigint as replay, \
	(pg_wal_lsn_diff("
#define PG_STAT_REPLICATION_QUERY_P4 \
    ",replay_lsn))::bigint / 1024 as total_lag, \
    date_trunc('seconds', write_lag) AS write_lag, \
    date_trunc('seconds', flush_lag) AS flush_lag, \
    date_trunc('seconds', replay_lag) AS replay_lag \
    FROM pg_stat_replication \
    ORDER BY left(md5(client_addr::text || client_port::text), 10) DESC"
#define PG_STAT_REPLICATION_QUERY_EXT_P4 \
    ",replay_lsn))::bigint / 1024 as total_lag, \
    date_trunc('seconds', write_lag) AS write_lag, \
    date_trunc('seconds', flush_lag) AS flush_lag, \
    date_trunc('seconds', replay_lag) AS replay_lag, \
    (pg_last_committed_xact()).xid::text::bigint - backend_xmin::text::bigint as xact_age, \
    date_trunc('seconds', (pg_last_committed_xact()).timestamp - pg_xact_commit_timestamp(backend_xmin)) as time_age \
    FROM pg_stat_replication \
    ORDER BY left(md5(client_addr::text || client_port::text), 10) DESC"

/* use functions depending on recovery */
#define PG_STAT_REPLICATION_NOREC "pg_current_xlog_location()"
#define PG_STAT_REPLICATION_REC "pg_last_xlog_receive_location()"
#define PG_STAT_REPLICATION_NOREC_10 "pg_current_wal_lsn()"
#define PG_STAT_REPLICATION_REC_10 "pg_last_wal_receive_lsn()"
#define PG_STAT_REPLICATION_CMAX_94 10
#define PG_STAT_REPLICATION_CMAX_96 10
#define PG_STAT_REPLICATION_CMAX_96_EXT 12
#define PG_STAT_REPLICATION_CMAX_LT 13
#define PG_STAT_REPLICATION_CMAX_LT_EXT 15
/* diff array using only one column */
#define PG_STAT_REPLICATION_DIFF_MIN     5

#define PG_STAT_TABLES_QUERY_P1 \
    "SELECT \
        schemaname || '.' || relname as relation, \
        seq_scan, seq_tup_read as seq_read, \
        idx_scan, idx_tup_fetch as idx_fetch, \
        n_tup_ins as inserts, n_tup_upd as updates, \
        n_tup_del as deletes, n_tup_hot_upd as hot_updates, \
        n_live_tup as live, n_dead_tup as dead \
    FROM pg_stat_"
#define PG_STAT_TABLES_QUERY_P2 "_tables ORDER BY (schemaname || '.' || relname) DESC"

#define PG_STAT_TABLES_DIFF_MIN     1
#define PG_STAT_TABLES_DIFF_MAX     10
#define PG_STAT_TABLES_CMAX_LT      10

#define PG_STATIO_TABLES_QUERY_P1 \
    "SELECT \
        schemaname ||'.'|| relname as relation, \
        heap_blks_read * (SELECT current_setting('block_size')::int / 1024) AS heap_read, \
        heap_blks_hit * (SELECT current_setting('block_size')::int / 1024) AS heap_hit, \
        idx_blks_read * (SELECT current_setting('block_size')::int / 1024) AS idx_read, \
        idx_blks_hit * (SELECT current_setting('block_size')::int / 1024) AS idx_hit, \
        toast_blks_read * (SELECT current_setting('block_size')::int / 1024) AS toast_read, \
        toast_blks_hit * (SELECT current_setting('block_size')::int / 1024) AS toast_hit, \
        tidx_blks_read * (SELECT current_setting('block_size')::int / 1024) AS tidx_read, \
        tidx_blks_hit * (SELECT current_setting('block_size')::int / 1024) AS tidx_hit \
    FROM pg_statio_"
#define PG_STATIO_TABLES_QUERY_P2 "_tables ORDER BY (schemaname || '.' || relname) DESC"

#define PG_STATIO_TABLES_DIFF_MIN   1
#define PG_STATIO_TABLES_DIFF_MAX   8
#define PG_STATIO_TABLES_CMAX_LT    8

#define PG_STAT_INDEXES_QUERY_P1 \
    "SELECT \
        s.schemaname ||'.'|| s.relname as relation, s.indexrelname AS index, \
        s.idx_scan, s.idx_tup_read, s.idx_tup_fetch, \
        i.idx_blks_read * (SELECT current_setting('block_size')::int / 1024) AS idx_read, \
        i.idx_blks_hit * (SELECT current_setting('block_size')::int / 1024) AS idx_hit \
    FROM \
        pg_stat_"
#define PG_STAT_INDEXES_QUERY_P2 "_indexes s, pg_statio_"
#define PG_STAT_INDEXES_QUERY_P3 "_indexes i WHERE s.indexrelid = i.indexrelid \
        ORDER BY (s.schemaname ||'.'|| s.relname ||'.'|| s.indexrelname) DESC"

#define PG_STAT_INDEXES_DIFF_MIN    2
#define PG_STAT_INDEXES_DIFF_MAX    6
#define PG_STAT_INDEXES_CMAX_LT     6

#define PG_TABLES_SIZE_QUERY_P1 \
    "SELECT \
        s.schemaname ||'.'|| s.relname AS relation, \
        pg_total_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024 AS total_size, \
        pg_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024 AS rel_size, \
        (pg_total_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024) - \
            (pg_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024) AS idx_size, \
        pg_total_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024 AS total_change, \
        pg_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024 AS rel_change, \
        (pg_total_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024) - \
            (pg_relation_size((s.schemaname ||'.'|| s.relname)::regclass) / 1024) AS idx_change \
        FROM pg_stat_"
#define PG_TABLES_SIZE_QUERY_P2 "_tables s, pg_class c WHERE s.relid = c.oid \
        ORDER BY (s.schemaname || '.' || s.relname) DESC"

#define PG_TABLES_SIZE_DIFF_MIN     4
#define PG_TABLES_SIZE_DIFF_MAX     6
#define PG_TABLES_SIZE_CMAX_LT      6

#define PG_STAT_ACTIVITY_LONG_91_QUERY_P1 \
    "SELECT \
        procpid AS pid, client_addr AS cl_addr, client_port AS cl_port, \
        datname, usename, waiting, \
        date_trunc('seconds', clock_timestamp() - xact_start) AS xact_age, \
        date_trunc('seconds', clock_timestamp() - query_start) AS query_age, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(current_query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_activity \
    WHERE ((clock_timestamp() - xact_start) > '"
#define PG_STAT_ACTIVITY_LONG_91_QUERY_P2 \
    "'::interval OR (clock_timestamp() - query_start) > '"
#define PG_STAT_ACTIVITY_LONG_91_QUERY_P3 \
    "'::interval) AND current_query <> '<IDLE>' AND procpid <> pg_backend_pid() \
    ORDER BY procpid DESC"

#define PG_STAT_ACTIVITY_LONG_95_QUERY_P1 \
    "SELECT \
        pid, client_addr AS cl_addr, client_port AS cl_port, \
        datname, usename, state, waiting, \
        date_trunc('seconds', clock_timestamp() - xact_start) AS xact_age, \
        date_trunc('seconds', clock_timestamp() - query_start) AS query_age, \
        date_trunc('seconds', clock_timestamp() - state_change) AS change_age, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_activity \
    WHERE ((clock_timestamp() - xact_start) > '"
#define PG_STAT_ACTIVITY_LONG_95_QUERY_P2 \
    "'::interval OR (clock_timestamp() - query_start) > '"
#define PG_STAT_ACTIVITY_LONG_95_QUERY_P3 \
    "'::interval) AND state <> 'idle' AND pid <> pg_backend_pid() \
    ORDER BY pid DESC"

#define PG_STAT_ACTIVITY_LONG_96_QUERY_P1 \
    "SELECT \
        pid, client_addr AS cl_addr, client_port AS cl_port, \
        datname, usename, state, wait_event_type AS wait_etype, wait_event, \
        date_trunc('seconds', clock_timestamp() - xact_start) AS xact_age, \
        date_trunc('seconds', clock_timestamp() - query_start) AS query_age, \
        date_trunc('seconds', clock_timestamp() - state_change) AS change_age, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_activity \
    WHERE ((clock_timestamp() - xact_start) > '"
#define PG_STAT_ACTIVITY_LONG_96_QUERY_P2 \
    "'::interval OR (clock_timestamp() - query_start) > '"
#define PG_STAT_ACTIVITY_LONG_96_QUERY_P3 \
    "'::interval) AND state <> 'idle' AND pid <> pg_backend_pid() \
    ORDER BY pid DESC"

#define PG_STAT_ACTIVITY_LONG_QUERY_P1 \
    "SELECT \
        pid, client_addr AS cl_addr, client_port AS cl_port, \
        datname, usename, state, backend_type, wait_event_type AS wait_etype, wait_event, \
        date_trunc('seconds', clock_timestamp() - xact_start) AS xact_age, \
        date_trunc('seconds', clock_timestamp() - query_start) AS query_age, \
        date_trunc('seconds', clock_timestamp() - state_change) AS change_age, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_activity \
    WHERE ((clock_timestamp() - xact_start) > '"
#define PG_STAT_ACTIVITY_LONG_QUERY_P2 \
    "'::interval OR (clock_timestamp() - query_start) > '"
#define PG_STAT_ACTIVITY_LONG_QUERY_P3 \
    "'::interval) AND state <> 'idle' AND pid <> pg_backend_pid() \
    ORDER BY pid DESC"

/* don't use array sorting when showing long activity, row order defined in query */
#define PG_STAT_ACTIVITY_LONG_CMAX_91       8
#define PG_STAT_ACTIVITY_LONG_CMAX_95       10
#define PG_STAT_ACTIVITY_LONG_CMAX_96       11
#define PG_STAT_ACTIVITY_LONG_CMAX_LT       12

#define PG_STAT_FUNCTIONS_QUERY_P1 \
    "SELECT \
        funcid, schemaname ||'.'||funcname AS function, \
        calls AS total_calls, calls AS calls, \
        date_trunc('seconds', total_time / 1000 * '1 second'::interval) AS total_t, \
        date_trunc('seconds', self_time / 1000 * '1 second'::interval) AS self_t, \
        round((total_time / calls)::numeric, 4) AS avg_t, \
        round((self_time / calls)::numeric, 4) AS avg_self_t \
    FROM pg_stat_user_functions \
    ORDER BY funcid DESC"

/* diff array using only one column */
#define PG_STAT_FUNCTIONS_DIFF_MIN     3
#define PG_STAT_FUNCTIONS_CMAX_LT      7

#define PG_STAT_STATEMENTS_TIMING_91_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        date_trunc('seconds', round(sum(p.total_time)) / 1000 * '1 second'::interval) AS t_all_t, \
        round(sum(p.total_time)) AS all_t, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PG_STAT_STATEMENTS_TIMING_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        date_trunc('seconds', round(sum(p.total_time)) / 1000 * '1 second'::interval) AS t_all_t, \
        date_trunc('seconds', round(sum(p.blk_read_time)) / 1000 * '1 second'::interval) AS t_read_t, \
        date_trunc('seconds', round(sum(p.blk_write_time)) / 1000 * '1 second'::interval) AS t_write_t, \
        date_trunc('seconds', round((sum(p.total_time) - (sum(p.blk_read_time) + sum(p.blk_write_time)))) / 1000 * '1 second'::interval) AS t_cpu_t, \
        round(sum(p.total_time)) AS all_t, \
        round(sum(p.blk_read_time)) AS read_t, \
        round(sum(p.blk_write_time)) AS write_t, \
        round((sum(p.total_time) - (sum(p.blk_read_time) + sum(p.blk_write_time)))) AS cpu_t, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PGSS_TIMING_DIFF_MIN_91  3
#define PGSS_TIMING_DIFF_MAX_91  4
#define PGSS_TIMING_DIFF_MIN_LT  6
#define PGSS_TIMING_DIFF_MAX_LT  10
#define PGSS_TIMING_CMAX_91      6
#define PGSS_TIMING_CMAX_LT      12

#define PG_STAT_STATEMENTS_GENERAL_91_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        sum(p.calls) AS t_calls, sum(p.rows) as t_rows, \
        sum(p.calls) AS calls, sum(p.rows) as rows, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PG_STAT_STATEMENTS_GENERAL_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        sum(p.calls) AS t_calls, sum(p.rows) as t_rows, \
        sum(p.calls) AS calls, sum(p.rows) as rows, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PGSS_GENERAL_DIFF_MIN_LT    4
#define PGSS_GENERAL_DIFF_MAX_LT    5
#define PGSS_GENERAL_CMAX_LT        7

#define PG_STAT_STATEMENTS_IO_91_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        (sum(p.shared_blks_hit) + sum(p.local_blks_hit)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_hits, \
        (sum(p.shared_blks_read) + sum(p.local_blks_read)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_reads, \
        (sum(p.shared_blks_written) + sum(p.local_blks_written)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_written, \
        (sum(p.shared_blks_hit) + sum(p.local_blks_hit)) \
            * (SELECT current_setting('block_size')::int / 1024) as hits, \
        (sum(p.shared_blks_read) + sum(p.local_blks_read)) \
            * (SELECT current_setting('block_size')::int / 1024) as reads, \
        (sum(p.shared_blks_written) + sum(p.local_blks_written)) \
            * (SELECT current_setting('block_size')::int / 1024) as written, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PG_STAT_STATEMENTS_IO_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        (sum(p.shared_blks_hit) + sum(p.local_blks_hit)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_hits, \
        (sum(p.shared_blks_read) + sum(p.local_blks_read)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_reads, \
        (sum(p.shared_blks_dirtied) + sum(p.local_blks_dirtied)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_dirtied, \
        (sum(p.shared_blks_written) + sum(p.local_blks_written)) \
            * (SELECT current_setting('block_size')::int / 1024) as t_written, \
        (sum(p.shared_blks_hit) + sum(p.local_blks_hit)) \
            * (SELECT current_setting('block_size')::int / 1024) as hits, \
        (sum(p.shared_blks_read) + sum(p.local_blks_read)) \
            * (SELECT current_setting('block_size')::int / 1024) as reads, \
        (sum(p.shared_blks_dirtied) + sum(p.local_blks_dirtied)) \
            * (SELECT current_setting('block_size')::int / 1024) as dirtied, \
        (sum(p.shared_blks_written) + sum(p.local_blks_written)) \
            * (SELECT current_setting('block_size')::int / 1024) as written, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PGSS_IO_DIFF_MIN_91    5
#define PGSS_IO_DIFF_MAX_91    8
#define PGSS_IO_DIFF_MIN_LT    6
#define PGSS_IO_DIFF_MAX_LT    10
#define PGSS_IO_CMAX_91    10
#define PGSS_IO_CMAX_LT    12

#define PG_STAT_STATEMENTS_TEMP_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        sum(p.temp_blks_read) \
            * (SELECT current_setting('block_size')::int / 1024) as t_tmp_read, \
        sum(p.temp_blks_written) \
            * (SELECT current_setting('block_size')::int / 1024) as t_tmp_write, \
        sum(p.temp_blks_read) \
            * (SELECT current_setting('block_size')::int / 1024) as tmp_read, \
        sum(p.temp_blks_written) \
            * (SELECT current_setting('block_size')::int / 1024) as tmp_write, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PGSS_TEMP_DIFF_MIN_LT   4
#define PGSS_TEMP_DIFF_MAX_LT   6
#define PGSS_TEMP_CMIN_LT       2
#define PGSS_TEMP_CMAX_LT       8

#define PG_STAT_STATEMENTS_LOCAL_91_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        (sum(p.local_blks_hit)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_hits, \
        (sum(p.local_blks_read)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_reads, \
        (sum(p.local_blks_written)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_written, \
        (sum(p.local_blks_hit)) * (SELECT current_setting('block_size')::int / 1024) as lo_hits, \
        (sum(p.local_blks_read)) * (SELECT current_setting('block_size')::int / 1024) as lo_reads, \
        (sum(p.local_blks_written)) * (SELECT current_setting('block_size')::int / 1024) as lo_written, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PG_STAT_STATEMENTS_LOCAL_QUERY_P1 \
    "SELECT \
        a.rolname AS user, d.datname AS database, \
        (sum(p.local_blks_hit)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_hits, \
        (sum(p.local_blks_read)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_reads, \
        (sum(p.local_blks_dirtied)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_dirtied, \
        (sum(p.local_blks_written)) * (SELECT current_setting('block_size')::int / 1024) as t_lo_written, \
        (sum(p.local_blks_hit)) * (SELECT current_setting('block_size')::int / 1024) as lo_hits, \
        (sum(p.local_blks_read)) * (SELECT current_setting('block_size')::int / 1024) as lo_reads, \
        (sum(p.local_blks_dirtied)) * (SELECT current_setting('block_size')::int / 1024) as lo_dirtied, \
        (sum(p.local_blks_written)) * (SELECT current_setting('block_size')::int / 1024) as lo_written, \
        sum(p.calls) AS calls, \
        left(md5(d.datname || a.rolname || p.query ), 10) AS queryid, \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace( \
        regexp_replace(p.query, \
            E'\\\\?(::[a-zA-Z_]+)?( *, *\\\\?(::[a-zA-Z_]+)?)+', '?', 'g'), \
            E'\\\\$[0-9]+(::[a-zA-Z_]+)?( *, *\\\\$[0-9]+(::[a-zA-Z_]+)?)*', '$N', 'g'), \
            E'--.*$', '', 'ng'), \
            E'/\\\\*.*?\\\\*\\/', '', 'g'), \
            E'\\\\s+', ' ', 'g') AS query \
    FROM pg_stat_statements p \
    JOIN pg_roles a ON a.oid=p.userid \
    JOIN pg_database d ON d.oid=p.dbid \
    GROUP BY a.rolname, d.datname, query \
    ORDER BY left(md5(d.datname || a.rolname || p.query ), 10) DESC"

#define PGSS_LOCAL_DIFF_MIN_91    5
#define PGSS_LOCAL_DIFF_MAX_91    8
#define PGSS_LOCAL_DIFF_MIN_LT    6
#define PGSS_LOCAL_DIFF_MAX_LT    10
#define PGSS_LOCAL_CMAX_91    10
#define PGSS_LOCAL_CMAX_LT    12

#define PG_STAT_PROGRESS_VACUUM_QUERY \
    "SELECT \
     	a.pid, \
	date_trunc('seconds', clock_timestamp() - xact_start) AS xact_age, \
        v.datname, v.relid::regclass AS relation, \
	a.state, v.phase, \
	v.heap_blks_total * (SELECT current_setting('block_size')::int / 1024) AS total, \
	v.heap_blks_scanned * (SELECT current_setting('block_size')::int / 1024) AS scanned, \
	v.heap_blks_vacuumed * (SELECT current_setting('block_size')::int / 1024) AS vacuumed, \
	a.wait_event_type AS wait_etype, a.wait_event, \
	a.query \
    FROM pg_stat_progress_vacuum v \
    JOIN pg_stat_activity a ON v.pid = a.pid \
    ORDER BY a.pid DESC"

#define PG_STAT_PROGRESS_VACUUM_CMAX_LT 11

/* other queries */
/* don't log our queries */
#define PG_SUPPRESS_LOG_QUERY "SET log_min_duration_statement TO 10000"

/* set work_mem for pg_stat_statements queries */
#define PG_INCREASE_WORK_MEM_QUERY "SET work_mem TO '32MB'"

/* check pg_is_in_recovery() */
#define PG_IS_IN_RECOVERY_QUERY "SELECT pg_is_in_recovery()"

/* get full config query */
#define PG_SETTINGS_QUERY "SELECT name, setting, unit, category FROM pg_settings ORDER BY 4"

/* get one setting query */
#define PG_SETTINGS_SINGLE_OPT_P1 "SELECT name, setting FROM pg_settings WHERE name = '"
#define PG_SETTINGS_SINGLE_OPT_P2 "'"

/* reload postgres */
#define PG_RELOAD_CONF_QUERY "SELECT pg_reload_conf()"

/* cancel/terminate backend */
#define PG_CANCEL_BACKEND_P1 "SELECT pg_cancel_backend("
#define PG_CANCEL_BACKEND_P2 ")"
#define PG_TERM_BACKEND_P1 "SELECT pg_terminate_backend("
#define PG_TERM_BACKEND_P2 ")"

/* cancel/terminate group of backends */
#define PG_SIG_GROUP_BACKEND_P1 "SELECT pg_"
#define PG_SIG_GROUP_BACKEND_P2 "_backend(pid) FROM pg_stat_activity WHERE "
#define PG_SIG_GROUP_BACKEND_P3 " AND ((clock_timestamp() - xact_start) > '"
#define PG_SIG_GROUP_BACKEND_P4 "'::interval OR (clock_timestamp() - query_start) > '"
#define PG_SIG_GROUP_BACKEND_P5 "'::interval) AND pid <> pg_backend_pid()"

/* reset statistics query */
#define PG_STAT_RESET_QUERY "SELECT pg_stat_reset(), pg_stat_statements_reset()"

/* postmaster uptime query */
#define PG_UPTIME_QUERY "SELECT date_trunc('seconds', now() - pg_postmaster_start_time())"

#endif  /* __QUERIES_H__ */
