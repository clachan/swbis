/* vplob.c: void pointer list object 

  Copyright (C) 2006  James H. Lowe, Jr.  <jhlowe@acm.org>
 
  COPYING TERMS AND CONDITIONS
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include "vplob.h"

VPLOB *
vplob_open(void)
{
	return (VPLOB*)(cplob_open(3));
}

void
vplob_close(VPLOB * vplob)
{
	CPLOB * cplob = (CPLOB*)vplob;
	free(cplob->list);
	free(cplob);
}

void
vplob_shallow_close(VPLOB * vplob)
{
	CPLOB * cplob = (CPLOB*)vplob;
	free(cplob);
}

void
vplob_add(VPLOB * vplob, void * addr)
{
	CPLOB * cplob = (CPLOB*)vplob;
	cplob_add_nta(cplob, addr);
}

void **
vplob_get_list(VPLOB * vplob)
{
	CPLOB * cplob = (CPLOB*)vplob;
	return (void**)(cplob->list);
}

void *
vplob_val(VPLOB * vplob, int index)
{
	CPLOB * cplob = (CPLOB*)vplob;
	return (void*) cplob_val(cplob, index);	
}

int
vplob_get_nstore(VPLOB * vplob)
{
	CPLOB * cplob = (CPLOB*)vplob;
	int i;
	int ret = 0;
	for (i = 0; i < cplob->nused; i++) {
		if (*(cplob->list + (i)) != (char*)NULL) {
			ret++;
		}
	}
	return ret;
}

int
vplob_delete_store(VPLOB * vplob, void (*f_delete)(void*))
{
	CPLOB * cplob = (CPLOB*)vplob;
	void * addr;
	int i;
	int n = vplob_get_nstore(vplob);
	for (i = 0; i < n; i++) {
		addr = *(cplob->list + (i));
		if (addr != (void*)NULL) {
			(*f_delete)(addr);
		}
	}
	return 0;
}

int
vplob_remove_from_list(VPLOB * vplob, void * addr_to_remove)
{
	CPLOB * cplob = (CPLOB*)vplob;
	int i;
	int n;
	void * addr;

	n = vplob_get_nstore(vplob);
	for (i = 0; i < n; i++) {
		addr = *(cplob->list + (i));
		if (addr == addr_to_remove) {
			cplob_remove_index(cplob, i);
		}
	}
	return 0;
}

