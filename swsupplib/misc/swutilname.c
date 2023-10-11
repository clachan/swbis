/* swutilname.c - Set and Get the utility name at runtime
 */

/*
   Copyright (C) 1998-2004  James H. Lowe, Jr.
   All rights reserved.
  
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "swutilname.h"

static char swutilname[30];

void
swlib_utilname_set(char * s)
{
	strncpy(swutilname, s, sizeof(swutilname) - 1);
	swutilname[sizeof(swutilname) - 1] = '\0';
}

char * 
swlib_utilname_get(void)
{
	return swutilname;
}
