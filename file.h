/*****************************************************************************
*!
*! FILE NAME  : file.h
*!
*! DESCRIPTION: Misc file utilities
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

#ifndef FILE_H
#define FILE_H

/****************** INCLUDE FILES SECTION ***********************************/

#include <stdio.h>

/****************** EXPORTED FUNCTION DECLARATION SECTION *******************/

char *get_pid_file(unsigned int pid, const char *filename);
char *get_proc_file(const char *filename);
char *find_file(const char *top_path, const char *filename);

#endif
/****************** END OF FILE file.h **************************************/
