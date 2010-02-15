/*****************************************************************************
*!
*! FILE NAME  : application.h
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

#ifndef APPLICATION_H
#define APPLICATION_H

/****************** INCLUDE FILES SECTION ***********************************/

#include "profile.h"

/****************** CONSTANT AND MACRO SECTION ******************************/

/****************** TYPE DEFINITION SECTION *********************************/

struct application {
	struct application *next;
	int pid;     /* PID for this application */
	int samples; /* Number of samples in this application */
	int printed; /* 1 if this application has been listed in output */
	char name[MAX_STRING_LEN];
	struct map *maps; /* All mmap:ed areas */
};

/****************** EXPORTED FUNCTION DECLARATION SECTION *******************/

struct application *add_application(unsigned int pid,
				    struct application *kernel);

#endif
/****************** END OF FILE application.h *******************************/
