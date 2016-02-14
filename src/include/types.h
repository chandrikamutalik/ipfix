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

#ifndef __NVIPFIX_TYPES_H
#define __NVIPFIX_TYPES_H


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>


#if defined (__linux__) || defined (__unix)
#define NVIPFIX_DEF_POSIX
#define NVIPFIX_DEF_USE_INOTIFY
#endif

#define NVIPFIX_NULL_ARGS_GUARD_1_VOID( a ) { if ((a) == NULL) return; }
#define NVIPFIX_NULL_ARGS_GUARD_1( a, result ) { if ((a) == NULL) return (result); }
#define NVIPFIX_NULL_ARGS_GUARD_2_VOID( a, b ) { if ((a) == NULL || (b) == NULL) return; }
#define NVIPFIX_NULL_ARGS_GUARD_2( a, b, result ) { if ((a) == NULL || (b) == NULL) return (result); }

#define NVIPFIX_HOURS_PER_DAY 24
#define NVIPFIX_MINUTES_PER_HOUR 60
#define NVIPFIX_SECONDS_PER_MINUTE 60
#define NVIPFIX_MILLISECONDS_PER_SECOND 1000
#define NVIPFIX_MICROSECONDS_PER_MILLISECOND 1000
#define NVIPFIX_NANOSECONDS_PER_MICROSECOND 1000

#define NVIPFIX_TIMESPAN_INIT_FROM_SECONDS( a_varName, a_seconds ) \
	nvIPFIX_timespan_t a_varName = { \
			.microseconds = (a_seconds) *  NVIPFIX_MILLISECONDS_PER_SECOND * NVIPFIX_MICROSECONDS_PER_MILLISECOND, \
			.hasValue = true }

#define NVIPFIX_TIMESPAN_SET_SECONDS( a_varName, a_seconds ) \
	(a_varName).microseconds = (a_seconds) *  NVIPFIX_MILLISECONDS_PER_SECOND * NVIPFIX_MICROSECONDS_PER_MILLISECOND; \
	(a_varName).hasValue = true;

#define NVIPFIX_TIMESPAN_GET_SECONDS( a_varName ) \
		((a_varName)->microseconds / (NVIPFIX_MICROSECONDS_PER_MILLISECOND	* NVIPFIX_MILLISECONDS_PER_SECOND))

#define NVIPFIX_TIMESPAN_SET_NANOSECONDS( a_varName, a_nanoseconds ) \
	(a_varName).microseconds = (a_nanoseconds) / NVIPFIX_NANOSECONDS_PER_MICROSECOND; \
	(a_varName).hasValue = true;

#define NVIPFIX_ARGSF_IP_ADDRESS( a ) \
	(unsigned)(((a).value >> 24) & 0xff), \
	(unsigned)(((a).value >> 16) & 0xff), \
	(unsigned)(((a).value >> 8) & 0xff), \
	(unsigned)((a).value & 0xff)

#define NVIPFIX_ARGSF_MAC_ADDRESS( a ) \
	(unsigned)((a).octets[0]), \
	(unsigned)((a).octets[1]), \
	(unsigned)((a).octets[2]), \
	(unsigned)((a).octets[3]), \
	(unsigned)((a).octets[4]), \
	(unsigned)((a).octets[5])

#define NVIPFIX_CHAR_PTR_TO_CCHAR_PTR( a_ptr ) (char *)(a_ptr)

#define NVIPFIX_HASHTABLE8_ADD( a_table, a_key, a_keyLen, a_value ) \
	nvipfix_hashtable8_add( a_table, a_key, a_keyLen, a_value, NULL, NULL )

#define NVIPFIX_STRLEN_CHECKED( a_str ) ((a_str) != NULL ? strlen( a_str ) : 0)
#define NVIPFIX_STREQUAL_CHECKED( a_str1, a_str2 ) ((a_str1) == NULL ? ((a_str2) == NULL) \
	: ((a_str2) != NULL && strcmp( a_str1, a_str2 ) == 0) )


enum {
	NV_IPFIX_SIZE_STRING_IP_ADDRESS = 3 * 4 + 3 + 1
};


typedef char nvIPFIX_CHAR;

#ifdef NVIPFIX_DEF_UNICODE
typedef wchar_t nvIPFIX_TCHAR;
#define NVIPFIX_F_WS "%ls"
#define NVIPFIX_F_S "%s"
#define NVIPFIX_F_T "%ls"
#define NVIPFIX_T( a ) (L ## a)
#else // !NVIPFIX_DEF_UNICODE
typedef char nvIPFIX_TCHAR;
#define NVIPFIX_F_WS "%ls"
#define NVIPFIX_F_S "%s"
#define NVIPFIX_F_T "%s"
#define NVIPFIX_T( a ) (a)
#endif // NVIPFIX_DEF_UNICODE

