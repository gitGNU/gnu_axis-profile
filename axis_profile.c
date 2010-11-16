/*****************************************************************************
*!
*! FILE NAME  : axis_profile.c
*!
*! DESCRIPTION: Main for the profiler.
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

#define _GNU_SOURCE	/* To get getline() */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <poll.h>

#include "debug.h"
#include "application.h"
#include "function.h"
#include "profile.h"
#include "map.h"

/****************** CONSTANT AND MACRO SECTION ******************************/

#define MAX_TRACED 10

/****************** TYPE DEFINITION SECTION *********************************/

/****************** LOCAL FUNCTION DECLARATION SECTION **********************/

static void usage(void);

static struct application *find_application(int pid);

static void increment_function(struct application *app, unsigned int address);

static void add_sample(int pid, unsigned int address);

static void get_applications(void);

struct application *find_max_app(void);
struct function *find_max_function(struct application *app);

static void handle_signal(int signum);

/****************** GLOBAL VARIABLE DECLARATION SECTION *********************/

int verbose;
char remote_host[MAX_STRING_LEN];
char cpu_arch[MAX_STRING_LEN];
char *top_dir;
char *kernel_dir;

/****************** LOCAL VARIABLE DECLARATION SECTION **********************/

static int sample_interval = 5;
static int function_count = 20;
static int restricted_ps = 0;
static int dump_symbols = readelf;
static int samples;
static int lost_kernel_samples;
static int lost_user_samples;
static int trace_pid[MAX_TRACED];
static int trace_pids;
static char *trace_app[MAX_TRACED];
static int trace_apps;
struct application *applications = NULL;

/****************** FUNCTION DEFINITION SECTION *****************************/

enum dump_symbols get_dump_symbols(void)
{
	return dump_symbols;
}

static void usage(void)
{
	printf("axis_profile version " AXIS_PROFILER_VERSION "\n");
	printf("Usage: axis_profile [options] hostname\n");
	printf("Options:\n");
	printf("-v num    set verbose level (0-3, default 0)\n");
	printf("-c num    set number of functions to print (default 20)\n");
	printf("-s ival   set sample interval (default 5s)\n");
	printf("-p pid    pid to trace (default all)\n");
	printf("-a name   application to trace (default all)\n");
	printf("-b num    use restricted \"ps\": 0 = no (default), 1 = yes\n");
	printf("-d prog   do symbol dump using readelf (default), nm or objcopy\n");
	printf("-h        print this help text\n");
	exit(EXIT_FAILURE);
}

static struct application *find_application(int pid)
{
	struct application *app = applications;

	while (app && app->pid != pid)
		app = app->next;

	return app;
}

#if 0
static void dump(struct map *m)
{
	struct function *f = m->functions;

	while (f)
	{
		printf("Function %s, start %X end %X\n",
			f->name, m->offset + f->start, m->offset + f->end);

		f = f->next;
	}
}
#endif

static void
increment_function(struct application *app, unsigned int address)
{
	struct map *m;
	struct function *f;

	app->samples++;

	/* Find the map that contains the address */
	for (m = app->maps; m; m = m->next) {
		if (address >= m->start && address < m->end) {
			break;
		}
	}

	if (!m) {
		if (app->pid) {
			lost_user_samples++;
		} else {
			lost_kernel_samples++;
		}
		printf("No map: Invalid sample at %X for %s\n", address, app->name);
		return;
	}

	/* Find the function that contains the address */
	for (f = m->functions; f; f = f->next) {
		if (address >= m->offset + f->start &&
		    address < m->offset + f->end) {
			break;
		}
	}

	if (!f) {
		if (app->pid)
			lost_user_samples++;
		else
			lost_kernel_samples++;
		DEBUG(1, "No function: Invalid sample at %X for %s\n",
			address, app->name);
		return;
	}

	/* Store the sample */
	DEBUG(3, "Found address %X in %s:%s\n", address, m->name, f->name);
	if(!f->next) {
		// catchall func
		DEBUG(1, "Address %X in %s unknown\n", address, m->name);
	}
	f->samples++;
}

static void add_sample(int pid, unsigned int address)
{
	struct application *app;

	samples++;

	app = find_application(pid);
	if (app) {
		DEBUG(3, "Add sample pid %d address %X\n", pid, address);
		increment_function(app, address);
	} else {
		DEBUG(1, "Lost sample pid %d address %X\n", pid, address);
		if (pid)
			lost_user_samples++;
		else
			lost_kernel_samples++;
	}
}

