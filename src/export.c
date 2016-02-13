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

#include <stdbool.h>

#include "fixbuf/public.h"

#include "include/types.h"
#include "include/error.h"
#include "include/log.h"

#include "include/export.h"


#define NVIPFIX_TEMPLATE_ITEM( a_name ) { .name = a_name, .len_override = 0, .flags = 0 }


typedef struct {
	uint64_t messageCount;
	uint64_t flowRecordCount;
} nvIPFIX_collector_t;

typedef struct {
	uint32_t flowStartSeconds;
    uint32_t flowEndSeconds;

    uint64_t layer2SegmentId;

    uint64_t transportOctetDeltaCount;
    uint64_t initiatorOctets;
    uint64_t responderOctets;

    uint64_t latencyMicroseconds;
    uint32_t flowDurationMilliseconds;

    uint32_t ingressInterface;
    uint32_t egressInterface;

    uint16_t vlanId;

    uint16_t ethernetType;

    uint32_t sourceIpAddress;
    uint32_t destinationIpAddress;
    uint16_t sourceTransportPort;
    uint16_t destinationTransportPort;
    uint8_t sourceMacAddress[NV_IPFIX_ADDRESS_OCTETS_COUNT_MAC];
    uint8_t destinationMacAddress[NV_IPFIX_ADDRESS_OCTETS_COUNT_MAC];

    uint8_t protocolIdentifier;

    uint8_t tcpControlBits;
} nvIPFIX_export_data_t;

typedef struct {
	uint64_t exportedMessageTotalCount;
	uint64_t exportedFlowRecordTotalCount;
} nvIPFIX_export_stats_data_t;


static const char * InfoElementLatencyName = NVIPFIX_IE_LATENCY_NAME;

static fbInfoElementSpec_t Template[] = {
		NVIPFIX_TEMPLATE_ITEM( "flowStartSeconds" ),
		NVIPFIX_TEMPLATE_ITEM( "flowEndSeconds" ),
		NVIPFIX_TEMPLATE_ITEM( "layer2SegmentId" ),
		NVIPFIX_TEMPLATE_ITEM( "transportOctetDeltaCount" ),
		NVIPFIX_TEMPLATE_ITEM( "initiatorOctets" ),
		NVIPFIX_TEMPLATE_ITEM( "responderOctets" ),
		NVIPFIX_TEMPLATE_ITEM( NVIPFIX_IE_LATENCY_NAME ),
		NVIPFIX_TEMPLATE_ITEM( "flowDurationMilliseconds" ),
		NVIPFIX_TEMPLATE_ITEM( "ingressInterface" ),
		NVIPFIX_TEMPLATE_ITEM( "egressInterface" ),
		NVIPFIX_TEMPLATE_ITEM( "vlanId" ),
		NVIPFIX_TEMPLATE_ITEM( "ethernetType" ),
		NVIPFIX_TEMPLATE_ITEM( "sourceIPv4Address" ),
		NVIPFIX_TEMPLATE_ITEM( "destinationIPv4Address" ),
		NVIPFIX_TEMPLATE_ITEM( "sourceTransportPort" ),
		NVIPFIX_TEMPLATE_ITEM( "destinationTransportPort" ),
		NVIPFIX_TEMPLATE_ITEM( "sourceMacAddress" ),
		NVIPFIX_TEMPLATE_ITEM( "destinationMacAddress" ),
		NVIPFIX_TEMPLATE_ITEM( "protocolIdentifier" ),
		NVIPFIX_TEMPLATE_ITEM( "tcpControlBits" ),
		FB_IESPEC_NULL
};

static fbInfoElementSpec_t StatsTemplate[] = {
		NVIPFIX_TEMPLATE_ITEM( "exportedMessageTotalCount" ),
		NVIPFIX_TEMPLATE_ITEM( "exportedFlowRecordTotalCount" ),
		FB_IESPEC_NULL
};

static fbInfoModel_t * InfoModel = NULL;

static nvIPFIX_hashtable8_t * CollectorsTable = NULL;


