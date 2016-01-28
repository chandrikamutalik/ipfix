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
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "include/types.h"
#include "include/log.h"
#include "include/fwatch.h"

#include "include/config.h"


#define NVIPFIX_CONFIG_SETTING( a_name, a_id, a_parentId, a_value, a_offset, a_parseValue ) \
	{ .name = a_name, .id = a_id, .parentId = a_parentId, .value = a_value, .offset = a_offset, .parseValue = a_parseValue }

#define NVIPFIX_CONFIG_SETTING_COLLECTOR( a_name, a_id, a_parentId, a_value, a_field, a_parseValue ) \
	{ .name = a_name, \
		.id = a_id, \
		.parentId = a_parentId, \
		.value = a_value, \
		.offset = offsetof( nvIPFIX_collector_info_t, a_field ), \
		.parseValue = a_parseValue }


typedef struct _nvIPFIX_collector_info_list_item_t {
	struct _nvIPFIX_collector_info_list_item_t * next;
	nvIPFIX_collector_info_t * current;
} nvIPFIX_collector_info_list_item_t;

typedef struct _nvIPFIX_setting_t {
	const char * name;
	int id;
	int parentId;
	void * value;
	size_t offset;
	bool (* parseValue)( const char *, void * );
} nvIPFIX_setting_t;


static void nvipfix_config_init();
static void nvipfix_config_read();
static void nvipfix_config_cleanup();
static void nvipfix_config_add_collector( nvIPFIX_collector_info_t * );

static bool nvipfix_config_is_empty_line( char * a_line );

static bool nvipfix_config_parse_transport( const char *, void * );

static const nvIPFIX_setting_t * nvipfix_config_get_setting( const char *, int );


static const char * ConfigFileName = "nvipfix.config";

enum {
	SizeofFileBuffer = 4096,
	SizeofCollectorInfo = sizeof (nvIPFIX_collector_info_t),
	SizeofCollectorInfoListItem = sizeof (nvIPFIX_collector_info_list_item_t)
};

enum {
	SettingIdSwitch = 1,
	SettingIdSwitchApiHost,
	SettingIdSwitchApiLogin,
	SettingIdSwitchApiPassword,
	SettingIdCollector,
	SettingIdCollectorIpAddress,
	SettingIdCollectorHostname,
	SettingIdCollectorTransport,
	SettingIdCollectorTransportPort,
	SettingIdCollectorDscp
};

static char * SwitchName = NULL;
static char * SwitchApiHost = NULL;
static char * SwitchApiLogin = NULL;
static char * SwitchApiPassword = NULL;

