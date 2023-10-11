/* swcommon.c -- swbis utility common routines.

 Copyright (C) 2003,2004,2005,2006,2007  James H. Lowe, Jr. 
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/


#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "vplob.h"
#include "strob.h"
#include "uxfio.h"
#include "taru.h"
#include "swlib.h"
#include "swheader.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swheaderline.h"
#include "swgp.h"
#include "swssh.h"
#include "swfork.h"
#include "swcommon.h"
#include "swevents.h"
#include "swutillib.h"
#include "atomicio.h"
#include "swutilname.h"
#include "globalblob.h"
#include "swproglib.h"
#include "shlib.h"
#include "swi.h"

char * CHARTRUE = "true";
 
extern struct extendedOptions * optionsArray;
static int g_logger_sigterm = 0;

#define SWC_SHELL_KEY_SH	SH_A_sh
#define SWC_SHELL_KEY_KSH	SH_A_ksh
#define SWC_SHELL_KEY_MKSH	SH_A_mksh
#define SWC_SHELL_KEY_BASH	SH_A_bash
#define SWC_SHELL_KEY_POSIX	"posix"
#define SWC_SHELL_KEY_DETECT	"detect"
	
static
ssize_t
try_nibble(int fd, void * buf, int timeout)
{
	/* must be blocking read */
	return read(fd, buf, 1); 
}

static
void
logger_sig_handler(int signum)
{
	switch(signum) {
		case SIGTERM:
		case SIGUSR2:
		case SIGABRT:
			g_logger_sigterm = signum;
			break;
	}
}
	
static
void
detect_special_verbose_status(GB * G, char * sptr)
{
	if (strstr(sptr, "SW_CONTROL_SCRIPT_BEGINS")) {
		G_set_is_in_control_script(G, 1);
	} else if (strstr(sptr, "SW_CONTROL_SCRIPT_ENDS")) {
		G_set_is_in_control_script(G, 0);
	}
}

static
int
is_ssh_sys_error(char * sptr)
{
	if (
		strstr(sptr, "DOING SOMETHING NASTY!") ||
		strstr(sptr, "man-in-the-middle attack!") ||
		strstr(sptr, "Offending key in") ||
		strstr(sptr, "Offending key for") ||
		strstr(sptr, "Connection refused") ||
		strstr(sptr, " Name or service not known") ||
		0
	) { 
		return 1;
	} else {
		return 0;
	}
}

static
int
is_unix_sys_error(char * sptr)
{
	return (
		strstr(sptr, "bnormal termination") || 
		strstr(sptr, "bnormal Termination") || 
		strstr(sptr, "SIGABRT") || 
		strstr(sptr, "SIGINT") || 
		strstr(sptr, "ermission denied") || 
		strstr(sptr, "ot permitted") ||
		strstr(sptr, " input/output") ||
		strstr(sptr, " Input/Output") ||
		strstr(sptr, " i/o") || 
		strstr(sptr, " I/O") ||
		strstr(sptr, "i/o ") || 
		strstr(sptr, "I/O ") ||
		strstr(sptr, " error") ||
		strstr(sptr, " Error") ||
		strstr(sptr, "error ") ||
		strstr(sptr, "Error ") ||
		strstr(sptr, "roken pipe") ||
		strstr(sptr, "ead-only") ||
		strstr(sptr, "ead only") ||
		strstr(sptr, "o such file") || 
		strstr(sptr, "o such device") ||
		strstr(sptr, "o space") || 
		strstr(sptr, "ccess denied") || 
		strstr(sptr, "out of memory") ||
		strstr(sptr, "ot enough space") ||
		strstr(sptr, "format error") ||
		strstr(sptr, "Bad ") || 
		strstr(sptr, "bad file") ||
		strstr(sptr, "Too many ") ||
		strstr(sptr, "too many ") ||
		strstr(sptr, "oo large ") ||
		strstr(sptr, "nvalid arg") ||
		strstr(sptr, "Unexpected EOF in archive") ||
		strstr(sptr, "nexpected eof in archive") ||
		strstr(sptr, "NUL blocks at start") || /* messsage from shell_lib.sh:shls_bashin() */
		0
		);
}

static
int
writeline(int do_stderr, int do_log, int logfd, int efd, char * msg_prefix, char * p_sptr, int len)
{
	char * sptr;
	int ret;
	int eret = 0;
	int lret = 0;
	STROB * tmp;

	E_DEBUG("");
	E_DEBUG3("do_log=%d  logfd=%d", do_log, logfd);
	E_DEBUG3("do_stderr=%d  efd=%d", do_stderr, efd);
	
	if (msg_prefix && strlen(msg_prefix) && strstr(p_sptr, msg_prefix) == NULL) {
		tmp = strob_open(40);
		strob_sprintf(tmp, 0, "%s%s", msg_prefix, p_sptr);
		sptr = strob_str(tmp);
		len = strlen(sptr);
	} else {
		tmp = NULL;
		sptr = p_sptr;
	}


	if (do_stderr) {
		eret = atomicio((ssize_t (*)(int, void *, size_t))write,
					efd, sptr, len);
	}
	if (do_log && logfd > 0)
		lret = swutil_writelogline(logfd, sptr);
	ret = eret;
	ret = lret < 0 ? lret : ret;
	E_DEBUG2("return = %d", ret);

	if (tmp) strob_close(tmp);
	return ret;
}

static
int
is_event(char * line, int * status,
		int * is_swbis_event, int * is_swi_eventp, int * is_posix_eventp)
{
	*status = 0;
	*is_swbis_event = 0;
	*is_swi_eventp = 0;
	*is_posix_eventp = 0;

	if (
		strstr(line, ": SW_") || 
		strstr(line, ": SWI_") || 
		strstr(line, ": SWBIS_")
	) {
		if (swevent_is_error(line, status) && *status == 0) {
			*status = -1;
		}
		if (strstr(line, ": SWBIS_")) /* Extra utility events */
			*is_swbis_event = 1;
		if (strstr(line, ": SWI_")) /* Internal protocol events */
			*is_swi_eventp = 1;
		if (strstr(line, ": SW_"))
			*is_posix_eventp = 1;
		return 1;
	} else {
		return 0;
	}
}

static
int
write_error_line(GB * G, struct sw_logspec * logspec, int vofd, int vefd, STROB * linebuf, int len,
			int verbose_level, int swi_event_fd)
{
	int evret = 0;
	int ret = 0;
	int event_status;
	int is_swi_event = 0;
	int is_posix_event = 0;
	int is_swbis_event;
	char * news;
	char * sptr;
	int do_log = 0;
	int do_stderr = 0;
	struct swEvents * ev = NULL;

	E_DEBUG("");
	E_DEBUG2("verbose_level=%d" , verbose_level);
	E_DEBUG3("vefd=%d  vofd=%d", vefd, vofd);

	sptr = strob_str(linebuf);
	detect_special_verbose_status(G, sptr);
	E_DEBUG("");

	if (is_event(sptr, &event_status, &is_swbis_event, &is_swi_event, &is_posix_event)) {
		E_DEBUG("");
		if (swi_event_fd >= 0) {
			/*
			 * swi_event_fd is the pipe to the swi_<*> routine
			 * which is gated by these events in an expect/send
			 * type of arrangement.
			 */
			E_DEBUG("");
			news = strdup(sptr);
			E_DEBUG2("sptr=[%s]", news);

			if ((ev=swevents_get_struct_by_message(sptr, swevent_get_events_array())) == NULL) {
				fprintf(stderr, "swbis internal error: EVENT NOT Found for [%s]\n", sptr);
			} else {
				;
				/*
				fprintf(stderr, "FOUND EVENT [%s] with threshhold of %d\n", ev->codeM, ev->verbose_threshholdM);
				*/
			}

			/*
			 * The next function is very important, it reads the human readable
			 * messages and forms machine readable ascii text consisting only of
			 * CODE:STATUS (e.g. 262:0) separated by newlines. This form is written
			 * into the event_fd and read by the main process in the G->g_swi_event_fd
			 * descriptor.
			 */

			evret = swevent_write_rpsh_event(swi_event_fd, news, len);

			E_DEBUG("");
			if (evret < 0) {
				fprintf(stderr,
					"error writing event pipe in write_error_line: %d\n",
						evret);
			}
			free(news);
		}

		if (is_swi_event == 0) {
			E_DEBUG("");
			do_log = 1;
		} else {
			E_DEBUG("");
			do_log = 0;
		}

		do_stderr = swlib_test_verbose(ev, verbose_level, is_swbis_event,
					is_swi_event, event_status, is_posix_event);
		if (
			do_log || do_stderr
		) {
			E_DEBUG("");
			if (event_status) {
				/*
				* the status indicates an error ocurred.
				*/
				E_DEBUG("calling writeline");
				ret = writeline(do_stderr, do_log, logspec->logfdM, vefd, "", sptr, len);
			} else {
				E_DEBUG("");
				if (vofd >= 0) {
					E_DEBUG("calling writeline");
					ret = writeline(do_stderr, do_log,
						logspec->logfdM, vofd, "", sptr, len);
				}
			}
		} else {
			E_DEBUG("nothing");
			;
		}
	} else {
		if (logspec->loglevelM > 0) {
			E_DEBUG("");
			 do_log = 1;
		}
		if (verbose_level > 0 ) {
			int is_ssh = 0;
			E_DEBUG("");
			if (
				is_unix_sys_error(sptr) || 
				(is_ssh=is_ssh_sys_error(sptr)) || 
				verbose_level >= SWC_VERBOSE_6
			) {
				/*
				* Error, send to stderr.
				*/
				E_DEBUG("calling writeline");

				do_stderr = 1;
				if (is_ssh) {
					ret = writeline(do_stderr, do_log, logspec->logfdM, vefd, "ssh: ", sptr, len);
				} else {
					ret = writeline(do_stderr, do_log, logspec->logfdM, vefd, "", sptr, len);
				}
			} else {
				/*
				* Info, send to stdout.
				*/
				E_DEBUG("");
				if (
					logspec->loglevelM >= 2 ||
					G_is_in_control_script(G) ||
					0
				) {
					do_log = 1;
				} else {
					do_log = 0;
				}
				if (
					verbose_level >= G->g_verbose_threshold ||
					G_is_in_control_script(G) ||
					0
				) {
					do_stderr = 1;
				} else {
					do_stderr = 0;
				}
				E_DEBUG("calling writeline");
				ret = writeline(do_stderr, do_log, logspec->logfdM, vofd, "", sptr, len);
			}
		} 
	}
	return ret;
}

static
int
write_log(GB * G, STROB * linebuf, struct sw_logspec * logspec, int vofd, int vefd, void * bufa,
			size_t len, int verbose_level, int swi_event_fd)
{
	char * s;
	char s1;
	char * nl;
	int ret;
	int tot = 0;

	strob_strncat(linebuf, (char*)bufa, len);
	s = strob_str(linebuf);
	nl = strpbrk(s, "\n");
	while(nl) {
		s1 = *(nl+1);	
		*(nl + 1) = '\0';
		ret = write_error_line(G, logspec, vofd, vefd, linebuf, strlen(s),
					verbose_level, swi_event_fd);
		if (ret < 0) return -1;
		if (ret > 0) tot += ret;
		*(nl + 1) = s1;	
		memmove(s, (nl+1), strlen(nl+1) + 1);
		nl = strpbrk(s, "\n");
	}
	return tot;
}

