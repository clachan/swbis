/* swi_common.c -- Posix package decoding common routines. 

   Copyright (C) 2004 Jim Lowe
   All Rights Reserved.
  
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swheader.h"
#include "swheaderline.h"
#include "ugetopt_help.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "atomicio.h"
#include "ls_list.h"
#include "swevents.h"
#include "glbindex.h"
#include "swi_common.h"

#include "debug_config.h"
#ifdef SWSWINEEDDEBUG
#define SWSWI_E_DEBUG(format) SWBISERROR("SWI DEBUG: ", format)
#define SWSWI_E_DEBUG2(format, arg) SWBISERROR2("SWI DEBUG: ", format, arg)
#define SWSWI_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWI DEBUG: ", format, arg, arg1)
#else
#define SWSWI_E_DEBUG(arg)
#define SWSWI_E_DEBUG2(arg, arg1)
#define SWSWI_E_DEBUG3(arg, arg1, arg2)
#endif 
	
static
int
is_clean_relative_path(char * name)
{
	if (
		swlib_is_sh_tainted_string(name) ||
		strstr(name, "../") ||
		strstr(name, "/..") ||
		*name == '/'
	) {
		return 0; /* error */
	} else {
		return 1; /* OK */
	}
}

static
void
fatal_error(char * msg0, char * msg, int msg2)
{
	fprintf(stderr, "%s : %s : %s line=[%d]\n", 
		swlib_utilname_get(), msg0,  msg, msg2);
	exit(99);
}

/*
char *
swi_com_determine_control_directory(SWHEADER * swheader)
{
	char * value;
	value = swheader_get_single_attribute_value(swheader, SW_A_control_directory);
	if (!value)
		value = swheader_get_single_attribute_value(swheader, SW_A_tag);
	return value;
}
*/

int
swi_com_set_header_index(SWHEADER * header, SWPATH_EX * swpath_ex, int * ai)
{
	int index_offset;
	int retval;

	index_offset = glbindex_find_by_swpath_ex(header, swpath_ex);
	if (index_offset >= 0) {
        	*ai = index_offset;
		retval = 0;
	} else {
		swi_com_internal_error(__FILE__, __LINE__);
		*ai = 0;
		retval = -1;
	}
	return retval;
}

void
swi_com_internal_error(char * file, int line)
{
	fprintf(stderr, "%s: internal error : %s:%d\n", swlib_utilname_get(), file, line);
}

void
swi_com_internal_fatal_error(char * file, int line)
{
	fprintf(stderr, "%s: internal fatal error : %s:%d\n", swlib_utilname_get(), file, line);
	exit(1);
}

void
swi_com_assert_pointer(void * p, char * file, int lineno)
{
	if (!p) fatal_error("fatal null pointer error", file, lineno);
}

void
swi_com_assert_value(int p, char * file, int lineno)
{
	if (!p) fatal_error("fatal zero value error", file, lineno);
}

void
swi_com_fatal_error(char * msg, int msg2)
{
	fprintf(stderr, "%s: fatal error : %s [%d]\n", swlib_utilname_get(), msg, msg2);
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	exit(10);
}

int
swi_com_close_memfd(int fd)
{
	return uxfio_close(fd);
}

char *
swi_com_new_fd_mem(int fd, int * datalen)
{
	char * s;
	char * ret;
	int len;

	s = swi_com_get_fd_mem(fd, &len);
	ret = (char*)malloc(len + 1);
	memcpy(ret, s, len);
	memcpy(ret+len, "\x00", 1);
	if (datalen)
		*datalen = len;
	return ret;
}

char *
swi_com_get_fd_mem(int fd, int * datalen)
{
	unsigned char nullchar = '\0';
	char * s;
	int ret;

	if (uxfio_get_dynamic_buffer(fd, &s, (int*)NULL, datalen) < 0)
		return NULL;
	/* 
	 * Null terminate the data but don't report it as part of the data
	 */
	ret = uxfio_lseek(fd, 0, SEEK_END);
	swi_com_assert_value(ret >= 0, __FILE__, __LINE__);

	ret = uxfio_write(fd, (void *)(&nullchar), 1);
	swi_com_assert_value(ret == 1, __FILE__, __LINE__);

	if (uxfio_get_dynamic_buffer(fd, &s, 
			(int*)NULL, (int*)NULL) < 0) 
				return NULL;
	return s;
}

