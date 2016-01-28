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

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "openssl/ssl.h"

#include "include/types.h"
#include "include/error.h"
#include "include/log.h"
#include "include/data.h"
#include "include/nvc.h"

#include "include/import.h"


#define NVIPFIX_IMPORT_ITEM( a_name, a_field, a_parseValue ) { .name = a_name, \
	.offset = offsetof( nvIPFIX_data_record_t, a_field ), .parseValue = a_parseValue }


typedef struct {
	const char * name;
	size_t offset;
	bool (* parseValue)( const char *, void * );
} nvIPFIX_import_item_t;


static bool nvipfix_import_parse_ingress( const char *, void * );
static bool nvipfix_import_parse_egress( const char *, void * );
static bool nvipfix_import_parse_layer2( const char *, void * );
static bool nvipfix_import_parse_tcp_control_flags( const char *, void * );
static bool nvipfix_import_parse_protocol( const char *, void * );
static bool nvipfix_import_parse_ethernet_type( const char *, void * );

static const nvIPFIX_import_item_t * nvipfix_import_get_item( const char * );


enum {
	SizeofFileBuffer = 64 * 1024
};

static const nvIPFIX_import_item_t Items[] = {
		NVIPFIX_IMPORT_ITEM( "vlan", vlanId, nvipfix_parse_u16 ),
		NVIPFIX_IMPORT_ITEM( "src-switch-port", ingressInterface, nvipfix_import_parse_ingress ),
		NVIPFIX_IMPORT_ITEM( "dst-switch-port", egressInterface, nvipfix_import_parse_egress ),
		NVIPFIX_IMPORT_ITEM( "dscp", dscp, nvipfix_parse_byte ),
		NVIPFIX_IMPORT_ITEM( "src-port", sourcePort, nvipfix_parse_u16 ),
		NVIPFIX_IMPORT_ITEM( "dst-port", destinationPort, nvipfix_parse_u16 ),
		NVIPFIX_IMPORT_ITEM( "ibytes", responderOctets, nvipfix_parse_u64 ),
		NVIPFIX_IMPORT_ITEM( "obytes", initiatorOctets, nvipfix_parse_u64 ),
		NVIPFIX_IMPORT_ITEM( "total-bytes", transportOctetDeltaCount, nvipfix_parse_u64 ),
		NVIPFIX_IMPORT_ITEM( "vxlan", layer2SegmentId, nvipfix_import_parse_layer2 ),
		NVIPFIX_IMPORT_ITEM( "cur-state", tcpControlBits, nvipfix_import_parse_tcp_control_flags ),
		NVIPFIX_IMPORT_ITEM( "proto", protocol, nvipfix_import_parse_protocol ),
		NVIPFIX_IMPORT_ITEM( "ether-type", ethernetType, nvipfix_import_parse_ethernet_type ),
		NVIPFIX_IMPORT_ITEM( "src-mac", sourceMac, nvipfix_parse_mac_address ),
		NVIPFIX_IMPORT_ITEM( "dst-mac", destinationMac, nvipfix_parse_mac_address ),
		NVIPFIX_IMPORT_ITEM( "src-ip", sourceIp, nvipfix_parse_ip_address ),
		NVIPFIX_IMPORT_ITEM( "dst-ip", destinationIp, nvipfix_parse_ip_address ),
		NVIPFIX_IMPORT_ITEM( "dur", flowDuration, nvipfix_parse_timespan_microseconds ),
		NVIPFIX_IMPORT_ITEM( "started-time", flowStart, nvipfix_parse_datetime_iso8601 ),
		NVIPFIX_IMPORT_ITEM( "ended-time", flowEnd, nvipfix_parse_datetime_iso8601 ),
		NVIPFIX_IMPORT_ITEM( "ended-time", flowEnd, nvipfix_parse_datetime_iso8601 ),
		NVIPFIX_IMPORT_ITEM( "latency", latency, nvipfix_parse_timespan_microseconds ),
		{ NULL }
};

static const size_t ItemsCount = ((sizeof Items) / sizeof (nvIPFIX_import_item_t)) - 1;