static
int
do_run_cmd(SHCMD * cmd, int verbose_level)
{
	int ret;
	SHCMD * cmdvec[2];
	
	if (cmd == NULL) return 0;
	cmdvec[0] = cmd;
	cmdvec[1] = (SHCMD*)NULL;
	swlib_doif_writef(verbose_level, 3, 0, STDERR_FILENO,
		"sending SIGTERM to remote script.\n");
	if (verbose_level >= SWC_VERBOSE_6) {
		shcmd_debug_show_command(cmdvec[0], STDERR_FILENO);
	}
	shcmd_cmdvec_exec(cmdvec);
	shcmd_cmdvec_wait(cmdvec);
	ret = shcmd_get_exitval(cmdvec[0]);
	if (ret != 0) {
		fprintf(stderr, 
			"%s: Error: client script shutdown failed.\n", 
						swlib_utilname_get());
		fprintf(stderr, 
"%s: Stranded processes may be left on target and intermediate hosts.\n",
						 swlib_utilname_get());
		fprintf(stderr, "%s: The failed command was: ", 
						swlib_utilname_get());
		shcmd_debug_show_command(cmd, STDERR_FILENO);
	}
	swlib_doif_writef(verbose_level, 3, 0, STDERR_FILENO,
			"remote kill returned %d\n", ret);
	return ret;
}

static
int
process_targetfile_line(STROB * tmp, char ** current_arg)
{
	char * end;
	char * line;

	strob_set_length(tmp, strob_strlen(tmp) + 2);

	line = strob_str(tmp);
	if (strlen(line) == 0) {
		*current_arg = line;
		return 0;
	}

	if ((end=strpbrk(line, "\n\r#"))) {
		*end = '\0';
		end--;
	} else {
		end = line + strlen(line) - 1;
	}

	/*
	* Squash whitespace at the end of the line.
	*/
	while(end > line && isspace(*end)) { *end = '\0'; end--; }
	
	/*
	* Squash whitespace at the beginning of the line.
	*/
	while(isspace(*line)) line++;

	/*
	* Add a leading '@'
	*/
	if (strlen(line) && *line != '@') {
		memmove(line+1, line, strlen(line) + 1);
		*line = '@';
	}

	*current_arg = line;
	return strlen(line);
}

static
void
get_dir_component(STROB * ret, char * news, int nf)
{
	STROB * tmp;
	char * s;
	int i;

	tmp = strob_open(20);
	i = 0;
	s = strob_strtok(tmp, news, "/");
	while(s && i < nf) {
		s = strob_strtok(tmp, NULL, "/");
		i++;
	}
	if (s) {
		if (strob_strtok(tmp, NULL, "/") == NULL)
			strob_strcpy(ret, "");
		else	
			strob_strcpy(ret, s);
	} else {
		strob_strcpy(ret, "");
	}
	strob_close(tmp);
}

static
STROB *
determine_longest_common_dir(STRAR * p_sourcepath_list)
{
	char * s;
	char * dirname;
	int is_same;
	int nf;
	int n;
	int nrel;
	char * news;
	STROB * tmp;
	STROB * ret;
	char * current_dir;
	STRAR * sourcepath_list;


	E_DEBUG("");
	if (!p_sourcepath_list) return NULL;

	sourcepath_list = strar_copy_construct(p_sourcepath_list);
	if (!sourcepath_list) return NULL;

	ret = strob_open(24);
	tmp = strob_open(24);
	dirname = NULL;

	E_DEBUG("");
	/* Check that the path are all relative or all
	   absolute */
	n = 0;
	nrel = 0;
	while ((s=strar_get(sourcepath_list, n))) {
		news = strdup(s);
		swlib_squash_all_dot_slash(news);
		swlib_squash_double_slash(news);
		swlib_squash_trailing_slash(news);
		if ( *news != '/') 
			nrel++;
		free(news);
		n++;
	}
	E_DEBUG("");

	if (nrel && nrel != (n - 1)) {
		/* commonality not supported */
		/* mixed relative and absolute paths */
		strob_close(ret);
		strar_close(sourcepath_list);
		return NULL;
	}

	/* sort the list, this algorithm requires it */
	strar_qsort(sourcepath_list, strar_qsort_neg_strcmp);

	if (nrel == 0) {
		strob_strcpy(ret, "/");	
	}

	E_DEBUG("");
	current_dir = NULL;
	nf = 0;
	is_same = 1;
	while (is_same) {
		E_DEBUG("");
		nf++;
		n = 0;
		while ((s=strar_get(sourcepath_list, n))) {
			E_DEBUG2("s=[%s]", s);
			news = strdup(s);
			swlib_squash_all_dot_slash(news);
			swlib_squash_trailing_slash(news);
			swlib_squash_double_slash(news);
			get_dir_component(tmp, news, nf-1);	
			E_DEBUG2("component is [%s]", strob_str(tmp));
			if (current_dir == NULL) {
				if (strob_strlen(tmp)) {
					current_dir = strdup(strob_str(tmp));
				} else {
					is_same = 0;
				}
				E_DEBUG2("setting current_dir=[%s]", current_dir);
			} else {
				E_DEBUG2("current_dir is already [%s]", current_dir);
				if (strcmp(strob_str(tmp), current_dir) != 0) {
					/* a component is different, return the
					   last component */
					is_same = 0;
					E_DEBUG3("found difference [%s] [%s]", current_dir, strob_str(tmp));
					free(current_dir);
					current_dir = NULL;
					break;
				} else {
					;	
				}
			}
			n++;
			free(news);
		}
		E_DEBUG("out of while");
		if (current_dir) {
			E_DEBUG2("dir catting [%s]", current_dir);
			swlib_unix_dircat(ret, current_dir);
			free(current_dir);
			current_dir = NULL;
		}
		if (s != NULL) {
			E_DEBUG("s == NULL");
			break;
		}
	}
	E_DEBUG("");
	strob_close(tmp);
	E_DEBUG2("returning [%s]", strob_str(ret));
	strar_close(sourcepath_list);
	return ret;
}

STROB *
swc_make_multiplepathlist(STRAR * sourcepath_listM, STROB * rootdir, STROB * firstpath)
{
	char * s;
	char * b;
	char * dir2;
	char * base;
	int n;
	STROB * tmp;
	STROB * strb;
	STROB * lpath;

	lpath = determine_longest_common_dir(sourcepath_listM);
	if (lpath == NULL) {
		E_DEBUG("return NULL");
		return NULL;
	}

	/* Report the longest common directory to the caller */
	if (rootdir) {
		strob_strcpy(rootdir, strob_str(lpath));
		E_DEBUG2("rootdir is [%s]", strob_str(rootdir));
	}
	base = strob_str(lpath);
	E_DEBUG2("base is [%s]", base);

	/* Now make a list of paths that are relative to the
	   longest common directory */

	strb = strob_open(24);
	tmp = strob_open(24);
	n = 0;

	while ((s=strar_get(sourcepath_listM, n))) {
		E_DEBUG2("s=[%s]", s);
		swlib_squash_trailing_slash(s);
		b = strstr(s, base);
		if (b == NULL || b != s) {
			E_DEBUG("return NULL");
			return NULL;
		} else {
			dir2 = s + strlen(base);
			if (strcmp(base, "/")) {
				/* base is more than "/" */
				if (*dir2 != '/') {
					E_DEBUG("return NULL");
					return NULL;
				}
				dir2++;
			} else {
				;	
			}
			E_DEBUG2("dir2=[%s]", dir2);
			strob_sprintf(strb, 1, "\"%s\" ", (strlen(dir2) == 0 ? "." : dir2));	
			if (firstpath)
				strob_strcpy(firstpath, dir2);
		}
		E_DEBUG("");
		n++;
	}	
	strob_close(tmp);
	strob_close(lpath);
	E_DEBUG2("list is [%s]", strob_str(strb));
	return strb;
}

char *
swc_print_umask(char * buf, size_t len)
{
	int ret;
	mode_t mode = swlib_get_umask();
        ret = snprintf(buf, len-1, "%03o", (unsigned int)mode);
	buf[ret] = '\0';
	return buf;
}

ERRORCODE * createErrorCode(void)
{ 
	ERRORCODE * ec = (ERRORCODE *)malloc(sizeof(ERRORCODE));
	ec->codeM = 0;
	return ec;
}

void destroyErrorCode(ERRORCODE * EC) { free(EC); }

void swc_setErrorCode(ERRORCODE * EC, int code, char * msg) { EC->codeM = code; }

int swc_getErrorCode(ERRORCODE * EC) { return EC->codeM; }

void 
swc_copyright_info(FILE * fp)
{
	fprintf(fp,  "%s",
"Copyright (C) 2004,2005,2006,2007,2008,2009,2010 Jim Lowe\n"
"Portions are copyright 1985-2000 Free Software Foundation, Inc.\n"
"This software is distributed under the terms of the GNU General Public License\n"
"and comes with NO WARRANTY to the extent permitted by law.\n"
"See the file named COPYING for details.\n");
}

int 
swc_parse_soc_spec(char * arg, char ** selections, char ** target)
{
	char * p;
	*target = NULL;
	*selections = NULL;

	if (!arg) {
		*selections = NULL;
		*target = NULL;
		return 0;
	}

	if (strncmp(arg, "@:", 2) == 0) {
		/*
		* Implementation Extention:
		* Support relative path
		* using '':path'' syntax
		*/
		if (strlen(arg) > 2) {
			*target = strdup(arg + 1);
		} else {
			/*
			* colon not allowed by itself, per spec.
			*/
			return 1;
		}
		return 0;
	}

	if (arg[0] != '@') {
		/*
		 * this happens for ''selections @ targets'' syntax
		 */
		*selections = strdup(arg);
		if ((p = strrchr(*selections, (int) '@')) != NULL) {
			*p = '\0';
			*target = strdup(p + 1);
		}
	} else if (arg[0] == '@') {
		*target = strdup(arg + 1);
	} else {
		; /* dead branch */
	}

	if (*target && **target == ':') 
			*target = strdup(++(*target));
	return 0;
}

int
swc_construct_newkill_vector(
		SHCMD * kmd, 
		int nhops,
		STRAR * tramp_list, 
		char * script_pid, 
		int verbose_level)
{
	int i = 0;
	char ** argvec;
	char * clientmsg;
	char * pid;
	char * s;
	char * h;

	if (!script_pid || strlen(script_pid) == 0) {
		return -1;
	}
	
	swlib_doif_writef(verbose_level, 7, 0, STDERR_FILENO, 
		"In swc_construct_newkill_vector: looping thru argvec\n");
	argvec = shcmd_get_argvector(kmd);
	while(*argvec) {
		if (strcmp(*argvec, SWC_KILL_PID_MARK) == 0) {
			clientmsg = strar_get(tramp_list, i++);
			if (!clientmsg) return -1;
			if (strlen(clientmsg) == 0) return -2;
			pid = strrchr(clientmsg, ':');
			if (!pid) return -3;
			if (strlen(pid) == 0) return -4;
			pid++;
			s = pid;
			while(*s) {
				if (!isdigit((int)*s)) return -5;
				s++;
			}
			*(pid - 1) = '\0';
			h = strrchr(clientmsg, ':');
			if (!h) return -8;
			h++;	
			swlib_doif_writef(verbose_level, 7, 0, STDERR_FILENO, 
			"construct_newkill_vector: list:  host:pid [%s:%s]\n",
								h, pid);
			*argvec = strdup(pid);
		}
		argvec++;
	}

	/*
	* Sanity check.
	*/
	if (i != nhops-1) return -9; 

	clientmsg = strar_get(tramp_list, i);
	if (clientmsg) {
		/*
		* Sanity check.
		*/
		if (strstr(clientmsg, 
				SWBIS_TARGET_CTL_MSG_125) != clientmsg) {
			swlib_doif_writef(verbose_level, -1, 0, STDERR_FILENO, 
		"construct_newkill_vector: unexpected tramp in the list: %s]\n"
				,strar_get(tramp_list, i));
			return -12;
		}
	} else {
		/*
		* Sanity check.
		*/
		return -14;
	}

	argvec = shcmd_get_argvector(kmd);
	while(*argvec) {
		if (*(argvec+1) == (char*)NULL) {
			shcmd_add_arg(kmd, "kill");
			shcmd_add_arg(kmd, script_pid);
			break;
		}
		argvec++;
	}
	return 0;
}

