/* swptrlist_debug.cxx -
 */ 

   Not Used ???


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

extern "C" {
#include "taru.h"
#include "swlib.h"
#include "swheaderline.h"
#include "md5.h"
#include "ahs.h"
}


template<class T> char * swPtrList<T>::swptrlist_dump_string_s(char * prefix, void* xxswptrlistbuf)
{
	STROB * swptrlistbuf = static_cast<STROB*>(xxswptrlistbuf);
	void * x;
	int i;
	swPtrList * pf = this;


	strob_sprintf(swptrlistbuf, 0, "%s%p (swPtrList*)\n", prefix,  (void*)pf);
	strob_sprintf(swptrlistbuf, 1, "%s%p->listlen_           = [%d]\n",  prefix, (void*)pf, pf->listlen_);
	strob_sprintf(swptrlistbuf, 1, "%s%p->reslen_            = [%d]\n",  prefix, (void*)pf, pf->reslen_);
	strob_sprintf(swptrlistbuf, 1, "%s%p->list_              = [%p]\n",  prefix, (void*)pf, (void*)(pf->list_));

	i = 0;
	x = static_cast<void*>(pf->get_pointer_from_index(i));
	while (x) {
		strob_sprintf(swptrlistbuf, 1, "%s%p->list_[%d]           = [%p]\n",  prefix, (void*)pf, i, x);
		x = static_cast<void*>(pf->get_pointer_from_index(++i));
	}
	
	return strob_str(swptrlistbuf);
}
