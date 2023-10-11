/* hllist: hard link list object.

   Copyright (C) 1999  James H. Lowe, Jr.

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

#ifndef hllist_1999h
#define hllist_1999h
#include "swuser_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cplob.h"

typedef struct {
	char * path_;
	dev_t dev_;
	ino_t ino_;
	dev_t v_dev_;	/* used by porinode facility */
	ino_t v_ino_;	/* used by porinode facility */
} hllist_entry;


typedef struct {
	int disable_find_;
	int disable_add_;
	CPLOB * list_;
} HLLIST;

HLLIST * hllist_open ( void );
void hllist_close(HLLIST* lr);
void  hllist_add_record (HLLIST *, char * path, dev_t dev, ino_t ino);
void  hllist_add_vrecord(HLLIST * hllist, char * path, dev_t dev, ino_t ino, dev_t v_dev, ino_t v_ino);
hllist_entry * hllist_find_file_entry(HLLIST * linkrec, dev_t dev, ino_t ino, int occurance, int * nfound);
void hllist_show_to_file(HLLIST * hllist, FILE * fp);
void hllist_clear_entries_and_disable(HLLIST * hllist);
void hllist_disable_find(HLLIST * hllist);
void hllist_disable_add(HLLIST * hllist);


#endif

