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

#include "include/types.h"

#ifdef NV_IPFIX_USE_INOTIFY
#include <sys/inotify.h>
#endif

#include "include/log.h"

#include "include/fwatch.h"


#ifdef NV_IPFIX_USE_INOTIFY
static int inotifyDescriptor;
#endif


nvIPFIX_FWATCH_HANDLE nvipfix_fwatch_add()
{
	static bool isInitialized = false;
	nvIPFIX_FWATCH_HANDLE result = NV_IPFIX_INVALID_FWATCH_HANDLE;

	if (!isInitialized)
	{
#ifdef NV_IPFIX_USE_INOTIFY
		inotifyDescriptor = inotify_init();
		isInitialized = (inotifyDescriptor != -1);
#endif
	}

	return result;
}
