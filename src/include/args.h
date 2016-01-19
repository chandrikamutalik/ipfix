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

#ifndef __NVIPFIX_ARGS_H
#define __NVIPFIX_ARGS_H


#include <stdbool.h>

#include "types.h"


typedef struct {
	const nvIPFIX_CHAR * dataFilename;
	bool hasDataFilename;
	nvIPFIX_datetime_t startTs;
	nvIPFIX_datetime_t endTs;
} nvIPFIX_args_t;


nvIPFIX_error_t nvipfix_args_parse( char * a_argv[], size_t a_argc, nvIPFIX_args_t * a_args );


#endif /* __NVIPFIX_ARGS_H */