typedef uint8_t nvIPFIX_OCTET;
typedef unsigned char nvIPFIX_BYTE;
typedef uint16_t nvIPFIX_U16;
typedef uint32_t nvIPFIX_U32;
typedef uint64_t nvIPFIX_U64;
typedef int64_t nvIPFIX_I64;

typedef struct {
	nvIPFIX_I64 microseconds;

	bool hasValue;
} nvIPFIX_timespan_t;

typedef struct {
	int year;
	int month;
	int day;
	int hours;
	int minutes;
	int seconds;
	int milliseconds;
	nvIPFIX_timespan_t tzOffset;

	bool hasValue;
} nvIPFIX_datetime_t;

typedef enum {
	NV_IPFIX_ADDRESS_OCTETS_COUNT_IPV4 = 4,
	NV_IPFIX_ADDRESS_OCTETS_COUNT_IPV6 = 6,
	NV_IPFIX_ADDRESS_OCTETS_COUNT_MAC = 6
} nvIPFIX_ADDRESS_OCTETS_COUNT;

typedef struct {
	nvIPFIX_U32 value;

	bool hasValue;
} nvIPFIX_ip_address_t;

typedef struct {
	nvIPFIX_OCTET octets[NV_IPFIX_ADDRESS_OCTETS_COUNT_MAC];

	bool hasValue;
} nvIPFIX_mac_address_t;

typedef struct {
	nvIPFIX_CHAR * value;
	size_t len;
	size_t bufferLen;
} nvIPFIX_string_t;

typedef struct _nvIPFIX_string_list_item_t {
	const nvIPFIX_CHAR * value;
	struct _nvIPFIX_string_list_item_t * next;
	struct _nvIPFIX_string_list_item_t * prev;
} nvIPFIX_string_list_item_t;

typedef struct {
	nvIPFIX_string_list_item_t * head;
	nvIPFIX_string_list_item_t * tail;
	size_t count;
} nvIPFIX_string_list_t;

typedef struct {
	const nvIPFIX_BYTE * value;
	size_t len;
} nvIPFIX_hashtable_key_t;

typedef struct _nvIPFIX_hashnode_t {
	const nvIPFIX_BYTE * key;
	size_t keyLen;
	const void * value;
	struct _nvIPFIX_hashnode_t * lt;
	struct _nvIPFIX_hashnode_t * gt;
} nvIPFIX_hashnode_t;

typedef void (* nvIPFIX_hashtable_free_ft)( const void * );
typedef const void * (* nvIPFIX_hashtable_copy_ft)( const void * );

typedef struct {
	nvIPFIX_hashnode_t nodes[UINT8_MAX];
	nvIPFIX_hashtable_free_ft freeValue;
	nvIPFIX_hashtable_copy_ft copyValue;
} nvIPFIX_hashtable8_t;


/**
 * add string to a list
 * @param a_list pointer to a list (if NULL - new list will be created)
 * @param a_value string
 * @return
 */
nvIPFIX_string_list_t * nvipfix_string_list_add( nvIPFIX_string_list_t * a_list, const nvIPFIX_CHAR * a_value );

/**
 * add a copy of string to a list
 * @param a_list pointer to a list (if NULL - new list will be created)
 * @param a_value string
 * @return
 */
nvIPFIX_string_list_t * nvipfix_string_list_add_copy( nvIPFIX_string_list_t * a_list, const nvIPFIX_CHAR * a_value );

/**
 * free string list
 * @param a_list pointer pointer to a list
 * @param a_shouldFreeValues flag indicates if strings in a list should be freed too
 */
void nvipfix_string_list_free( nvIPFIX_string_list_t * a_list, bool a_shouldFreeValues );

/**
 *
 * @param a_hashtable
 * @param a_key
 * @param keyLen
 * @param a_value
 * @param a_copyF
 * @param a_freeF
 * @return
 */
nvIPFIX_hashtable8_t * nvipfix_hashtable8_add( nvIPFIX_hashtable8_t * a_hashtable,
		const void * a_key, size_t a_keyLen, const void * a_value,
		const nvIPFIX_hashtable_copy_ft a_copyF, const nvIPFIX_hashtable_free_ft a_freeF );

/**
 *
 * @param a_hashtable
 * @param a_key
 * @param a_keyLen
 */
const void * nvipfix_hashtable8_get( nvIPFIX_hashtable8_t * a_hashtable,
		const void * a_key, size_t a_keyLen );

/**
 *
 * @param a_hashtable
 */
void nvipfix_hashtable8_free( nvIPFIX_hashtable8_t * a_hashtable );

