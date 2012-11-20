/*****************************************************************************
*!
*! FILE NAME  : function.c
*!
*! DESCRIPTION: A function in some binary that is mapped in an application.
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
#include <unistd.h>
#include <ctype.h>

#include "debug.h"
#include "function.h"
#include "profile.h"
#include "file.h"

/****************** CONSTANT AND MACRO SECTION ******************************/

#define CACHE_NAME "axis_profile_cache.txt"

/****************** TYPE DEFINITION SECTION *********************************/

struct symbol_table
{
	struct symbol_table *next;
	char *name;
	struct function *symbols;
};

/****************** LOCAL FUNCTION DECLARATION SECTION **********************/

static struct symbol_table *get_functions(const char *name);
static struct symbol_table *find_symbol_table(const char *name);
static struct symbol_table *read_symbol_table(const char *name,
                                              FILE *symbol_file);
static char *find_binary(const char *app);

/****************** GLOBAL VARIABLE DECLARATION SECTION *********************/

/****************** LOCAL VARIABLE DECLARATION SECTION **********************/

static struct symbol_table *symbol_tables;
static struct symbol_table *last_symbol_table;

/****************** FUNCTION DEFINITION SECTION *****************************/

struct function *copy_functions(struct map *map)
{
	struct symbol_table *symbol_table;
	struct function *symbols = NULL;
	struct function *last_symbol = NULL;
	struct function *new_symbol;

	if (!(symbol_table = get_functions(map->name)) ||
	    !symbol_table->symbols) {
		DEBUG(1, "No symbols found for %s\n", map->name);
	} else {
		struct function *symbol;

		for (symbol = symbol_table->symbols;
		     symbol;
		     symbol = symbol->next) {
			new_symbol = malloc(sizeof(struct function));
			*new_symbol = *symbol;

			new_symbol->map = map;

			if (!symbols) {
				symbols = new_symbol;
			} else {
				last_symbol->next = new_symbol;
			}
			last_symbol = new_symbol;
		}
	}

	/* Add a catch-all symbol at the end. */
	new_symbol = calloc(1, sizeof(struct function));
	new_symbol->map = map;
	new_symbol->name = strdup("<unknown>");
	new_symbol->end = map->end - map->offset;

	if (!symbols) {
		symbols = new_symbol;
	} else {
		last_symbol->next = new_symbol;
	}

	return symbols;
}

struct function *copy_functions_kernel(struct map *map)
{
	struct function *f = map->functions;
	struct function *ret = NULL;
	struct function *tmp;

	while (f) {
		if (f->next == NULL)
			break;

		tmp = malloc(sizeof(struct function));
		memcpy(tmp, f, sizeof(struct function));
		tmp->name = strdup(f->name);
		f = f->next;
		tmp->next = ret;
		ret = tmp;
	}
	return ret;
}

static struct symbol_table *get_functions(const char *name)
{
	FILE *f;
	struct symbol_table *symbol_table;
	char path[MAX_STRING_LEN];
	char app[MAX_STRING_LEN];
	char command[MAX_STRING_LEN];
	char *binary;
	char *fmt = NULL;
	int ret;

	DEBUG(3, "Get functions from %s\n", name);

	/* We already have the kernel symbols */
	if (!strcmp(name, "kernel")) {
		if ((symbol_table = find_symbol_table(name))) {
			return symbol_table;
		}

		sprintf(path, "%s/System.map", kernel_dir);
		if (!(f = fopen(path, "r"))) {
			DEBUG(1, "Could not extract kernel symbols: %m\n");
			return NULL;
		}

		symbol_table = read_symbol_table(name, f);

		fclose(f);

		return symbol_table;
	}

	/* Get the real application name. */
	strcpy(app, name);
	sprintf(path, "%s/%s\n", top_dir, name);
	ret = readlink(path, app, MAX_STRING_LEN);
	if (ret)
		app[ret] = '\0';
	if (strrchr(app, '/'))
		memmove(app, strrchr(app, '/') + 1, strlen(app));

