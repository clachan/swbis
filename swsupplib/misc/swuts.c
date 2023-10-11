/* swuts.h -  The swbis system identification object.

   Copyright (C) 2006,2007 Jim Lowe
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS:
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swheader.h"
#include "swheaderline.h"
#include "atomicio.h"
#include "swevents.h"
#include "swuts.h"
#include "fnmatch_u.h"

static
void
swuts_store_value(SWUTS * uts, char ** dest, char * source)
{
	if (*dest) free(*dest);
	*dest = strdup(source);
}

static
int
compare_name(char * target, char * sw, int verbose)
{
	int ret;
	if (sw == NULL || strlen(sw) == 0) return 0;
	if (target == NULL || strlen(target) == 0) return 0;
	ret = fnmatch(sw, target, 0);	
	return ret;
}

SWUTS *
swuts_create(void)
{
	SWUTS * uts;
	uts = (SWUTS *) malloc (sizeof(SWUTS));
	uts->machineM = NULL;
        uts->sysnameM = NULL;
        uts->releaseM = NULL;
        uts->versionM = NULL;
        uts->arch_tripletM = NULL;
	uts->result_machineM = 0;
        uts->result_sysnameM = 0;
        uts->result_releaseM = 0;
        uts->result_versionM = 0;
        uts->match_resultM = -1;
	return uts;
}

void
swuts_delete(SWUTS * uts)
{
	if (uts->machineM) free(uts->machineM);
        if (uts->sysnameM) free(uts->sysnameM); 
        if (uts->releaseM) free(uts->releaseM);
        if (uts->versionM) free(uts->versionM);
        if (uts->arch_tripletM) free(uts->arch_tripletM);
	free(uts); 
}

int
swuts_read_from_events(SWUTS * uts, char * events)
{
	char * line;
	char * value;
	char * attribute;
	int ret = 0;
	STROB * tmp;
	
	/*
	The line to parse looks something like this

	swinstall: swicol: 315:machine_type=i686
	swinstall: swicol: 315:os_name=Linux
	swinstall: swicol: 315:os_release=2.4.18
	swinstall: swicol: 315:os_version=#1 Tue Dec 30 20:43:14 EST 2003
	swinstall: swicol: 315:architecture=686-pc-linux-gnu
	*/

	tmp = strob_open(10);
	line = strob_strtok(tmp, events, "\r\n");
	while (line) {
		ret = swevent_parse_attribute_event(line, &attribute, &value);
		if (ret == 0) {
			if (strcmp(attribute, SW_A_os_name) == 0) {
				uts->sysnameM = strdup(value);
			} else if (strcmp(attribute, SW_A_os_version) == 0) {
				uts->versionM = strdup(value);
			} else if (strcmp(attribute, SW_A_os_release) == 0) {
				uts->releaseM = strdup(value);
			} else if (strcmp(attribute, SW_A_machine_type) == 0) {
				uts->machineM = strdup(value);
			} else if (strcmp(attribute, SW_A_architecture) == 0) {
				uts->arch_tripletM = strdup(value);
			} else {
				fprintf(stderr, "bad message in swi_uts_read_from_events: %s\n", attribute);
				ret = -1;
			}
		}
		line = strob_strtok(tmp, NULL, "\r\n");
	}
	strob_close(tmp);
	return ret;
}

int
swuts_read_from_swdef(SWUTS * uts)
{
	return 0;
}

int
swuts_compare(SWUTS * uts_target, SWUTS * uts_swdef, int verbose)
{
	int ret;
	/* uts_target values are actual, they have no patterns
           uts_swdef values may be patterns */
	
	ret = compare_name(uts_target->sysnameM, uts_swdef->sysnameM, verbose);
        uts_swdef->result_sysnameM = ret;
	ret = compare_name(uts_target->machineM, uts_swdef->machineM, verbose);
	uts_swdef->result_machineM = ret;
	ret = compare_name(uts_target->releaseM, uts_swdef->releaseM, verbose);
        uts_swdef->result_releaseM = ret;
	ret = compare_name(uts_target->versionM, uts_swdef->versionM, verbose);
        uts_swdef->result_versionM = ret;

	if (
		uts_swdef->result_sysnameM == 0 &&
		uts_swdef->result_machineM == 0 &&
		uts_swdef->result_releaseM == 0 &&
		uts_swdef->result_versionM == 0 &&
		1
	) {
		uts_swdef->match_resultM = 0;
		return 0;
	} else {
		uts_swdef->match_resultM = 1;
		return 1;
	}
}

void
swuts_add_attribute(SWUTS * uts, char * attribute, char * value)
{
	if (strcmp(attribute, SW_A_os_name) == 0) {
		swuts_store_value(uts, &(uts->sysnameM), value);
	} else if (strcmp(attribute, SW_A_os_version) == 0) {
		swuts_store_value(uts, &(uts->versionM), value);
	} else if (strcmp(attribute, SW_A_os_release) == 0) {
		swuts_store_value(uts, &(uts->releaseM), value);
	} else if (strcmp(attribute, SW_A_machine_type) == 0) {
		swuts_store_value(uts, &(uts->machineM), value);
	} else if (strcmp(attribute, SW_A_architecture) == 0) {
		swuts_store_value(uts, &(uts->arch_tripletM), value);
	} 
}

int
swuts_is_uts_attribute(char * name)
{
	if (
		strcmp(name, SW_A_os_name) &&
		strcmp(name, SW_A_os_release) &&
		strcmp(name, SW_A_os_version) &&
		strcmp(name, SW_A_machine_type) &&
		strcmp(name, SW_A_architecture) 
	)
		return 0;
	else
		return 1;
}
