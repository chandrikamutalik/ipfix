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


#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "include/types.h"
#include "include/log.h"
#include "include/error.h"
#include "include/config.h"
#include "include/import.h"
#include "include/export.h"

#ifdef NVIPFIX_DEF_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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


int main_daemon( void );


void Usage()
{
	printf( "nvIPFIX %s\n", NVIPFIX_VERSION_STRING );
	puts( "Usage: nvIPFIX [[-fdatafile] <start_ts> <end_ts>] [daemon]" );
	puts( "\tdatafile - JSON file (for debug purpose)" );
	puts( "\tstart_ts/end_ts - ISO 8601 datetime (YYYY-MM-DDTHH:mm:SS)" );
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
	else if (strcmp( "daemon", argv[1] ) == 0) {
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

	nvIPFIX_switch_info_t * switchInfo = nvipfix_config_switch_info_get();
	nvipfix_log_debug( "switch: host = %s, login = %s, password = %s",
			switchInfo->host,
			switchInfo->login,
			switchInfo->password );

	size_t collectorsCount;
	nvIPFIX_collector_info_t * collectors = nvipfix_config_collectors_get( &collectorsCount );

	if (collectors != NULL) {
		nvIPFIX_data_record_list_t * dataRecords = NULL;

		if (useFile) {
			dataRecords = nvipfix_import_file( argv[1] + 2 );
		}
		else {
#ifdef NVIPFIX_DEF_ENABLE_NVC
			dataRecords = nvipfix_import_nvc( switchInfo->host, switchInfo->login, switchInfo->password, &startTs, &endTs );
#endif
		}

		if (dataRecords != NULL) {
			for (size_t i = 0; i < collectorsCount; i++) {
				nvipfix_log_debug( "collector: name = %s, host = %s, ip = %d.%d.%d.%d, port = %s",
						collectors[i].name, collectors[i].host,
						NVIPFIX_ARGSF_IP_ADDRESS( collectors[i].ipAddress ),
						collectors[i].port );

				//nvipfix_export( collectors[i].host, collectors[i].port, collectors[i].transport, dataRecords, &startTs, &endTs );
			}
		}
		else {
			nvipfix_log_error( "no data" );
			return NV_IPFIX_RETURN_CODE_DATA_ERROR;
		}
	}
	else {
		nvipfix_log_error( "no collector(s) defined" );
		return NV_IPFIX_RETURN_CODE_CONFIGURATION_ERROR;
	}

	nvipfix_config_switch_info_free( switchInfo );

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

		}

		while (true) {
			sleep( 10 );
		}
	}
#endif

	return result;
}


#endif // NVIPFIX_DEF_TEST
