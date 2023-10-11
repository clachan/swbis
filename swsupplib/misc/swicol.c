/*  swicol.c -- Write script and data into a shell's stdin.
 */

/*
  Copyright (C) 2004,2005,2006,2007,2010 Jim Lowe
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


#define FILENEEDDEBUG 1 /* controls the E_DEBUG macros */
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swheaderline.h"
#include "swheader.h"
#include "ugetopt_help.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "atomicio.h"
#include "swi.h"
#include "swgp.h"
#include "swevents.h"
#include "swlib.h"
#include "swicol.h"
#include "swutilname.h"

#include "debug_config.h"
#ifdef SWICOLNEEDDEBUG
#define SWICOL_E_DEBUG(format) SWBISERROR("SWI DEBUG: ", format)
#define SWICOL_E_DEBUG2(format, arg) SWBISERROR2("SWI DEBUG: ", format, arg)
#define SWICOL_E_DEBUG3(format, arg, arg1) \
			SWBISERROR3("SWI DEBUG: ", format, arg, arg1)
#else
#define SWICOL_E_DEBUG(arg)
#define SWICOL_E_DEBUG2(arg, arg1)
#define SWICOL_E_DEBUG3(arg, arg1, arg2)
#endif 

#define SWICOL_STOP_STRING "305:"
#define SWICOL_START_STRING "304:"
#define SWICOL_CTS_STRING "318:"

static
char * 
subshell_marks(STROB * subsh, char * type, int wh, 
			int nhops, int verbose_level, char * group_delim)
{
	int is_subshell;

	is_subshell = (*group_delim == '(');

	strob_strcpy(subsh, "");
	if (wh == (int)'L') {
		/*
		 * Left
		 */
		strob_sprintf(subsh, STROB_DO_APPEND, "%s\n", group_delim);
	} else if (wh == (int)'R') {
		/*
		 * Right
		 */
		strob_sprintf(subsh, STROB_DO_APPEND, "exit $sw_retval;\n%s", group_delim+2);
		if (is_subshell && strcmp(type, "source") == 0) {
			/*
			* Source
			*/
			if (nhops >= 1) {
				if (verbose_level == 0) {
					strob_strcat(subsh, 
						"0</dev/null 2>/dev/null");
				} else {
					strob_strcat(subsh, "0</dev/null");
				}
			} else {
				if (verbose_level == 0) {
					strob_strcat(subsh, "2>/dev/null");
				} else {
					strob_strcat(subsh, "0</dev/null");
				}
			}
		} else if (is_subshell && strcmp(type, "target") == 0) {
			/*
			* Target
			*/
			if (nhops >= 1) {
				if (verbose_level == 0) {
					strob_strcat(subsh, 
						"1>/dev/null 2>/dev/null");
				} else {
					strob_strcat(subsh, "1>/dev/null");
				}
			} else {
				if (verbose_level == 0) {
					strob_strcat(subsh, 
						"1>/dev/null 2>/dev/null");
				} else {
					strob_strcat(subsh, "1>/dev/null");
				}
			}
		} else if (is_subshell && strcmp(type, "install_target") == 0) {
			if (nhops >= 1) {
				if (verbose_level == 0) {
					strob_strcat(subsh, "");
				} else {
					strob_strcat(subsh, "");
				}
			} else {
				if (verbose_level == 0) {
					strob_strcat(subsh, 
						"2>/dev/null");
				} else {
					strob_strcat(subsh, "");
				}
			}
		} else {
			if (is_subshell) {
				fprintf(stderr, "internal error in swicol_task_wait at line %d\n", __LINE__);
				exit(1);
			}
		}
		if (is_subshell) {
			strob_strcat(subsh, "; exit $?");
		} else {
			strob_strcat(subsh, "");
		}
	} else {
		/* error */
		;
	}
	return strob_str(subsh);
}

static
void
init_magic_header(SWICOL * swicol)
{
	int i;
	STROB * tmp;
	/* This makes a string 513 bytes long */
	tmp = strob_open(10);
	for (i=0; i<9; i++)
		strob_sprintf(tmp, 1, "%s\n", "# # # # # # # # # # # # # # # # # # # # # # # # # # # ##");
	swicol->magic_headerM = strob_release(tmp);
	if (strlen(swicol->magic_headerM) < 513) exit(42);
}


