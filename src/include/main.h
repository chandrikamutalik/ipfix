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

#ifndef __NVIPFIX_MAIN_H
#define __NVIPFIX_MAIN_H


#include "types.h"
#include "data.h"


/**
 *
 * @param a_dataRecords
 * @param a_startTs
 * @param a_endTs
 */
void nvipfix_main_export( nvIPFIX_data_record_list_t * a_dataRecords,
		const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs );

/**
 *
 * @param a_filename
 * @param a_startTs
 * @param a_endTs
 */
void nvipfix_main_export_file( const nvIPFIX_CHAR * a_filename,
		const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs );

/**
 *
 * @param a_startTs
 * @param a_endTs
 */
void nvipfix_main_export_nvc( const nvIPFIX_datetime_t * a_startTs, const nvIPFIX_datetime_t * a_endTs );


#endif /* __NVIPFIX_MAIN_H */
