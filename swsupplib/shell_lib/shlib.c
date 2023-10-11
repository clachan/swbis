/*   shlib.c: Decompress the shell routines in shell_lib.c (shell_lib.sh)
 */

/*
 * Copyright (C) 2006 James H. Lowe, Jr.  <jhlowe@acm.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "strob.h"
#include "shlib.h"
#include "minilzo.h"
#include "lzoconf.h"

/* Include the generated file of shell functions */
#include "shell_lib.c"

int
uncompress_func(STROB * plain_text, struct shell_lib_function * f)
{
	char * s;
	int ret;
	lzo_uint in_len;
	lzo_uint out_len;
	lzo_uint new_out_len;
	lzo_byte * inbuf;
	lzo_byte * outbuf;

	/*
	controlM looks like this "0000000331:0000000890:COMPRESSION=1:"
	*/	

	in_len = atoi(f->controlM);
	s = strchr(f->controlM, ':');
	if (s) {
		s++;
	} else {
		return -1;
	}
	out_len = atoi(s);
	strob_set_length(plain_text, (int)(out_len)+100);
	inbuf = (lzo_byte *)(f->textM);
	outbuf = (lzo_byte *)strob_str(plain_text);

	ret = lzo1x_decompress(inbuf, (lzo_uint)(in_len),
                                outbuf, &new_out_len, NULL);


	if (ret == LZO_E_OK && new_out_len == out_len) {
		return 0;
	} else {
		fprintf(stderr, "decompression failed for %s  %d %d\n", f->nameM, new_out_len , out_len );
		return -1;
	}
}

int
unescape_text(char * text, STROB * buf, int len)
{
	char * s;
	char * end;
	int m;

	s = text;
	strob_strcpy(buf, "");
	while(*s) {
		if (strncmp(s, "\\x", 2) == 0) {
			s+=2;
			m = (int)strtol(s, &end, 16);
			s+=2;
			if (end != s) {
				fprintf(stderr, "unescape_text: error, loc=2\n");
				return -2;
			}
			strob_chr(buf, m);
		} else {
			fprintf(stderr, "unescape_text: error, loc=1\n");
			return -1;
		}
	}	
	return 0;
}

static int
decode_text(struct shell_lib_function * f, STROB * buf)
{
	int ret;
	int len;
	ret = 0;
	if (f->functionM != NULL) {
		return 0;
	}
	if (strstr(f->controlM, "COMPRESSION=1")) {
		ret = uncompress_func(buf, f);
	} else if (*(f->textM) == '\\') {
		/* This happens if we double \\ the '\x' codes which
		   prevents the compiler from unescaping strings in shell_lib.c */

		/* 2006-10 Current not used */
		len = atoi(f->controlM);
		ret = unescape_text(f->textM, buf, len);
	} else {
		strob_strcpy(buf, f->textM);
	}
	if (f->functionM == NULL) {
		f->functionM = strdup(strob_str(buf));
	}
	return ret;
}

struct shell_lib_function *
shlib_get_function_array(void)
{
	return shell_lib_function_pallet;
}

struct shell_lib_function *
shlib_get_function_struct(char * function_name)
{
	struct shell_lib_function * f;
	f = shlib_get_function_array();

	while (f->textM) {
		if (strcmp(f->nameM, function_name) == 0) {
			return f;
		}
		f++;
	}
	return (struct shell_lib_function *)NULL;
}

char *
shlib_get_function_text(struct shell_lib_function * f, STROB * buf)
{
	int ret;
	ret = decode_text(f, buf);
	if (ret) return NULL;
	return f->functionM;
}

char *
shlib_get_function_text_by_name(char * function_name, STROB * buf, int * pret)
{
	int ret;
	struct shell_lib_function * f;

	f = shlib_get_function_struct(function_name);
	if (f == NULL) {
		ret = -1;
		goto out;
	}

	ret = decode_text(f, buf);
	if (ret) {
		ret = -1;
		goto err_out;
	} else {
		ret = 0;
		goto out;
	}
err_out:	
	strob_strcpy(buf, "");
out:
	if (pret) *pret = ret;
	if (ret) {
		return NULL;
	} else {
		return f->functionM;
	}
}

