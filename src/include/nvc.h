/**
 * This file is part of nvIPFIX
 * Copyright (C) 2015 Denis Rozhkov <denis@rozhkoff.com>
 *
 * nvIPFIX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef __NVIPFIX_NVC_H
#define __NVIPFIX_NVC_H


#include <stdint.h>

#include "types.h"

#ifdef NVIPFIX_DEF_POSIX
#include <arpa/inet.h>
#ifdef __linux__
#include <net/ethernet.h>
#elif defined (__SVR4)
#include <sys/ethernet.h>
#endif
#endif


#define	nvOS_RESULT_MSG_LEN 255

#define	nvc_PCL_NAME_LEN 60

#define	nvc_FIELD_FLAG_SET( flags, field ) (void)((flags) |= (field))

#define nvc_conn_args_start_time ((uint64_t)1 << 1)
#define nvc_conn_args_end_time ((uint64_t)1 << 2)


#ifndef __SVR4
typedef enum { B_FALSE, B_TRUE } boolean_t;
typedef struct ether_addr ether_addr_t;
#endif
typedef uint64_t nvOS_time_t;
typedef uint16_t nvc_pcl_vlan_id_t;
typedef uint32_t nvc_pcl_vxlan_id_t;
typedef char nvc_name_string_t[nvc_PCL_NAME_LEN];
typedef uint16_t nvc_ether_type_t;
typedef uint16_t nvc_service_port_type_t;
typedef uint8_t nvc_ip_protocol_t;
typedef struct in6_addr in6_addr_t;
typedef int64_t hrtime_t;

typedef enum {
	nvc_CONN_TRANS_ST_AND_END = 0,
	nvc_CONN_TRANS_STARTED,
	nvc_CONN_TRANS_ENDED,
	nvc_CONN_TRANS_RUNNING,
} nvc_conn_trans_t;

typedef enum {
	nvc_TCP_STATE_SYN,
	nvc_TCP_STATE_EST,
	nvc_TCP_STATE_FIN,
	nvc_TCP_STATE_RST,
} nvc_tcp_state_t;

typedef int (* nvOS_out_func_t)( void * arg, const void * buf, int size );
typedef int (* nvOS_in_func_t)( void * arg, void * buf, int size );
typedef int (* nvOS_status_func_t)( void * arg, char * status_msg );

typedef struct nvOS_io_s {
	nvOS_out_func_t out_func;
	void * out_arg;
	nvOS_in_func_t in_func;
	void * in_arg;
	nvOS_out_func_t stream_func;
	void * stream_arg;
	nvOS_status_func_t status_func;
	void * status_arg;
} nvOS_io_t;

typedef enum {
	nvOS_SUCCESS = 0,
	nvOS_FAILURE = -1,
} nvOS_status_t;

typedef struct nvOS_result_s {
	nvOS_status_t res_status;
	uint32_t res_code;
	char res_msg[nvOS_RESULT_MSG_LEN];
	char * res_file;
	int res_lineno;
	const char * res_func;
} nvOS_result_t;

typedef struct nvc_stats_args_s {
	nvOS_time_t time;
	nvOS_time_t start_time;
	nvOS_time_t end_time;
	uint64_t duration;
	uint64_t interval;
	boolean_t since_start;
	uint64_t older_than;
	uint64_t within_last;
} nvc_stats_args_t;

typedef struct nvc_conn_s {
	nvc_stats_args_t conn_args;
	uint32_t conn_sum_by_count;
	nvc_pcl_vlan_id_t conn_vlan;
	nvc_pcl_vxlan_id_t conn_vxlan;
	nvc_name_string_t conn_vnet;
	uint32_t conn_client_switch_port;
	uint32_t conn_server_switch_port;
	nvc_ether_type_t conn_ether_type;
	ether_addr_t conn_client_mac_addr;
	ether_addr_t conn_server_mac_addr;
	in6_addr_t conn_client_ip;
	in6_addr_t conn_server_ip;
	nvc_service_port_type_t conn_client_port;
	nvc_service_port_type_t conn_server_port;
	uint8_t conn_tos;
	nvc_ip_protocol_t conn_proto;
	nvc_tcp_state_t conn_state;
	hrtime_t conn_dur;
	hrtime_t conn_avg_latency;
	uint32_t conn_bytes_sent;
	uint32_t conn_bytes_recv;
	uint32_t conn_bytes_total;
	nvOS_time_t conn_started_time;
	nvOS_time_t conn_ended_time;
	uint64_t conn_age;
	nvc_conn_trans_t conn_trans;
	struct nvc_conn_s * conn_next;
} nvc_conn_t;

typedef struct nvc_format_args_s {
	char sort_asc[256];
	char sort_desc[256];
	uint32_t limit_output;
	char sum_by[256];
} nvc_format_args_t;

typedef int (* nvc_show_conn_stat_func_t)( void * arg, uint64_t fields, nvc_conn_t * conn_stat );


/**
 *
 * @param io
 */
void nvc_init( nvOS_io_t * io );

/**
 *
 * @param io
 * @param hostport
 */
void nvc_init_net( nvOS_io_t * io, char * hostport );

/**
 *
 * @param io
 * @return
 */
int nvc_connect( nvOS_io_t * io );

/**
 *
 * @param io
 * @param user_name
 * @param password
 * @param result
 * @return
 */
int nvc_authenticate( nvOS_io_t * io, char * user_name, char * password, nvOS_result_t * result );

/**
 *
 * @param io
 * @param user_name
 * @param sz
 * @param result
 * @return
 */
int nvc_check_uid( nvOS_io_t * io, char * user_name, int sz, nvOS_result_t * result );

/**
 *
 * @param io
 * @param fields
 * @param filter
 * @param format_fields
 * @param format_args
 * @param show_func
 * @param arg
 * @param result
 * @return
 */
int nvc_show_conn_stat( nvOS_io_t * io, uint64_t fields, nvc_conn_t * filter, uint64_t format_fields,
		nvc_format_args_t * format_args, nvc_show_conn_stat_func_t show_func, void * arg, nvOS_result_t * result );

/**
 *
 * @param io
 * @return
 */
int nvc_logout( nvOS_io_t * io );

/**
 *
 * @param io
 */
void nvc_disconnect( nvOS_io_t * io );

/**
 *
 * @param io
 */
void nvc_done( nvOS_io_t * io );


#endif /* __NVIPFIX_NVC_H */
