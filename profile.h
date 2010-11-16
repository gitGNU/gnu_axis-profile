/*****************************************************************************
*!
*! FILE NAME  : profile.h
*!
*! DESCRIPTION: Common defines and variables
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

#ifndef PROFILE_H
#define PROFILE_H

/****************** INCLUDE FILES SECTION ***********************************/

/****************** CONSTANT AND MACRO SECTION ******************************/

#define MAX_STRING_LEN 512

enum dump_symbols {
	readelf = 0,
	nm,
	objdump,
};

/****************** GLOBAL VARIABLES SECTION ********************************/

extern char *top_dir;
extern char *kernel_dir;
extern char remote_host[];
extern char cpu_arch[];

enum dump_symbols get_dump_symbols(void);

#endif
/****************** END OF FILE profile.h ***********************************/
