/*  xformat_debug2.c:
 */

/*
 * Copyright (C) 2002  James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "swlib.h"
#include "ahs.h"

static STROB * buf = NULL;
	
char *
unix_stat_dump_string_s(struct stat * uin, char * prefix)
{
	char prebuf[300];

	if (!buf) buf = strob_open(100);
	if (!prefix) prefix = "";

	strob_sprintf(buf, 0, "%s%p (struct stat*)\n", prefix, (void*)uin);
	strob_sprintf(buf, 1, "%s%p->st_dev               = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_dev));
	strob_sprintf(buf, 1, "%s%p->st_ino               = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_ino));
	strob_sprintf(buf, 1, "%s%p->st_mode              = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_mode));
	strob_sprintf(buf, 1, "%s%p->st_nlink             = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_nlink));
	strob_sprintf(buf, 1, "%s%p->st_uid               = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_uid));
	strob_sprintf(buf, 1, "%s%p->st_gid               = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_gid));
	strob_sprintf(buf, 1, "%s%p->st_rdev              = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_rdev));
	strob_sprintf(buf, 1, "%s%p->st_size              = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_size));
	strob_sprintf(buf, 1, "%s%p->st_blksize           = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_blksize));
	strob_sprintf(buf, 1, "%s%p->st_blocks            = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_blocks));
	strob_sprintf(buf, 1, "%s%p->st_atime             = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_atime));
	strob_sprintf(buf, 1, "%s%p->st_mtime             = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_mtime));
	strob_sprintf(buf, 1, "%s%p->st_ctime             = [%d]\n", prefix, (void*)(uin), 	(int)(uin->st_ctime));

	return strob_str(buf);
}
