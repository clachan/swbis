/* Generate a C source file that contain the shell routines in
   shell_lib.sh as static constant strings */

/*
   Copyright (C) 2004  James H. Lowe, Jr.
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "strob.h"
#include "minilzo.h"
#include "lzoconf.h"

#define VERSION "0.1"

/* Work-memory needed for compression. Allocate memory in units
 * of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
 */
#define HEAP_ALLOC(var,size) \
	lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]
static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);

static int
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

static
void
conv_clump(STROB * convline, unsigned char * line, int len)
{
	unsigned char * s;
	int count;
	int linecount;

	linecount = 0;
	count = 0;
	s = line;
	strob_sprintf(convline, 0, "");
	while (count < len) {
		if (linecount == 0) strob_sprintf(convline, 1, "\"");
		linecount++;
		strob_sprintf(convline, 1, "\\x%02x", (int)(*s));
		s++;
		count++;
		if (linecount > 15) {
			linecount = 0;
			strob_sprintf(convline, 1, "\"\n");
		}
	}
	if (linecount) strob_sprintf(convline, 1, "\"\n");
	return;
}

static
int
decompress_func(STROB * dst, unsigned char * z, int compressed_len)
{
	int ret;
	lzo_uintp dst_len;

	strob_strcpy(dst, "");
	strob_set_length(dst, compressed_len*40);
	ret = lzo1x_decompress_safe(z, (lzo_uint)compressed_len,
                    (lzo_byte *)strob_str(dst), (lzo_uintp)(&dst_len), NULL);
	return ret;
}

static
int
compress_func(STROB * convline, STROB * plain_text, unsigned char ** cloc, int ** clen)
{
	int r;
	lzo_uint in_len;
	lzo_uint out_len;
	int IN_LEN = strob_strlen(plain_text) + 100;
	int OUT_LEN = (IN_LEN + IN_LEN / 64 + 16 + 3);
	lzo_byte * inbuf = malloc(IN_LEN);
	lzo_byte * outbuf = malloc(OUT_LEN);

/*
 * Step 1: initialize the LZO library
 */
    if (lzo_init() != LZO_E_OK)
    {
        printf("lzo_init() failed !!!\n");
        return -1;
    }
 
/*
 * Step 2: prepare the input block that will get compressed.
 *         We just fill it with zeros in this example program,
 *         but you would use your real-world data here.
 */
    in_len = strob_strlen(plain_text) + 1;
    memcpy(inbuf, (void*)(strob_str(plain_text)), (size_t)in_len);

/*
 * Step 3: compress from `in' to `out' with LZO1X-1
 */
    r = lzo1x_1_compress(inbuf, in_len, outbuf, &out_len, wrkmem);
    if (r == LZO_E_OK) {
	;
	/*
        fprintf(stderr, "compressed %lu bytes into %lu bytes\n",
            (long) in_len, (long) out_len);
	*/
    }
    else
    {
        /* this should NEVER happen */
        printf("internal error - compression failed: %d\n", r);
        return -2;
    }
    /* check for an incompressible block */
    /*
    if (out_len >= in_len)
    {
        printf("This block contains incompressible data.\n");
        return -3;
    }
    */
    if (cloc && clen) {
	    *cloc = malloc(out_len + 1);
	    memcpy(*cloc, outbuf, out_len);
	    /* *clen = (int)out_len; */
    }
    conv_clump(convline, outbuf, (int)out_len);
    free(inbuf);
    free(outbuf);
    return (int)out_len;
}

static
void
write_function_data_header(STROB * funcheader, int compression, int uncompressed_size, int size)
{
	strob_sprintf(funcheader, 0, "\t\"%010d:%010d:COMPRESSION=%d:\",\n", size, uncompressed_size, compression);
}

static
void
write_file_header(int compression_on)
{
	time_t tm = time(NULL);

	fprintf(stdout, "/*\n");
	fprintf(stdout, "This file was automatically generated on %s", ctime(&tm));
	fprintf(stdout, "by gencfile.c version " VERSION ".\n");
	if (compression_on) {
		fprintf(stdout, "Each shell function is compressed using the minilzo compression library.\n");
	} else {
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "*/\n");
	fprintf(stdout, "\n\n");
	fprintf(stdout, "\n\n");
	fprintf(stdout, 
		"#include \"shlib.h\"\n"
		"\n"
		"\n"
		"struct shell_lib_function shell_lib_function_pallet[]={\n"
	);
}

static
void
convert_line(STROB * convline, char * line, int compress_on)
{
	char * s = line;

	if (compress_on) {
		strob_strcat(convline, line);
		return;
	}

	strob_sprintf(convline, 1, "\"");
	while (*s) {
		strob_sprintf(convline, 1, "\\x%02x", (int)(*s));
		s++;
	}
	strob_sprintf(convline, 1, "\"\n");
	return;
}