int
swc_form_command_args(GB * G,
			char * context, 
			char * targetpath, 
			STRAR * target_cmdlist, 
			int opt_preview,
			int nhops, int opt_no_getconf,
			char * landing_shell)
{
	STROB * tmpcommand = strob_open(10);
	if (strcmp(targetpath, "-") == 0 ) {
		strar_add(target_cmdlist, "/bin/cat");
	} else {
		/* strob_strcpy(tmpcommand, SWSSH_POSIX_SHELL_COMMAND); */
		strob_strcpy(tmpcommand,
				swssh_landing_command(landing_shell,
				opt_no_getconf));

		if (G->g_do_cleanshM == 0) {
			strob_sprintf(tmpcommand, STROB_DO_APPEND, " _swbis _%s", swlib_utilname_get());
		} else {
			;
		}

		swssh_protect_shell_metacharacters(tmpcommand, 
							nhops, 
							CMD_TAINTED_CHARS);
		strar_add(target_cmdlist, strob_str(tmpcommand));
	}
	strob_close(tmpcommand);	
	return 0;
}

char *
swc_make_absolute_path(char * target_current_dir, char * target_path)
{
	char * ret;
	STROB * tmp;

	if (*target_path == '/')
		return target_path;

	tmp = strob_open(32);
	strob_strcpy(tmp, target_current_dir);
	strob_strcat(tmp, "/");
	strob_strcat(tmp, target_path);

	ret = strdup(strob_str(tmp));	
	strob_close(tmp);
	return ret;
}

char *
swc_validate_targetpath(int nhops, char * targetpath, char * default_target, char * cwd, char * context)
{
	if (nhops < 0 ) {
		fprintf(stderr, "%s: %s error\n", swlib_utilname_get(), context);
		return NULL;
	}

	if (targetpath == NULL) {
		/* apply the default target */
		targetpath = default_target;
	}

	E_DEBUG("");
	if (
		nhops == 0 &&
		*targetpath == ':'
	) {
		/*
		* This path support implem. extension
		* relative path targets.
		*/
		if ( *(targetpath+1) != '/' &&
		     *(targetpath+1) != '-'
		) {
			STROB * tmp = strob_open(10);
			strob_strcpy(tmp, cwd);
			swlib_unix_dircat(tmp, targetpath+1);	
			targetpath = strdup(strob_str(tmp));
			strob_close(tmp);	
		} else {
			/*
			* Skip over the leading ':'
			*/
			targetpath++;
		}
	}

	E_DEBUG("");
	if (nhops == 0 && strncmp(targetpath, "\\:", 2) == 0) {
		/*
		* handle escaped colon ':'
		*/
		targetpath++;
	}

	E_DEBUG("");
	if (nhops > 0 && strcmp(default_target, "-") == 0) {
		/*
		* This combination, stdout on a remote host makes
		* little sense.  Change the default target to "."
		* This will have the effect of supporting the 
		* target syntax of a plain host with no path
		* spec.  e.g.    @host1
		*/
		default_target = ".";
	}

	E_DEBUG("");
	if (targetpath == (char*)NULL || strlen(targetpath) == 0) {
		targetpath = default_target;
		if (nhops >= 1 && (*targetpath) == '\0' ) {
			/*
			* If the target path is empty
			*/
			targetpath = strdup("/");
		}
	}

	E_DEBUG("");
	if (swlib_is_sh_tainted_string(targetpath)) {
		E_DEBUG2("string tainted with shell meta-characters: %s", targetpath);
		swlib_doif_writef(1, -1, 0, STDERR_FILENO, 
			"%s directory name is tainted with shell meta-characters\n", context);
		return NULL;
	}

	E_DEBUG("");
	if (strcmp(targetpath, "-")) {
		if (strcmp(targetpath, ".") == 0) {
			/*
			* allow a "." as a targetpath.
			* This is an implementation extension.
			*/
			;
		} else if (nhops < 1 && *targetpath != '/') {
			/*
			* allow relative path for remote targets
			*/
			swlib_doif_writef(1, -1, 0, STDERR_FILENO, 
				"target path [%s] not absolute.\n", 
						targetpath);
			return NULL;
		}
	}
	E_DEBUG("");
	return strdup(targetpath);
}

int
swc_do_preview_cmd(GB * G, char * prefix, FILE * fver,
		char * targetpath,
		SHCMD * sshcmd, 
		SHCMD * kmd, 
		char * cl_target, 
		STRAR * cmdlist,
		int nhops,
		int do_first, int opt_no_getconf,
		char * landing_shell)
{
	int ret = 0;
	char ** argvec;
	char ** args;
	char ** p;

	if (do_first)
		ret = swc_form_command_args(G, "target", 
				targetpath, 
				cmdlist, 
				1 /*opt_preview*/,
				nhops, opt_no_getconf,
				landing_shell);
	if (ret) {
		return -1;
	}
	/* swssh_reset_module(); */
	ret = swssh_assemble_ssh_cmd(sshcmd, cmdlist, (STRAR*)NULL, nhops);

	if (ret) {
		return -2;
	}

	args = shcmd_get_argvector(sshcmd);
	p = args;
	while(p && *p) {
		fprintf(fver, "[%s]", *p);
		p++;
	}
	fprintf(fver, "\n");

	argvec = shcmd_get_argvector(kmd);
	while(*argvec) {
		if (*(argvec+1) == (char*)NULL) {
			shcmd_add_arg(kmd, "kill");
			shcmd_add_arg(kmd, "$PPID");
			break;
		}
		argvec++;
	}

	fprintf(fver, "%s", prefix);
	args = shcmd_get_argvector(kmd);
	p = args;
	while(p && *p) {
		fprintf(fver, "[%s]", *p);
		p++;
	}
	fprintf(fver, "\n");

	return 0;
}

void
swc_initialize_logspec(struct sw_logspec * logspec,
		char * logfile, int log_level)
{
	int fd;
	logspec->loglevelM = 0;
	logspec->logfdM = -1;

	if (log_level > 0) {
		fd = swc_open_logfile(logfile);
		if (fd >= 0) {
			logspec->logfdM = fd;
			logspec->loglevelM = log_level;
		} else {
			fprintf(stderr, "%s: error opening logfile : %s\n",
					swlib_utilname_get(),
					logfile);
		}	
	}
	return;
}

int
swc_open_logfile(char * logfile)
{
	int ret;
	ret = open(logfile, O_WRONLY|O_CREAT|O_APPEND, 0644);
	return ret;
}

int
swc_open_filename(char * sourcefilename, int * open_errorp)
{
	int fd;
	struct stat st;
	int flag;

	*open_errorp = -1;

	if (strcmp(sourcefilename, "-") == 0) {
		*open_errorp = 0;
		return STDIN_FILENO; 
	}
	
	if (stat(sourcefilename, &st) < 0) {
		*open_errorp = 1;
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		*open_errorp = 0;
		return -1;
	}

	if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
		flag = O_RDONLY;
	} else {
		flag = O_RDONLY;
	}

	fd = open(sourcefilename, flag, 0);
	if (fd < 0) {
		fprintf(stderr, 
			"open error : (%s) : %s\n", 
					sourcefilename, strerror(errno));
		*open_errorp = 1;
		return -1;
	}
	*open_errorp = 0;
	return fd;
}

pid_t
swc_run_ssh_command (GB * G,
		SHCMD ** sshcmd,
		STRAR * cmdlist,
		char * path,
		int opt_preview,
		int nhops,
		int * o_fdar,
		int * i_fdar,
		struct termios * login_orig_termiosP,
		struct winsize * login_sizeP, 
		pid_t * pump_pid,
		char * fork_type, 
		int make_master_raw, 
		sigset_t * ignoremask,
		int is_no_fork_ok,
		int verbose_level,
		int * source_file_size,
		int opt_no_getconf,
		int * is_local_fd,
		char * landing_shell,
		int error_fd,
		struct sw_logspec * logspec
		)
{
	int did_open = 0;
	int i_fdm[3];
	int fdm[3];
	pid_t ts_pid;
	int ret;
	int fd;

	*pump_pid = 0;
	
	E_DEBUG("");
	E_DEBUG2("path=[%s]", path);
	E_DEBUG2("nhops=[%d]", nhops);
	E_DEBUG2("is_no_fork_ok=[%d]", is_no_fork_ok);

	if (source_file_size) *source_file_size = 0;
	
	if (is_local_fd) *is_local_fd = 0;
	if (
		is_no_fork_ok > 0 &&
		nhops == 0 &&
		*path == '/' &&
		1
	) {
		/*
		* Optimization for Local source file.
		*/
		struct stat st;

		E_DEBUG("Local File");
		if (stat(path, &st) < 0) {
			/*
			* Source Access error
			*/
			swlib_doif_writef(verbose_level, 1, logspec, error_fd,
					"SW_SOURCE_ACCESS_ERROR while accessing local file %s : %s\n"
							,
							path, strerror(errno));
			/* fprintf(stderr, "%s : %s\n", path, strerror(errno)); */
			return -2;
		}	
		E_DEBUG("");
		if (
			S_ISREG(st.st_mode) &&
			G->g_do_createM == 0 &&
			1	
		) {

			if (source_file_size) {
				*source_file_size = (int)(st.st_size);
			}
			fd = open(path, O_RDONLY, 0);
			if (fd < 0) {
				fprintf(stderr, 
					"open error %s : %s\n", 
						path, strerror(errno));
			}

			if (S_ISREG(st.st_mode)) if (is_local_fd) *is_local_fd = 1;
	 		i_fdm[0] = fd;
			i_fdm[1] = open("/dev/null", O_RDWR, 0);
			i_fdm[2] = i_fdar[2];
			did_open = 1;
			fork_type = SWFORK_NO_FORK;
			if (fd < 0) return -1;
			if (verbose_level > SWC_VERBOSE_8) {
				fprintf(stderr, "Local file opened : %s\n",
					path);
			}
		}
	}

	E_DEBUG2("did_open=%d", did_open);
	if (did_open == 0) {
	 	i_fdm[0] = i_fdar[0];
		i_fdm[1] = i_fdar[1];
		i_fdm[2] = i_fdar[2];

		E_DEBUG("");
		ret = swc_form_command_args(G,
			"target", 
			path, 
			cmdlist, 
			opt_preview, 
			nhops,
			opt_no_getconf, landing_shell);
		SWLIB_ASSERT(ret == 0);

		/* if (opt_preview) swssh_reset_module(); */
		ret = swssh_assemble_ssh_cmd(
			sshcmd[0], 
			cmdlist, 
			(STRAR*)(NULL), 
			nhops);

		SWLIB_ASSERT(ret == 0);
	}

	if (nhops == 0) {
		E_DEBUG("Set sigdelset(ignoremask, SIGINT);");
		sigdelset(ignoremask, SIGINT);
	}

	ts_pid = swlib_fork(fork_type,
			fdm,
		 	i_fdm[1],
			i_fdm[0],
			i_fdm[2],
			login_orig_termiosP, 
			login_sizeP, pump_pid, make_master_raw, ignoremask);

	if (strcmp(fork_type, SWFORK_NO_FORK) != 0) {
		/*
		 * Fork happened.
		 */
		E_DEBUG("Fork Happened");
		if (ts_pid == 0) {
			/*
			 * Execute the ssh pipeline.
			 */
			if (verbose_level >= SWC_VERBOSE_4) {
				swlib_doif_writef(verbose_level,
							SWC_VERBOSE_4,
							0, STDERR_FILENO,
						"Executing : ");
				shcmd_debug_show_command(
						sshcmd[0], STDERR_FILENO);
			}
			/* setup the fd closes */
			swgp_close_all_fd(3);
			
			/* Set the SIG_IGN in the specified signals */
			if (nhops >= 1) {
				swc_set_sig_ign_by_mask(ignoremask);
			}
			shcmd_unix_execve(sshcmd[0]);
			fprintf(stderr, "exec failed\n");
			_exit(1);
		}

		if (ts_pid < 0) {
			fprintf(stderr, "fork error : %s\n", strerror(errno));
			_exit(1);
		}
	} else {
		;
		E_DEBUG("No Fork Happened");
		/*
		 * No fork required.
		 */
	}

	o_fdar[0] = fdm[0];
	o_fdar[1] = fdm[1];
	o_fdar[2] = fdm[2];
	return ts_pid;
}

