/* vrealpath.c - Virtual Real path resolution.
 */

/*
 * Copyright (C) 2003  James H. Lowe, Jr.
   All rights reserved.
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "taru.h"
#include "uxfio.h"
#include "strob.h"
#include "shcmd.h"
#include "swpath.h"
#include "swlib.h"
#include "ugetopt_help.h"


int
swlib_vrealpath(char * vpwd, char * ppath, int * depth, STROB * resolved_path)
{
	STROB * tmp;
	STROB * bpath;
	int count = 0;
	int numcomponents = 0;
	char * path;
	char * s;
	int is_absolute_path = 0;
	int compcount = 0;
	int do_add = 0;

	tmp = strob_open(10);
	bpath = strob_open(10);

	if (vpwd && strlen(vpwd)) {
		strob_strcpy(bpath, vpwd);
		if (vpwd[strlen(vpwd) - 1] != '/') {
			strob_strcat(bpath, "/");
		}
	}
	strob_strcat(bpath, ppath);

	path = strob_str(bpath);

	swlib_slashclean(path);

	if (resolved_path) strob_strcpy(resolved_path, "");

	if (vpwd && strlen(vpwd)) {
		is_absolute_path = (*vpwd == '/');
	} else {
		is_absolute_path = (*ppath == '/');
	}

	s = strob_strtok(tmp, path, "/");
	while (s) {
		do_add = 0;
		if (strcmp(s, "..") == 0) {
			do_add = -1;
			count--;
		} else if (strcmp(s, ".") == 0) {
			/* do nothing */
		} else {
			do_add = 1;
			count++;
		}
		
		if (resolved_path) {
			if (do_add > 0) {
				if ((is_absolute_path && !compcount) || compcount) {
					if ( strob_strlen(resolved_path) &&
						*(strob_str(resolved_path) + strob_strlen(resolved_path) - 1) != '/')
						strob_strcat(resolved_path, "/");
				}
				strob_strcat(resolved_path, s);
				compcount++;
			} else if (do_add < 0) {
				char * ls;
				ls = strrchr(strob_str(resolved_path), '/');
				if (ls && ls != strob_str(resolved_path)) {
					*ls = '\0';
				} else if (is_absolute_path && count <= 0) {
					strob_strcpy(resolved_path, "/");
				} else if (is_absolute_path  == 0 && count <= 0) {
					strob_strcpy(resolved_path, "");
				}
			} else {
				;
			}
			if (is_absolute_path == 0) {
				swlib_squash_leading_slash(strob_str(resolved_path));
			}
		}
	
		numcomponents ++;
		s = strob_strtok(tmp, NULL, "/");
	}

	if (depth) *depth = count;
	strob_close(tmp);
	strob_close(bpath);
	return numcomponents;
}
