/* swi_common.h -  Posix package decoding
   Copyright (C) 2005  James H. Lowe, Jr.  <jhlowe@acm.org>
   All Rights Reserved.

   COPYING TERMS AND CONDITIONS:
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

#ifndef swi_base_h_200501
#define swi_base_h_200501

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shcmd.h"
#include "swlib.h"
#include "swutilname.h"
#include "uxfio.h"
#include "strob.h"
#include "swvarfs.h"
#include "hllist.h"
#include "defer.h"
#include "porinode.h"
#include "ahs.h"
#include "taru.h"
#include "taruib.h"
#include "uinfile.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swutillib.h"
#include "sw.h"
#include "swicol.h"
#include "swi_common.h"


typedef struct {
	unsigned char id_startM;  	/* Always SWI_BASE_ID_BEGIN */
	char type_idM;	
	int is_activeM;		
	char * b_tagM;
	time_t create_timeM;
	time_t mod_timeM;
	int header_indexM;		/* index in the global INDEX file to relevant part */
	SWHEADER * global_headerM;	/* Global INDEX access object	*/
	SWVERID * swveridM;
	int verboseM;			/* verbose level */
	unsigned char id_endM; 		/* Always SWI_BASE_ID_END */
	char * numberM;
} SWI_BASE;

typedef struct {
	SWI_BASE baseM;
} SWI_BASE_Derived_;

#define SWI_BASE_ID_BEGIN	85	 /* Sanity id */ 
#define SWI_BASE_ID_END		170	 /* Sanity id */

void swi_base_assert(SWI_BASE * base);
void swi_base_set_is_active(SWI_BASE * base, int n);

int swi_vbase_update(void * derived, void * user_);
void swi_vbase_init(void * derived, int type, SWHEADER * index, SWPATH_EX * current);
int swi_vbase_generate_swverid(void * derived, void * user_);
int swi_vbase_set_verbose_level(void * derived, void * verbose_level);

#endif
