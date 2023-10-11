/* swlex_supp.h
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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


#ifndef SWLEX_SUPP_H_CPP_SENTINEL
#define SWLEX_SUPP_H_CPP_SENTINEL

#include "swuser_config.h"
#include "strob.h"
#include "swparse.tab.h"

int handle_termination (char termch, STROB * strb, char * str, char * loc); 
int swlex_handle_keyword ( char *string, int keycode , int * keytype);
int swlex_get_input_policy(void);
void swlex_set_input_policy(int);

#define SWLEX_INPUT_UTF_ALLOW	2
#define SWLEX_INPUT_UTF_TR	1 /* translate to ~ */
#define SWLEX_INPUT_UTF_FATAL	0

#define SWLEX_TERM_NEWLINE -3
#define SWLEX_DISAMBIGUATE -2
#define SWLEX_NOT_EXTENDED_KEYWORD 0
#define SWLEX_IS_EXTENDED_KEYWORD 1

#define SWLEX_KEYTYPE_AMBIG 0
#define SWLEX_KEYTYPE_ATTR 1
#define SWLEX_KEYTYPE_EXT 2
#define SWLEX_KEYTYPE_OBJECT 3
#define SWLEX_KEYTYPE_RPM 4
#define SWLEX_KEYTYPE_AMBIG_AE 5 /* attribute or extended */
#define SWLEX_KEYTYPE_AMBIG_OE 6 /* object or extended */
#define SWLEX_KEYTYPE_AMBIG_OA 7 /* object or extended */

#ifdef SWLEX_STAND_ALONE

#define SW_ATTRIBUTE_KEYWORD 257
#define SW_EXT_KEYWORD 258
#define SW_RPM_KEYWORD 259
#define SW_OBJECT_KEYWORD 260
#define SW_NEWLINE_STRING 261
#define SW_OK_NEWLINE_STRING 262
#define SW_TERM_NEWLINE_STRING 263
#define SW_PATHNAME_CHARACTER_STRING 264
#define SW_SHELL_TOKEN_STRING 265
#define SW_WHITE_SPACE_STRING 266
#define SW_EXT_WHITE_SPACE_STRING 267
#define SW_INDEX 268
#define SW_INFO 269
#define SW_PSF 270
#define SW_SWBISINFO 271
#define SW_LEXER_EOF 272
#define SW_AK_LAYOUT_VERSION 273
#define SW_OK_HOST 274
#define SW_OK_DISTRIBUTION 275
#define SW_OK_INSTALLED_SOFTWARE 276
#define SW_OK_BUNDLE 277
#define SW_OK_PRODUCT 278
#define SW_OK_SUBPRODUCT 279
#define SW_OK_FILESET 280
#define SW_OK_CONTROL_FILE 281
#define SW_OK_FILE 282
#define SW_OK_VENDOR 283
#define SW_OK_MEDIA 284
#define SW_OK_CATEGORY 285

#endif
#endif
