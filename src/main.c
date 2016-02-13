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

#include "include/log.h"
#include "include/data.h"
#include "include/config.h"
#include "include/import.h"
#include "include/export.h"

#include "include/main.h"


void nvipfix_main_export( nvIPFIX_data_record_list_t * a_dataRecords,
		const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs )
{
	size_t collectorsCount;
	nvIPFIX_collector_info_t * collectors = nvipfix_config_collectors_get( &collectorsCount );

	if (collectors != NULL) {
		if (a_dataRecords != NULL) {
			for (size_t i = 0; i < collectorsCount; i++) {
				nvipfix_log_debug( "collector: name = %s, host = %s, ip = %d.%d.%d.%d, port = %s",
						collectors[i].name, collectors[i].host,
						NVIPFIX_ARGSF_IP_ADDRESS( collectors[i].ipAddress ),
						collectors[i].port );

				nvipfix_export( collectors[i].host, collectors[i].port, collectors[i].transport, &(collectors[i].key),
						a_dataRecords, a_startTs, a_endTs );
			}
		}
		else {
			nvipfix_log_warning( "no data" );
		}
	}
	else {
		nvipfix_log_warning( "no collector(s) defined" );
	}

	nvipfix_config_collectors_free( collectors );
}

void nvipfix_main_export_file( const nvIPFIX_CHAR * a_filename,
		const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs )
{
	nvIPFIX_data_record_list_t * dataRecords = nvipfix_import_file( a_filename );
	nvipfix_main_export( dataRecords, a_startTs, a_endTs );

	nvipfix_data_list_free( dataRecords );
}

void nvipfix_main_export_nvc( const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs )
{
	nvIPFIX_switch_info_t * switchInfo = nvipfix_config_switch_info_get();
	nvipfix_log_debug( "switch: host = %s, login = %s, password = %s",
			switchInfo->host,
			switchInfo->login,
			switchInfo->password );

	nvIPFIX_data_record_list_t * dataRecords = NULL;

#ifdef NVIPFIX_DEF_ENABLE_NVC
	dataRecords = nvipfix_import_nvc(
			switchInfo->host, switchInfo->login, switchInfo->password,
			a_startTs, a_endTs );
#endif

	nvipfix_main_export( dataRecords, a_startTs, a_endTs );

	nvipfix_data_list_free( dataRecords );
	nvipfix_config_switch_info_free( switchInfo );
}