/**
 * task_wait_for - wait for task
 *
 * Returns 0 on success, Non-zero on error
 */

static
int
task_wait_for(SWICOL * swicol, STROB * retbuf, 
			int event_fd, int timelimit, char * stop_string)
{
	int retval = -1;
	int ret = 0;
	int cret;
	int do_stop;
	int readReturn;
	STROB * buf = strob_open(180);
	char * s;
	int alarm_monitor;
	time_t start = time(NULL);
	time_t alarm_start = time(NULL);
	time_t now = start;
	struct timespec req;
              
	req.tv_sec = 0;  	/* seconds */
	req.tv_nsec = 297000;  /* 1/30th second in nanoseconds */

	E_DEBUG2("Entering: %s", stop_string);	
	if (retbuf) strob_strcpy(retbuf, "");

	alarm_start = (time_t)(0);
	alarm_monitor = 0;
	cret = 0;
	do_stop = 0;
	E_DEBUG("");	
	while(
		(alarm_monitor == 0) &&
		(
			(cret == 0) || 
			(do_stop == 0 && ((int)(now - start) < timelimit))
		)
	     )
	{
		nanosleep(&req, (struct timespec *)(NULL));
		now = time(NULL);

		if (swicol->master_alarmM) {
			/* allow a several second delay before terminating
			   this loop to allow the signals to perculate */
			E_DEBUG("Got Alarm");
			if (alarm_start == (time_t)(0)) {
				E_DEBUG("Set Alarm Start");
				alarm_start = time(NULL);
			}
			if ((now - alarm_start) > 1 /*seconds*/) {
				E_DEBUG("Alarm Expired setting alarm_monitor");
				alarm_monitor = 1;
			}
		}

		ret = swgpReadLine(buf, event_fd, &readReturn);

		if (cret == 0) {
			cret = ret;
		}
	
		E_DEBUG2("read buf: [%s]", strob_str(buf));	
		s = strstr(strob_str(buf), stop_string);
		if (s) {
			/*
			 * Got the terminating event.
			 * The terminating event is 305 "SWI_TASK_ENDS"
			 */

			if (
				strlen(s) == 6 /* e.g. "305:0\n" */ &&
				*(s + 5) == '\n' 
			) {
				/*
				 * Normal stop
				 */
				E_DEBUG("Normal stop");
				retval = 0;
				do_stop = 1;
			} else if (
				strstr(s, SWICOL_START_STRING) &&
				*(s + strlen(s) - 1) == '\n'
			) {
				/*
				 * Normal stop
				 */
				E_DEBUG("Got Start");
				retval = 0;
				do_stop = 1;
			} else if (strlen(s) > 8 && strstr(s, SWICOL_STOP_STRING)) {
				/*
				 * error
				 */
				E_DEBUG("");	
				retval = 1;
				do_stop = 1;
				fprintf(stderr, "internal error in swicol_task_wait at line %d\n", __LINE__);
				fprintf(stderr, "expect string [%s]\n", s);
			} else {
				/*
				* OK
				* partial read on stop event
				*/
				E_DEBUG("partial read");	
				;	
			}
		} else {
			/*
			 * OK
			 */
			E_DEBUG("nothing");	
			;
		}
		if (ret && retbuf) 
			strob_strcat(retbuf, strob_str(buf));
	}
	if (do_stop == 0 && swicol->master_alarmM == 0) {
		/*
		 * time limit exceeded.
		 */
		E_DEBUG("time limit exceeded");	
		retval = 2;
		swutil_doif_writef(swicol->verbose_levelM, 1, swicol->logspecM,
				STDERR_FILENO,
				"SW_RESOURCE_ERROR: time limit of %d seconds exceeded\n",
							timelimit);

	} else if (alarm_monitor || swicol->master_alarmM) {
		E_DEBUG("ALARM");	
		retval = 2;
		swutil_doif_writef(swicol->verbose_levelM, 1, swicol->logspecM,
				STDERR_FILENO, "SW_ABORT_SIGNAL_RECEIVED: user interrupt\n");
	}

	strob_close(buf);
	E_DEBUG2("Leaving: %d", retval);	
	return retval;
}

