/*****************************************************************************
*!
*! FILE NAME  : function.h
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

#ifndef FUNCTION_H
#define FUNCTION_H

/****************** INCLUDE FILES SECTION ***********************************/

#include "profile.h"
#include "map.h"

/****************** TYPE DEFINITION SECTION *********************************/

struct function {
	struct function *next;

	struct map *map;

	char *name;

	unsigned long start;   /* Start address for the function */
	unsigned long end;     /* Last address in the function */

	unsigned long samples; /* Number of samples in this function */

	int printed;           /* 1 if this function has been listed in output yet */
};

/****************** EXPORTED FUNCTION DECLARATION SECTION *******************/

struct function *copy_functions(struct map *map);
struct function *copy_functions_kernel(struct map *map);

#endif
/****************** END OF FILE function.h **********************************/
