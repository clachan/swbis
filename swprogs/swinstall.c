/*  swinstall.c -- The swbis swinstall utility. 

 Copyright (C) 2004,2005,2006,2007,2008,2010 Jim Lowe
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vplob.h"
#include "swlib.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swinstall.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swevents.h"
#include "etar.h"
#include "swicol.h"
#include "swutillib.h"
#include "swmain.h"  /* this file should be included by the main program file */
#include "swproglib.h"
#include "swicat.h"
#include "swicat_e.h"
#include "globalblob.h"

#define PROGNAME		"swinstall"
#define SW_UTILNAME		PROGNAME
#define SWPROG_VERSION 		SWINSTALL_VERSION

#define WOPT_LAST		241
#define WOPT_FIRST		150

#define DEFAULT_PUMP_DELAY	0 		/* OLD default 1000100 */ /* 1/1000th sec */
#define DEFAULT_BURST_ADJUST	4000		/* similar to OpenSSH rcvd adjust ??? */

static char progName[] = PROGNAME;
static char * CHARTRUE = "True";
static char * CHARFALSE = "False";

static GB g_blob; 
static GB * G; 

#define TA_UUID_OFFSET		4  /* i.e.:  {E|C|P}:<result>:<uuid>
					e.g.   P:0:31  where 31 is the uuid */

#define SL_LT_INDEX		0	/* samepackage revision less than */
#define SL_EQ_INDEX		1	/* samepackage revision equal */
#define SL_GT_INDEX		2	/* samepackage revision greater than */
#define SL_EQ_ANY_LOC_INDEX	3	/* samepackage revision equal with any location */
#define SL_FIRST_DEP_INDEX	4	/* First dependency spec */

static 
void 
version_string(FILE * fp)
{
	fprintf(fp,  
"%s (swbis) version " SWINSTALL_VERSION "\n",
		progName);
}

static
void 
version_info(FILE * fp)
{
	version_string(fp);
	/* fprintf(fp,  "\n"); */
	swc_copyright_info(fp);
}

static
int 
usage(FILE * fp, 
		char * pgm, 
		struct option  lop[], 
		struct ugetopt_option_desc helpdesc[])
{
	
fprintf(fp, "%s",
"swinstall installs a directory or serial archive\n"
"to a target location.  The source and target may be different hosts.\n"
"A source distribution must be a valid posix package.\n"

"By default, when a new package is installed (i.e. not an upgrade\n"
"or reinstall) conflicting files in the file system cause a rejection\n"
"of the installation.  To override use --replacefiles or --keepfiles\n"

);
	
	fprintf(fp, "%s",
"\n"
"Usage: swinstall [-p] [-r] [-f file] [-s source] [-x option=value]\\\n"
"      [-X options_file] [-W option[=value]] [-t targetfile] [-c catalog]\\\n"
"      [impl_specific_options] [software_selections][@target]\n"
"\n"
"Examples:\n"
"  swinstall -s -               # install a package from stdin to /\n"
"\n"
"  swinstall l=/packages        # install to a location /packages \n"
"\n"
"  # Command with remote target\n"
"  swinstall -s :somepackage-1.1.tar.gz @ 192.168.1.1\n"
"\n"
"  # Write the payload to stdout\n"
"  swinstall @- | tar tvf - \n"
"\n"
"  # Command with multiple targets\n"
"  swinstall -s:somepkg-1.1.tar.gz @ 192.168.1.1:/tmp/newdir/ root@192.168.1.2\n"
);

fprintf(fp, "%s",
"\n"
"Options:\n"
);
	ugetopt_print_help(fp, pgm, lop, helpdesc);

fprintf(fp , "%s",
"\n"
"Implementation Extension Options:\n"
"       Syntax : -W option[=option_argument] [-W ...]\n"
"           or   -W option[=option_argument],option...\n"
"           or   --option[=option_argument]\n"
"\n"
"   Alternate Modes:\n"
"\n"
"      --show-options  Show the extended options and exit\n"
"      --show-options-files   Show the options files that will be parsed.\n"
"      --no-defaults-files  Do not read any defaults files.\n"
"      --to-stdout   write the payload to stdout in the form of a tar archive\n"
"      --preview-tar-file=FILE  Writes the actual fileset archive to FILE.\n"
"      --slackware  recognize slackware tarballs for installation\n"
"\n"
"   Operational Options:\n"
"\n"
"      --noop       Legal option with no effect\n"
"      --reinstall  Same as -x reinstall=yes\n"
"      --enforce-scripts Same as -x enforce_scripts=yes\n"
"      --no-enforce-scripts Same as -x enforce_scripts=no\n"
"      --justdb  install just the catalog, not any payload files\n"
"      --force  ignore downdate, prerequisites, reinstall etc.\n"
"      --force-locks  ignore locking, and remove existing lock.\n"
"      --noscripts  Same as --ignore-scripts, --no-scripts\n"
"      --nodeps  Same as -x enforce_dependencies=false\n"
"      --nosignature  Same as --sig-level=0\n"
"      --enforce-all-signatures -Wswbis_enforce_all_signatures=true\n"
"      --ignore-scripts  Ignore (do not run) control scripts\n"
"      --keep-volatile-files  Install with a suffix e.g. <name>.swbisnew\n"
"      --keep-old-files   Do not overwrite existing files\n"
"			The new (incoming) file is not installed).\n"
"      --keepfiles   (same as keep-old-files)\n"
"      --replacefiles  replace files that may belong to other packages\n"
"      --sig-level=N  N=0,1, or 2,.. number of sigs required, 0 is none\n"
"      --sign  Add a signature to the package if already signed\n"
"      --no-chdir-for-scripts  version 1.11 compatability option; do not\n"
"                  chdir to $SW_ROOT for script execution \n"
"      --allow-rpm  (depreceated, has no effect) Allow installation of\n"
"                    RPM format packages.\n"
"      --allow-missing-files  Allow some files to be missing from storage\n"
"           section. (usually only happens from a translated RPM package).\n"

"\n"
"   Fileset loading Options:\n"
"\n"
"      H, --format=FORMAT generate or re-write archive of given format\n"
"             FORMAT is one of gnu, pax (or posix), or ustar (default)\n"
"      --pax-command={tar|pax|star|gtar}\n"
"      --pax-read-command={tar|pax|star|gtar}\n"
"      --local-pax-read-command={tar|pax|star|gtar}\n"
"      --remote-pax-read-command={tar|pax|star|gtar}\n"
"              pax  = \"pax -r\"\n"
"              star = \"star xpUf -\"\n"
"              tar  = \"tar xpf -\"\n"
"              gtar = \"tar xpf - --overwrite\"\n"
"      --pax-write-command={tar|pax|star|gtar}\n"
"      --local-pax-write-command={tar|pax|star|swbistar}\n"
"      --remote-pax-write-command={tar|pax|star|swbistar}\n"
"              pax  = \"pax -w\"\n"
"              star = \"star cf - -H ustar\"\n"
"              tar  = \"tar cf -\"\n"
"              gtar = \"gtar cf -\"\n"
"              swbistar = \"" SWBISLIBEXECDIR "/swbis/swbistar\"\n"
" 	The default is \"tar\"\n"
"\n"
"  Target and Remote connection options\n"
"\n"
"      --remote-shell=NAME  NAME is the client \"rsh\" or \"ssh\"(default).\n"
"      --shell-command={detect|posix|sh|bash|ksh} detect is the default\n"
"      --use-getconf  This causes getconf to be used.  Opposite to --no-getconf.\n"
"      --no-getconf  Don't use the getconf utility to set the PATH variable on the\n"
"               target host. This sets the remote command to be '/bin/sh -s'\n"
"               instead of the default \"PATH=`getconf PATH` sh -s\"\n"
"      --no-remote-kill  Do not execute a kill command. Normally a kill command\n"
"               is issued when signals TERM, PIPE, and INT are received for\n"
"               targets where nhops > 1.\n"
"   -A,--A,--enable-ssh-agent-forwarding\n"
"      --a,--disable-ssh-agent-forwarding\n"
"\n"
"  Bug/workaround options\n"
"\n"
"      --pump-delay (same as pump-delay1).\n"
"      --pump-delay1=NANOSECONDS Adds delay every burst adjust bytes.\n"
"                           Default is 1/1000th sec for >= 1-hop targets.\n"
"      --burst-adjust=SIZE  default is 9000 (for >= 1-hop targets)\n"
"      --ssh-options=options  Single letter ssh options. e.g. --ssh-options=1\n"
"\n"
"  Verbosity options\n"
"\n"
"   -v,--verbose   give one or more times.\n"
"      --quiet-progress      disable progress bar.\n"
"      --show-progress-bar   enable progress bar.\n"
"      --progress-bar-fd={1|2}  1 (stdout) is the default\n"
"\n"
"  Debugging Verbosity options\n"
"\n"
"      --debug-verbose \n"
"      --debug-task-scripts   write the task scripts to files in /tmp\n"
"      --debug-events  Show the internal events listing to stderr\n"
"      --swi-debug-name=NAME  write a ascii dump of the the internal package object\n"
"      --source-script-name=NAME  write the internal stdin script to NAME.\n"
"      --target-script-name=NAME  write the internal stdin script to NAME.\n"
"                           NAME may be a number, for example 2 meaning stderr\n"
"\n"
);

      swc_helptext_target_syntax(fp);

fprintf(fp, "%s",
"\n"
"Posix Extended Options:\n"
"        File : <libdir>/swbis/swdefaults\n"
"               ~/.swdefaults\n"
"        Override values in defaults file.\n"
"        Command Line Syntax : -x option=option_argument [-x ...]\n"
"            or : -x \"option=option_argument  ...\"\n"
"   allow_downdate              = false      # Not Implemented\n"
"   allow_incompatible          = false      # Not Implemented\n"
"   ask                         = false      # Not Implemented\n"
"   autoreboot                  = false      # Not Implemented\n"
"   autorecover                 = false      # Not Implemented\n"
"   autoselect_dependencies     = false      # Not Implemented\n"
"   defer_configure             = false      # Not Implemented\n"
"   distribution_source_directory   = -	     # Stdin\n"
"   enforce_dependencies        = false      # Not Implemented\n"
"   enforce_locatable           = false      # Not Implemented\n"
"   enforce_scripts             = false      # Not Implemented\n"
"   enforce_dsa                 = false      # Not Implemented\n"
"   installed_software_catalog  = var/lib/swbis/catalog\n"
"   logfile                     = /var/log/swinstall.log\n"
"   loglevel                    = 0\n"
"   reinstall                   = false      \n"
"   select_local		= false      # Not Implemented\n"
"   verbose			= 1\n"
"\n"
"Swbis Extended Options:\n"
"        File : <libdir>/swbis/swbisdefaults\n"
"               ~/.swbis/swbisdefaults\n"
"        Command Line Syntax : -Woption=option_argument\n"
"  swinstall.swbis_no_getconf		    = true\n"
"  swinstall.swbis_shell_command	    = detect\n"
"  swinstall.swbis_no_remote_kill	    = true\n"
"  swinstall.swbis_quiet_progress_bar       = true\n"
"  swinstall.swbis_local_pax_write_command  = tar\n"
"  swinstall.swbis_remote_pax_write_command = tar\n"
"  swinstall.swbis_local_pax_read_command   = tar\n"
"  swinstall.swbis_remote_pax_read_command  = tar\n"
"  swinstall.swbis_enforce_all_signatures   = false\n"
"  swinstall.swbis_replacefiles             = false\n"
"  swinstall.swbis_enforce_file_md5         = false\n"
"  swinstall.swbis_allow_rpm                = true\n"
"  swinstall.swbis_any_format               = true\n"
"  swinstall.swbis_remote_shell_client	    = ssh\n"
"  swinstall.swbis_install_volatile	    = true\n"
"  swinstall.swbis_volatile_newname	    = \"\"  # empty string\n"
"  swinstall.swbis_forward_agent            = true\n"
"  swinstall.swbis_ignore_scripts           = false\n"
"\n"
"Examples:\n"
"  Verbosely preview the installed files:\n"
"     swinstall -p -x verbose=5\n"
"  Install a package to three hosts\n"
"     swinstall @ host1 host2 host3\n"
"  Use some extension features to translate an RPM:\n"
"     swinstall -p -s - --allow-rpm -x verbose=1 --preview-tar-file=- | tar tvf -\n"
"  Use some extension features to write the archive to be installed:\n"
"  to stdout:\n"
"     swinstall --to-stdout -s - | tar tvf -\n"
" or  swinstall -s - @- | tar tvf -\n"
);

fprintf(fp , "%s", "\n");
	version_string(fp);
	fprintf(fp , "\n%s", 
        "Report bugs to " REPORT_BUGS "\n");
	return 0;
}

static
int
show_all_sw_selections(VPLOB * swspecs)
{
	int i;
	SWVERID * swverid;
	STROB * tmp;
	tmp = strob_open(1);
	i=0;
	while ((swverid=vplob_val(swspecs, i++)) != NULL) {
		fprintf(stderr, "%s\n", swverid_debug_print(swverid, tmp));
	}
	strob_close(tmp);
	return 0;
}

static
int
is_supported_swspecs(VPLOB * swspecs) {

	int i;
	SWVERID * swverid;
	STROB * tmp;
	struct  VER_ID  * next;
	char * s;

	tmp = strob_open(1);
	
	if (vplob_get_nstore(swspecs) > 1) return 0;
	swverid=vplob_val(swspecs, 0);

	i = 0;	
	E_DEBUG("");
	while ((s = cplob_val(swverid->taglistM, i++))) {
		E_DEBUG2("s=[%s]", s);
		if (strcmp(s, "*")) return 0;	
	}
		E_DEBUG("");
	next = swverid->ver_id_listM;
	while (next) {
		E_DEBUG("");
		if (strlen(next->vqM) &&
		    strcmp(next->vqM, SWVERID_QUALIFIER_PRODUCT)
		) return 0;

		E_DEBUG("");
		if (
			strcmp(next->idM, SWVERID_VERIDS_LOCATION) != 0 &&
			strcmp(next->idM, SWVERID_VERIDS_QUALIFIER) != 0 &&
			1
		) {
			/* Only location and qualifier version ids are supported */
			return 0;
		}
		E_DEBUG("");
		next = next->nextM;
	}
	strob_close(tmp);
		E_DEBUG("");
	return 1;  /* is allowable swspec */
}

static
void
tty_raw_ctl(int c)
{
	static int g = 0;
	if (c == 0) {
		g = 0;
	} else if (c == 1) {
		g = 1;
	} else {
		if (g) {
		g = 0;
		if (swlib_tty_raw(STDIN_FILENO) < 0)
			sw_e_msg(G, "tty_raw error");
		}
	}
}