static
char *
getnextpath(FILE * fp)
{
	char * ret;
	static char buf[1024];

	ret = fgets(buf, sizeof(buf), fp);
	if (!ret) return NULL;
	if (strlen(ret) >= (sizeof(buf) -1)) exit(9);
	buf[sizeof(buf) - 1] = '\0';
	return ret;
}

static 
char *
get_next_line(FILE * flp)
{
	char * path;
	path = getnextpath(flp);
	return path;
}
			
static
void
parse_name(STROB * function_name, char * line)
{
	char * s;
	strob_strcpy(function_name, line);
	s = strob_str(function_name);
	/*
	* assume the name starts at *line
	*/
	while (isalnum((int)(*s)) || *s == '_') s++;
	*s = '\0';
	return;
}
				
static
int
det_strlen(char * buf)
{
	char * s = buf;
	int nq = 0;
	int len = strlen(buf);
	while(*s) {
		if (*s == '"' || *s == '\n') nq++;
		s++;
	}
	return (len - nq)/4;
}

int 
main (int argc, char ** argv ) {
	int retval = 0;
	int ret = 0;
	char * line;
	char * tmp_s;
	int start_func = 0;
	int compress_on = 0;
	int compressed_len = 0;
	int uncompressed_len = 0;
	FILE * flp;
	STROB * function_name = strob_open(10);
	STROB * convline = strob_open(10);
	STROB * shell_function = strob_open(10);
	STROB * text = strob_open(10);
	STROB * function_header = strob_open(10);

	if (argc > 1) {
		/*
		* arg1 may be:
		*	--help
		*	--compression=1
		*	--compression=0
		*/
		if (strcmp(argv[1], "--help") == 0) {
			fprintf(stdout, "Usage: gencfile {--compression=0|--compression=1}\n");
			fprintf(stdout, "Generate a C file to stdout from a shell library read on stdin.\n");
			fprintf(stdout, "For example: ./gencfile <shell_lib.sh\n");
			exit(0);
		}
		tmp_s = strchr(argv[1], (int)'=');
		if (tmp_s) {
			tmp_s++;
			compress_on = atoi(tmp_s);
		}
	}
	write_file_header(compress_on);
	
	flp = stdin;	
	line = get_next_line(flp);
	while (ret == 0 && line) {

		if (start_func == 0 && isalpha(*line) && strstr(line, "()")) {
			start_func = 1;
			/*
			 * Start of a new shell function
			 */
			strob_strcpy(convline, "");
			parse_name(function_name, line);

			fprintf(stdout,	"{\n"
					"	\"%s\",\n", strob_str(function_name));


			strob_strcpy(shell_function, "");
			convert_line(convline, line, compress_on);
		} else if (start_func == 0 && !isalpha(*line) && !strstr(line, "()")) {
			/*
			* comment lines or blank lines.
			*     Do nothing.
			*/
			;
		} else if (start_func && *line == '}') {
			/*
			 * End of shell function.
			 */
			convert_line(convline, line, compress_on);
			if (compress_on == 0) {
				/* Compression OFF */
				strob_strcpy(text, strob_str(convline));
				uncompressed_len = strob_strlen(text);
				compressed_len = strob_strlen(text);
				write_function_data_header(function_header,
							compress_on,
							uncompressed_len,
							compressed_len);
			} else {
				/* Compression ON, do compress */
				/*
				* The whole shell function is in convline.
				* compress it now.
				*/
				strob_strcpy(text, strob_str(convline));
				uncompressed_len = strob_strlen(text) + 1;
				compressed_len = compress_func(convline, text, NULL, NULL);
				if (compressed_len <= 0) {
					retval++;
				}
				write_function_data_header(function_header, compress_on,
							uncompressed_len,
							compressed_len);
			}
			fprintf(stdout, "%s", strob_str(function_header));
			fprintf(stdout, "%s", strob_str(convline));
			fprintf(stdout, ","
					"(char*)NULL\n"
					"}, /* AA */ \n");
			start_func = 0;			
		} else if (start_func && *line != '}') {
			/*
			 * Body of shell function.
			 */	
			convert_line(convline, line, compress_on);
		} else {
			;
		}
		line = get_next_line(flp);
	}

	fprintf(stdout, "{\n"
			"(char*)NULL,\n"
			"(char*)NULL,\n"
			"(char*)NULL,\n"
			"(char*)NULL\n"
			"}\n"
			"};\n"
		);

	exit(retval);
}
