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


#include <sys/time.h>
#include <arpa/inet.h>


#define	nvc_PCL_NAME_LEN 60


typedef uint64_t nvOS_time_t;
typedef uint16_t nvc_pcl_vlan_id_t;
typedef uint32_t nvc_pcl_vxlan_id_t;
typedef char nvc_name_string_t[nvc_PCL_NAME_LEN];
typedef uint16_t nvc_ether_type_t;
typedef uint16_t nvc_service_port_type_t;
typedef uint8_t nvc_ip_protocol_t;

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

typedef struct nvc_stats_args_s {
	nvOS_time_t time;
	nvOS_time_t start_time;
	nvOS_time_t end_time;
	uint64_t duration;
	uint64_t interval;
//	boolean_t	since_start;
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
//	ether_addr_t		conn_client_mac_addr;
//	ether_addr_t		conn_server_mac_addr;
//	in6_addr_t		conn_client_ip;
//	in6_addr_t		conn_server_ip;
	nvc_service_port_type_t conn_client_port;
	nvc_service_port_type_t conn_server_port;
	uint8_t conn_tos;
	nvc_ip_protocol_t conn_proto;
	nvc_tcp_state_t conn_state;
//	hrtime_t		conn_dur;
//	hrtime_t		conn_avg_latency;
	uint32_t conn_bytes_sent;
	uint32_t conn_bytes_recv;
	uint32_t conn_bytes_total;
	nvOS_time_t conn_started_time;
	nvOS_time_t conn_ended_time;
	uint64_t conn_age;
	nvc_conn_trans_t conn_trans;
	struct nvc_conn_s * conn_next;
} nvc_conn_t;


#endif /* __NVIPFIX_NVC_H */
