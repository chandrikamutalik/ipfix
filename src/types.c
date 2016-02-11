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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "include/types.h"
#include "include/error.h"
#include "include/log.h"


#define NVIPFIX_PARSE_UINT( a_s, a_value, a_type ) { \
	unsigned long long ulValue = strtoull( a_s, NULL, 0 ); \
	*((a_type *)a_value) = (a_type)ulValue; \
	return true; }

#define NVIPFIX_PARSE_INT( a_s, a_value, a_type ) { \
	long long lValue = strtoll( a_s, NULL, 0 ); \
	*((a_type *)a_value) = (a_type)lValue; \
	return true; }

#define NVIPFIX_TM_INIT_FROM_DT( a_varName, a_datetime ) \
	struct tm a_varName = { 0 };	\
	a_varName.tm_year = (a_datetime)->year - 1900;	\
	a_varName.tm_mon = (a_datetime)->month - 1;	\
	a_varName.tm_mday = (a_datetime)->day;	\
	a_varName.tm_hour = (a_datetime)->hours;	\
	a_varName.tm_min = (a_datetime)->minutes;	\
	a_varName.tm_sec = (a_datetime)->seconds

#define NVIPFIX_DT_FROM_TM( a_datetime, a_tm ) \
	(a_datetime)->year = (a_tm).tm_year + 1900;	\
	(a_datetime)->month = (a_tm).tm_mon + 1;	\
	(a_datetime)->day = (a_tm).tm_mday;	\
	(a_datetime)->hours = (a_tm).tm_hour;	\
	(a_datetime)->minutes = (a_tm).tm_min;	\
	(a_datetime)->seconds = (a_tm).tm_sec;	\
	(a_datetime)->milliseconds = 0;	\
	NVIPFIX_TIMESPAN_SET_SECONDS( (a_datetime)->tzOffset, 0 )


enum {
	SizeofStringList = sizeof (nvIPFIX_string_list_t),
	SizeofStringListItem = sizeof (nvIPFIX_string_list_item_t),
	SizeofCharset = 1 << (sizeof (nvIPFIX_CHAR) * CHAR_BIT)
};


nvIPFIX_string_list_t * nvipfix_string_list_add( nvIPFIX_string_list_t * a_list, const nvIPFIX_CHAR * a_value )
{
	nvIPFIX_string_list_t * result = NULL;
	nvIPFIX_string_list_item_t * item = NULL;

	item = malloc( SizeofStringListItem );

	if (item != NULL) {
		item->next = NULL;
		item->value = a_value;

		if (a_list == NULL) {
			a_list = malloc( SizeofStringList );

			if (a_list == NULL) {
				free( item );
				return result;
			}

			item->prev = NULL;
			a_list->head = item;
			a_list->count = 0;
		}
		else {
			item->prev = a_list->tail;
			a_list->tail->next = item;
		}

		a_list->tail = item;
		a_list->count++;

		result = a_list;
	}

	return result;
}

nvIPFIX_string_list_t * nvipfix_string_list_add_copy( nvIPFIX_string_list_t * a_list, const nvIPFIX_CHAR * a_value )
{
	if (a_value != NULL) {
		nvIPFIX_CHAR * copy = nvipfix_string_duplicate( a_value );

		if (copy == NULL) {
			return NULL;
		}

		a_value = copy;
	}

	return nvipfix_string_list_add( a_list, a_value );
}

void nvipfix_string_list_free( nvIPFIX_string_list_t * a_list, bool a_shouldFreeValues )
{
	if (a_list != NULL) {
		nvIPFIX_string_list_item_t * item = a_list->head;

		while (item != NULL) {
			nvIPFIX_string_list_item_t * next = item->next;

			if (a_shouldFreeValues) {
				free( (void *)(item->value) );
			}

			free( item );

			item = next;
		}

		free( a_list );
	}
}

