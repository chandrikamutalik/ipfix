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

#ifdef NVIPFIX_DEF_TEST


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "include/types.h"
#include "include/log.h"
#include "include/config.h"


#define NVIPFIX_TEST_LOG_RESULT( a_result, a_failResult, a_testResult, a_fmt, ... ) \
	{ bool r = (a_testResult); a_result |= !r ? a_failResult : 0; \
	printf( "[%s] %s: " a_fmt, __func__, r ? "PASSED" : "FAILED", __VA_ARGS__ ); }


void HashtableFree( const void * a_value )
{
	puts( "HashtableFree" );
}

const void * HashtableCopy( const void * a_value )
{
	puts( "HashtableCopy" );

	return a_value;
}

int TestHashtable8( void )
{
#define fmt "key = %s, value = %s\n"

	int result = 0;

	nvIPFIX_hashtable8_t * table = nvipfix_hashtable8_add( NULL, "a", 1, "a1", HashtableCopy, HashtableFree );

	if (table != NULL) {
		NVIPFIX_HASHTABLE8_ADD( table, "b", 1, "b1" );
		NVIPFIX_HASHTABLE8_ADD( table, "c", 1, "c1" );
		NVIPFIX_HASHTABLE8_ADD( table, "d", 1, "d1" );
		NVIPFIX_HASHTABLE8_ADD( table, "e", 1, "e1" );
		NVIPFIX_HASHTABLE8_ADD( table, "bb", 2, "bb2" );
		NVIPFIX_HASHTABLE8_ADD( table, "ccc", 3, "ccc3" );
		NVIPFIX_HASHTABLE8_ADD( table, "dddd", 4, "dddd4" );

		char * key = "a";
		char * value = (char *)nvipfix_hashtable8_get( table, key, 1 );
		NVIPFIX_TEST_LOG_RESULT( result, 1, NVIPFIX_STREQUAL_CHECKED( value, "a1" ), fmt, key, value );

		key = "b";
		value = (char *)nvipfix_hashtable8_get( table, key, 1 );
		NVIPFIX_TEST_LOG_RESULT( result, 1, NVIPFIX_STREQUAL_CHECKED( value, "b1" ), fmt, key, value );

		key = "bb";
		value = (char *)nvipfix_hashtable8_get( table, key, 2 );
		NVIPFIX_TEST_LOG_RESULT( result, 1, NVIPFIX_STREQUAL_CHECKED( value, "bb2" ), fmt, key, value );

		key = "ccc";
		value = (char *)nvipfix_hashtable8_get( table, key, 3 );
		NVIPFIX_TEST_LOG_RESULT( result, 1, NVIPFIX_STREQUAL_CHECKED( value, "ccc3" ), fmt, key, value );

		key = "cca";
		value = (char *)nvipfix_hashtable8_get( table, key, 3 );

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
		NVIPFIX_TEST_LOG_RESULT( result, 1, NVIPFIX_STREQUAL_CHECKED( value, NULL ), fmt, key, value );
#pragma GCC diagnostic pop

		NVIPFIX_HASHTABLE8_ADD( table, "ccc", 3, "ccc3.new" );
		key = "ccc";
		value = (char *)nvipfix_hashtable8_get( table, key, 3 );
		NVIPFIX_TEST_LOG_RESULT( result, 1, NVIPFIX_STREQUAL_CHECKED( value, "ccc3.new" ), fmt, key, value );
	}
	else {
		puts( "nvipfix_hashtable8_add failed" );
		result = 1;
	}

	nvipfix_hashtable8_free( table );

	return result;
}

int TestConfig( void )
{
	int result = 0;
	size_t collectorsCount;
	nvIPFIX_collector_info_t * collectors = nvipfix_config_collectors_get( &collectorsCount );

	if (collectors != NULL) {
		NVIPFIX_TEST_LOG_RESULT( result, 2, collectorsCount == 2, "collectorsCount = %d\n", (int)collectorsCount );

		if (result == 0) {
			const char * name = collectors[0].name;
			NVIPFIX_TEST_LOG_RESULT( result, 2, NVIPFIX_STREQUAL_CHECKED( name, "fooCollector2" ),
					"collector: name = %s\n", name );

			const char * key = (const char *)collectors[0].key.value;
			NVIPFIX_TEST_LOG_RESULT( result, 2, NVIPFIX_STREQUAL_CHECKED( key, "{192.168.0.2}:{9992}" ),
					"collector: key = %s\n", key );
		}
	}
	else {
		result = 2;
	}

	nvipfix_config_collectors_free( collectors );

	return result;
}

int TestDatetime( void )
{
	int result = 0;
	nvIPFIX_datetime_t datetime = { 0 };
	time_t ctime = time( NULL );

	NVIPFIX_TEST_LOG_RESULT( result, 4, nvipfix_ctime_to_datetime( &datetime, &ctime ),
			"year = %d, month = %d, day = %d, hours = %d, minutes = %d, tz = %d\n",
			datetime.year, datetime.month, datetime.day, datetime.hours, datetime.minutes,
			(int)nvipfix_timespan_get_minutes( &(datetime.tzOffset) ) );

	time_t ctimeDt = nvipfix_datetime_to_ctime( &datetime );

	NVIPFIX_TEST_LOG_RESULT( result, 4, ctime == ctimeDt, "%f, %f\n",
			(double)ctime, (double)ctimeDt );

	NVIPFIX_TIMESPAN_INIT_FROM_SECONDS( timespan, -60 );
	NVIPFIX_TEST_LOG_RESULT( result, 4, nvipfix_datetime_add_timespan( &datetime, &timespan ) > (time_t)0,
			"year = %d, month = %d, day = %d, hours = %d, minutes = %d, tz = %d\n",
			datetime.year, datetime.month, datetime.day, datetime.hours, datetime.minutes,
			(int)nvipfix_timespan_get_minutes( &(datetime.tzOffset) ) );

	return result;
}

int main( int argc, char * argv[] )
{
	int rc = 0;
	rc = TestHashtable8();
	rc |= TestConfig();
	rc |= TestDatetime();

	printf( "test result = %d\n", rc );

	return rc;
}


#endif // NVIPFIX_DEF_TEST
