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
#include <errno.h>

#include "include/types.h"
#include "include/log.h"
#include "include/error.h"
#include "include/config.h"
#include "include/main.h"

#ifdef NVIPFIX_DEF_POSIX
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#endif


#define	NV_IPFIX_VERSION_MAJOR 0
#define NV_IPFIX_VERSION_MINOR 0
#define NV_IPFIX_VERSION_REV 4

#define NVIPFIX_VERSION( a_ver_major, a_ver_minor, a_ver_rev ) (#a_ver_major "." #a_ver_minor "." #a_ver_rev)
#define NVIPFIX_VERSION_TO_STRING( a_ver_major, a_ver_minor, a_ver_rev ) NVIPFIX_VERSION( a_ver_major, a_ver_minor, a_ver_rev )
#define NVIPFIX_VERSION_STRING NVIPFIX_VERSION_TO_STRING( NV_IPFIX_VERSION_MAJOR, NV_IPFIX_VERSION_MINOR, NV_IPFIX_VERSION_REV )


enum {
	NV_IPFIX_RETURN_CODE_OK = 0,
	NV_IPFIX_RETURN_CODE_ARGS_ERROR,
	NV_IPFIX_RETURN_CODE_CONFIGURATION_ERROR,
	NV_IPFIX_RETURN_CODE_DATA_ERROR,
	NV_IPFIX_RETURN_CODE_START_DAEMON_ERROR,
	NV_IPFIX_RETURN_CODE_DAEMON_ALREADY_RUNNING,
	NV_IPFIX_RETURN_CODE_DAEMON_SHARED_MEMORY_ERROR,
};


int main_daemon( char * a_shmName );
char * shm_name_escape( char * a_shmName );


void Usage()
{
	printf( "nvIPFIX %s\n", NVIPFIX_VERSION_STRING );
	puts( "Usage: nvIPFIX start|stop" );
    puts( "\tstart - start nvIPFIX daemon" );
    puts( "\tstop - stop nvIPFIX daemon" );
    puts( "Usage: nvIPFIX [-fdatafile] <start_ts> <end_ts>" );
	puts( "\tdatafile - data file in JSON format (for debug purpose)" );
	puts( "\tstart_ts/end_ts - ISO 8601 datetime (YYYY-MM-DDTHH:mm:SS)" );
}

int main( int argc, char * argv[] )
{
	char *appPath;

	if (argc < 2) {
		Usage();
		return NV_IPFIX_RETURN_CODE_ARGS_ERROR;
	}

	size_t argIndexTs = 1;
	bool useFile = false;

	appPath = (char *) malloc(FILENAME_MAX);

	if (strncmp( "-f", argv[1], 2 ) == 0) {
		argIndexTs = 2;
		useFile = true;
	}
	else if (strcmp( "start", argv[1] ) == 0) {
#ifdef NVIPFIX_DEF_POSIX
		if (realpath( argv[0], appPath ) == NULL) {
			nvipfix_log_error("Failed to resolve path of binary");
			return NV_IPFIX_RETURN_CODE_ARGS_ERROR;
		}
#endif

		int rc = main_daemon( appPath );

#ifdef NVIPFIX_DEF_POSIX
		free( appPath );
#endif

		return rc;
	}
	else if (strcmp( "stop", argv[1] ) == 0) {
#ifdef NVIPFIX_DEF_POSIX
		appPath = shm_name_escape( realpath( argv[0], appPath) );

		int shmHandle = shm_open( appPath, O_RDWR, 0 );

		if (shmHandle < 0) {
			nvipfix_log_error( "shm_open: %s, %s", appPath, strerror( errno ) );

			return NV_IPFIX_RETURN_CODE_DAEMON_SHARED_MEMORY_ERROR;
		}

		volatile bool * isRunning = (bool *)mmap( NULL, sizeof (bool), PROT_READ | PROT_WRITE, MAP_SHARED, shmHandle, 0 );

		if (isRunning == NULL) {
			nvipfix_log_error( "mmap" );
		}
		else {
			*isRunning = false;
		}

		close( shmHandle );
		shm_unlink( appPath );

		free( appPath );
#endif

		return NV_IPFIX_RETURN_CODE_OK;
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
		int within_last;
		within_last = nvipfix_datetime_to_ctime(&endTs) - nvipfix_datetime_to_ctime(&startTs);
		nvipfix_main_export_nvc(&startTs, &endTs, within_last);
	}

	return NV_IPFIX_RETURN_CODE_OK;
}

int main_daemon( char * a_shmName )
{
	int result = NV_IPFIX_RETURN_CODE_OK;

	volatile bool * isRunning = NULL;

#ifdef NVIPFIX_DEF_POSIX
	a_shmName = shm_name_escape( a_shmName );
	int shmHandle = shm_open( a_shmName, O_RDWR, 0 );

	if (shmHandle >= 0) {
		close( shmHandle );
		shm_unlink( a_shmName );
		nvipfix_log_error( "Daemon is already running" );

		return NV_IPFIX_RETURN_CODE_DAEMON_ALREADY_RUNNING;
	}

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

			int stdFileHandle = open( "/dev/null", O_RDWR );

			if (stdFileHandle == 0) {
				dup( stdFileHandle );
				dup( stdFileHandle );
			}

			shmHandle = shm_open( a_shmName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH );

			if (shmHandle < 0) {
				nvipfix_log_error( "shm_open: %s, %s", a_shmName, strerror( errno ) );

				return NV_IPFIX_RETURN_CODE_DAEMON_SHARED_MEMORY_ERROR;
			}

			if (ftruncate( shmHandle, sizeof (bool) ) != 0) {
				nvipfix_log_error( "ftruncate" );

				close( shmHandle );
				shm_unlink( a_shmName );

				return NV_IPFIX_RETURN_CODE_DAEMON_SHARED_MEMORY_ERROR;
			}

			isRunning = (bool *)mmap( NULL, sizeof (bool), PROT_READ | PROT_WRITE, MAP_SHARED, shmHandle, 0 );

			if (isRunning == NULL) {
				nvipfix_log_error( "mmap" );

				close( shmHandle );
				shm_unlink( a_shmName );

				return NV_IPFIX_RETURN_CODE_DAEMON_SHARED_MEMORY_ERROR;
			}

			*isRunning = true;
#endif

			nvIPFIX_datetime_t startTs = { 0 };
			nvIPFIX_datetime_t endTs = { 0 };

			time_t startT = time( NULL );
			nvIPFIX_timespan_t exportInterval = nvipfix_config_get_export_interval();
			int waitSeconds = NVIPFIX_TIMESPAN_GET_SECONDS( &exportInterval );

			while (*isRunning) {
#ifdef NVIPFIX_DEF_POSIX
				sleep( waitSeconds );
#endif
				time_t nowT = time( NULL );
				if (nvipfix_ctime_to_datetime( &startTs, &startT ) &&  nvipfix_ctime_to_datetime( &endTs, &nowT )) {
					nvipfix_main_export_nvc( &startTs, &endTs, waitSeconds );
				}

				startT = nowT;
			}

#ifdef NVIPFIX_DEF_POSIX
			close( shmHandle );
			shm_unlink( a_shmName );
#endif
		}
	}
	else {
		nvipfix_log_info( "Starting daemon..." );
	}

	return result;
}

char * shm_name_escape( char * a_shmName )
{
	char * slashPtr;

	while ((slashPtr = strchr( a_shmName + 1, '/')) != NULL) {
		*slashPtr = ':';
	}

	return a_shmName;
}


#endif // NVIPFIX_DEF_TEST
