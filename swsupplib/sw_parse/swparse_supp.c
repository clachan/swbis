/* swparse_supp.c: support routines for the parser.
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.
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
#include <ctype.h>
#include "strob.h"
#include "uxfio.h"
#include "swparse.h"
#include "swlib.h"
#include "swheaderline.h"
#include "swutilname.h"

extern int swparse_outputfd;
int swparse_construct_attribute (STROB * strb, int uxfio_ofd, char * src,
			int cmd, int level, char s_keytype, int form_flag);
static int do_indent_only(STROB * strb, int outputfd, int level);
static int do_not_warn_utf;

static
void
squash_non_ascii_chars(unsigned char * ptr)
{
	unsigned char * s;
	s = ptr;
	while (*s) {
		if (*s <= 127) {
			;
		} else {
			memmove(s, s+1, (size_t)(strlen((char*)(s+1))+1));
		}
		s++;
	}
}

static
void
check_keyword(char * w)
{

}

static int
check_ignores(char * line, char ** ignores)
{
	char ** sp = ignores;
	while (*sp) {
		if (strncmp(line, *sp,  strlen(*sp)) == 0) return 1;
		sp++;
	}
	return 0;
}

static 
int 
do_indent_only(STROB * strb, int outputfd, int level)
{
	char * keyword = strob_get_str(strb);
	char * value = strchr(strob_get_str(strb), (int)' ');
	
	if (value) {
		*value='\0'; value++;
		return 
			swdef_write_attribute(keyword, value, level, 0,
				'A' /* SWPARSE_MD_TYPE_ATT */, outputfd);   
	} else {
		return swdef_write_attribute(keyword, value, level, (int)0,
				'O' /* SWPARSE_MD_TYPE_OBJ */, outputfd);   
	}
}

int
swparse_write_attribute_att(int outputfd, char * key, char * val,
					int level, int form_flag)
{
	int ret, newlen;
	STROB * strb;
	char * p;
    
	strb = strob_open (12);
	check_keyword(key);
	strob_strcpy(strb, key);
	strob_strcat(strb, " ");
	swparse_expand_n((void**)(&p), &newlen, val);
	strob_strcat ( strb, p );
	if (p) swbis_free(p);
	ret=swparse_construct_attribute(strb, outputfd, (char*)(NULL),
						SWPARSE_ACMD_EMIT, level,
		SWPARSE_MD_TYPE_ATT, form_flag);
	strob_close (strb);
	return ret;
}

int
swparse_ignore_attribute(int filetype, int location, char * line)
{
	int ret = 0;
	char * psf_file_ignores[] = 
		{ "cksum ",
		"compressed_cksum ",
		"compressed_size ",
		"compression_state ",
		"compression_type ",
		"size ",
		(char*)(NULL)
		};
	char * psf_fileset_ignores[] = 
		{ 
		"location ",
		"media_sequence_number ",
		"size ",
		"state ",
		(char*)(NULL)
		};
	char * psf_bundle_ignores[] = 
		{ 
		"location ",
		"qualifier ",
		(char*)(NULL)
		};
	char * psf_distribution_ignores[] = 
		{ 
		"uuid ",
		(char*)(NULL)
		};
	char * info_file_ignores[] = 
		{ 
		"source ",
		(char*)(NULL)
		};
	char * index_ignores[] = 
		{ 
		"size ",
		(char*)(NULL)
		};

	if (filetype == SWPARSE_SWDEF_FILETYPE_INFO) {
		if (location == SWPARSE_ILOC_FILE) {
			ret = check_ignores(line, info_file_ignores);
		}
	} else if (filetype == SWPARSE_SWDEF_FILETYPE_PSF) {
		if (location == SWPARSE_ILOC_FILE) {
			ret = check_ignores(line, psf_file_ignores);
		}
		else if (location == SWPARSE_ILOC_CONTROL_FILE) {
			ret = check_ignores(line, psf_file_ignores);
			if (ret == 0) {
				ret = !strncmp(line, "result ",
						strlen("result "));
			}
		}
		else if (location == SWPARSE_ILOC_FILESET) {
			ret = check_ignores(line, psf_fileset_ignores);
		}
		else if (location == SWPARSE_ILOC_BUNDLE) {
			ret = check_ignores(line, psf_bundle_ignores);
		}
		else if (location == SWPARSE_ILOC_DISTRIBUTION) {
			ret = check_ignores(line, psf_distribution_ignores);
		}
	} else if (filetype == SWPARSE_SWDEF_FILETYPE_INDEX) {
		ret = check_ignores(line, index_ignores);
	}

	return ret;
}

