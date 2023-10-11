/* swparse.h
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


#ifndef SWPARSE_H_02dec
#define SWPARSE_H_02dec

#include "swuser_config.h"

#define SWPARSE_ACMD_COPY 0
#define SWPARSE_ACMD_CAT 1
#define SWPARSE_ACMD_EMIT 2

#define SWPARSE_SWDEF_FILETYPE_INFO		0
#define SWPARSE_SWDEF_FILETYPE_INDEX		1
#define SWPARSE_SWDEF_FILETYPE_PSF		2
#define SWPARSE_SWDEF_FILETYPE_INSTALLED 	3

#define SWPARSE_MKUP_LEN_WIDTH 		4
#define SWPARSE_MKUP_LEN_WIDTH_C 	"4"
#define SWPARSE_FORM_MKUP	(1 << 0)  /* default markup */
#define SWPARSE_FORM_MKUP_LEN	(1 << 1) /* include  attribute lengths 
					    in output */
#define SWPARSE_FORM_INDENT	(1 << 2)  /* beautify only */
#define SWPARSE_FORM_POLICY_POSIX_IGNORES (1 << 3)  /* ignore mtime as Std 
					 	       requires */
#define SWPARSE_FORM_ALL	(SWPARSE_FORM_MKUP|\
					SWPARSE_FORM_MKUP_LEN|\
						SWPARSE_FORM_INDENT)

#define SWPARSE_MD_TYPE_COMMENT '#'
#define SWPARSE_MD_TYPE_ATT 'A'   /* attribute keyword */
#define SWPARSE_MD_TYPE_EXT 'E'   /* extended keyword */
#define SWPARSE_MD_TYPE_OBJ 'O'   /* object keyword */
#define SWPARSE_MD_TYPE_DATA 'D'  /* file data of the 'F' file reference */
#define SWPARSE_MD_TYPE_FILEREF 'F'   /* file name reference */

#define SWPARSE_OFFSET_LEN 0	/* offset in line to the length of the value */
#define SWPARSE_MKUP_RES 30     /* Total conceivable length of the length,
				   plus some extra. */

#define SWPARSE_ILOC_OFF 0
#define SWPARSE_ILOC_FILE 1
#define SWPARSE_ILOC_CONTROL_FILE 2
#define SWPARSE_ILOC_FILESET 3
#define SWPARSE_ILOC_PRODUCT 4
#define SWPARSE_ILOC_DISTRIBUTION 5
#define SWPARSE_ILOC_BUNDLE 6

int yyerror(char *);
int yylex(void);
int yyparse(void);

int swparse_expand_n (void **pa, int *newlen, char * src); 
int swparse_write_attribute_att (int outputfd, char * key, char * val,
				int level, int offset);
int swparse_write_attribute_obj (int outputfd, char * key, int level,
					int offset);
/* int swparse__is_escapable (char c); */
int swparse_print_filename_by_fd (char * buf, int buflen, int fd, char * filetype,
				char * ws_level_string, char * lex_filename,
				int form_flag);
int swparse_print_filename (char * buf, int buflen, char * filetype,
				char * ws_level_string, char * lex_filename,
				int form_flag);
int sw_yyparse (int uxfio_ifd, int  uxfio_ofd, char * metadata_filename,
					int atlevel, int mark_up_flag);
int swparse_is_psf_ignore_attribute(char * line);
int swparse_ignore_attribute(int filetype, int location, char * line);
int utf8_valid(const unsigned char *buf, unsigned int len);
void swparse_set_do_not_warn_utf(void);
void swparse_unset_do_not_warn_utf(void);

#endif
