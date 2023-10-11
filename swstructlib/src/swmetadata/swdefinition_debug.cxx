/* swdefinition_debug.cxx
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

char * swDefinition::swdefinition_dump_string_s(char * prefix)
{
	swDefinition * pf = this;
	int uxfiofd;
	char * buffer;
	int sizeofprebuf = 2000;
	char * prebuf = (char*)malloc(sizeofprebuf);
	STROB * ptrbuf = strob_open(100);
	STROB * buf = pf->sbufM;
	int len;

	if (!buf) {
		pf->sbufM = strob_open(100);
		buf = pf->sbufM;
	}

	strob_sprintf(buf, 0, "%s%p (swDefinition*)\n", prefix,  (void*)pf);
	strob_sprintf(buf, 0, "%s%p->keyword            = [%s]\n", prefix, (void*)pf, pf->get_keyword());
	strob_sprintf(buf, 1, "%s%p->nextM              = [%p]\n",  prefix, (void*)pf, (void*)(pf->nextM));
	strob_sprintf(buf, 1, "%s%p->prevM              = [%p]\n",  prefix, (void*)pf, (void*)(pf->prevM));
	
	
	uxfiofd=uxfio_open("/dev/null", O_RDONLY, 0 );
	if (uxfio_fcntl(uxfiofd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON)) {
		fprintf (stderr,"error in %s:%d\n", __FILE__, __LINE__);
	}
	if (uxfio_fcntl(uxfiofd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM)) {
		fprintf (stderr,"error in %s:%d\n", __FILE__, __LINE__);
	}
	strob_sprintf(buf, 1, "%s%p  <Definition BEGIN>\n",  prefix, (void*)pf);
	
	snprintf(prebuf, sizeofprebuf-1, "%s%p->%p ", prefix, (void*)pf, (void*)(pf->get_mem_addr()));
	pf->write_fd_debug(uxfiofd, prebuf);

	uxfio_get_dynamic_buffer(uxfiofd, &buffer, NULL, &len);

	buffer[len] = '\0';
	strob_strcat(buf, buffer);

	strob_sprintf(buf, 1, "\n%s%p  <Definition END>\n",  prefix, (void*)pf);
	uxfio_close(uxfiofd);
	strob_close(ptrbuf);
	free(prebuf);
	return strob_str(buf);
}