int
swc_analyze_status_array(pid_t * pid, int num, int * status, int debug)
{
	int i;
	int ret = 0;
	int x;
	for(i=0; i<num; i++) {
		if (pid[i] < 0 && status[i]) {
			if (WIFEXITED(status[i])) {
				x = WEXITSTATUS(status[i]);
				if (x) ret++;
				if (debug >= SWC_VERBOSE_IDB) {
					fprintf(stderr,
					"status[pid=%d] exit val = %d\n",
							 (int)pid[i], x);
				}
			} else {
				fprintf(stderr, 
				"%s: process[pid=%d] failed to exit normally\n",
							swlib_utilname_get(),
							(int)pid[i]);
				ret++;
			}
		}
	}
	return ret;
}

int
swc_do_run_kill_cmd(SHCMD * killcmd, SHCMD * target_kmd, 
				SHCMD * source_kmd, int verbose_level)
{
	int ret;
	ret = do_run_cmd(killcmd, verbose_level);
	ret = do_run_cmd(target_kmd, verbose_level);
	ret = do_run_cmd(source_kmd, verbose_level);
	return ret;
}

int
swc_checkbasename(char * s) {
	/*
	* Make sure the basename of the pathname
	* is a non-zero length filename (i.e. has no '/')
	*/
	if (strlen(s) > 1)
		if (
			!s || 
			*s == '\0' || 
			*s == '/' || 
			strchr(s, '/') ||
			s[strlen(s) - 1] == '/' ||
			0) 
				return 1;
	return 0;
}

void
swc_record_pid(pid_t pid, pid_t * p_pid_array, int * p_pid_array_len, 
		int verbose_level)
{
	if (*p_pid_array_len < SWC_PID_ARRAY_LEN && pid > 0) {
		swlib_doif_writef(verbose_level, SWC_VERBOSE_IDB,
			0, STDERR_FILENO,
			"%s : record_pid [%d]\n", SW_UTILNAME, (int)pid);
		p_pid_array[(*p_pid_array_len)] = pid;
		p_pid_array[(*p_pid_array_len) + SWC_PID_ARRAY_LEN] = pid;
		(*p_pid_array_len)++;
	}
}

void
swc_set_sig_ign_by_mask(sigset_t * ignoremask)
{
	if (sigismember(ignoremask, SIGTERM)) {
		swgp_signal(SIGTERM, SIG_IGN);
	}
	if (sigismember(ignoremask, SIGALRM)) {
		swgp_signal(SIGALRM, SIG_IGN);
	}
	if (sigismember(ignoremask, SIGINT)) {
		swgp_signal(SIGINT, SIG_IGN);
	}
}

int
swc_process_selection_files(GB * G, VPLOB * swspec_list)
{
	STROB * tmp;
	char * s;
	int n;
	int i;
	int fd;
	int retval;
	SWVERID * swverid = NULL;

	retval = 0;
	tmp = strob_open(10);
	for (i=0; i<SWC_TARGET_FD_ARRAY_LEN; i++) {
		fd = G->g_selectfd_array[i];
		if (fd >= 0) {
			while ((n=swgp_read_line(fd, (STROB*)tmp, DO_NOT_APPEND)) > 0) {
				s = strob_str(tmp);
				while(swlib_squash_trailing_char(s, '\n') == 0);
				if (strlen(s) == 0)
					continue;
				swverid = swverid_open(NULL, s);
				if (swverid == NULL) {
					fprintf(stderr, "%s: error processing software selection from file: [%s]\n",
						swlib_utilname_get(), s);
					retval = -1;
					goto error_out;
				} else {
					vplob_add(swspec_list, (void*)swverid);
				}
			}
			if (fd > STDERR_FILENO) close(fd);
			G->g_selectfd_array[i] = -1;
		} else {
			;
		}
	}
error_out:
	strob_close(tmp);
	return retval;
}

int
swc_process_selection_args(VPLOB * swspec_list, char ** argvector,
			int uargc,
			int * poptind)
{
	char * s;
	int count = 0;
	int error_count = 0;
	SWVERID * swverid = NULL;
	int is_last;

	is_last = 0;
	error_count = 0;
	if (!swspec_list) return -1;
	
	while (*poptind < uargc && is_last == 0) {
		s = argvector[*poptind];
		if (s && *s == '@') {
			break;
		}
		if (strlen(s) > 0 && s[strlen(s)-1] == '@') {
			is_last = 1;
		}
		(*poptind)++;
		count++;
		swverid = swverid_open(NULL, s);
		if (swverid == NULL) {
			fprintf(stderr, "%s: error processing software selection: %s\n", swlib_utilname_get(), s);
			 (error_count)++;
		} else {
			vplob_add(swspec_list, (void*)swverid);
		}
	}
	return error_count;
}

int
swc_read_selections_file(void)
{
	return 0;
}

char *
swc_get_next_target(char ** argvector, 
		int uargc, 
		int * poptind, 
		int * targetsfd_array, 
		char * default_target,
		int * pnum_remains)
{ 
	static int did_one = 0;
	static int targetfd_index = 0;
	char * tmpch;
	char * current_arg;
	STROB * tmp;
	int fd;
	int n;

	*pnum_remains = 0;
	tmp = strob_open(10);

	if (*poptind == uargc && targetsfd_array[targetfd_index] < 0 && 
							did_one == 0) {
		/*  
		* No args, use the default.
		*/
		did_one = 1;
		*pnum_remains = 0;
		strob_strcpy(tmp, "@");
		tmpch = default_target;
		strob_strcat(tmp, tmpch);
		current_arg = strdup(strob_str(tmp));
		(*poptind)++;
		strob_close(tmp);
		return current_arg;
	} 
	did_one = 1;
	
	if ((*poptind) < uargc) {
		/*
		* Return the next arg.
		*/
		current_arg = argvector[(*poptind)++];
		*pnum_remains = uargc - (*poptind);
		if (strcmp(current_arg, "@") == 0) {
			/*
			* Handle the case of  `` @<space>target_spec ''
			*/
			current_arg = argvector[(*poptind)++];
			if (!current_arg) {
				fprintf(stderr, "invalid target.\n");
				return ((char*)NULL);
			}
			strob_strcpy(tmp, "@");
			strob_strcat(tmp, current_arg);
			current_arg = strdup(strob_str(tmp));
		} else if (*current_arg != '@') {
			strob_strcpy(tmp, "@");
			strob_strcat(tmp, current_arg);
			current_arg = strdup(strob_str(tmp));
		} else {
			current_arg = strdup(current_arg);
		}
		strob_close(tmp);
		return current_arg;
	}

	fd = targetsfd_array[targetfd_index];
	while (fd >= 0) {
		/*
		* read next line in the targets file.
		*/
		*pnum_remains = 1;  /* assume more that one target */
		n = swgp_read_line(fd, (STROB*)tmp, DO_NOT_APPEND);
		if (n > 0) {
			if (process_targetfile_line(tmp, &current_arg) >= 1) {
				current_arg = strdup(current_arg);
				strob_close(tmp);
				return current_arg;
			} else {
				/*
				* A comment or empty line.
				*/
				;
			}
		} else if (n == 0) {
			targetfd_index++;
		} else {
			targetfd_index++;
		}
		fd = targetsfd_array[targetfd_index];
	}
	strob_close(tmp);
	return ((char*)NULL);
}

void
swc_flush_logger(GB * G, STROB * slinebuf, STROB * tlinebuf,
		int efd, int ofd, struct sw_logspec * logspec, int verboselevel)
{
	write_log(G, slinebuf, logspec, ofd, efd, "", 0, verboselevel, -1);
	write_log(G, tlinebuf, logspec, ofd, efd, "", 0, verboselevel, -1);
	if (logspec && logspec->logfdM > 0) {
		close(logspec->logfdM);
	}
}