static
void
form_debug_task_filename(char * buf)
{
	char * s;
	s = buf;

	while (*s) {
		if (*s == ' ') *s = '_';
		if (*s == ':') {
			*s = '\0';
			break;
		}
		s++;
	}
}

static
int
set_selected_index(SWICOL * swicol, int first_current_event)
{
	if (first_current_event < 0) 
		return swicol->event_indexM;
	else
		return first_current_event;
}
	
static
void
set_buf(char ** dst, char * buf)
{
	if (*dst) free(*dst);
	*dst = strdup(buf);	
}

static
int
convert_result_to_event_list(char * event_string, STRAR * event_list)
{
	char * s;
	char * current;
	int ret;

	E_DEBUG2("event string: %s", event_string);	

	ret = strar_num_elements(event_list);
	s = strchr(event_string, '\n');
	current = event_string;
	while(s && *s) {
		*s = '\0';
		E_DEBUG2("adding event to event list: %s", current);	
		strar_add(event_list, current);	
		s++;
		current = s;
		s = strchr(s, '\n');
	}

	/*
	 * Add a newline in the list as a separator
	 */
	strar_add(event_list, "\n");	

	/*
	 * return the index of these current events
	 */
	return ret;
}

static
int
print_task_script(SWICOL * swicol, STROB * command, uintmax_t size, char * dir, char * task_script, char * task_desc)
{
	char * verbosestring;
	char * tonullstring;
	char * ablocks;
	STROB * enddesc;
	STROB * ubuf;
	STROB * synct_eof_buf;
	STROB * synct_eof_buf2;
	uintmax_t blocks;
	
	ubuf = strob_open(32);
	enddesc = strob_open(32);
	synct_eof_buf = strob_open(32);
	synct_eof_buf2 = strob_open(32);
	blocks = (int)(size / 512);
	if (size % 512) {
		/*
		 * size must be a whole number of 512 blocks.
		 */
		blocks ++;
		fprintf(stderr, "Internal error: in swicol.c:print_task_script\n");
	}

	/*
	 * make sure directory is not tainted and is atleast 1 char long.
	 */

	if (swlib_is_ascii_noaccept(dir, SWBIS_TAINTED_CHARS "\a\b\n\r\t \v\\", 1)) {
		fprintf(stderr, "print_task_script: illegal dir: %s\n", dir);
		return 1;
	}

	if (swicol->verbose_levelM > SWC_VERBOSE_8) {
		verbosestring = "set -vx";
	} else {
		verbosestring = "#set -vx";
	}
	
	if (swicol->verbose_levelM >= SWC_VERBOSE_5) {
		tonullstring = " ";
	} else {
		tonullstring = "2>/dev/null";
	}

	if (swicol->needs_synct_eoaM) {
		strob_sprintf(synct_eof_buf, 0,
		"(\n"
		"(\n"
		);
		swlib_append_synct_eof(synct_eof_buf2);
	}

	strob_sprintf(enddesc, 0, "%s: " SWEVENT_STATUS_PFX "$sw_retval", task_desc);

	ablocks = swlib_umaxtostr(blocks, ubuf);

	strob_sprintf(command, 0,
		"{\n"
		"dd bs=512 count=%s %s | (\n"
		"# trap true 1 2 13 14 15\n"
		CSHID
		"%s\n"			/* SWI_TASK_BEGINS */
		"swxdir=\"%s\"\n"
		"cd \"$swxdir\"\n"
		"swret=$?; export swret\n"
		"%s\n"
		"	case $swret in\n"
		"		0)\n"
		" 			;;\n"
		"		*)\n"
		"			%s\n"	
		"			dd count=%s of=/dev/null 2>/dev/null\n"
		"			%s\n"	
		"			%s\n"
		"			exit \"$swret\"\n"
		"			;;\n"
		"	esac\n"
		"%s"		/* Task main script */
		"case \"$sw_retval\" in\n"
		"	\"\") echo \"%s:\" Warning: sw_retval is not set for task script: %s 1>&2\n"
		"		sw_retval=0\n"
		"		;;\n"
		"esac\n"
		"exit \"$sw_retval\"\n"
		"); sw_retval=$?\n"
		"%s\n"			/* SWI_TASK_ENDS */
		"exit $sw_retval\n"	/* exit the task shell */
		"}\n"
		,
		ablocks,
		tonullstring,
		TEVENT(2, -1, SWI_TASK_BEGINS, task_desc),
		dir,
		verbosestring,
		strob_str(synct_eof_buf),
		ablocks,
		strob_str(synct_eof_buf2),
		TEVENT(2, -1, SW_INTERNAL_ERROR, "No such directory or no access: $swxdir"),
		task_script,
		swlib_utilname_get(),
		task_desc,
		TEVENT(2, -1, SWI_TASK_ENDS, strob_str(enddesc))
		);

	strob_close(synct_eof_buf);
	strob_close(synct_eof_buf2);
	strob_close(ubuf);
	strob_close(enddesc);
	return 0;
}