static uint8_t nvipfix_hashtable8_hash( const void * a_value, size_t a_len )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_value, 0 );

	uint8_t result = 0;

	const char * value = a_value;

	for (size_t i = 0; i < a_len; i++) {
		result = result ^ (value[i] + i);
	}

	return result;
}

static nvIPFIX_hashnode_t * nvipfix_hashtable8_get_or_add( nvIPFIX_hashtable8_t * a_hashtable,
		const void * a_key, size_t a_keyLen, bool a_shouldAdd )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_hashtable, a_key, NULL );

	uint8_t keyHash = nvipfix_hashtable8_hash( a_key, a_keyLen );
	nvIPFIX_hashnode_t * node = &(a_hashtable->nodes[keyHash]);
	nvIPFIX_hashnode_t * * parentNodePtr = NULL;

	if (node->keyLen != 0) {
		NVIPFIX_LOG_DEBUG0( "%s", "root node not empty" );

		while (node != NULL) {
			int cmp = a_keyLen - node->keyLen;

			if (cmp == 0) {
				cmp = memcmp( a_key, node->key, a_keyLen );

				if (cmp == 0) {
					break;
				}
			}

			if (cmp < 0) {
				parentNodePtr = &(node->lt);
				node = node->lt;
			}
			else {
				parentNodePtr = &(node->gt);
				node = node->gt;
			}
		}
	}
	else if (a_shouldAdd) {
		NVIPFIX_LOG_DEBUG0( "%s", "root node empty. adding..." );

		node->key = malloc( a_keyLen );

		if (node->key != NULL) {
			node->keyLen = a_keyLen;
			memcpy( (void *)node->key, a_key, a_keyLen );
		}
		else {
			node = NULL;
			a_shouldAdd = false;
		}
	}
	else {
		NVIPFIX_LOG_DEBUG0( "%s", "root node empty" );

		node = NULL;
	}

	if (node == NULL && a_shouldAdd) {
		NVIPFIX_LOG_DEBUG0( "%s", "node empty. adding..." );

		node = malloc( sizeof (nvIPFIX_hashnode_t) + a_keyLen );

		if (node != NULL) {
			node->gt = NULL;
			node->lt = NULL;
			node->keyLen = a_keyLen;
			node->key = (const void *)(((char *)node) + sizeof (nvIPFIX_hashnode_t));
			NVIPFIX_LOG_DEBUG0( "%s", "copying key..." );
			memcpy( (void *)node->key, a_key, a_keyLen );
			node->value = NULL;

			if (parentNodePtr != NULL) {
				*parentNodePtr = node;
			}
		}
	}

	return node;
}

nvIPFIX_hashtable8_t * nvipfix_hashtable8_add( nvIPFIX_hashtable8_t * a_hashtable,
		const void * a_key, size_t a_keyLen, const void * a_value,
		const nvIPFIX_hashtable_copy_ft a_copyF, const nvIPFIX_hashtable_free_ft a_freeF )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_key, NULL );

	nvIPFIX_hashtable8_t * result = a_hashtable;

	if (result == NULL) {
		NVIPFIX_LOG_DEBUG0( "%s", "new table" );
		result = malloc( sizeof (nvIPFIX_hashtable8_t) );

		if (result != NULL) {
			memset( result, 0, sizeof (nvIPFIX_hashtable8_t) );
			result->copyValue = a_copyF;
			result->freeValue = a_freeF;
		}
		else {
			NVIPFIX_LOG_DEBUG0( "%s", "new table malloc failed" );
		}
	}

	NVIPFIX_NULL_ARGS_GUARD_1( result, NULL );

	nvIPFIX_hashnode_t * node = nvipfix_hashtable8_get_or_add( result, a_key, a_keyLen, true );

	if (node != NULL) {
		if (node->value != NULL && result->freeValue != NULL) {
			result->freeValue( node->value );
		}

		if (result->copyValue != NULL) {
			node->value = result->copyValue( a_value );
		}
		else {
			node->value = a_value;
		}
	}

	if (node == NULL || (a_value != NULL && node->value == NULL)) {
		if (a_hashtable == NULL) {
			free( result );
		}

		result = NULL;
	}

	return result;
}

