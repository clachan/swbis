/* swi_afile.h -  Posix control/attribute file decoding.

   Copyright (C) 2005  James H. Lowe, Jr.  <jhlowe@acm.org>
   All rights reserved.

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

#ifndef swi_afile_h_200501
#define swi_afile_h_200501

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
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
#include "sw.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swi_common.h"


typedef struct {
	SWI_BASE baseM;

	char * pathnameM;	/* package filename 			*/
	int lenM;		/* length of data 			*/
	char * dataM;		/* data 				*/
	int data_start_offsetM;
	int data_end_offsetM;
	int header_start_offsetM;
	int refcountM;
} SWI_FILE_MEMBER;	/* General file data, e.g control files		*/

typedef struct {		/* Control script object 		*/
	SWI_BASE baseM;
	int sidM;		/* Id number 				*/
	int resultM;		/* result				*/
	SWI_FILE_MEMBER * afileM;	/* SWI_FILE_MEMBER 		*/
	void * swi_xfileM;	/* (SWI_XFILE*) context			*/
	int INFO_offsetM;	/* offset in swi_xfileM->info_headerM	*/
			  	/* to the relevant software definition */
} SWI_CONTROL_SCRIPT;

typedef struct { 		/* List of SWI_CONTROL_SCRIPT		*/
	SWI_CONTROL_SCRIPT * swi_coM[SWI_MAX_OBJ];
} SWI_SCRIPTS;

void swi_scripts_delete(SWI_SCRIPTS * s);
SWI_SCRIPTS * swi_scripts_create(void);
void swi_file_member_delete(SWI_FILE_MEMBER * s);
SWI_FILE_MEMBER * swi_file_member_create(void);
void swi_control_script_delete(SWI_CONTROL_SCRIPT * s);
SWI_CONTROL_SCRIPT * swi_control_script_create(void);
void swi_add_script(SWI_SCRIPTS * thisisit, SWI_CONTROL_SCRIPT * v);
char * swi_control_script_posix_result(SWI_CONTROL_SCRIPT * s);
int swi_afile_write_script_cases(SWI_SCRIPTS * scripts, STROB * buf, char * isc);
char ** swi_afile_ieee_control_script_list(void);
int swi_afile_is_ieee_control_script(char * pathname);
int swi_control_script_get_return_code(char * posix_result);


#endif