static
void
safe_sig_handler(int signum)
{
	G->g_master_alarm = 1;
	switch(signum) {
		case SIGUSR1:
			if (G->g_do_progressmeter) alarm(0);
			sw_e_msg(G, "Caught SIGUSR1\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGTERM:
			if (G->g_do_progressmeter) alarm(0);
			sw_l_msg(G, SWC_VERBOSE_1, "Caught SIGTERM\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGINT:
			if (G->g_do_progressmeter) alarm(0);
			sw_l_msg(G, SWC_VERBOSE_1, "Caught SIGINT\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGPIPE:
			if (G->g_do_progressmeter) alarm(0);
			sw_l_msg(G, SWC_VERBOSE_1, "Caught SIGPIPE\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
	}
}

static
void
abort_script(SWICOL * swicol, GB * G, int fd)
{
	if (G->g_target_did_abortM)
		return;
        swicol_set_master_alarm(swicol);
	/* swpl_send_abort(swicol, fd, G->g_swi_event_fd, ""); */
	sw_e_msg(G, "Executing abort_script() ... \n");
	G->g_target_did_abortM = 1;
}

static
void
main_sig_handler(int signum)
{
	int fd;
	int loggersig = SIGABRT;
	E_DEBUG("");
	switch(signum) {
		case SIGTERM:
		case SIGINT:
		case SIGPIPE:
			if (G->g_running_signalsetM) return;
			G->g_psignalM = signum;
			if (G->g_swicolM) {
				/* need to shutdown the target script */
				sw_e_msg(G, "Executing abort\n");
				abort_script(G->g_swicolM, G, G->g_target_fdar[1]);

				raise(SIGUSR1);
				 /* FIXME, this should be raised
				 after it has had the chance for the swicol_alarm
				 status to have the effect of aborting the target
				 script */

				/* return to the execution with the
				   swicol_master_alarm set */

				G->g_running_signalsetM = 1;
				break;
			} else {
				/* fall through */
				G->g_running_signalsetM = 1;
				;
			}
		case SIGUSR1:
			if (G->g_running_signalusr1M) return;
			G->g_running_signalusr1M = 1;
			E_DEBUG("");
			sw_d_msg(G, "Executing handler for signal %d.\n", signum);
			G->g_signal_flag = 0;
			if (G->g_do_progressmeter) alarm(0);
			swgp_signal_block(SIGTERM, (sigset_t *)NULL);
			swgp_signal_block(SIGINT, (sigset_t *)NULL);
			E_DEBUG("");
			/* swgp_signal_block(SIGPIPE, (sigset_t *)NULL); */
			swlib_tty_atexit();
			fd = G->g_target_fdar[0]; if (fd >= 0) close(fd);
			fd = G->g_target_fdar[1]; if (fd >= 0) close(fd);
			fd = G->g_source_fdar[0]; if (fd >= 0) close(fd);
			fd = G->g_source_fdar[1]; if (fd >= 0) close(fd);
			E_DEBUG("");
			if (G->g_xformat && G->g_swi) swc_gf_xformat_close(G, G->g_xformat);
			E_DEBUG("");
			swlib_kill_all_pids(G->g_pid_array + SWC_PID_ARRAY_LEN, 
						G->g_pid_array_len, 
						SIGTERM, 
						G->g_verboseG);
			E_DEBUG("");
			swlib_wait_on_all_pids(G->g_pid_array, 
						G->g_pid_array_len, 
						G->g_status_array, 
						0, 
						G->g_verboseG);
			E_DEBUG("");
			swc_shutdown_logger(G, loggersig);
			switch(signum) {
				case SIGTERM:
				case SIGINT:
					/* FIXME per spec */
					E_DEBUG("");
					LCEXIT(SW_FAILED_ALL_TARGETS); 
					break;
			}
			E_DEBUG("");
			break;
	}
	E_DEBUG("");
}

static
pid_t
run_target_script(GB * G,
		int target_pipe,
		char * source_path,
		char * target_path,
		STROB * source_control_message,
		int keep_old_files,
		int nhops,
		char * pax_read_command_key,
		sigset_t *fork_blockmask,
		sigset_t *fork_defaultmask,
		char * hostname,
		char * blocksize,
		SWI_DISTDATA * distdata,
		char * installed_software_catalog,
		int opt_preview,
		int alt_catalog_root
		) 
{
	pid_t write_pid;

	write_pid = swndfork(fork_blockmask, fork_defaultmask);
	if (write_pid == 0) {
		int ret_target = 1;
		/*
		 * Write the target scripts.
		 */
		ret_target = swinstall_write_target_install_script(G,
			target_pipe, 
			target_path,
			source_control_message,
			source_path, 
			SWC_SCRIPT_SLEEP_DELAY, 
			keep_old_files, 
			nhops, 
			G->g_verboseG,
			pax_read_command_key,
			hostname,
			blocksize,
			distdata,
			installed_software_catalog,
			opt_preview,
			G->g_sh_dash_s, alt_catalog_root);
		/*
		 * 0 OK
		 * >0 is error
		 */
		if (ret_target < 0) ret_target = 2;
		_exit(ret_target);
	} else if (write_pid < 0) {
		sw_e_msg(G, "fork error : %s\n", strerror(errno));
	}
	return write_pid;
}

static
int
combine_version_id(SWVERID * product_swverid, SWVERID * selection_swverid)
{
	char * p_location;
	char * p_qualifier;
	struct VER_ID * newverid;
	STROB * tmp;

	tmp = strob_open(100);
	p_location = swverid_get_verid_value(selection_swverid, SWVERID_VERIDS_LOCATION, 1);
	p_qualifier = swverid_get_verid_value(selection_swverid, SWVERID_VERIDS_QUALIFIER, 1);

	if (p_location) {
		strob_sprintf(tmp, 0, "l=%s", p_location);
		newverid = swverid_create_version_id(strob_str(tmp));
		swverid_replace_verid(product_swverid, newverid);
	}
	if (p_qualifier) {
		strob_sprintf(tmp, 0, "q=%s", p_qualifier);
		newverid = swverid_create_version_id(strob_str(tmp));
		swverid_replace_verid(product_swverid, newverid);
	}
	strob_close(tmp);
	return 0;
}

static
int
swc_read_catalog_ctl_message(GB * G, int fd,
	STROB * control_message, 
	int vlevel, 
	char * loc,
	char * installed_software_catalog)
{
	int ret = 0;
	strob_strcpy(control_message, "");

	E_DEBUG("");
	while(ret >= 0 &&
		strstr(strob_str(control_message),
			SWBIS_TARGET_CTL_MSG_128) == (char*)NULL
	) {
		E_DEBUG("");
		ret = swc_read_target_ctl_message(G, fd,
					control_message,
					vlevel,
					loc);
	}
	E_DEBUG("");
	if (ret < 0) {
		E_DEBUG("");
		return ret;
	} 
	ret = 0;
	return ret;
}

static
int
do_install(GB * G,
	SWI * swi,
	int target_fd1,
	uintmax_t * pstatbytes,
	int do_progressmeter,
	int source_file_size,
	char * target_path,
	char * working_arg,
	VPLOB * swspecs,
	SWI_DISTDATA * distdata,
	char * catalog_path,
	int opt_preview,
	char * pax_read_command_key,
	int keep_old_files,
	int alt_catalog_root,
	int event_fd,
	struct extendedOptions * opta,
	int allow_missing_files,
	SWICAT_E * up_e)
{
	int format;
	int ret;
	int sfd;
	XFORMAT * xformat;
	void (*alarm_handler)(int) = NULL;
	char * pax_read_command;

	if (do_progressmeter) {
		swgp_signal_block(SIGALRM, (sigset_t *)NULL);
		alarm_handler = update_progress_meter;
	}

	xformat = swi->xformatM;
	format = xformat_get_format(xformat);
	sfd = target_fd1;

	E_DEBUG("");
	E_DEBUG2("pax_read_command_key=%s", pax_read_command_key);
	pax_read_command = swc_get_pax_read_command(G->g_pax_read_commands,
				pax_read_command_key, 
				G->g_verboseG >= SWC_VERBOSE_3, 
				keep_old_files, DEFAULT_PAX_R);
	E_DEBUG2("pax_read_command=%s", pax_read_command);

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_3, &G->g_logspec,
		swc_get_stderr_fd(G), "Using archive read command: %s\n", pax_read_command);

	swi->distdataM = distdata;
	E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
	ret = swinstall_arfinstall(G, swi, sfd, &G->g_signal_flag,
			target_path, catalog_path,
			swspecs, SWI_SECTION_BOTH,
			G->g_stdout_testfd, opt_preview, pax_read_command_key,
			alt_catalog_root, event_fd, opta,
			keep_old_files, do_progressmeter ? pstatbytes : NULL, allow_missing_files, up_e);

	
	sw_d_msg(G, "do_install: finished swinstall_arfinstall\n");

	return ret;
}

static
void
do_preview(GB * G,
	FILE * fver,
	int fverfd, 
	char * working_arg,
	STROB * source_control_message,
	int local_stdin,
	int source_nhops,
	char * source_path,
	char * cl_source_target,
	char * source_fork_type,
	int target_nhops,
	char * target_path,
	char * cl_target_target,
	char * target_fork_type,
	char * pax_read_command_key,
	SHCMD * target_sshcmd0,
	SHCMD * target_kill_sshcmd0,
	SHCMD * source_sshcmd0,
	SHCMD * source_kill_sshcmd0,
	int DELAY,
	int wopt_keep_old_files,
	char * wopt_blocksize,
	int use_no_getconf,
	char * pax_write_command_key,
	int target_loop_count,
	int swutil_x_mode,
	char * wopt_shell_command,
	STRAR * target_cmdlist,
	STRAR * source_cmdlist,
	SWI_DISTDATA * distdata,
	char * installed_software_catalog,
	char * sh_dash_s, int alt_catalog_root)
{
	if (
		cl_source_target[strlen(cl_source_target) - 1]
		== '/'
	)
	{
		strob_strcpy(source_control_message, 
			SWBIS_SWINSTALL_SOURCE_CTL_DIRECTORY
			);
	} else {
		strob_strcpy(source_control_message, 
			SWBIS_SWINSTALL_SOURCE_CTL_ARCHIVE
			);
	}
	fprintf(fver, "<> Preview Mode\n");
	fprintf(fver, "<> local_stdin = %d\n", local_stdin);
	if (target_loop_count == 0) {
	fprintf(fver, "<> Source Preview:\n");
	fprintf(fver, "<> Source: soc spec: [%s]\n",
			cl_source_target);
	fprintf(fver, "<> Source: nhops: %d\n", source_nhops);
	fprintf(fver, "<> Source: path: [%s]\n", source_path);
	fprintf(fver, "<> Source: fork type: %s\n", source_fork_type);
	fprintf(fver, "<> Source: pax_write_command: %s\n", 
			swc_get_pax_write_command(G->g_pax_write_commands, pax_write_command_key, 
					G->g_verboseG, DEFAULT_PAX_W));
	fprintf(fver, "<> Source: Command Args: ");
	fflush(fver);
	swc_do_preview_cmd(G, "<> Source: Kill Command Args: ", fver,
		source_path, 
		source_sshcmd0, 
		source_kill_sshcmd0, 
		cl_source_target, 
		source_cmdlist, 
		source_nhops,
		target_loop_count == 0,
		use_no_getconf, wopt_shell_command);
	fflush(fver);
	fprintf(fver, "<> Source: command ends here:\n");
	fprintf(fver, "<> Source: script starts here:\n");
	fflush(fver);
	if (local_stdin == 0) {
		swc_write_source_copy_script(G,
			fverfd, 
			source_path,
			SWINSTALL_DO_SOURCE_CTL_MESSAGE,
			SWC_VERBOSE_4, 
			DELAY, 
			source_nhops, 
			pax_write_command_key, 
			G->g_source_terminal_host,
			wopt_blocksize
			);
	}
	fflush(fver);
	fprintf(fver, "<> Source: End of Source script:\n");
	fprintf(fver, "<> Source: End of Source Preview\n");
	}
	fprintf(fver, "<> Target Preview\n");
	fprintf(fver, "<> Target: soc spec : [%s]\n", working_arg);
	fprintf(fver, "<> Target: nhops: %d\n", target_nhops);
	fprintf(fver, "<> Target: path: [%s]\n", target_path);
	fprintf(fver, "<> Target: source is %s\n",
			strob_str(source_control_message));
	fprintf(fver, "<> Target: fork type: %s\n", target_fork_type);
	fprintf(fver, "<> Target: pax_read_command: %s\n", 
			swc_get_pax_read_command(G->g_pax_read_commands,
				pax_read_command_key, 
				G->g_verboseG >= SWC_VERBOSE_3, 
				wopt_keep_old_files, DEFAULT_PAX_R));
	fprintf(fver, "<> Target: Command Args: ");
	fflush(fver);
	swc_do_preview_cmd(G, "<> Target: Kill Command Args: ", fver,
		target_path, 
		target_sshcmd0,
		target_kill_sshcmd0,
		cl_target_target,
		target_cmdlist,
		target_nhops, 1,
		use_no_getconf, wopt_shell_command);
	fflush(fver);
	fprintf(fver, "<> Target: script starts here:\n");
	fflush(fver);
	if (distdata->did_part1M) {
		swinstall_write_target_install_script(G,
			fverfd, 
			target_path, 
			source_control_message, 
			source_path, 
			DELAY, 
			wopt_keep_old_files, 	
			target_nhops, 
			G->g_verboseG,
			pax_read_command_key,
			G->g_target_terminal_host,
			wopt_blocksize, distdata,
			installed_software_catalog,
			0 /*opt_preview*/, sh_dash_s,
			alt_catalog_root);
	}
	fflush(fver);
	fprintf(fver, "<> Target: End of Target script:\n");
	fprintf(fver, "<> Target: End of Target Preview\n");
	fprintf(fver, "<> End of Preview Mode\n");
	fflush(fver);
}

static
VPLOB *
get_samepackage_specs(GB * G, SWI * swi)
{
	char * tmpval;
	VPLOB * specs;

	/* ''samepackage_specs'' are a list of swspecs that test the installed
	   catalog for a reinstall, downdate and upgrade disposition */

	tmpval = swverid_get_verid_value(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM, SWVERID_VERIDS_LOCATION, 0);
	if (tmpval == NULL)
		tmpval="/";

	specs = swpl_get_same_revision_specs(G, swi, 0 /*First Product FIXME */, tmpval);
	SWLIB_ASSERT(specs != NULL);
	if (0 && G->devel_verboseM)
		show_all_sw_selections(specs);

	return specs;
}

static
char *
dummy_catalog_response(void)
{
	/* Make a default response image */
	STROB * tmp;
	tmp = strob_open(50);
	E_DEBUG("");
	strob_sprintf(tmp, 0,
		"Q0:P:foo,r<1\n"
		"R0:P:...:\n"
		"Q1:P:foo,r==1\n"
		"R1:P:...:\n"
		"Q2:P:foo,r>1\n"
		"R2:P:...:\n");
	E_DEBUG("returning default dummy image");
	/* FIXME, possible memory leak */
	return strob_release(tmp);
}

static
void
copy_vplob_items(VPLOB * dst, VPLOB * src)
{
	void * v;
	int i;
	i = 0;
	while((v=vplob_val(src, i++))) {
		vplob_add(dst, v);
	}
}

static
VPLOB *
get_dependency_specs(GB * G, SWI * swi, STRAR ** p_type_array)
{
	int i;
	VPLOB * retspecs;
	VPLOB * specs;
	STROB * tmp;
	SWVERID * swverid;

	tmp = strob_open(32);
	*p_type_array = strar_open();
	retspecs = vplob_open();

	specs = swpl_get_dependency_specs(G, swi, SW_A_prerequisites, 0, 0);
	SWLIB_ASSERT(specs != NULL);
	i = 0;
	while(vplob_val(specs, i++)) {
		swverid = vplob_val(specs, i-1);
		strob_sprintf(tmp, 0, "%s:0:%d", SWICAT_REQ_TYPE_P, swverid->alter_uuidM);
		strar_add(*p_type_array, strob_str(tmp));
	}
	copy_vplob_items(retspecs, specs);
	vplob_shallow_close(specs);

	specs = swpl_get_dependency_specs(G, swi, SW_A_exrequisites, 0, 0);
	SWLIB_ASSERT(specs != NULL);
	i = 0;
	while(vplob_val(specs, i++)) {
		/* strar_add(*p_type_array, SWICAT_REQ_TYPE_E); */
		swverid = vplob_val(specs, i-1);
		strob_sprintf(tmp, 0, "%s:0:%d", SWICAT_REQ_TYPE_E, swverid->alter_uuidM);	
		strar_add(*p_type_array, strob_str(tmp));
	}
	copy_vplob_items(retspecs, specs);
	vplob_shallow_close(specs);
	
	specs = swpl_get_dependency_specs(G, swi, SW_A_corequisites, 0, 0);
	SWLIB_ASSERT(specs != NULL);
	i = 0;
	while(vplob_val(specs, i++)) {
		/* strar_add(*p_type_array, SWICAT_REQ_TYPE_C); */
		swverid = vplob_val(specs, i-1);
		strob_sprintf(tmp, 0, "%s:0:%d", SWICAT_REQ_TYPE_C, swverid->alter_uuidM);	
		strar_add(*p_type_array, strob_str(tmp));
	}
	copy_vplob_items(retspecs, specs);
	vplob_shallow_close(specs);
	strob_close(tmp);
	return retspecs;
}

static
void
delete_samepackage_spec(VPLOB * specs)
{
	vplob_delete_store(specs, (void(*)(void*))(swverid_close));
}

static
VPLOB *
depd_get_responses_by_type(VPLOB * src, STRAR * type_array, char * type)
{
	VPLOB * dst;
	int i;
	int req_offset = SL_FIRST_DEP_INDEX; /* the first three specs are the samepackage specs */
	char * t;
	void * v;
	SWICAT_SC * sc;
	int err;

	dst = vplob_open();
	i = 0;
	while ((t=strar_get(type_array,i)) != NULL) {
		if (*t == *type) {
			v = vplob_val(src, i + req_offset);
			/* v is a SWICAT_SC object pointer */
			if (!v) return NULL;
			sc = (SWICAT_SC*)v;
			sc->statusM = SWICAT_SC_STATUS_UNSET;
			sc->swverid_uuidM = swlib_atoi(t+TA_UUID_OFFSET, &err);
			if (err != 0) return NULL;
			vplob_add(dst, v);
		}
		i++;	
	}
	return dst;
}

static
int
is_target_a_relocation(SWI * swi, char * target_path)
{
	char * p_location;
	SWVERID * product_swverid;
	if (strcmp(target_path, "/") != 0) return 1;
	product_swverid = SWI_GET_PRODUCT(swi, 0 /* FIXME, support multiple products */)->p_baseM.swveridM;
	p_location = swverid_get_verid_value(product_swverid, SWVERID_VERIDS_LOCATION, 1);
	if (p_location && strcmp(p_location, "/") != 0) {
		return 1;
	}
	return 0;
}

static
int 
l_enforce_locatable(GB * G, SWI * swi, char * target_path, int product_number, struct extendedOptions * opta)
{
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	SWHEADER * global_index;
	int ret;
	char * is_locatable;
	char * prod_tag;
	char * fileset_tag;
	int fileset_number;
	int do_enforce;
	int status = 0;

	ret = 0;
	do_enforce = swc_is_option_true(get_opta(opta, SW_E_enforce_locatable));
	global_index = swi_get_global_index_header(swi);
	swheader_store_state(global_index, NULL);
	
	/* FIXME support multiple products */
	product_number = 0;
	product = swi_package_get_product(swi->swi_pkgM, product_number/* The first product */);
	SWLIB_ASSERT(product != NULL);
	swheader_set_current_offset(global_index, product->p_baseM.header_indexM);
	prod_tag = swheader_get_single_attribute_value(global_index, SW_A_tag);
	SWLIB_ASSERT(prod_tag != NULL);

	/* FIXME support multiple filesets */
	fileset_number = 0;
	while ((fileset = swi_product_get_fileset(product, fileset_number++)) != NULL) {
		/* Set the search to the beginning of the fileset object in the INDEX file */
		swheader_set_current_offset(global_index, fileset->baseM.header_indexM);
		is_locatable = swheader_get_single_attribute_value(global_index, SW_A_is_locatable);
		fileset_tag = swheader_get_single_attribute_value(global_index, SW_A_tag);
		SWLIB_ASSERT(fileset_tag != NULL);
		if (swc_is_option_true(is_locatable) == 0 /* Not true */) {
			if (do_enforce)
				status = SW_ERROR;
			else
				status = SW_WARNING;
			sw_e_msg(G, "SW_NOT_LOCATABLE: %s.%s: status=%d\n", prod_tag, fileset_tag, status);
			ret++;
		} else {
			/* A Null value is "true" because the default value is "true" */
		}
	}
	SWLIB_ASSERT(fileset_number > 1); /* sanity check */

	swheader_restore_state(global_index, NULL);
	if (do_enforce == 0) return 0;
	return ret;
}

static
VPLOB *  /* array of SWICAT_SC pointers */
depd_requisite_test(GB * G,
		SWI * swi,
		SWICAT_SL * sl,
		STRAR * type_array, char * type)
{
	int i;
	int j;
	VPLOB * sc_array;
	SWICAT_SC * sc;
	SWICAT_SR * sr;
	STROB * buf;
	char *s;

	E_DEBUG("");
	buf = strob_open(48);

	/* Create an array of SWICAT_SC objects that are of the type
	   requested */

	sc_array = depd_get_responses_by_type(sl->scM, type_array, type);

	E_DEBUG("");
	if (sc_array == NULL) {
		sw_e_msg(G, "internal error\n");
		return NULL;
	}

	i = 0;	
	E_DEBUG("");
	while((sc=vplob_val(sc_array, i++))) {
		j = 0;
		E_DEBUG("");
		while((sr=vplob_val(sc->srM, j++))) {
			s = swicat_sr_form_catalog_path(sr, swi->swi_pkgM->installed_software_catalogM, NULL);
			/* if a non-zero length catalog path is formed then
			   a entry exists for the dependency_spec */

			E_DEBUG("");
			if (strlen(s) > 0) {
				/* entry found for query */
				E_DEBUG2("entry found for %s", s);
				sc->statusM = SWICAT_SC_STATUS_FOUND;
			} else {
				sc->statusM = SWICAT_SC_STATUS_NOT_FOUND;
			}
		}
		/*
		fprintf(stderr, "%s: sl->scM[%d]->sqM->swspec_string=[%s]\n", type, i-1,
				sc->sqM->swspec_stringM);
		*/
	}
	strob_close(buf);
	E_DEBUG("");
	return sc_array;
}

static
void
delete_if_alternate_match(VPLOB * sca, SWICAT_SC * alternate)
{
	int i;
	SWICAT_SC * sc;
	int uuid;

	E_DEBUG("");
	uuid = alternate->swverid_uuidM;
	i = 0;	
	while((sc=vplob_val(sca, i++))) {
		if (
			sc != alternate &&
			sc->swverid_uuidM == uuid &&
			sc->statusM == SWICAT_SC_STATUS_FOUND
		) {
			E_DEBUG2("removing %d", i-1);
			alternate->statusM = SWICAT_SC_STATUS_DELETE;
		}
	}
	E_DEBUG("");
}

static
int
depd_handle_alternate_prerequisites(VPLOB * sca, STRAR * type_array)
{
	/* eliminate alternates with no match for groups that have a match */
	int i;
	SWICAT_SC * sc;
	VPLOB * sc_array;

	E_DEBUG("");
	sc_array = sca;
	i = 0;	
	while((sc=vplob_val(sc_array, i++))) {
		E_DEBUG2("sc->statusM=%d", sc->statusM);
		if (sc->statusM != SWICAT_SC_STATUS_FOUND) {
			E_DEBUG("");
			/* if depency_spec not found, see if there is a
			   match in its alternation group */
			delete_if_alternate_match(sca, sc);
		}
	}
	E_DEBUG("");
	return 1;
}

static
int
depd_prerequisite_test(GB * G, SWI * swi, SWICAT_SL * sl, STRAR * type_array,
		char * isc_path,
		char * enforce_dependencies,
		char * soc_spec_target)
{
	STROB * tmp;
	VPLOB * sc_array;
	SWICAT_SC * sc;
	SWICAT_SR * sr;
	int j;
	int i;
	int ret;
	int local_debug = 0;
	int status;
	char * catalog_entry_directory;

	tmp = strob_open(64);
	ret = 0;
	sc_array = depd_requisite_test(G, swi, sl, type_array, SWICAT_REQ_TYPE_P);

	/* sc_array is an array of prerequisites */

	if (sc_array == NULL) {
		SWLIB_FATAL("depd_requisite_test failed for prerequisites");
	}


	/* Now mark for deletion failed prerequisites that had a match in
	   their alternation group */

	depd_handle_alternate_prerequisites(sc_array, type_array);

	if (local_debug) {
		i = 0;	
		while((sc=vplob_val(sc_array, i++))) {
			fprintf(stderr, "sc->statusM = [%d] uuid = [%d]\n", sc->statusM, sc->swverid_uuidM);
			fprintf(stderr, "sl->scM[%d]->sqM->swspec_string=[%s]\n", i-1, sc->sqM->swspec_stringM);
			{
				j = 0;
				while((sr=vplob_val(sc->srM, j++))) {
					catalog_entry_directory = swicat_sr_form_catalog_path(sr, "_ISC_", tmp);
					fprintf(stderr, "ENTRY: [%s]\n", catalog_entry_directory);	
				}
			}
	
		}
	}

	/* Now, all members of (SWICAT_SC*)sc_array should have a statusM
	   of SWICAT_SC_STATUS_FOUND or SWICAT_SC_STATUS_DELETE, anything else
	   is a failed dependency. (SWICAT_SC_STATUS_DELETE means it had a match
	   in its alternation group so just ignore it) */

	ret = 0;
	i = 0;
	while((sc=vplob_val(sc_array, i++))) {
		if (sc->statusM == SWICAT_SC_STATUS_NOT_FOUND) { 
			if (swc_is_option_true(enforce_dependencies)) {
				ret++;
				status=1;
				sw_e_msg(G, "SW_DEPENDENCY_NOT_MET: prerequisite %s: status=%d\n",
					sc->sqM->swspec_stringM, status);
			} else {
				status=2;
				sw_e_msg(G, "SW_DEPENDENCY_NOT_MET: prerequisite %s: status=%d\n",
					sc->sqM->swspec_stringM, status);
			}
		} else if (sc->statusM == SWICAT_SC_STATUS_FOUND) { 
			j = 0;
			while((sr=vplob_val(sc->srM, j++))) {
				catalog_entry_directory = swicat_sr_form_catalog_path(sr, isc_path, NULL);
				sw_l_msg(G, SWC_VERBOSE_3, "prerequisiste %s satisfied: %s\n",
					sc->sqM->swspec_stringM, catalog_entry_directory);
			}
		}
	}
	strob_close(tmp);
	return ret;
}

static
int
depd_exrequisite_test(GB * G, SWI * swi, SWICAT_SL * sl, STRAR * type_array,
		char * isc_path,
		char * enforce_dependencies,
		char * soc_spec_target)
{
	VPLOB * sc_array;
	SWICAT_SC * sc;
	SWICAT_SR * sr;
	int j;
	int i;
	int ret;
	char * catalog_entry_directory;

	ret = 0;
	sc_array = depd_requisite_test(G, swi, sl, type_array, SWICAT_REQ_TYPE_E);

	/* sc_array is an array of exrequisites */

	if (sc_array == NULL) {
		SWLIB_FATAL("depd_requisite_test failed for exrequisites");
	}

	ret = 0;
	i = 0;
	while((sc=vplob_val(sc_array, i++))) {
		if (sc->statusM == SWICAT_SC_STATUS_FOUND) { 
			/* Bad */
			if (swc_is_option_true(enforce_dependencies)) {
				ret++;
				j = 0;
				while((sr=vplob_val(sc->srM, j++))) {
					catalog_entry_directory = swicat_sr_form_catalog_path(sr, isc_path, NULL);
					sw_e_msg(G,
				"SW_DEPENDENCY_NOT_MET: exrequisiste %s violated: %s: status=1\n",
						sc->sqM->swspec_stringM, catalog_entry_directory);
				}
			} else {
				j = 0;
				while((sr=vplob_val(sc->srM, j++))) {
					catalog_entry_directory = swicat_sr_form_catalog_path(sr, isc_path, NULL);
					sw_e_msg(G,
				"SW_DEPENDENCY_NOT_MET: exrequisiste %s violated: %s: status=2\n",
						sc->sqM->swspec_stringM, catalog_entry_directory);
				}
			}
		} else if (sc->statusM == SWICAT_SC_STATUS_NOT_FOUND) { 
			/* Good */
			sw_l_msg(G, SWC_VERBOSE_3, "exrequisiste %s satisfied\n", sc->sqM->swspec_stringM);
		}
	}
	return ret;
}

static
int
samepackage_exte_test(GB * G,
		SWI * swi,
		SWICAT_SL * sl,
		char * isc_path,
		int sc_index,
		char * msg,
		int verbose_level,
		int do_allow,
		char * status_string,
		STRAR * list_of_matches)
{
	int i;
	int ret;
	SWICAT_SC * sc;
	SWICAT_SR * sr;
	char * s;

	ret = 0;
	sc = vplob_val(sl->scM, sc_index);

	SWLIB_ASSERT(sc != NULL);
	
	i = 0;
	while((sr=vplob_val(sc->srM, i++))) {
		s = swicat_sr_form_catalog_path(sr, isc_path, NULL);
		if (strlen(s) > 0) {
			ret++;
			if (list_of_matches) {
				strar_add(list_of_matches, s);
			}
			if (verbose_level > 0) {
				if (do_allow) {
					sw_e_msg(G, "%s: %s%s\n", msg, s, status_string);
				} else {
					sw_e_msg(G, "%s: %s%s\n", msg, s, status_string);
				}
			}
		}
	}
	return ret;
}

/* Determine a list of SWVERID objects to remove */
static
VPLOB *
s_determine_upgrade_selection(GB * G,
		SWI * swi,
		SWICAT_SL * sl,
		char * isc_path,
		char * allow_multiple_versions,
		char * soc_spec_target, STRAR * catalog_paths)
{
	char * s;
	int i;
	int ret0;
	int ret1;
	int ret2;
	char * swspec;
	STROB * sws;
	SWVERID * swverid;
	VPLOB * swspecs;
	STRAR * s1;
	STRAR * s2;
	STRAR * s3;

	swspecs = vplob_open();
	sws = strob_open(20);
	s1 = strar_open();
	s2 = strar_open();
	s3 = strar_open();
	
	ret0 = samepackage_exte_test(G, swi, sl, isc_path, SL_LT_INDEX, "", 0, 0, "", s1);
	ret1 = samepackage_exte_test(G, swi, sl, isc_path, SL_EQ_INDEX, "", 0, 0, "", s2);
	ret2 = samepackage_exte_test(G, swi, sl, isc_path, SL_GT_INDEX, "", 0, 0, "", s3);

	/* FIXME assume there are no multiples installed, pick *only* one as the upgrade object */

	i = 0; while((s=strar_get(s3, i++)) != NULL) {
		swspec = swicat_sr_form_swspec_from_catalog_path(s, sws);
		if (G->devel_verboseM) {
			fprintf(stderr, "SL_GT_INDEX: [%s] [%s]\n", s, swspec);
		}
		if (vplob_get_nstore(swspecs) == 0) {
			swverid = swverid_open(NULL, swspec);
			if (swverid == NULL) return NULL;
			vplob_add(swspecs, (void*)swverid);
			if (catalog_paths) {
				strar_add(catalog_paths, s);
			}
		}
	}

	i = 0; while((s=strar_get(s2, i++)) != NULL) {
		swspec = swicat_sr_form_swspec_from_catalog_path(s, sws);
		if (G->devel_verboseM) {
			fprintf(stderr, "SL_EQ_INDEX: [%s] [%s]\n", s, swspec);
		}
		if (vplob_get_nstore(swspecs) == 0) {
			swverid = swverid_open(NULL, swspec);
			if (swverid == NULL) return NULL;
			vplob_add(swspecs, (void*)swverid);
			if (catalog_paths) {
				strar_add(catalog_paths, s);
			}
		}
	}

	i = 0; while((s=strar_get(s1, i++)) != NULL) {
		swspec = swicat_sr_form_swspec_from_catalog_path(s, sws);
		if (G->devel_verboseM) {
			fprintf(stderr, "SL_LT_INDEX: [%s] [%s]\n", s, swspec);
		}
		if (vplob_get_nstore(swspecs) == 0) {
			swverid = swverid_open(NULL, swspec);
			if (swverid == NULL) return NULL;
			vplob_add(swspecs, (void*)swverid);
			if (catalog_paths) {
				strar_add(catalog_paths, s);
			}
		}
	}

	strar_close(s1);
	strar_close(s2);
	strar_close(s3);
	strob_close(sws);
	return swspecs;
}

static
int
samepackage_multiple_version_test(GB * G,
		SWI * swi,
		SWICAT_SL * sl,
		char * isc_path,
		char * allow_multiple_versions,
		char * soc_spec_target)
{
	int ret0;
	int ret1;
	int ret2;
	int total;

	ret0 = samepackage_exte_test(G, swi, sl, isc_path, SL_LT_INDEX, "", 0, 0, "", NULL);
	ret1 = samepackage_exte_test(G, swi, sl, isc_path, SL_EQ_INDEX, "", 0, 0, "", NULL);
	ret2 = samepackage_exte_test(G, swi, sl, isc_path, SL_GT_INDEX, "", 0, 0, "", NULL);

	total = ret0 + ret1 + ret2;
	E_DEBUG2("ret0=%d", ret0);
	E_DEBUG2("ret1=%d", ret1);
	E_DEBUG2("ret2=%d", ret2);
	E_DEBUG2("total=%d", total);

	if (total > 1) {
		int event_status;
		STROB * tmp = strob_open(64);
		/* multiple packages installed, this is always an error
		   for swinstall */

		/* The following three line serve to print the results */
		if (swc_is_option_true(allow_multiple_versions)) {
			event_status = SW_WARNING;
		} else {
			event_status = SW_ERROR;
		}
		strob_sprintf(tmp, 0, ": status=%d", event_status);

		samepackage_exte_test(G, swi, sl, isc_path, SL_LT_INDEX, "SW_EXISTING_MULTIPLE_VERSION", 1, 0, strob_str(tmp), NULL);
		samepackage_exte_test(G, swi, sl, isc_path, SL_EQ_INDEX, "SW_EXISTING_MULTIPLE_VERSION", 1, 0, strob_str(tmp), NULL);
		samepackage_exte_test(G, swi, sl, isc_path, SL_GT_INDEX, "SW_EXISTING_MULTIPLE_VERSION", 1, 0, strob_str(tmp), NULL);
		strob_close(tmp);
	}
	if (swc_is_option_true(allow_multiple_versions)) return 0;
	if (total == 1) return 0;
	return total;
}


static
int
samepackage_reinstall_test(GB * G,
		SWI * swi,
		SWICAT_SL * sl,
		char * isc_path,
		char * allow,
		char * soc_spec_target)
{
	int do_allow;
	int ret;
	STROB * tmp;
	int verbose_level;

	verbose_level = G->g_verboseG;
	tmp = strob_open(64);	
	do_allow = swc_is_option_true(allow);

	if (do_allow) {
		/* "reinstall" is true */
		strob_sprintf(tmp, 0, "SW_SAME_REVISION_INSTALLED at %s: status=%d", soc_spec_target, SW_NOTICE);
	}  else {
		/* "reinstall" is false */
		verbose_level = 1;
		strob_sprintf(tmp, 0, "SW_SAME_REVISION_SKIPPED at %s: status=%d", soc_spec_target, SW_NOTICE);
	}
	ret = samepackage_exte_test(G, swi, sl, isc_path, SL_EQ_INDEX, strob_str(tmp), verbose_level, do_allow, "", NULL);
	
	if (do_allow) {
		ret = 0;
	}

	if (ret) {
		sw_e_msg(G, "try option: -x reinstall=true\n");
	}
	strob_close(tmp);
	return ret;
}

static
int
samepackage_downdate_test(GB * G,
		SWI * swi,
		SWICAT_SL * sl,
		char * isc_path,
		char * allow,
		char * soc_spec_target)
{
	int do_allow;
	int ret;
	STROB * tmp;
	int verbose_level;

	verbose_level = G->g_verboseG;
	tmp = strob_open(64);	
	do_allow = swc_is_option_true(allow);

	if (do_allow) {
		strob_sprintf(tmp, 0, "SW_HIGER_REVISION_INSTALLED at %s: status=%d", soc_spec_target, SW_WARNING);
	}  else {
		verbose_level = 1;
		strob_sprintf(tmp, 0, "SW_HIGHER_REVISION_INSTALLED at %s, status=%d", soc_spec_target, SW_ERROR);
	}
	ret = samepackage_exte_test(G, swi, sl, isc_path, SL_GT_INDEX, strob_str(tmp), verbose_level, do_allow, "", NULL);
	
	if (do_allow) {
		ret = 0;
	}
	if (ret) {
		sw_e_msg(G, "try option: -x allow_downdate=true\n");
	}
	strob_close(tmp);
	return ret;
}

static
void
s_rev_instance(char * s, char ** p_rev, char ** p_slash, char ** p_instance)
{
	if (s) {
		swlib_squash_double_slash(s);
		*p_slash = strrchr(s, '/');
		SWLIB_ASSERT(*p_slash != NULL);
		*(*p_slash) = '\0';
		*p_instance = (*p_slash)+1;
		*p_rev = strrchr(s, '/');
		SWLIB_ASSERT(*p_rev != NULL);
		(*p_rev)++;
	} else {
		*p_slash = NULL;
		*p_rev = NULL;
		*p_instance = NULL;
	}
}

/**  samepackage_determine_instance  - Determine the instance_Id to use
  *
  *  Return the decimal value of the instance id, -1 on error.
  *
  *  The instance_id is the catalog path component that is a integer 0 thru 9...
  *  for example:  var/lib/swbis/catalog/foo/foo/1.1/8   where 8 is the instance Id
  */

static
int
samepackage_determine_instance(GB * G, SWI * swi, SWICAT_SL * sl,
		char * isc_path, STRAR * upgrade_catalog_paths, int *p_did_find)
{
	STRAR * same_revision_list;
	STRAR * id_list;
	char * s;
	int i;
	int n;
	int found;
	int ret;
	char * upgrade_revision;
	char * upgrade_slash;
	char * upgrade_instance;
	char * revision;
	char * slash;
	char * instance;

	*p_did_find = 0;
	same_revision_list = strar_open();
	id_list = strar_open();
	ret = samepackage_exte_test(G, swi, sl, isc_path, SL_EQ_ANY_LOC_INDEX, "", 0, 0, "", same_revision_list);

	/* same_revision_list is now a list of catalog paths that have the same
	   revision and any location as the incoming product */

	/* (STRAR *)upgrade_catalog_paths contains the catalog path of the selection
	   that is going to be upgraded (i.e. replaced with the incoming product)
	   right now there should be only one of these  */

	/* verbose debugging */
	i = 0;
	while((s=strar_get(upgrade_catalog_paths, i++))) {
		if (G->devel_verboseM)
			fprintf(stderr, "Upgrade: %d: %s\n", i-1, s);
	}

	/* verbose debugging */
	i = 0;
	while((s=strar_get(same_revision_list, i++))) {
		if (G->devel_verboseM)
			fprintf(stderr, "Same_revision: %d: %s\n", i-1, s);
	}

	if (strar_get(same_revision_list, 0) == NULL) {
		/* there are no installations of this revision
		   therefore return 0 */
		return 0;
	} 

	/* pick out the the revision and instance in the catalog path */

	s = strar_get(upgrade_catalog_paths, 0);
	s_rev_instance(s, &upgrade_revision, &upgrade_slash, &upgrade_instance);

	ret = -1;
	i = 0;
	while((s=strar_get(same_revision_list, i++))) {
		s_rev_instance(s, &revision, &slash, &instance);
		if (revision) {
			if (upgrade_revision && strcmp(revision, upgrade_revision) == 0) {
				/* i.e. reinstalling the same revision, use
				   this instance of the upgrade since it is being
				   replaced */

				*p_did_find = 1;
				ret = atoi(upgrade_instance);
				*slash = '/';
				break;
			} else {
				/* the upgrade_revision is not involved since it is
				   different, a new instance will be made in /<revision>,
				   pick a instance Id that is unique among the list in
				   same_revision_list */
				strar_add(id_list, instance);
				if (slash) {
					*slash = '/';
				}
			}
		} else {
			/* never happens */
			break;		
		}
	}

	n = 0;
	found = 0;
	if (strar_num_elements(id_list) > 0) {
		ret = -1;
		for (n=0; n < 20; n++) {
			i = 0;
			while((s=strar_get(id_list, i++)) != NULL) {
				found = 1;
				if (atoi(s) == n) {
					found = 0;
					break;
				}
			}
			if (found) {
				*p_did_find = 1;
				ret = n;
				break;
			}
		}
	}

	if (upgrade_slash) {
		*upgrade_slash = '/';
	}
	strar_close(same_revision_list);
	strar_close(id_list);
	return ret;
}

static
SWICAT_REQ *
analyze_samepackage_query_response(GB * G, char * response_image, SWICAT_SL ** p_sl)
{
	int ret;
	int i;
	int j;
	SWICAT_REQ  * req;
	SWICAT_SL * sl;
	SWICAT_SC * sc;
	SWICAT_SR * sr;

	if (response_image == NULL) return NULL;
	req = swicat_req_create();

	/* analyze the image and make the SWICAT_SL object */

	ret = swicat_req_analyze(G, req, response_image, p_sl);
	sl = *p_sl;
	if (ret < 0 || ret > 0) {
		sw_e_msg(G, "internal error from swicat_req_analyze, ret=%d\n", ret);
	} else {
		;
	}
	
	/* Run tests on the SWICAT_SL object to determine the
           upgrade/downgrade/reinstall disposition */
	if (G->devel_verboseM && 0) {
		i = 0;	
		while((sc=vplob_val(sl->scM, i++))) {
			fprintf(stderr, "sl->scM[%d]->sqM->swspec_string=[%s]\n", i-1,
					sc->sqM->swspec_stringM);
		}	
	}

	if (0 && G->devel_verboseM && 0) {
	i = 0;
	while((sc=vplob_val(sl->scM, i++))) {
		fprintf(stderr, "sl->scM[%d]->sqM->swspec_string=[%s]\n", i-1,
				sc->sqM->swspec_stringM);
		j = 0;
		while((sr=vplob_val(sc->srM, j++))) {
			fprintf(stderr, "sl->scM[%d]->srM[%d]->lineM=[%s]\n", i-1, j-1, sr->lineM);
		}
	}
	}

	/* swicat_sl_delete(sl); */
	return req;
}


/*
 = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
 = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
        Main Program
 = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
 = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
*/

int
main(int argc, char **argv)
{
	int i;
	int c;
	int ret;
	int tmpret;
	int optret = 0;
	int source_fdar[3];
	int target_fdar[3];
	int source_stdio_fdar[3];
	int target_stdio_fdar[3];
	int swi_event_fd = -1;
	int save_stdio_fdar[3];
	int targetlist_fd = -1;
	int e_argc = 1;
	int w_argc = 1;
	int main_optind = 0;
	int opt_loglevel = 0;
	int target_nhops;
	int source_nhops;
	int is_source_local_regular_file = 0;
	int num_remains;
	int do_to_stdout = 0;
	int did_find_instance;
	int uinfile_decode_flags;
	char * tmp_s;
	pid_t ss_pid = 0;
	pid_t ts_pid = 0;
	pid_t target_write_pid = 0;
	pid_t source_write_pid = 0;
	pid_t * pid_array;
	int   * status_array; 
	int * p_pid_array_len;
	pid_t target_pump_pid = 0;
	pid_t source_pump_pid = 0;
	int swutil_x_mode = 1;
	int wopt_pump_delay1 = -1;
	int wopt_burst_adjust = DEFAULT_BURST_ADJUST;  /* sort is similar to rcvd adjust size in openssh */
	char * wopt_shell_command;
	char * wopt_tar_format; /* gnu, pax, ustar */
	char * wopt_any_format;
	int wopt_no_defaults = 0;
	int wopt_kill_hop_threshhold = 2;  /* 2 */
	int wopt_debug_events= 0;
	int wopt_allow_missing = 0;
	char * installer_sig;
	char * wopt_force;
	char * wopt_enforce_file_md5;
	char * wopt_enforce_all_signatures;
	char * wopt_swbis_replacefiles;
	char * wopt_sig_level = "0";
	char * wopt_allow_rpm;
	char * wopt_install_volatile;
	char * wopt_volatile_newname;
	char * wopt_no_getconf;
	char * wopt_forward_agent;
	char * wopt_noscripts;
	char * wopt_no_remote_kill;
	char * wopt_quiet_progress_bar;
	char * wopt_show_progress_bar;
	char * optionname;
	char * wopt_ssh_options = (char*)NULL;
	int is_seekable = 0;
	int  optionEnum;
	int vofd;
	int wopt_with_hop_pids = 1;
	int devel_no_fork_optimization = 1;
	int local_stdin = 0;
	int do_extension_options_via_goto = 0;
	int tmp_status;
	int fverfd;
	int target_loop_count = 0;
	int target_success_count = 0;
	int cfd; 
	int * p_do_progressmeter;
	int make_master_raw = 1;
	int is_local_stdin_seekable = 0;
	int nullfd;
	int opt_preview = 0;
	int testnum = 0;
	int s_retval = 0;  /* affected by fileset control script results */
	int f_retval = 0;  /* affected by target task_script results */
	int t_retval = 0;  /* status for the target */
	int source_file_size;
	int target_file_size;
	int same_revision_skipped;
	uintmax_t statbytes;
	int target_array_index = 0;
	int select_array_index = 0;
	int stdin_in_use = 0;
	int use_no_getconf;
	int swevent_fd = STDOUT_FILENO;
	int instance_id;
	struct extendedOptions * opta = optionsArray;
	struct termios	*login_orig_termiosP = NULL;
	struct winsize	*login_sizeP = NULL;
	int sigs_passed;
	int wopt_sign;
	int overwrite_fd;
	int n_file_conflicts;
	int num_sigs_checked;
	int sig_level;
	
	char * xcmd;
	char * wopt_blocksize = "5120";
	char * wopt_show_options = NULL;
	char * wopt_show_options_files = NULL;
	int  wopt_keep_old_files = 0;
	char * eopt_allow_downdate;
	char * eopt_allow_incompatible;
	char * eopt_ask;
	char * eopt_autoreboot;
	char * eopt_autorecover;
	char * eopt_defer_configure;
	char * eopt_autoselect_dependencies;
	char * eopt_distribution_source_directory;
	char * eopt_installed_software_catalog;
	char * eopt_enforce_dependencies;
	char * eopt_enforce_locatable;
	char * eopt_enforce_scripts;
	char * eopt_enforce_dsa;
	char * eopt_logfile;
	char * eopt_loglevel;
	char * eopt_reinstall;
	char * eopt_select_local;
	char * eopt_verbose;
	char * opt_distribution_target_directory = "/";
	char * tty_opt = NULL;
	char * source_fork_type;
	char * target_fork_type;
	char * source_script_pid = (char*)NULL;
	char * target_script_pid = (char*)NULL;
	char * tmpcharp = (char*)NULL;
	char * exoption_arg;
	char * exoption_value;
	char * soc_spec_target = (char*)NULL;
	char * soc_spec_source = (char*)NULL;
	char * cl_target_target = (char*)NULL;
	char * new_cl_target_target = (char*)NULL;
	char * xcl_target_target = (char*)NULL;
	char * xcl_source_target = (char*)NULL;
	char * cl_target_selections = (char*)NULL;
	char * cl_source_target = (char*)NULL;
	char * cl_source_selections = (char*)NULL;
	char * target_path = (char*)NULL;
	char * pax_read_command_key = (char*)NULL;
	char * pax_write_command_key = (char*)NULL;
	char * wopt_local_pax_write_command = (char*)NULL;
	char * wopt_remote_pax_write_command = (char*)NULL;
	char * wopt_local_pax_read_command = (char*)NULL;
	char * wopt_remote_pax_read_command = (char*)NULL;
	char * working_arg = (char*)NULL;
	char * current_arg = (char*)NULL;
	char * opt_option_files = (char*)NULL;
	char * system_defaults_files = (char*)NULL;
	char * source_path = (char*)NULL;
	char cwd[1024];
	char * remote_shell_command = REMOTE_SSH_BIN;
	char * remote_shell_path;
	char * response_img = NULL;
	char * epath;
	char * dummy_catalog;
	char * location;

	SWI_DISTDATA  * distdataO;
	STRAR * target_cmdlist;
	STRAR * source_cmdlist;
	STRAR * target_tramp_list;
	STRAR * source_tramp_list;
	STRAR * type_array;
	STRAR * upgrade_catalog_paths;
	VPLOB * samepackagespecs;
	VPLOB * upgrade_specs;
	VPLOB * upgrade_catalog_entries;
	VPLOB * dep_specs;
	VPLOB * swspecs;
	STROB * tmp;
	STROB * source_control_message;
	STROB * target_control_message;
	STROB * target_current_dir;
	STROB * target_start_message;
	STROB * source_start_message;
	STROB * target_catalog_message;
	STROB * target_catalog_path;
	STROB * source_access_message;
	STROB * source_line_buf;
	STROB * target_line_buf;
	STROB * tmpcommand;
	CPLOB * w_arglist;
	CPLOB * e_arglist;
	SWI * swi = (SWI*)NULL;
	SWI * e_swi = (SWI*)NULL;
	SHCMD * target_sshcmd[2];
	SHCMD * source_sshcmd[2];
	SHCMD * kill_sshcmd[2];
	SHCMD * target_kill_sshcmd[2];
	SHCMD * source_kill_sshcmd[2];
	FILE * fver;
	sigset_t * ssh_fork_blockmask;
	sigset_t * fork_blockmask;
	sigset_t * fork_defaultmask;
	sigset_t * currentmask;
	SWVERID * swverid;
	SWLOG * swlog;
	SWICAT_SL * sl;
	SWICAT_E * up_e;
	SWICAT_REQ  * req;
	SWI_FILELIST * fl;
	
	struct option main_long_options[] =
             {
		{"selections-file", 1, 0, 'f'},
		{"alternate-catalog-root", 0, 0, 'r'},
		{"preview", 0, 0, 'p'},
		{"test", 0, 0, 'p'},  /* same as preview */
		{"block-size", 1, 0, 'b'},
		{"target-file", 1, 0, 't'},
		{"source", 1, 0, 's'},
		{"format", 1, 0, 'H'},
		{"defaults-file", 1, 0, 'X'},
		{"verbose", 0, 0, 'v'},
		{"enable-agent-forwarding", 0, 0, 'A'},
		{"extension-option", 1, 0, 'W'},
		{"extended-option", 1, 0, 'x'},
		{"version", 0, 0, 'V'},
		{"help", 0, 0, '\012'},
		{"enforce-all-signatures", 0, 0, 150},
		{"nosignature", 0, 0, 151},
		{"nodeps", 0, 0, 152},
		{"slackware", 0, 0, 153},
		{"noop", 0, 0, 154},
		{"no-enforce-scripts", 0, 0, 155},
		{"no-chdir-for-scripts", 0, 0, 156},
		{"show-options", 0, 0, 158},
		{"show-options-files", 0, 0, 159},
		{"testnum", 1, 0, 164},
		{"debug-verbose", 0, 0, 165},
		{"force", 0, 0, 166},
		{"force-locks", 0, 0, 167},
		{"reinstall", 0, 0, 168},
		{"enforce-scripts", 0, 0, 169},
		{"keep-old-files", 0, 0, 170},
		{"keepfiles", 0, 0, 170},
		{"replacefiles", 0, 0, 171},
		{"remote-shell", 1, 0, 172},
		{"quiet-progress", 0, 0, 173},
		{"local-pax-write-command", 1, 0, 175},
		{"remote-pax-write-command", 1, 0, 176},
		{"pax-write-command", 1, 0, 177},
		{"remote-pax-read-command", 1, 0, 178},
		{"local-pax-read-command", 1, 0, 179},
		{"pax-read-command", 1, 0, 180},
		{"no-defaults-files", 0, 0, 181},
		{"pax-command", 1, 0, 182},
		{"show-progress-bar", 0, 0, 183},
		{"no-remote-kill", 0, 0, 184},
		{"no-getconf", 0, 0, 185},
		{"use-getconf", 0, 0, 186},
		{"progress-bar-fd", 1, 0, 187},
		{"shell-command", 1, 0, 188},
		{"preview-tar-file", 1, 0, 189},
		{"enforce-sig", 0, 0, 190},
		{"enforce-file-md5", 0, 0, 191},
		{"allow-rpm", 0, 0, 192},
		{"allow-missing-files", 0, 0, 157},
		{"pump-delay", 1, 0, 193},
		{"pump-delay1", 1, 0, 193},
		{"burst-adjust", 1, 0, 194},
		{"ssh-options", 1, 0, 195},
		{"debug-events", 0, 0, 196},
		{"to-stdout", 0, 0, 197},
		{"source-script-name", 1, 0, 198},
		{"target-script-name", 1, 0, 199},
		{"swi-debug-name", 1, 0, 200},
		{"debug-task-scripts", 0, 0, 201},
		{"enable-ssh-agent-forwarding", 0, 0, 202},
		{"A", 0, 0, 202},
		{"disable-ssh-agent-forwarding", 0, 0, 203},
		{"a", 0, 0, 203},
		{"ignore-scripts", 0, 0, 204},
		{"no-scripts", 0, 0, 204},
		{"noscripts", 0, 0, 204},
		{"sig-level", 1, 0, 205},
		{"keep-volatile-files", 0, 0, 206},
		{"sign", 0, 0, 207},
		{"justdb", 0, 0, 208},
		{"swbis-any-format", 		1, 0, 247},
		{"swbis_any_format", 		1, 0, 247},
		{"swbis-no-getconf", 		1, 0, 248},
		{"swbis_no_getconf", 		1, 0, 248},
		{"swbis-shell-command", 	1, 0, 249},
		{"swbis_shell_command", 	1, 0, 249},
		{"swbis-no-remote-kill", 	1, 0, 250},
		{"swbis_no_remote_kill", 	1, 0, 250},
		{"swbis-quiet-progress-bar", 	1, 0, 251},
		{"swbis_quiet_progress_bar", 	1, 0, 251},
		{"swbis-local-pax-write-command", 1, 0, 252},
		{"swbis_local_pax_write_command", 1, 0, 252},
		{"swbis-remote-pax-write-command", 1, 0, 253},
		{"swbis_remote_pax_write_command", 1, 0, 253},
		{"swbis-local-pax-read-command", 1, 0, 224},
		{"swbis_local_pax_read_command", 1, 0, 224},
		{"swbis-remote-pax-read-command", 1, 0, 225},
		{"swbis_remote_pax_read_command", 1, 0, 225},
		{"swbis-enforce-all-signatures",	1, 0, 226},
		{"swbis_enforce_all_signatures",	1, 0, 226},
		{"swbis-enforce-file-md5", 	1, 0, 227},
		{"swbis_enforce_file_md5", 	1, 0, 227},
		{"swbis-allow-rpm", 		1, 0, 228},
		{"swbis_allow_rpm", 		1, 0, 228},
		{"swbis-install-volatile", 	1, 0, 229},
		{"swbis_install_volatile", 	1, 0, 229},
		{"swbis-volatile-newname", 	1, 0, 230},
		{"swbis_volatile_newname", 	1, 0, 230},
		{"swbis-forward-agent", 	1, 0, 236},
		{"swbis_forward_agent", 	1, 0, 236},
		{"swbis-replacefiles", 		1, 0, 237},
		{"swbis_replacefiles", 		1, 0, 237},
		{"swbis-sig-level", 		1, 0, 241}, /* WOPT_LAST */
		{"swbis_sig_level", 		1, 0, 241}, /* WOPT_LAST */
		{0, 0, 0, 0}
             };

	struct option posix_extended_long_options[] =
             {
		{"allow_downdate", 	1, 0, 210},
		{"allow_incompatible", 	1, 0, 211},
		{"ask", 		1, 0, 212},
		{"autoreboot", 		1, 0, 213},
		{"autorecover", 	1, 0, 214},
		{"autoselect_dependencies", 	1, 0, 231},
		{"defer_configure", 	1, 0, 209},
		{"distribution_source_directory", 1, 0, 218},
		{"enforce_dependencies", 	1, 0, 219},
		{"enforce_dsa", 		1, 0, 220},
		{"logfile", 			1, 0, 221},
		{"loglevel", 			1, 0, 222},
		{"reinstall", 			1, 0, 223},
		{"select_local", 		1, 0, 239},
		{"verbose", 			1, 0, 240},
		{"enforce_locatable", 		1, 0, 215},
		{"enforce_scripts", 		1, 0, 216},
		{"installed_software_catalog",	1, 0, 217},
               {0, 0, 0, 0}
             };

	struct option std_long_options[] =
             {
		{"selections-file", 1, 0, 'f'},
		{"preview", 0, 0, 'p'},
		{"installed-software", 0, 0, 'r'},
		{"target-file", 1, 0, 't'},
		{"source", 1, 0, 's'},
		{"extended-option", 1, 0, 'x'},
		{"defaults-file", 1, 0, 'X'},
		{"extension-option", 1, 0, 'W'},
		{"verbose", 0, 0, 'v'},
		{"block-size", 1, 0, 'b'},
		{"version", 0, 0, 'V'},
		{"help", 0, 0, '\012'},
               {0, 0, 0, 0}
             };
       
	struct ugetopt_option_desc main_help_desc[] =
             {
	{"", "FILE","Take software selections from FILE."},
	{"", "", "Preview only by showing information to stdout"},
	{"", "", "causes swinstall to operate on installed software\n"
	"          located at an alternate root."},
	{"", "FILE", "Specify a FILE containing a list of targets."
	},
	{"", "source", "Specify the file, directory, or '-' for stdin.\n"
	"                     source may have target syntax."
	},
	{"", "option=value", "Specify posix extended option."},
	{"", "FILE[ FILE2 ...]", "Specify files that override \n"
	"        system option defaults. Specify empty string to disable \n"
	"        option file reading."
	},
	{"", "option[=value]", "Specify implementation extension option."},
	{"verbose", "",  "same as '-x verbose=2'\n"
	"     (implementation extension).\n"
	"     -v  is level 2, -vv is level 3,... etc.\n"
	"         level 0: silent on stdout and stderr.\n"
	"         level 1: fatal and warning messages to stderr.\n"
	"     -v  level 2: level 1 plus a progress bar.\n"
	"     -vv level 3: level 2 plus script stderr.\n"
	"     -vvv level 4: level 3 plus events.\n"
	"     -vvvv level 5: level 4 plus events.\n"
	"     -vvvvv level 6: level 5 plus set shell -vx option.\n"
	"     -vvvvvv level 7 and higher: level 6 plus debugging messages.\n"
	},
	{"block-size", "N", "specify block size N octets, 0<N<=32256\n"
	"          default size is 5120. (implementation extension)."},
	{"version", "", "Show version information to stdout. (impl. extension)."},
	{"help", "", "Show this help to stdout (impl. extension)."},
	{0, 0, 0}
	};

	/*
	 *  = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  End of Decalarations.
	 *  = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	/*
	 * open the sw utility object, pile all paramater that
	 * cry out to be global into this object so that they are
	 * contained.
	 */

	installer_sig = NULL;
	wopt_sign = 0;
	dummy_catalog = NULL;
	G = &g_blob;
	gb_init(G);
	G->g_target_fdar = target_fdar;
	G->g_source_fdar = source_fdar;
	pid_array = G->g_pid_array;
        status_array = G->g_status_array;
        p_pid_array_len = &G->g_pid_array_len;
	G->g_main_sig_handler = main_sig_handler;
	G->g_safe_sig_handler = safe_sig_handler;
	G->optaM = opta;

	swlog = swutil_open();
	G->g_swlog = swlog;

	G->g_logspec.logfdM = -1;
	G->g_logspec.loglevelM = 0;
	G->g_logspec.fail_loudlyM = -1;
	swc_set_stderr_fd(G, STDERR_FILENO);
	swlib_utilname_set(progName);
	swgp_check_fds();
	p_do_progressmeter = &G->g_do_progressmeter;

	G->g_targetfd_array[0] = -1;
	statbytes = 0;
	fork_defaultmask = &G->g_fork_defaultmask;
	fork_blockmask = &G->g_fork_blockmask;
	ssh_fork_blockmask = &G->g_ssh_fork_blockmask;
	currentmask = &G->g_currentmask;
	sigemptyset(currentmask);
	sigemptyset(fork_blockmask);
	sigaddset(fork_blockmask, SIGALRM);
	sigemptyset(fork_defaultmask);
	sigaddset(fork_defaultmask, SIGINT);
	sigaddset(fork_defaultmask, SIGPIPE);
	sigemptyset(ssh_fork_blockmask);
	
	target_current_dir = strob_open(10);
	source_line_buf = strob_open(10);
	target_line_buf = strob_open(10);

	uxfio_devnull_open("/dev/null", O_RDWR, 0);  /* initiallize the null fd */
	
	G->g_vstderr = stderr;
	nullfd = G->g_nullfd;

	target_fdar[0] = -1;
	target_fdar[1] = -1;
	target_fdar[2] = -1;
	source_fdar[0] = -1;
	source_fdar[1] = -1;
	source_fdar[2] = -1;
	
	overwrite_fd = -1;
	n_file_conflicts = 0;

	source_stdio_fdar[0] = (STDIN_FILENO);
	source_stdio_fdar[1] = (STDOUT_FILENO);
	source_stdio_fdar[2] = (STDERR_FILENO);

	target_stdio_fdar[0] = (STDIN_FILENO);
	target_stdio_fdar[1] = (STDOUT_FILENO);
	target_stdio_fdar[2] = (STDERR_FILENO);

	save_stdio_fdar[0] = dup(STDIN_FILENO);
	save_stdio_fdar[1] = dup(STDOUT_FILENO);
	save_stdio_fdar[2] = dup(STDERR_FILENO);

	swc0_create_parser_buffer();
	
	target_sshcmd[1] = (SHCMD*)NULL;
	source_sshcmd[1] = (SHCMD*)NULL;
	kill_sshcmd[1] = (SHCMD*)NULL;
	target_kill_sshcmd[1] = (SHCMD*)NULL;
	source_kill_sshcmd[1] = (SHCMD*)NULL;
	
	kill_sshcmd[0] = (SHCMD*)NULL;
	source_sshcmd[0] = shcmd_open();
	target_sshcmd[0] = (SHCMD*)NULL;
	target_kill_sshcmd[0] = shcmd_open();
	source_kill_sshcmd[0] = shcmd_open();
		

	tmp = strob_open(10);		/* General use String object. */
	tmpcommand = strob_open(10);
	w_arglist = cplob_open(1);	/* Pointer list object. */
	e_arglist = cplob_open(1);	/* Pointer list object. */

	initExtendedOption();

	if (getcwd(cwd, sizeof(cwd) - 1) == NULL) {
		sw_e_msg(G, "%s\n", strerror(errno));
		LCEXIT(1);
	}
	cwd[sizeof(cwd) - 1] = '\0';

	uinfile_decode_flags = UINFILE_DETECT_FORCEUXFIOFD |
			UINFILE_DETECT_UNCPIO |
			UINFILE_DETECT_UNRPM |
			UINFILE_DETECT_IEEE;

	/*
	* Set the compiled-in defaults for the extended options.
	*/
	eopt_allow_downdate		= CHARFALSE;
	eopt_allow_incompatible		= CHARFALSE;
	eopt_ask			= CHARFALSE;
	eopt_autoreboot			= CHARFALSE;
	eopt_autorecover		= CHARFALSE;
	eopt_autoselect_dependencies	= CHARFALSE;
	eopt_defer_configure		= CHARFALSE;
	eopt_distribution_source_directory = "-";
	eopt_enforce_dependencies	= CHARFALSE;
	eopt_enforce_locatable		= CHARFALSE;
	eopt_enforce_scripts		= CHARFALSE;
	eopt_enforce_scripts		= CHARFALSE;
	eopt_enforce_dsa		= CHARFALSE;
	eopt_installed_software_catalog = "var/lib/swbis/catalog/";
	eopt_logfile			= "/var/log/swinstall.log";
	eopt_loglevel			= "0";
	eopt_reinstall			= CHARFALSE;
	eopt_select_local		= CHARFALSE;
	eopt_verbose			= "1";

	wopt_tar_format			= SWBIS_A_pax; /* default */
	wopt_no_getconf			= "true";
	wopt_no_remote_kill		= "true";
	wopt_quiet_progress_bar		= "true";
	wopt_show_progress_bar		= CHARFALSE;
	wopt_shell_command		= "detect";
	wopt_any_format			= "true";
	wopt_local_pax_write_command 	= "tar";
	wopt_remote_pax_write_command 	= "tar";
	wopt_local_pax_read_command 	= "tar";
	wopt_remote_pax_read_command 	= "tar";
	wopt_enforce_all_signatures	= CHARFALSE;
	wopt_swbis_replacefiles		= CHARFALSE;
	wopt_enforce_file_md5		= CHARFALSE;
	wopt_allow_rpm			= CHARFALSE;
	wopt_install_volatile		= "true";
	wopt_sig_level			= "0";
	wopt_forward_agent		= "true";
	wopt_force 			= NULL;
	wopt_noscripts			= CHARFALSE;
	wopt_volatile_newname		= "";

	wopt_show_options = NULL;
	wopt_show_options_files = NULL;

	set_opta_initial(opta, SW_E_allow_downdate, eopt_allow_downdate);
	set_opta_initial(opta, SW_E_allow_incompatible, eopt_allow_incompatible);
	set_opta_initial(opta, SW_E_ask, eopt_ask);
	set_opta_initial(opta, SW_E_autoreboot, eopt_autoreboot);
	set_opta_initial(opta, SW_E_autorecover, eopt_autorecover);
	set_opta_initial(opta, SW_E_autoselect_dependencies, 
					eopt_autoselect_dependencies);
	set_opta_initial(opta, SW_E_defer_configure, eopt_defer_configure);
	set_opta_initial(opta, SW_E_distribution_source_directory, 
					eopt_distribution_source_directory);
	set_opta_initial(opta, SW_E_enforce_dependencies, 
					eopt_enforce_dependencies);
	set_opta_initial(opta, SW_E_enforce_locatable, eopt_enforce_locatable);
	set_opta_initial(opta, SW_E_enforce_scripts, eopt_enforce_scripts);
	set_opta_initial(opta, SW_E_enforce_dsa, eopt_enforce_dsa);
	set_opta_initial(opta, SW_E_installed_software_catalog, eopt_installed_software_catalog);
	set_opta_initial(opta, SW_E_logfile, eopt_logfile);
	set_opta_initial(opta, SW_E_loglevel, eopt_loglevel);
	set_opta_initial(opta, SW_E_reinstall, eopt_reinstall);
	set_opta_initial(opta, SW_E_select_local, eopt_select_local);
	set_opta_initial(opta, SW_E_verbose, eopt_verbose);

	set_opta_initial(opta, SW_E_swbis_format, wopt_tar_format);
	set_opta_initial(opta, SW_E_swbis_no_getconf, wopt_no_getconf);
	set_opta_initial(opta, SW_E_swbis_shell_command, wopt_shell_command);
	set_opta_initial(opta, SW_E_swbis_any_format, wopt_any_format);
	set_opta_initial(opta, SW_E_swbis_remote_shell_client, remote_shell_command);
	set_opta_initial(opta, SW_E_swbis_no_remote_kill, wopt_no_remote_kill);
	set_opta_initial(opta, SW_E_swbis_quiet_progress_bar,
					wopt_quiet_progress_bar);
	set_opta_initial(opta, SW_E_swbis_local_pax_write_command,
					wopt_local_pax_write_command);
	set_opta_initial(opta, SW_E_swbis_remote_pax_write_command,
					wopt_remote_pax_write_command);
	set_opta_initial(opta, SW_E_swbis_local_pax_read_command,
					wopt_local_pax_read_command);
	set_opta_initial(opta, SW_E_swbis_remote_pax_read_command,
					wopt_remote_pax_read_command);
	set_opta_initial(opta, SW_E_swbis_enforce_all_signatures, wopt_enforce_all_signatures);
	set_opta_initial(opta, SW_E_swbis_replacefiles, wopt_swbis_replacefiles);
	set_opta_initial(opta, SW_E_swbis_enforce_file_md5, wopt_enforce_file_md5);
	set_opta_initial(opta, SW_E_swbis_allow_rpm, wopt_allow_rpm);
	set_opta_initial(opta, SW_E_swbis_install_volatile, wopt_install_volatile);
	set_opta_initial(opta, SW_E_swbis_volatile_newname, wopt_volatile_newname);
	set_opta_initial(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
	set_opta_initial(opta, SW_E_swbis_ignore_scripts, wopt_noscripts);
	set_opta_initial(opta, SW_E_swbis_sig_level, wopt_sig_level);

	cplob_add_nta(w_arglist, strdup(argv[0]));
	cplob_add_nta(e_arglist, strdup(argv[0]));

	while (1) {
		int option_index = 0;

		c = ugetopt_long(argc, argv, "b:f:t:prVvs:X:x:W:AH:", 
					main_long_options, &option_index);
		if (c == -1) break;

		/* SWLIB_WARN2("here %c\n", (char)(c)); */

		switch (c) {
		case 'p':
			opt_preview = 1;
			G->g_opt_previewM = 1;
			break;
		case 'r':
			G->g_opt_alt_catalog_root = 1;
			break;
		case 'v':
			G->g_verboseG++;
			eopt_verbose = (char*)malloc(12);
			snprintf(eopt_verbose, 12, "%d", G->g_verboseG);
			eopt_verbose[11] = '\0';
			set_opta(opta, SW_E_verbose, eopt_verbose);
			break;
		case 'f':
			if (swc_process_swoperand_file(swlog,
				"selections", optarg, &stdin_in_use,
				&select_array_index, G->g_selectfd_array))
			{
				LCEXIT(1);
			}
			break;
		case 'b':
			wopt_blocksize = strdup(optarg);
			{
				int n;
				char * t = wopt_blocksize;
				while (*t) {
					if (!isdigit((int)(*t))) {
						fprintf(stderr,
						"illegal block size\n");
						exit(1);	
					}
					t++;
				}
				n = swlib_atoi(wopt_blocksize, NULL);
				if (n <= 0 || n > 32256) {
					fprintf(stderr,
					"illegal block size\n");
					exit(1);	
				}
			}
			break;
		case 't':
			/*
			 * Process the target file
			 */
			if (swc_process_swoperand_file(swlog,
				"target", optarg, &stdin_in_use,
				&target_array_index, G->g_targetfd_array))
			{
				LCEXIT(1);
			}
			break;
		case 's':
			soc_spec_source = strdup(optarg);
			if (*soc_spec_source != '@') {
				strob_strcpy(tmp, "@");
				strob_strcat(tmp, soc_spec_source);
				soc_spec_source = strdup(strob_str(tmp));
			}
			SWLIB_ALLOC_ASSERT(soc_spec_source != NULL);
			break;

		case 'W':
			swc0_process_w_option(tmp, w_arglist, optarg, &w_argc);
			break;
		case 'x':
			exoption_arg = strdup(optarg);
			SWLIB_ALLOC_ASSERT(exoption_arg != NULL);
			/*
			 * 
			 *  Parse the extended option and add to pointer list
			 *  for later processing.
			 * 
			 */
			{
				char * np;
				char * t;
				STROB * etmp = strob_open(10);
				
				t = strob_strtok(etmp, exoption_arg, " ");
				while (t) {
					exoption_value = strchr(t,'=');
					if (!exoption_value) {
						sw_e_msg(G, "invalid extended arg : %s\n", optarg);
						_exit(1);
					}
					np = exoption_value;
					*exoption_value = '\0';
					exoption_value++;
					/*
					 * 
					 *  Now add the option and value to 
					 *  the arg list.
					 * 
					 */
					strob_strcpy(tmp, "--");
					strob_strcat(tmp, t);
					strob_strcat(tmp, "=");
					strob_strcat(tmp, exoption_value);
					cplob_add_nta(e_arglist, 
						strdup(strob_str(tmp)));
					e_argc++;

					*np = '=';
					t = strob_strtok(etmp, NULL, " ");
				}
				/*
				*  setExtendedOption(optarg, exoption_value);
				*/
				strob_close(etmp);
			}
			break;
		case 'A':
			wopt_forward_agent = CHARTRUE;
			set_opta(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
			break;

		case 'H':
			if (
				strcmp(optarg, SWBIS_A_ustar) &&
				strcmp(optarg, SWBIS_A_gnu) &&
				strcmp(optarg, SWBIS_A_pax) &&
				strcmp(optarg, SWBIS_A_posix) 
			) {
				sw_e_msg(G, "invalid format : %s\n", optarg);
				_exit(1);
			}
			wopt_tar_format = strdup(optarg);
			set_opta(opta, SW_E_swbis_format, wopt_tar_format);
			break;

		case 'X':
			if (opt_option_files) {
				opt_option_files = (char*)realloc(
						(void*)opt_option_files,
						strlen(opt_option_files) \
						+ strlen(optarg) + 2);
				strcat(opt_option_files, " ");
				strcat(opt_option_files, optarg);
			} else {
				opt_option_files = strdup(optarg);
			}
			break;
               case 'V':
			version_info(stdout);
			LCEXIT(0);
		 	break;
               case '\012':
			usage(stdout, 
				argv[0], 
				std_long_options, 
				main_help_desc);
			LCEXIT(0);
		 	break;
               case '?':
			sw_e_msg(G, "Try `swinstall --help' for more information.\n");
			LCEXIT(1);
		 	break;
               default:
			if (c >= WOPT_FIRST && c <= WOPT_LAST) { 
				/*
				 *  This provides the ablility to specify 
				 *  extension options by using the 
				 *  --long-option syntax (i.e. without using 
				 *  the -Woption syntax) .
				 */
				do_extension_options_via_goto = 1;
				E_DEBUG2("doing goto gotoExtensionOptions with c=%d", c);
				goto gotoExtensionOptions;
gotoStandardOptions:
				;
			} else {
				sw_e_msg(G, "invalid args.\n");
		 		LCEXIT(1);
			}
			break;
               }
	}

	main_optind = optind;

	optind = 1;
	optarg =  NULL;
		
	while (1) {
		int e_option_index = 0;
		c = ugetopt_long (e_argc, 
				cplob_get_list(e_arglist), 
				"", 
				posix_extended_long_options, 
				&e_option_index);
		if (c == -1) break;
		
		switch (c) {
		
		case 231:
			set_opta_boolean(opta, SW_E_autoselect_dependencies, optarg);
			break;
		case 218:
			eopt_distribution_source_directory = strdup(optarg);
			set_opta(opta, 
				SW_E_distribution_source_directory, 
				eopt_distribution_source_directory);
			SWLIB_ALLOC_ASSERT
				(eopt_distribution_source_directory != NULL);
			break;
		case 219:
			set_opta_boolean(opta, SW_E_enforce_dependencies, optarg);
			break;

		case 220:
			set_opta_boolean(opta, SW_E_enforce_dsa, optarg);
			break;
		case 221:
			eopt_logfile = strdup(optarg);
			set_opta(opta, SW_E_logfile, eopt_logfile);
			SWLIB_ALLOC_ASSERT(eopt_logfile != NULL);
			break;

		case 222:
			eopt_loglevel = strdup(optarg);
			opt_loglevel = swlib_atoi(optarg, NULL);
			G->g_loglevel = opt_loglevel;
			G->g_logspec.loglevelM = opt_loglevel;
			set_opta(opta, SW_E_loglevel, eopt_loglevel);
			SWLIB_ALLOC_ASSERT(eopt_loglevel != NULL);
			break;

		case 223:
			swc_set_boolean_x_option(opta, 
					SW_E_reinstall, 
					optarg, 
					&eopt_reinstall);
			break;
		case 239:
			swc_set_boolean_x_option(opta, 
				SW_E_select_local, 
				optarg, 
				&eopt_select_local);
			break;
		case 240:
			eopt_verbose = strdup(optarg);
			G->g_verboseG = swlib_atoi(eopt_verbose, NULL);
			set_opta(opta, SW_E_verbose, eopt_verbose);
			break;
		case 209:
			eopt_defer_configure = strdup(optarg);
			set_opta_boolean(opta, SW_E_defer_configure, eopt_defer_configure);
			break;
		case 210:
			eopt_allow_downdate = strdup(optarg);
			set_opta_boolean(opta, SW_E_allow_downdate, eopt_allow_downdate);
			break;
		case 211:
			eopt_allow_incompatible = strdup(optarg);
			set_opta_boolean(opta, SW_E_allow_incompatible, eopt_allow_incompatible);
			break;
		case 212:
			eopt_ask = strdup(optarg);
			set_opta(opta, SW_E_ask, eopt_ask);
			break;
		case 213:
			eopt_autoreboot = strdup(optarg);
			set_opta_boolean(opta, SW_E_autoreboot, eopt_autoreboot);
			break;
		case 214:
			eopt_autorecover = strdup(optarg);
			set_opta_boolean(opta, SW_E_autorecover, eopt_autorecover);
			break;
		case 215:
			eopt_enforce_locatable = strdup(optarg);
			set_opta_boolean(opta, SW_E_enforce_locatable, eopt_enforce_locatable);
			break;
		case 216:
			eopt_enforce_scripts = strdup(optarg);
			set_opta_boolean(opta, SW_E_enforce_scripts, eopt_enforce_scripts);
			break;
		case 217:
			eopt_installed_software_catalog = strdup(optarg);
			set_opta(opta, SW_E_installed_software_catalog, eopt_installed_software_catalog);
			break;
		default:
			sw_e_msg(G, "error processing extended option\n");
			sw_e_msg(G,"Try `swinstall --help' for more information.\n");
		 	LCEXIT(1);
		break;
		}
		if (swextopt_get_status()) {
			optionname = getLongOptionNameFromValue(posix_extended_long_options, c);
			sw_e_msg(G, "bad value detected for extended option '%s': %s\n", optionname, optarg);
			LCEXIT(1);
		}
	}
	optind = 1;
	optarg =  NULL;

	/*
	 * 
	 *  Now run the Implementation extension options (-W) through getopt.
	 * 
	 */

	while (1) {
		int w_option_index = 0;
		c = ugetopt_long(w_argc, 
				cplob_get_list(w_arglist), 
				"", 
				main_long_options, 
				&w_option_index);
		if (c == -1) break;
	
gotoExtensionOptions:    /* Goto, Uhhg... */
		switch (c) {
		case 'b':
			wopt_blocksize = strdup(optarg);
			{
				int n;
				char * t = wopt_blocksize;
				while (*t) {
					if (!isdigit((int)*t)) {
						fprintf(stderr,
						"illegal block size\n");
						exit(1);	
					}
					t++;
				}
				n = swlib_atoi(wopt_blocksize, NULL);
				if (n <= 0 || n > 32256) {
					fprintf(stderr,
					"illegal block size\n");
					exit(1);	
				}
			}
			break;
		case 150: 
			/* --enforce-all-signature */
			set_opta_initial(opta, SW_E_swbis_enforce_all_signatures, CHARTRUE);
			optionEnum = getEnumFromName("swbis_sig_level", opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, "-1"); /* negative 1 means all present */
			break;
		case 151: 
			/* --nosignature */
			/* same as --sig-level=0 */
			optionEnum = getEnumFromName("swbis_sig_level", opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, "0");
			break;
		case 152: /* WOPT_FIRST */
			/* --nodeps */
			swc_set_boolean_x_option(opta, 
				SW_E_enforce_dependencies, 
				CHARFALSE, 
				&eopt_enforce_dependencies);
			break;
		case 153:
			uinfile_decode_flags &= (~UINFILE_DETECT_IEEE);
			uinfile_decode_flags |= (UINFILE_DETECT_SLACK);
				/* RPM, dpkg, do not require a special flag to detect */
			break;
		case 154:
			break;
		case 157:
			wopt_allow_missing = 1;
			break;
		case 158:
			wopt_show_options = CHARTRUE;
			break;
		case 159:
			wopt_show_options_files = CHARTRUE;
			break;
		case 164:
			/*
			 * Development testing controls.
			 */
			testnum = swlib_atoi(optarg, NULL);
			if (testnum == 10) {
				devel_no_fork_optimization = 1;
			} else if (testnum == 11) {
				wopt_with_hop_pids = 1;
			}
			break;
		case 165:
			G->devel_verboseM = 1;
			break;
		case 166:
			swc_set_boolean_x_option(opta, 
				SW_E_enforce_dependencies, 
				CHARFALSE, 
				&eopt_enforce_dependencies);
			swc_set_boolean_x_option(opta, 
				SW_E_allow_downdate, 
				"true", 
				&eopt_allow_downdate);
			swc_set_boolean_x_option(opta, 
				SW_E_reinstall, 
				"true", 
				&eopt_reinstall);
			swc_set_boolean_x_option(opta, 
				SW_E_enforce_locatable, 
				CHARFALSE, 
				&eopt_enforce_locatable);
			swc_set_boolean_x_option(opta, 
				SW_E_allow_incompatible, 
				"true", 
				&eopt_allow_incompatible);
			G->g_force = 1;
			wopt_force = CHARTRUE;
			break;
		case 167:
			G->g_force_locks = 1;
			break;
		case 168:
			/* --reinstall */
			/* wopt_swbis_replacefiles = CHARTRUE; */
			set_opta(opta, SW_E_swbis_replacefiles, CHARTRUE);
			swc_set_boolean_x_option(opta, SW_E_reinstall, 
					"true", &eopt_reinstall);
			break;
		case 155:
			set_opta_boolean(opta, SW_E_enforce_scripts, CHARFALSE);
			break;
		case 156:
			/* no-chdir-for-scripts */
			G->g_no_script_chdirM = 1;
			break;
		case 169:
			set_opta_boolean(opta, SW_E_enforce_scripts, CHARTRUE);
			break;
		case 170:
			wopt_keep_old_files = 1;
			break;
		case 171:
			wopt_swbis_replacefiles = CHARTRUE;
			set_opta(opta, SW_E_swbis_replacefiles, CHARTRUE);
			break;
		case 172:
			remote_shell_command = strdup(optarg);
			set_opta(opta, SW_E_swbis_remote_shell_client, remote_shell_command);
			break;
		case 173:
			wopt_quiet_progress_bar = CHARTRUE;
			set_opta(opta, SW_E_swbis_quiet_progress_bar, CHARTRUE);
			break;
		case 176:
			wopt_remote_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(G->g_pax_write_commands,    
				wopt_remote_pax_write_command, G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax write command: %s \n", wopt_remote_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_remote_pax_write_command,
						wopt_remote_pax_write_command);
			break;
		case 175:
			wopt_local_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(G->g_pax_write_commands,
					wopt_local_pax_write_command,
							 G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax write command: %s \n", wopt_local_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_write_command,
					wopt_local_pax_write_command);
			break;
		case 177:
			wopt_local_pax_write_command = strdup(optarg);
			wopt_remote_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(G->g_pax_write_commands,
					wopt_local_pax_write_command,
							G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax write command: %s \n",
						wopt_local_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_write_command,  
						wopt_local_pax_write_command);
			set_opta(opta, SW_E_swbis_remote_pax_write_command,  
						wopt_local_pax_write_command);
			break;
		case 178:
			wopt_remote_pax_read_command = strdup(optarg);
			xcmd = swc_get_pax_read_command(G->g_pax_read_commands,
					wopt_remote_pax_read_command,
						 0, 0, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax read command: %s \n",
						wopt_remote_pax_read_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_remote_pax_read_command,
						wopt_remote_pax_read_command);
			break;
		case 179:
			wopt_local_pax_read_command = strdup(optarg);
			xcmd = swc_get_pax_read_command(G->g_pax_read_commands,
						wopt_local_pax_read_command, 
							0, 0, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax read command: %s \n", wopt_local_pax_read_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_read_command,  
						wopt_local_pax_read_command);
			break;
		case 180:
			wopt_local_pax_read_command = strdup(optarg);
			wopt_remote_pax_read_command = strdup(optarg);
			xcmd = swc_get_pax_read_command(G->g_pax_read_commands,
						wopt_local_pax_read_command,
							0, 0, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax read command: %s \n",
					wopt_local_pax_read_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_read_command,
						wopt_local_pax_read_command);
			set_opta(opta, SW_E_swbis_remote_pax_read_command,
						wopt_local_pax_read_command);
			break;
		case 181:
			wopt_no_defaults = 1;
			break;
		case 182:
			wopt_local_pax_read_command = strdup(optarg);
			wopt_remote_pax_read_command = strdup(optarg);
			wopt_local_pax_write_command = strdup(optarg);
			wopt_remote_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_read_command(G->g_pax_read_commands,
						wopt_local_pax_read_command,
							0, 0, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax read command: %s \n",
					wopt_local_pax_read_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_read_command,
						wopt_local_pax_read_command);
			set_opta(opta, SW_E_swbis_remote_pax_read_command,
						wopt_local_pax_read_command);
			set_opta(opta, SW_E_swbis_local_pax_write_command,
						wopt_local_pax_write_command);
			set_opta(opta, SW_E_swbis_remote_pax_write_command,
						wopt_local_pax_write_command);
			break;
		case 183:
			wopt_show_progress_bar = CHARTRUE;
			wopt_quiet_progress_bar = CHARFALSE;
			set_opta(opta, SW_E_swbis_quiet_progress_bar, CHARFALSE);
			break;
		case 184:
			wopt_no_remote_kill = CHARTRUE;
			set_opta(opta, SW_E_swbis_no_remote_kill, "true");
			break;
		case 185:
			wopt_no_getconf = CHARTRUE;
			set_opta(opta, SW_E_swbis_no_getconf, "true");
			break;
		case 186:
			wopt_no_getconf = CHARFALSE;
			set_opta(opta, SW_E_swbis_no_getconf, CHARFALSE);
			break;
		case 187:
			G->g_meter_fd = swlib_atoi(optarg, NULL);
			if (G->g_meter_fd != 1 && G->g_meter_fd != 2) {
				G->g_meter_fd = 1;
			}
			break;
		case 188:
			wopt_shell_command = strdup(optarg);
			swc_check_shell_command_key(G, wopt_shell_command);
			set_opta(opta, SW_E_swbis_shell_command, wopt_shell_command);
			break;
		case 189:
			if (strcmp(optarg, "-") == 0) {
				G->g_stdout_testfd = STDOUT_FILENO;
			} else {
				G->g_stdout_testfd = open(optarg, O_RDWR|O_CREAT|O_TRUNC, 640);
			}
			break;
		case 190:
			wopt_enforce_all_signatures = CHARTRUE;
			set_opta(opta, SW_E_swbis_enforce_all_signatures, "true");
			break;
		case 191:
			wopt_enforce_file_md5 = CHARTRUE;
			set_opta(opta, SW_E_swbis_enforce_file_md5, "true");
			break;
		case 192:
			wopt_allow_rpm = CHARTRUE;
			wopt_allow_missing = 1;
			set_opta(opta, SW_E_swbis_allow_rpm, "true");
			break;
		case 193:
			wopt_pump_delay1 = swlib_atoi(optarg, NULL);
			break;
		case 194:
			wopt_burst_adjust = swlib_atoi(optarg, NULL);
			break;
		case 195:
			wopt_ssh_options = strdup(optarg);
			break;
		case 196:
			G->g_do_debug_events = 1;
			wopt_debug_events = 1;
			break;
		case 197:
			do_to_stdout = 1;
			break;
		case 198:
			G->g_source_script_name = strdup(optarg);
			break;
		case 199:
			G->g_target_script_name = strdup(optarg);
			break;
		case 200:
			G->g_swi_debug_name = strdup(optarg);
			break;
		case 201:
			G->g_do_task_shell_debug = 1;
			break;
		case 202:
			wopt_forward_agent = CHARTRUE;
			set_opta(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
			break;
		case 203:
			wopt_forward_agent = CHARFALSE;
			set_opta(opta, SW_E_swbis_forward_agent, CHARFALSE);
			break;
		case 204:
			wopt_noscripts = CHARTRUE;
			G->g_noscripts = 1;
			set_opta(opta, SW_E_swbis_ignore_scripts, wopt_noscripts);
			break;
		case 205:
			optionEnum = getEnumFromName("swbis_sig_level", opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			if (strcmp(optarg, "-1") == 0) {
				/* set --enforce-all-signatures */
				set_opta_initial(opta, SW_E_swbis_enforce_all_signatures, CHARTRUE);
			}
			break;
		case 206:
			set_opta(opta, SW_E_swbis_install_volatile, CHARTRUE);
			set_opta(opta, SW_E_swbis_volatile_newname, SWINSTALL_VOLATILE_SUFFIX);
			break;
		case 207:
			wopt_sign = 1;
			break;
		case 208:
			G->g_justdbM = 1;
			break;

			/* Non-Boolean Extended options */
		case 249:
		case 252:
		case 253:
		case 224:
		case 225:
		case 230:
		case 241:

			E_DEBUG("running with getLongOptionNameFromValue");
			optionname = getLongOptionNameFromValue(main_long_options , c);
			E_DEBUG2("option name from getLongOptionNameFromValue is [%s]", optionname);
			SWLIB_ASSERT(optionname != NULL);
			E_DEBUG2("running getEnumFromName with name [%s]", optionname);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			break;

			/* Boolean Extended options */
		case 247:
		case 248:
		case 250:
		case 251:
		case 226:
		case 227:
		case 228:
		case 229:
		case 236:
		case 237:

			optionname = getLongOptionNameFromValue(main_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			E_DEBUG2("option name is [%s]", optionname);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta_boolean(opta, optionEnum, optarg);
			break;

		default:
			sw_e_msg(G, "error processing implementation extension option\n");
		 	exit(1);
		break;
		}
		if (do_extension_options_via_goto == 1) {
			do_extension_options_via_goto = 0;
			goto gotoStandardOptions;
		}
	}

	if (opt_preview) {
		G->g_verbose_threshold = SWC_VERBOSE_1;
	}

	optind = main_optind;

	system_defaults_files = initialize_options_files_list(NULL);

	/*
	 * = = = = = = = = = = = = = = =
	 *  Show the options to stdout.
	 * = = = = = = = = = = = = = = =
	 */
	if (wopt_show_options_files) { 
		/* Show the file names of the options files */
		swextopt_parse_options_files(opta, 
			system_defaults_files, 
			progName, 1 /* not req'd */,  1 /* show_only */);
		swextopt_parse_options_files(opta, 
			opt_option_files, 
			progName,
		 	1 /* not req'd */,  1 /* show_only */);
		LCEXIT(0);
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  Read the system defaults files and home directory copies
	 *  if HOME is set.
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */
	if (wopt_no_defaults == 0) {
		optret += swextopt_parse_options_files(opta, 
			system_defaults_files, 
			progName, 
			0 /* not req'd */,  0 /* not show_only */);
		if (optret) {
			sw_e_msg(G, "defaults file error\n");
			LCEXIT(1);
		}
	}
	
 	*swlib_burst_adjust_p() = wopt_burst_adjust;
	if (wopt_pump_delay1 > 0) {
 		*swlib_get_io_req_p() = &G->g_io_req;
		G->g_io_req.tv_sec = 0;
		G->g_io_req.tv_nsec = wopt_pump_delay1;
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  Read the defaults files given with the -X option.
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */
	optret += swextopt_parse_options_files(opta, 
		opt_option_files, 
		progName, 	
		1 /* req'd */, 0 /* not show_only */);
	if (optret) {
		sw_e_msg(G, "defaults file error\n");
		LCEXIT(1);
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  Reset the option values to pick up the values from 
	 *  the defaults file(s).
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	eopt_allow_downdate = get_opta(opta, SW_E_allow_downdate);
	eopt_allow_incompatible = get_opta(opta, SW_E_allow_incompatible);
	eopt_ask = get_opta(opta, SW_E_ask);
	eopt_autoreboot = get_opta(opta, SW_E_autoreboot);
	eopt_autorecover = get_opta(opta, SW_E_autorecover);
	eopt_autoselect_dependencies	= get_opta(opta, SW_E_autoselect_dependencies);
	eopt_defer_configure	= get_opta(opta, SW_E_defer_configure);
	eopt_distribution_source_directory = get_opta(opta, 
					SW_E_distribution_source_directory);
	eopt_enforce_dependencies	= get_opta(opta, SW_E_enforce_dependencies);
	eopt_enforce_locatable		= get_opta(opta, SW_E_enforce_locatable);
	eopt_enforce_scripts		= get_opta(opta, SW_E_enforce_scripts);
	eopt_enforce_dsa		= get_opta(opta, SW_E_enforce_dsa);
	eopt_installed_software_catalog = get_opta_isc(opta, SW_E_installed_software_catalog);
	eopt_logfile			= get_opta(opta, SW_E_logfile);
	eopt_loglevel			= get_opta(opta, SW_E_loglevel);
			G->g_loglevel = swlib_atoi(eopt_loglevel, NULL);
			opt_loglevel = G->g_loglevel;
			G->g_logspec.loglevelM = opt_loglevel;
	eopt_reinstall			= get_opta(opta, SW_E_reinstall);
	eopt_select_local		= get_opta(opta, SW_E_select_local);
	eopt_verbose			= get_opta(opta, SW_E_verbose);
		G->g_verboseG = swlib_atoi(eopt_verbose, NULL);
	
	wopt_no_remote_kill		= swbisoption_get_opta(opta, 
							SW_E_swbis_no_remote_kill);
	wopt_no_getconf			= swbisoption_get_opta(opta, 
							SW_E_swbis_no_getconf);
	wopt_shell_command		= swbisoption_get_opta(opta, 
							SW_E_swbis_shell_command);
	remote_shell_command		= swbisoption_get_opta(opta, 
							SW_E_swbis_remote_shell_client);
	wopt_quiet_progress_bar		= swbisoption_get_opta(opta, 
						SW_E_swbis_quiet_progress_bar);
	wopt_local_pax_write_command	= swbisoption_get_opta(opta, 	
					SW_E_swbis_local_pax_write_command);
	wopt_remote_pax_write_command	= swbisoption_get_opta(opta, 
					SW_E_swbis_remote_pax_write_command);
	wopt_local_pax_read_command	= swbisoption_get_opta(opta,
						SW_E_swbis_local_pax_read_command);
	wopt_remote_pax_read_command	= swbisoption_get_opta(opta,
						SW_E_swbis_remote_pax_read_command);

	wopt_enforce_all_signatures		= swbisoption_get_opta(opta, SW_E_swbis_enforce_all_signatures);
	wopt_swbis_replacefiles		= swbisoption_get_opta(opta, SW_E_swbis_replacefiles);
	wopt_sig_level			= swbisoption_get_opta(opta, SW_E_swbis_sig_level);
	wopt_forward_agent		= swbisoption_get_opta(opta, SW_E_swbis_forward_agent);
	wopt_noscripts			= swbisoption_get_opta(opta, SW_E_swbis_ignore_scripts);
	wopt_tar_format			= swbisoption_get_opta(opta, SW_E_swbis_format);

	if (swextopt_get_status()) {
		sw_e_msg(G, "bad value detected for extended option\n");
		LCEXIT(1);
	}
	if (wopt_show_options) { 
		/* Show the actual options */
		swextopt_writeExtendedOptions(STDOUT_FILENO, opta, SWC_U_I);
		if (G->g_verboseG > 4) {
			debug_writeBooleanExtendedOptions(STDOUT_FILENO, opta);
		}
		LCEXIT(0);
	}

	swc_set_shell_dash_s_command(G, wopt_shell_command);
	
	if (swc_is_option_true(wopt_show_progress_bar)) wopt_quiet_progress_bar = (char*)NULL;

	if (*remote_shell_command == '/') {
		remote_shell_path = remote_shell_command;
	} else {
		remote_shell_path = shcmd_find_in_path(getenv("PATH"), remote_shell_command);
	}

	use_no_getconf = swc_is_option_true(wopt_no_getconf);	

	/*
	 * Respect the swbis_allow_rpm exteneded option in swbisdefaults
	 */	
	if (!swextopt_is_option_true(SW_E_swbis_allow_rpm, opta))
	{
		E_DEBUG("turning off rpm detection");
		uinfile_decode_flags &= (~UINFILE_DETECT_UNRPM); /* turn off RPM decoding */
	}

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 * set up the options for the '--to-stdout' option
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */
	E_DEBUG2("opt_preview=%d", opt_preview);
	E_DEBUG2("do_to_stdout=%d", do_to_stdout);

	if (do_to_stdout && opt_preview == 0) {
		opt_preview = 1;
		G->g_opt_previewM = 1;
		G->g_to_stdout = 1;
		G->g_verboseG = 1;
		G->g_stdout_testfd = STDOUT_FILENO;
		wopt_allow_rpm = CHARTRUE;
		set_opta(opta, SW_E_swbis_allow_rpm, "true");
		/* UINFILE_DETECT_UNRPM on by default in this mode */
		uinfile_decode_flags |= (UINFILE_DETECT_UNRPM); /* turn on RPM decoding */
	} else if (do_to_stdout && opt_preview) {
		/* this is not allowed */
		sw_e_msg(G, "stdout and preview modes are mutually exclusive\n");
		LCEXIT(1);
	}

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *   Configure the standard I/O usages.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

	fver = G->g_vstderr;
	fverfd = STDERR_FILENO;
	
	if (G->g_verboseG == 0 && opt_preview == 0) {
		fver = fopen("/dev/null", "r+");
		if (!fver) LCEXIT(1);
		G->g_vstderr = fver;
		fverfd = nullfd;
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
	}

	swlib_set_verbose_level(G->g_verboseG);
	
	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *   Set up the logger spec
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */
	swc_initialize_logspec(&G->g_logspec, eopt_logfile, G->g_loglevel);
	
	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *   Set the terminal setting and ssh tty option.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */
	
	tty_opt = "-T";
	login_orig_termiosP = NULL;
	login_sizeP = NULL;
	
	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *        Set the signal handlers.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

	swgp_signal(SIGINT, main_sig_handler);
	swgp_signal(SIGPIPE, main_sig_handler);
	swgp_signal(SIGTERM, main_sig_handler);

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *           Process the source spec.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

	ret = swextopt_combine_directory(tmp,
			soc_spec_source,
			eopt_distribution_source_directory);
	if (ret) {
		sw_e_msg(G, "software source specification ambiguous or not valid: %s\n", soc_spec_source);
		LCEXIT(1);
	}

	if (soc_spec_source) {
		free(soc_spec_source);
		soc_spec_source = strdup(strob_str(tmp));
	}

	if (opt_preview) swssh_deactivate_sanity_check();

	if (swc_parse_soc_spec(soc_spec_source, &cl_source_selections, &xcl_source_target) != 0) {
		sw_e_msg(G, "error parsing source target spec: %s\n", soc_spec_source);
		LCEXIT(1);
	}

	if (cl_source_selections) {
		sw_e_msg(G, "software selection feature for the source is ambiguous: %s\n", soc_spec_source);
		LCEXIT(1);
	}

	if (xcl_source_target == NULL) {
		cl_source_target = strdup(eopt_distribution_source_directory);
	} else {
		cl_source_target = strdup(xcl_source_target);
	}
			
	sw_d_msg(G, "source spec: %s\n", cl_source_target);

	if (G->g_source_terminal_host) {
		free(G->g_source_terminal_host);
		G->g_source_terminal_host = NULL;
	}
	source_nhops = swssh_parse_target(source_sshcmd[0],
				source_kill_sshcmd[0],
				cl_source_target,
				remote_shell_path,
				remote_shell_command,
				&tmpcharp,
				&(G->g_source_terminal_host),
				tty_opt, wopt_with_hop_pids, wopt_ssh_options,
				swc_is_option_true(wopt_forward_agent));
	SWLIB_ASSERT(source_nhops >= 0);
	if (G->g_source_terminal_host == NULL) {
		G->g_source_terminal_host = strdup("localhost");
	}

	if (tmpcharp == NULL) {
		sw_e_msg(G, "invalid args\n");
		LCEXIT(1);
	}
	source_path = swc_validate_targetpath(
				source_nhops, 
				tmpcharp, 
				eopt_distribution_source_directory, cwd, "source");
	SWLIB_ASSERT(source_path != NULL);

	if (source_nhops >= 1 && strcmp(source_path, "-") == 0) {
			sw_e_msg(G, "remote stdin is not supported\n");
		LCEXIT(1);
	}

	if (strcmp(source_path, "-") == 0) { 
		if (stdin_in_use) {
			sw_e_msg(G, "invalid usage of stdin\n");
			LCEXIT(1);
		}
		local_stdin = 1; 
	}

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *      Set the Serial archive write command.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

	if (pax_write_command_key == (char*)NULL) {
		if (source_nhops < 1) {
			pax_write_command_key = wopt_local_pax_write_command;
		} else {
			pax_write_command_key = wopt_remote_pax_write_command;
		}		
	}

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *          Process the Software Selections
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

	swspecs = vplob_open(); /* List object containing list of SWVERID objects */

	ret = swc_process_selection_files(G, swspecs);

	if (ret == 0)
		if (argv[optind]) {
			if (*(argv[optind]) != '@') {
				/*
				* Must be a software selection.
				*/
				ret = swc_process_selection_args(swspecs, argv, argc, &optind);
			}
		}

	if (ret) {
		/*
		 * Software selection error
		 */
		sw_e_msg(G, "error processing selections\n");
		LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
	}

	if (vplob_get_nstore(swspecs) > 0)
	{
		/*
		 * Software selections not supported yet, except for
		 * a location spec.
		 */
		if (vplob_get_nstore(swspecs) > 1) {
			sw_e_msg(G, "more than one software selection not implemented\n");
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}

		if (is_supported_swspecs(swspecs) == 0) {
			sw_e_msg(G, "this particular software selection not implemented: %s\n",
				swverid_print((SWVERID*)(vplob_val(swspecs, 0)), tmp));
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		} else {
			/* It specifies a location, such as "l=/" */
		}
	}

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *           Loop over the targets.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

   	current_arg = swc_get_next_target(argv, argc, &optind, 
						G->g_targetfd_array, 
					opt_distribution_target_directory,
						&num_remains);
	while (current_arg && G->g_master_alarm == 0) {
		distdataO = swi_distdata_create();
		swgp_signal(SIGINT, safe_sig_handler);
		swgp_signal(SIGPIPE, safe_sig_handler);
		swgp_signal(SIGTERM, safe_sig_handler);
		statbytes = 0;
		s_retval = 0;
		t_retval = 0;
		f_retval = 0;
		G->g_target_did_abortM = 0;
		target_tramp_list = strar_open();
		source_tramp_list = strar_open();
		upgrade_catalog_paths = strar_open();
		target_sshcmd[0] = shcmd_open();
		target_cmdlist = strar_open();
		source_cmdlist = strar_open();
		source_control_message = strob_open(10);
		target_control_message = strob_open(10);
		target_start_message = strob_open(10);
		target_catalog_message = strob_open(10);
		target_catalog_path = strob_open(10);
		source_start_message = strob_open(10);
		source_access_message = strob_open(10);
		cl_target_target = NULL;
		cl_target_selections = NULL;
		working_arg = strdup(current_arg);

		/*  Target Spec --- 
		 *   Parse the target sofware_selections and targets.
		 */

		soc_spec_target = strdup(working_arg);
		if (target_loop_count == 0) {
			if (swc_parse_soc_spec(soc_spec_target,
						&cl_target_selections,
						&xcl_target_target) != 0) {
				sw_e_msg(G, "error parsing target spec: %s\n", soc_spec_source);
				LCEXIT(1);
			}

			if (xcl_target_target == NULL) {
				cl_target_target = strdup(opt_distribution_target_directory);
			} else {
				cl_target_target = strdup(xcl_target_target);
			}

			/*
			 * Selections are not supported here.  They are applied
			 * globally and processed before entering the target processing
			 * loop.
			 */
			if (cl_target_selections) {
				sw_e_msg(G, "software selection not valid when specified with a specific target.\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
		} else {
			/*
			 * subsequext args are targets, the same
			 * software selection applies.
			 */
			if (do_to_stdout) {
				sw_e_msg(G, "invalid use\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));

			}
			cl_target_target = strdup(soc_spec_target);
		}

		new_cl_target_target = swc_convert_multi_host_target_syntax(G, cl_target_target);
		free(cl_target_target);
		cl_target_target = new_cl_target_target;
		G->g_cl_target_targetM = strdup(cl_target_target);

		if (strcmp(cl_target_target, "-") == 0 && opt_preview == 0) {
			/*
			 * Here the user gave the target as "@-"
			 * this should be the same as "--to-stdout"
			 */
			do_to_stdout = 1;
			cl_target_target = strdup("/");
			opt_preview = 1;
			G->g_opt_previewM = 1;
			G->g_to_stdout = 1;
			G->g_verboseG = 1;
			G->g_stdout_testfd = STDOUT_FILENO;
			wopt_allow_rpm = CHARTRUE;
			set_opta(opta, SW_E_swbis_allow_rpm, "true");
			/* UINFILE_DETECT_UNRPM on by default in this mode */
			uinfile_decode_flags |= (UINFILE_DETECT_UNRPM); /* turn on RPM decoding */
		} else if (strcmp(cl_target_target, "-") == 0 && opt_preview == 1) {
			sw_e_msg(G, "stdout and preview modes are mutually exclusive\n");
			LCEXIT(1);
			/*
			if (cl_target_target) free(cl_target_target);
			cl_target_target = strdup("/");
			*/
		}

		E_DEBUG("");

		if (G->g_target_terminal_host) {
			free(G->g_target_terminal_host);
			G->g_target_terminal_host = NULL;
		}
		target_nhops = swssh_parse_target(target_sshcmd[0],
				target_kill_sshcmd[0],
				cl_target_target,
				remote_shell_path,
				remote_shell_command,
				&tmpcharp,
				&(G->g_target_terminal_host),
				tty_opt, wopt_with_hop_pids,
				wopt_ssh_options, 1 /* do set -A */);
		SWLIB_ASSERT(target_nhops >= 0);
		if (G->g_target_terminal_host == NULL) {
			G->g_target_terminal_host = strdup("localhost");
		}

		E_DEBUG("");
		if (target_nhops >= 1 && wopt_pump_delay1 < 0) {
 			*swlib_get_io_req_p() = &G->g_io_req;
			G->g_io_req.tv_sec = 0;
			G->g_io_req.tv_nsec = DEFAULT_PUMP_DELAY;
		}
	
		E_DEBUG("");
		if (xcl_target_target == NULL && target_nhops >= 1 && 
					strcmp(cl_target_target, "-") == 0) {
			/*
			 * writing to stdout on a remote host is not supported.
			 * Reset the the default target
			 */
			cl_target_target = strdup(".");
		}

		E_DEBUG("");
		target_path = swc_validate_targetpath(
					target_nhops, 
					tmpcharp, 
					opt_distribution_target_directory, cwd, "target");
		SWLIB_ASSERT(target_path != NULL);

		E_DEBUG("");
		if (strcmp(target_path, "-") == 0 && target_nhops >= 1) {
			/*
			 * This is a useless mode, all it does is echo
			 * keystrokes through the ssh pipeline, don't allow
			 * it.
			 */
			sw_e_msg(G, "invalid target spec\n");
			LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
		}

		G->g_swevent_fd = swevent_fd;

		/*
		 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
		 *   Establish the logger process and stderr redirection.
		 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		E_DEBUG("");
		if (target_loop_count == 0) {
			E_DEBUG("");
			G->g_logger_pid = swc_fork_logger(G, source_line_buf,
				target_line_buf, swc_get_stderr_fd(G),
				swevent_fd, &G->g_logspec, &G->g_s_efd, &G->g_t_efd,
				G->g_verboseG,  &swi_event_fd);
		}
		E_DEBUG("");

		G->g_swi_event_fd = swi_event_fd;
		target_stdio_fdar[2] = G->g_t_efd;
		source_stdio_fdar[2] = G->g_s_efd;
		sw_d_msg(G, "logger_pid is %d\n", G->g_logger_pid);

		sw_l_msg(G, SWC_VERBOSE_3, "SWBIS_TARGET_BEGINS for %s\n", current_arg);
	
		E_DEBUG("");
		if (target_nhops && source_nhops) {
			/*
			 * disallow remote source *and* target if either is
			 * a double hop.  But allow it if both are single
			 * hops.
			 */
			if (target_nhops + source_nhops >= 3) {
				sw_e_msg(G, "simultaneous remote source and target is not supported.\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
		}	

		/*
		 * pseudo tty is never needed for swinstall
		 */
		source_fork_type = G->g_fork_pty_none;
		target_fork_type = G->g_fork_pty_none;

		if (pax_read_command_key == (char*)NULL) {
			if (target_nhops < 1) {
				pax_read_command_key = 
					wopt_local_pax_read_command;
			} else {
				pax_read_command_key = 
					wopt_remote_pax_read_command;
			}		
		}

		/*
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 *   Do the real install.
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		sw_d_msg(G, "target_fork_type : %s\n", target_fork_type);
		sw_d_msg(G, "source_fork_type : %s\n", source_fork_type);
		
		E_DEBUG("");
		if (swi) {
			if (swi->xformatM)
				xformat_close(swi->xformatM);
			G->g_swi = NULL;
			G->g_xformat = NULL;
			swi_delete(swi);
		}
		swi = swi_create();
		G->g_swi = swi;

		/*
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 * Make the source piping.
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		E_DEBUG("");
		if (local_stdin == 0) {
			/*
			 * Remote Source
			 */

			/* set the safe2 signal handler here if
			   source_nhops > 0 */
			if (source_nhops > 0) {
				swgp_signal(SIGINT, safe_sig_handler);
				swgp_signal(SIGPIPE, safe_sig_handler);
				swgp_signal(SIGTERM, safe_sig_handler);
			}

			E_DEBUG("");
			ss_pid = swc_run_ssh_command(G,
				source_sshcmd,
				source_cmdlist,
				source_path,
				opt_preview,
				source_nhops,
				source_fdar,
				source_stdio_fdar,
				login_orig_termiosP,
				login_sizeP, 
				&source_pump_pid, 
				source_fork_type, 
				make_master_raw, 
				(sigset_t*)ssh_fork_blockmask, 
				devel_no_fork_optimization,
				G->g_verboseG,
				&source_file_size,
				use_no_getconf,
				&is_source_local_regular_file,
				wopt_shell_command,
				swc_get_stderr_fd(G),
				&G->g_logspec);
			E_DEBUG("");
			swc_check_for_current_signals(G, __LINE__,  wopt_no_remote_kill);
			/* fprintf(stderr,"XABC200511  HERE %d\n", __LINE__); */

			if (is_source_local_regular_file) {
				E_DEBUG("");
				if (lseek(source_fdar[0],
						(off_t)0, SEEK_SET) != 0) {
					sw_e_msg(G, "lseek internal error on stdio_fdar[0]\n");
					f_retval = 1;
					goto TARGET1;
				}
			} else {
				/* fprintf(stderr,"XABC200511  HERE %d %d\n", __LINE__, num_remains); */
				E_DEBUG("");
				if (num_remains > 1) {
					/*
					* Can't have multiple targets unless
					* the source file is seekable.
					*/
					sw_e_msg(G, "multiple targets require the source to be a regular file.\n");
					f_retval = 1;
					goto TARGET1;
				}
			}

			E_DEBUG("");
			if (ss_pid < 0) {
				/*
				 * Fatal
				 */		
				LCEXIT(sw_exitval(G,
					target_loop_count, 
					target_success_count));
			}

			E_DEBUG("");
			if (ss_pid > 0) { 
				swc_record_pid(ss_pid, 
					G->g_pid_array, 
					&G->g_pid_array_len, 
					G->g_verboseG);
			}

			E_DEBUG("");
			if (source_pump_pid > 0) {
				/*
				 * This is the child of slave 
				 * pty process, zero if there 
				 * isn't one.
				 */
				swc_record_pid(source_pump_pid, 
					G->g_pid_array, 
					&G->g_pid_array_len, 
					G->g_verboseG);
			}
			
			if (isatty(source_fdar[0])) {
				if (swlib_tty_raw(source_fdar[0]) < 0) {
					sw_e_msg(G, "tty_raw error : source_fdar[0]\n");
					LCEXIT(
					sw_exitval(G, target_loop_count, 
						target_success_count));
				}
			}
		} else {
			/*
			 * here is the source piping for local stdin.
			 */ 
			E_DEBUG("");
			swgp_signal(SIGINT, main_sig_handler);
			swgp_signal(SIGPIPE, main_sig_handler);
			swgp_signal(SIGTERM, main_sig_handler);
			ss_pid = 0;
			source_fdar[0] = source_stdio_fdar[0];
			source_fdar[1] = source_stdio_fdar[1];
			source_fdar[2] = source_stdio_fdar[2];

			E_DEBUG("");
			if (target_loop_count > 0) {
				E_DEBUG("");
				dup2(save_stdio_fdar[0], source_fdar[0]);
				is_local_stdin_seekable = 
					(lseek(source_fdar[0],
						(off_t)0,
						SEEK_CUR) >= 0);
				if (is_local_stdin_seekable) {
					if (lseek(source_fdar[0],
							0, SEEK_SET) != 0) {
						sw_e_msg(G, "lseek internal error on source_fdar[0]\n");
						f_retval = 1;
						goto TARGET1;
					} else {
						;
					}
				} else {
					/*
					 * can't have multiple targets when the
					 * source is not not seekable
					 */
					sw_e_msg(G, "source is not seekable, multiple targets not allowed\n");
					f_retval = 1;
					goto TARGET1;
				}
			}
		}

		if (is_local_stdin_seekable  || is_source_local_regular_file) {
			is_seekable = 1;
		} else {
			is_seekable = 0;
		}

		/*
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 * Write the source script.
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		E_DEBUG("");
		source_write_pid = swc_run_source_script(G,
					swutil_x_mode, 
				local_stdin || is_source_local_regular_file,
					source_fdar[1],
					source_path,
					source_nhops, 
					pax_write_command_key,
					fork_blockmask,
					fork_defaultmask,
					G->g_source_terminal_host,
					wopt_blocksize,
					SWC_VERBOSE_4,
					swc_write_source_copy_script);
		swc_check_for_current_signals(G, __LINE__,  wopt_no_remote_kill);

		if (source_write_pid > 0) {
			sw_d_msg(G, "waiting on source script pid\n");
			/*
		 	 * ==========================
			 * Wait on the source script.
			 * ==========================
			 */
			if ((ret=swc_wait_on_script(G, source_write_pid, 
						"source")) != 0) {
				/*
			 	 * ====================================
				 * non zero is error for source script.
				 * ====================================
				 */
				sw_e_msg(G, "write_scripts waitpid : exit value = %d\n", ret );
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				LCEXIT(
					sw_exitval(G, target_loop_count,	
						target_success_count));
			}
				
			sw_d_msg(G, "wait() on source script succeeded.\n");

			/*
			 * ===================================
			 * Read the start message and the pid
			 * ===================================
			 */
			if (swc_read_start_ctl_message(G,
				source_fdar[0], 
				source_start_message,
				source_tramp_list,
				G->g_verboseG,
				&source_script_pid, "source") 
				< 0
			) 
			{
				/*
				 * start message failed.
				 */
				LC_RAISE(SIGTERM);
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				sw_e_msg(G, "read_start_ctl_message error"
					" (loc=start)\n");
				LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
			}
	
			if (source_nhops >= wopt_kill_hop_threshhold) {
				/*
				 * Construct the remote kill vector.
				 */
				G->g_killcmd = NULL;
				sw_d_msg(G, "Running swc_construct_newkill_vector\n");
				if ((ret=swc_construct_newkill_vector(
					source_kill_sshcmd[0], 
					source_nhops,
					source_tramp_list, 
					source_script_pid, 
					G->g_verboseG)) < 0) 
				{
					source_kill_sshcmd[0] = NULL;
					sw_e_msg(G, 
			"source kill command not constructed (ret=%d), maybe a shell does not have PPID.\n", ret);
				}
				G->g_source_kmd = source_kill_sshcmd[0];
			}
			swc_check_for_current_signals(G, __LINE__,  wopt_no_remote_kill);

			/*
			 * ===================================
			 * Read the leading control message from the 
			 * output of the source script.
			 * This is how the target script knows to
			 * create a file or directory archive.
			 * ===================================
			 */
			if ( SWINSTALL_DO_SOURCE_CTL_MESSAGE ) {
				/*
				* Read the control message 
				* from the remote source.
				*/
				sw_d_msg(G, "reading source script control messages\n");

				if (swc_read_target_ctl_message(G,
					source_fdar[0], 
					source_control_message,
					G->g_verboseG, "source") < 0) 
				{
					sw_d_msg(G, "read_target_ctl_message error"
						" (loc=source_start)\n");
					main_sig_handler(SIGTERM);
					LCEXIT(sw_exitval(G, target_loop_count,
						target_success_count));
				}
				sw_d_msg(G, "Got source control message [%s]\n",
					strob_str(source_control_message));
			} else {
				sw_d_msg(G, "No source control message expected\n");
			}

			/*
			 * ===================================
			 * Read the SW_SOURCE_ACCESS_BEGIN or
			 * the SW_SOURCE_ACCESS_ERROR message
			 * ===================================
			 */	

			sw_d_msg(G, "reading source script access messages\n");
			if (swc_read_target_ctl_message(G,
				source_fdar[0], 
				source_access_message,
				G->g_verboseG, "source") < 0
			) 
			{
				sw_d_msg(G, "read_target_ctl_message error"
					" (loc=source_access)\n");
				main_sig_handler(SIGTERM);
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G,
					target_loop_count,
					target_success_count));
			}
			
			/*
			 * =================================
			 * Analyze the source event.
			 * =================================
			 */	
			if ((ret=swevent_get_value(
					swevent_get_events_array(),
					strob_str(source_access_message))) != 
					SW_SOURCE_ACCESS_BEGINS) 
			{
				/*
				* Source access error.
				*/
				sw_e_msg(G, "source access error: ret=%d :%s\n", ret, strob_str(source_access_message));
				main_sig_handler(SIGTERM);
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G,
					target_loop_count,
					target_success_count));
			}
		} else if (source_write_pid == 0) {
			/* 
			 * ====================================
			 * fork did not happen.
			 * This is Normal, source script not required. 
			 * ====================================
			 */
			sw_d_msg(G, "No fork required for source control script\n");
		} else {
			/*
			 * error
			 */
			sw_e_msg(G, "fatal internal error. fork error.\n");
			main_sig_handler(SIGTERM);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, target_loop_count, 
					target_success_count));
		}
		
		if (opt_preview && G->g_verboseG >= SWC_VERBOSE_8) {
			do_preview(G,
				fver, fverfd, 
				working_arg, source_control_message,
				local_stdin, 
				source_nhops, source_path,
				cl_source_target, source_fork_type,
				target_nhops, target_path,
				cl_target_target, target_fork_type,
				pax_read_command_key, target_sshcmd[0],
				target_kill_sshcmd[0], source_sshcmd[0],
				source_kill_sshcmd[0], SWC_SCRIPT_SLEEP_DELAY,
				wopt_keep_old_files, wopt_blocksize,
				use_no_getconf, pax_write_command_key,
				target_loop_count, swutil_x_mode,
				wopt_shell_command, target_cmdlist,
				source_cmdlist, distdataO,
				eopt_installed_software_catalog,
				G->g_sh_dash_s, G->g_opt_alt_catalog_root);
		}

		/*
		 * ===================================
		 * Decode the package catalog section
		 * ===================================
		 */

		/*
		 * Reset the source file if not the first time
		 */
		E_DEBUG("");
		if (target_loop_count > 0) {
			E_DEBUG("");
			if (uxfio_lseek(source_fdar[0], 0, SEEK_SET) != 0) {
				fprintf(stderr, "%s: uxfio_lseek error: %s : %d\n",
					swlib_utilname_get(),  __FILE__, __LINE__);
				exit(2);
			}
		}

		E_DEBUG("into do_decode");
		swi_set_utility_id(swi, SWC_U_I);
		SWLIB_ASSERT(swi != NULL);
		ret = swi_do_decode(swi, swlog, G->g_nullfd,
			dup(source_fdar[0]),
			target_path,
			source_path,
			swspecs,
			G->g_target_terminal_host,
			opta,
			is_seekable,
			wopt_debug_events,
			G->g_verboseG,
			&G->g_logspec,
			uinfile_decode_flags);
		G->g_xformat = swi->xformatM;

		E_DEBUG("out of do_decode");
		if (ret) {
			sw_e_msg(G, "error decoding source\n");
			main_sig_handler(SIGTERM);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G,
				target_loop_count, 
				target_success_count));
		}

		taru_set_tar_header_policy(G->g_xformat->taruM, wopt_tar_format, (int*)NULL);

		swi->opt_alt_catalog_rootM = G->g_opt_alt_catalog_root;

		/* JL */
		/* record the target path, these will be reset later
		   if target_path is not absolute  */

		swi->swi_pkgM->target_pathM = strdup(target_path);
		swicol_set_targetpath(swi->swicolM, target_path);

		swi->swi_pkgM->installed_software_catalogM = strdup(
				get_opta_isc(opta, SW_E_installed_software_catalog));

		E_DEBUG2("get_opta_isc(opta, SW_E_installed_software_catalog) is [%s]", get_opta_isc(opta, SW_E_installed_software_catalog));
		if (swlib_check_clean_path(swi->swi_pkgM->target_pathM)) {
			SWLIB_FATAL("tainted path");
		}

		E_DEBUG("");
		if (swlib_check_clean_path(swi->swi_pkgM->installed_software_catalogM)) {
			SWLIB_FATAL("tainted path");
		}

		swicol_set_delaytime(swi->swicolM, SWC_SCRIPT_SLEEP_DELAY);
		swicol_set_nhops(swi->swicolM, target_nhops);
		swicol_set_verbose_level(swi->swicolM, G->g_verboseG);
		swicol_set_task_debug(swi->swicolM, G->g_do_task_shell_debug);
		
		if (G->g_swi_debug_name) {
			swc_write_swi_debug(swi, G->g_swi_debug_name);
		}

		/*
		 * ==========================================
		 * Get the product tag(s) and revision(s) and
		 * other metadata needed by the target script
		 * ==========================================
		 */
		E_DEBUG("");
		if (distdataO->did_part1M == 0) {
			f_retval = swi_distdata_resolve(swi, distdataO,
						1 /*enforce swinstall policy*/);
			if (f_retval) {
				sw_e_msg(G, "error decoding source INDEX file: swbis_code=%d\n", f_retval);
				LCEXIT(sw_exitval(G,
					target_loop_count, 
					target_success_count));
	
			}
			distdataO->did_part1M = 1;
		}

		
		/*
		 * ==========================================
		 * Check the signature of the package if 
		 * --sig-level > 0, 0 is none check, -1 is all sigs
		 * ==========================================
		 */
		E_DEBUG("");

		sigs_passed = 0;
		sig_level = swlib_atoi(wopt_sig_level, (int*)NULL);
		if ((sig_level > 0 || sig_level == -1) && sigs_passed == 0) {
			ret = swpl_check_package_signatures(G, swi, &num_sigs_checked);
			if (sig_level < 0) {
				if (ret != num_sigs_checked) {
					/* signature failed */
					sw_e_msg(G, "authentication failed at sig-level %s: %d found\n", wopt_sig_level, ret);
					LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
				}
			} else if (sig_level > 0) {
				if (ret < sig_level) {
					/* signature failed */
					sw_e_msg(G, "authentication failed at sig-level %s: %d found\n", wopt_sig_level, ret);
					LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
				}
			}
			sigs_passed = 1;
		}
		E_DEBUG("");


		/*
		 * Sign the package if already signed
		 */

		if (sigs_passed == 0 && wopt_sign) {
			sw_e_msg(G, "installer signature not allowed on unverified package\n");
			LCEXIT(sw_exitval(G,
				target_loop_count, 
				target_success_count));
		} else if (sigs_passed == 1 && wopt_sign) {
			/* Only sign a signed package that has been verified */
			installer_sig = swpl_make_package_signature(G, swi);
			if (installer_sig == NULL) {
				sw_e_msg(G, "installer signature failed\n");
				LCEXIT(sw_exitval(G,
					target_loop_count, 
					target_success_count));
			} else {
				swi->swi_pkgM->installer_sigM = installer_sig;
			}
		} else {
			installer_sig = NULL;
		}


		/*
		 * ++++++++++++++++++++++++++++++++++ 
		 * Make the target piping.
		 * ++++++++++++++++++++++++++++++++++ 
		 */
		swc_check_for_current_signals(G, __LINE__,  wopt_no_remote_kill);

		/* Set the safe signal handler here */
		if (target_nhops > 0) {
			swgp_signal(SIGINT, safe_sig_handler);
			swgp_signal(SIGPIPE, safe_sig_handler);
			swgp_signal(SIGTERM, safe_sig_handler);
		}
	
		E_DEBUG("");
		ts_pid = swc_run_ssh_command(G,
			target_sshcmd,
			target_cmdlist,
			target_path,
			opt_preview,
			target_nhops,
			target_fdar,
			target_stdio_fdar,
			login_orig_termiosP,
			login_sizeP, 
			&target_pump_pid,
			target_fork_type, 
			make_master_raw, 
			(sigset_t*)ssh_fork_blockmask,
			0 /*devel_no_fork_optimization*/,
			G->g_verboseG,
			&target_file_size,
			use_no_getconf,
			(int*)(NULL),
			wopt_shell_command,
			swc_get_stderr_fd(G),
			&G->g_logspec);

		E_DEBUG("");
		if (ts_pid < 0) {
			sw_e_msg(G, "fork error : %s\n", strerror(errno));
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, target_loop_count, 
					target_success_count));
		}
		E_DEBUG("");
		if (ts_pid)
			swc_record_pid(ts_pid, G->g_pid_array,
				&G->g_pid_array_len,
				G->g_verboseG);

		if (target_pump_pid > 0) {
			/*
			 * ================
			 * This is the child of slave pty process, 
			 * zero if there isn't one.
			 * ================
			 */
			swc_record_pid(target_pump_pid,
					G->g_pid_array,
					&G->g_pid_array_len, 
					G->g_verboseG);
		}

		E_DEBUG("");
		if (isatty(target_fdar[0])) {
			if (swlib_tty_raw(target_fdar[0]) < 0) {
				sw_e_msg(G, "tty_raw error : target_fdar[0]\n");
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G,
					target_loop_count,
					target_success_count));
			}
		}

		/*  Write the target script.  */

		E_DEBUG("");
		target_write_pid = run_target_script(G,
					target_fdar[1],
					source_path,
					target_path,
					source_control_message,
					wopt_keep_old_files, 
					target_nhops,
					pax_read_command_key,
					fork_blockmask,
					fork_defaultmask,
					G->g_target_terminal_host,
					wopt_blocksize,
					distdataO,
					eopt_installed_software_catalog,
					opt_preview,
					G->g_opt_alt_catalog_root
					);
		/* swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill); */

		if (target_write_pid <= 0) {
			/*
			 * error
			 */
			sw_e_msg(G, "fatal internal error. target script"
				" fork error target_write_pid=%d\n", target_write_pid);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}

		/* Wait on the target script */

		sw_d_msg(G, "waiting on target script pid\n");
		if ((ret=swc_wait_on_script(G, target_write_pid, "target")) != 0) {
			/* zero is error for target script  */
			sw_e_msg(G, "write_scripts waitpid : exit value"
				" = %d which is an error.\n", ret );
			LC_RAISE(SIGTERM);
			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G,
				target_loop_count, 
				target_success_count));
		}
		if (ret == 0) {
			/* Normal exit, OK */
			sw_d_msg(G, "OK: target script returned %d\n", ret);
		} else {
			/* Error */
			/* FIXME maybe need to abort here */
			sw_d_msg(G, "Error: target script returned %d\n", ret);
		}
		sw_d_msg(G, "wait() on target script succeeded.\n");

		G->g_swicolM = swi->swicolM;
		G->g_running_signalusr1M = 0;
		G->g_running_signalsetM = 0;

		/*
		 * ===============================
		 * read the start control message.
		 * ===============================
		 */
		E_DEBUG("");
		ret = swc_read_start_ctl_message(G,
			target_fdar[0], 
			target_start_message, 
			target_tramp_list,
			G->g_verboseG,
			&target_script_pid, "target");
		if (ret < 0) {
			/*
			 * ==================================
			 * Read of the start message failed.
			 * ==================================
			 */

			/*
			 * This is (may be) the failure path for ssh authentication
			 * failure.
			 */
	
			/*
			 * See if the ssh command has finished.
			 */

			if (swlib_wait_on_pid_with_timeout(ts_pid, &tmp_status, 0 /*flags*/, 1/*verbose_level*/, 3 /* Seconds */) > 0) {
				/*
				 * Store the result in the status array.
				 */
				SWLIB_ASSERT(
					swlib_update_pid_status(ts_pid, tmp_status,
						pid_array, status_array, *p_pid_array_len) == 0);
			}
			if (WIFEXITED(tmp_status)) {
				/*
				 * This is the exit status of ssh (or rsh)
				 */
				tmpret = WEXITSTATUS(tmp_status);
				swpl_agent_fail_message(G, current_arg, tmpret);
			} else {
				sw_e_msg(G, "SW_INTERNAL_ERROR for target %s in file %s at line %d\n",
					current_arg, __FILE__, __LINE__);
			}
			/*
			 * FIXME, maybe need to send the abort task to the script
			 */


			/*
			 * Continue with next target.
			 */
			goto ENTER_WITH_SSH_FAILURE;
		}
		
		/*
		 * ===============================
		 * read the pwd current directory message
		 * from the target
		 * MSG_129 
		 * ===============================
		 */

		ret = swc_read_target_message_129(G, target_fdar[0], target_current_dir, G->g_verboseG, "target pwd");
		if (ret < 0) {
			/* FIXME, should be bigger error than this */
			sw_e_msg(G, "SW_INTERNAL_ERROR for target %s in file %s at line %d: message _129 failed\n",
					current_arg, __FILE__, __LINE__);
			goto ENTER_WITH_SSH_FAILURE; /* was goto ENTER_WITH_FAILURE; */
		}

		/* Now adjust the target_path to be an absolute path
		   if it is not already */

		if (*target_path != '/') {
			E_DEBUG("MAKING path absolute");
			E_DEBUG2("before: target path now [%s]", target_path);
			target_path = swc_make_absolute_path(strob_str(target_current_dir), target_path);
			*(swi->swi_pkgM->target_pathM) = '\0';
			free(swi->swi_pkgM->target_pathM);
			swi->swi_pkgM->target_pathM = strdup(target_path);
			swicol_set_targetpath(swi->swicolM, target_path);
			E_DEBUG2("after: target path now [%s]", target_path);
			E_DEBUG2("swi->swicolM->targetpathM=[%s]", swi->swicolM->targetpathM);
		}

		/*
		 * ===============================
		 * read catalog path message,
		 * either 128 or 508
		 * ===============================
		 */

		E_DEBUG("");
		ret = swc_read_catalog_ctl_message(G,
			target_fdar[0],
			target_catalog_message, /* no longer used */
			G->g_verboseG,
			"catalog",
			eopt_installed_software_catalog);
		E_DEBUG2("ret=%d", ret);
		if (ret < 0) {
			/*
			 * ==================================
			 * error message
			 * ==================================
			 */
			E_DEBUG("");
			sw_e_msg(G, "catalog path analysis for target %s: ret=%d\n",
				current_arg,
				ret);
			/*
			 * This is the exit path for failed remote connection and
			 * no access (permission denied) to the target directory 
			 * shunt this to the loop end to 
			 * support continuation of the target list.
			 */
			goto ENTER_WITH_SSH_FAILURE; /* was goto ENTER_WITH_FAILURE; */
		}
		E_DEBUG("");

		/*
		 * ===============================
		 * Begin Analysis Phase
		 * ===============================
		 */

		/*
		 * Continue with installation.
		 * The script should now determine the catalog path.
		 */

		/*
		 * Get the uname attributes from the target host.
		 */
		vofd = target_fdar[1];
		E_DEBUG("");
		if (swpl_get_utsname_attributes(G, swi->swicolM, 
				swi->target_version_idM->swutsM,
				vofd, G->g_swi_event_fd)) {
			E_DEBUG("");
			SWLIB_ASSERT(0);
		}

		E_DEBUG("");  /* TS_get_catalog_perms */
		if (
			swpl_get_catalog_perms(G, swi, vofd, G->g_swi_event_fd) &&
			opt_preview == 0 &&
			1
		) {
			E_DEBUG("");
			SWLIB_ASSERT(0);
		}

		/* Combine the version Ids from the command line with the product objects
		   version id */

		E_DEBUG("");
		if (vplob_get_nstore(swspecs) > 0) {
			/* This is where the location and qualifier attributes from the command
			   line are added to the product object */

			E_DEBUG("");
			combine_version_id(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM, vplob_val(swspecs, 0));
			swverid_print(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM, tmp);
		}

		/* FIXME, need to evaluate not disallow wildcard in the command line selections */

		/* Make sure that there are no wildcards in the version object for the product */

		E_DEBUG("");
		swverid_print(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM, tmp);
		if (swverid_delete_non_fully_qualified_verids(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM)) {
			E_DEBUG("");
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1,
				&G->g_logspec,
				swc_get_stderr_fd(G),
				"Warning: version id has wildcards: %s\n",
				strob_str(tmp));
		}

		/*
		 * ------------------------------
		 * Check for relocate'ability 
		 * ------------------------------
		 */

		E_DEBUG("");
		if (is_target_a_relocation(swi, target_path)) {
			E_DEBUG("");
			ret = l_enforce_locatable(G, swi, target_path, 0 /* product_number */, opta);
			if (ret) {
				if (!opt_preview) {
					E_DEBUG("abort");
					abort_script(swi->swicolM, G, target_fdar[1]);
					f_retval++;
				}
			}
		}

		E_DEBUG("");

		/* 
		 * --------------------------------------------------------------
		 * Now Determine if we are upgrading, downdate'ing or reinstalling
		 * --------------------------------------------------------------
		 */

		/* ''samepackagespecs'' are a list of swspecs that test the installed
		   catalog for a reinstall, downdate and upgrade */

		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		samepackagespecs = get_samepackage_specs(G, swi);
		SWLIB_ASSERT(samepackagespecs != NULL);
		
		if (G->devel_verboseM) {
			show_all_sw_selections(samepackagespecs);
		}
		
		E_DEBUG("");
		dep_specs = get_dependency_specs(G, swi, &type_array);
		SWLIB_ASSERT(dep_specs != NULL);

		/* Now combine the dep_specs and the samepackage specs,
		   this will get the dependency information and the downdate,
		   reinstall information at the same time. */

		E_DEBUG("");
		copy_vplob_items(samepackagespecs, dep_specs);
		vplob_shallow_close(dep_specs);

		E_DEBUG("");
		if (0 && G->devel_verboseM) {
			i = 0;
			while(strar_get(type_array, i)) {
				fprintf(stderr, "type=%s: %s\n",
					strar_get(type_array, i),
					swverid_print(vplob_val(samepackagespecs, i + SL_FIRST_DEP_INDEX), tmp));
				i++;
			}
		}


		/* Now get the catalog entries that match these specs.
		   The first three are the reinstall, downdate detection
		   specs, the remaining tar the prerequistes, corequistites
		   and exrequisists which are determined according to the
		   (STRAR*)type_array object.  */

		E_DEBUG("");
		E_DEBUG2("f_retval=%d", f_retval);
		if (
			f_retval == 0 &&
			swicol_get_master_alarm_status(swi->swicolM) == 0 &&
			1
		) {
			E_DEBUG("running get_samepackage_query_response");
			if (G->g_to_stdout) {
				response_img = dummy_catalog_response();
				dummy_catalog = response_img;
			} else {
				/* TS_Get_iscs_listing */
				response_img = swpl_get_samepackage_query_response(G, swi->swicolM, target_path,
					target_fdar[0], target_fdar[1],
					opta, samepackagespecs,
					&f_retval,
					0 /*Not used*/);
			}
			E_DEBUG2("f_retval=%d", f_retval);
			E_DEBUG("");
			if (response_img == NULL && opt_preview) {
				E_DEBUG("no catalog found");
				fprintf(stderr, "%s: warning: preview mode continuing with no catalog\n", swlib_utilname_get());
				response_img = dummy_catalog_response();
				dummy_catalog = response_img;
			}

			if (response_img == NULL) {
				/* No catalog */
				if (!opt_preview) {
					f_retval++;
					E_DEBUG("");
					abort_script(swi->swicolM, G, target_fdar[1]);
					swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
					goto ENTER_WITH_FAILURE;
				}
			}
			E_DEBUG("");
			E_DEBUG2("response_img = %p", response_img);
			/* SWLIB_ASSERT(response_img != NULL); */
		} else {
			E_DEBUG2("swicol_get_master_alarm_status(swi->swicolM) = [%d]", swicol_get_master_alarm_status(swi->swicolM));
			E_DEBUG2("f_retval=%d", f_retval);
			E_DEBUG("");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				goto ENTER_WITH_FAILURE;
			}
		}

		if (G->devel_verboseM) 	
			fprintf(stderr, "%s\n", response_img);

		if (G->g_to_stdout == 0)
			if (f_retval == 0) {
				ret = swpl_send_success(swi->swicolM, target_fdar[1], G->g_swi_event_fd,
					SWBIS_TS_Do_nothing ": guarding 123");
			}

		/* Now parse the response */
		E_DEBUG2("f_retval=%d", f_retval);
		req = swpl_analyze_samepackage_query_response(G, response_img, &sl);
		SWLIB_ASSERT(req != NULL);
		SWLIB_ASSERT(sl != NULL);

		/* Now interpret the data structures representing the parsed
		   response */

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG("");
		/* Test for a downdate */
		ret = samepackage_downdate_test(G, swi, sl, eopt_installed_software_catalog,
						eopt_allow_downdate, soc_spec_target);
		if (ret) {
			E_DEBUG("abort");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				/* f_retval++; */ /* This causes a hanging bash */
				goto ENTER_WITH_FAILURE;
			}
		}
		E_DEBUG("");

		/*
		 * ----------------------------
		 * Test for a reinstall
		 * ----------------------------
		 */

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		ret = samepackage_reinstall_test(G, swi, sl, eopt_installed_software_catalog,
						eopt_reinstall, soc_spec_target);
		same_revision_skipped = 0;
		E_DEBUG("");
		if (ret) {
			E_DEBUG("abort");
			same_revision_skipped = 1;
			/* f_retval++; */
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				/* goto ENTER_WITH_FAILURE; */
			}
		}

		/*
		 * --------------------------------
		 * Test for multiple versions
		 * --------------------------------
		 */

		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG("");
		ret = samepackage_multiple_version_test(G, swi, sl,
					eopt_installed_software_catalog,
					wopt_force,
					soc_spec_target);
		if (ret) {
			E_DEBUG("abort");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				goto ENTER_WITH_FAILURE;
			}
		}

		/*
		 * Determine which existing package the incoming package replaces
		 */
		E_DEBUG("");
		upgrade_specs = s_determine_upgrade_selection(G, swi, sl,
					eopt_installed_software_catalog,
					wopt_force,
					soc_spec_target, upgrade_catalog_paths);
	
		/* FIXME, right now only one object is returned even if
		   there are multiple installed, multiple instances that
		   qualify as upgradable is an error */

		/* upgrade_specs[0] is a (SWVERID*) object 
		   upgrade_specs[1] is NULL */


		/* 
		 * ------------------------------------------
		 * Determine the catalog instance that is 
		 * being replaced
		 * ------------------------------------------
		 */

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		up_e = NULL;
		if (upgrade_specs == NULL) {
			/* internal error */
			E_DEBUG("abort");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				goto ENTER_WITH_FAILURE;
			}
		} else if (
			vplob_get_nstore(upgrade_specs) > 0 &&
			G->g_to_stdout == 0 &&
			1
		) {
			E_DEBUG("Get catalog.tar for the upgrade spec");
			if (G->devel_verboseM) {
				fprintf(stderr, "Upgrading/Replacing %s at ",
					swverid_print(vplob_val(upgrade_specs, 0), tmp));
				fprintf(stderr, "%s\n", strar_get(upgrade_catalog_paths, 0));
			}
			/* TS_Get_iscs_entry task */
			E_DEBUG("running swpl_get_catalog_tar for  TS_Get_iscs_entry task");
			ret = swpl_get_catalog_tar(G, swi, target_path, upgrade_specs, target_fdar[0], target_fdar[1]);
			E_DEBUG("");
			if (ret < 0) {
				E_DEBUG("abort");
				if (!opt_preview) {
					abort_script(swi->swicolM, G, target_fdar[1]);
					swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
					goto ENTER_WITH_FAILURE;
				}
			}

			/*   'ret' is a uxfio file descriptor containing the catalog entry
			      tarball blobs of the upgrade_specs,  there may be more than one
			      entry represented in the tarball */

			E_DEBUG("");
			cfd = ret;	

			uxfio_lseek(cfd, (off_t)0, SEEK_SET);
			E_DEBUG("");
			upgrade_catalog_entries = swicat_e_open_catalog_tarball(cfd);	
			E_DEBUG("");
			if (upgrade_catalog_entries == NULL) {
				E_DEBUG("abort");
				if (!opt_preview) {
					abort_script(swi->swicolM, G, target_fdar[1]);
					swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
					goto ENTER_WITH_FAILURE;
				}
			}

			/* upgrade_catalog_entries are a list of (SWICAT_E*) objects,
			   these can now be interogated for their file lists, signatures, etc. */

			E_DEBUG("");

			/* up_e is the upgraded entry that will be moved out the way */

			up_e = vplob_val(upgrade_catalog_entries, 0 /* the first one */);

			if (0) { /* Here is some test code ---------------------- */
				e_swi = swicat_e_open_swi(up_e);
				if (e_swi == NULL) {
					E_DEBUG("abort");
					abort_script(swi->swicolM, G, target_fdar[1]);
					swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
					goto ENTER_WITH_FAILURE;
				}
	
				epath = swicat_e_form_catalog_path(up_e, tmp,
						get_opta_isc(G->optaM, SW_E_installed_software_catalog),
						SWICAT_ACTIVE_ENTRY);
				SWLIB_INFO2("catalog entry (active) path = %s", epath);
				epath = swicat_e_form_catalog_path(up_e, tmp,
						get_opta_isc(G->optaM, SW_E_installed_software_catalog),
						SWICAT_DEACTIVE_ENTRY);
				SWLIB_INFO2("catalog entry (active) path = %s", epath);
				swi_delete(e_swi);
				/* e_file_list = swicat_e_make_file_list(up_e, swi->target_version_idM->swutsM, swi); */
			} /*  End of test code ------------- */


			if (G->devel_verboseM)
				swc_write_swi_debug(e_swi, "2");

			E_DEBUG("");
		} else if ( G->g_to_stdout == 0 ) {
			/* SWBIS_TS_Get_iscs_entry */
			ret = swpl_send_nothing_and_wait(swi->swicolM, target_fdar[1],
					G->g_swi_event_fd,
					SWBIS_TS_Get_iscs_entry,
					SWICOL_TL_8, SW_SUCCESS);
		} else {
			;
		}

		/* 
		 * ------------------------------------------
		 * Here, check for overwrites
		 * ------------------------------------------
		 */
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG("");
		if (G->g_to_stdout == 0) {
			/* TS_check_OVERWRITE */

			if (0) {
				ret = swpl_send_nothing_and_wait(swi->swicolM, target_fdar[1],
					G->g_swi_event_fd, SWBIS_TS_check_OVERWRITE,
					SWICOL_TL_8, SW_SUCCESS);
			} else {
			/* get the location spec */
			location = (char*)NULL;
			if ((swverid=vplob_val(swspecs, 0)) != NULL) {
				location = swverid_get_verid_value(swverid, SWVERID_VERIDS_LOCATION, 1);
			}
			if (location == (char*)NULL || strlen(location) == 0) {
				location = "/";
			}

			fl = swi_make_fileset_file_list(swi, 0, 0, location);
			overwrite_fd = swlib_open_memfd();
			SWLIB_ASSERT(overwrite_fd >= 0);
			ret = swpl_run_check_overwrite(G, fl, swi, target_fdar[1], target_path, overwrite_fd);
			if (ret && opt_preview == 0) {
				E_DEBUG("abort");
				fprintf(stderr, "%s: error in swpl_run_check_overwrite, aborting [ret=%d]\n", swlib_utilname_get(), ret);
				/* f_retval++; */
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				goto ENTER_WITH_FAILURE;
			}
			n_file_conflicts = swinstall_analyze_overwrites(G, swi, overwrite_fd, wopt_keep_old_files, (int)(up_e != NULL));
			if (up_e) {
				/* When upgrading ignore file conflicts */
				/* FIXME */
				E_DEBUG("upgrading, ignore overwrites");
				n_file_conflicts = 0;
			}

			E_DEBUG("");
			if (n_file_conflicts > 0 &&
			    wopt_keep_old_files == 0 &&
			    swc_is_option_true(wopt_swbis_replacefiles) == 0)
			{
				/* reject installation */
				/* this initiates an abort */
				E_DEBUG("aborting");
				sw_l_msg(G, SWC_VERBOSE_1, "aborting due to existing files\n");
				sw_l_msg(G, SWC_VERBOSE_1, "Use --replacefiles or --keepfiles to override\n");
				/* f_retval++; */
				if (!opt_preview) {
					abort_script(swi->swicolM, G, target_fdar[1]);
					swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
					goto ENTER_WITH_FAILURE;
				}
			}
			}
		}

		if (G->g_to_stdout == 0)
			if (f_retval == 0) {
				ret = swpl_send_success(swi->swicolM, target_fdar[1], G->g_swi_event_fd,
					SWBIS_TS_Do_nothing ": guarding 1234");
			}

		/*
		 * --------------------------
		 * Test Dependencies
		 * --------------------------
		 */

		/* Test dependencies, FIXME, need to test the dependencies for all the
		   filesets pending for installation, for right now only packages with
		   one fileset are supported */

		E_DEBUG("");
		if ( G->g_to_stdout == 0 && dummy_catalog == NULL) {
			E_DEBUG("");
			ret = depd_prerequisite_test(G, swi, sl, type_array,
				eopt_installed_software_catalog,
				eopt_enforce_dependencies,
				soc_spec_target);
		} else ret = 0;

		if (ret) {
			E_DEBUG("abort");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				goto ENTER_WITH_FAILURE;
				/* f_retval++; */
			}
		}

		if ( G->g_to_stdout == 0 && dummy_catalog == NULL) {
			ret = depd_exrequisite_test(G, swi, sl, type_array,
				eopt_installed_software_catalog,
				eopt_enforce_dependencies,
				soc_spec_target);
		} else ret = 0;

		if (ret) {
			E_DEBUG("abort");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				goto ENTER_WITH_FAILURE;
				/* f_retval++; */
			}
		}
		strar_close(type_array);
		delete_samepackage_spec(samepackagespecs);

		/*
		 * -----------------------------------------------
		 * determine the proper instance Id in the catalog
		 * -----------------------------------------------
		 */

		ret = 0; /* default instance_id */
		if ( G->g_to_stdout == 0 && dummy_catalog == NULL) 
			ret = samepackage_determine_instance(G, swi, sl, eopt_installed_software_catalog,
				upgrade_catalog_paths, &did_find_instance);

		instance_id = 0;
		if (ret < 0) {
			E_DEBUG("");
			if (!opt_preview) {
				sw_e_msg(G, "SW_INTERNAL_ERROR for target %s in file %s at line %d\n",
				current_arg, __FILE__, __LINE__);
				abort_script(swi->swicolM, G, target_fdar[1]);
				/* f_retval++; */
				instance_id = 0;
			}
		} else {
			instance_id = ret;
		}
		E_DEBUG2("using instance_id=%d", ret);


		/* 
		 * -------------------------------
		 * Determine target catalog path
		 * -------------------------------
		 */

		ret = 0;
		strob_strcpy(target_catalog_path, SW_UNUSED_CATPATH);
		if ( G->g_to_stdout == 0 && dummy_catalog == NULL) 
			ret = swinstall_lib_determine_catalog_path(target_catalog_path, distdataO,
				0,
				eopt_installed_software_catalog,
				instance_id);
		E_DEBUG2("target_catalog_path: [%s]", strob_str(target_catalog_path));
		if (ret) {
			E_DEBUG("abort");
			if (!opt_preview) {
				abort_script(swi->swicolM, G, target_fdar[1]);
				swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
				/* f_retval++; */
			}
		}
		E_DEBUG2("target catalog path is [%s]", strob_str(target_catalog_path));

		/*
		 * ------------------------------------------
		 * Determine if the product is compatible
		 * ------------------------------------------
		 */

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		ret = swi_audit_all_compatible_products(swi, swi->target_version_idM->swutsM, (int*)NULL, (int*)NULL);
		if (G->g_to_stdout != 0 || dummy_catalog) {
			ret = 1;
		}
		if (ret > 1) {
			/* Selection is ambiguous based on uts attributes
			   FIXME, need to disambiguate using software selections */
			sw_e_msg(G, "software selection feature for the source is ambiguous: %s\n", soc_spec_source);
			abort_script(swi->swicolM, G, target_fdar[1]);
			f_retval++;
		} else if (ret == 0) {
			/* No compatible product found */
			if (swc_is_option_true(eopt_allow_incompatible) == 0) {
				if (strcmp(target_path, "/") == 0) {
					/* error, don't allow install */
					swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1,
						&G->g_logspec,
						swc_get_stderr_fd(G),
						"SW_NOT_COMPATIBLE with host: %s\n",
						cl_target_target);

					/* The next task script Id is SWBIS_TS_Load_catalog,
					   hence, send the abort script */

					E_DEBUG("sending abort");
					abort_script(swi->swicolM, G, target_fdar[1]);
					/* swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, ""); */
					/* f_retval++; This line caused a zombied bash on a one (1) ssh localhost hop*/
					E_DEBUG("");
				} else {
					/* allow incompatible installation to
					   alternate root */
					;
				}
			} else {
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1,
					&G->g_logspec,
					swevent_fd,
					"Warning: SW_NOT_COMPATIBLE with host: %s\n",
						cl_target_target);

				/* allow this, with a warning */
				;
			}
		} else if (ret == 1) {
			/* Good, compatible product found */
			;	
		} else {
			/* Never gets here */
			/* Not used at present */
			SWLIB_FATAL("depd_requisite_test failed for prerequisites");
		}

		if ( vplob_get_nstore(upgrade_specs) > 0 &&
		     G->g_to_stdout == 0 
		) {
			/*
			 * -----------
			 * Remove catalog entry
			 * ------------
			 */

			E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
			E_DEBUG2("swi->swicolM->targetpathM=[%s]", swi->swicolM->targetpathM);
			/* SWBIS_TS_remove_catalog_entry */
			if (opt_preview == 0)
				ret = swpl_remove_catalog_entry(G, swi->swicolM, up_e, target_fdar[1],
						get_opta_isc(G->optaM, SW_E_installed_software_catalog));
			else
				ret = swpl_send_nothing_and_wait(swi->swicolM, target_fdar[1],
					G->g_swi_event_fd, SWBIS_TS_remove_catalog_entry,
					SWICOL_TL_8, SW_SUCCESS);

			if (ret == 0) {
				;
			} else if (ret < 0) {
				/* internal error */
				E_DEBUG("abort");
				if (!opt_preview) {
					abort_script(swi->swicolM, G, target_fdar[1]);
					swpl_send_abort(swi->swicolM, target_fdar[1], G->g_swi_event_fd, "");
					goto ENTER_WITH_FAILURE;
				}
			} else if (ret > 0) {
				/* script error */
				/* FIXME: add warning message */

				/* since there was exception set the entry to NULL
				   so the invert (restore task is not run on it */
				up_e = NULL;
				;
			} else {
				/* never gets here */
			}
		} else if ( G->g_to_stdout == 0 ) {
			/*
			 * -----------------------------------------------
			 * send nothing and wait to fullfil the task script
			 * -----------------------------------------------
			 */
			E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
			E_DEBUG("");
			if (G->devel_verboseM) {
				fprintf(stderr, "Nothing to upgrade\n");
			}
			/* SWBIS_TS_remove_catalog_entry */
			ret = swpl_send_nothing_and_wait(swi->swicolM, target_fdar[1],
					G->g_swi_event_fd,
					SWBIS_TS_remove_catalog_entry,
					SWICOL_TL_8, SW_SUCCESS);
			E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		} else if ( G->g_to_stdout != 0 ) {
			/* nothing to do */
			E_DEBUG("to_stdout");
			;
		} else {
			;
		}

		/* 
		 * -----------------------
		 * OK, prepare to install
		 * -----------------------
		 */

		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		if (target_nhops >= wopt_kill_hop_threshhold /*2*/ ) {
			/*
			 * Construct the clean-up command 
			 * to shutdown remote script.
			 */ 
			E_DEBUG("");
			G->g_killcmd = NULL;
			if ((ret=swc_construct_newkill_vector(
				target_kill_sshcmd[0], 
				target_nhops,
				target_tramp_list, 
				target_script_pid, 
				G->g_verboseG)) < 0) 
			{
				target_kill_sshcmd[0] = NULL;
				sw_e_msg(G, "target kill command not constructed (ret=%d)\n", ret);
			}
			G->g_target_kmd = target_kill_sshcmd[0];
		}

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG("");
		swi->swi_pkgM->catalog_entryM = strdup(strob_str(target_catalog_path));
		if (swlib_check_clean_path(swi->swi_pkgM->catalog_entryM)) {
			E_DEBUG("");
			sw_e_msg(G, "fatal internal error. target script"
				" catalog entry path error\n");
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G,
				target_loop_count, 
				target_success_count));
		}

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG2("swicol_get_master_alarm_status(swi->swicolM) = [%d]", swicol_get_master_alarm_status(swi->swicolM));
		if (swicol_get_master_alarm_status(swi->swicolM) == 0) {
			/* Only impose this if master_alarm is clear
			   it is raised above by the product compatibility checks */

			E_DEBUG("checking for early shutdown");
			/* Check for early shutdown ??? */	
			if (ts_pid && 
				waitpid(ts_pid, &tmp_status, WNOHANG) > 0) {
				E_DEBUG("");
				swc_shutdown_logger(G, SIGABRT);
				E_DEBUG("early shutdown NOW");
				LCEXIT(sw_exitval(G,
					target_loop_count, 
					target_success_count));
			}
			E_DEBUG("");
		}

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		if (
			(isatty(G->g_meter_fd)) &&
			((
			G->g_verboseG >= 2 &&
			swc_is_option_true(wopt_quiet_progress_bar) == 0
			) || 
			(
			G->g_verboseG >= 1 &&
			swc_is_option_true(wopt_show_progress_bar) == 1
			))
		) {
			*p_do_progressmeter = 1;
		}

		sw_d_msg(G, "target_fdar[0] = %d\n", target_fdar[0]);
		sw_d_msg(G, "target_fdar[1] = %d\n", target_fdar[1]);
		sw_d_msg(G, "target_fdar[2] = %d\n", target_fdar[2]);
		sw_d_msg(G, "source_fdar[0] = %d\n", source_fdar[0]);
		sw_d_msg(G, "source_fdar[1] = %d\n", source_fdar[1]);
		sw_d_msg(G, "source_fdar[2] = %d\n", source_fdar[2]);
			
		tty_raw_ctl(2);
		ret = 0;

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG2("G->g_signal_flag=%d", G->g_signal_flag);
		E_DEBUG("");
		if (strstr(strob_str(source_control_message), 
			"SW_SOURCE_ACCESS_ERROR :") != 
			(char*)NULL) {
			/*
			 * Error
			 */	
			sw_e_msg(G, "SW_SOURCE_ACCESS_ERROR : []\n");
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G,
				target_loop_count, 
				target_success_count));
		}

		/* swc_check_for_current_signals(G, __LINE__,  wopt_no_remote_kill); */

		/*
		 * --------------------------------------
		 *  Install it.
		 * --------------------------------------
		 */
		
		E_DEBUG("");
		if (G->g_verboseG >= SWC_VERBOSE_7) {
			/* 2006-11-27 this causes segfault on solaris/sparc */
			tmp_s = swi_dump_string_s(swi, "swinstall: (SWI*)");
			fprintf(fver, "%s\n", tmp_s);
		}

		E_DEBUG2("f_retval=%d", f_retval);
		E_DEBUG("");
		if (f_retval == 0 ) {
			E_DEBUG("");
			ret = swpl_send_nothing_and_wait(swi->swicolM, target_fdar[1],
				G->g_swi_event_fd,
				SWBIS_TS_check_status,
				SWICOL_TL_8, SW_SUCCESS);
			E_DEBUG2("target_catalog_path is [%s]", strob_str(target_catalog_path));
			ret = do_install(G,
				swi,
				target_fdar[1],
			 	&statbytes,
			 	*p_do_progressmeter,
			 	source_file_size,
				target_path,
			 	working_arg,
				swspecs,
				distdataO,
				strob_str(target_catalog_path), /* catalog_path */
				opt_preview,
				pax_read_command_key,
				wopt_keep_old_files,
				G->g_opt_alt_catalog_root,
				G->g_swi_event_fd,
				opta,
				wopt_allow_missing, up_e);
		} else {
			E_DEBUG("");
			swpl_send_nothing_and_wait(swi->swicolM, target_fdar[1],
					G->g_swi_event_fd,
					SWBIS_TS_Abort,
					SWICOL_TL_8, SW_SUCCESS);
			ret = 0;
		}
		E_DEBUG("");
		if (ret) f_retval++;
		
		/* swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill); */
		G->g_swicolM = NULL;