nvIPFIX_data_record_list_t * nvipfix_import( FILE * a_file )
{
	nvIPFIX_data_record_list_t * result = NULL;

	if (a_file != NULL) {
		size_t index = 0;
		char * buffer = NULL;
		size_t bufferSize = 0;

		do {
			bufferSize += SizeofFileBuffer;
			char * newBuffer = realloc( buffer, bufferSize );

			if (newBuffer == NULL) {
				free( buffer );
				buffer = NULL;
				break;
			}

			buffer = newBuffer;

			fread( buffer + index, 1, SizeofFileBuffer, a_file );

			index += (SizeofFileBuffer - 1);
		} while (!feof( a_file ));

		if (buffer != NULL) {
			nvipfix_tlog_debug( NVIPFIX_T( "%s: data len = %d" ), __func__, strlen( buffer ) );
			nvIPFIX_string_list_t * tokens = nvipfix_string_split( buffer, "[]" );

			if (tokens != NULL) {
				nvipfix_tlog_debug( NVIPFIX_T( "%s: tokens count = %d" ), __func__, tokens->count );

				nvIPFIX_string_list_item_t * token = tokens->head;

				while (token != NULL) {
					nvIPFIX_CHAR * tokenCopy = nvipfix_string_trim_copy( token->value, "{}\r\n\t \":" );

					if (tokenCopy != NULL) {
						if (strcmp( "data", tokenCopy ) == 0 && token->next != NULL) {
							token = token->next;
							nvIPFIX_string_list_t * records = nvipfix_string_split( token->value, "{" );

							if (records != NULL) {
								nvipfix_tlog_debug( NVIPFIX_T( "%s: records count = %d" ), __func__, records->count );

								nvIPFIX_string_list_item_t * record = records->head;

								while (record != NULL) {
									nvIPFIX_data_record_t data = { 0 };
									nvIPFIX_string_list_t * items = nvipfix_string_split(
											nvipfix_string_trim( (nvIPFIX_CHAR *)record->value, ",}" ),
											"," );

									if (items != NULL) {
										nvIPFIX_string_list_item_t * item = items->head;

										while (item != NULL) {
											if (item->value != NULL) {
												char * delimiterPtr = strchr( item->value, ':' );

												if (delimiterPtr != NULL) {
													*delimiterPtr = '\v';

													nvIPFIX_string_list_t * itemPair = nvipfix_string_split(
															item->value, "\v" );

													if (itemPair != NULL) {
														if (itemPair->count == 2) {
															nvIPFIX_CHAR * itemName = nvipfix_string_trim(
																	(nvIPFIX_CHAR *)itemPair->head->value, " \t\r\n\"" );

															nvIPFIX_CHAR * itemValue = nvipfix_string_trim(
																	(nvIPFIX_CHAR *)itemPair->tail->value, " \t\r\n\"" );

															if (itemName != NULL && itemValue != NULL) {
																const nvIPFIX_import_item_t * importItem =
																		nvipfix_import_get_item( itemName );

																if (importItem != NULL) {
																	importItem->parseValue(
																			itemValue,
																			((char *)&data) + importItem->offset );
																}
																else {
																	nvipfix_tlog_warning(
																			NVIPFIX_T( "%s: unknown item = '%s'" ),
																			__func__, itemName );
																}
															}
														}

														nvipfix_string_list_free( itemPair, true );
													}
												}
											}

											item = item->next;
										}

										nvIPFIX_data_record_list_t * list = nvipfix_data_list_add_copy( result, &data );
										result = (list != NULL) ? list : result;

										nvipfix_string_list_free( items, true );
									}

									record = record->next;
								}

								nvipfix_string_list_free( records, true );
							}
						}

						free( tokenCopy );
					}

					token = token->next;
				}

				nvipfix_string_list_free( tokens, true );
			}
			else {
				nvipfix_tlog_error( NVIPFIX_T( "%s: unable to tokenize data file" ), __func__ );
			}

			free( buffer );
		}
		else {
			nvipfix_tlog_error( NVIPFIX_T( "%s: memory allocation failed" ), __func__ );
		}
	}
	else {
		nvipfix_tlog_error( NVIPFIX_T( "%s: FILE is null" ), __func__ );
	}

	return result;
}

nvIPFIX_data_record_list_t * nvipfix_import_file( const nvIPFIX_TCHAR * a_fileName )
{
	nvIPFIX_data_record_list_t * result = NULL;

	FILE * dataFile = fopen( a_fileName, "r" );

	if (dataFile != NULL) {
		result = nvipfix_import( dataFile );
		fclose( dataFile );
	}
	else {
		nvipfix_tlog_error( NVIPFIX_T( "%s: unable to open file '%s'" ), __func__, a_fileName );
	}

	return result;
}

#ifdef NVIPFIX_DEF_ENABLE_NVC

