/* etar.h -  A simpler interface to the tar writing routines

   Copyright (C) 2005 Jim Lowe
 
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

#ifndef etar_20050130_h
#define etar_20050130_h

#include "swuser_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "strob.h"
#include "cpiohdr.h"
/* #include "taru.h" */
#include "to_oct.h"

typedef struct {
	int etar_tarheaderflagsM;
	char * tar_hdrM;
	time_t timeM;
} ETAR;

ETAR * etar_open(int tarheaderflags);
void  etar_close(ETAR * itar);
struct tar_header * etar_get_hdr (ETAR * etar);
void etar_init_hdr (ETAR * etar);
int etar_set_pathname(ETAR * etar, char * pathname);
int etar_set_uname(ETAR * etar, char * name);
int etar_set_gname(ETAR * etar, char * name);
void etar_set_chksum(ETAR * tar);
void etar_set_mode_ul(ETAR * etar, unsigned int mode_i);
void etar_set_uid(ETAR * etar, unsigned int val);
void etar_set_gid(ETAR * etar, unsigned int val);
void etar_set_size(ETAR * etar, unsigned int val);
void etar_set_time(ETAR * etar, time_t val);
void etar_set_typeflag(ETAR * etar, int tar_type);
void etar_set_devmajor(ETAR * etar, unsigned long devno);
void etar_set_devminor(ETAR * etar, unsigned long devno);
int etar_set_linkname(ETAR * etar, char * name);
int etar_emit_data_from_fd(ETAR * etar, int, int fd);
int etar_emit_data_from_buffer(ETAR * etar, int fd, char * buf, int bufsize);
int etar_set_size_from_buffer(ETAR * etar, char * buf, int bufsize);
int etar_set_size_from_fd(ETAR * etar, int fd, int * newfd);
int etar_write_trailer_blocks(ETAR * etar, int ofd, int nblocks);
int etar_emit_header(ETAR * etar, int fd);

#endif