	/* Check if we already got it. */
	DEBUG(3, "Search for binary %s\n", app);

	if ((symbol_table = find_symbol_table(app))) {
		return symbol_table;
	}

	/* Did not find it. Begin the quest to find the symbols */
	if (!(binary = find_binary(app))) {
		return NULL;
	}

	if (get_dump_symbols() == readelf) {
		fmt = "readelf -s %s 2>/dev/null | "
			"sed \"1,/Symbol table '.symtab'/ d\" | "
			"cut -d : -f 2 | "
			"sort | "
			"awk '/ FUNC / && !/ UND / { print $1 \" \" $2 \" \" $7 }'";
	} else if (get_dump_symbols() == nm) {
		fmt = "nm -n -f bsd -S %s 2>/dev/null | "
			"grep -i ' t ' | "
			"sed 's/ [tT] / /'";

	} else if (get_dump_symbols() == objdump) {
		fmt = "objdump -t %s 2>/dev/null | "
			"sort | "
			"grep ' F ' | "
			"sed 's/\t/ /'| "
			"cut -c9-22 --complement | "
			"awk '{ print $1 \" \" $2 \" \" $3 }'";
	}
	sprintf(command, fmt, binary);
	DEBUG(20, "Symbol dump command:\n===\n%s\n===\n", command);
	if (!(f = popen(command, "r"))) {
		DEBUG(1, "Could not extract symbols from %s: %m\n", binary);
		free(binary);
		return NULL;
	}

	symbol_table = read_symbol_table(app, f);

	ret = pclose(f);
	if (ret < 0 || !WIFEXITED(ret) || WEXITSTATUS(ret)) {
		DEBUG(1, "Could not extract symbols from %s\n", binary);
		free(binary);
		return NULL;
	}

	free(binary);

	return symbol_table;
}

static struct symbol_table *find_symbol_table(const char *name)
{
	struct symbol_table *symbol_table;

	for (symbol_table = symbol_tables;
			symbol_table;
			symbol_table = symbol_table->next)
	{
		if (!strcmp(symbol_table->name, name))
		{
			break;
		}
	}

	return symbol_table;
}

static struct symbol_table *
read_symbol_table(const char *name, FILE *symbol_file)
{
	struct symbol_table *symbol_table;
	struct function *symbols = NULL;
	struct function *last_symbol = NULL;
	struct function *symbol = calloc(1, sizeof(struct function));
	char *line = NULL;
	char type[20];
	size_t size;
	memset(symbol, 0, sizeof(struct function));

	while (getline(&line, &size, symbol_file) != -1) {
		if (sscanf(line, "%lX %s %as",
			   &symbol->start, type, &symbol->name) == 3) {
			if (last_symbol &&
			    symbol->start == last_symbol->start) {
				DEBUG(3, "Ignoring duplicate symbol %s "
					"(same as %s)\n",
					symbol->name, last_symbol->name);
				continue;
			} else if (symbol->name[0] == '$') {
				/* Ignore $a, $b etc. They are issued */
				/* all over the place by the ARM gcc 3.4.4 */
				/* at least and obscure the real symbols. */
				continue;
			} else if (isdigit(*type) || isxdigit(*type)) {
				unsigned int val;
				if (get_dump_symbols() == readelf) {
					val = atoi(type);
				} else {
					val = strtol(type, NULL, 16);
				}
				symbol->end = (0xffffffff & symbol->start) + val;
			} else if (*type != 't' && *type != 'T') {
				continue;
			}

			symbol->next = NULL;

			if (!symbols) {
				symbols = symbol;
			} else {
				last_symbol->next = symbol;
			}
			last_symbol = symbol;

			symbol->end &= 0xffffffff;
			symbol->start &= 0xffffffff;

			DEBUG(2, "Got function %s %lX (%ld)\n",
				symbol->name, symbol->start,
				symbol->end - symbol->start);

			symbol = calloc(1, sizeof(struct function));
			memset(symbol, 0, sizeof(struct function));
		}
	}

	free(line);
	free(symbol);

	if (symbols) {
		/* Calculate the end of all symbols (that do not have one yet). */
		for (symbol = symbols; symbol->next; symbol = symbol->next) {
			if (!symbol->end) {
				symbol->end = symbol->next->start;
			}
			DEBUG(30, "Symbol %s (%lX - %lX)\n",
				symbol->name,
				symbol->start, symbol->end);
		}

		/* Set the end of the last symbol (if it is not set already). */
		if (!symbol->end) {
			symbol->end = symbol->start;
		}
	}

	symbol_table = malloc(sizeof(struct symbol_table));
	symbol_table->next = NULL;
	symbol_table->name = strdup(name);
	symbol_table->symbols = symbols;

	if (!symbol_tables) {
		symbol_tables = last_symbol_table = symbol_table;
	} else {
		last_symbol_table->next = symbol_table;
		last_symbol_table = symbol_table;
	}

	return symbol_table;
}