ENTER_WITH_FAILURE:
		/* swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, 0); */
		if (f_retval == 0)
			swicol_rpsh_wait_for_event(swi->swicolM, (STROB *)NULL,
					G->g_swi_event_fd, SWI_MAIN_SCRIPT_ENDS);
ENTER_WITH_SSH_FAILURE:
		E_DEBUG("");

		if (kill_sshcmd[0]) {
			shcmd_close(kill_sshcmd[0]);
			kill_sshcmd[0] = (SHCMD*)NULL;
		}
		E_DEBUG("");

		if (target_kill_sshcmd[0]) {
			shcmd_close(target_kill_sshcmd[0]);
			target_kill_sshcmd[0] = (SHCMD*)NULL;
		}
		E_DEBUG("");

		if (source_kill_sshcmd[0]) {
			shcmd_close(source_kill_sshcmd[0]);
			source_kill_sshcmd[0] = (SHCMD*)NULL;
		}
		/*
		 * Now close down.
		 */
		/* swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill); */
		E_DEBUG("");
		close(target_fdar[0]);
		E_DEBUG("");
		if (target_fdar[1] != STDOUT_FILENO) 
			close(target_fdar[1]);
		E_DEBUG("");
		if (source_fdar[0] != STDIN_FILENO)
			close(source_fdar[0]);
		E_DEBUG("");
		if (source_fdar[1] != STDOUT_FILENO)
			close(source_fdar[1]);


		/* Here, this use of swlib_kill_all_pids() prevents a hang
		   due to an error for failed dependency, etc.*/
		if (G->g_target_did_abortM || G->g_signal_flag) {
			swlib_kill_all_pids(G->g_pid_array +
				SWC_PID_ARRAY_LEN, 
				G->g_pid_array_len, 
				SIGTERM, 
				G->g_verboseG);
		}

		E_DEBUG("");
		swlib_wait_on_all_pids(
				pid_array, 
				*p_pid_array_len, 
				status_array, 0 /*waitpid flags*/, 
				G->g_verboseG - 2);

		E_DEBUG("");
		if (f_retval == 0) {
			f_retval = swc_analyze_status_array(pid_array,
						*p_pid_array_len, 
						status_array,
						G->g_verboseG - 2);
		}
		E_DEBUG("");
		if (f_retval == 0 && ret ) f_retval++;
		/*
		 * Now re-Initialize.
		 */
		*p_pid_array_len = 0;

		if (soc_spec_target) free(soc_spec_target);
		soc_spec_target = NULL;

		if (working_arg) free(working_arg);
		working_arg = NULL;

		if (cl_target_target) free(cl_target_target);
		cl_target_target = NULL;
		
		if (cl_target_selections) free(cl_target_selections);
		cl_target_selections = NULL;

		if (target_path) free(target_path);
		target_path = NULL;

		/* End real copy, not a preview */
