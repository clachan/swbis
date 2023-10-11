/* swexcat_debug.cxx
 *
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

#include "swdefinition.h"
#include "swdefinitionfile.h"
#include "swexcat.h"
#include "swptrlist.h"

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

char * swExCat::dump_string_s(char * prefix)
{
	int i;
	swExCat * pf = this;
	STROB * buf = sbufM;
	STROB * ptrbuf = strob_open(100);
	swExStruct * swex;
	char prebuf[1000];
	

	//strob_sprintf(buf, 0, "%s%p <swExCat*>\n", prefix,  (void*)pf);
	//strob_sprintf(buf, 1, "%s%p->swExStruct_i           = [%p]\n",  prefix, (void*)pf, (void*)(static_cast<swExStruct_i*>(pf)));
	//snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(static_cast<swExStruct_i*>(pf)));
	//strob_sprintf(buf, 1, "%s", pf->swExStruct_i::swexstruct_i_dump_string_s(prebuf));

	strob_sprintf(buf, 0, "%s%p <swExCat*> Object Keyword = %s\n", prefix,  (void*)pf, pf->getObjectName());
	strob_sprintf(buf, 1, "%s%p->swObjFiles_i           = [%p]\n",  prefix, (void*)pf, (void*)(static_cast<swObjFiles_i*>(pf)));
	snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(static_cast<swObjFiles_i*>(pf)));
	strob_sprintf(buf, 1, "%s", pf->swObjFiles_i::dump_string_s(prebuf));


	strob_sprintf(buf, 1, "%s%p->swPtrList             = [%p]\n",  prefix, (void*)pf, (void*)(pf));
	snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf));
	strob_sprintf(buf, 1, "%s", pf->swptrlist_dump_string_s(prebuf, ptrbuf));

	i = 0;
	swex = pf->containedByIndex(i++);
	while(swex != NULL) {
		strob_sprintf(buf, 1, "%s%p->swPtrList<swExStruct>[%d]          = [%p]\n",  prefix, (void*)pf, i-1, (void*)(swex));
		snprintf(prebuf, sizeof(prebuf) - 1, "%s%p->%p ", prefix, (void*)(pf), (void*)(swex));
		strob_strcat(buf, swex->dump_string_s(prebuf));
		//fprintf(stderr, "%s", swex->dump_string_s(prebuf));
		swex = pf->containedByIndex(i++);
	}

	strob_close(ptrbuf);
	return strob_str(buf);
}
