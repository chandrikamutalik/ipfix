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

#ifndef __NVIPFIX_LOG_H
#define __NVIPFIX_LOG_H


#include "types.h"


#ifdef NVIPFIX_DEF_DEBUG
#define NVIPFIX_TLOG_DEBUG0( a_fmt, ... ) nvipfix_tlog_debug( NVIPFIX_T( "" NVIPFIX_F_S ": " a_fmt ), __func__, __VA_ARGS__ )
#define NVIPFIX_LOG_DEBUG0( a_fmt, ... ) nvipfix_log_debug( "%s: " a_fmt, __func__, __VA_ARGS__ )
#else // !NVIPFIX_DEF_DEBUG
#define NVIPFIX_TLOG_DEBUG0( a_fmt, ... )
#define NVIPFIX_LOG_DEBUG0( a_fmt, ... )
#endif // NVIPFIX_DEF_DEBUG

#define NVIPFIX_TLOG_DEBUG( a_fmt, ... ) nvipfix_tlog_debug( NVIPFIX_T( "" NVIPFIX_F_S ": " a_fmt ), __func__, __VA_ARGS__ )
#define NVIPFIX_LOG_DEBUG( a_fmt, ... ) nvipfix_log_debug( "%s: " a_fmt, __func__, __VA_ARGS__ )
#define NVIPFIX_TLOG_INFO( a_fmt, ... ) nvipfix_tlog_info( NVIPFIX_T( a_fmt ), __VA_ARGS__ )
#define NVIPFIX_TLOG_WARNING( a_fmt, ... ) nvipfix_tlog_warning( NVIPFIX_T( a_fmt ), __VA_ARGS__ )
#define NVIPFIX_TLOG_ERROR( a_fmt, ... ) nvipfix_tlog_error( NVIPFIX_T( a_fmt ), __VA_ARGS__ )
#define NVIPFIX_TLOG_ERROR_IF( a_condition, a_fmt, ... ) if ((a_condition)) { \
	nvipfix_tlog_error( NVIPFIX_T( a_fmt ), __VA_ARGS__ ); }


/**
 *
 */
void nvipfix_tlog_debug( const nvIPFIX_TCHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_log_debug( const nvIPFIX_CHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_tlog_info( const nvIPFIX_TCHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_log_info( const nvIPFIX_CHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_tlog_warning( const nvIPFIX_TCHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_log_warning( const nvIPFIX_CHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_tlog_error( const nvIPFIX_TCHAR * a_fmt, ... );

/**
 *
 */
void nvipfix_log_error( const nvIPFIX_CHAR * a_fmt, ... );


#endif /* __NVIPFIX_LOG_H */
