/* swheaderline.c  --  Low-Level Routines to read the parser output.
   
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

#define MAGNITUDE_ARRAY_LEN 10

/*
   The routines in this file parse or scan the parser output line 
   produced by the parser.  The stand-alone parser can make this
   output by:

	swprogs/swbisparse -n {--psf|--info|--index} <INPUT_FILE

An example of INFO file output is:

0    O01 file
12   A02 path ./autogen.sh
1    A02 type f
3    A02 mode 755

A line has the form:

SSSS X0N keyword value
	where
		SSSS is a decimal number which is the length of the value in octets.
		X    is the attribute type 'A' or 'O' or 'E' for
			Attribute, Object or Extended.
		0    is the first digit of the containment level, always 0 since
			levels >9 do not exist in the grammar.

		N   is ones digit of the containment level.

		keyword is the attribute keyword
	
		value is the value
*/

static
char *
get_value(char *output_line, int *value_len, int do_terminate)
{
	char *ret;
	int len;

	if (!output_line) return (char*)(NULL);
	if (!(ret = swheaderline_get_keyword(output_line)))
		return NULL;

	if (swheaderline_get_type(output_line) == SWPARSE_MD_TYPE_OBJ) {
		if (value_len) *value_len = 0;
		return ret + strlen(ret);
	}

	len=swheaderline_get_value_length(output_line);

	if (value_len) *value_len=len;
	
	if (do_terminate)
		*(ret + strlen(ret) + 1 +len) = '\0';
	return ret + strlen(ret) + 1;	/* pointer to  the value */
}

char * 
swheaderline_get_type_pointer(char *output_line)
{
	char * ret;
	if (isdigit((int) (*output_line))) {
		if (*(output_line + SWPARSE_MKUP_LEN_WIDTH) == ' ') {
			ret = output_line + SWPARSE_MKUP_LEN_WIDTH + 1;
		} else {
			/* long value, the line image image has shifted 1 column. */
			char *t;

			t = output_line;
	
			/* Step over the size, which we hope is > than  SWPARSE_MKUP_LEN_WIDTH
				digits long, otherwise there are other problems */

			while (isdigit((int)(*t))) {
				t++;
			}

			if (*t == '\0' || *t != ' ') {
				fprintf(stderr, "internal error in swheaderline_get_type_pointer\n");
				return NULL;
			}

			/* step over the space */
			t++;
	
			if (
				t[0] == SWPARSE_MD_TYPE_ATT || 
				t[0] == SWPARSE_MD_TYPE_OBJ ||
				t[0] == SWPARSE_MD_TYPE_EXT || 
				t[0] == SWPARSE_MD_TYPE_FILEREF || 
				0
			)  {
				;
				/* good */
			} else {
				fprintf(stderr, "internal error in swheaderline_get_keyword loc=2\n");
				return NULL;
			}
			ret = t;
		}
		return ret;
	} else {
		return output_line;
	}
}

int
swheaderline_write_to_buffer (STROB * buf,  char * line)
{
	char * keyword;
	char * value;
        int type;
	int level;

	if (!line) return -1;
	keyword = swheaderline_get_keyword(line);
	value =  swheaderline_get_value(line, NULL);
        type = (int)swheaderline_get_type(line);
	level = swheaderline_get_level(line);
	if (keyword && strlen(keyword))	
		return swdef_write_attribute_to_buffer(buf, keyword, value, level, (int)type);
	else
		return 0;
}


int
swheaderline_write (char * line, int uxfio_fd)
{
	int len;
	char * keyword = swheaderline_get_keyword(line);
	char * value = 	swheaderline_get_value(line, &len);
        int type = (int)swheaderline_get_type(line);
	int level = swheaderline_get_level(line);
	len=-1;
	if (keyword)	
		return swdef_write_attribute(keyword, value,
					level, len, (int)type, uxfio_fd);
	else
		return 0;
}

int
swheaderline_get_level(char *outputline)
{
	char * typep;
	typep = swheaderline_get_type_pointer(outputline);
	if (!typep) { 
		fprintf(stderr, "internal error in swheaderline_get_level\n");
		exit(2);
	}

	typep += 2;
		/*
		* NOTE : MAX level is 9 since only the one's digit is scanned.
		*/
	return (int)((*typep) - 0x30);
}