int
swparse_write_attribute_obj(int outputfd, char * key, int level, int form_flag)
{
	int ret;
	STROB * strb;
  
	check_keyword(key);
	if (form_flag & SWPARSE_FORM_INDENT) {
		return swdef_write_keyword(key, level, 
				'O' /* SWPARSE_MD_TYPE_OBJ */ , outputfd); 
	}
    
	strb = strob_open (12);
	strob_strcpy(strb, key);
	ret=swparse_construct_attribute(strb, outputfd, (char*)NULL,
						SWPARSE_ACMD_EMIT, level, 
					SWPARSE_MD_TYPE_OBJ, form_flag);
	strob_close (strb);
	return ret;
}

void
swparse_set_do_not_warn_utf(void)
{
	do_not_warn_utf = 1;
}

void
swparse_unset_do_not_warn_utf(void)
{
	do_not_warn_utf = 0;
}

int
swparse_construct_attribute(STROB * strb, int outputfd, char * src,
			int cmd, int level, char s_keytype, int form_flag)
{
	static STROB * ustore;
	char * ptr, *eptr; 
	char * p;
	char l_swws[]="                                       ";
	int newlen, ret=0, value_length;
	int extra_len;

	if (level > 10) return -1;
	if ((form_flag & SWPARSE_FORM_INDENT) && cmd == SWPARSE_ACMD_EMIT) {
		return do_indent_only(strb, outputfd, level);
	}

	l_swws[level]='\0';
	if ((form_flag & SWPARSE_FORM_MKUP) ||
		((form_flag & SWPARSE_FORM_ALL) == 0)) {
		extra_len=0;
	} else { 
		extra_len=SWPARSE_MKUP_RES; 
	}

	if (cmd == SWPARSE_ACMD_COPY) {
		swparse_expand_n((void**)(&p), &newlen, src);
		strob_strcpy(strb, p);
		swbis_free(p);
	} else if (cmd == SWPARSE_ACMD_CAT) {
		swparse_expand_n((void**)(&p), &newlen, src);
		strob_strcat(strb, p);
		swbis_free(p);
	} else if (cmd == SWPARSE_ACMD_EMIT) {
		int memlen;
		ptr=strob_str(strb);
		if (!strlen(ptr)) {
			strob_strcat(strb,"\"\"");
			ptr=strob_str(strb);
		}
		/* Now tranaslate \xXX escapes and validate to UTF-8 */
		swlib_process_hex_escapes(ptr);
		if (utf8_valid((const unsigned char *)ptr, (unsigned int)strlen(ptr)) == 0) {
			if (!ustore) ustore = strob_open(100);
			swlib_expand_escapes(NULL, NULL, ptr, ustore);
			if (do_not_warn_utf == 0)
				fprintf(stderr, "%s: warning: invalid UTF-8 characters: %s\n", swlib_utilname_get(), strob_str(ustore));
			/* squash_non_ascii_chars((unsigned char *)ptr); */
		}

		ptr=strob_str(strb);
		memlen = strlen(ptr) + level + 6 + extra_len;
		eptr = malloc(memlen);
		strob_setlen(strb, memlen);
		ptr=strob_str(strb);
		if (form_flag & SWPARSE_FORM_MKUP) {
			snprintf(eptr, memlen,
				"%c%02d%s%s\n", s_keytype, level, l_swws, ptr);
			eptr[memlen - 1] = '\0';
		} else if (form_flag & SWPARSE_FORM_MKUP_LEN) {
			p=ptr; 
			while(*p && !isspace((int)*p)) p++; 
			p++;
			value_length=strlen(p);
			snprintf(eptr, memlen,  
				"%-" SWPARSE_MKUP_LEN_WIDTH_C "d %c%02d %s\n",
					value_length, s_keytype, level, ptr);
			eptr[memlen - 1] = '\0';
		} else {
			return -1;
		}
		ret=uxfio_write(outputfd, eptr, strlen(eptr));
		swbis_free(eptr); 
	} else {
		return -1;
	}
	return ret;
}

