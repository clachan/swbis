/* swheaderline1.c  --  Routines to set values in the parser output.
   
   Copyright (C) 1998, 1999  Jim Lowe 
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swheaderline.h"

#define NEWLINE_LEN 1

void
swheaderline_set_level(char *outputline, int level)
{
	char slev[4];
       	char * p = swheaderline_get_type_pointer(outputline);
	if (p) {
		snprintf(slev, sizeof(slev), "%02d",level);
		slev[sizeof(slev) - 1] = '\0';
		memcpy(p+1, slev, 2);
	}
}

/*
o__inline__
void
swheaderline_set_value_length(char *outputline, int length)
{
NOT USED  sprintf (outputline, "%-12d", length);
}
*/

int
swheaderline_write_debug(char * line, int fd)
{
	int i, ret;
	char * p;
	char * keyw=swheaderline_get_keyword(line);
	char * value=swheaderline_get_value(line, &i);
	int level=swheaderline_get_level(line);

	if (!strlen(value)) {
		ret=swparse_write_attribute_obj(fd, keyw, level, 1);
	} else {
		p=strchr(value, (int)'\n');
		if (!p){
			p=value + strlen(value);	
		}
		*p='\0';
		ret=swparse_write_attribute_att(fd, keyw, value, level, 1);
		*p=(char)'\n';
	}
	return ret;
}