const void * nvipfix_hashtable8_get( nvIPFIX_hashtable8_t * a_hashtable,
		const void * a_key, size_t a_keyLen )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_hashtable, a_key, NULL );

	const void * result = NULL;

	nvIPFIX_hashnode_t * node = nvipfix_hashtable8_get_or_add( a_hashtable, a_key, a_keyLen, false );

	if (node != NULL) {
		result = node->value;
	}

	return result;
}

static void nvipfix_hashtable8_free_node( nvIPFIX_hashtable8_t * a_hashtable, nvIPFIX_hashnode_t * a_node, bool a_isRoot )
{
	NVIPFIX_NULL_ARGS_GUARD_1_VOID( a_node );

	nvipfix_hashtable8_free_node( a_hashtable, a_node->lt, false );
	nvipfix_hashtable8_free_node( a_hashtable, a_node->gt, false );

	if (a_hashtable->freeValue != NULL) {
		a_hashtable->freeValue( a_node->value );
	}

	if (a_isRoot) {
		free( (void *)a_node->key );
	}
	else {
		free( a_node );
	}
}

void nvipfix_hashtable8_free( nvIPFIX_hashtable8_t * a_hashtable )
{
	NVIPFIX_NULL_ARGS_GUARD_1_VOID( a_hashtable );

	for (size_t i = 0; i < UINT8_MAX; i++) {
		if (a_hashtable->nodes[i].keyLen != 0) {
			nvipfix_hashtable8_free_node( a_hashtable, &(a_hashtable->nodes[i]), true );
		}
	}
}

nvIPFIX_string_list_t * nvipfix_string_split( const nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_delimiters )
{
	nvIPFIX_string_list_t * result = NULL;

	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_delimiters, NULL );

	bool delimitersMap[SizeofCharset] = { false };
	size_t delimLen = strlen( a_delimiters );

	for (size_t i = 0; i < delimLen; i++) {
		delimitersMap[(size_t)(a_delimiters[i])] = true;
	}

	size_t sLen = strlen( a_s );
	size_t tokenStart = 0;
	size_t tokenLength = 0;
	bool isTokenReady = false;

	for ( size_t i = 0; i < sLen; i++ ) {
		if (delimitersMap[(size_t)(a_s[i])]) {
			if (tokenLength > 0) {
				isTokenReady = true;
			}
			else {
				tokenStart = i + 1;
			}
		}
		else {
			tokenLength++;
		}

		if (i == (sLen - 1)) {
			isTokenReady = true;
		}

		if (isTokenReady) {
			nvIPFIX_CHAR * buffer = malloc( sizeof (nvIPFIX_CHAR) * (tokenLength + 1) );

			if (buffer == NULL) {
				nvipfix_string_list_free( result, true );
				result = NULL;
				break;
			}

			strncpy( buffer, a_s + tokenStart, tokenLength );
			buffer[tokenLength] = '\0';

			nvIPFIX_string_list_t * list = nvipfix_string_list_add( result, buffer );

			if (list == NULL) {
				nvipfix_string_list_free( result, true );
				free( buffer );
				result = NULL;
				break;
			}

			result = list;
			tokenLength = 0;
			tokenStart = i + 1;
			isTokenReady = false;
		}
	}

	return result;
}

bool nvipfix_string_contains_only( const nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_charset )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_charset, false );

	bool result = false;
	bool charsetMap[SizeofCharset] = { false };

	size_t sLen = strlen( a_charset );

	for (size_t i = 0; i < sLen; i++) {
		charsetMap[(size_t)(a_charset[i])] = true;
	}

	result = true;
	sLen = strlen( a_s );

	for (size_t i = 0; i < sLen; i++) {
		if (!charsetMap[(size_t)(a_s[i])]) {
			result = false;
			break;
		}
	}

	return result;
}

