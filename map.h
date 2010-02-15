/*****************************************************************************
*!
*! FILE NAME  : map.h
*!
*! ---------------------------------------------------------------------------
*! Copyright (C) 2004, Axis Communications AB, LUND, SWEDEN
*!***************************************************************************/

/**
 *  This file is part of axis_profile.
 *
 *  axis_profile is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  axis_profile is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with axis_profile.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MAP_H
#define MAP_H

/****************** INCLUDE FILES SECTION ***********************************/

#include "profile.h"
#include "application.h"

/****************** TYPE DEFINITION SECTION *********************************/

/* Mapped application or lib */
struct map {
	struct map *next;
	struct function *functions;
	char name[MAX_STRING_LEN];
	unsigned int start;  /* Start address of this mmap_ed binary */
	unsigned int end;    /* Last address */
	unsigned int offset; /* Offset to add to start of library */
};

/****************** EXPORTED FUNCTION DECLARATION SECTION *******************/

struct map *get_map_entry(struct application *app);
struct map *copy_map(struct map *map);

#endif
/****************** END OF FILE map.h ***************************************/
