/* swevents.c  - Event creation and reading.
 
   Copyright (C) 2003-2004 James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "swutilname.h"
#include "swlib.h"
#include "strob.h"
#include "vplob.h"
#include "swevents.h"

static char g_redir1[] = "";
static char g_redir2[] = "1>&2";
static char g_redirnull[] = "1>/dev/null";

/*
extern struct swEvents eventsArray[];
static struct swEvents * g_evnt = eventsArray;
*/

static int s_verboseM  = SWC_VERBOSE_4;

static VPLOB * s_arrM = NULL;
static int s_arr_indexM = 0;

#define SWEVENT_LV1	1
#define SWEVENT_LV2	2
#define SWEVENT_LV3	3
#define SWEVENT_LV4	4
#define SWEVENT_LV5	5
#define SWEVENT_LV6	6
#define SWEVENT_LV	SWEVENT_LV3
#define SWI_EVENT	1  /* Is an event that takes place in swi.c */

				/* -SW_ERROR means event is always an error */

/* **********************************
	struct swEvents {
		char * codeM;	     / * POSIX event code	* /
		int valueM;	     / * POSIX event value	* /
		int verbose_threshholdM;
		char * msgM;	     / * Message text	   	* /
		int is_swi_eventM;   / * May occur in a swicol_<*> task* /
		int default_statusM; / * Worst status is default * /
	};
********************************** */