TARGET1:
		E_DEBUG("");
		swgp_signal(SIGINT, safe_sig_handler); /* this handler does not depend on (SWI*) */
		swgp_signal(SIGPIPE, safe_sig_handler); /* hence it must be set before the swi_delete() */
		swgp_signal(SIGTERM, safe_sig_handler); /* is called above */

		E_DEBUG("");
		G->g_pid_array_len = 0;
		/* swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill); */
		strar_close(target_tramp_list);
		strar_close(source_tramp_list);
		strar_close(upgrade_catalog_paths);
		shcmd_close(target_sshcmd[0]);
		strar_close(target_cmdlist);
		strar_close(source_cmdlist);
		strob_close(source_control_message);
		strob_close(target_control_message);
		strob_close(target_start_message);
		strob_close(target_catalog_path);
		strob_close(source_start_message);
		strob_close(source_access_message);

		if (source_script_pid) free(source_script_pid);
		source_script_pid = NULL;
		if (target_script_pid) free(target_script_pid);
		target_script_pid = NULL;

		target_loop_count++;
		if (f_retval == 0) {
			target_success_count++;
		}
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_3, &G->g_logspec,
				f_retval ? swc_get_stderr_fd(G) : swevent_fd,
				"SWBIS_TARGET_ENDS for %s: status=%d\n",
						current_arg,
						f_retval);
		/* swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill); */
		free(current_arg);
   		current_arg = swc_get_next_target(argv, 
						argc, 
						&optind, 
						G->g_targetfd_array,
					opt_distribution_target_directory, &num_remains);
	} /* target loop */
	E_DEBUG("");
	sw_d_msg(G, "Finished processing targets\n");
	if (targetlist_fd >= 0) close(targetlist_fd);
	sw_d_msg(G, "exiting at %s:%d with value %d\n", __FILE__, __LINE__,
		sw_exitval(G, target_loop_count, target_success_count));

	sw_l_msg(G, SWC_VERBOSE_6, "SWI_NORMAL_EXIT: status=%d\n",
		sw_exitval(G, target_loop_count, target_success_count));

	E_DEBUG("");
	if (	do_to_stdout &&
		sw_exitval(G, target_loop_count, target_success_count) == 0
	) {
		/*
		 * Write the NUL trailer blocks for the --to-stdout mode
		 */
		etar_write_trailer_blocks(NULL, STDOUT_FILENO, 2);
	}

	swutil_close(swlog);
	f_retval = swc_shutdown_logger(G, 0);
	if (f_retval) return(3);
	return(sw_exitval(G, target_loop_count, target_success_count));	
}
