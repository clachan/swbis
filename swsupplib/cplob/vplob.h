/* vplob: void pointer list object.

   Copyright (C) 2006  James H. Lowe, Jr.
  
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

#ifndef vplob_h_
#define vplob_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cplob.h"

typedef union {
	CPLOB cplob;
} VPLOB;

VPLOB * vplob_open(void);
void vplob_close(VPLOB * vplob);
void vplob_shallow_close(VPLOB * vplob);
void vplob_add(VPLOB * vplob, void * addr);
void ** vplob_get_list(VPLOB * vplob);
void * vplob_val(VPLOB * vplob, int index);
int vplob_get_nstore(VPLOB * vplob);
int vplob_delete_store(VPLOB * vplob, void (*f_delete)(void*));
int vplob_remove_from_list(VPLOB * vplob, void * addr_to_remove);

#endif
