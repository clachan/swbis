/* swattributes.c -Attribute mappings to other format's attributes
 */

/*
   Copyright (C) 2005  James H. Lowe, Jr.
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS
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
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swattributes.h"

static
struct swattributes_map *
get_table(int table_id)
{
	if (table_id == SWATMAP_ATT_Header) {
		return (struct swattributes_map *)swdef_pkg_maptable;
	} else {
		return (struct swattributes_map *)swdef_sig_maptable;
	}
}

static
struct swattributes_map *
find_entry_by_rpmtag(int table_id, int rpmtag)
{
	struct swattributes_map * entry;
	struct swattributes_map * table;
	
	table = get_table(table_id);

	entry = table;
	while(entry->rpmtag_number) {
		if (entry->rpmtag_number == rpmtag) {
			return entry;
		}
		entry++;	
	}
	return NULL;
}

int
swatt_get_rpmtype(int table_id, int rpmtag)
{
	struct swattributes_map * entry;
	entry = find_entry_by_rpmtag(table_id, rpmtag);
	if (entry)
		return entry->count;
	else
		return -1;
}

int
swatt_get_rpmcount(int table_id, int rpmtag)
{
	struct swattributes_map * entry;
	entry = find_entry_by_rpmtag(table_id, rpmtag);
	if (entry)
		return entry->rpmtag_type;
	else
		return -1;
}