int 
swparse_print_filename(char * buf, int len, char * filetype,
		char * ws_level_string, char * lex_filename, int form_flag)
{ 
	int value_length; 
	if (form_flag & SWPARSE_FORM_MKUP) {
		snprintf(buf, len, "F%02d %s%s %s\n",
			(int)strlen(ws_level_string)+1,
			ws_level_string, filetype, lex_filename);
		buf[len-1] = '\0';
		uxfio_write (swparse_outputfd, buf, strlen(buf));
	} else {
		value_length=strlen(lex_filename); 
		snprintf(buf, len, 
			"%-" SWPARSE_MKUP_LEN_WIDTH_C "d F%02d %s%s %s\n",
					(int)value_length,
                			(int)strlen(ws_level_string)+1, 
					ws_level_string,  
					filetype, lex_filename);
		buf[len-1] = '\0';
		uxfio_write(swparse_outputfd, buf, strlen(buf));
	}
	return 0;
}

int 
swparse_print_filename_by_fd(char * buf, int len, int fd, char * filetype,
		char * ws_level_string, char * lex_filename, int form_flag)
{ 
	{
	/* extern overridden */ swparse_outputfd = fd;
	return swparse_print_filename(buf, len, filetype, ws_level_string, lex_filename, form_flag);
	}
}

/**
 * utf8_valid(const unsigned char *buf, unsigned int len)
 * Taken from RFC2640 
 *
 * Copyright (C) The Internet Society (1999).  All Rights Reserved.
 *
 * This document and translations of it may be copied and furnished to
 * others, and derivative works that comment on or otherwise explain it
 * or assist in its implementation may be prepared, copied, published
 * and distributed, in whole or in part, without restriction of any
 * kind, provided that the above copyright notice and this paragraph are
 * included on all such copies and derivative works.  However, this
 * document itself may not be modified in any way, such as by removing
 * the copyright notice or references to the Internet Society or other
 * Internet organizations, except as needed for the purpose of
 * developing Internet standards in which case the procedures for
 * copyrights defined in the Internet Standards process must be
 * followed, or as required to translate it into languages other than
 * English.
 *
 * The limited permissions granted above are perpetual and will not be
 * revoked by the Internet Society or its successors or assigns.
 *
 * This document and the information contained herein is provided on an
 * "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
 * TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 */

int utf8_valid(const unsigned char *buf, unsigned int len)
{
 const unsigned char *endbuf = buf + len;
 unsigned char byte2mask=0x00, c;
 int trailing = 0;  // trailing (continuation) bytes to follow

 while (buf != endbuf)
 {
   c = *buf++;
   if (trailing) {
    if ((c&0xC0) == 0x80) {  /* Does trailing byte follow UTF-8 format? */
    	if (byte2mask) {        /* Need to check 2nd byte for proper range? */
	      if (c&byte2mask)     /* Are appropriate bits set? */
		byte2mask=0x00;
	else
		return 0;
	}
     trailing--;
    } else {
     return 0;
    }
   } else {
    if ((c&0x80) == 0x00)  continue;      /* valid 1 byte UTF-8 */
    else if ((c&0xE0) == 0xC0)            /* valid 2 byte UTF-8 */
          if (c&0x1E)                     /* Is UTF-8 byte in */
                                          /* proper range? */
           trailing =1;
          else
           return 0;
    else if ((c&0xF0) == 0xE0)           /* valid 3 byte UTF-8 */
          {if (!(c&0x0F))                /* Is UTF-8 byte in */
                                         /* proper range? */
            byte2mask=0x20;              /* If not set mask */
                                         /* to check next byte */
            trailing = 2;}
    else if ((c&0xF8) == 0xF0)           /* valid 4 byte UTF-8 */
          {if (!(c&0x07))                /* Is UTF-8 byte in */
            byte2mask=0x30;              /* If not set mask */
                                         /* to check next byte */
            trailing = 3;}
    else if ((c&0xFC) == 0xF8)           /* valid 5 byte UTF-8 */
          {if (!(c&0x03))                /* Is UTF-8 byte in */
                                         /* proper range? */
            byte2mask=0x38;              /* If not set mask */
                                         /* to check next byte */
            trailing = 4;}
    else if ((c&0xFE) == 0xFC)           /* valid 6 byte UTF-8 */
          {if (!(c&0x01))                /* Is UTF-8 byte in */
                                         /* proper range? */
            byte2mask=0x3C;              /* If not set mask */
                                         /* to check next byte */
            trailing = 5;}
    else  return 0;
  }
 }
  return trailing == 0;
}