static bool nvipfix_export_init( void );
static void nvipfix_export_cleanup( void );
static const nvIPFIX_collector_t * nvipfix_export_get_collector( const nvIPFIX_hashtable_key_t * a_key );


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
bool nvipfix_export_init( void )
{
	static volatile bool isInitialized = false;

	#pragma omp critical (nvipfixCritical_ExportInit)
	{
		if (!isInitialized) {
			NVIPFIX_ERROR_INIT( error );

			InfoModel = fbInfoModelAlloc();
			NVIPFIX_ERROR_RAISE_IF( InfoModel == NULL, error, NV_IPFIX_ERROR_CODE_ALLOCATE_INFO_MODEL, InfoModel,
					"%s", "fbInfoModelAlloc failed" );

			fbInfoElement_t ieLatency = FB_IE_INIT(
					InfoElementLatencyName,
					NVIPFIX_PEN,
					NV_IPFIX_IE_LATENCY,
					8,
					FB_IE_F_ENDIAN | FB_IE_F_REVERSIBLE | FB_UNITS_MICROSECONDS );

			fbInfoModelAddElement( InfoModel, &ieLatency );

			atexit( nvipfix_export_cleanup );
			isInitialized = true;

			NVIPFIX_ERROR_RAISE( error, NV_IPFIX_ERROR_CODE_NONE, None );

			fbInfoModelFree( InfoModel );

			NVIPFIX_ERROR_HANDLER( InfoModel );

			NVIPFIX_ERROR_HANDLER( None );
		}
	}

	return isInitialized;
}
#pragma GCC diagnostic pop

void nvipfix_export_cleanup( void )
{
	if (InfoModel != NULL) {
		fbInfoModelFree( InfoModel );
	}
}

