/* swheaderline.h
 
 Copyright (C) 1998, 1999  Jim Lowe <jhlowe@acm.org>

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

#ifndef SWHEADERLINE_122998_H
#define SWHEADERLINE_122998_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "strob.h"
#include "uxfio.h"
#include "swparse.h"

int swdef_write_value (char * value, int uxfio_ofd, int value_length, int indention_level);
int swdef_write_value_to_buffer (STROB * ob, char * value);
int swdef_write_keyword (char * keyword, int level, int attribute_type, int uxfio_fd);
int swdef_write_keyword_to_buffer (STROB * ob, char * keyword, int level, int attribute_type);
int swdef_write_attribute (char * keyword, char * value, int level, int len, int attribute_type, int uxfio_fd);
int swdef_write_attribute_to_buffer(STROB * buf, char *keyword, char *value, int level, int attribute_type);

char *  swheaderline_get_type_pointer		(char *output_line);
int     swheaderline_write			(char * line, int uxfio_fd);
int     swheaderline_write_to_buffer		(STROB * buf, char * line);
int     swheaderline_write_debug		(char * line, int uxfio_fd);
int     swheaderline_get_level			(char * outputline);
void    swheaderline_set_level			(char * outputline, int level);
char    swheaderline_get_type			(char * outputline);
char *  swheaderline_get_keyword		(char * outputline);
int     swheaderline_get_value_length		(char * outputline);
void 	swheaderline_set_value_length		(char * outputline, int length);
char *  swheaderline_get_value 			(char * outputline, int * value_len);
char *  swheaderline_get_value_pointer		(char * outputline, int * value_len);
char * 	swheaderline_strdup			(char * outputline);
void    swheaderline_set_flag1(char * output_line);
void    swheaderline_clear_flag1(char * output_line);
int     swheaderline_get_flag1(char * output_line);

#endif