/**
 * split a string
 * @param a_s string
 * @param a_delimiters delimiters string
 * @return
 */
nvIPFIX_string_list_t * nvipfix_string_split( const nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_delimiters );

/**
 *
 * @param a_s
 * @param a_charset
 * @return
 */
bool nvipfix_string_contains_only( const nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_charset );

/**
 *
 * @param a_s
 * @return
 */
nvIPFIX_CHAR * nvipfix_string_duplicate( const nvIPFIX_CHAR * a_s );

/**
 * trim a string
 * @param a_s
 * @param a_trimChars
 * @return
 */
nvIPFIX_CHAR * nvipfix_string_trim( nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_trimChars );

/**
 * trim a string
 * @param a_s
 * @param a_trimChars
 * @return
 */
nvIPFIX_CHAR * nvipfix_string_trim_copy( const nvIPFIX_CHAR * a_s, const nvIPFIX_CHAR * a_trimChars );

/**
 * duplicate a string
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_string( const char * a_s, void * a_value );

/**
 * convert string to an int
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_int( const char * a_s, void * a_value );

/**
 * convert string to an unsigned int
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_unsigned( const char * a_s, void * a_value );

/**
 * convert string to an octet (nvIPFIX_OCTET)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_octet( const char * a_s, void * a_value );

/**
 * convert string to a byte (nvIPFIX_BYTE)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_byte( const char * a_s, void * a_value );

/**
 * convert string to an unsigned 16-bit integer (nvIPFIX_U16)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_u16( const char * a_s, void * a_value );

/**
 * convert string to an unsigned 32-bit integer (nvIPFIX_U32)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_u32( const char * a_s, void * a_value );

/**
 * convert string to a signed 64-bit integer (nvIPFIX_I64)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_i64( const char * a_s, void * a_value );

/**
 * convert string to an unsigned 64-bit integer (nvIPFIX_U64)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_u64( const char * a_s, void * a_value );

/**
 * convert string to an IP address (nvIPFIX_ip_address_t)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_ip_address( const char * a_s, void * a_value );

/**
 * convert string to a MAC address (nvIPFIX_mac_address_t)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_mac_address( const char * a_s, void * a_value );

/**
 * convert ISO 8601 string to a datetime (nvIPFIX_datetime_t)
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_datetime_iso8601( const char * a_s, void * a_value );

/**
 *
 * @param
 * @param
 * @return
 */
bool nvipfix_parse_timespan( const char * a_s, void * a_value );

/**
 *
 * @param
 * @param
 * @return
 */
bool nvipfix_parse_timespan_milliseconds( const char * a_s, void * a_value );

/**
 *
 * @param a_s
 * @param a_value
 * @return
 */
bool nvipfix_parse_timespan_microseconds( const char * a_s, void * a_value );

/**
 *
 * @param a_address
 * @return
 */
const nvIPFIX_CHAR * nvipfix_ip_address_to_string( const nvIPFIX_ip_address_t * a_address );

/**
 *
 * @param a_datetime
 * @param a_epoch_year 1900 -
 * @param a_epoch_month 1 - 12
 * @return
 */
nvIPFIX_U32 nvipfix_datetime_get_seconds_since_epoch( const nvIPFIX_datetime_t * a_datetime,
		int a_epoch_year, int a_epoch_month );

/**
 *
 * @param a_datetime
 * @param a_timespan
 */
time_t nvipfix_datetime_add_timespan( nvIPFIX_datetime_t * a_datetime, const nvIPFIX_timespan_t * a_timespan );

/**
 *
 * @param a_datetime
 * @return
 */
time_t nvipfix_datetime_to_ctime( const nvIPFIX_datetime_t * a_datetime );

/**
 *
 * @param a_datetime
 * @param a_ctime
 * @return
 */
bool nvipfix_ctime_to_datetime( nvIPFIX_datetime_t * a_datetime, const time_t * a_ctime );

/**
 *
 * @param a_timespan
 * @return
 */
nvIPFIX_I64 nvipfix_timespan_get_minutes( const nvIPFIX_timespan_t * a_timespan );

/**
 *
 * @param a_timespan
 * @return
 */
nvIPFIX_I64 nvipfix_timespan_get_seconds( const nvIPFIX_timespan_t * a_timespan );

/**
 *
 * @param a_timespan
 * @return
 */
nvIPFIX_I64 nvipfix_timespan_get_milliseconds( const nvIPFIX_timespan_t * a_timespan );

/**
 *
 * @param a_timespan
 * @return
 */
nvIPFIX_I64 nvipfix_timespan_get_microseconds( const nvIPFIX_timespan_t * a_timespan );


#endif /* __NVIPFIX_TYPES_H */