static const nvIPFIX_setting_t Settings[] = {
		NVIPFIX_CONFIG_SETTING( "switch", SettingIdSwitch, 0,
				&SwitchName, 0, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING( "switch-nvapi-host", SettingIdSwitchApiHost, 0,
				&SwitchApiHost, 0, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING( "switch-nvapi-login", SettingIdSwitchApiLogin, 0,
				&SwitchApiLogin, 0, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING( "switch-nvapi-password", SettingIdSwitchApiPassword, 0,
				&SwitchApiPassword, 0, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING_COLLECTOR( "collector", SettingIdCollector, 0,
				NULL, name, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING_COLLECTOR( "collector-ip-address", SettingIdCollectorIpAddress, SettingIdCollector,
				NULL, ipAddress, nvipfix_parse_ip_address ),

		NVIPFIX_CONFIG_SETTING_COLLECTOR( "collector-hostname", SettingIdCollectorHostname, SettingIdCollector,
				NULL, host, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING_COLLECTOR( "transport", SettingIdCollectorTransport, SettingIdCollector,
				NULL, transport, nvipfix_config_parse_transport ),

		NVIPFIX_CONFIG_SETTING_COLLECTOR( "transport-port", SettingIdCollectorTransportPort, SettingIdCollector,
				NULL, port, nvipfix_parse_string ),

		NVIPFIX_CONFIG_SETTING_COLLECTOR( "dscp", SettingIdCollectorDscp, SettingIdCollector,
				NULL, dscp, nvipfix_parse_octet ),

		{ NULL }
};

static const size_t SettingsCount = ((sizeof Settings) / sizeof (nvIPFIX_setting_t)) - 1;

static nvIPFIX_collector_info_list_item_t * CollectorList = NULL;


void nvipfix_config_init()
{
	nvipfix_config_cleanup();
	nvipfix_config_read();

	atexit( nvipfix_config_cleanup );
}

nvIPFIX_collector_info_t * nvipfix_config_collectors_get( size_t * a_count )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_count, NULL );

	static volatile bool isInitialized = false;

	#pragma omp critical (nvipfixCritical_ConfigInit)
	{
		if (!isInitialized) {
			nvipfix_config_init();
			isInitialized = true;
		}
	}

	size_t count = 0;
	char * result = NULL;

	#pragma omp critical (nvipfixCritical_CollectorList)
	{
		nvIPFIX_collector_info_list_item_t * collector = CollectorList;

		while (collector != NULL) {
			count++;
			collector = collector->next;
		}

		nvipfix_tlog_debug( NVIPFIX_T( "%s: collectors count = %d" ), __func__, count );

		size_t bufSize = count * SizeofCollectorInfo;
		size_t bufIndex = 0;
		size_t bufTop = bufSize;

		collector = CollectorList;

		while (collector != NULL) {
			size_t nameLen = strlen( collector->current->name ) + 1;
			size_t hostLen = strlen( collector->current->host ) + 1;
			size_t portLen = strlen( collector->current->port ) + 1;

			bufSize += nameLen + hostLen + portLen;
			void * ptr = realloc( result, bufSize );

			if (ptr == NULL) {
				free( result );
				result = NULL;
				break;
			}

			result = ptr;

			memcpy( result + bufIndex, collector->current, SizeofCollectorInfo );

			if (nameLen > 1) {
				memcpy( result + bufTop, collector->current->name, nameLen );
			}
			else {
				*(result + bufTop) = '\0';
			}

			((nvIPFIX_collector_info_t *)(result + bufIndex))->name = result + bufTop;

			bufTop += nameLen;

			if (hostLen > 1) {
				memcpy( result + bufTop, collector->current->host, hostLen );
			}
			else {
				*(result + bufTop) = '\0';
			}

			((nvIPFIX_collector_info_t *)(result + bufIndex))->host = result + bufTop;

			bufTop += hostLen;

			if (portLen > 1) {
				memcpy( result + bufTop, collector->current->port, portLen );
			}
			else {
				*(result + bufTop) = '\0';
			}

			((nvIPFIX_collector_info_t *)(result + bufIndex))->port = result + bufTop;

			bufTop = bufSize;
			bufIndex += SizeofCollectorInfo;

			collector = collector->next;
		}
	}

	*a_count = count;

	return (nvIPFIX_collector_info_t *)result;
}

void nvipfix_config_collectors_free( nvIPFIX_collector_info_t * a_collectors )
{
	NVIPFIX_NULL_ARGS_GUARD_1_VOID( a_collectors );

	free( a_collectors );
}

void nvipfix_config_read()
{
	nvipfix_tlog_debug( NVIPFIX_T( "reading configuration file" ) );
	FILE * configFile = fopen( ConfigFileName, "r" );

	if (configFile != NULL) {
		char buffer[SizeofFileBuffer];
		int line = 1;
		int id = 0;
		int parentId = 0;
		nvIPFIX_collector_info_t collector;

		#pragma omp critical (nvipfixCritical_CollectorList)
		{
			while (fgets( buffer, sizeof buffer, configFile ) != NULL) {
				if (strchr(buffer, '\n') == NULL) {
					nvipfix_tlog_error( NVIPFIX_T( "configuration file line too long. line = %d" ), line );
					break;
				}

				if (!nvipfix_config_is_empty_line( buffer )) {
					nvIPFIX_string_list_t * tokens = nvipfix_string_split( buffer, " \t\n\r" );

					if (tokens == NULL) {
						nvipfix_tlog_error( NVIPFIX_T( "%s: unable to tokenize a line" ), __func__ );
						break;
					}

					nvIPFIX_string_list_item_t * token = tokens->head;

					int tokenIndex = 0;
					const nvIPFIX_setting_t * setting = NULL;

					while (token != NULL) {
						tokenIndex++;

						if (token->value[0] == '{') {
							parentId = id;
							tokenIndex = 0;
						}
						else if (token->value[0] == '}') {
							if (parentId == SettingIdCollector) {
								nvipfix_config_add_collector( &collector );
							}

							parentId = 0;
							tokenIndex = 0;
						}
						else if (tokenIndex == 1) {
							setting = nvipfix_config_get_setting( token->value, parentId );

							if (setting != NULL) {
								id = setting->id;

								if (id == SettingIdCollector) {
									memset( &collector, 0, sizeof (nvIPFIX_collector_info_t) );
									parentId = id;
								}
							}
							else {
								nvipfix_tlog_error( NVIPFIX_T( "%s: unknown setting '%s', %d" ), __func__, token->value, line );
							}
						}
						else if (tokenIndex == 2) {
							if (parentId == SettingIdCollector && setting != NULL) {
								setting->parseValue( token->value, ((char *)&collector) + setting->offset );
							}
						}

						if (tokenIndex > 2) {
							nvipfix_tlog_error( NVIPFIX_T( "%s: bad configuration. line %d" ), __func__, line );
						}

						token = token->next;
					}

					nvipfix_string_list_free( tokens, true );
				}
				else {
					nvipfix_tlog_debug( NVIPFIX_T( "line is empty. %d" ), line );
				}

				line++;
			}
		}

		if (parentId != 0) {
			nvipfix_tlog_error( NVIPFIX_T( "%s: '}' expected" ), __func__ );
		}

		fclose( configFile );
	}
	else {
		nvipfix_tlog_error( NVIPFIX_T( "%s: unable to open configuration file" ), __func__ );
	}
}

void nvipfix_config_cleanup()
{
	nvipfix_tlog_debug( NVIPFIX_T( "nvipfix_config_cleanup" ) );

	free( SwitchName );
	SwitchName = NULL;

	free( SwitchApiHost );
	SwitchApiHost = NULL;

	free( SwitchApiLogin );
	SwitchApiLogin = NULL;

	free( SwitchApiPassword );
	SwitchApiPassword = NULL;

	nvIPFIX_collector_info_list_item_t * listPtr = CollectorList;

	while (listPtr != NULL) {
		nvIPFIX_collector_info_list_item_t * tPtr = listPtr;
		free( (void *)tPtr->current->name );
		free( (void *)tPtr->current->host );
		free( (void *)tPtr->current->port );

		listPtr = listPtr->next;
		free( tPtr );
	}

	CollectorList = NULL;
}

void nvipfix_config_add_collector( nvIPFIX_collector_info_t * a_collector )
{
	if (a_collector->host == NULL && !a_collector->ipAddress.hasValue) {
		nvipfix_tlog_error( NVIPFIX_T( "%s: collector '%s'. host or IP address expected" ), __func__, a_collector->name );
		return;
	}

	nvIPFIX_collector_info_list_item_t * listItem =
			(nvIPFIX_collector_info_list_item_t *)(malloc( SizeofCollectorInfoListItem + SizeofCollectorInfo ));

	if (listItem != NULL) {
		if (a_collector->port == NULL) {
			a_collector->port = nvipfix_string_duplicate( "4739" );
		}

		if (a_collector->transport == NV_IPFIX_TRANSPORT_UNDEFINED) {
			a_collector->transport = NV_IPFIX_TRANSPORT_UDP;
		}

		nvIPFIX_collector_info_t * collector = (nvIPFIX_collector_info_t *)(((char *)listItem) + SizeofCollectorInfoListItem);
		memcpy( collector, a_collector, sizeof (nvIPFIX_collector_info_t) );
		listItem->current = collector;
		listItem->next = CollectorList;
		CollectorList = listItem;
	}
	else {
		nvipfix_tlog_error( NVIPFIX_T( "%s: unable to allocate memory" ), __func__ );
	}
}

bool nvipfix_config_is_empty_line( char * a_line )
{
	bool result = true;

	if (a_line != NULL) {
		while (*a_line != '\0') {
			char ch = *a_line;

			if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
				if (ch != '#') {
					result = false;
				}
				else {
					*a_line = '\0';
					break;
				}
			}

			a_line++;
		}
	}

	return result;
}

bool nvipfix_config_parse_transport( const char * a_s, void * a_value )
{
	NVIPFIX_NULL_ARGS_GUARD_2( a_s, a_value, false );

	bool result = true;
	nvIPFIX_TRANSPORT * transport = a_value;

	if (strcmp( "udp", a_s ) == 0) {
		*transport = NV_IPFIX_TRANSPORT_UDP;
	}
	else if (strcmp( "tcp", a_s ) == 0) {
		*transport = NV_IPFIX_TRANSPORT_TCP;
	}
	else if (strcmp( "sctp", a_s ) == 0) {
		*transport = NV_IPFIX_TRANSPORT_SCTP;
	}
	else {
		result = false;
	}

	return result;
}

const nvIPFIX_setting_t * nvipfix_config_get_setting( const char * a_name, int a_parentId )
{
	const nvIPFIX_setting_t * result = NULL;

	for (size_t i = 0; i < SettingsCount; i++) {
		if (Settings[i].parentId == a_parentId && strcmp( Settings[i].name, a_name ) == 0) {
			result = Settings + i;
			break;
		}
	}

	return result;
}

nvIPFIX_switch_info_t * nvipfix_config_switch_info_get()
{
	nvIPFIX_switch_info_t * result = malloc( sizeof (nvIPFIX_switch_info_t) );

	if (result != NULL) {
		result->name = nvipfix_string_duplicate( SwitchName );
		result->host = nvipfix_string_duplicate( SwitchApiHost );
		result->login = nvipfix_string_duplicate( SwitchApiLogin );
		result->password = nvipfix_string_duplicate( SwitchApiPassword );
	}

	return result;
}

void nvipfix_config_switch_info_free( nvIPFIX_switch_info_t * a_switchInfo )
{
	NVIPFIX_NULL_ARGS_GUARD_1_VOID( a_switchInfo );

	free( (void *)a_switchInfo->name );
	free( (void *)a_switchInfo->host );
	free( (void *)a_switchInfo->login );
	free( (void *)a_switchInfo->password );
}