static void add_binary_to_cache(const char *app, const char *path)
{
	char *command;

	asprintf(&command,
		"grep -v '^%s:' %s/" CACHE_NAME " 2>/dev/null | "
		"( cat; echo '%s:%s' ) > %s/" CACHE_NAME ".tmp; "
		"mv -f %s/" CACHE_NAME ".tmp %s/" CACHE_NAME "",
		app, top_dir, app, path, top_dir, top_dir, top_dir);
	if (command) {
		system(command);
		free(command);
	}
}

static char *find_binary_in_cache(const char *app)
{
	FILE *f;
	char *command;
	char *binary = NULL;

	asprintf(&command,
		"grep '^%s:' %s/" CACHE_NAME " 2>/dev/null | cut -d : -f 2",
		app, top_dir);
	if (command && (f = popen(command, "r"))) {
		char line[MAX_STRING_LEN];

		if (fgets(line, sizeof(line), f)) {
			line[strlen(line) - 1] = '\0';
			binary = find_file(NULL, line);
		}

		pclose(f);
		free(command);
	}

	return binary;
}

static char *find_binary(const char *app)
{
	char *binary;

	DEBUG(2, "No cached symbol file for %s, search\n", app);

	/* First check if the binary is known in the file cache. */
	binary = find_binary_in_cache(app);

	/* Five possiblities:
	 * 1: Binary exist under libs.
	 * 2: Binary exist under apps.
	 * 3: Binary exist under packages.
	 * 4. Binary is a module
	 * 5. Binary exist under target.
	 */

	if (!binary)
		binary = find_file("libs", app); /* Case 1 */
	if (!binary)
		binary = find_file("apps", app); /* Case 2 */
	if (!binary)
		binary = find_file("packages", app); /* Case 3 */
	if (!binary) {
		char tmp[MAX_STRING_LEN];

		strcpy(tmp, app);
		strcat(tmp, ".o");
		binary = find_file("modules", tmp); /* Case 4 */
		if (!binary) {
			strcpy(tmp, app);
			strcat(tmp, ".ko");
			binary = find_file("modules", tmp);
			if (!binary) {
				char *t;

				strcpy(tmp, app);
				t = strchr(tmp, '_');
				if (t) {
					*t = '-';
					strcat(tmp, ".ko");
					binary = find_file("modules", tmp);
				}
			}
		}
	}
	if (!binary)
		binary = find_file("target", app); /* Case 5 */
	if (!binary) {
		DEBUG(1,"Application %s not found!\n", app);
		return NULL;
	}

	DEBUG(1, "Found binary %s\n", binary);

	add_binary_to_cache(app, binary);

	return binary;
}

/****************** END OF FILE function.c **********************************/