static int nvipfix_import_conn_stat_handler( void * arg, uint64_t fields, nvc_conn_t * conn_stat )
{
    NVIPFIX_TLOG_DEBUG( "%" PRId32 "-> %" PRId32, conn_stat->conn_client_switch_port, conn_stat->conn_server_switch_port );
}

nvIPFIX_data_record_list_t * nvipfix_import_nvc( const nvIPFIX_CHAR * a_host, 
    const nvIPFIX_CHAR * a_login, const nvIPFIX_CHAR * a_password,
	const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs )
{
    NVIPFIX_INIT_ERROR( error );

    nvOS_io_t io = { 0 };

    if (a_host != NULL) {
        CRYPTO_malloc_init();
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
        
        nvc_init_net( &io, a_host );
    }
    else {
		nvc_init( &io );
    }
    
    nvOS_result_t nvcResult;
	int nvcError;

	nvcError = nvc_connect( &io );

    NVIPFIX_RAISE_ERROR_IF( nvcError != 0, error, NV_IPFIX_ERROR_CODE_NVC_CONNECT, Connect );    
    
	if (a_login != NULL && a_password != NULL) {
		nvcError = nvc_authenticate( &io, a_login, a_password, &nvcResult );
	} 
    else {
		char userName[nvc_PCL_NAME_LEN];
		nvcError = nvc_check_uid( &io, userName, sizeof userName, &nvcResult );
	}

    NVIPFIX_RAISE_ERROR_IF( nvcError != 0, error, NV_IPFIX_ERROR_CODE_NVC_AUTH, Auth );
    
    nvIPFIX_data_record_list_t * result = NULL;

    nvc_conn_t filter = { 0 };
    uint64_t filterFields = 0;
    nvc_format_args_t format = { 0 };
    uint64_t formatFields = 0;
    nvcError = nvc_show_conn_stat( &io, 
        filterFields, &filter, 
        formatFields, &format,
        nvipfix_import_conn_stat_handler, result,
		&nvcResult );

    nvc_logout( &io );

    NVIPFIX_ERROR_HANDLER( Auth );
	nvc_disconnect( &io );
    
    NVIPFIX_ERROR_HANDLER( Connect );
	nvc_done( &io );
    
    return result;
}

#endif

bool nvipfix_import_parse_ingress( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	return nvipfix_parse_u32( a_s, a_value );
}

bool nvipfix_import_parse_egress( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	return nvipfix_parse_u32( a_s, a_value );
}

bool nvipfix_import_parse_layer2( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	bool result = nvipfix_parse_u64( a_s, a_value );

	if (result) {
		nvIPFIX_U64 * u64Ptr = a_value;
		*u64Ptr |= ((nvIPFIX_U64)NV_IPFIX_LAYER2_NETWORK_TYPE_VxLAN << 56);
	}

	return result;
}

bool nvipfix_import_parse_tcp_control_flags( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	bool result = true;
	nvIPFIX_TCP_CONTROL_FLAG * flagPtr = a_value;

	if (strcmp( "fin", a_s ) == 0) {
		*flagPtr = NV_IPFIX_TCP_CONTROL_FLAG_FIN;
	}
	else if (strcmp( "rst", a_s ) == 0) {
		*flagPtr = NV_IPFIX_TCP_CONTROL_FLAG_RST;
	}
	else if (strcmp( "syn", a_s ) == 0) {
		*flagPtr = NV_IPFIX_TCP_CONTROL_FLAG_SYN;
	}
	else if (strcmp( "est", a_s ) == 0) {
		*flagPtr = NV_IPFIX_TCP_CONTROL_FLAG_ACK;
	}
	else {
		result = false;
	}

	return result;
}

bool nvipfix_import_parse_protocol( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	nvIPFIX_OCTET protocol;
	bool result = nvipfix_parse_octet( a_s, &protocol );

	if (result) {
		*((nvIPFIX_PROTOCOL *)a_value) = (nvIPFIX_PROTOCOL)protocol;
	}

	return result;
}

bool nvipfix_import_parse_ethernet_type( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	nvIPFIX_U16 type;
	bool result = nvipfix_parse_u16( a_s, &type );

	if (result) {
		*((nvIPFIX_ETHERNET_TYPE *)a_value) = (nvIPFIX_ETHERNET_TYPE)type;
	}

	return result;
}

const nvIPFIX_import_item_t * nvipfix_import_get_item( const char * a_name )
{
	const nvIPFIX_import_item_t * result = NULL;

	for (size_t i = 0; i < ItemsCount; i++) {
		if (strcmp( Items[i].name, a_name ) == 0) {
			result = Items + i;
			break;
		}
	}

	return result;
}
