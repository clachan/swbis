/* swpackagefile_debug.cxx
 */ 

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include "swpackagefile.h"

extern "C" {
#include "taru.h"
#include "swlib.h"
#include "swheaderline.h"
#include "md5.h"
#include "ahs.h"
}

static STROB * buf = NULL;

/*
*  	static uxFormat * uxformatM;	// The archive package.
*	
*	int is_ieee_layoutM;		// Non zero if package is IEEE 1387.2 layout.
*	STROB *  	user_path_strob_buf_;	
*  	swPathName      * swpathM; 
*  	int		fdM;		// source file descriptor.
*/

char * swpackagefile_dump_string_s(swPackageFile  * pf, char * prefix)
{
	char prebuf[300];
	if (buf == (STROB*)NULL) buf = strob_open(100);

	strob_sprintf(buf, 0, "%s%p (swPackageFile*)\n", prefix,  (void*)pf);
	strob_sprintf(buf, 1, "%s%p->is_ieee_layoutM       = [%d]\n",  prefix, (void*)pf, pf->is_ieee_layoutM);
	strob_sprintf(buf, 1, "%s%p->fdM                   = [%d]\n",  prefix, (void*)pf, pf->fdM);
	strob_sprintf(buf, 1, "%s%p->user_path_strob_buf_  = [%s]\n",  prefix, (void*)pf, strob_str(pf->user_path_strob_buf_));
	strob_sprintf(buf, 1, "%s%p->source_filename       = [%s]\n",  prefix, (void*)pf, pf->swfile_get_source_filename());
	strob_sprintf(buf, 1, "%s%p->package_filename      = [%s]\n",  prefix, (void*)pf, pf->swfile_get_package_filename());
	strob_sprintf(buf, 1, "%s%p->uxformatM             = [%p]\n",  prefix, (void*)pf, pf->uxformatM);
	
	if (pf->uxformatM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf->uxformatM));
		strob_sprintf(buf, 1, "%s", pf->uxformatM->uxFormat_dump_string_s(prebuf));
	}

	strob_sprintf(buf, 1, "%s%p->swpathM               = [%p]\n",  prefix, (void*)pf, pf->swpathM);
	if (pf->swpathM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf->swpathM));
		strob_sprintf(buf, 1, "%s", pf->swpathM->swp_dump_string_s(prebuf));
	}

	return strob_str(buf);
}

