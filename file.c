/*****************************************************************************
*!
*! FILE NAME  : file.c
*!
*! DESCRIPTION: File utilities.
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

/****************** INCLUDE FILES SECTION ***********************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "file.h"
#include "profile.h"
#include "errno.h"

/****************** CONSTANT AND MACRO SECTION ******************************/

/****************** TYPE DEFINITION SECTION *********************************/

/****************** LOCAL FUNCTION DECLARATION SECTION **********************/

/****************** GLOBAL VARIABLE DECLARATION SECTION *********************/

/****************** LOCAL VARIABLE DECLARATION SECTION **********************/

/****************** FUNCTION DEFINITION SECTION *****************************/

char *get_pid_file(unsigned int pid, const char *filename)
{
	char proc_file[MAX_STRING_LEN];

	sprintf(proc_file, "%d/%s", pid, filename);
	return get_proc_file(proc_file);
}

char *get_proc_file(const char *filename)
{
	FILE *f;
	size_t buffer_size = MAX_STRING_LEN;
	char command[MAX_STRING_LEN];
	char *file = malloc(buffer_size + 1);
	size_t file_size = 0;
	int ret;

	sprintf(command,
		"wget --proxy=off -O - ftp://root:pass@%s/proc/%s 2>/dev/null",
		remote_host, filename);
	if (!(f = popen(command, "r"))) {
		printf("Could not retrieve proc/%s: %m\n", filename);
		exit(EXIT_FAILURE);
	}

	while (!feof(f)) {
		size_t bytes = fread(file + file_size, 1, MAX_STRING_LEN, f);

		file_size += bytes;

		if (bytes < MAX_STRING_LEN) {
			break;
		}

		file = realloc(file, buffer_size += MAX_STRING_LEN);
	}

	file[file_size] = '\0';

	ret = pclose(f);

	if (ret < 0 || !WIFEXITED(ret) || WEXITSTATUS(ret)) {
		free(file);
		return NULL;
	}

	return file;
}

char *find_file(const char *top_path, const char *filename)
{
	char command[MAX_STRING_LEN];
	char *line = malloc(MAX_STRING_LEN);
	FILE *f;
	char *ret;

	if (top_path) {
		sprintf(command,
			"find %s/%s \\( -type f -o -type l \\) -name %s "
			"-exec file -L {} \\; 2> /dev/null | grep ELF | cut -d : -f 1",
			top_dir, top_path, filename);
	} else {
		sprintf(command,
			"file -L %s 2> /dev/null | grep ELF | cut -d : -f 1", filename);
	}
	if (!(f = popen(command, "r"))) {
		return NULL;
	}
	ret = fgets(line, MAX_STRING_LEN, f);
	pclose(f);

	if (!ret) {
		free(line);
		return NULL;
	}

	line[strlen(line) - 1] = '\0';
	return line;
}

/****************** END OF FILE file.c **************************************/