nvIPFIX_CHAR * nvipfix_string_trim( nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_trimChars )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_s, NULL );

	if (a_trimChars == NULL) {
		a_trimChars = " \t\r\n";
	}

	bool charsetMap[SizeofCharset] = { false };

	size_t sLen = strlen( a_trimChars );

	for (size_t i = 0; i < sLen; i++) {
		charsetMap[(size_t)(a_trimChars[i])] = true;
	}

	for (size_t i = strlen( a_s ); i > 0; i--) {
		if (!charsetMap[(size_t)(a_s[i - 1])]) {
			break;
		}

		a_s[i - 1] = '\0';
	}

	size_t srcI = 0;

	while (a_s[srcI] != '\0' && charsetMap[(size_t)(a_s[srcI])]) {
		srcI++;
	}

	if (srcI > 0) {
		sLen = strlen( a_s + srcI );
		memmove( a_s, a_s + srcI, sLen );
		a_s[sLen] = '\0';
	}

	return a_s;
}

nvIPFIX_CHAR * nvipfix_string_trim_copy( const nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_trimChars ) {
	nvIPFIX_CHAR * result = NULL;
	nvIPFIX_CHAR * s = nvipfix_string_duplicate( a_s );

	if (s != NULL) {
		result = nvipfix_string_trim( s, a_trimChars );
		result = realloc( result, strlen( result ) + 1 );

		if (result == NULL) {
			free( s );
		}
	}

	return result;
}

nvIPFIX_CHAR * nvipfix_string_duplicate( const nvIPFIX_CHAR * a_s )
{
	if (a_s == NULL) {
		return NULL;
	}

	nvIPFIX_CHAR * result = malloc( sizeof (nvIPFIX_CHAR) * (strlen( a_s ) + 1) );

	if (result != NULL) {
		strcpy( result, a_s );
	}

	return result;
}

bool nvipfix_parse_string( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	nvIPFIX_CHAR * value = nvipfix_string_duplicate( a_s );

	*((nvIPFIX_CHAR * *)a_value) = value;

	return (value != NULL);
}

bool nvipfix_parse_int( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_INT( a_s, a_value, int );
}

bool nvipfix_parse_unsigned( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_UINT( a_s, a_value, unsigned );
}

bool nvipfix_parse_octet( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_UINT( a_s, a_value, nvIPFIX_OCTET );
}

bool nvipfix_parse_byte( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_UINT( a_s, a_value, nvIPFIX_BYTE );
}

bool nvipfix_parse_u16( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_UINT( a_s, a_value, nvIPFIX_U16 );
}

bool nvipfix_parse_u32( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_UINT( a_s, a_value, nvIPFIX_U32 );
}

bool nvipfix_parse_i64( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_INT( a_s, a_value, nvIPFIX_I64 );
}

bool nvipfix_parse_u64( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	NVIPFIX_PARSE_UINT( a_s, a_value, nvIPFIX_U64 );
}

bool nvipfix_parse_ip_address( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	bool result = false;

	nvIPFIX_string_list_t * tokens = nvipfix_string_split( a_s, "." );

	if (tokens == NULL) {
		return result;
	}

	nvIPFIX_ip_address_t * address = a_value;

	if (tokens->count == NV_IPFIX_ADDRESS_OCTETS_COUNT_IPV4) {
		nvIPFIX_string_list_item_t * token = tokens->head;

		address->value = 0;
		size_t index = 0;

		while (token != NULL) {
			index++;
			address->value |= ((nvIPFIX_OCTET)strtoul( token->value, NULL, 0 )) << (32 - 8 * index);

			token = token->next;
		}

		result = (index == NV_IPFIX_ADDRESS_OCTETS_COUNT_IPV4);
	}

	nvipfix_string_list_free( tokens, true );

	address->hasValue = result;

	return result;
}

