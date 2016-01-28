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
#include <string.h>

#include "include/types.h"

#include "include/data.h"


nvIPFIX_data_record_list_t * nvipfix_data_list_add( nvIPFIX_data_record_list_t * a_list, nvIPFIX_data_record_t * a_record )
{
	NVIPFIX_NULL_ARGS_GUARD_1( a_record, NULL );

	if (a_list == NULL) {
		a_list = malloc( sizeof (nvIPFIX_data_record_list_t) );

		if (a_list == NULL) {
			return NULL;
		}

		a_record->prev = NULL;
		a_list->head = a_record;
	}
	else {
		a_record->prev = a_list->tail;
		a_list->tail->next = a_record;
	}

	a_record->next = NULL;
	a_list->tail = a_record;

	return a_list;
}

nvIPFIX_data_record_list_t * nvipfix_data_list_add_copy( nvIPFIX_data_record_list_t * a_list, nvIPFIX_data_record_t * a_record )
{
	nvIPFIX_data_record_list_t * result = NULL;

	nvIPFIX_data_record_t * record = malloc( sizeof (nvIPFIX_data_record_t) );

	if (record != NULL) {
		*record = *a_record;
		result = nvipfix_data_list_add( a_list, record );
	}

	return result;
}

void nvipfix_data_list_free( nvIPFIX_data_record_list_t * a_list )
{
	NVIPFIX_NULL_ARGS_GUARD_1_VOID( a_list );

	nvIPFIX_data_record_t * record = a_list->head;

	while (record != NULL) {
		nvIPFIX_data_record_t * next = record->next;
		free( record );
		record = next;
	}

	free( a_list );
}