SWICOL *
swicol_create(void)
{
	SWICOL * swicol;
	swicol = (SWICOL*)malloc(sizeof(SWICOL));
	if (swicol == NULL) return NULL;
	swicol->tmpM = strob_open(10);
	swicol->scriptM = strob_open(10);
	swicol->logspecM = (struct sw_logspec *)(NULL);
	swicol->verbose_levelM = 0;
	swicol->event_listM = strar_open();
	swicol->umaskM = NULL;
	swicol->blocksizeM = NULL;
	swicol->setvxM = NULL;
	swicol->delaytimeM = 0;
	swicol->targetpathM = (char*)NULL;
	swicol->nhopsM = 1;
	swicol->event_indexM = 1;
	swicol->id_stringM = strob_open(24);
	swicol->debug_task_scriptsM = 0;
	swicol->needs_synct_eoaM = 0;
	init_magic_header(swicol);
	swicol_clear_master_alarm(swicol);
	swicol_set_event_fd(swicol, -1);
	return swicol;
}

void
swicol_delete(SWICOL * swicol)
{
	strob_close(swicol->tmpM);
	strob_close(swicol->scriptM);
	strob_close(swicol->id_stringM);
	free(swicol);
}

void
swicol_set_event_fd(SWICOL * swicol, int fd)
{
	swicol->event_fdM = fd;
}

void
swicol_set_delaytime(SWICOL * swicol, int delay)
{
	swicol->delaytimeM = delay;
}

void
swicol_set_nhops(SWICOL * swicol, int nhops)
{
	swicol->nhopsM = nhops;
}

void
swicol_set_verbose_level(SWICOL * swicol, int level)
{
	swicol->verbose_levelM = level;
}

void
swicol_set_umask(SWICOL * swicol, char * buf)
{
	set_buf(&(swicol->umaskM), buf);
}

void
swicol_set_setvx(SWICOL * swicol, char * buf)
{
	set_buf(&(swicol->setvxM), buf);
}

void
swicol_set_targetpath(SWICOL * swicol, char * buf)
{
	set_buf(&(swicol->targetpathM), buf);
}

char *
swicol_get_umask(SWICOL * swicol)
{
	return swicol->umaskM;
}

char *
swicol_get_setvxk(SWICOL * swicol)
{
	return swicol->setvxM;
}

char *
swicol_get_blocksize(SWICOL * swicol)
{
	return swicol->blocksizeM;
}

/**
 * swicol_rpsh_task_send_script2 - send script to shell stdin
 *
 * Return 0 on success, -1 on error
 */

