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

#ifndef __NVIPFIX_IMPORT_H
#define __NVIPFIX_IMPORT_H


#include <stdio.h>

#include "types.h"
#include "data.h"


/**
 * import datafile
 * @param a_file
 * @return
 */
nvIPFIX_data_record_list_t * nvipfix_import( FILE * a_file );

/**
 * import datafile
 * @param a_fileName filename
 * @return
 */
nvIPFIX_data_record_list_t * nvipfix_import_file( const nvIPFIX_TCHAR * a_fileName );

#ifdef NVIPFIX_DEF_ENABLE_NVC

/**
 *
 * @return
 */
nvIPFIX_data_record_list_t * nvipfix_import_nvc( const nvIPFIX_CHAR * a_host, 
    const nvIPFIX_CHAR * a_login, const nvIPFIX_CHAR * a_password );

#endif


#endif /* __NVIPFIX_IMPORT_H */
