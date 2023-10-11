/*  swutillib.c -- The top-level common routines for the sw<*> utilities.  */
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "swlib.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "swi.h"
#include "progressmeter.h"
/* #include "swevents_array.h" */
#include "swevents.h"
#include "swicol.h"
#include "swutillib.h"

extern void main_sig_handler(int signum);  /* provided by the main program of each utility */
extern void safe_sig_handler(int signum);  /* provided by the main program of each utility */

static
int
printlogline(STROB * buf, char * sptr)
{
	time_t tm;
	char result[64];
	char * s;
	time(&tm);
	/*ctime_r(&tm, result); */
        strncpy(result, asctime(localtime(&tm)), sizeof(result) - 1);
        result[sizeof(result)-1] = '\0';
	s = result;
	s = strchr(result, ' ');
	if (s) {
		s++; 
	} else {
		s = result;
	}
	swlib_squash_trailing_vnewline(result);
	strob_sprintf(buf, 0, "%s [%d] %s",
		s, (int)getpid(), sptr);
	return 0;
}

SWLOG *
swutil_open(void)
{
	SWLOG * swutil = malloc(sizeof (SWLOG));
	if (!swutil) return NULL;
	swutil->swu_efdM = STDERR_FILENO;
	swutil->swu_logspecM = NULL;
	swutil->swu_verboseM = 1;
	swutil->swu_fail_loudlyM = 0;
	return swutil;
}

void
swutil_close(SWLOG* swutil)
{
	free(swutil);
}

void
swutil_set_stderr_fd(SWLOG * swutil, int fd)
{
	swutil->swu_efdM = fd;
}

int
doif_i_writef(int verbose_level, int write_at_level, 
			struct sw_logspec * logspec, int fd, char * format, va_list * pap)
{
	STROB * buffer;
	int newret;
	int level;

	buffer = strob_open(100);

	level = verbose_level;
	if (write_at_level >= 0 && level < write_at_level) {
		;
		newret = 0;
	} else {
		strob_strcpy(buffer, "");
		if (write_at_level >= SWC_VERBOSE_7 ) {
			strob_strcpy(buffer, "debug> ");
			strob_sprintf(buffer, 1, "%s", swlib_utilname_get());
			strob_sprintf(buffer, 1, "[%d]: ", (int)getpid());
		} else {
			strob_sprintf(buffer, 1, "%s: ", swlib_utilname_get());
		}
		newret = swlib_doif_writeap(fd, buffer, format, pap);
	}

	if (logspec && logspec->logfdM > 0) {
		if (swlib_do_log_is_true(logspec, verbose_level, write_at_level)) {
			/*
			* The only lines logged here are lines that
			* are written to STDERR_FILENO.  If fd != STDERR_FILENO
			* then it is assumed the line will be logged by the 
			* logger process forked by swc_fork_logger.
			*/
			if (fd == STDERR_FILENO || fd == STDOUT_FILENO || fd < 0) {
				STROB * logbuf = strob_open(100);
				strob_strcpy(buffer, "");
				if (write_at_level >= SWC_VERBOSE_7) {
					strob_strcpy(buffer, "debug> ");
					strob_sprintf(buffer, 1, "%s", swlib_utilname_get());
					strob_sprintf(buffer, 1, "[%d]: ", (int)getpid());
				} else {
					strob_sprintf(buffer, 1, "%s: ", swlib_utilname_get());
				}
				printlogline(logbuf, strob_str(buffer));
				newret = swlib_doif_writeap(logspec->logfdM, logbuf, format, pap);
				strob_close(logbuf);
			}
		} else {
			newret = 0;
		}
	}
	strob_close(buffer);
	return newret;
}

