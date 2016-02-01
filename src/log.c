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
#include <stdio.h>
#include <wchar.h>

#include "log4c.h"

#include "include/types.h"

#include "include/log.h"


static void nvipfix_log_init( void );
static void nvipfix_log_cleanup( void );
static void nvipfix_log( int a_priority, const char * a_fmt, va_list * args );
static void nvipfix_tlog( int a_priority, const nvIPFIX_TCHAR * a_fmt, va_list * args );
static void nvipfix_log_message( int a_priority, const char * a_message );


enum {
	SizeofVariadicBufferSize = 1024
};

static log4c_category_t * Category;


void nvipfix_tlog_debug( const nvIPFIX_TCHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_tlog( LOG4C_PRIORITY_DEBUG, a_fmt, &args );

	va_end( args );
}

void nvipfix_log_debug( const nvIPFIX_CHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_log( LOG4C_PRIORITY_DEBUG, a_fmt, &args );

	va_end( args );
}

void nvipfix_tlog_info( const nvIPFIX_TCHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_tlog( LOG4C_PRIORITY_INFO, a_fmt, &args );

	va_end( args );
}

void nvipfix_log_info( const nvIPFIX_CHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_log( LOG4C_PRIORITY_INFO, a_fmt, &args );

	va_end( args );
}

void nvipfix_tlog_warning( const nvIPFIX_TCHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_tlog( LOG4C_PRIORITY_WARN, a_fmt, &args );

	va_end( args );
}

void nvipfix_log_warning( const nvIPFIX_CHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_log( LOG4C_PRIORITY_WARN, a_fmt, &args );

	va_end( args );
}

void nvipfix_tlog_error( const nvIPFIX_TCHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_tlog( LOG4C_PRIORITY_ERROR, a_fmt, &args );

	va_end( args );
}

void nvipfix_log_error( const nvIPFIX_CHAR * a_fmt, ... )
{
	va_list args;
	va_start( args, a_fmt );

	nvipfix_log( LOG4C_PRIORITY_ERROR, a_fmt, &args );

	va_end( args );
}

void nvipfix_log_init( void )
{
	static volatile bool isInitialized = false;

	#pragma omp critical (nvipfixCritical_LogInit)
	{
		if (!isInitialized) {
			log4c_init();
			Category = log4c_category_get( "nvipfix.app" );
			atexit( nvipfix_log_cleanup );
			isInitialized = true;
		}
	}
}

void nvipfix_log_cleanup( void )
{
	log4c_fini();
}

void nvipfix_log( int a_priority, const char * a_fmt, va_list * args )
{
	char buffer[SizeofVariadicBufferSize + 1];

	if (vsnprintf( buffer, sizeof buffer, a_fmt, *args ) > 0) {
		nvipfix_log_message( a_priority, buffer );
	}
}

void nvipfix_tlog( int a_priority, const nvIPFIX_TCHAR * a_fmt, va_list * args )
{
#ifdef NVIPFIX_DEF_UNICODE
	wchar_t buffer[SizeofVariadicBufferSize + 1];

	if (vswprintf( buffer, (sizeof buffer) / sizeof (wchar_t), a_fmt, *args ) > 0) {
		char bufferA[SizeofVariadicBufferSize + 1];
		mbstate_t state = { 0 };

		const wchar_t * bufferPtr = buffer;

		if (wcsrtombs( bufferA, &bufferPtr, sizeof bufferA, &state ) > 0) {
			nvipfix_log_message( a_priority, bufferA );
		}
	}
#else
	nvipfix_log( a_priority, a_fmt, args );
#endif
}

void nvipfix_log_message( int a_priority, const char * a_message )
{
	nvipfix_log_init();
	log4c_category_log( Category, a_priority, "%s", a_message );
}
