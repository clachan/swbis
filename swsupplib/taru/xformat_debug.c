/*  xformat_debug.c:
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
static STROB * buf1 = NULL;
	
/* ------------------------------------ 	
	int ifdM; 
	int ofdM; 
	enum archive_format format_codeM;

	int eoaM;
	int bytes_writtenM;
	int make_false_inodesM;
	SWVARFS * swvarfsM;
	AHS * ahsM;
	HLLIST *link_recordM;
	DEFER * deferM; 
	PORINODE * use_false_inodesM;
	PORINODE * porinodeM;
	int swvarfs_is_externalM;
	int last_header_sizeM;



------------------------------------------*/


char *
xformat_dump_string_s(XFORMAT * uin, char * prefix)
{
	char prebuf[300];

	if (!buf) buf = strob_open(100);
	if (!prefix) prefix = "";

	strob_sprintf(buf, 0, "%s%p (XFORMAT*)\n", prefix, uin);
	strob_sprintf(buf, 1, "%s%p->ifdM                 = [%d]\n", prefix, (void*)(uin), uin->ifdM);
	strob_sprintf(buf, 1, "%s%p->ofdM                 = [%d]\n", prefix, (void*)(uin), uin->ofdM);
	strob_sprintf(buf, 1, "%s%p->format_codeM         = [%d]\n", prefix, (void*)(uin), uin->format_codeM);
	strob_sprintf(buf, 1, "%s%p->output_format_codeM  = [%d]\n", prefix, (void*)(uin), uin->output_format_codeM);
	strob_sprintf(buf, 1, "%s%p->eoaM                 = [%d]\n", prefix, (void*)(uin), uin->eoaM);
	strob_sprintf(buf, 1, "%s%p->bytes_writtenM       = [%d]\n", prefix, (void*)(uin), uin->bytes_writtenM);
	strob_sprintf(buf, 1, "%s%p->make_false_inodeM    = [%d]\n", prefix, (void*)(uin), uin->make_false_inodesM);
	strob_sprintf(buf, 1, "%s%p->is_externalM         = [%d]\n", prefix, (void*)(uin), uin->swvarfs_is_externalM);
	strob_sprintf(buf, 1, "%s%p->last_header_sizeM    = [%d]\n", prefix, (void*)(uin), uin->last_header_sizeM);
	strob_sprintf(buf, 1, "%s%p->link_recordM         = [%p]\n", prefix, (void*)(uin), uin->link_recordM);
	strob_sprintf(buf, 1, "%s%p->deferM               = [%p]\n", prefix, (void*)(uin), uin->deferM);
	strob_sprintf(buf, 1, "%s%p->porinodeM            = [%p]\n", prefix, (void*)(uin), uin->porinodeM);
	strob_sprintf(buf, 1, "%s%p->use_false_inodesM    = [%p]\n", prefix, (void*)(uin), uin->use_false_inodesM);
	strob_sprintf(buf, 1, "%s%p->trailer_bytesM       = [%d]\n", prefix, (void*)(uin), uin->trailer_bytesM);
	strob_sprintf(buf, 1, "%s%p->swvarfs_is_externalM = [%d]\n", prefix, (void*)(uin), uin->swvarfs_is_externalM);
	strob_sprintf(buf, 1, "%s%p->last_header_sizeM    = [%d]\n", prefix, (void*)(uin), uin->last_header_sizeM);
	/* strob_sprintf(buf, 1, "%s%p->do_strip_leading_slashM = [%d]\n", prefix, (void*)(uin), uin->do_strip_leading_slashM); */
	/* strob_sprintf(buf, 1, "%s%p->taru_tarheaderflagsM    = [%d]\n", prefix, (void*)(uin), uin->taru_tarheaderflagsM); */
	strob_sprintf(buf, 1, "%s%p->swvarfsM             = [%p]\n", prefix, (void*)(uin), uin->swvarfsM);

	if (uin->swvarfsM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->swvarfsM ", prefix, (void*)(uin));
		strob_sprintf(buf, 1, "%s", swvarfs_dump_string_s(uin->swvarfsM, prebuf));
	} 

	strob_sprintf(buf, 1, "%s%p->ahsM              = [%p]\n", prefix, (void*)(uin), uin->ahsM);
	
	if (uin->ahsM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->ahsM ", prefix, (void*)(uin));
		strob_sprintf(buf, 1, "%s", ahs_dump_string_s(uin->ahsM, prebuf));
	}

	return strob_str(buf);
}
	
void xformat_debug_dump(XFORMAT * xux, FILE * fp) {
	XFORMAT_E_DEBUG("");
	ahs_debug_dump(xux->ahsM, fp);
}


