/*****************************************************************************
*!
*! FILE NAME  : application.c
*!
*! DESCRIPTION: Handles an application running on the target.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "application.h"
#include "file.h"
#include "function.h"
#include "map.h"
#include "debug.h"
#include "profile.h"

/****************** CONSTANT AND MACRO SECTION ******************************/

/****************** TYPE DEFINITION SECTION *********************************/

/****************** LOCAL FUNCTION DECLARATION SECTION **********************/

static void add_maps(struct application *app);

/****************** GLOBAL VARIABLE DECLARATION SECTION *********************/

/****************** LOCAL VARIABLE DECLARATION SECTION **********************/

/****************** FUNCTION DEFINITION SECTION *****************************/

/* Add list of binaries mapped for this application */
static void add_maps(struct application *app)
{
	struct map *m;

	while ((m = get_map_entry(app))) {
		m->functions = copy_functions(m);

		m->next = app->maps;
		app->maps = m;
	}
}

static void add_maps_from(struct application *app, struct application *src)
{
	struct map *m;
	if (src == NULL)
		return;
	m = src->maps;
	while (m) {
		struct map *cpy = copy_map(m);
		cpy->next = app->maps;
		app->maps = cpy;
		m = m->next;
	}
}

/* Get information about application from target */
struct application *add_application(unsigned int pid,
				    struct application *kernel)
{
	char *command;
	struct application *app;

	/* Extract the name of the application by looking in proc/pid/cmdline */
	command = pid ? get_pid_file(pid, "cmdline") : strdup("kernel");
	if (!command)
		return NULL;

	DEBUG(1, "Got new application %s (pid %d)\n", command, pid);

	app = malloc(sizeof(struct application));
	strcpy(app->name, command);
	app->next = NULL;
	app->pid = pid;
	app->samples = 0;
	app->printed = 0;
	app->maps = NULL;
	add_maps(app);
	add_maps_from(app, kernel);
	free(command);

	return app;
}

/****************** END OF FILE application.c *******************************/
