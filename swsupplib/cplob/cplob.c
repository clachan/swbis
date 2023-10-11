/* cplob.c: character pointer list object 
 */

/*
 * Copyright (C) 1997  James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include "cplob.h"

static
void
null_list(CPLOB * lob, int start, int stop)
{
	int i;
	for (i = start; i < stop; i++) {
		*(lob->list + i) = (char*)NULL;
	}
}


/*-------- CPLOB public API routines ------------------------- */

CPLOB *
cplob_open(int nobj)
{
	CPLOB *lob;

	lob = (CPLOB *) malloc(sizeof(CPLOB));
	if (lob == (CPLOB *) (NULL)) {
		return (CPLOB *) (NULL);
	}

	if (nobj <= 0) {
		nobj = CPLOB_NINITLENGTH;
	}

	lob->nlen = nobj;
	if ((lob->list = (char **) malloc(nobj * sizeof(char*))) == NULL) {
		 return (CPLOB *) (NULL);
	}
	null_list(lob, 0, nobj);
	lob->width = sizeof(char*);
	lob->refcountM = 0;
	cplob_shallow_reset(lob);
	return lob;
}


o__inline__
char ** 
cplob_release(CPLOB * lob)
{
	char ** p = lob->list;
	swbis_free(lob); 
	return p;
}

void 
cplob_close(CPLOB * lob)
{
        cplob_freeall (lob);
	free(lob->list);
	free(lob);
}

int 
cplob_shallow_close(CPLOB * lob)
{
	swbis_free(lob);
	return 0;
}

void
cplob_shallow_reset(CPLOB * lob)
{
	int i;
	lob->nused = 0;
	for (i = 0; i < lob->nlen; i++) {
		*(lob->list + (i)) = (char *)(NULL);
	}
}

char **
cplob_get_list(CPLOB * cplob)
{
	return cplob->list;
}

CPLOB * 
cplob_reopen(int new_length, CPLOB * lob)
{
	if (new_length <= 1) {
		new_length = 2;
	}

	if (new_length > lob->nlen) {
		lob->list = (char **)SWBIS_REALLOC((void*)lob->list,
			(size_t)((new_length) * lob->width), lob->nlen);
		if (!lob->list) return (CPLOB *) (NULL);
		/* initialize new memory to NULL pointer values */
		null_list(lob, lob->nlen, new_length);
		lob->nlen = new_length;
	}
	return lob;
}

void
cplob_freeall (CPLOB * lob) 
{
	int i;
	for (i = 0; i < lob->nused; i++) {
		if (*(lob->list + (i)) != NULL) {
			swbis_free(*(lob->list + (i)));
			(*(lob->list + (i))) = NULL;
		}
	}
}

void
cplob_add_nta(CPLOB * lob, char *addr) 
{
	/* add to end of null terminated array */
	if (!lob->nused){
		lob->nused++;
		cplob_reopen(lob->nused + CPLOB_NLENGTHINCR, lob);
		*(lob->list + (lob->nused-1)) = addr;
		if (!addr){
			return;						
		}
	} else {
		cplob_reopen(lob->nused + CPLOB_NLENGTHINCR, lob);
	}
	*(lob->list + (lob->nused-1)) = addr;
	cplob_additem(lob, lob->nused, NULL);
}

void
cplob_add(CPLOB * lob, char *addr) 
{
	cplob_additem(lob, lob->nused, addr);
}

void
cplob_additem(CPLOB * lob, int index, char *addr)
{
	if (index + 1 > lob->nlen) {
		cplob_reopen(index + 1 + CPLOB_NLENGTHINCR, lob);
	}
	
	*(lob->list + index) = addr;

	if (index + 1 > lob->nused) {
		lob->nused = index + 1;
	}
	return;
}

void
cplob_set_nused(CPLOB * lob, int n)
{
	lob->nused=n;
}

int
cplob_get_nused(CPLOB * lob)
{
	return lob->nused;
}

char *
cplob_val(CPLOB * lob, int index)
{
	if (index < 0) {
		return NULL;
	}
	if (index > lob->nused) return NULL;
	return *(lob->list + (index));
}

int
cplob_backfill_and_nullterminate (CPLOB * lob)
{
	int i=0;

	while(i<lob->nused){
		while (i<lob->nused && *(lob->list + i) == NULL) {
			if (i + 1 == lob->nused)
				break;
			memmove (lob->list+i, lob->list+i+1,
				lob->width * (lob->nused - i - 1));
			lob->nused--;
		}	
		i++;
	}
	if (*(lob->list + lob->nused) != NULL) {
		cplob_add(lob, (char *)(NULL));
	}
	return 0;
}

int
cplob_remove_index(CPLOB * lob, int i)
{
	if (i < 0 || i >= lob->nused) return -1;
	if (lob->nused == 0 || lob->nlen == 0) return 0;
	memmove (lob->list+i, lob->list+i+1,
		lob->width * (lob->nused - i - 1));
	lob->nused--;
	lob->nlen--;
	return 0;
}