/* Get CPU architecture from target */
static void get_cpu_arch(void)
{
	char buffer[MAX_STRING_LEN];
	unsigned int s;
	FILE *f;
	char *line = NULL;

	sprintf(buffer, "profile_run_remote.exp %s 'uname -m'", remote_host);
	f = popen(buffer, "r");
	getline(&line, &s, f);
	free(line);
	line = NULL;
	getline(&line, &s, f);
	/* Strip \r\n */
	line[strlen(line)-2] = '\0';
	strcpy(cpu_arch, line);
	printf("Architecture: %s\n", cpu_arch);
	free(line);
	pclose(f);
}

/* Get all applications from target */
static void get_applications(void)
{
	char buffer[MAX_STRING_LEN];
	unsigned int pid;
	unsigned int size;
	int ret;
	FILE *f;
	struct application *app;
	int i;

	printf("Retrieving application information (takes quite some time)\n");
	/* Add kernel */
	app = add_application(0, NULL);
	if (app) {
		app->next = applications;
		applications = app;
	}

	if (trace_pids) {
		for (i = 0; i < trace_pids; i++) {
			app = add_application(trace_pid[i],
					      find_application(0));
			if (app) {
				app->next = applications;
				applications = app;
			} else {
				fprintf(stderr,
					"Warning: Unknown PID specified: %d\n",
					trace_pid[i]);
			}
		}
		return;
	}

	if (restricted_ps) {
		sprintf(buffer, "profile_run_remote.exp %s 'ps'", remote_host);
	} else {
		sprintf(buffer,
			"profile_run_remote.exp %s 'ps axo pid,user,vsz'",
			remote_host);
	}
	f = popen(buffer, "r");
	while (!feof(f)) {
		char *line = NULL;
		unsigned int s;

		getline(&line, &s, f);
		if (restricted_ps) {
			ret = sscanf(line, "%d%*[ \t]%*s%*[ \t]%d",
				     &pid, &size);
		} else {
			ret = sscanf(line, "%d %*s %d", &pid, &size);
		}
		free(line);
		if ((ret == 2) && (size != 0)) {
			app = add_application(pid, find_application(0));
			if (app) {
				char buffer[MAX_STRING_LEN];
				char *status;
				if (trace_apps) {
					int found = 0;

					for (i = 0; i < trace_apps; i++) {
						if (!strstr(app->name,
							    trace_app[i])) {
							found = 1;
						}
					}
					if (!found) {
						free(app);
						continue;
					}
				}

				/* Check for NPTL threads */
				{
						char buffer[MAX_STRING_LEN];
						unsigned int s;
						FILE* f;
						char* line = NULL;
						int pid2;

						sprintf(buffer, "profile_run_remote.exp %s 'ls -la /proc/%d/task'", remote_host, pid);
						f = popen(buffer, "r");
						while (!feof(f)) {
							getline(&line, &s, f);
							if (sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %d", &pid2) == 1)
							{
								if (pid2 != pid) {
									struct application *app2;
									app2 = add_application(pid2, find_application(0));
									app2->next = applications;
									applications = app2;
								}
							}
							free(line);
							line = NULL;
						}
						pclose(f);

				}
				app->next = applications;
				applications = app;
			}
		}
	}
	pclose(f);
}

/* Find the application that has the most samples of the not yet printed apps.*/
struct application *find_max_app(void)
{
	struct application *best_app = NULL;
	struct application *a = applications;

	while (a) {
		if (!a->printed &&
		    (!best_app || (a->samples > best_app->samples)))
			best_app = a;

		a = a->next;
	}

	if (best_app)
		best_app->printed = 1;

	return best_app;
}

/* Find the function with the most samples if the not yet printed functions */
struct function *find_max_function(struct application *app)
{
	struct function *best_func = NULL;
	struct map *m = app->maps;

	/* Have to look in all maps. */
	while (m) {
		struct function *f = m->functions;

		while (f) {
			if (!f->printed &&
			    (!best_func || (f->samples > best_func->samples)))
				best_func = f;

			f = f->next;
		}

		m = m->next;
	}

	if (best_func)
		best_func->printed = 1;

	return best_func;
}

static void print_stat(struct application *app)
{
	int app_samples;
	int i;

	if (!app->samples)
		return;

	printf("Application %s (pid %d) %d%% (%.2f seconds)\n",
		app->name, app->pid, 100 * app->samples / samples,
		app->samples / 100.0);

	app_samples = 0;
	for (i = 0; i < function_count; i++) {
		struct function *f = find_max_function(app);

		if (!f || !f->samples)
			break;

		if (f->map->offset || !strcmp(f->map->name, "kernel")) {
			printf("%9.2fs %6.2f%%  %-25s  %s\n",
				f->samples / 100.0,
				100.0 * f->samples / app->samples,
				f->name, f->map->name);
		} else {
			printf("%9.2fs %6.2f%%  %-25s\n",
				f->samples / 100.0,
				100.0 * f->samples / app->samples,
				f->name);
		}

		app_samples += f->samples;
	}
	printf("---------- -------\n");
	printf("%9.2fs %6.2f%%\n",
		app_samples / 100.0,
		100.0 * app_samples / app->samples);

	printf("\n");
}

/* Print the statistics */
static void print_stats(void)
{
	struct application* app;

	printf("Total sample time: %.2f seconds\n\n", samples / 100.0);

	while ((app = find_max_app())) {
		print_stat(app);
	}

	/* Reset print flag */
	app = applications;
	while (app) {
		struct map* m = app->maps;
		/* Have to look in all maps. */
		while (m) {
			struct function* f = m->functions;

			while (f) {
				f->printed = 0;
				f = f->next;
			}
			m = m->next;
		}
		app->printed = 0;
		app = app->next;
	}

	DEBUG(1, "Lost %d kernel samples and %d user mode samples\n",
	lost_kernel_samples, lost_user_samples);
}

static void wait_sample_interval(void)
{
	struct pollfd fd = {
		.fd = STDIN_FILENO,
		.events = POLLIN,
		.revents = 0,
	};
	int ret;

	ret = poll(&fd, 1, sample_interval * 1000);

	if (ret > 0) {
		char buf;
		read(STDIN_FILENO, &buf, sizeof(buf));
		print_stats();
	}
}


static void print_and_exit(int exit_code)
{
	print_stats();
	exit(exit_code);
}

static void handle_signal(int signum)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		print_and_exit(0);
		break;
	default:
		printf("Unexpected signal %d caught\n", signum);
		break;
	}
}