pid_t
swc_fork_logger(GB * G, STROB * x_slinebuf, STROB * x_tlinebuf, int efd, int ofd,
		struct sw_logspec * logspec, int * s_efd, int * t_efd,
		int vlevel, int * swi_event_fd_p)
{
	pid_t pid;
	int spipe[2];
	int tpipe[2];
	int ppipe[2];
	sigset_t fork_defaultmask;
	sigset_t fork_blockmask;
	STROB * slinebuf = strob_open(100);
	STROB * tlinebuf = strob_open(100);

	if (s_efd) {
		if (pipe(spipe) < 0) return -1;
	} else {
		spipe[0] = -1;
		spipe[1] = -1;
	}

	if (pipe(tpipe) < 0) return -1;

	if (swi_event_fd_p) {
		if (pipe(ppipe) < 0) return -1;
	} else {
		ppipe[0] = -1;
		ppipe[1] = -1;
	}	

	sigemptyset(&fork_blockmask);
	sigaddset(&fork_blockmask, SIGINT); /* block in logger process */
	sigaddset(&fork_blockmask, SIGTERM); /* block in logger process */
	sigaddset(&fork_blockmask, SIGALRM); /* block in logger process */
	sigemptyset(&fork_defaultmask);
	sigaddset(&fork_defaultmask, SIGINT);
	sigaddset(&fork_defaultmask, SIGPIPE);
	sigaddset(&fork_defaultmask, SIGTERM);
	
	pid = swndfork(&fork_blockmask, &fork_defaultmask);
	if (pid == 0) {
		int status;
		int rra;
		int sra;
		int done_s;
		int rrb;
		int srb;
		int done_t;
		char * bufa;
		char * bufb;
		STROB * tmp;
	
		/* swgp_signal(SIGTERM, logger_sig_handler); */
		swgp_signal(SIGABRT, logger_sig_handler);
		swgp_signal(SWBIS_LOGGER_SIGTERM, logger_sig_handler);
		bufa = (char*)malloc(SWLIB_PIPE_BUF + SWLIB_PIPE_BUF);
		bufb = (char*)malloc(SWLIB_PIPE_BUF + SWLIB_PIPE_BUF);
		tmp = strob_open(100);
		if (s_efd)
			close(spipe[1]);
		close(tpipe[1]);
		if (ppipe[0] >= 0) close(ppipe[0]);

		memset(bufa, '\0', SWLIB_PIPE_BUF + SWLIB_PIPE_BUF);
		memset(bufb, '\0', SWLIB_PIPE_BUF + SWLIB_PIPE_BUF);

		/*
		 * Force all file descriptors to 6 or below and
		 * close all descriptors greater than 6.
		 */
		if ( (s_efd && spipe[0] <= 6) || tpipe[0] <= 6 ||
			(ppipe[1] >= 0 && ppipe[1] <= 6) ) {
			/*
			 * Sanity check, make sure they are all above where
			 * we want to dup them to.
			 */
			fprintf(stderr, "%s: internal error in fork_logger\n",
				swlib_utilname_get());
			return -1;
		}

		if (logspec->logfdM >= 0 && logspec->logfdM <= 6) {
			while (logspec->logfdM <= 6 && logspec->logfdM != -1)
				logspec->logfdM = dup(logspec->logfdM);
		} 

		if (s_efd) {
			if (dup2(spipe[0], 3) < 0) 
				fprintf(stderr, "dup2 error (loc=a)\n");
		} else {
			close(3);
		}
		if (dup2(tpipe[0], 4) < 0)
			fprintf(stderr, "dup2 error (loc=b)\n");

		if (ppipe[1] >= 0) {
			if (dup2(ppipe[1], 5) < 0)
				fprintf(stderr, "dup2 error (loc=c)\n");
			ppipe[1] = 5;
		} else {
			close(5);
		}

		if (efd != 2) 
			if (dup2(efd, 2) < 0)
				fprintf(stderr, "dup2 error (loc=c)\n");

		if (logspec->logfdM >= 0) {
			if (dup2(logspec->logfdM, 6) < 0)
				fprintf(stderr, "dup2 error (loc=e)\n");
			logspec->logfdM = 6;
		} else {
			close(6);
		}

		/* close all fd's 7 and above */

		swgp_close_all_fd(7);

		done_s = 0;
		done_t = 0;
		if (!s_efd) done_s++;

		while((!done_s || !done_t) && g_logger_sigterm == 0) {
			rra = 0;
			rrb = 0;
			if (!done_s) {
				sra = swgpReadFdNonblock(bufa, 3 /*spipe[0]*/, &rra);
				if (sra < 0 || (sra && !rra)) done_s++;
			}
			if (rra > 0) {
				write_log(G, slinebuf, logspec, ofd, efd, bufa, rra, vlevel, ppipe[1]);
			}
			if (!done_t) {
				srb = swgpReadFdNonblock(bufb, 4 /*tpipe[0]*/, &rrb);
				if (srb < 0 || (srb && !rrb)) done_t++;
			}
			if (rrb > 0) {
				write_log(G, tlinebuf, logspec, ofd, efd, bufb, rrb, vlevel, ppipe[1]);
			}
		}
		status = 0;
		if (g_logger_sigterm) {
			switch(g_logger_sigterm) {
				case SIGTERM: /* deprecated, dead case */
				case SIGUSR2:
					strob_sprintf(tmp, 0, "logger process terminated normally on SIGUSR2.\n",
						swlib_utilname_get());
					break;
				case SIGABRT:
					status = 1;
					strob_sprintf(tmp, 0, "logger process: abnormal termination on SIGABRT.\n",
						swlib_utilname_get());
					break;
				default:
					status = 2;
					strob_sprintf(tmp, 0, "logger process: abnormal termination: signum=%d\n",
						swlib_utilname_get(), (int)g_logger_sigterm);
					break;
			}
			swlib_doif_writef(vlevel, status ? 1: SWC_VERBOSE_6, logspec, efd, "%s", strob_str(tmp));
		} else {
			swlib_doif_writef(vlevel, SWC_VERBOSE_8, logspec, efd, "logger unexpected exit.\n");
		}
		swc_flush_logger(G, slinebuf, tlinebuf, efd, ofd, logspec, vlevel);
		_exit(status);
	}
	strob_close(slinebuf);
	strob_close(tlinebuf);
	if (s_efd) close(spipe[0]);
	close(tpipe[0]);
	if (ppipe[1] >= 0) close(ppipe[1]);
	if (swi_event_fd_p) *swi_event_fd_p = ppipe[0];  /* this contains the events SW* and SWI */
	if (s_efd) *s_efd = spipe[1];
	*t_efd = tpipe[1];
	return pid;
}

char *
swc_get_pax_read_command(struct g_pax_read_command g_pax_read_commands[],
			char * keyid, int verbose_level,
			int keep, char * paxdefault)
{
	int i;
	char * id;
	char * ret;
	struct g_pax_read_command * readcmd;
	
	E_DEBUG2("BEGIN struct=%p", (void*)g_pax_read_commands);
	E_DEBUG2("BEGIN keyid=%s", keyid);
	E_DEBUG2("BEGIN verbose_level=%d", verbose_level);
	E_DEBUG2("BEGIN keep=%d", keep);
	E_DEBUG2("BEGIN paxdefault=%s", paxdefault);
	if (! keyid) {
		return NULL;
	}

	if (keyid == NULL || strlen(keyid) == 0) {
		E_DEBUG("strlen(keyid) == 0");
		keyid = DEFAULT_PAX_R;
	}
	i = 0;
	readcmd = &(g_pax_read_commands[i]);
	id = readcmd->idM;
	while (id) {
		E_DEBUG("while..");
		if (strcmp(keyid, id) == 0) {
			E_DEBUG("strcmp(keyid, id) == 0");
			ret = NULL;
			if (keep) {
				E_DEBUG("");
				ret = readcmd->keep_commandM;
			}
			if (ret == NULL && verbose_level <= 0) {
				E_DEBUG("");
				ret = readcmd->commandM;
			}
			if (ret == NULL) {
				E_DEBUG("");
				ret = readcmd->verbose_commandM;
			}
			E_DEBUG2("return pax command [%s]", ret);
			return ret;
		}
		i++;
		readcmd = &(g_pax_read_commands[i]);
		id = readcmd->idM;
	}
	return swc_get_pax_read_command(
				g_pax_read_commands,
				paxdefault, 
				verbose_level, keep, DEFAULT_PAX_R);
}

char *
swc_get_pax_write_command(struct g_pax_write_command g_pax_write_commands[],
			char * keyid, int verbose_level, char * paxdefault)
{
	int i = 0;
	char * id;
	struct g_pax_write_command * writecmd;

	if (! keyid) return NULL;
	
	if (strlen(keyid) == 0) {
		return swc_get_pax_write_command(g_pax_write_commands, DEFAULT_PAX_W, 
						verbose_level, DEFAULT_PAX_W);
	}
	writecmd = &(g_pax_write_commands[i]);
	id = writecmd->idM;
	while (id) {
		if (strcmp(keyid, id) == 0)
			return writecmd->commandM;
		i++;
		writecmd = &(g_pax_write_commands[i]);
		id = writecmd->idM;
	}

	return swc_get_pax_write_command(g_pax_write_commands, paxdefault, 
					verbose_level, DEFAULT_PAX_W);
}

char *
swc_get_pax_remove_command(struct g_pax_remove_command g_pax_remove_commands[],
			char * keyid, int verbose_level, char * paxdefault)
{
	int i = 0;
	char * id;
	struct g_pax_remove_command * removecmd;

	if (! keyid) return NULL;
	
	if (strlen(keyid) == 0) {
		return swc_get_pax_remove_command(g_pax_remove_commands, DEFAULT_PAX_REM, 
						verbose_level, DEFAULT_PAX_REM);
	}
	removecmd = &(g_pax_remove_commands[i]);
	id = removecmd->idM;
	while (id) {
		if (strcmp(keyid, id) == 0) return removecmd->commandM;
		i++;
		removecmd = &(g_pax_remove_commands[i]);
		id = removecmd->idM;
	}

	return swc_get_pax_remove_command(g_pax_remove_commands, paxdefault, 
					verbose_level, DEFAULT_PAX_REM);
}

int
swc_process_swoperand_file(SWLOG * swutil,
		char * type_string,
		char * filename, 
		int * p_stdin_in_use,
		int * p_array_index,
		int * fd_array)
{
	int list_fd;
	if (strcmp(filename, "-") == 0) {
		(*p_stdin_in_use)++;
		list_fd = STDIN_FILENO;
	} else {
		list_fd = open(
			filename,
			O_RDONLY, 0);
		if (list_fd < 0) {
			swutil_doif_writef2(swutil, swutil->swu_fail_loudlyM,
				"error opening %s list file\n", type_string);
				return -1;
		}
	}
	if (*p_array_index >= 
			SWC_TARGET_FD_ARRAY_LEN - 2) {
		swutil_doif_writef2(swutil, swutil->swu_fail_loudlyM,
			"too many %s files\n", type_string);
		return -1;
	}
	fd_array[(*p_array_index)++] = list_fd;
	fd_array[(*p_array_index)] = -1;
	return 0;
}

void
swc_set_boolean_x_option(struct extendedOptions * opta, 
		enum eOpts nopt, 
		char * arg, 
		char ** optionp)
{
	set_opta(opta, nopt, arg);
}

