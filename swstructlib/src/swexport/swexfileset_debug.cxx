/* swexfileset_debug.cxx -
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

#include "swobjfiles_i.h"
#include "swexfileset.h"
#include "swdefinitionfile.h"
#include "swpackagefile.h"

extern "C" {
#include "uxfio.h"
#include "strob.h"
#include "cplob.h"
#include "taru.h"
#include "swlib.h"
#include "swheaderline.h"
#include "md5.h"
#include "ahs.h"
}

//static STROB * buf = NULL;



char * swExFileset::dump_string_s(char * prefix)
{
	swExFileset * pf = this;
	STROB * buf = sbufM;
	char prebuf[1000];

	//if (buf == (STROB*)NULL) buf = strob_open(100);

	strob_sprintf(buf, 0, "%s%p <swExFileset*>\n", prefix,  (void*)pf);
	strob_sprintf(buf, 1, "%s%p->swObjFiles_i           = [%p]\n",  prefix, (void*)pf, (void*)(static_cast<swObjFiles_i*>(pf)));
	snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(static_cast<swObjFiles_i*>(pf)));
	strob_sprintf(buf, 1, "%s", pf->swObjFiles_i::dump_string_s(prebuf));

	return strob_str(buf);
}

