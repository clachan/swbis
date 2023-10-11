/* strar.c:  Stores a null terminated string in a list.
 */

/* 
   Copyright (C) 2002,2008  James H. Lowe, Jr.
   All rights reserved.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "swuser_config.h"
#include "stdlib.h"
#include "swlib.h"
#include "strar.h"

#define swbis_free free

static
int
f_strcmp(int sense, const void * vf1, const void * vf2)
{
        char * f1;
        char * f2;
        f1 = *((char**)(vf1));
        f2 = *((char**)(vf2));
        if (sense < 0 ) return -strcmp(f1, f2);
        return strcmp(f1, f2);
}

STRAR *
strar_copy_construct(STRAR * src)
{
	int i;
	char * s;
	STRAR * ret;
	if (!src) return NULL;
	ret = strar_open();
	if (!ret) return NULL;
	i = 0;	
	while((s=strar_get(src, i++))) {
		strar_add(ret, s);
	}
	return ret;
}

STRAR *
strar_open(void)
{
	STRAR * strb;

	strb = (STRAR *)malloc(sizeof(STRAR));
	if (strb == (STRAR *)(NULL))
		return (STRAR *)(NULL);

	strb->lenM = 0;
	strb->nsM = 0;
	strb->storageM = strob_open(132);
	strb->listM = cplob_open(10);
	cplob_add_nta(strb->listM, NULL);
	return strb;
}

void
strar_reset(STRAR * strar)
{
	strar->lenM = 0;
	strar->nsM = 0;
	strob_strcpy(strar->storageM, "");
	cplob_shallow_reset(strar->listM);
}

void
strar_close(STRAR * strar)
{
	strob_close(strar->storageM);
	swbis_free(cplob_release(strar->listM));
	swbis_free(strar);
	return;
}

int
strar_num_elements(STRAR * strar)
{
	return strar->nsM;
}

char *
strar_get(STRAR * strar, int index)
{
	char * s;
	if (index >= strar->nsM || index < 0) return NULL;
	s = cplob_val(strar->listM, index);
	return s;
}

char *
strar_return_store(STRAR * strar, int len)
{
	char * oldbase;
	char * base;
	char * last;
	int nused;

	strar->lenM = strar->lenM + len + 1;

	oldbase = strob_str(strar->storageM);
	strob_set_memlength(strar->storageM, strar->lenM);

	base = strob_str(strar->storageM);
	strar->nsM ++;

	if (base != oldbase) {
		int index = 0;
		char * p;	
		while((p=cplob_val(strar->listM, index))) {
			cplob_additem(strar->listM, index,  
					base + ((int)(p - oldbase)));
			index++;
		}

	}

	nused = cplob_get_nused(strar->listM) - 1;
		/**/
		/* the terminating NULL counts as being in use. */
		/**/
	if (nused > 0) {
		last = cplob_val(strar->listM, nused - 1);
	} else {
		last = NULL;
	}

	if (last) {
			/* step over the last string and null */
		last = last + strlen(last) + 1;
	} else {
		last = base;
	}
	cplob_add_nta(strar->listM, last);
	memset(last, (int)'_', len);
	*(last+len) = '\0';
	return last;
}

int
strar_add(STRAR * strar, char * src)
{
	char * last;
	last = strar_return_store(strar, strlen(src));
	strcpy(last, src);
	return 0;
}

void
strar_qsort(STRAR * strar, int (*f_comp)(const void*, const void*))
{
	size_t nused;
	nused = (size_t)cplob_get_nused(strar->listM) - 1;
	std_quicksort(strar->listM->list, (size_t)nused, sizeof(char*), f_comp);
}

int
strar_qsort_neg_strcmp(const void * vf1, const void * vf2)
{
	return f_strcmp(-1, vf1, vf2);
}

int
strar_qsort_strcmp(const void * vf1, const void * vf2)
{
	return f_strcmp(1, vf1, vf2);
}

void
strar_remove_index(STRAR * strar, int index)
{
	cplob_remove_index(strar->listM, index);
	strar->nsM--;
}
