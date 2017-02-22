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
#include <strings.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#include "include/types.h"
#include "include/log.h"
#include "include/error.h"
#include "include/config.h"
#include "include/main.h"

#ifdef NVIPFIX_DEF_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#include <pthread.h>
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

#define NVIPFIX_PID_FILE  "/var/run/nvipfix.pid"

enum {
	NV_IPFIX_RETURN_CODE_OK = 0,
	NV_IPFIX_RETURN_CODE_ARGS_ERROR,
	NV_IPFIX_RETURN_CODE_CONFIGURATION_ERROR,
	NV_IPFIX_RETURN_CODE_DATA_ERROR,
	NV_IPFIX_RETURN_CODE_START_DAEMON_ERROR,
	NV_IPFIX_RETURN_CODE_DAEMON_ALREADY_RUNNING,
	NV_IPFIX_RETURN_CODE_STOP_DAEMON_ERROR,
};


int main_daemon(void);

void Usage(void)
{
	printf( "nvIPFIX %s\n", NVIPFIX_VERSION_STRING );
	puts( "Usage: nvIPFIX start|stop" );
	puts( "\tstart - start nvIPFIX daemon" );
	puts( "\tstop - stop nvIPFIX daemon" );
	puts( "Usage: nvIPFIX [-fdatafile] <start_ts> <end_ts>" );
	puts( "\tdatafile - data file in JSON format (for debug purpose)" );
	puts( "\tstart_ts/end_ts - ISO 8601 datetime (YYYY-MM-DDTHH:mm:SS)" );
}

int
cleanup_and_exit()
{
	int err;

	err = remove(NVIPFIX_PID_FILE);
	if (err) {
		return err;
	}

	nvipfix_data_cleanup();
	exit(0);
}

void
set_signal_mask(sigset_t *sigset)
{
	sigemptyset(sigset);
	sigaddset(sigset, SIGINT);
	sigaddset(sigset, SIGHUP);
	sigaddset(sigset, SIGTERM);
}

void *
ipfix_signal_thread(void *args)
{
	sigset_t sigset;
	int sig, err;

	set_signal_mask(&sigset);
	err = sigwait(&sigset, &sig);
	if (err) {
		return err;
	}

	err = cleanup_and_exit();
	if (err) {
		return err;
	}
}

int
nvipfix_set_pid(void)
{
	if( access(NVIPFIX_PID_FILE, F_OK) != -1 ) {
		return NV_IPFIX_RETURN_CODE_DAEMON_ALREADY_RUNNING;
	}

	pid_t pid = getpid();
	char pid_str[10];
	FILE *fp = fopen(NVIPFIX_PID_FILE, "w+");
	snprintf(pid_str, sizeof(pid_str), "%ld ", (long)pid);
	fprintf(fp, "%s", pid_str);
	fclose(fp);
	return 0;
}

int main( int argc, char * argv[] )
{
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
	else if (strncmp( "start", argv[1], 5 ) == 0) {
		int rc = main_daemon();
		return rc;
	}
	else if (strncmp( "stop", argv[1], 4 ) == 0) {
#ifdef NVIPFIX_DEF_POSIX
		char *kill_str = "kill -INT";
		char pid_str[10] = {'\0'};
		char cmd[20] = {'\0'};

		FILE *fp = fopen(NVIPFIX_PID_FILE, "r");
		if (!fp) {
			return 1;
		}
		fscanf(fp, "%s", pid_str);
		fclose(fp);

		snprintf(cmd, sizeof(cmd), "%s %s", kill_str, pid_str);
		system(cmd);
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

int main_daemon(void)
{
	int result = NV_IPFIX_RETURN_CODE_OK;
	int err;
	sigset_t sigset;
	pthread_t signal_thread;

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

			int stdFileHandle = open( "/dev/null", O_RDWR );

			if (stdFileHandle == 0) {
				dup( stdFileHandle );
				dup( stdFileHandle );
			}

			err = nvipfix_set_pid();
			if (err != 0) {
				return err;
			}

			set_signal_mask(&sigset);
			err = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
			if (err) {
				return err;
			}
			if ((err = pthread_create(&signal_thread, NULL, ipfix_signal_thread, NULL))) {
				return err;
			}
#endif

			nvIPFIX_datetime_t startTs = { 0 };
			nvIPFIX_datetime_t endTs = { 0 };

			time_t startT = time( NULL );
			nvIPFIX_timespan_t exportInterval = nvipfix_config_get_export_interval();
			int waitSeconds = NVIPFIX_TIMESPAN_GET_SECONDS( &exportInterval );

			while (1) {
#ifdef NVIPFIX_DEF_POSIX
				sleep( waitSeconds );
#endif
				time_t nowT = time( NULL );
				if (nvipfix_ctime_to_datetime( &startTs, &startT ) &&  nvipfix_ctime_to_datetime( &endTs, &nowT )) {
					nvipfix_main_export_nvc( &startTs, &endTs, waitSeconds );
				}

				startT = nowT;
			}

		}
	}
	else {
		nvipfix_log_info( "Starting daemon..." );
	}

	return result;
}

#endif // NVIPFIX_DEF_TEST
