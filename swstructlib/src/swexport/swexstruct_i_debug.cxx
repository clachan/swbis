/* swexstruct_i_debug.cxx
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
#include "swexstruct_i.h"

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

char * swExStruct_i::swexstruct_i_dump_string_s(char * prefix)
{
	STROB * buf;
	swExStruct_i * pf = this;
	char prebuf[1000];

	buf = pf->sbufM;

	strob_sprintf(buf, 0, "%s%p <swExStruct_i*> Object Keyword = %s\n", prefix,  (void*)pf, pf->getObjectName());
	strob_sprintf(buf, 1, "%s%p->debugM                = [%d]\n",  prefix, (void*)pf, pf->debugM);
	strob_sprintf(buf, 1, "%s%p->mediaTypeM            = [%d]\n",  prefix, (void*)pf, pf->mediaTypeM);
	strob_sprintf(buf, 1, "%s%p->controlPathM          = [%s]\n",  prefix, (void*)pf, strob_str(pf->controlPathM));
	strob_sprintf(buf, 1, "%s%p->parentPathM           = [%s]\n",  prefix, (void*)pf, strob_str(pf->parentPathM));
	strob_sprintf(buf, 1, "%s%p->controlDirectoryM     = [%s]\n",  prefix, (void*)pf, strob_str(pf->controlDirectoryM));
	strob_sprintf(buf, 1, "%s%p->refererM              = [%p]\n",  prefix, (void*)pf, (void*)(pf->refererM));
	strob_sprintf(buf, 1, "%s%p->lastM                 = [%p]\n",  prefix, (void*)pf, (void*)(pf->lastM));
	
	strob_sprintf(buf, 1, "%s%p->swpathM               = [%p]\n",  prefix, (void*)pf, (void*)(pf->swpathM));
	if (pf->swpathM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf->swpathM));
		strob_sprintf(buf, 1, "%s", pf->swpathM->swp_dump_string_s(prebuf));
	}
	
	if (pf->refererM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf->refererM));
		strob_sprintf(buf, 1, "%s", pf->refererM->swdefinition_dump_string_s(prebuf));
	}

	return strob_str(buf);
}
