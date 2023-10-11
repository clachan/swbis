/*  swlex_supp.c: support routines for the scanner/parser.
 */

/* 
   //  Copyright (C) 1998  James H. Lowe, Jr.

   //  This program is free software; you can redistribute it and/or modify
   //  it under the terms of the GNU General Public License as published by
   //  the Free Software Foundation; either version 3, or (at your option)
   //  any later version.

   //  This program is distributed in the hope that it will be useful,
   //  but WITHOUT ANY WARRANTY; without even the implied warranty of
   //  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   //  GNU General Public License for more details.

   //  You should have received a copy of the GNU General Public License
   //  along with this program; if not, write to the Free Software
   //  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "strob.h"
#include "uxfio.h"
#include "swlex_supp.h"
#include "sw_keyword.h"


extern struct swbis_keyword sw_keywords[];
extern int swlex_inputfd;
static int is_keyword(char *string);
int fnmatch(const char *, const char *, int flags);

int
swlex_handle_keyword(char *string, int keycode, int *keytype)
{
	int lexuser_keyword_offset = -1;
	struct swbis_keyword *keyword_ptr = sw_keywords;
	char *object_keyword;

	*keytype = -1;
	lexuser_keyword_offset = is_keyword(string);
	if (lexuser_keyword_offset >= 0) {

		if (!strcmp("layout_version",
			((keyword_ptr + lexuser_keyword_offset)->name))) {
			*keytype = SWLEX_KEYTYPE_ATTR;
			return SW_AK_LAYOUT_VERSION;
		}
		if (

			   (((keyword_ptr + lexuser_keyword_offset)->flag) !=
						SWLEX_SW_ATTRIBUTE_KEYWORD) &&
			   (((keyword_ptr + lexuser_keyword_offset)->flag) !=
						SWLEX_SW_EXTENDED_KEYWORD) &&
			   (((keyword_ptr + lexuser_keyword_offset)->flag) !=
						SWLEX_SW_OBJECT_KEYWORD) &&

			   (keycode < 0)
		    ) {
			*keytype = SWLEX_KEYTYPE_AMBIG;

		}
		if (
			((keyword_ptr + lexuser_keyword_offset)->flag) ==
			   ((SWLEX_SW_ATTRIBUTE_KEYWORD) |
						(SWLEX_SW_EXTENDED_KEYWORD))
		    ) {
			/* printf ("--------------AE\n"); */
			*keytype = SWLEX_KEYTYPE_AMBIG_AE;
		}
		if (
			((keyword_ptr + lexuser_keyword_offset)->flag) ==
		((SWLEX_SW_OBJECT_KEYWORD) | (SWLEX_SW_ATTRIBUTE_KEYWORD))
		    ) {
			/* fprintf (stderr, 
			"%s ============= OA =============\n", string); */
			*keytype = SWLEX_KEYTYPE_AMBIG_OA;
		}
		if (
			((keyword_ptr + lexuser_keyword_offset)->flag) ==
		((SWLEX_SW_OBJECT_KEYWORD) | (SWLEX_SW_EXTENDED_KEYWORD))
		    ) {
			/* printf ("--------------OE\n"); */
			*keytype = SWLEX_KEYTYPE_AMBIG_OE;
		}
		if (((keyword_ptr + lexuser_keyword_offset)->flag) &
						SWLEX_SW_OBJECT_KEYWORD) {

			object_keyword = 
				(keyword_ptr + lexuser_keyword_offset)->name;

			if ((*keytype) < 0)
				*keytype = SWLEX_KEYTYPE_OBJECT;

			if (!strcmp("distribution", object_keyword)) {
				return SW_OK_DISTRIBUTION;
			} else if (!strcmp("installed_software",
							object_keyword)) {
				return SW_OK_INSTALLED_SOFTWARE;
			} else if (!strcmp("category", object_keyword)) {
				return SW_OK_CATEGORY;
			} else if (!strcmp("bundle", object_keyword)) {
				return SW_OK_BUNDLE;
			} else if (!strcmp("product", object_keyword)) {
				return SW_OK_PRODUCT;
			} else if (!strcmp("subproduct", object_keyword)) {
				return SW_OK_SUBPRODUCT;
			} else if (!strcmp("fileset", object_keyword)) {
				return SW_OK_FILESET;
			} else if (!strcmp("control_file", object_keyword)) {
				if ((*keytype) < 0)
					*keytype = SWLEX_KEYTYPE_AMBIG;
				return SW_OK_CONTROL_FILE;
			} else if (!strcmp("file", object_keyword)) {
				if ((*keytype) < 0)
					*keytype = SWLEX_KEYTYPE_AMBIG;
				return SW_OK_FILE;
			} else if (!strcmp("vendor", object_keyword)) {
				return SW_OK_VENDOR;
			} else if (!strcmp("media", object_keyword)) {
				return SW_OK_MEDIA;
			} else if (!strcmp("host", object_keyword)) {
				return SW_OK_HOST;
			} else {
				return -1;
			}
		} else if (((keyword_ptr + lexuser_keyword_offset)->flag) &
						SWLEX_RPM_ATTRIBUTE_KEYWORD) {
			if ((*keytype) < 0) {
				*keytype = SWLEX_KEYTYPE_RPM;
			}
			return SW_RPM_KEYWORD;
		} else if (((keyword_ptr + lexuser_keyword_offset)->flag) ==
						SWLEX_SW_EXTENDED_KEYWORD) {
			if ((*keytype) < 0) {
				*keytype = SWLEX_KEYTYPE_EXT;
			} else {
				fprintf(stderr, 
					"internal error in swlex_supp\n");
			}
			return SW_EXT_KEYWORD;
		} else {
			if ((*keytype) < 0)
				*keytype = SWLEX_KEYTYPE_ATTR;
			return SW_ATTRIBUTE_KEYWORD;
		}
	} else {		/* unknown keyword */
		*keytype = SWLEX_KEYTYPE_ATTR;
		return -1;
	}
}

