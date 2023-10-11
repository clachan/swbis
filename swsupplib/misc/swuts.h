/*  swi_uts.h -  The swbis system identification object.

  Copyright (C) 2006  James H. Lowe, Jr.
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

#ifndef swuts_h_200601
#define swuts_h_200601

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sw.h"
#include "swlib.h"

typedef struct {
	char * machineM;   /* machine_type */
	char * sysnameM;   /* os_name */
	char * releaseM;   /* os_release */
	char * versionM;   /* os_version */
	char * arch_tripletM;   /* output of config.guess */
	char result_machineM;
	char result_sysnameM;
	char result_releaseM;
	char result_versionM;
	int match_resultM;
} SWUTS;

SWUTS *   swuts_create(void);
void      swuts_delete(SWUTS *);
int       swuts_read_from_events(SWUTS *, char * events);
int       swuts_read_from_swdef(SWUTS *);
int       swuts_compare(SWUTS * uts_target, SWUTS * uts_swdef, int verbose);
void swuts_add_attribute(SWUTS * uts, char * attribute, char * value);
int	  swuts_is_uts_attribute(char * attr_name);
#endif
