/* swdef.c: write attribute/value pairs in metadata file.
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 * All Rights Reserved.
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
#include <unistd.h>
#include "uxfio.h"
#include "swlib.h"
#include "swheaderline.h"
#include "swparse.h"
#include "strob.h"


int
swdef_write_attribute_to_buffer(STROB * buf, char *keyword, char *value, int level, int attribute_type)
{
	int iret, ret;

	ret = swdef_write_keyword_to_buffer(buf, keyword, level, attribute_type);
	if (attribute_type != SWPARSE_MD_TYPE_OBJ) {
		iret = swdef_write_value_to_buffer(buf, value);
		if (iret < 0)
			return -1;
		ret += iret;
	}
	return ret;
}


int
swdef_write_attribute(char *keyword, char *value, int level, int len, 
					int attribute_type, int uxfio_fd)
{
	int iret, ret;

	ret = swdef_write_keyword(keyword, level, attribute_type, uxfio_fd);
	if (attribute_type != SWPARSE_MD_TYPE_OBJ) {
		iret = swdef_write_value(value, uxfio_fd, len, 0);
		if (iret < 0)
			return -1;
		ret += iret;
	}
	return ret;
}

int 
swdef_write_keyword_to_buffer(STROB * ob, char *keyword, int level, 
		int attribute_type)
{
	char sp[8];

	memset(sp, 040, sizeof(sp));
	if (level > (int)sizeof(sp)) level = 1;

	if (level)
		strob_strncat(ob, sp, level);

	strob_strcat(ob, keyword);
	if (attribute_type == SWPARSE_MD_TYPE_OBJ) {
		strob_strcat(ob, "\n");
	} else {
		strob_strcat(ob, " ");
	}
	return strob_strlen(ob);
}

int 
swdef_write_keyword(char *keyword, int level, 
			int attribute_type, int uxfio_fd)
{
	STROB * ob;
	int ret;
	int iret;
	ob = strob_open(32);
	ret = swdef_write_keyword_to_buffer(ob, keyword, level, attribute_type);
	iret = uxfio_write(uxfio_fd, strob_str(ob), ret);
	strob_close(ob);
	return iret;
}

int 
swdef_write_value_to_buffer(STROB * ob, char *value)
{
	char *newbuf = NULL, *s;
	int newlen, ret = 0;


	/* strob_strcpy(ob, ""); */
	if ((s = strstr(value, "\n"))) {
		*s = '\0';
	}
	swlib_expand_escapes(&newbuf, &newlen, value, (STROB*)NULL);
	if (newlen == 0) {
		strob_strcat(ob, "\"\"");
		ret = 2;
	} else if (strstr(value, "\\n") || 
			strstr(value, "\n") || strpbrk(value, "#\"\\")) {
		strob_strcat(ob, "\"");
		ret = 1;
		strob_strncat(ob, newbuf, newlen);
		ret += newlen;
		strob_strcat(ob, "\"");
		ret ++;
	} else {
		strob_strcat(ob, value);
		ret = strlen(value);
	}
	
	strob_strcat(ob, "\n");
	ret++;
	if (newbuf)
		swbis_free(newbuf);
	return ret;
}

int 
swdef_write_value(char *value, int uxfio_ofd, 
				int value_length, int quote_off)
{
	char buf0[8];
	char * newbuf = NULL;
	char * s;
	int newlen, ret = 0;


	if (value_length > 0) {
		ret += uxfio_write(uxfio_ofd, value, value_length);
	} else if (!strlen(value)) {
		sprintf(buf0, "\"\"");
		ret = uxfio_write(uxfio_ofd, buf0, strlen(buf0));
	} else {
		if (value_length < 0) {
			if ((s = strstr(value, "\n"))) {
				*s = '\0';
			}
		}
		swlib_expand_escapes(&newbuf, &newlen, value, (STROB*)NULL);
		if (!newlen) {
			sprintf(buf0, "\"\"");
			ret = uxfio_write(uxfio_ofd, buf0, strlen(buf0));
		} else if (
				strstr(value, "\\n") || 
				strstr(value, "\n") || 
				((s=strstr(value, "<")) && s != value) || 
				strstr(value, ">") || 
				strpbrk(value, "#\"\\") ||
				0
		) {
			sprintf(buf0, "\"");
			if (quote_off == 0) 
				ret = uxfio_write(uxfio_ofd, buf0, 
								strlen(buf0));
			ret += uxfio_write(uxfio_ofd, newbuf, newlen);
			if (quote_off == 0) ret += uxfio_write(uxfio_ofd, buf0,
								 strlen(buf0));
		} else {
			ret = uxfio_write(uxfio_ofd, value, strlen(value));
		}
	}
	sprintf(buf0, "\n");
	ret += uxfio_write(uxfio_ofd, buf0, strlen(buf0));
	if (newbuf)
		swbis_free(newbuf);
	return ret;
}