int
swicol_rpsh_task_send_script2(SWICOL * swicol, int fd,
		uintmax_t data_size, char * dir, char * script, char * f_desc)
{
	STROB * tmp;
	int ret;
	char * desc;

	desc = f_desc;	
	if (swicol_get_master_alarm_status(swicol) == SWICOL_ABORT_STEP_2) {
		return -1;
		return 0;
	} else if (swicol_get_master_alarm_status(swicol) == SWICOL_ABORT_STEP_1) {
		/* do the task script but intensionally send a wrong task
		   id_string, this will cause the target main script to abort */
		swicol->master_alarmM = SWICOL_ABORT_STEP_2;
		desc =  SWICOL_ALARM_EVENT;
	}

	tmp = swicol->scriptM;
	strob_strcpy(tmp, "");

	strob_sprintf(tmp, 1, "%s", swicol->magic_headerM);
	strob_sprintf(tmp, 1, "# SWI_TASK: %s\n", desc);
	strob_sprintf(tmp, 1, "\n");
	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
		fd, (void*)(strob_str(tmp)), (size_t)(strob_strlen(tmp)));
	if (ret != (int)strob_strlen(tmp)) {
		return -1;
	}
	E_DEBUG("");
	swicol_set_task_idstring(swicol, f_desc);
	ret = swicol_rpsh_task_send_script(swicol, fd, data_size, dir, script, desc);
	E_DEBUG("");

	if (swicol->debug_task_scriptsM) {
		E_DEBUG("");
		swicol_write_debug_task_script(swicol, strob_str(swicol->scriptM));
	}

	if (swicol->event_fdM >= 0)
		swicol_rpsh_wait_304(swicol, swicol->event_fdM);

	E_DEBUG("");
	if (swicol_get_master_alarm_status(swicol) != 0) {
		if (ret == 0)
			return -1;
	}
	return ret;
}

/**
 * swicol_rpsh_task_send_script - send script to shell stdin
 *
 * Return 0 on success, -1 on error
 */
int
swicol_rpsh_task_send_script(SWICOL * swicol, int fd,
		uintmax_t data_size, char * dir, char * script, char * desc)
{
	int ret;
	int eret;
	STROB * tmp;
	
	tmp = swicol->scriptM;
	strob_strcpy(tmp, "");

	if (data_size == 0) {
		fprintf(stderr, "%s: Error: a non-zero length data payload is required.\n",
				swlib_utilname_get());
	}

	if (print_task_script(swicol, tmp, data_size, dir, script, desc)) {
		fprintf(stderr, "swicol_rpsh_task_send_script: error forming task script\n");
		return -2;
	}
	
	E_DEBUG2("\n<AA>\n%s</AA>\n", strob_str(tmp));

	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
		fd, (void*)(strob_str(tmp)), (size_t)(strob_strlen(tmp)));

	eret = strob_strlen(tmp);
	if (ret == eret) {
		return 0;
	} else {
		fprintf(stderr, "rpsh_task_send_script: error writing task script to fd=%d: %s\n", fd, strerror(errno));
		return -1;
	}
}

int
swicol_rpsh_wait_cts(SWICOL * swicol, int event_fd)
{
	int ret = 0;
	int first_current_event;
	STROB * buf;
	char * stop_string = SWICOL_CTS_STRING;
	
	buf = strob_open(32);
	ret = task_wait_for(swicol, buf, event_fd, SWICOL_TL_20, stop_string);
	first_current_event = convert_result_to_event_list(strob_str(buf), swicol->event_listM);
	swicol->event_indexM = first_current_event;
	strob_close(buf);
	return ret;
}

int
swicol_rpsh_wait_304(SWICOL * swicol, int event_fd)
{
	int ret = 0;
	char * stop_string = SWICOL_START_STRING;
	ret = task_wait_for(swicol, (STROB*)NULL, event_fd, SWICOL_TL_9, stop_string);
	return ret;
}

int
swicol_rpsh_task_wait(SWICOL * swicol, STROB * retbuf, 
			int event_fd, int timelimit)
{
	int ret = 0;
	char * stop_string = SWICOL_STOP_STRING;   /* SWI_TASK_ENDS */
	ret = task_wait_for(swicol, retbuf, event_fd, timelimit, stop_string);
	return ret;
}

int
swicol_rpsh_wait_for_event(SWICOL * swicol, STROB * retbuf, int event_fd, int event)
{
	int ret;
	STROB * buf;
	buf = strob_open(32);
	strob_sprintf(buf, 0, "%d:", event);
	ret = task_wait_for(swicol, (STROB*)NULL, event_fd, SWICOL_TL_10, strob_str(buf));
	strob_close(buf);
	return ret;
}

