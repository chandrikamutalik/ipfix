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

#ifndef NVIPFIX_DEF_TEST


#define _BSD_SOURCE


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "include/types.h"
#include "include/log.h"
#include "include/error.h"
#include "include/main.h"

#ifdef NVIPFIX_DEF_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#endif


#define	NV_IPFIX_VERSION_MAJOR 0
#define NV_IPFIX_VERSION_MINOR 0
#define NV_IPFIX_VERSION_REV 1

#define NVIPFIX_VERSION( a_ver_major, a_ver_minor, a_ver_rev ) (#a_ver_major "." #a_ver_minor "." #a_ver_rev)
#define NVIPFIX_VERSION_TO_STRING( a_ver_major, a_ver_minor, a_ver_rev ) NVIPFIX_VERSION( a_ver_major, a_ver_minor, a_ver_rev )
#define NVIPFIX_VERSION_STRING NVIPFIX_VERSION_TO_STRING( NV_IPFIX_VERSION_MAJOR, NV_IPFIX_VERSION_MINOR, NV_IPFIX_VERSION_REV )


enum {
	NV_IPFIX_RETURN_CODE_OK = 0,
	NV_IPFIX_RETURN_CODE_ARGS_ERROR,
	NV_IPFIX_RETURN_CODE_CONFIGURATION_ERROR,
	NV_IPFIX_RETURN_CODE_DATA_ERROR,
	NV_IPFIX_RETURN_CODE_START_DAEMON_ERROR
};


NVIPFIX_TIMESPAN_INIT_FROM_SECONDS( ExportDuration, 1 );


int main_daemon( void );


void Usage()
{
	printf( "nvIPFIX %s\n", NVIPFIX_VERSION_STRING );
	puts( "Usage: nvIPFIX [[-fdatafile] <start_ts> <end_ts>] [start|stop]" );
	puts( "\tdatafile - JSON file (for debug purpose)" );
	puts( "\tstart_ts/end_ts - ISO 8601 datetime (YYYY-MM-DDTHH:mm:SS)" );
}

int main( int argc, char * argv[] )
{
#ifdef NVIPFIX_DEF_POSIX
	char * appPath = realpath( argv[0], NULL );
	NVIPFIX_LOG_DEBUG( "full path = %s", appPath );
	free( appPath );
#endif

	if (argc < 2) {
		Usage();
		return NV_IPFIX_RETURN_CODE_ARGS_ERROR;
	}

	size_t argIndexTs = 1;
	bool useFile = false;

	if (strncmp( "-f", argv[1], 2 ) == 0) {
		argIndexTs = 2;
		useFile = true;
	}
	else if (strcmp( "start", argv[1] ) == 0) {
		return main_daemon();
	}

	nvIPFIX_datetime_t startTs = { 0 };
	nvIPFIX_datetime_t endTs = { 0 };

	if (!nvipfix_parse_datetime_iso8601( argv[argIndexTs], &startTs )
			|| !nvipfix_parse_datetime_iso8601( argv[argIndexTs + 1], &endTs )) {

		nvipfix_log_error( "invalid arguments" );
		Usage();
		return NV_IPFIX_RETURN_CODE_ARGS_ERROR;
	}

	if (useFile) {
		nvipfix_main_export_file( argv[1] + 2, &startTs, &endTs );
	}
	else {
		nvipfix_main_export_nvc( &startTs, &endTs );
	}

	return NV_IPFIX_RETURN_CODE_OK;
}

int main_daemon( void )
{
	int result = NV_IPFIX_RETURN_CODE_OK;

#ifdef NVIPFIX_DEF_POSIX
	pid_t pid = fork();

	if (pid < 0) {
		result = NV_IPFIX_RETURN_CODE_START_DAEMON_ERROR;
	}
	else if (pid == 0) {
		umask( 0 );
		pid_t sid = setsid();

		if (sid < 0) {
			result = NV_IPFIX_RETURN_CODE_START_DAEMON_ERROR;
		}
		else {
			struct rlimit rlimit = { 0 };

			if (getrlimit( RLIMIT_NOFILE, &rlimit ) == 0) {
				if (rlimit.rlim_max == RLIM_INFINITY) {
					rlimit.rlim_max = FOPEN_MAX;
				}

				for (int i = 0; i < rlimit.rlim_max; i++) {
					close( i );
				}
			}
			else {
				close( STDIN_FILENO );
				close( STDOUT_FILENO );
				close( STDERR_FILENO );
			}

			int stdFileHnadle = open( "/dev/null", O_RDWR );

			if (stdFileHnadle == 0) {
				dup( stdFileHnadle );
				dup( stdFileHnadle );
			}

			time_t startT = time( NULL );

			while (true) {
				time_t nowT = time( NULL );
				double timeDiff = difftime( nowT, startT );
				int waitSeconds = NVIPFIX_TIMESPAN_GET_SECONDS( &ExportDuration ) - timeDiff;

				if (waitSeconds > 0) {
					sleep( waitSeconds );

					nowT = time( NULL );
				}

				nvIPFIX_datetime_t startTs = { 0 };
				nvIPFIX_datetime_t endTs = { 0 };

				if (nvipfix_ctime_to_datetime( &startTs, &startT ) &&  nvipfix_ctime_to_datetime( &endTs, &nowT )) {
					nvipfix_main_export_nvc( &startTs, &endTs );
				}

				startT = nowT;
			}
		}
	}
#endif

	return result;
}


#endif // NVIPFIX_DEF_TEST
