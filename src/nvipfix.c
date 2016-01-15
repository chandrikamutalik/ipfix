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

#include <stdio.h>
#include <string.h>


#include "include/types.h"
#include "include/log.h"
#include "include/config.h"
#include "include/import.h"
#include "include/export.h"


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
	NV_IPFIX_RETURN_CODE_DATA_ERROR
};


void Usage()
{
	printf( "nvIPFIX %s\n", NVIPFIX_VERSION_STRING );
	puts( "Usage: nvIPFIX [-fdatafile] <start_ts> <end_ts>" );
	puts( "\tdatafile - JSON file" );
	puts( "\tstart_ts/end_ts - ISO 8601 datetime (YYYY-MM-DDTHH:mm:SS)" );
}

int main( int argc, char * argv[] )
{
	if (argc < 3) {
		Usage();
		return NV_IPFIX_RETURN_CODE_ARGS_ERROR;
	}

	size_t argIndexTs = 2;

	if (strncmp( "-f", argv[1], 2 ) == 0) {
		argIndexTs = 1;
	}

	nvIPFIX_datetime_t startTs = { 0 };
	nvIPFIX_datetime_t endTs = { 0 };

	if (!nvipfix_parse_datetime_iso8601( argv[argIndexTs], &startTs )
			|| !nvipfix_parse_datetime_iso8601( argv[argIndexTs + 1], &endTs )) {

		puts( "invalid arguments" );
		Usage();
		return NV_IPFIX_RETURN_CODE_ARGS_ERROR;
	}

	size_t collectorsCount;
	nvIPFIX_collector_info_t * collectors = nvipfix_config_collectors_get( &collectorsCount );

	if (collectors != NULL) {
		nvIPFIX_data_record_list_t * dataRecords = nvipfix_import_file( argv[1] + 2 );

		if (dataRecords != NULL) {
			for (size_t i = 0; i < collectorsCount; i++) {
				nvipfix_log_debug( "collector: name = %s, host = %s, ip = %d.%d.%d.%d, port = %s",
						collectors[i].name, collectors[i].host,
						NVIPFIX_ARGSF_IP_ADDRESS( collectors[i].ipAddress ),
						collectors[i].port );

				nvipfix_export( collectors[i].host, collectors[i].port, collectors[i].transport, dataRecords, &startTs, &endTs );
			}
		}
		else {
			puts( "no data" );
			return NV_IPFIX_RETURN_CODE_DATA_ERROR;
		}
	}
	else {
		puts( "no collector(s) defined" );
		return NV_IPFIX_RETURN_CODE_CONFIGURATION_ERROR;
	}

	return NV_IPFIX_RETURN_CODE_CONFIGURATION_ERROR;
}