int
swc_write_source_copy_script(GB * G,
	int ofd, 
	char * sourcepath, 
	int do_get_file_type, 
	int verbose_threshold,
	int delaytime,
	int nhops,
	char * pax_write_command_key,
	char * hostname,
	char * blocksize
	)
{
	int ret;
	int n_source_specs;
	char * dirname;
	char * spath;
	char * pax_write_command;
	char * xx;
	STROB * buffer, *opt1, *opt2;
	STROB * buffer_new;
	STROB * subsh, *subsh2;
	STROB * tmp;
	STROB * tmp1;
	STROB * tmp2;
	STROB * mpath;
	STROB * set_vx;
	STROB * to_devnull;
	STROB * rootdir;
	STROB * firstpath;
	STROB * shell_lib_buf;
	int vlv;

	vlv = G->g_verboseG;

	/*
	 * error for source script is >0
	 */
	if (strstr(sourcepath, "..") == sourcepath) { return 1; }
	if (strstr(sourcepath, "../")) { return 1; }
	if (swlib_is_sh_tainted_string(sourcepath)) { return 1; }

	/*
	 * Squash the trailing slash.
	 */
	swlib_squash_double_slash(sourcepath);
	if (strlen(sourcepath) > 1) {
		if (sourcepath[strlen(sourcepath) - 1] == '/' ) {
			sourcepath[strlen(sourcepath) - 1] = '\0';
		}
	}

	buffer = strob_open(1);
	buffer_new = strob_open(1);
	to_devnull = strob_open(10);
	opt1 = strob_open(1);
	opt2 = strob_open(1);
	set_vx = strob_open(10);
	tmp = strob_open(10);
	tmp1 = strob_open(10);
	tmp2 = strob_open(10);
	subsh = strob_open(10);
	subsh2 = strob_open(10);
	rootdir = strob_open(10);
	firstpath = strob_open(10);
	shell_lib_buf = strob_open(10);

	if (vlv >= SWC_VERBOSE_SWIDB) {
		strob_strcpy(set_vx, "set -vx\n");
	}
	
	if (vlv <= verbose_threshold ) {
		strob_strcpy(to_devnull, "2>/dev/null");
	}
	
	swlib_dirname(tmp, sourcepath);
	dirname = strdup(strob_str(tmp));

	swlib_basename(tmp, sourcepath);
	spath = strdup(strob_str(tmp));

	if (do_get_file_type) {
		if (strchr(spath, '\n') || 
			strlen(spath) > MAX_CONTROL_MESG_LEN - 
				strlen(SWBIS_SWINSTALL_SOURCE_CTL_ARCHIVE ":")
		) {
			/*
			 * Too long, or spath Contains a newline char.
			 */
			return 1;
		}
		/*
		 * this is the control message.
		 */
		strob_sprintf(opt1, 0, 
		"echo " "\"" SWBIS_SWINSTALL_SOURCE_CTL_ARCHIVE ": %s\"",
			spath
			);
		strob_sprintf(opt2, 0, 
		"echo " "\""SWBIS_SWINSTALL_SOURCE_CTL_DIRECTORY ": %s\"",
			 spath
			);
	}
	
	pax_write_command = swc_get_pax_write_command(G->g_pax_write_commands, pax_write_command_key,
						G->g_verboseG, DEFAULT_PAX_W);

	if (swlib_is_sh_tainted_string(spath)) { return 1; }
	if (swlib_is_sh_tainted_string(dirname)) { return 1; }

	if (
		strcmp(spath, "/") == 0 &&
		strcmp(dirname, "/") == 0
	) {
		free(spath);
		spath = strdup(".");	
	}

	if (G->g_sourcepath_listM)
		n_source_specs = strar_num_elements(G->g_sourcepath_listM);
	else
		n_source_specs = 1;

	if (n_source_specs > 1) {
		E_DEBUG("n_source_specs > 1");
		mpath = swc_make_multiplepathlist(G->g_sourcepath_listM, rootdir, firstpath);
		if (mpath) {
			dirname = strdup(strob_str(rootdir));
			spath = strdup(strob_str(firstpath));
		}
	} else {
		E_DEBUG("n_source_specs not > 1");
		mpath = NULL;
	}

	if (n_source_specs > 1) {
		E_DEBUG("");
		if (mpath == NULL) {
			E_DEBUG("");
			n_source_specs = 1;
		}
	}

	strob_sprintf(tmp1, 0, "%s", spath),
	strob_sprintf(tmp2, 0, "%s", sourcepath),

	strob_strcpy(buffer_new, "");

	if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), "detect") == 0) {
		swpl_bashin_detect(buffer_new);
	} else if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), "posix") == 0) {
		swpl_bashin_posixsh(buffer_new);
	} else {
		swpl_bashin_testsh(buffer_new, get_opta(G->optaM, SW_E_swbis_shell_command));
	}

	/* ret = swlib_writef(ofd, buffer,  */
	strob_sprintf(buffer_new, STROB_DO_APPEND,
		"echo " SWBIS_TARGET_CTL_MSG_125 ": " KILL_PID "\n"
		CSHID
		"%s\n"
		"%s"
		"%s"
		"%s\n"			/* function:  shls_check_for_gnu_tar */
		"do_create=\"%s\"\n"
		"multiple_source=\"%s\"\n"
		"blocksize=\"%s\"\n"
		"dirname=\"%s\"\n"
		"path=\"%s\"\n"
		"sourcepath=\"%s\"\n"
		"sw_retval=0\n"
		"sb__delaytime=%d\n"
		"pax_write_command_key=\"%s\"\n"
		CSHID
		"if test -d \"$dirname\"; then\n"
		"	cd \"$dirname\"\n"
		"	sw_retval=$?\n"
		"	case $sw_retval in\n"
		"	0)\n"
		"	if test \"$multiple_source\" -o \\( -d \"$path\" -a -r \"$path\" \\) -o \\( \"$do_create\" -a -r \"$path\" \\); then\n"
		"		\t\t%s\n"
		"		\t\t%s\n"
		"		\t\t%s\n"
		"	# disable test for gnu tar when the selected tar command\n"
		"	# is tar, this preserves ability to only use generic tar options\n"
		"	# and avoid addition of the --format=pax string\n"
		"		case \"${pax_write_command_key}\" in\n"
		"		tar)\n"
		"		( exit 1 ) # false\n"
		"		;;\n"
		"		*)\n"
		"		shls_check_for_gnu_tar \n"
		"		;;\n"
		"		esac\n"
		"		has_gnu_tar=$?\n"
		"		case \"$multiple_source\" in\n"
		"			\"\")\n"
		"				case \"$has_gnu_tar\" in \n"
		"				0)  # Yes, has gnu tar as tar\n"
		"				tar cbf 1 - --format=pax \"$path\"\n"
		"				sw_retval=$?\n"
		"				;;\n"
		"				*)\n"
		"				%s \"$path\"\n"
		"				sw_retval=$?\n"
		"				;;\n"
		"				esac\n"
		"			;;\n"
		"			x)\n"
		"				case \"$has_gnu_tar\" in \n"
		"				0)  # Yes, has gnu tar as tar\n"
		"				tar cbf 1 - --format=pax %s\n"
		"				sw_retval=$?\n"
		"				;;\n"
		"				*)\n"
		"				%s %s\n"
		"				sw_retval=$?\n"
		"				;;\n"
		"				esac\n"
		"			;;\n"
		"		esac\n"
				"\t\t%s\n"
		"	elif test -e \"$path\" -a -r \"$path\"; then\n"
				"\t\t%s\n"
				"\t\t%s\n"
				"\t\t%s\n"
		"		dd ibs=\"$blocksize\" obs=512 if=\"$path\" %s;\n"
		"		sw_retval=$?\n"
				"\t\t%s\n"
		"	else\n"
				"\t\t%s\n"
				"\t\t%s\n"
		"		sw_retval=1\n"
		"	fi\n"
		"	;;\n"
		"	*)\n"
			"\t\t%s\n"
			"\t\t%s\n"
		"	;;\n"
		"	esac\n"

		CSHID
		"elif test -e \"$sourcepath\" -a -r \"$sourcepath\"; then\n"
			"\t%s\n"
			"\t%s\n"
			"\t%s\n"
		"	dd ibs=\"$blocksize\" obs=512 if=\"$sourcepath\" %s;\n"
		"	sw_retval=$?\n"
			"\t%s\n"

		CSHID
		"else\n"
			"\t%s\n"
			"\t%s\n"
		"	sw_retval=1\n"
		"fi\n"

		"if test \"$sw_retval\" != \"0\"; then\n"
		"	 sb__delaytime=0;\n"
		"fi\n"
		"sleep \"$sb__delaytime\"\n"
		"%s\n"
		"%s\n"
		,
		SEVENT(2, vlv, SW_SESSION_BEGINS, ""),
		swicol_subshell_marks(subsh, "source", 'L', nhops, vlv),	
		strob_str(set_vx),
		shlib_get_function_text_by_name("shls_check_for_gnu_tar", shell_lib_buf, NULL),
		(G->g_do_createM != 0) ? "x" : "",
		(n_source_specs > 1) ? "x" : "",
		blocksize,
		dirname,
		spath,
		sourcepath,
		delaytime,
		pax_write_command_key,
		strob_str(opt2),
		SEVENT(1, -1, SW_SOURCE_ACCESS_BEGINS, ""),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_BEGINS, ""),
		pax_write_command,
		mpath ? strob_str(mpath) : ".",
		pax_write_command,
		mpath ? strob_str(mpath) : ".",
		SEVENT(2, vlv, SW_SOURCE_ACCESS_ENDS, "status=0"),
		strob_str(opt1),
		SEVENT(1, -1, SW_SOURCE_ACCESS_BEGINS, ""),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_BEGINS, ""),
		strob_str(to_devnull),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_ENDS, "status=0"),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_ERROR, strob_str(tmp1)),
		SEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, strob_str(tmp1)),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_ERROR, strob_str(tmp1)),
		SEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, strob_str(tmp1)),
		strob_str(opt1),
		SEVENT(1, -1, SW_SOURCE_ACCESS_BEGINS, ""),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_BEGINS, ""),
		strob_str(to_devnull),
		SEVENT(2, vlv, SW_SOURCE_ACCESS_ENDS, "status=0"),
		SEVENT(2, vlv,  SW_SOURCE_ACCESS_ERROR, strob_str(tmp2)),
		SEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, strob_str(tmp2)),
		SEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
		swicol_subshell_marks(subsh2, "source", 'R', nhops, vlv)
		);

	xx = strob_str(buffer_new);
	ret = atomicio((ssize_t (*)(int, void *, size_t))write, ofd, xx, strlen(xx));
	if (ret != (int)strlen(xx)) {
		return 1;
	}

	if (G->g_source_script_name) {
		E_DEBUG2("writing source script: %s", G->g_source_script_name);
		swlib_tee_to_file(G->g_source_script_name, -1, xx, -1, 0);
	}

	free(spath);
	free(dirname);
	strob_close(tmp);
	strob_close(tmp1);
	strob_close(tmp2);
	strob_close(buffer);
	strob_close(buffer_new);
	strob_close(opt1);
	strob_close(opt2);
	strob_close(set_vx);
	strob_close(to_devnull);
	strob_close(subsh);
	strob_close(subsh2);
	strob_close(rootdir);
	strob_close(shell_lib_buf);
	if (mpath) strob_close(mpath);
	/*
	 * 0 is OK
	 * !0 is error
	 */
	return !(ret > 0);
}

int
swc_read_target_message_129(GB * G, int fd, STROB * cdir, 
				int vlevel, char * loc)
{
	int ret;
	STROB * control_message;	
	char * tag;
	char * d;
	
	tag = SWBIS_TARGET_CTL_MSG_129 ": ";
	control_message = strob_open(32);	
	strob_strcpy(cdir, "");
	ret = swc_read_target_ctl_message(G, fd, control_message, vlevel, loc);
	if (ret < 0) return -1;

	if (strstr(strob_str(control_message), tag) == NULL)
		return -1;

	d = strob_str(control_message) + strlen(tag);
	strob_strcpy(cdir, d);
	swlib_squash_all_trailing_vnewline(strob_str(cdir));
	strob_close(control_message);
	return 0;
}

int
swc_read_target_ctl_message(GB * G, int fd, STROB * control_message, 
				int vlevel, char * loc)
{
	int n;
	char c[2];
	int count = 0;

	E_DEBUG2("BEGIN loc=%s",loc)
	strob_strcpy(control_message, "");
	c[1] = '\0';
	n = try_nibble(fd, (void*)&c, 3 /* seconds */);
	if (n != 1) {
		E_DEBUG("");
		return -1;
	} else {
		E_DEBUG("");
		count++;
	}
	E_DEBUG("");
	while (n == 1 && c[0] != '\n' && count < MAX_CONTROL_MESG_LEN) {
		E_DEBUG("");
		strob_strcat(control_message, c);
		n = read(fd, (void*)&c, 1);
		E_DEBUG("");
		if (n == 1) count ++;
	}
	E_DEBUG("out of loop");
	swlib_doif_writef(vlevel, SWC_VERBOSE_IDB, &G->g_logspec, G->g_stderr_fd,
		"%s control message: %s\n", 
			loc, strob_str(control_message));
	E_DEBUG("after msg");
	if (n == 1 && count == 1) { 
		E_DEBUG("return 0");
		return 0;
	} else if (n == 1 && count < MAX_CONTROL_MESG_LEN) {
		E_DEBUG2("return count=%d", count);
		return count;
	} else {
		E_DEBUG("return -1");
		return -1;
	}
}