static struct swEvents eventsArray[] = {
{"", 0, 0, "", 0, SW_NOTICE},
{"SW_ILLEGAL_STATE_TRANSITION",		1, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_BAD_SESSION_CONTEXT",		2, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_ILLEGAL_OPTION",			3, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_ACCESS_DENIED", 			4, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_MEMORY_ERROR", 			5,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_RESOURCE_ERROR", 			6,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_INTERNAL_ERROR", 			7,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_IO_ERROR", 			8,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_AGENT_INITIALIZATION_FAILED", 	10,	SWEVENT_LV, "", 0, -SW_ERROR},
{"SW_SERVICE_NOT_AVALIABLE", 		11,	SWEVENT_LV, "", 0, -SW_ERROR},
{"SW_OTHER_SESSIONS_IN_PROGRESS", 	12,	SWEVENT_LV, "", 0, SW_WARNING},
{"SW_SESSION_BEGINS", 			28, 	SWEVENT_LV, "", 0, SW_NOTICE},
{"SW_SESSION_ENDS", 			29, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_CONNECTION_LIMIT_EXCEEDED", 	30,	SWEVENT_LV, "", 0, -SW_ERROR}, 
{"SW_SOC_DOES_NOT_EXIST", 		31, 	SWEVENT_LV, "", 0, -SW_ERROR},
{"SW_SOC_IS_CORRUPT",	 		32,	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_CREATED", 			34, 	SWEVENT_LV, "", 0, SW_NOTICE},
{"SW_CONFLICTING_SESSION_IN_PROGRESS", 	35, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_LOCK_FAILURE",			36, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_IS_READ_ONLY",			37, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_IS_REMOTE", 			38, 	SWEVENT_LV, "", 0, SW_NOTICE},
{"SW_SOC_IS_SERIAL", 			40, 	SWEVENT_LV, "", 0, SW_NOTICE},
{"SW_SOC_INCORRECT_TYPE", 		41, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_CANNOT_OPEN_LOGFILE", 		42, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_AMBIGUOUS_TYPE",		49, 	SWEVENT_LV, "", 0, -SW_NOTICE},
{"SW_ANALYSIS_BEGINS", 			52, 	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_ANALYSIS_ENDS", 			53, 	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_EXREQUISITE_EXCLUDE",		56,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_CHECK_SCRIPT_EXCLUDE",		57,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_CONFIGURE_EXCLUDE", 		58,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_SELECTION_IS_CORRUPT", 		59,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_SOURCE_ACCESS_ERROR", 		60, 	SWEVENT_LV, ": File not found.", 0, SW_ERROR},
{"SW_SELECTION_NOT_FOUND", 		62, 	SWEVENT_LV1, "", 0, SW_ERROR},
{"SW_SELECTION_NOT_FOUND_RELATED", 	63, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SELECTION_NOT_FOUND_AMBIG", 	64, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_FILESYSTEMS_NOT_MOUNTED", 		65, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_FILESYSTEMS_MORE_MOUNTED", 	66, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_HIGHER_REVISION_INSTALLED", 	67, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_NEW_MULTIPLE_VERSION", 		68, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_EXISTING_MULTIPLE_VERSION", 	69, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_DEPENDENCY_NOT_MET",		70,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_NOT_COMPATIBLE",			71,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_CHECK_SCRIPT_WARNING",		72,	SWEVENT_LV, "", 1, SW_WARNING},
{"SW_CHECK_SCRIPT_ERROR",		73,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_DSA_OVER_LIMIT",			75,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_DSA_FAILED_TO_RUN",		76,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_ALREADY_CONFIGURED",		78,	SWEVENT_LV1, "", 1, SW_NOTICE},
{"SW_SKIPPED_GLOBAL_ERROR", 		80,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_FILE_WARNING",			84,	SWEVENT_LV, "", 1, SW_WARNING},
{"SW_FILE_ERROR",			85,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_NOT_LOCATABLE",			86,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_SAME_REVISION_SKIPPED", 		87,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_EXECUTION_BEGINS", 		88, 	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_EXECUTION_ENDS", 			89, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_PRE_SCRIPT_WARNING",		95,	SWEVENT_LV, "", 1, SW_WARNING},
{"SW_PRE_SCRIPT_ERROR",			96,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_FILESET_WARNING",			97,	SWEVENT_LV, "", 1, SW_WARNING},
{"SW_FILESET_ERROR",			98,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_POST_SCRIPT_WARNING",		99,	SWEVENT_LV, "", 1, SW_WARNING},
{"SW_POST_SCRIPT_ERROR",		100,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_CONFIGURE_WARNING",		103,	SWEVENT_LV, "", 1, SW_WARNING},
{"SW_CONFIGURE_ERROR",			104,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_DATABASE_UPDATE_ERROR",		105,	SWEVENT_LV, "", 1, -SW_ERROR},
{"SW_FILESET_BEGINS",			117,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_CONTROL_SCRIPT_BEGINS",		118,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SW_CONTROL_SCRIPT_NOT_FOUND",		119,	SWEVENT_LV, "", 1, SW_NOTICE},

	/*	
	 * Events 0 - 255 are POSIX or are reserved.
	 * Events >255 are specific to the swbis implementation.
	 */ 

{"SW_SOURCE_ACCESS_BEGINS", 		260, 	SWEVENT_LV, "", 0, SW_NOTICE},
{"SW_SOURCE_ACCESS_ENDS", 		261, 	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_CONTROL_SCRIPT_ENDS",		262,	SWEVENT_LV2, "", 1, SW_NOTICE},
{"SW_SOC_LOCK_CREATED",			263,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_SOC_LOCK_REMOVED",			264,	SWEVENT_LV, "", 1, SW_ERROR},
{"SW_SELECTION_EXECUTION_BEGINS",	265,	SWEVENT_LV2, "", 0, SW_NOTICE},
{"SW_SELECTION_EXECUTION_ENDS",		266,	SWEVENT_LV2, "", 0, SW_ERROR},
{"SW_ISC_INTEGRITY_CONFIRMED",		267,	SWEVENT_LV, "", 0, SW_NOTICE},
{"SW_ISC_INTEGRITY_NOT_CONFIRMED",	268,	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SPECIAL_MODE_BEGINS",		269,	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_INTEGRITY_CONFIRMED",		270,	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_SOC_INTEGRITY_NOT_CONFIRMED",	271,	SWEVENT_LV, "", 0, SW_ERROR},
{"SW_ABORT_SIGNAL_RECEIVED",		272,	SWEVENT_LV1, "", 0, SW_ERROR},
{"SW_FILE_EXISTS",			273,	SWEVENT_LV1, "", 0, SW_WARNING},

{"SWI_PRODUCT_SCRIPT_ENDS", 		280, 	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_FILESET_SCRIPT_ENDS", 		281, 	SWEVENT_LV, "", 1, SW_NOTICE},

{"SWI_CATALOG_UNPACK_BEGINS", 		302, 	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_CATALOG_UNPACK_ENDS", 		303, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SWI_TASK_BEGINS", 			304, 	SWEVENT_LV6, "", 1, SW_NOTICE},
{"SWI_TASK_ENDS", 			305, 	SWEVENT_LV6, "", 1, SW_ERROR},
{"SWI_SWICOL_ERROR", 			306, 	SWEVENT_LV, "", 1, SW_ERROR},
{"SWI_CATALOG_ANALYSIS_BEGINS",		307,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_CATALOG_ANALYSIS_ENDS",		308,	SWEVENT_LV, "", 1, SW_ERROR},
{"SWBIS_TARGET_BEGINS",			309,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWBIS_TARGET_ENDS",			310,	SWEVENT_LV, "", 1, SW_ERROR},
{"SWI_NORMAL_EXIT",			311,	SWEVENT_LV, "", 0, SW_ERROR},
{"SWI_SELECTION_BEGINS",		312,	SWEVENT_LV, "", 0, SW_NOTICE},
{"SWI_SELECTION_ENDS",			313,	SWEVENT_LV, "", 0, SW_ERROR},
{"SWI_MSG",				314,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_ATTRIBUTE",			315,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_GROUP_BEGINS",			316,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_GROUP_ENDS",			317,	SWEVENT_LV, "", 1, SW_NOTICE},
{"SWI_TASK_CTS",			318,	SWEVENT_LV6, "", 1, SW_NOTICE},
{"SWI_MAIN_SCRIPT_ENDS",		319,	SWEVENT_LV6, "", 1, SW_NOTICE},
{"<_nil_>", -1, 0, "", 0, SW_NOTICE}
};

static
void *
arr_get_p(void)
{
	void * s;
	if (s_arrM == NULL) s_arrM = vplob_open();
	s = vplob_val(s_arrM, s_arr_indexM);
	if (s == NULL) {
		s = (void*)strob_open(80);
		vplob_add(s_arrM, s);	
	} else {
		strob_strcpy((STROB*)s, "");
	}
	s_arr_indexM++;
	return s;
}

static
struct swEvents *
get_struct_by_message(char * line, struct swEvents * evnt)
{
	char * ws;
	char * event_code;	
	static char buf[100];
	struct swEvents * eop;

	strncpy(buf, line, sizeof(buf)-1);
	buf[sizeof(buf)-1] = '\0';

	event_code=strstr(buf, "SW");
	if (event_code == NULL) return NULL;
	if ((ws=strpbrk(event_code, ": \r\n")) == NULL)
		return NULL;
	else
		*ws = '\0';	

	eop = evnt;
	while (eop->valueM != -1) {
		if (strcmp(eop->codeM, event_code) == 0)  {
			return eop;
		}
		eop++;
	}
	return NULL;
}

static
struct swEvents *
get_struct_by_value(int value, struct swEvents * evnt)
{
	struct swEvents * eop;
	eop = evnt;
	while (eop->valueM != -1) {
		if (eop->valueM == value)  {
			return eop;
		}
		eop++;
	}
	return NULL;
}

char *
swevent_code(int value)
{
	struct swEvents * eop;
	eop = swevent_get_events_array();
	while (eop->valueM != -1) {
		if (eop->valueM == value)  {
			return eop->codeM;
		}
		eop++;
	}
	return  eop->codeM;
}

static
int
get_event_status(char * line, int * statusp, char ** message)
{
	char * s;
	char * s1;
	*message = (char*)NULL;
	*statusp = -1;
	/*
	 * Example:
	 * swcopy: SW_SESSION_ENDS on source host low08-11 at line 56: status=0
	 */

	if (
		strstr(line, ": SW_") ||
		strstr(line, ": SWI_") ||
		strstr(line, ": SWBIS_")
	) {
		if (strstr(line, SWEVENT_STATUS_PFX)) {
			/*
			* The event has a status
			*/
			if (swevent_is_error(line, statusp) < 0 &&
				*statusp == 0) 
			{
				/*
				 * Make sure the status is non zero 
				 * for internal errors
				 */
				*statusp = -1;
			}
		} else {
			/*
			 * No explicit status.
			 * Set the default status.
			 */
			if (swevent_is_error(line, statusp) < 0) {
				fprintf(stderr, "error from swevent_is_error\n");
				*statusp = -1;
			}
		}

		/*
		 * Set the message.
		 * Find the message by finding the 2nd ':'
		 * or (less reliably) the last ':'
		 */
		s = strrchr(line, ':');
		s1 = strchr(line, ':');
		if (s1) s1 = strchr(s1+1, ':');
		if (strstr(line, "SWI_ATTRIBUTE") && s1 && s != s1) {
			/*
			* This may happen if the message has a ':'
			*/
			s = s1;
			E_DEBUG2("s=[%s]", s);
			E_DEBUG2("s1=[%s]", s1);
		}
		if (s) {
			s++;
			while(s && *s == ' ') s++; 
			*message = s;
		}
		return 0;
	}
	return -1;
}

static
char *
swevent_message(struct swEvents * evnt, STROB * buf, int value, char * host, 
				char * loc, char * msg, int verbose_level)
{
	if (verbose_level >= s_verboseM)
		strob_sprintf(buf, 0, 
				"%s: %s on %s host `hostname` at line $LINENO: %s",
			 		swlib_utilname_get(), 
					swevent_code(value), 
					loc, msg); 
	else
		strob_sprintf(buf, 0, "%s: %s on %s host `hostname`: %s", 
				swlib_utilname_get(), 
				swevent_code(value),
				 loc, msg); 
	return strob_str(buf);
}

void
swevent_set_verbose(int v)
{
	s_verboseM  = v;
}

char *
swevent_shell_echo(int to_fd, int verbose_level, 
			char * loc, int value, char * host, char * msg)
{
	char * ret;
	STROB * buf;
	STROB * sbuf;
	struct swEvents * eop;
	char * redir = g_redirnull;
	struct swEvents * evnt;

	sbuf = strob_open(10);
	buf = (STROB*)arr_get_p();

	evnt = swevent_get_events_array();
	eop = get_struct_by_value(value, evnt);
	if (!eop) {
		return strdup("echo \"\" 1>/dev/null");
	}

	if (	1 || /* actually always do this, this test probably should be removed */
		eop->is_swi_eventM || /* need to always echo this internal event */
		verbose_level >= eop->verbose_threshholdM || 
		verbose_level < 0
	) {
		if (to_fd == 2 || eop->is_swi_eventM) {
			/*
			 * Internal swi protocol events are always on
			 * stderr.
			 */
			redir = g_redir2;
		}
		if (to_fd == 1) redir = g_redir1;
		if (to_fd == 0) redir = g_redirnull;
	} else {
		redir = g_redirnull;
	}

	strob_sprintf(buf, 0, "echo \"");
	strob_sprintf(buf, 1, "%s", 
			swevent_message(evnt, sbuf, value, host, loc, 
						msg, verbose_level));
	strob_sprintf(buf, 1, "\" %s", redir);
	strob_close(sbuf);
	ret = strob_str(buf);
	return ret;
}

int
swevent_get_value(struct swEvents * evnt, char * msg)
{
	struct swEvents * eop;
	eop = evnt;
	while (eop->valueM != -1) {
		if (strlen(eop->codeM) && strstr(msg, eop->codeM)) {
			return eop->valueM;
		}
		eop++;
	}
	return  eop->valueM;
}

int
swevent_is_error(char * line, int * statusp)
{
	int n = 0;
	int does_have_status = 0;
	struct swEvents * evnts = eventsArray;
	struct swEvents * ev;
	char * s = (char*)NULL;
	if ((s=strstr(line, SWEVENT_STATUS_PFX))) {
		s += strlen(SWEVENT_STATUS_PFX);
		n = /* swlib_status_ */ atoi(s);
		if (n == INT_MAX || n == INT_MIN) {
			/*
			 FIXME this is dead code.
			 * error
			 */
			does_have_status = 0;
		} else {
			/*
			 * read a valid status
			 */
			does_have_status = 1;
		}
	}
	*statusp = n;

	ev = get_struct_by_message(line, evnts);

	if (!ev) {
		fprintf(stderr, "get_struct_by_message returned null\n");
		*statusp = n;
		return -1;
	}
	if (
		*statusp == SW_ERROR ||
		(ev->default_statusM == SW_ERROR && !does_have_status) ||
		(ev->default_statusM == -(SW_ERROR)) /* -SW_ERROR means always an error */
	) {
		/*
		* The event and/or message indicates an error.
		*/
		return 1;
	}
	
	/*
	* The event is not an error.
	*/
	return 0;
}

ssize_t
swevent_write_rpsh_event(int fd, char * event_line, int len)
{
	char * msg;
	struct swEvents * evnt = eventsArray;
	struct swEvents * this_evnt;
	int ret = 0;
	int status;
	int event_value;
	STROB * tmp;

	tmp = strob_open(100);

	event_value = swevent_get_value(evnt, event_line);
	if (event_value < 0) {
		fprintf(stderr, "swevent_write_rpsh_event: event line not found [%s]\n", event_line);
		return -10;
	} 
	this_evnt = get_struct_by_value(event_value, evnt);
	if (!this_evnt) return -20;
	if (this_evnt->is_swi_eventM == 0) {
		/*
		 * this event is not monitored by the
		 * swicol_ routines therefore don't
		 * send it.
		 */
		return 0;
	}
	E_DEBUG2("event line=[%s]", event_line);

	get_event_status(event_line, &status, &msg);
	strob_strcpy(tmp, "");

	/*
	 * Form a simplified message in the form of
	 *   <number>:<status>\n
	 * where number is the event number
	 */

	if (msg && strstr(msg, SWEVENT_STATUS_PFX) != msg) {
		/*
		 * The message is not "status=X"
		 */
		if (msg)
			strob_sprintf(tmp, 0, "%d:%s", event_value, msg);
		else
			strob_sprintf(tmp, 0, "%d:", event_value);
		swlib_squash_trailing_vnewline(strob_str(tmp));
		strob_sprintf(tmp, 1, "\n");
	} else {
		/*
		 * The message is a simple status
		 */
		strob_sprintf(tmp, 0, "%d:%d\n", event_value, status);
	}
	ret = uxfio_unix_safe_write(fd, strob_str(tmp), strob_strlen(tmp));
	if (ret < 0 || ret != (int)strob_strlen(tmp)) {
		fprintf(stderr, "swevent_write_rpsh_event: error: write: ret=%d fd=%d %s\n",
				ret, fd, strerror(errno));
		ret = -1;
	}
	strob_close(tmp);
	return ret;
}

int
swevent_parse_attribute_event(char * line, char ** attribute, char ** value)
{
	/*
	The line to parse looks something like this

	swinstall: swicol: 315:machine_type=i686
	swinstall: swicol: 315:os_name=Linux
	swinstall: swicol: 315:os_release=2.4.18
	swinstall: swicol: 315:os_version=#1 Tue Dec 30 20:43:14 EST 2003
	*/
	
	char * s;
	char * s1;
	const char attribute_event[]="315:";
		
	s = strstr(line, attribute_event);
	if (!s) return -1;

	s += strlen(attribute_event);
	s1 = s;

	while(*s1 && *s1 != '=') s1++;

	if (s1 == s) return -2;
	if ( ! (*s1)) return -3;
	*s1 = '\0';
	s1++;         /* step over the '=' */
	*attribute = s;
	*value = s1;
	return 0;
}

struct swEvents *
swevent_get_events_array(void)
{
	return eventsArray;
}

struct swEvents *
swevents_get_struct_by_message(char * line, struct swEvents * evnt)
{
	return get_struct_by_message(line, evnt);
}

void
swevent_s_arr_reset(void)
{
	/* This will force reuse of exiting string objects,
	   however the caller needs to be sure that the reused
	   objects have finished being used.  Aggressive use of
	   this routine can unexpectedly cause a malfunction */
	s_arr_indexM = 0;
}

void
swevent_s_arr_delete(void)
{
	void * s;
	int i;
	i = 0;
	while((s=vplob_val(s_arrM, i++)) != NULL) strob_close((STROB*)s);
	vplob_close(s_arrM);
}
