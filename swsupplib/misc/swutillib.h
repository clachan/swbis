/*  swutillib.h -- Common routines for the sw<*> utilities.  */
/*
   Copyright (C) 2006 Jim Lowe
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

#ifndef swutillib_2006_h
#define swutillib_2006_h

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vplob.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swextopt.h"
#include "swparse.h"
#include "swlib.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swevents.h"
#include "swicol.h"

#define swlib_doif_writef swutil_doif_writef
#define swlib_do_log_is_true swutil_do_log_is_true 

#define SWUTIL_NAME_SWLIST	"swlist"
#define SWUTIL_NAME_SWINSTALL  	"swinstall"
#define SWUTIL_NAME_SWCOPY	"swcopy"
#define SWUTIL_NAME_SWPACKAGE  	"swpackage"
#define SWUTIL_NAME_SWVERIFY  	"swverify"
#define SWUTIL_NAME_SWREMOVE 	"swremove"

struct sw_logspec {
	int loglevelM;
	int logfdM;
	int fail_loudlyM;
};

typedef struct {
	int swu_efdM;
	struct sw_logspec * swu_logspecM;
	int swu_verboseM;
	int swu_fail_loudlyM;
} SWLOG;
	
SWLOG * swutil_open(void);
void swutil_close(SWLOG*);
void swutil_set_stderr_fd(SWLOG*, int fd);

XFORMAT * swutil_setup_xformat(SWLOG * swutil, XFORMAT * xformat, int source_fd0, char * source_path,
		struct extendedOptions * opta, int is_seekable, int g_verboseG, struct sw_logspec * g_logspec,
		int uinfile_open_flags);

int swutil_doif_writef(int verbose_level, int write_at_level, struct sw_logspec * logspec, int fd, char * format, ...);
int swutil_do_log_is_true(struct sw_logspec * logspec, int verbose_level, int write_at_level);
int swutil_doif_writef2(SWLOG * swutil, int write_at_level, char * format, ...);
int swutil_writelogline(int logfd, char * sptr);
int doif_i_writef(int verbose_level, int write_at_level, struct sw_logspec * logspec, int fd, char * format, va_list * pap);
int cpp_doif_i_writef(int verbose_level, int write_at_level, struct sw_logspec * logspec, int fd,
		char * cpp_FILE, int cpp_LINE, const char * cpp_FUNCTION, char * format, va_list * pap);
int swutil_cpp_doif_writef(int verbose_level, int write_at_level, struct sw_logspec * logspec, int fd,
		char * cpp_FILE, int cpp_LINE, const char * cpp_FUNCTION, char * format, ...);
#endif
