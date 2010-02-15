/*****************************************************************************
*!
*! FILE NAME  : debug.h
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

#ifndef DEBUG_H
#define DEBUG_H

/****************** CONSTANT AND MACRO SECTION ******************************/

#define DEBUG(level, x...) \
	do { \
		if (verbose >= (level)) \
			fprintf(stderr, x); \
	} while (0)

/****************** GLOBAL VARIABLES SECTION ********************************/

extern int verbose;

#endif
/****************** END OF FILE debug.h *************************************/