static void nvipfix_export_free_collector( const void * a_collector )
{
	free( (void *)a_collector );
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
const nvIPFIX_collector_t * nvipfix_export_get_collector( const nvIPFIX_hashtable_key_t * a_key )
{
	const nvIPFIX_collector_t * result = NULL;

	NVIPFIX_ERROR_INIT( error );

	#pragma omp critical (nvipfixCritical_CollectorsTable)
	{
		result = nvipfix_hashtable8_get( CollectorsTable, a_key->value, a_key->len );

		if (result == NULL) {
			nvIPFIX_collector_t * collector = malloc( sizeof (nvIPFIX_collector_t) );

			NVIPFIX_ERROR_RAISE_IF( collector == NULL, error, NV_IPFIX_ERROR_CODE_MALLOC, CollectorAlloc,
					"%s", "Collector malloc failed" );

			collector->messageCount = 0;
			collector->flowRecordCount = 0;

			CollectorsTable = nvipfix_hashtable8_add( CollectorsTable,
				a_key->value, a_key->len, collector,
				NULL, nvipfix_export_free_collector );

			NVIPFIX_ERROR_RAISE_IF( CollectorsTable == NULL, error, NV_IPFIX_ERROR_CODE_HASHTABLE_ADD, CollectorAdd,
					"%s", "Collector add failed" );

			NVIPFIX_ERROR_RAISE( error, NV_IPFIX_ERROR_CODE_NONE, None );

			NVIPFIX_ERROR_HANDLER( CollectorAdd );
			free( collector );

			NVIPFIX_ERROR_HANDLER( CollectorAlloc );

			NVIPFIX_ERROR_HANDLER( None );
		}
	}

	return result;
}
#pragma GCC diagnostic pop

nvIPFIX_error_t nvipfix_export(
		const nvIPFIX_CHAR * a_host,
		const nvIPFIX_CHAR * a_port,
		nvIPFIX_TRANSPORT a_transport,
		const nvIPFIX_hashtable_key_t * a_key,
		const nvIPFIX_data_record_list_t * a_data,
		const nvIPFIX_datetime_t * a_startTs,
		const nvIPFIX_datetime_t * a_endTs )
{
	NVIPFIX_ERROR_INIT( error );

	NVIPFIX_ERROR_RAISE_IF( !nvipfix_export_init(), error, NV_IPFIX_ERROR_CODE_EXPORT_INIT, Init, "", NULL );

	nvIPFIX_collector_t * collector = (nvIPFIX_collector_t *)nvipfix_export_get_collector( a_key );
	NVIPFIX_ERROR_RAISE_IF( collector == NULL, error, NV_IPFIX_ERROR_CODE_EXPORT_GET_COLLECTOR, CollectorGet,
			"%s", "Unable to get collector" );

	fbSession_t * session = fbSessionAlloc( InfoModel );
	NVIPFIX_ERROR_RAISE_IF( session == NULL, error, NV_IPFIX_ERROR_CODE_ALLOCATE_SESSION, SessionAlloc,
			"%s", "fbSessionAlloc failed" );

	fbConnSpec_t connSpec = { 0 };
	connSpec.transport = (a_transport == NV_IPFIX_TRANSPORT_TCP) ? FB_TCP
			: (a_transport == NV_IPFIX_TRANSPORT_UDP) ? FB_UDP
			: FB_SCTP ;

	connSpec.host = (char *)a_host;
	connSpec.svc = (char *)a_port;
	connSpec.ssl_ca_file = NULL;
	connSpec.ssl_cert_file = NULL;
	connSpec.ssl_key_file = NULL;
	connSpec.ssl_key_pass = NULL;
	connSpec.vai = NULL;
	connSpec.vssl_ctx = NULL;

	NVIPFIX_TLOG_DEBUG( "host = %s, port = %s, transport = %d",
			connSpec.host,
			connSpec.svc,
			(unsigned)connSpec.transport );

	fbExporter_t * exporter = fbExporterAllocNet( &connSpec );
	//fbExporter_t * exporter = fbExporterAllocFile( "./ipfix.out" );
	NVIPFIX_ERROR_RAISE_IF( exporter == NULL, error, NV_IPFIX_ERROR_CODE_ALLOCATE_EXPORTER, ExporterAlloc,
			"%s", "Exporter alloc failed" );

	fbTemplate_t * template = fbTemplateAlloc( InfoModel );
	NVIPFIX_ERROR_RAISE_IF( template == NULL, error, NV_IPFIX_ERROR_CODE_ALLOCATE_TEMPLATE, TemplateAlloc,
			"%s", "Template alloc failed" );

	fbTemplate_t * statsTemplate = fbTemplateAlloc( InfoModel );
	NVIPFIX_ERROR_RAISE_IF( exporter == NULL, error, NV_IPFIX_ERROR_CODE_ALLOCATE_TEMPLATE, StatsTemplateAlloc,
			"%s", "Stats template alloc failed" );

	GError * fbError = NULL;

	NVIPFIX_ERROR_RAISE_IF( !fbTemplateAppendSpecArray( template, Template, UINT32_MAX, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_TEMPLATE_APPEND_SPEC, TemplateAppendSpec,
			"%s", "Template append spec failed" );

	NVIPFIX_ERROR_RAISE_IF( !fbTemplateAppendSpecArray( statsTemplate, StatsTemplate, UINT32_MAX, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_TEMPLATE_APPEND_SPEC, StatsTemplateAppendSpec,
			"%s", "Stats template append spec failed" );

	uint16_t templateId = fbSessionAddTemplate( session, TRUE, FB_TID_AUTO, template, &fbError );
	uint16_t templateIdExt = fbSessionAddTemplate( session, FALSE, FB_TID_AUTO, template, &fbError );
	uint16_t statsTemplateId = fbSessionAddTemplate( session, TRUE, FB_TID_AUTO, statsTemplate, &fbError );
	uint16_t statsTemplateIdExt = fbSessionAddTemplate( session, FALSE, FB_TID_AUTO, statsTemplate, &fbError );
	NVIPFIX_ERROR_RAISE_IF( templateId == 0 || templateIdExt == 0 || statsTemplateId == 0 || statsTemplateIdExt == 0,
			error, NV_IPFIX_ERROR_CODE_EXPORT_SESSION_ADD_TEMPLATE, SessionAddTemplate,
			"%s", "Session add template failed" );

	NVIPFIX_TLOG_DEBUG( "%s: templateId = %d, stats templateId = %d", (unsigned)templateId, (unsigned)statsTemplateId );

	fBuf_t * buffer = fBufAllocForExport( session, exporter );
	NVIPFIX_ERROR_RAISE_IF( buffer == NULL,
			error, NV_IPFIX_ERROR_CODE_EXPORT_BUF_ALLOC, BufAlloc,
			"%s", "Buf alloc failed" );

	NVIPFIX_ERROR_RAISE_IF( !fbSessionExportTemplates( session, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_SESSION_EXPORT_TEMPLATES, SessionExportTemplates,
			"%s", "Session export templates failed" );

	NVIPFIX_ERROR_RAISE_IF( !fBufSetInternalTemplate( buffer, templateId, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_SET_INTERNAL_TEMPLATE, SetInternalTemplate,
			"%s", "Set internal template failed" );

	NVIPFIX_ERROR_RAISE_IF( !fBufSetExportTemplate( buffer, templateIdExt, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_SET_EXPORT_TEMPLATE, SetExportTemplate,
			"%s", "Set export template failed" );

	nvIPFIX_U32 startTs = nvipfix_datetime_get_seconds_since_epoch( a_startTs, 1970, 1 );
	nvIPFIX_U32 endTs = nvipfix_datetime_get_seconds_since_epoch( a_endTs, 1970, 1 );

	nvIPFIX_data_record_t * record = a_data->head;
	int recordCount = 0;

	while (record != NULL) {
		nvIPFIX_export_data_t data = { 0 };

		data.flowStartSeconds = record->flowStart.hasValue
				? nvipfix_datetime_get_seconds_since_epoch( &(record->flowStart), 1970, 1 )
						: startTs;

		data.flowEndSeconds = record->flowEnd.hasValue
				? nvipfix_datetime_get_seconds_since_epoch( &(record->flowEnd), 1970, 1 )
						: endTs;

		data.flowDurationMilliseconds = nvipfix_timespan_get_milliseconds( &record->flowDuration );
		data.ingressInterface = record->ingressInterface;
		data.egressInterface = record->egressInterface;
		data.vlanId = record->vlanId;
		data.layer2SegmentId = record->layer2SegmentId;
		data.transportOctetDeltaCount = record->transportOctetDeltaCount;
		data.initiatorOctets = record->initiatorOctets;
		data.responderOctets = record->responderOctets;
		data.sourceIpAddress = record->sourceIp.value;
		data.destinationIpAddress = record->destinationIp.value;
		data.sourceTransportPort = record->sourcePort;
		data.destinationTransportPort = record->destinationPort;
		memcpy( data.sourceMacAddress, record->sourceMac.octets, sizeof data.sourceMacAddress );
		memcpy( data.destinationMacAddress, record->destinationMac.octets, sizeof data.destinationMacAddress );
		data.protocolIdentifier = record->protocol;
		data.tcpControlBits = (uint8_t) record->tcpControlBits;
		data.ethernetType = (uint16_t) record->ethernetType;
		data.latencyMicroseconds = nvipfix_timespan_get_microseconds( &record->latency );

		NVIPFIX_TLOG_DEBUG(
				"%d.%d.%d.%d:%d -> %d.%d.%d.%d:%d %d %d-%d %d %02x:%02x:%02x:%02x:%02x:%02x",
				NVIPFIX_ARGSF_IP_ADDRESS( record->sourceIp ),
				record->sourcePort,
				NVIPFIX_ARGSF_IP_ADDRESS( record->destinationIp ),
				record->destinationPort, record->transportOctetDeltaCount,
				data.flowStartSeconds, data.flowEndSeconds,
				data.flowDurationMilliseconds,
				NVIPFIX_ARGSF_MAC_ADDRESS( record->sourceMac ));

		if (!fBufAppend( buffer, (uint8_t *) &data, sizeof (nvIPFIX_export_data_t), &fbError )) {
			NVIPFIX_TLOG_ERROR( "%s: fBufAppend %s", __func__, fbError->message );
		}
		else {
			recordCount++;
		}

		record = record->next;
	}

	NVIPFIX_ERROR_RAISE_IF( !fBufSetInternalTemplate( buffer, statsTemplateId, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_SET_INTERNAL_TEMPLATE, SetInternalTemplateStats,
			"%s", "Set internal template (stats) failed" );

	NVIPFIX_ERROR_RAISE_IF( !fBufSetExportTemplate( buffer, statsTemplateIdExt, &fbError ),
			error, NV_IPFIX_ERROR_CODE_EXPORT_SET_EXPORT_TEMPLATE, SetExportTemplateStats,
			"%s", "Set export template (stats) failed" );

	collector->flowRecordCount += recordCount;
	collector->messageCount++;

	nvIPFIX_export_stats_data_t stats = { 0 };
	stats.exportedFlowRecordTotalCount = collector->flowRecordCount;
	stats.exportedMessageTotalCount = collector->messageCount;

	NVIPFIX_TLOG_ERROR_IF(
			!fBufAppend( buffer, (uint8_t *) &stats, sizeof (nvIPFIX_export_stats_data_t), &fbError ),
			"%s: fBufAppend (stats) %s", __func__, fbError->message );

	NVIPFIX_ERROR_HANDLER( SetExportTemplateStats );

	NVIPFIX_ERROR_HANDLER( SetInternalTemplateStats );

	NVIPFIX_ERROR_HANDLER( SetExportTemplate );

	NVIPFIX_ERROR_HANDLER( SetInternalTemplate );

	NVIPFIX_ERROR_HANDLER( SessionExportTemplates );

	fBufFree( buffer );

	NVIPFIX_ERROR_HANDLER( BufAlloc );

	NVIPFIX_ERROR_HANDLER( SessionAddTemplate );

	NVIPFIX_ERROR_HANDLER( StatsTemplateAppendSpec );

	NVIPFIX_ERROR_HANDLER( TemplateAppendSpec );

	NVIPFIX_ERROR_HANDLER( StatsTemplateAlloc );

	NVIPFIX_ERROR_HANDLER( TemplateAlloc );

	NVIPFIX_ERROR_HANDLER( ExporterAlloc );

	NVIPFIX_ERROR_HANDLER( SessionAlloc );

	NVIPFIX_ERROR_HANDLER( CollectorGet );

	NVIPFIX_ERROR_HANDLER( Init );

	return error;
}