XFORMAT *
swutil_setup_xformat(SWLOG * swutil,
		XFORMAT * xformat,
		int source_fd0,
		char * source_path,
		struct extendedOptions * opta,
		int is_seekable,
		int g_verboseG,
		struct sw_logspec * g_logspec,
		int uinfile_open_flags
	)
{
	XFORMAT * ret;
	int open_error = 0;
	int flags;

	flags = uinfile_open_flags;
	
	E_DEBUG("");
	if (is_seekable) {
		E_DEBUG("");
		flags |= UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		swlib_doif_writef(g_verboseG, SWC_VERBOSE_7, 
			g_logspec, swutil->swu_efdM, "using dynamic mem\n");
	} else {
		E_DEBUG("");
		swlib_doif_writef(g_verboseG, SWC_VERBOSE_7, 
			g_logspec, swutil->swu_efdM, "not using dynamic mem\n");
		flags &= ~UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
	}

	if (
		opta && 
		( 
			swextopt_is_option_true(SW_E_swbis_allow_rpm, opta) || 
			swextopt_is_option_true(SW_E_swbis_any_format, opta)
		)
	) {
	
		E_DEBUG("turning on UINFILE_DETECT_UNRPM");
		flags |= UINFILE_DETECT_UNRPM;
	}

	ret = xformat;

	/*
	 *
	 * Open the source archive.
	 *
	 */
	E_DEBUG2("source_path is [%s]", source_path);
	if (
		source_fd0 >= 0 &&
		(source_path == NULL || uinfile_open_flags & UINFILE_DETECT_IEEE) && 
		open_error == 0)
	{
		/*
		 * Open the archive on fd
		 */
		E_DEBUG("open by fd");
		E_DEBUG2("source_path is [%s]", source_path);
		E_DEBUG2("flags=[%d]", flags);
		if (xformat_open_archive_by_fd(xformat, 
						source_fd0, flags, 0)) {
			swlib_doif_writef(g_verboseG, g_logspec->fail_loudlyM, 
				g_logspec, swutil->swu_efdM, "archive open failed\n");
			xformat_close(xformat);
			E_DEBUG("open error");
			ret = (XFORMAT*)NULL;
		}
		E_DEBUG("after xformat_open_archive_by_fd");

	} else if (source_fd0 >= 0 && source_path && open_error == 0) {
		E_DEBUG("~~~~~~~~~~~~~~~ open by fd and name");
		/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
		if (xformat_open_archive_by_fd_and_name(xformat, source_fd0, flags, 0, source_path)) {
			swlib_doif_writef(g_verboseG, g_logspec->fail_loudlyM, 
				g_logspec, swutil->swu_efdM, "archive open failed\n");
			xformat_close(xformat);
			E_DEBUG("open error");
			ret = (XFORMAT*)NULL;
		}
	} else if (source_fd0 < 0 && open_error == 0) {
		/*
		 * Open the archive in the directory.
		 */
		E_DEBUG("open file or directory");
		if (xformat_open_archive_dirfile(xformat, 
					source_path, flags, 0)) {
			swlib_doif_writef(g_verboseG, g_logspec->fail_loudlyM, 
				g_logspec, swutil->swu_efdM, "open failed on %s (by dir)\n", 
					source_path);
			xformat_close(xformat);
			ret = (XFORMAT*)NULL;
		}
	} else {
		/*
		 * Open error.
		 */
		E_DEBUG("open error");
		swlib_doif_writef(g_verboseG, g_logspec->fail_loudlyM, g_logspec, swutil->swu_efdM,
			"%s not found\n", source_path);
		ret = (XFORMAT*)NULL;
	}
	return ret;
}

int
swutil_writelogline(int logfd, char * sptr)
{
	int ret;
	STROB * buf = strob_open(64);
	printlogline(buf, sptr);
	ret = uxfio_unix_safe_write(logfd, strob_str(buf), strob_strlen(buf));
	if (ret != (int)strob_strlen(buf)) {
		SWLIB_EXCEPTION("error writing logfile");
	}
	strob_close(buf);
	return ret;
}

int
swutil_doif_writef2(SWLOG * swutil, int write_at_level, char * format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = doif_i_writef(swutil->swu_verboseM,
			write_at_level, 
			swutil->swu_logspecM,
			swutil->swu_efdM, format, &ap);
	va_end(ap);
	return ret;
}

int
swutil_do_log_is_true(struct sw_logspec * logspec, int verbose_level, int write_at_level)
{
	if (
		(
			logspec->logfdM == 1 && 
			(write_at_level < SWC_VERBOSE_6)
			/*
				Level 6 is the level where the file list
				appears. It is excluded (per spec) for
				loglevel=1
			*/
			
		) ||
		(	logspec->logfdM == 2 &&
			(write_at_level < SWC_VERBOSE_7)
		) ||
		(	logspec->logfdM == 2 &&
			(verbose_level > SWC_VERBOSE_7)
		) 
	) {
		return 1;
	} else {
		return 0;
	}	
}

int
swutil_doif_writef(int verbose_level, int write_at_level, 
			struct sw_logspec * logspec, int fd, char * format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = doif_i_writef(verbose_level,
			write_at_level, 
			logspec,
			fd, format, &ap);
	va_end(ap);
	return ret;
}

int
cpp_doif_i_writef(int verbose_level,
		int write_at_level, 
		struct sw_logspec * logspec,
		int fd,
		char * cpp_FILE,
		int cpp_LINE,
		const char * cpp_FUNCTION,
		char * format, va_list * pap)
{
	int ret;
	STROB * b;
	b = strob_open(100);
	strob_sprintf(b, 0, "%s:%d (%s): %s",
			cpp_FILE,
			cpp_LINE,
			cpp_FUNCTION,
			format);
	ret = doif_i_writef(verbose_level,
			write_at_level, 
			logspec,
			fd,
			strob_str(b),
			pap);
	strob_close(b);
	return ret;
}

int
swutil_cpp_doif_writef(int verbose_level,
		int write_at_level, 
		struct sw_logspec * logspec,
		int fd,
		char * cpp_FILE,
		int cpp_LINE,
		const char * cpp_FUNCTION,
		char * format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = cpp_doif_i_writef(verbose_level,
			write_at_level, 
			logspec,
			fd,
			cpp_FILE,
			cpp_LINE,
			cpp_FUNCTION,
		 	format, &ap);
	va_end(ap);
	return ret;
}
