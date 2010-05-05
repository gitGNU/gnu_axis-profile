/*****************************************************************************
*!
*! FILE NAME  : function.c
*!
*! DESCRIPTION: A mmap:ed binary.
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

#define _GNU_SOURCE                     /* To get getline() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "profile.h"
#include "debug.h"
#include "file.h"
#include "map.h"
#include "function.h"

/****************** CONSTANT AND MACRO SECTION ******************************/

/****************** TYPE DEFINITION SECTION *********************************/

/****************** LOCAL FUNCTION DECLARATION SECTION **********************/

static char *get_map(unsigned int pid);

/****************** GLOBAL VARIABLE DECLARATION SECTION *********************/

/****************** LOCAL VARIABLE DECLARATION SECTION **********************/

/****************** FUNCTION DEFINITION SECTION *****************************/

struct map *copy_map(struct map *map)
{
	struct map *ret = malloc(sizeof(struct map));
	memcpy(ret, map, sizeof(struct map));
	ret->functions = copy_functions_kernel(map);
	return ret;
}

static char *get_map(unsigned int pid)
{
	DEBUG(2, "Get map for pid %d\n", pid);

	if (!pid) {
		size_t map_size = MAX_STRING_LEN;
		char *modules = get_proc_file("modules");
		char *symbols = NULL;
		char *map = malloc(map_size);
		char *mod;
		char *next_mod;
		unsigned int length;
		unsigned int start;

		symbols = get_proc_file("kallsyms");
		if (!symbols) {
			symbols = get_proc_file("ksyms");
		}

		/* Create a faked maps file with kernel and modules */
		sprintf(map, "c0000000-c0200000 r-xp 00000000 00:00 200000 kernel\n");

		for (mod = modules; mod; mod = next_mod) {
			char module_name[MAX_STRING_LEN];
			char *sym;
			char *next_sym;

			if ((next_mod = strchr(mod, '\n'))) {
				next_mod++;
			}

			if (sscanf(mod, "%s %d %*s %*s Live %X",
				   module_name, &length, &start) == 3) {
				char tmp[MAX_STRING_LEN];
				sprintf(tmp,
					"%X-%X r-xp 00000000 00:00 %d %s\n",
					start, start+length, length, module_name);
				if (strlen(map) + strlen(tmp) >= map_size) {
					char *tmp_map;

					map_size += MAX_STRING_LEN;
					if (!(tmp_map = realloc(map, map_size))) {
						fprintf(stderr,
							"%s: Out of memory!\n",
							__FUNCTION__);
						exit(EXIT_FAILURE);
					}
					map = tmp_map;
				}
				strcat(map, tmp);
			}

			if (sscanf(mod, "%s %*s %*s", module_name) != 1)
				break;

			for (sym = symbols; sym; sym = next_sym) {
				char name[MAX_STRING_LEN];

				if ((next_sym = strchr(sym, '\n'))) {
					next_sym++;
				}

				if (sscanf(sym, "%X %s %*s", &start, name) == 2) {
					char tmp[MAX_STRING_LEN];

					sprintf(tmp, "__insmod_%s_S.text_L%%u", module_name);
					if (sscanf(name, tmp, &length) == 1) {
						sprintf(tmp, "%X-%X r-xp 00000000 00:00 %d %s\n",
								start, start+length, length, module_name);

						if (strlen(map) + strlen(tmp) >= map_size) {
							char *tmp_map;

							map_size += MAX_STRING_LEN;
							if (!(tmp_map = realloc(map, map_size))) {
								fprintf(stderr, "%s: Out of memory!\n", __FUNCTION__);
								exit(EXIT_FAILURE);
							}

							map = tmp_map;
						}
						strcat(map, tmp);
						break;
					}
				}
			}
		}
		free(modules);
		free(symbols);
		return map;
	} else {
		return get_pid_file(pid, "maps");
	}
}

struct map *get_map_entry(struct application *app)
{
	static char *map;
	static char *next_line;
	struct map *m = malloc(sizeof(struct map));
	char *line;
	int first = 0;

	m->functions = NULL;
	m->next = NULL;

	if (!map) {
		if (!(map = get_map(app->pid))) {
			free(m);
			return NULL;
		}

		next_line = map;
		first = 1;
	}

	for (line = next_line; line; line = next_line) {
		char modifier[5];

		if ((next_line = strchr(line, '\n'))) {
			next_line++;
		}

		if (sscanf(line, "%X-%X %5s %*s %*s %*s %s",
			   &m->start, &m->end, modifier, m->name) == 4 &&
			   !strcmp(modifier, "r-xp")) {
			DEBUG(2, "Got map entry %s %X-%X\n",
				m->name, m->start, m->end);
			m->offset = first ? 0 : m->start;
			return m;
		}
	}

	free(map);
	map = NULL;

	free(m);

	return NULL;
}

/****************** END OF FILE map.c ***************************************/
