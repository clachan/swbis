/* swdefinitionfile_debug.cxx
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
#include "swptrlist.h"
// #include "swptrlist_debug.cxx"
#include "swdefinitionfile.h"

extern "C" {
#include "taru.h"
#include "swlib.h"
#include "swheaderline.h"
#include "md5.h"
#include "ahs.h"
}

//static STROB * buf = NULL;
	
/*	
	int parserOutputLenM;
	swparser * parserM;
       	swPtrList<swDefinition> * swdefinition_listM;  
	int did_setM;
	static swFileMap * swfilemapM;
	
	swDefinition * currentM;	// Used by the iterator.
	swDefinition * headM;
	swDefinition * tailM;
*/

char * swDefinitionFile::swdefinitionfile_dump_string_s(char * prefix)
{
	swDefinitionFile * pf = this;
	int uxfiofd;
	char * buffer;
	int sizeofprebuf = 2000;
	char * prebuf = (char*)malloc(sizeofprebuf);
	STROB * ptrbuf = strob_open(100);
	STROB * buf = pf->sbufM;
	int len;

	//if (buf == (STROB*)NULL) buf = strob_open(100);

	strob_sprintf(buf, 0, "%s%p (swDefinitionFile*)\n", prefix,  (void*)pf);
	strob_sprintf(buf, 1, "%s%p->parserOutputLenM   = [%d]\n",  prefix, (void*)pf, pf->parserOutputLenM);
	strob_sprintf(buf, 1, "%s%p->parserM            = [%p]\n",  prefix, (void*)pf, (void*)(pf->parserM));
	strob_sprintf(buf, 1, "%s%p->swdefinition_listM = [%p]\n",  prefix, (void*)pf, (void*)(pf->swdefinition_listM));
	strob_sprintf(buf, 1, "%s%p->did_setM           = [%d]\n",  prefix, (void*)pf, pf->did_setM);
	strob_sprintf(buf, 1, "%s%p->currentM           = [%p]\n",  prefix, (void*)pf, (void*)(pf->currentM));
	strob_sprintf(buf, 1, "%s%p->headM              = [%p]\n",  prefix, (void*)pf, (void*)(pf->headM));
	strob_sprintf(buf, 1, "%s%p->tailM              = [%p]\n",  prefix, (void*)pf, (void*)(pf->tailM));
	
	
	strob_sprintf(buf, 1, "%s%p->%p\n",  prefix, (void*)pf, (void*)(pf->swdefinition_listM));
	if ((void*)(pf->swdefinition_listM)) {
		snprintf(prebuf, sizeofprebuf-1, "%s%p->%p ", prefix, (void*)pf, (void*)(pf->swdefinition_listM));
		strob_sprintf(buf, 1,"%s", pf->swdefinition_listM->swPtrList<swDefinition>::swptrlist_dump_string_s(prebuf, (void*)ptrbuf));
	}

	uxfiofd=uxfio_open("/dev/null", O_RDONLY, 0 );
	if (uxfio_fcntl(uxfiofd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON)) {
		fprintf (stderr,"error in %s:%d\n", __FILE__, __LINE__);
	}
	if (uxfio_fcntl(uxfiofd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM)) {
		fprintf (stderr,"error in %s:%d\n", __FILE__, __LINE__);
	}
	strob_sprintf(buf, 1, "%s%p  <<<Definition Linked List BEGIN>>>\n",  prefix, (void*)pf);
	
	snprintf(prebuf, sizeofprebuf-1, "%s%p->%p ", prefix, (void*)pf, (void*)(pf->swdefinition_listM));
	pf->swdeffile_linki_write_fd_debug(uxfiofd, prebuf);

	uxfio_get_dynamic_buffer(uxfiofd, &buffer, NULL, &len);

	buffer[len] = '\0';
	strob_strcat(buf, buffer);

	strob_sprintf(buf, 1, "\n%s%p  <<<Definition Linked List END>>>\n",  prefix, (void*)pf);
	uxfio_close(uxfiofd);
	strob_close(ptrbuf);
	free(prebuf);
	return strob_str(buf);
}
