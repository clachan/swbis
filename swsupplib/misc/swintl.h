/* swintl.h - Non-english language support.

 (C) Copyright 1999 James Lowe, Jr.
  This file may redistributed under the terms of the GNU GPL.
*/

#ifndef SWINTL_19990121_h
#define SWINTL_19990121_h

struct swintl_lang_map
        {
		char * language_code_; /* ISO 639:1988 two letter code */
		char * language_name_; /* english name */
	};


char * swintl_get_lang_code (char * lang_name);
char * swintl_get_lang_name (char * lang_code);

#endif