char
swheaderline_get_type(char *output_line)
{
	char * p = swheaderline_get_type_pointer(output_line);
	if (p) return *p; else return (char)(0);
}

char *
swheaderline_get_keyword(char *output_line)
{
	char *t;
	char *ret;

	if (!output_line) return (char*)(NULL);

	if (isdigit((int) (*output_line))) {
		/* this step over the length field which is, which
		   is a fixed length unless the attribute value is very long */
		t = output_line + SWPARSE_MKUP_LEN_WIDTH + 1;
	} else {
		t = output_line;
	}

	/* 
	* Find the keyword part of the parser output line and
	* put a NULL after it.  This means a NULL char will separate
	* the keyword and value.
	*/

	/* t now points to the <type><level> field */
	/* e.g. A01' ' */

	if (
		t[0] == SWPARSE_MD_TYPE_ATT || 
		t[0] == SWPARSE_MD_TYPE_OBJ ||
		t[0] == SWPARSE_MD_TYPE_EXT || 
		t[0] == SWPARSE_MD_TYPE_FILEREF || 
		0)  {
		;
		/* good */
	} else {
		/* bad */
		/* Ok, now start from scratch, it must be a long attribute value */
		t = swheaderline_get_type_pointer(output_line);
		if (t == NULL) return NULL;
	}

	t+=3;
	/* the next char should be a space */

	while (*t && isspace((int)(*t))) t++;
	ret = t;
	while (*t && !isspace((int)(*t))) t++;
	*t = '\0';
	return ret;
}

int
swheaderline_get_value_length(char *output_line)
{
	int i;
	char  * t, *p;
	int n;
	int mytens[MAGNITUDE_ARRAY_LEN] = {0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};

	if (!isdigit((int)(*output_line))) {
		fprintf(stderr, "bad parser output, value length not found\n"); 
		return -1;
	}

	/*
	* Unoptimized working code.
	* sscanf(output_line, "%d", &n);
	* return n;
	*/


	/*
	 * The code below is a poor replacement for calling scanf.
	 * another case of early optimization causing problems.
	 */

	n = 0;
	t = output_line;
	p = output_line;
	while (*t && !isspace((int)(*t))) t++;
	for (i = (int)(t - output_line); i > 0; i--) {
		if (i >= MAGNITUDE_ARRAY_LEN) {
			fprintf(stderr,
				"fatal error: attribute value length in swheaderline_get_value_length() too big\n");
			/*
			 * JL  2018-11-1
			 * FIXME? just exit for now,
			 * the previous max length was 99999 which was exceeded by big packages,
			 * this should be considerered a security fix since it led to buffer overrun.
			 * Now really, really this should never happen.
			 */
			exit(33);
		}
		if (!isdigit(*p)) {
			fprintf(stderr,
		"internal error in swheaderline_get_value_length [%s]\n",
					output_line); 
		}
		n += ((int)((int)(*p) - 48)) * mytens[i];
		p++;
	}
	return n;
}

char *
swheaderline_strdup(char * outputline) {
	int value_length = 0;
	char * value = swheaderline_get_value(outputline, &value_length);
	int len = (int)(value - outputline) + value_length + 1;
	char * dup = (char *)malloc(len+1);
	*(dup+len) = '\0';
	memcpy(dup, outputline, len+1);
	return dup;
}

char *
swheaderline_get_value_pointer(char *output_line, int *value_len)
{
	return get_value(output_line, value_len, 0);
}

char *
swheaderline_get_value(char *output_line, int *value_len)
{
	return get_value(output_line, value_len, 1);
}

	/* Flag1 is only valid for object lines */
void
swheaderline_set_flag1(char * output_line) {
	*(output_line + 1) = '0';
}

	/* Flag1 is only valid for object lines */
void
swheaderline_clear_flag1(char * output_line) {
	*(output_line + 1) = ' ';  /* original value from parser */
}

	/* Flag1 is only valid for object lines */
int
swheaderline_get_flag1(char * output_line) {
	return *(output_line + 1) == ' ' ? 0 : 1;
}