char *
swc_get_target_script_pid(GB * G, STROB * control_message)
{
	char *s;
	char *p;
	char *ed;
	char *ret;

	s = strob_str(control_message);
	p = strstr(s, SWBIS_TARGET_CTL_MSG_125);
	if (!p)
		return (char*)NULL;
	if (p != s)
		return (char*)NULL;
	p+=strlen(SWBIS_TARGET_CTL_MSG_125);
	if (*p != ':')
		return (char*)NULL;
	p++;
	if (*p != ' ')
		return (char*)NULL;
	p++;
	ret = strdup(p);
	if ((ed=strpbrk(ret, "\n\r"))) *ed = '\0';
	return ret;
}

int
swc_read_start_ctl_message(GB * G, int fd, 
	STROB * control_message, 
	STRAR * tramp_list, 
	int vlevel, 
	char ** pscript_pid, 
	char * loc)
{
	int ret = 0;
	E_DEBUG2("START loc=%s", loc);
	strob_strcpy(control_message, "");

	while(ret >= 0 &&
		strstr(strob_str(control_message), SWBIS_TARGET_CTL_MSG_125) == 
							(char*)NULL
	) {
			E_DEBUG("");
			ret = swc_read_target_ctl_message(G, fd, 
						control_message, 
						vlevel, 
						loc);
			E_DEBUG2("ret=%d", ret);
			if (ret >= 0 && strob_strlen(control_message)) {
				E_DEBUG("");
				strar_add(tramp_list, 
					strob_str(control_message));
			}
	}
	if (ret < 0) {
		E_DEBUG("error");
		return ret;
	} 
	*pscript_pid = swc_get_target_script_pid(G, control_message);
	if (*pscript_pid == (char*)NULL) {
		/*
		 * allow this to be OK, A NetBSD PATH=`getconf PATH` sh
		 * did not support $PPID. Use of the PPID is only needed for
		 * multiple ssh-hops, and maybe it shouldn't be needed at all.
		 */
		ret = 0;
	} else {
		swlib_doif_writef(vlevel, SWC_VERBOSE_IDB, &G->g_logspec, G->g_stderr_fd,
			"remote script pid: %s\n", *pscript_pid);
		
	}
	E_DEBUG2("returning %d", ret);
	return ret;
}

int
swc_tee_to_file(char * filename, char * buf)
{
	int ret;
	int fd;
	int res;
	if (isdigit((int)(*filename))) {
		fd = swlib_atoi(filename, &res);
		if (res) return -1;
	} else {
		fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644);
	}
	if (fd < 0) return fd;
	ret = uxfio_unix_safe_write(fd, buf, strlen(buf));
	if (ret != (int)strlen(buf)) return -1;
	close(fd);
	return ret;
}

void
swc_gf_xformat_close(GB * G, XFORMAT * xformat)
{
	if (xformat) xformat_close(xformat);
	G->g_xformat = NULL;
}

int
swc_get_stderr_fd(GB * G)
{
	return G->g_stderr_fd;
}

int
swc_shutdown_logger(GB * G, int signum)
{
	int i;
	int status = 0;
	int time_to_wait = 10; /* seconds */
	int ret;
	int retval;

	E_DEBUG("");
	if (signum == 0) {
		signum = SWBIS_LOGGER_SIGTERM;
	}
	if (G->g_logger_pid > 0) {
		if (kill(G->g_logger_pid, signum) < 0) {
			fprintf(stderr, "%s: kill(2): %s\n",
				swlib_utilname_get(), strerror(errno));
		}
	}
	E_DEBUG("");
	if (G->g_swevent_fd >= 0) close(G->g_swevent_fd);
	E_DEBUG("");
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
			&G->g_logspec, swc_get_stderr_fd(G), 
			"waitpid() on logger_pid [%d]\n", G->g_logger_pid);
	E_DEBUG("");

AGAIN:
	ret = waitpid(G->g_logger_pid, &status, WNOHANG);
	i = 0;
	E_DEBUG("");
	while (ret == 0 && i < time_to_wait) {
		usleep(50000);
		ret = waitpid(G->g_logger_pid, &status, WNOHANG);
		E_DEBUG3("waitpid returned %d status=%d" , ret, status);
		if (ret < 0) return ret;
		i++;
	}
	E_DEBUG("");
	if (ret < 0) return -1;

	if (ret == 0 &&  signum != SIGKILL) {
		E_DEBUG("");
		signum = SIGKILL;
		if (kill(G->g_logger_pid, signum) < 0) {
			E_DEBUG("");
			fprintf(stderr, "%s: kill(2): %s\n",
				swlib_utilname_get(), strerror(errno));
		}
		goto AGAIN;
	}
	E_DEBUG("");
	if (signum != SIGKILL) {
		if (WIFEXITED(status)) {
			retval = WEXITSTATUS(status);
		} else {
			E_DEBUG("");
			retval = -1;
		}
	} else {
		E_DEBUG("");
		retval = -2;
	}
	E_DEBUG("");
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
			&G->g_logspec, swc_get_stderr_fd(G), 
			"finished waitpid() on logger_pid [%d]\n", G->g_logger_pid);

	/* if (G->g_swi_event_fd >= 0) close(G->g_swi_event_fd); */
	E_DEBUG("");
	close(G->g_t_efd);
	close(G->g_s_efd);
	E_DEBUG("");

	E_DEBUG2("retval=%d", retval);
	return retval;
}

void
swc_set_stderr_fd(GB * G, int fd)
{
	G->g_stderr_fd = fd;
	swutil_set_stderr_fd(G->g_swlog, fd);
}

void
swc_stderr_fd2_set(GB * G, int fd)
{
	G->g_save_stderr_fdM = swc_get_stderr_fd(G);	
	swc_set_stderr_fd(G, fd);
}

void
swc_stderr_fd2_restore(GB * G)
{
	swc_set_stderr_fd(G, G->g_save_stderr_fdM);
}

void 
swc_lc_raise(GB * G, char * file, int line, int signo)
{
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
				&G->g_logspec, swc_get_stderr_fd(G), 
					"raising signal %d at %s:%d\n",
						signo, file, line);
	G->g_signal_flag = signo;
	raise(signo);
}

void 
swc_lc_exit(GB * G, char * file, int line, int status)
{
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
			&G->g_logspec, swc_get_stderr_fd(G), 
				"exiting at %s:%d with value %d\n",
						file, line, status);
	exit(status);
}

int
swc_is_option_true(char * s)
{
	return swlib_is_option_true(s);
}

int
sw_exitval(GB * G, int count, int suc)
{
	if (G->g_master_alarm == 0 && suc && count == suc) return 0;
	if (suc && count) return 2;
	if (suc == 0) return 1;
	swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
			&G->g_logspec, swc_get_stderr_fd(G),
			"abnormal exit\n");
	return 255;
}

void
swc_check_for_current_signals(GB * G, int lineno, char * no_remote_kill)
{
	sigset_t  pending_set;
	int signum;
	int ret;

	/* check for pending signals */

	ret = sigpending(&pending_set);
	if (ret == 0) {
		signum = 0;
		if (sigismember(&pending_set, SIGINT) == 1) {
			E_DEBUG("got pending SIGINT");
			signum = SIGINT;
		} else if (sigismember(&pending_set, SIGTERM) == 1) {
			E_DEBUG("got pending SIGTERM");
			signum = SIGTERM;
		}
	
		if (signum) {
			(*G->g_safe_sig_handler)(signum);
		}
	}

	switch(G->g_signal_flag) {
		case SIGUSR1:
			if (G->g_running_signalusr1M) {
				return;
			}
			G->g_running_signalusr1M = 1;
			(*(G->g_main_sig_handler))(G->g_signal_flag);
			break;
		case SIGTERM:
		case SIGPIPE:
		case SIGINT:
			if (G->g_running_signalsetM) {
				return;
			}
			E_DEBUG("");
			swlib_doif_writef(G->g_verboseG,
				SWC_VERBOSE_IDB, &G->g_logspec, swc_get_stderr_fd(G),
			"swc_check_for_current_signals: line=%d Got signal %d.\n",
					 	lineno, G->g_signal_flag);
			E_DEBUG("");
			swgp_signal_block(SIGTERM, (sigset_t *)NULL);
			swgp_signal_block(SIGINT, (sigset_t *)NULL);
			swgp_signal_block(SIGPIPE, (sigset_t *)NULL);
			E_DEBUG("");
			/**************
			Depreicated Code ???
			if (!swc_is_option_true(no_remote_kill)) {
				E_DEBUG("");
				swc_do_run_kill_cmd((SHCMD*)(NULL), 
						G->g_target_kmd, 
						G->g_source_kmd, 
						G->g_verboseG);
			} else {
				E_DEBUG("");
				swlib_doif_writef(G->g_verboseG, 
						SWC_VERBOSE_IDB,
						&G->g_logspec, swc_get_stderr_fd(G),
			"swc_check_for_current_signals:  not running kill cmds.\n");
			}
			**************/
			E_DEBUG("");

			/*
			 *	Call the main handler here
			 */

			G->g_running_signalsetM = 1;	
			(*(G->g_main_sig_handler))(G->g_signal_flag);

			E_DEBUG("");
			break;
	}
}

pid_t
swc_run_source_script(GB * G,
		int swutil_x_mode, 
		int local_stdin, 
		int source_pipe,
		char * source_path,
		int nhops,
		char * pax_write_command_key,
		sigset_t *fork_blockmask,
		sigset_t *fork_defaultmask,
		char * hostname,
		char * blocksize,
		int verbose_threshold,
		int (*fp_script) (GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv, int delaytime,
				int nhops, char * pax_write_command_key, char * hostname, char * blocksize)
		) 
{
	pid_t write_pid;

	if ((swutil_x_mode == 0 || local_stdin)) {
		/*
		* fork not required.
		*/
		return 0;
	}

	write_pid = swndfork(fork_blockmask, fork_defaultmask);
	if (write_pid == 0) {
		int ret_source = 0;
		/*
		 * Write the source scripts.
		 */
		if (swutil_x_mode && local_stdin == 0) {
			/*
			* If source is remote.
			*/
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
					&G->g_logspec,
					swc_get_stderr_fd(G), 
					"running source script.\n");
			/* ret_source = swc_write_source_copy_script(G, */
			ret_source = (*fp_script)(G,
				source_pipe, 
				source_path,
				SWINSTALL_DO_SOURCE_CTL_MESSAGE,
				verbose_threshold /*SWC_VERBOSE_4*/, 
				SWC_SCRIPT_SLEEP_DELAY, 
				nhops,
				pax_write_command_key,
				hostname,
				blocksize);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2,
					&G->g_logspec, swc_get_stderr_fd(G),
				"Source script finished return value=%d.\n",
								ret_source);

		} else {
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
					"Unexpected Dead code excpeption\n");
			swlib_doif_writef(G->g_verboseG,
					SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
		"NOT running source script in child, exiting with %d.\n",
								ret_source);
			LCEXIT(1);
		}
		/*
		 * 0 is OK
		 * !0 is error
		 */
		_exit(ret_source);
	} else if (write_pid < 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
				&G->g_logspec, swc_get_stderr_fd(G),
					"fork error : %s\n", strerror(errno));
	}
	return write_pid;
}