int main(int argc, char **argv)
{
	while (1) {
		int c = getopt(argc, argv, "hv:c:s:p:a:b:d:");

		if (c == -1)
			break;

		switch (c) {
		case '?':
		case ':':
			exit(EXIT_FAILURE);
			break;
		case 'h':
			usage();
			break;
		case 'v':
			verbose = atoi(optarg);
			break;
		case 'c':
			function_count = atoi(optarg);
			break;
		case 's':
			sample_interval = atoi(optarg);
			break;
		case 'p':
			if (trace_pids < MAX_TRACED) {
				if (atoi(optarg) >= 1) {
					trace_pid[trace_pids++] = atoi(optarg);
				}
			} else {
				fprintf(stderr,
					"Warning: Too many PIDs specified\n");
			}
			break;
		case 'a':
			if (trace_apps < MAX_TRACED) {
				trace_app[trace_apps++] = optarg;
			} else {
				fprintf(stderr, "Warning: "
					"Too many applications specified\n");
			}
			break;
		case 'b':
			restricted_ps = atoi(optarg);
			break;
		case 'd':
			if (!strncmp(optarg, "readelf", strlen(optarg))) {
				dump_symbols = readelf;
			} else if (!strncmp(optarg, "nm", strlen(optarg))) {
				dump_symbols = nm;
			} else if (!strncmp(optarg, "objdump",
					    strlen(optarg))) {
				dump_symbols = objdump;
			}
			break;
		default:
			usage();
			break;
		}
	}

	if (optind >= argc)
		usage();

	strcpy(remote_host, argv[optind]);
	top_dir = getenv("AXIS_TOP_DIR");
	kernel_dir = getenv("AXIS_KERNEL_DIR");

	if (top_dir == NULL) {
		printf("AXIS_TOP_DIR is not found in env\n");
		exit(1);
	}

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	get_applications();

	fprintf(stderr,
		"Collecting samples. Press ENTER to print statistics.\n");

	while (1) {
		char command[MAX_STRING_LEN];
		FILE *f;
		int ret;

		sprintf(command,
			"wget --proxy=off -O - "
			"ftp://root:pass@%s/proc/system_profile 2>/dev/null",
			remote_host);

		if (!(f = popen(command, "r"))) {
			printf("Couldn't get any more samples\n");
			print_and_exit(EXIT_FAILURE);
		}

		while (!feof(f)) {
			unsigned int pid;
			unsigned int sample;

			fread(&pid, sizeof(unsigned int), 1, f);
			fread(&sample, sizeof(unsigned int), 1, f);
			if (sample)
				add_sample(pid, sample);
		}

		ret = pclose(f);
		if (ret < 0 || !WIFEXITED(ret) || WEXITSTATUS(ret)) {
			printf("Couldn't get any more samples\n");
			print_and_exit(EXIT_FAILURE);
		}
		wait_sample_interval();
	}
}

/****************** END OF FILE axis_profile.c ******************************/