bool nvipfix_parse_mac_address( const char * a_s, void * a_value )
{
	bool result = false;

	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	nvIPFIX_string_list_t * tokens = nvipfix_string_split( a_s, ":" );

	if (tokens == NULL) {
		return result;
	}

	nvIPFIX_mac_address_t * address = a_value;

	if (tokens->count == NV_IPFIX_ADDRESS_OCTETS_COUNT_MAC) {
		nvIPFIX_string_list_item_t * token = tokens->head;

		size_t index = 0;

		while (token != NULL) {
			address->octets[index] = (nvIPFIX_OCTET)strtoul( token->value, NULL, 16 );
			index++;
			token = token->next;
		}

		result = (index == NV_IPFIX_ADDRESS_OCTETS_COUNT_MAC);
	}

	nvipfix_string_list_free( tokens, true );

	address->hasValue = result;

	return result;
}

bool nvipfix_parse_datetime_iso8601( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	bool result = false;
	nvIPFIX_string_list_t * tokens = nvipfix_string_split( a_s, "T" );
	nvIPFIX_datetime_t * datetime = a_value;

	if (tokens != NULL) {
		if (tokens->count == 2) {
			nvIPFIX_string_list_t * dateTokens = nvipfix_string_split( tokens->head->value, "-" );

			if (dateTokens != NULL) {
				if (dateTokens->count == 3) {
					unsigned value;

					if (nvipfix_parse_unsigned( dateTokens->head->value, &value )) {
						datetime->year = value;

						if (nvipfix_parse_unsigned( dateTokens->head->next->value, &value )) {
							datetime->month = value;

							if (nvipfix_parse_unsigned( dateTokens->tail->value, &value )) {
								datetime->day = value;

								nvIPFIX_string_list_t * timeTokens = nvipfix_string_split( tokens->tail->value, ":" );

								if (timeTokens != NULL) {
									if (timeTokens->count == 3) {

										if (nvipfix_parse_unsigned( timeTokens->head->value, &value )) {
											datetime->hours = value;

											if (nvipfix_parse_unsigned( timeTokens->head->next->value, &value )) {
												datetime->minutes = value;

												if (nvipfix_parse_unsigned( timeTokens->tail->value, &value )) {
													datetime->seconds = value;
													datetime->milliseconds = 0;

													result = true;
												}
											}
										}
									}

									nvipfix_string_list_free( timeTokens, true );
								}
							}
						}
					}
				}

				nvipfix_string_list_free( dateTokens, true );
			}
		}

		nvipfix_string_list_free( tokens, true );
	}

	datetime->hasValue = result;
	NVIPFIX_TIMESPAN_SET_SECONDS( datetime->tzOffset, 0 );

	return result;
}

bool nvipfix_parse_timespan_milliseconds( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	int milliseconds;
	bool result = nvipfix_parse_int( a_s, &milliseconds );

	nvIPFIX_timespan_t * timespan = a_value;

	if (result) {
		timespan->microseconds = (nvIPFIX_I64)milliseconds * NVIPFIX_MICROSECONDS_PER_MILLISECOND;
	}

	timespan->hasValue = result;

	return result;
}

bool nvipfix_parse_timespan_microseconds( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	nvIPFIX_I64 microseconds;
	bool result = nvipfix_parse_i64( a_s, &microseconds );

	nvIPFIX_timespan_t * timespan = a_value;

	if (result) {
		timespan->microseconds = microseconds;
	}

	timespan->hasValue = result;

	return result;
}

const nvIPFIX_CHAR * nvipfix_ip_address_to_string( const nvIPFIX_ip_address_t * a_address )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_address, NULL );

	char buffer[NV_IPFIX_SIZE_STRING_IP_ADDRESS];
	nvIPFIX_CHAR * result = NULL;

	if (snprintf( buffer, NV_IPFIX_SIZE_STRING_IP_ADDRESS, "%d.%d.%d.%d", NVIPFIX_ARGSF_IP_ADDRESS( *a_address ) ) > 0) {
		buffer[NV_IPFIX_SIZE_STRING_IP_ADDRESS - 1] = '\0';
		result = nvipfix_string_duplicate( buffer );
	}

	return result;
}