int
handle_termination(char termch, STROB * strb, char *str, char * loc)
{
	int len, i = 0, j = 0;
	char *p;

	strob_catstr(strb, str);
	p = strob_str(strb);
	len = strlen(p);

	if (len == 1)
		return 1;	/* terminate */
	if (termch == '\n' && p[len - 1] == '\n')
		return 1;	/* terminate */

	i = len - 2;
	while (i >= 0 && p[i] == '\\') {
		j++;
		i--;
	}
	if (j == 1 || j % 2) {
		return 0;	/* continue */
	} else {
		return 1;
	}
	return 0;	/* continue, the termination char was escaped */
}


int 
is_keyword(char *string)
{
	struct swbis_keyword *ptr = sw_keywords;

	while (ptr->name != NULL) {
		if (strchr(string, (int) ('.'))) { /* Multi Language support */
			if (fnmatch(ptr->name, string, 0) == 0) {
				return (int) (ptr - sw_keywords);
			}
		} else {
			if (strcmp(ptr->name, string) == 0) {
				return (int) (ptr - sw_keywords);
			}
		}
		ptr++;
	}
	return -1;
}


int 
keyword_check(char *string)
{
	int lexuser_keyword_offset = -1;
	struct swbis_keyword *keyword_ptr = sw_keywords;

	lexuser_keyword_offset = is_keyword(string);
	if (lexuser_keyword_offset >= 0) {
		fprintf(stderr, " * KEYWORD FOUND * %s\n", string);

		if (((keyword_ptr + lexuser_keyword_offset)->flag) &
						SWLEX_SW_ATTRIBUTE_KEYWORD) {
			printf("IS SW ATTRIBUTE KEYWORD");
		} else if (((keyword_ptr + lexuser_keyword_offset)->flag) &
						SWLEX_RPM_ATTRIBUTE_KEYWORD) {
			printf("IS RPM ATTRIBUTE KEYWORD");
		} else if (((keyword_ptr + lexuser_keyword_offset)->flag) &
						SWLEX_SW_OBJECT_KEYWORD) {
			printf("IS SW OBJECT KEYWORD");
		} else {
			printf("SYNTAX ERROR");
		}


	} else {
		fprintf(stderr, " * KEYWORD _*NOT*_ FOUND * %s\n", string);
	}

	printf("\n");
	return 0;
}

int 
swlex_squash_trailing_white_space(char *s)
{
	int i;

	i = strlen(s) - 1;
	while (i >= 0 && isspace(s[i]))
		s[i--] = '\0';
	return 0;
}
