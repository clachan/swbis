/* cplob: character pointer list object.
 */

/*
 * Copyright (C) 1997  James H. Lowe, Jr.
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

#ifndef cplob_h_19990124
#define cplob_h_19990124

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CPLOB_NINITLENGTH 16
#define CPLOB_NLENGTHINCR 8

typedef struct
{
  char ** list;
  int nlen;
  int nused;
  size_t width;  
  int refcountM;
} CPLOB;

typedef void (*CPLOB_F)(void *);

/*-------------------------------------------------------------*/
/*------- CPLOB  API (PUBLIC FUNCTIONS)------------------------*/

CPLOB * cplob_open        (int nobj);
int     cplob_shallow_close       (CPLOB * strb);
void    cplob_shallow_reset  (CPLOB * strb); 
void    cplob_close       (CPLOB * strb);
char ** cplob_release     (CPLOB * strb);
CPLOB * cplob_reopen      (int new_nobj, CPLOB * strb );
void    cplob_add_nta     (CPLOB * lob, char * addr);
void    cplob_add         (CPLOB * lob, char * addr);
void    cplob_additem     (CPLOB * lob, int index, char * addr);
char *  cplob_val         (CPLOB * lob, int index);
void    cplob_freeall     (CPLOB * lob);
int     cplob_backfill_and_nullterminate  (CPLOB * lob);
char ** cplob_get_list                    (CPLOB * lob);
int     cplob_get_nused                   (CPLOB * lob);
void    cplob_set_nused                   (CPLOB * lob, int n);
int	cplob_remove_index(CPLOB * cplob, int i);
#endif
