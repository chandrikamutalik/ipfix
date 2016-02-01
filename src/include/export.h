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

#ifndef __NVIPFIX_EXPORT_H
#define __NVIPFIX_EXPORT_H


#include "types.h"
#include "error.h"

#include "config.h"
#include "data.h"


#define NVIPFIX_PEN 47123

#define NVIPFIX_IE_LATENCY_NAME "latencyMicroseconds"


enum {
	NV_IPFIX_IE_LATENCY = 1
};


/**
 *
 * @param a_host
 * @param a_port
 * @param a_transport
 * @param a_data
 * @param a_startTs
 * @param a_endTs
 * @return
 */
nvIPFIX_error_t nvipfix_export(
		const nvIPFIX_CHAR * a_host,
		const nvIPFIX_CHAR * a_port,
		nvIPFIX_TRANSPORT a_transport,
		const nvIPFIX_data_record_list_t * a_data,
		const nvIPFIX_datetime_t * a_startTs,
		const nvIPFIX_datetime_t * a_endTs );


#endif /* __NVIPFIX_EXPORT_H */