void
swi_com_do_preview(TARU * taru, struct new_cpio_header * file_hdr,
		char * tar_header_p, int header_len, time_t now)
{
	char ftype;
	int ret;
	int eoa;
	struct stat sb;
	char uname[64];
	char gname[64];

	ret = taru_read_in_tar_header2(taru, file_hdr, -1, /*in_des*/
			tar_header_p, &eoa, 0 /*tarheaderflags*/, TARRECORDSIZE);

	if (ret <= 0) {
		fprintf(stderr, "error from taru_read_in_tar_header2 in do_swi_preview\n");
	}
	taru_filehdr2statbuf(&sb, file_hdr);

	strncpy(uname, tar_header_p+THB_BO_uname, 31);
	uname[31] = '\0';
	strncpy(gname, tar_header_p+THB_BO_gname, 31);
	gname[31] = '\0';

	ftype = taru_get_tar_filetype(sb.st_mode);
	if (ftype >= 0) 
		ls_list(ahsStaticGetTarFilename(file_hdr),
			ahsStaticGetTarLinkname(file_hdr),
			file_hdr, 
			now, 
			stdout,
			uname, /* ahsStaticGetTarUsername(file_hdr), */
			gname, /* ahsStaticGetTarGroupname(file_hdr), */
			ftype,
			LS_LIST_VERBOSE_L1 /* verbose listing */);
	else
		fprintf(stderr, "%s: unrecognized file type in mode [%d] for file: %s\n",
			swlib_utilname_get(), (int)(sb.st_mode), ahsStaticGetTarFilename(file_hdr));
}

void
swi_com_check_clean_relative_path(char * name)
{
	if (is_clean_relative_path(name)) {
		/*
		* Yes.
		*/
		return;
	} else {
		swi_com_assert_value(0, __FILE__, __LINE__);
		_exit(44);
	}
}

void
swiInitListOfObjects(void ** pp)
{
	int i;
	for(i=0;i<SWI_MAX_OBJ;i++) *(pp+i) = (void*)NULL;
}

int
swiGetNumberOfObjects(void ** pp)
{
	int i;
	for(i=0; i < (SWI_MAX_OBJ - 1); i++) {
		if (*(pp+i) == (void*)NULL) {
			return i;
		}
	}
	fprintf(stderr, "too many contained objects, fatal error\n");
	exit(88);
}

int
swiAddObjectToList(void ** pp, void * p)
{
	int i;
	for(i=0; i < (SWI_MAX_OBJ - 1); i++) {
		if (*(pp+i) == (void*)NULL) {
			*(pp+i) = p;
			return 0;
		}
	}
	fprintf(stderr, "too many contained objects, fatal error\n");
	exit(88);
}

void
swi_com_header_manifold_reset(SWHEADER * swheader)
{
	swheader_reset(swheader);
	swheader_set_current_offset_p_value(swheader, 0);
	swheader_goto_next_line(swheader, 
		swheader_get_current_offset_p(swheader),
			SWHEADER_GET_NEXT);
}

int
swi_com_field_edge_detect(char * current, char * previous)
{
	char * c = current;
	char * p = previous;

	if (!p || !c ) {
		swi_com_assert_pointer((void*)NULL, __FILE__, __LINE__);
	}

	if (strlen(p) == 0 && strlen(c)) return 1;
	if (strcmp(p, c) && strlen(c)) return 1;
	return 0;
}

int
swi_com_field_edge_detect_fileset(SWPATH_EX * current, SWPATH_EX * previous)
{
	int ret;
	ret = swi_com_field_edge_detect(current->fileset_control_dir,
		previous->fileset_control_dir);
	if (ret == 0) {
		/*
		* Check for falling pfiles and rising or static fileset.
		*/
		if (strlen(current->pfiles) == 0 && strlen(previous->pfiles)) {
			ret = 1;
		}
	}
	return ret;
}

void
print_header(SWHEADER * swheader)
{
            char * next_attr;
            char * next_line;
            swheader_reset(swheader);
            swheader_set_current_offset_p_value(swheader, 0);

            next_line = swheader_get_next_object(swheader, 
				(int)UCHAR_MAX, (int)UCHAR_MAX);
            while (next_line){
                    swheaderline_write_debug(next_line, STDERR_FILENO);

                    swheader_goto_next_line((void *)swheader, 
				swheader_get_current_offset_p(swheader), 
				SWHEADER_PEEK_NEXT);

                    while((next_attr=swheader_get_next_attribute(swheader)))
                            swheaderline_write_debug(next_attr, STDERR_FILENO);
                    next_line = swheader_get_next_object(swheader, 
					(int)UCHAR_MAX, (int)UCHAR_MAX);
            }
	
}

void
swi_check_clean_relative_path(char * name)
{
	if (is_clean_relative_path(name)) {
		/*
		* Yes.
		*/
		return;
	} else {
		swi_com_assert_value(0, __FILE__, __LINE__);
		_exit(44);
	}
}

int
swi_is_global_index(SWPATH * swpath, char * name)
{
	char * s;
	if (
		(s=strstr(name, "/INDEX")) && *(s+6) == '\0' &&
		strlen(swpath_get_dfiles(swpath)) == 0 &&
		strlen(swpath_get_pfiles(swpath)) == 0 &&
		strlen(swpath_get_product_control_dir(swpath)) == 0
	) {
		return 1;
	} else {
		return 0;
	}
}
