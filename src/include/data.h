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

#ifndef __NVIPFIX_DATA_H
#define __NVIPFIX_DATA_H


#include <stdint.h>
#include <stdbool.h>

#include "types.h"


typedef enum {
	NV_IPFIX_PROTOCOL_ICMP = 1,
	NV_IPFIX_PROTOCOL_TCP = 6,
	NV_IPFIX_PROTOCOL_UDP = 17
} nvIPFIX_PROTOCOL;

typedef enum {
	NV_IPFIX_ETHERNET_TYPE_IPV4 = 0x0800
} nvIPFIX_ETHERNET_TYPE;

typedef enum {
	NV_IPFIX_TCP_CONTROL_FLAG_NONE = 0x0000,
	NV_IPFIX_TCP_CONTROL_FLAG_FIN = 0x0001,
	NV_IPFIX_TCP_CONTROL_FLAG_SYN = 0x0002,
	NV_IPFIX_TCP_CONTROL_FLAG_RST = 0x0004,
	NV_IPFIX_TCP_CONTROL_FLAG_ACK = 0x0010
} nvIPFIX_TCP_CONTROL_FLAG;

typedef enum {
	NV_IPFIX_LAYER2_NETWORK_TYPE_VxLAN = 0x01,
	NV_IPFIX_LAYER2_NETWORK_TYPE_NVGRE = 0x02
} nvIPFIX_LAYER2_NETWORK_TYPE;

typedef struct _nvIPFIX_data_record_t {
	nvIPFIX_U16 vlanId;
	nvIPFIX_PROTOCOL protocol;
	nvIPFIX_ETHERNET_TYPE ethernetType;
	nvIPFIX_TCP_CONTROL_FLAG tcpControlBits;
	nvIPFIX_U32 ingressInterface;
	nvIPFIX_U32 egressInterface;
	nvIPFIX_datetime_t flowStart;
	nvIPFIX_datetime_t flowEnd;
	nvIPFIX_timespan_t flowDuration;
	nvIPFIX_timespan_t latency;
	nvIPFIX_BYTE dscp;
	nvIPFIX_U64 initiatorOctets;
	nvIPFIX_U64 responderOctets;
	nvIPFIX_U64 layer2SegmentId;
	nvIPFIX_U64 transportOctetDeltaCount;

	nvIPFIX_mac_address_t sourceMac;
	nvIPFIX_ip_address_t sourceIp;
	nvIPFIX_U16 sourcePort;

	nvIPFIX_mac_address_t destinationMac;
	nvIPFIX_ip_address_t destinationIp;
	nvIPFIX_U16 destinationPort;

	struct _nvIPFIX_data_record_t * prev;
	struct _nvIPFIX_data_record_t * next;
} nvIPFIX_data_record_t;

typedef struct {
	nvIPFIX_data_record_t * head;
	nvIPFIX_data_record_t * tail;
} nvIPFIX_data_record_list_t;


/**
 *
 * @param a_list
 * @param a_record
 * @return
 */
nvIPFIX_data_record_list_t * nvipfix_data_list_add( nvIPFIX_data_record_list_t * a_list, nvIPFIX_data_record_t * a_record );

/**
 *
 * @param a_list
 * @param a_record
 * @return
 */
nvIPFIX_data_record_list_t * nvipfix_data_list_add_copy( nvIPFIX_data_record_list_t * a_list, nvIPFIX_data_record_t * a_record );

/**
 *
 * @param a_list
 */
void nvipfix_data_list_free( nvIPFIX_data_record_list_t * a_list );


#endif /* __NVIPFIX_DATA_H */
