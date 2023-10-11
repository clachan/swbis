/* swparse_supp1.c
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "strob.h"
#include "uxfio.h"

int swlib_is_ansi_escape(char c);
static char * de_quote_it (char * src);

int 
swparse_expand_n ( void **pa, int *newlen, char * src) {
   int n;
   int i=0, j=0;
   char *lp;
   STROB * store;

	store = strob_open(2);
	strob_strcpy(store, "");
	src=de_quote_it(src);
	lp = src;
   
	n=strlen(lp);
   	strob_set_memlength(store, n+1);

	while ( i < n ) {
		if ( *(lp + i)  == '\n' ) {
			strob_chr_index(store, i+j,  '\\');
			j++;
			strob_chr_index(store, i+j, 'n');
     		} else if (*(lp + i)  == '\\') {
     			if ( *(lp + i + 1) == '\\' ) {
			        strob_chr_index(store, i+j, *(lp+i));
			        i++;
			        strob_chr_index(store, i+j, *(lp+i));
			} else if ( *(lp + i + 1) == 'n' ) {
			        strob_chr_index(store, i+j, *(lp+i));
			        i++;
			        strob_chr_index(store, i+j, *(lp+i-1));
				j++; 
			        strob_chr_index(store, i+j, *(lp+i));
			} else if ( *(lp + i + 1) == '#' ) {
			        strob_chr_index(store, i+j, '#');
			        i++; j--;
			} else if ( *(lp + i + 1) == '\"' ) {
			        strob_chr_index(store, i+j, '\"');
			        i++; j--;
			} else if ( *(lp + i + 1) == '\0' ) {
			        strob_chr_index(store, i+j, '\\');
			        j++;
			        strob_chr_index(store, i+j, '\\');
			} else if ( !swlib_is_ansi_escape ( *(lp + i + 1)) ) {
			        strob_chr_index(store, i+j, '\\');
			        j++;
			        strob_chr_index(store, i+j,  '\\');
			} else {
			        strob_chr_index(store, i+j, *(lp + i));
			}
		} else {
		        strob_chr_index(store, i+j, *(lp + i));
		}
		i++;
	}
   	strob_chr_index(store, i + j, '\0');
   	*pa = strob_release(store);
   	*newlen = i + j;
   	return 0;
}

static char *
de_quote_it (char * src)
{
	if (*src != '\"') return src;
	src++;
	if (!strlen(src)){
		return --src;
	}
	*(src + strlen(src) -1)='\0';
	return src;
}

