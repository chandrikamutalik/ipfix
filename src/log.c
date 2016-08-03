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
#include <unistd.h>
#include <sys/fcntl.h>

#include "log4c.h"

#include "include/types.h"
#include "include/log.h"

extern unsigned char log4cfg[];
extern int log4cfg_size;

#define NVIPFIX_LOG_CATEGORY_NAME_MAIN "nvipfix.log.app"
#define NVIPFIX_LOG_CATEGORY_NAME_ERROR "nvipfix.log.app.error"

#define NVIPFIX_TLOG( a_fmt, a_priority ) \
	va_list args; \
	va_start( args, a_fmt ); \
	nvipfix_tlog( a_priority, a_fmt, &args ); \
	va_end( args )

#define NVIPFIX_LOG( a_fmt, a_priority ) \
	va_list args; \
	va_start( args, a_fmt ); \
	nvipfix_log( a_priority, a_fmt, &args ); \
	va_end( args )


static void nvipfix_log_init( void );
static void nvipfix_log_cleanup( void );
static void nvipfix_log( int a_priority, const char * a_fmt, va_list * args );
static void nvipfix_tlog( int a_priority, const nvIPFIX_TCHAR * a_fmt, va_list * args );
static void nvipfix_log_message( int a_priority, const char * a_message );


enum {
	SizeofVariadicBuffer = 1024
};


static log4c_category_t * Category = NULL;
static log4c_category_t * CategoryError = NULL;


void nvipfix_log_set_config_path( const nvIPFIX_CHAR * a_path )
{

}

void nvipfix_tlog_trace( const nvIPFIX_TCHAR * a_fmt, ... )
{
	NVIPFIX_TLOG( a_fmt, LOG4C_PRIORITY_TRACE );
}

void nvipfix_log_trace( const nvIPFIX_CHAR * a_fmt, ... )
{
	NVIPFIX_LOG( a_fmt, LOG4C_PRIORITY_TRACE );
}

void nvipfix_tlog_debug( const nvIPFIX_TCHAR * a_fmt, ... )
{
	NVIPFIX_TLOG( a_fmt, LOG4C_PRIORITY_DEBUG );
}

void nvipfix_log_debug( const nvIPFIX_CHAR * a_fmt, ... )
{
	NVIPFIX_LOG( a_fmt, LOG4C_PRIORITY_DEBUG );
}

void nvipfix_tlog_info( const nvIPFIX_TCHAR * a_fmt, ... )
{
	NVIPFIX_TLOG( a_fmt, LOG4C_PRIORITY_INFO );
}

void nvipfix_log_info( const nvIPFIX_CHAR * a_fmt, ... )
{
	NVIPFIX_LOG( a_fmt, LOG4C_PRIORITY_INFO );
}

void nvipfix_tlog_warning( const nvIPFIX_TCHAR * a_fmt, ... )
{
	NVIPFIX_TLOG( a_fmt, LOG4C_PRIORITY_WARN );
}

void nvipfix_log_warning( const nvIPFIX_CHAR * a_fmt, ... )
{
	NVIPFIX_LOG( a_fmt, LOG4C_PRIORITY_WARN );
}

void nvipfix_tlog_error( const nvIPFIX_TCHAR * a_fmt, ... )
{
	NVIPFIX_TLOG( a_fmt, LOG4C_PRIORITY_ERROR );
}

void nvipfix_log_error( const nvIPFIX_CHAR * a_fmt, ... )
{
	NVIPFIX_LOG( a_fmt, LOG4C_PRIORITY_ERROR );
}

void nvipfix_log_init( void )
{
	static volatile bool isInitialized = false;

	#pragma omp critical (nvipfixCritical_LogInit)
	{
		if (!isInitialized) {
			/*
			 * Copy default log4c config if not already present.
			 */
			if (access(LOG4C_CFG, F_OK) != 0) {
				FILE *fh = fopen(LOG4C_CFG, "w");
				fwrite(log4cfg, log4cfg_size, 1, fh);
				fclose(fh);
			}
			log4c_init();
			log4c_load( LOG4C_CFG );
			Category = log4c_category_get( NVIPFIX_LOG_CATEGORY_NAME_MAIN );
			CategoryError = log4c_category_get( NVIPFIX_LOG_CATEGORY_NAME_ERROR );
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
	char buffer[SizeofVariadicBuffer + 1];

	if (vsnprintf( buffer, sizeof buffer, a_fmt, *args ) > 0) {
		nvipfix_log_message( a_priority, buffer );
	}
}

void nvipfix_tlog( int a_priority, const nvIPFIX_TCHAR * a_fmt, va_list * args )
{
#ifdef NVIPFIX_DEF_UNICODE
	wchar_t buffer[SizeofVariadicBuffer + 1];

	if (vswprintf( buffer, (sizeof buffer) / sizeof (wchar_t), a_fmt, *args ) > 0) {
		char bufferA[SizeofVariadicBuffer + 1];
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

	#pragma omp critical (nvipfixCritical_LogLog)
	{
		if (a_priority <= LOG4C_PRIORITY_ERROR) {
			log4c_category_log( CategoryError, a_priority, "%s", a_message );
		}
		else {
			log4c_category_log( Category, a_priority, "%s", a_message );
		}
	}
}