char *
swicol_rpsh_get_event_message(SWICOL * swicol, int event_value,
		int first_current_event, int * p_index)
{
	char * retval = (char*)NULL;
	STROB * tmp = strob_open(10);
	STRAR * event_list;
	char * s;
	int ix;

	ix = set_selected_index(swicol, first_current_event);
	event_list = swicol->event_listM;

	strob_sprintf(tmp, 0, "%d:", event_value);

	E_DEBUG2("event value=[%s]", strob_str(tmp));

	s = strar_get(event_list, ix++);
	while(s) {
		if (strstr(s, strob_str(tmp))) {
			s = strchr(s, ':');
			if (!s) return NULL;
			s++;
			retval = s;
			break;
		}
		s = strar_get(event_list, ix++);
	}
	strob_close(tmp);
	if (p_index) *p_index = --ix;
	E_DEBUG2("returning message: <<<<<<<< [%s]", retval);
	return retval;
}

int
swicol_rpsh_get_event_status(SWICOL * swicol, char * msg,
		int event_value, int first_current_event, int * p_index)
{
	int retval;
	int atoi_ret;

	if (!msg)
		msg = swicol_rpsh_get_event_message(swicol, event_value, first_current_event, p_index);

	/*
	 * the message is the status
	 */
	if (!msg) return -1;
	E_DEBUG2("Message [%s]", msg);	
	
	if (isdigit((int)(*msg))) {
		retval = swlib_atoi(msg, &atoi_ret);
		if (atoi_ret) {
			retval = SW_INTERNAL_ERROR_127;
		}
	} else if (strcmp(msg, SWEVENT_STATUS_PFX SWEVENT_VALUE_PREVIEW) == 0) {
		/* status=preview */
		/* special status for preview mode in swconfig */
		retval = 0;
	} else {
		fprintf(stderr,
			"%s: event %d did not return a valid status\n",
			swlib_utilname_get(), event_value);
		retval = SW_INTERNAL_ERROR_128;
	}
	return retval;
}

/**
 * swicol_rpsh_task_expect - wait for a task and read exit status
 *
 * Return task exit status, or -1 on internal error
 */

int
swicol_rpsh_task_expect(SWICOL * swicol, int event_fd, int timelimit)
{
	char * ev;
	int ret;
	int retval = 0;
	STROB * buf = strob_open(10);
	STRAR * event_list = swicol->event_listM;
	int first_current_event;

	swicol->event_indexM = -1;
	/*
	 * wait for the SWI_TASK_ENDS event
	 */
	ret = swicol_rpsh_task_wait(swicol, buf, event_fd, timelimit);
	/* fprintf(stderr, "swicol_rpsh_task_wait returned [%d]\n", ret); */
	if (ret)
		retval = -4;
	
	E_DEBUG2("expect buffer=[%s]", strob_str(buf));

	/*
	 * Assert a user expectation for a single event.
	 */
	if ((ev=strstr(strob_str(buf), "305:")) == NULL) {
		/*
		* SWI_TASK_ENDS event not found.
		*/
		fprintf(stderr, "%s: error: END event not received for task: %s\n",
			swlib_utilname_get(), strob_str(swicol->id_stringM));
		if (retval == 0) retval = -1;
	} else {
		ev += 4;
		if (isdigit((int)(*ev)) == 0) {
			fprintf(stderr, "%s: error: END event has invalid status: %s status=%s\n",
				swlib_utilname_get(), strob_str(swicol->id_stringM), strob_str(buf));
			retval = -2;
		} else {
			retval = swlib_atoi(ev, NULL);
		}
	}

	/*
	 * Store all the events.
	 */ 
	first_current_event = convert_result_to_event_list(strob_str(buf), event_list);

	/*
	 *  Print the events if the verbose level is high enough
	 */
	if (swicol->verbose_levelM >= SWC_VERBOSE_8) {
		/*
		fprintf(stderr, "%s\n", strob_str(buf));
		*/
		swicol_show_events_to_fd(swicol, STDERR_FILENO, first_current_event);
	}

	swicol->event_indexM = first_current_event;
	strob_close(buf);
	return retval;
}

