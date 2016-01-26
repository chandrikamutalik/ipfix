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

#include "fixbuf/public.h"

#include "include/types.h"
#include "include/error.h"
#include "include/log.h"

#include "include/export.h"


#define NVIPFIX_TEMPLATE_ITEM( a_name ) { .name = a_name, .len_override = 0, .flags = 0 }


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


static fbInfoElementSpec_t Template[] = {
		NVIPFIX_TEMPLATE_ITEM( "flowStartSeconds" ),
		NVIPFIX_TEMPLATE_ITEM( "flowEndSeconds" ),
		NVIPFIX_TEMPLATE_ITEM( "layer2SegmentId" ),
		NVIPFIX_TEMPLATE_ITEM( "transportOctetDeltaCount" ),
		NVIPFIX_TEMPLATE_ITEM( "initiatorOctets" ),
		NVIPFIX_TEMPLATE_ITEM( "responderOctets" ),
		NVIPFIX_TEMPLATE_ITEM( "latencyMicroseconds" ),
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


nvIPFIX_error_t nvipfix_export(
		const nvIPFIX_CHAR * a_host,
		const nvIPFIX_CHAR * a_port,
		nvIPFIX_TRANSPORT a_transport,
		const nvIPFIX_data_record_list_t * a_data,
		const nvIPFIX_datetime_t * a_startTs,
		const nvIPFIX_datetime_t * a_endTs )
{
	NVIPFIX_INIT_ERROR( error );

	fbInfoModel_t * infoModel = fbInfoModelAlloc();

	if (infoModel == NULL) {
		NVIPFIX_RETURN_ERROR( error, NV_IPFIX_ERROR_CODE_ALLOCATE_INFO_MODEL );
	}

	fbInfoElement_t ieLatency = FB_IE_INIT(
			"latencyMicroseconds",
			NVIPFIX_PEN,
			NV_IPFIX_IE_LATENCY,
			8,
			FB_IE_F_ENDIAN | FB_IE_F_REVERSIBLE | FB_UNITS_MICROSECONDS );

	fbInfoModelAddElement( infoModel, &ieLatency );

	fbSession_t * session = fbSessionAlloc( infoModel );

	if (session != NULL) {
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

		if (exporter != NULL) {
			fbTemplate_t * template = fbTemplateAlloc( infoModel );

			if (template != NULL) {
				GError * fbError = NULL;

				if (fbTemplateAppendSpecArray( template, Template, UINT32_MAX, &fbError )) {
					uint16_t templateId = fbSessionAddTemplate( session, TRUE, FB_TID_AUTO, template, &fbError );
					uint16_t templateIdExt = fbSessionAddTemplate( session, FALSE, FB_TID_AUTO, template, &fbError );

					nvipfix_tlog_debug( NVIPFIX_T( "%s: templateId = %d" ), __func__, (unsigned)templateId );

					if (templateId == 0) {
						nvipfix_tlog_error( NVIPFIX_T( "%s: fbSessionAddTemplate %s" ), __func__, fbError->message );
					}

					fBuf_t * buffer = fBufAllocForExport( session, exporter );

					if (fbSessionExportTemplates( session, &fbError )) {
						if (fBufSetInternalTemplate( buffer, templateId, &fbError )) {
							if (fBufSetExportTemplate( buffer, templateIdExt, &fbError )) {
								nvIPFIX_U32 startTs = nvipfix_get_seconds_since_epoch( a_startTs, 1970, 1 );
								nvIPFIX_U32 endTs = nvipfix_get_seconds_since_epoch( a_endTs, 1970, 1 );

								nvIPFIX_data_record_t * record = a_data->head;

								while (record != NULL) {
									nvIPFIX_export_data_t data = { 0 };
									data.flowStartSeconds = record->flowStart.hasValue
											? nvipfix_get_seconds_since_epoch( &(record->flowStart), 1970, 1 )
													: startTs;

									data.flowEndSeconds = record->flowEnd.hasValue
											? nvipfix_get_seconds_since_epoch( &(record->flowEnd), 1970, 1 )
													: endTs;

									data.flowDurationMilliseconds = nvipfix_get_timespan_milliseconds( &record->flowDuration );
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
									data.tcpControlBits = (uint8_t)record->tcpControlBits;
									data.ethernetType = (uint16_t)record->ethernetType;
									data.latencyMicroseconds = nvipfix_get_timespan_microseconds( &record->latency );

									NVIPFIX_TLOG_DEBUG( "%d.%d.%d.%d:%d -> %d.%d.%d.%d:%d %d %d-%d %d %02x:%02x:%02x:%02x:%02x:%02x",
											NVIPFIX_ARGSF_IP_ADDRESS( record->sourceIp ), record->sourcePort,
											NVIPFIX_ARGSF_IP_ADDRESS( record->destinationIp ), record->destinationPort,
											record->transportOctetDeltaCount,
											data.flowStartSeconds,
											data.flowEndSeconds,
											data.flowDurationMilliseconds,
											NVIPFIX_ARGSF_MAC_ADDRESS( record->sourceMac ) );

									if (!fBufAppend( buffer, (uint8_t *)&data, sizeof (nvIPFIX_export_data_t), &fbError )) {
										nvipfix_tlog_error( NVIPFIX_T( "%s: fBufAppend %s" ), __func__, fbError->message );
									}

									record = record->next;
								}
							}
							else {
								nvipfix_tlog_error( NVIPFIX_T( "%s: fBufSetExportTemplate %s" ), __func__, fbError->message );
							}
						}
						else {
							nvipfix_tlog_error( NVIPFIX_T( "%s: fBufSetInternalTemplate" ), __func__ );
						}
					}
					else {
						nvipfix_tlog_error( NVIPFIX_T( "%s: fbSessionExportTemplates" ), __func__ );
					}
				}
				else {
					nvipfix_tlog_error( NVIPFIX_T( "%s: fbTemplateAppendSpecArray" ), __func__ );
				}
			}
			else {
				error.code = NV_IPFIX_ERROR_CODE_ALLOCATE_TEMPLATE;
				fbSessionFree( session );
			}

			fbExporterClose( exporter );
		}
		else {
			error.code = NV_IPFIX_ERROR_CODE_ALLOCATE_EXPORTER;
			fbSessionFree( session );
		}
	}
	else {
		error.code = NV_IPFIX_ERROR_CODE_ALLOCATE_SESSION;
	}

	fbInfoModelFree( infoModel );

	return error;
}