nvIPFIX_U32 nvipfix_datetime_get_seconds_since_epoch( const nvIPFIX_datetime_t * a_datetime, int a_epoch_year, int a_epoch_month )
{
	nvIPFIX_U32 result = 0;

	struct tm epoch = { 0 };
	epoch.tm_year = a_epoch_year - 1900;
	epoch.tm_mon = a_epoch_month - 1;
	epoch.tm_mday = 1;

	NVIPFIX_TM_INIT_FROM_DT( datetime, a_datetime );

	result = difftime( mktime( &datetime ), mktime( &epoch ) );

	return result;
}

time_t nvipfix_datetime_to_ctime( const nvIPFIX_datetime_t * a_datetime )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_datetime, (time_t)-1 );

	NVIPFIX_TM_INIT_FROM_DT( datetime, a_datetime );

	return mktime( &datetime );
}

bool nvipfix_ctime_to_datetime( nvIPFIX_datetime_t * a_datetime, const time_t * a_ctime )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_datetime, a_ctime, false );

	bool result = true;
	struct tm datetime;
	struct tm datetimeUtc;

	#pragma omp critical (nvipfixCritical_gmtime)
	{
		datetimeUtc = *(gmtime( a_ctime ));
		datetime = *(localtime( a_ctime ));
	}

	NVIPFIX_DT_FROM_TM( a_datetime, datetime );
	NVIPFIX_TIMESPAN_SET_SECONDS( a_datetime->tzOffset, difftime( mktime( &datetimeUtc ), *a_ctime ) );

	return result;
}

time_t nvipfix_datetime_add_timespan( nvIPFIX_datetime_t * a_datetime, const nvIPFIX_timespan_t * a_timespan )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_datetime, a_timespan, (time_t)-1 );

	NVIPFIX_TM_INIT_FROM_DT( datetime, a_datetime );
	datetime.tm_sec += a_timespan->microseconds / (NVIPFIX_MICROSECONDS_PER_MILLISECOND * NVIPFIX_MILLISECONDS_PER_SECOND);

	time_t result = mktime( &datetime );

	NVIPFIX_DT_FROM_TM( a_datetime, datetime );

	return result;
}

nvIPFIX_I64 nvipfix_timespan_get_minutes( const nvIPFIX_timespan_t * a_timespan )
{
	nvIPFIX_I64 result = 0;

	NVIPFIX_NULL_ARGS_GUARD_1( a_timespan, result );

	if (a_timespan->hasValue) {
		result = a_timespan->microseconds / (NVIPFIX_MICROSECONDS_PER_MILLISECOND
				* NVIPFIX_MILLISECONDS_PER_SECOND
				* NVIPFIX_SECONDS_PER_MINUTE);
	}

	return result;
}

nvIPFIX_I64 nvipfix_timespan_get_seconds( const nvIPFIX_timespan_t * a_timespan )
{
	nvIPFIX_I64 result = 0;

	NVIPFIX_NULL_ARGS_GUARD_1( a_timespan, result );

	if (a_timespan->hasValue) {
		result = NVIPFIX_TIMESPAN_GET_SECONDS( a_timespan );
	}

	return result;
}

nvIPFIX_I64 nvipfix_timespan_get_milliseconds( const nvIPFIX_timespan_t * a_timespan )
{
	nvIPFIX_I64 result = 0;

	NVIPFIX_NULL_ARGS_GUARD_1( a_timespan, result );

	if (a_timespan->hasValue) {
		result = a_timespan->microseconds / NVIPFIX_MICROSECONDS_PER_MILLISECOND;
	}

	return result;
}

nvIPFIX_I64 nvipfix_timespan_get_microseconds( const nvIPFIX_timespan_t * a_timespan )
{
	nvIPFIX_I64 result = 0;

	NVIPFIX_NULL_ARGS_GUARD_1( a_timespan, result );

	if (a_timespan->hasValue) {
		result = a_timespan->microseconds;
	}

	return result;
}