void
swc_helptext_target_syntax(FILE * fp)
{
fprintf(fp, "%s",
"\n"
" Target Syntax:\n"
);

fprintf(fp, "%s",
"   software_selections\n"
"       Refer to the software objects (products, filesets)\n"
"       on which to be operated.\n"
"\n"
"   target\n"
"       Refers to the software_collection where the software\n"
"       selections are to be applied.\n"
"       Target follows syntax of Section 4.1.4.2 (Source and Target\n"
"       Specification and Logic) summarized as follows:\n"
"\n"
"       target : HOST_CHARACTER_STRING ':' PATHNAME_CHARACTER_STRING\n"
"              | HOST_CHARACTER_STRING ':'\n"
"              | HOST_CHARACTER_STRING \n"
"              | PATHNAME_CHARACTER_STRING \n"
"              ; \n"
"\n"
"       PATHNAME_CHARACTER_STRING must be an absolute path unless the\n"
"                                 target has a HOST_CHARACTER_STRING\n"
"                                 in which case it may be a relative path.\n"
"       HOST_CHARACTER_STRING is an IP or hostname or '-'\n"
"          NOTE: A '-' indicating standard output is an implementation\n"
"                    extension.\n"
);

fprintf(fp , "%s",
"\n"
" Implementation Extension Target Syntax:\n"
"\n"
" Multiple Ssh Hop Syntax:\n"
"   Two styles supported, one using @@ as the\n"
"   delimeter and another using a colon\n"
"\n"
"   Delimiter: @@  (double @)\n"
"     [User@]Host@@Host[@@Host]:File\n"
"\n"
"   Delimiter: : (colon)\n"
"     :File\n"
"     [User@]Host\n"
"     [User@]Host:\n"
"     [User@]Host:File\n"
"     [User@]Host:Host:\n"
"     [User@]Host:Host:File\n"
"     [User@]Host:[User@]Host:\n"
"         'Host' may contain an underscore and if it does\n"
"          [since it is illegal in a hostname] is interpreted\n"
"          as host_port\n"
"\n"
);
}

int 
swc_wait_on_script(GB * G, pid_t write_pid, char * location)
{
	/*
	 * wait on the write_script process and check for errors.
	 */
	pid_t ret;
	int status;
	ret = waitpid(write_pid, &status, 0);
	if (ret < 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
				&G->g_logspec, swc_get_stderr_fd(G),
			"%s scripts waitpid : %s\n", 
			location, strerror(errno));
		return -1;
	}
	if (WIFEXITED(status)) {
		return (int)(WEXITSTATUS(status));
	} else {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec,
			swc_get_stderr_fd(G),
			"%s script had abnormal exit.\n", 
			location);
		return -2;
	}
}
	
void
swc_set_shell_dash_s_command(GB * G, char * wopt_shell_command)
{
	STROB * tmp;
	char * sh_dash_s;
	
	tmp = strob_open(32);
	if (
		strcasecmp(wopt_shell_command, SWC_SHELL_KEY_POSIX) == 0 ||
		strcasecmp(wopt_shell_command, SWC_SHELL_KEY_DETECT) == 0
	) {
		wopt_shell_command = SWC_SHELL_KEY_SH;
	}
	sh_dash_s = swssh_landing_command(wopt_shell_command, 0 /*opt_no_getconf*/);
	if (G->g_do_cleanshM == 0) {
		strob_sprintf(tmp, STROB_NO_APPEND, "%s /_swbis /_%s", sh_dash_s, swlib_utilname_get());
	} else {
		/* If running is --cleansh mode don't tag the command with
		   the _swbis args otherwise the it will kill itself */
		strob_sprintf(tmp, STROB_NO_APPEND, "%s", sh_dash_s);
	}

	G->g_sh_dash_s = strdup(strob_str(tmp));
	strob_close(tmp);
}

int
swc_write_swi_debug(SWI * swi, char * filename)
{
	int fd;
	int res;
	char * image;
	if (isdigit((int)(*filename))) {
		fd = swlib_atoi(filename, &res);
		if (res) return -1;
	} else {
		fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644);
		if (fd < 0) return -1;
	}
	image = swi_dump_string_s(swi, "(SWI*)");
	if (!image) return -1;
	res = uxfio_unix_safe_write(fd, image, strlen(image));
	close(fd);
	return res;
}

char * 
swc_convert_multi_host_target_syntax(GB * G, char * target_spec)
{
	char * retval;
	char * tmp_target_spec;
	char * first;
	char * second;
	char * s;
	int len;
	int cnt;
	int is_a_host;
	int is_a_file;

	retval = strdup(target_spec);	
	if (
		(first = strchr(target_spec, ':')) != NULL &&
		(second = strchr(first+1, ':')) != NULL
	) {
		/* Two or more ':', this is a multi hop spec using the
		   Ssh scp-like syntax */
		if (
			first == target_spec &&
			second == target_spec + strlen(target_spec) - 1
		) {
			/* Its  :HOST: */
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec,
				swc_get_stderr_fd(G),
				"warning: assuming %s is a host\n", target_spec);

			*second = '\0';
			retval = strdup(first+1);
			return retval;	
		} else {
			/* Its  HOST:HOST:  ...
			   a trailing ':' means its a host */

			/* Scan the string backwards */
			len = strlen(target_spec);	

			is_a_host = 0;
			is_a_file = 0;
			cnt = len;
			tmp_target_spec = retval;
			s = tmp_target_spec + (len - 1);
			while(cnt > 0) {
				s = tmp_target_spec + (cnt - 1);
				if (cnt == len) {
					/* Check last char */
					if (*s == ':') {
						is_a_host = 1;
						*s = '\0';
					} else {
						is_a_file = 1;
					}
					/* squash this ':' while we have a chance */
				} else if (*s == ':') {
					if (is_a_host) {
						*s = '\r'; /* placeholde for @@ */
					} else {
						/* Leave the ':' it indicates a file,
						   all subsequent matches are hosts */
						is_a_host = 1;
					}
				}
				cnt--;
			}
			/* Now, convert the '\r' to "@@" */
			{
				STROB * tmp;
				tmp = strob_open(14);
				s = tmp_target_spec;
				while(*s) {
					if (*s == '\r') {
						strob_strcat(tmp, "@@");
					} else {
						strob_charcat(tmp, (int)(*s));
					}
					s++;
				}	
				retval = strdup(strob_str(tmp));
				strob_close(tmp);
				return retval;
			}
		}
	} else {
		/* Only one ':', normal usage */
		return retval;
	}
	return retval;  /* this should never happen */
}

int
sw_e_msg(GB * G, char * format, ...)
{
        int ret;
        va_list ap;
        va_start(ap, format);
        ret = doif_i_writef(G->g_verboseG, G->g_fail_loudly,
                        &(G->g_logspec), swc_get_stderr_fd(G), format, &ap);
        va_end(ap);
        return ret;
}

int
sw_l_msg(GB * G, int at_level, char * format, ...)
{
        int ret;
        va_list ap;
        va_start(ap, format);
        ret = doif_i_writef(G->g_verboseG, at_level,
                        &(G->g_logspec), swc_get_stderr_fd(G), format, &ap);
        va_end(ap);
        return ret;
}

int
sw_d_msg(GB * G, char * format, ...)
{
	int dl;
        int ret;
        va_list ap;
        va_start(ap, format);
	
	dl = G->g_verboseG;
	if (G->devel_verboseM)
		dl = SWC_VERBOSE_IDB2;
        ret = doif_i_writef(dl, SWC_VERBOSE_IDB2,
                        &(G->g_logspec), swc_get_stderr_fd(G), format, &ap);
        va_end(ap);
        return ret;
}

void
swc_check_shell_command_key(GB * G, char * cmd)
{
	char * wopt_shell_command = cmd;
	if (
		strcmp(wopt_shell_command, SWC_SHELL_KEY_BASH) &&
		strcmp(wopt_shell_command, SWC_SHELL_KEY_SH) &&
		strcmp(wopt_shell_command, SWC_SHELL_KEY_KSH) &&
		strcmp(wopt_shell_command, SWC_SHELL_KEY_MKSH) &&
		strcmp(wopt_shell_command, SWC_SHELL_KEY_DETECT) &&
		strcmp(wopt_shell_command, SWC_SHELL_KEY_POSIX)
	) {
		sw_e_msg(G, "illegal shell command: %s \n",
			wopt_shell_command);
		exit(1);
	}
}

void
swc_check_all_pax_commands(GB * G)
{
	char * command;
	char * xcmd;
	char * type;

	type = "write";
	command = get_opta(G->optaM, SW_E_swbis_local_pax_write_command),
	xcmd = swc_get_pax_write_command(G->g_pax_write_commands, command, 1, (char*)NULL);
	if (!xcmd) {sw_e_msg(G, "illegal pax %s command: %s \n", type, command); LCEXIT(1); }

	command = get_opta(G->optaM, SW_E_swbis_remote_pax_write_command);
	xcmd = swc_get_pax_write_command(G->g_pax_write_commands, command, 1, (char*)NULL);
	if (!xcmd) {sw_e_msg(G, "illegal pax %s command: %s \n", type, command); LCEXIT(1); }

	type = "read";
	command = get_opta(G->optaM, SW_E_swbis_remote_pax_read_command);
	xcmd = swc_get_pax_write_command(G->g_pax_write_commands, command, 1, (char*)NULL);
	if (!xcmd) {sw_e_msg(G, "illegal pax %s command: %s \n", type, command); LCEXIT(1); }

	command = get_opta(G->optaM, SW_E_swbis_local_pax_read_command);
	xcmd = swc_get_pax_write_command(G->g_pax_write_commands, command, 1, (char*)NULL);
	if (!xcmd) {sw_e_msg(G, "illegal pax %s command: %s \n", type, command); LCEXIT(1); }
}

void
swc_process_pax_command(GB * G, char * command, char * type, int sw_e_num)
{
	char * xcmd;

	if (strcmp(type, "write") == 0) {
		xcmd = swc_get_pax_write_command(
			G->g_pax_write_commands,
			command,
			G->g_verboseG, (char*)NULL);
	} else if (strcmp(type, "read") == 0) {
		xcmd = swc_get_pax_read_command(
			G->g_pax_read_commands,
			command,
			0, 0, (char*)NULL);
	} else if (strcmp(type, "remove") == 0) {
		xcmd = swc_get_pax_remove_command(
			G->g_pax_remove_commands,
			command, 0, (char*)NULL);
	} else {
		xcmd = NULL;
		sw_e_msg(G, "internal error\n");
		LCEXIT(1);
	}
	if (xcmd == NULL) {
		sw_e_msg(G, "illegal pax %s command: %s \n",
			type, command);
			LCEXIT(1);
	}
	set_opta(G->optaM, sw_e_num, command);
}

char *
swc_get_default_sh_dash_s(GB * G)
{
	if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), SWC_SHELL_KEY_DETECT) == 0)
		return SWSSH_DETECT_POISON_DEFAULT;
	else
		return G->g_sh_dash_s;	
}
