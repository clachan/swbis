/*  ahs_debug.c:
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

/*************************
typedef struct {
	char userM[64];
	char groupM[64];
	struct new_cpio_header * file_hdrM;
	char *c_nameM;    
	char *c_linknameM;    
} AHS;
**************************/

unsigned long
ahs_debug_dump(AHS * xhs, FILE *fp)
{
	unsigned long name_sum;
	char * name;
	char * linkname;

	AHS_E_DEBUG("");
	name = ahsStaticGetTarFilename(ahs_vfile_hdr(xhs));
	linkname = ahsStaticGetTarLinkname(ahs_vfile_hdr(xhs));
	if (!name) name = "<null>";
	if (!linkname) linkname = "<null>";

	name_sum = swlib_bsd_sum_from_mem(name, strlen(name));
	taru_header_dump(ahs_vfile_hdr(xhs), fp);
	/*
	fprintf(fp,"%05lu c_nameM = [%s]\n", name_sum, name);
	fprintf(fp,"%05lu c_linknameM = [%s]\n", name_sum, linkname);
	*/
	return name_sum;
}


char *
ahs_dump_string_s(AHS * uin, char * prefix)
{
	char prebuf[300];

	if (!buf) buf = strob_open(100);
	if (!prefix) prefix = "";

	strob_sprintf(buf, 0, "%s%p (AHS*)\n", prefix, uin);
	/* strob_sprintf(buf, 1, "%s%p->userM         = [%s]\n", prefix, (void*)(uin), uin->userM); */
	/* strob_sprintf(buf, 1, "%s%p->groupM        = [%s]\n", prefix, (void*)(uin), uin->groupM); */
	strob_sprintf(buf, 1, "%s%p->file_hdrM       = [%p]\n", prefix, (void*)(uin), uin->file_hdrM);
	if (uin->file_hdrM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->file_hdrM ", prefix, (void*)(uin));
		strob_sprintf(buf, 1, "%s", taru_header_dump_string_s(uin->file_hdrM, prebuf));
	}

	/* strob_sprintf(buf, 1, "%s%p->c_nameM       = [%s]\n", prefix, (void*)(uin), uin->c_nameM); */
	/* strob_sprintf(buf, 1, "%s%p->c_linknameM   = [%s]\n", prefix, (void*)(uin), uin->c_linknameM); */

	return strob_str(buf);
}