int
swicol_show_events_to_fd(SWICOL * swicol, int fd, int first_current_event)
{
	int ret;
	size_t len;
	int ix;

	if (first_current_event < 0) 
		ix = swicol->event_indexM;
	else
		ix = first_current_event;
	swicol_print_events(swicol, swicol->tmpM, ix);
	len = strob_strlen(swicol->tmpM);
	ret = atomicio(
			((ssize_t (*)(int, void *, size_t))write),
			fd, 
			(void*)(strob_str(swicol->tmpM)),
			len
		);	
	return !(ret == (int)len);
}

void
swicol_print_event(SWICOL * swicol, STROB * buf, char * ev)
{		
	strob_sprintf(buf, STROB_DO_APPEND,  "%s: swicol: %s\n", swlib_utilname_get(), ev);
}

void
swicol_print_events(SWICOL * swicol, STROB * buf, int index)
{
	char * ev;
	int i;

	i = set_selected_index(swicol, index);
	
	strob_strcpy(buf, "");
	strob_sprintf(buf, STROB_DO_APPEND,  "%s: swicol: event stack start\n", swlib_utilname_get());
	while(
		(ev=strar_get(swicol->event_listM, i++)) &&
		(
			1
			/*
			(strcmp(ev, "\n") && index >=0) ||
			(index <= 0)
			*/
		)
	) {

		if (strcmp(ev, "\n") == 0) {
			swicol_print_event(swicol, buf, "");
		} else {
			swicol_print_event(swicol, buf, ev);
		}

		if (strcmp(ev, "\n") == 0 && index < 0) {
			break;
		}
	}
	strob_sprintf(buf, STROB_DO_APPEND,  "%s: swicol: event stack end\n", swlib_utilname_get());
}

char * 
swicol_subshell_marks(STROB * subsh, char * type, int wh, 
			int nhops, int verbose_level)
{
	return subshell_marks(subsh, type, wh, nhops, verbose_level, "(\x00)");
}

char * 
swicol_brace_marks(STROB * subsh, char * type, int wh, int nhops, int verbose_level)
{
	return subshell_marks(subsh, type, wh, nhops, verbose_level, "{\x00}");
}

int
swicol_initiate_fall_thru(SWICOL * swicol)
{
	return 0;
}

void
swicol_clear_task_idstring(SWICOL * swicol)
{
	swicol_set_task_idstring(swicol, "__unset__");
}

void
swicol_set_task_debug(SWICOL * swicol, int do_debug)
{
	swicol->debug_task_scriptsM = do_debug;
}

void
swicol_set_task_idstring(SWICOL * swicol, char * id)
{
	strob_strcpy(swicol->id_stringM, id);
}

char *
swicol_get_task_idstring(SWICOL * swicol)
{
	return strob_str(swicol->id_stringM);
}

void
swicol_write_debug_task_script(SWICOL * swicol, char * script)
{
	int ret;
	STROB * name;
	name = strob_open(32);
	strob_strcpy(name, SWICOL_DEBUG_TASK_SCRIPT_PREFIX );
	strob_strcat(name, strob_str(swicol->id_stringM));
	form_debug_task_filename(strob_str(name));
	ret = swlib_tee_to_file(strob_str(name), -1, script, -1, 0 /*do_append*/);
	E_DEBUG3("name=[%s] ret=%d", strob_str(name), ret);
	strob_close(name);
}

void
swicol_set_master_alarm(SWICOL * swicol)
{
	swicol->master_alarmM = SWICOL_ABORT_STEP_1;
}

void
swicol_clear_master_alarm(SWICOL * swicol)
{
	swicol->master_alarmM = 0;
}

int
swicol_get_master_alarm_status(SWICOL * swicol)
{
	return swicol->master_alarmM;
}

int
swicol_send_loop_trailer(SWICOL * swicol, int fd)
{
	int ret;
	char * send = SWICOL_TRAILER "\n";
	E_DEBUG("");
	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
			fd, send, strlen(send));
	if (ret == (int)strlen(send)) {
		E_DEBUG("OK");
		return 0;
	} else {
		E_DEBUG("error");
		return -1;
	}
}
