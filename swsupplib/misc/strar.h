/* strar.h  string array object 
 */

/* 
   Copyright (C) 2002 James H. Lowe, Jr.

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

#ifndef strar_19990124_h
#define strar_19990124_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "strob.h"
#include "cplob.h"

typedef struct {
	int nsM;
	int lenM;
	CPLOB * listM;
	STROB * storageM;
} STRAR;

STRAR *		strar_copy_construct	(STRAR *);
STRAR *		strar_open	(void);
void		strar_close	(STRAR * strar);
void		strar_reset	(STRAR * strar); 
int 		strar_add	(STRAR * strar, char * s);
char * 		strar_get	(STRAR * strar, int index);
int 		strar_num_elements(STRAR * strar);
char *		strar_dump_string_s(STRAR * strar, char * prefix);
char *		strar_return_store(STRAR * strar, int len);
void		strar_qsort(STRAR * strar, int (*)(const void*, const void*));
int		strar_qsort_neg_strcmp(const void * vf1, const void * vf2);
int		strar_qsort_strcmp(const void * vf1, const void * vf2);
void		strar_remove_index(STRAR * strar, int index);
#endif
