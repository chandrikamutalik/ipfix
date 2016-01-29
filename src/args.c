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

#include <stddef.h>
#include <stdbool.h>

#include "include/error.h"
#include "include/args.h"


#define NVIPFIX_ARG_ITEM_WITH_FLAGS( a_key, a_field, a_parseValue, a_flags ) { .key = a_key, \
	.offset = offsetof( nvIPFIX_args_t, a_field ), .parseValue = a_parseValue, .flags = a_flags }

#define NVIPFIX_ARG_ITEM( a_key, a_field, a_parseValue ) \
		NVIPFIX_ARG_ITEM_WITH_FLAGS( a_key, a_field, a_parseValue, NV_IPFIX_ARG_FLAG_NONE )


typedef enum {
	NV_IPFIX_ARG_FLAG_NONE = 0,
	NV_IPFIX_ARG_FLAG_MANDATORY = 1,
} nvIPFIX_ARG_FLAGS;

typedef struct {
	const char * key;
	size_t offset;
	bool (* parseValue)( const char *, void * );
	nvIPFIX_ARG_FLAGS flags;
} nvIPFIX_arg_t;


static bool nvipfix_args_parse_data_filename( const char *, void * );


static const nvIPFIX_arg_t Items[] = {
		NVIPFIX_ARG_ITEM( "-f", dataFilename, nvipfix_args_parse_data_filename ),
		NVIPFIX_ARG_ITEM_WITH_FLAGS( NULL, startTs, nvipfix_args_parse_data_filename, NV_IPFIX_ARG_FLAG_MANDATORY ),
		NVIPFIX_ARG_ITEM_WITH_FLAGS( NULL, endTs, nvipfix_args_parse_data_filename, NV_IPFIX_ARG_FLAG_MANDATORY ),
		{ NULL }
};

static const size_t ItemsCount = (sizeof Items) / sizeof (nvIPFIX_arg_t);


nvIPFIX_error_t nvipfix_args_parse( char * a_argv[], size_t a_argc, nvIPFIX_args_t * a_args )
{
	NVIPFIX_ERROR_INIT( error );

	if (a_argv == NULL || a_args == NULL) {
		NVIPFIX_ERROR_RAISE( error, NV_IPFIX_ERROR_CODE_INVALID_ARGUMENTS, Main );
	}

	for (size_t i = 1; i < a_argc; i++) {

	}

	NVIPFIX_ERROR_HANDLER( Main );

	return error;
}

bool nvipfix_args_parse_data_filename( const char * s, void * value )
{
	bool result = false;

	return result;
}
