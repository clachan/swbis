/* taruib.h - verify archive digests
 
   Copyright (C) 2000  James H. Lowe, Jr.

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

#ifndef taruib_h_0930
#define taruib_h_0930

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TARUIB_N_OTHER    		-1
#define TARUIB_N_MD5    		0
#define TARUIB_N_ADJUNCT_MD5 		1
#define TARUIB_N_SIG	 		2
#define TARUIB_N_SHA1	   		3
#define TARUIB_N_SIG_HDR		4
#define TARUIB_N_SIZE	   		5
#define TARUIB_N_SHA512	   		6
#define TARUIB_N_MAX_INDEX   		6

#define TARUIB_TEXIT_512		11  /* signature is 512 bytes long */
#define TARUIB_TEXIT_1024		13  /* signature is 1024 bytes long */
#define TARUIB_TEXIT_NOT_SIGNED		10
#define TARUIB_TEXIT_ERROR_CONDITION	120
#define TARUIB_TEXIT_ERROR_CONDITION_0	120
#define TARUIB_TEXIT_ERROR_CONDITION_1	121
#define TARUIB_TEXIT_ERROR_CONDITION_2	122
#define TARUIB_TEXIT_ERROR_CONDITION_3	123
#define TARUIB_TEXIT_ERROR_CONDITION_4	124

int taruib_get_fd(void);
void taruib_set_fd(int fd);
char * taruib_get_buffer(void);
int taruib_get_datalen(void);
int taruib_get_bufferlen(void);
void taruib_set_datalen(int n);
int taruib_clear_buffer(void);
void taruib_unread(int n);
void taruib_set_overflow_release(int i);
int taruib_get_overflow_release(void);
int taruib_get_reserve(void);
int taruib_get_nominal_reserve(void);
int taruib_write_catalog_stream(void * XFORMATpackage, int ofd, int version, int verbose);
int taruib_write_storage_stream(void * XFORMATpackage, int ofd, int version, int ofd2, int verbose, int digest_type);
int taruib_write_pass_files(void * XFORMATpackage, int ofd, int adjunct_ofd_p);
int taruib_write_signedfile_if(void * vp, int ofd, char * sigfile, int verbose, int whichsig, int logger_fd);
void taruib_initialize_pass_thru_buffer(void);
int taruib_arfcopy(void * xpackage, 
		void * xswpath, 
		int ofd, 
		char * leadingpath, 
		int do_preview, 
		uintmax_t *,
		int * deadman,
		void (*)(int));
int taruib_arfinstall(void * swi, void * xpackage, 
		void * xswpath, 
		int ofd, 
		char * leadingpath, 
		int do_preview, 
		uintmax_t *,
		int * deadman,
		void (*)(int), /*STRAR * */ void * selections);
#endif
