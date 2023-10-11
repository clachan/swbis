/* swobjfiles_i_debug.cxx
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
#include "swdefinitionfile.h"
#include "swpackagefile.h"
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

char * swObjFiles_i::swptrlist_dump_string_s(char * prebuf, STROB * ptrbuf) {
	return containsM->swPtrList<swExStruct>::swptrlist_dump_string_s(prebuf, ptrbuf);
}

char * swObjFiles_i::dump_string_s(char * prefix)
{
	int i;
	swObjFiles_i * pf = this;
	STROB * buf;	
	swPackageFile * pkgfile;
	char prebuf[1000];
	
	//if (buf == (STROB*)NULL) buf = strob_open(100);

	buf = pf->sbufM;

	strob_sprintf(buf, 0, "%s%p <swObjFiles_i*> Object Keyword = %s\n", prefix,  (void*)pf, pf->getObjectName());
	strob_sprintf(buf, 1, "%s%p->psfM                  = [%p]\n",  prefix, (void*)pf, (void*)(pf->getPSF()));
	strob_sprintf(buf, 1, "%s%p->swheader_offsetM      = [%d]\n",  prefix, (void*)pf, pf->get_swheader_offset());

	strob_sprintf(buf, 1, "%s%p <swObjFiles_i*>\n", prefix,  (void*)pf);
	strob_sprintf(buf, 1, "%s%p->swExStruct_i           = [%p]\n",  prefix, (void*)pf, (void*)(static_cast<swExStruct_i*>(pf)));
	snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(static_cast<swExStruct_i*>(pf)));
	strob_sprintf(buf, 1, "%s", pf->swExStruct_i::swexstruct_i_dump_string_s(prebuf));

	strob_sprintf(buf, 1, "%s%p->infoM                 = [%p]\n",  prefix, (void*)pf, (void*)(pf->getInfo()));
	if (pf->getInfo()) {
		snprintf(prebuf, sizeof(prebuf) - 1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf->getInfo()));
		strob_sprintf(buf, 1, "%s", pf->getInfo()->swDefinitionFile::swdefinitionfile_dump_string_s(prebuf));
	}

	strob_sprintf(buf, 1, "%s%p->indexM                = [%p]\n",  prefix, (void*)pf, (void*)(pf->getIndex()));
	if (pf->getIndex()) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pf->getIndex()));
		strob_sprintf(buf, 1, "%s", pf->getIndex()->swDefinitionFile::swdefinitionfile_dump_string_s(prebuf));
	}

	strob_sprintf(buf, 1, "%s%p->controlFilesM         = [%p]\n",  prefix, (void*)pf, (void*)(pf->controlFilesM));
	i = 0;
	pkgfile = pf->controlFilesM->get_pointer_from_index(i++);
	while(pkgfile != NULL) {
		strob_sprintf(buf, 1, "%s%p->controlFilesM[%d]          = [%p]\n",  prefix, (void*)pf, i, (void*)(pkgfile));
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pkgfile));
		swpackagefile_dump_string_s(pkgfile, prebuf);	
		pkgfile = pf->controlFilesM->get_pointer_from_index(i++);
	}

	strob_sprintf(buf, 1, "%s%p->attributeFilesM       = [%p]\n",  prefix, (void*)pf, (void*)(pf->attributeFilesM));
	i = 0;
	pkgfile = pf->attributeFilesM->get_pointer_from_index(i++);
	while(pkgfile != NULL) {
		strob_sprintf(buf, 1, "%s%p->attributeFilesM[%d]        = [%p]\n",  prefix, (void*)pf, i, (void*)(pkgfile));
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(pf), (void*)(pkgfile));
		swpackagefile_dump_string_s(pkgfile, prebuf);	
		pkgfile = pf->attributeFilesM->get_pointer_from_index(i++);
	}

	return strob_str(buf);
}
