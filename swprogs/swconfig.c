/*  swconfig.c -- The swbis package config utility.

 Copyright (C) 2010 Jim Lowe
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
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swevents.h"
#include "swicol.h"
#include "swicat.h"
#include "swicat_s.h"
#include "swicat_e.h"
#include "swutillib.h"
#include "swmain.h"
#include "globalblob.h"
#include "swproglib.h"
#include "swconfig_lib.h"
#include "atomicio.h"
#include "swlex_supp.h"

#define SWCONFIG_UPVERBOSE	0
#define PROGNAME                SW_UTN_CONFIG
#define SW_UTILNAME             PROGNAME
#define SWPROG_VERSION          SWCONFIG_VERSION
#define WOPT_LAST		236   /* Highest option number */

static char progName[] = PROGNAME;
static char * CHARTRUE = "True";
static char * CHARFALSE = "False";

static GB g_blob;   /* Global blob of utility independent stuff */
static GB * G;      /* pointer to Global blob */

static
void 
version_string(FILE * fp)
{
        fprintf(fp,   
"%s (swbis) version " SWCONFIG_VERSION "\n",
                progName);
}

static
void 
version_info(FILE * fp)
{
	version_string(fp);
	fprintf(fp,  "%s",
"Copyright (C) 2010 Jim Lowe\n"
"Portions are copyright 1985-2000 Free Software Foundation, Inc.\n"
"This software is distributed under the terms of the GNU General Public License\n"
"and comes with NO WARRANTY to the extent permitted by law.\n"
"See the file named COPYING for details.\n");
}
					
static
int 
usage(FILE * fp, 
		char * pgm, 
		struct option  lop[], 
		struct ugetopt_option_desc helpdesc[])
{
	
fprintf(fp, "%s",
"swconfig configure installed software\n"
);
	
	fprintf(fp, "%s",
"\n"
"Usage: swconfig [-p] [-u] [-v] [-f file] [-t targetfile] [-x option=value]\\\n"
"[-X options_file] [-W option[=value]] [software_selections][@target]\n"
);

fprintf(fp, "%s",
"\n"
" Options:\n"
);
	ugetopt_print_help(fp, pgm, lop, helpdesc);

fprintf(fp, "%s",
"\n"
" Examples:\n"
"    swconfig --show-options\n"
"    swconfig --show-options-files\n"
"    swconfig --allow-ambig \\* @ 192.168.3.3    # Configure everything at 192.168.3.3\n"
"\n"
" Implementation Extension Options:\n"
"    Syntax : -W option[=option_argument] [-W ...]\n"
"        or   -W option[=option_argument],option...\n"
"        or   --option[=option_argument]\n"
"\n"
"  Options for Alternate Modes:\n"
"\n"
"     --show-options      Show the extended options and exit\n"
"     --show-options-files   Show the options files that will be parsed.\n"
"\n"
"  Operational Options:\n"
"\n"
"     --postinstall  Run the postinstall/unpostinstall scripts instead of the\n"
"                    configure/unconfigure scripts\n"
"     --allow-ambig  Allow multple packages to be selected by one selection spec\n"
"     --force-locks  ignore locking, and remove existing lock.\n"
"     --sig-level=N  N=0,1, or 2,.. number of sigs required\n"
"     --noop              Legal option with no effect\n"
"     --no-defaults-file        Do not read any defaults files.\n"
"     --send-environment   safely include enviroment variables from\n"
"                       the current environment in a remote target's environment.\n"
"                       Common system variables are excluded.  Tainted checks are\n"
"                       made and existing values [in remote target's environment]\n"
"                       are retained\n"
"\n"
"  Target and Remote connection options\n"
"\n"
"     --remote-shell=NAME  NAME is the client \"rsh\" or \"ssh\"(default).\n"
"     --no-remote-kill     Do not execute a kill command. Normally a kill\n"
"                        command is issued when signals TERM, PIPE, and INT\n"
"                        are received for targets where nhops > 1.\n"
"     --no-getconf      Don't use the getconf utility to set the PATH variable\n"
"                        on the target host.\n"
"     --use-getconf     This causes getconf to be used.\n"
"     --shell-command={detect|posix|sh|bash|ksh}  detect is the default\n"
"     --ssh-options=options      single letter ssh options. e.g. --ssh-options=1\n"
"  -A,--A,--enable-ssh-agent-forwarding\n"
"     --a,--disable-ssh-agent-forwarding\n"
"\n"
" Debugging Controls Options:\n"
"\n"
"     --debug-events             show the internal events listing to stderr\n"
"     --source-script-name=NAME  write the internal stdin script to NAME.\n"
"     --target-script-name=NAME  write the internal stdin script to NAME.\n"
"     --debug-task-scripts       write each task shell to a file in /tmp\n"
"\n"
);

    swc_helptext_target_syntax(fp);

fprintf(fp, "%s",
"\n"
" Posix Extended Options:\n"
"        File : <libdir>/swbis/swdefaults\n"
"               ~/.swdefaults\n"
"        Override values in defaults file.\n"
"        Command Line Syntax : -x option=option_argument [-x ...]\n"
"            or : -x \"option=option_argument  ...\"\n"
"  allow_incompatible          = false      # Not Implemented\n"
"  allow_multiple_versions     = -	     # Not Implemented\n"
"  ask                         = false      # Not Implemented\n"
"  autoselect_dependencies     = false      # Not Implemented\n"
"  autoselect_dependents       = false      # Not Implemented\n"
"  enforce_dependencies        = false      # Not Implemented\n"
"  enforce_scripts             = true \n"
"  installed_software_catalog  = var/lib/swbis/catalog\n"
"  logfile                     = /var/log/swconfig.log\n"
"  loglevel                    = 0\n"
"  reconfigure                 = false\n"
"  select_local                = false      # Not Implemented\n"
"  verbose                     = 1\n"
"\n"
" Swbis Extended Options:\n"
"        File : <libdir>/swbis/swbisdefaults\n"
"               ~/.swbis/swbisdefaults\n"
"        Command Line Syntax : -W option=option_argument\n"
"  swconfig.swbis_no_getconf		 = true # getconf is used\n"
"  swconfig.swbis_shell_command	         = detect\n"
"  swconfig.swbis_no_remote_kill	 = false\n"
"  swconfig.swbis_forward_agent          = true\n"
"  swconfig.swbis_local_pax_write_command  = tar\n"
"  swconfig.swbis_remote_pax_write_command = tar\n"
"  swconfig.swbis_local_pax_read_command   = tar\n"
"  swconfig.swbis_remote_pax_read_command  = tar\n"
"  swconfig.swbis_enforce_all_signatures   = false\n"
"  swconfig.swbis_enforce_all_signatures   = false\n"
"  swconfig.swbis_sig_level                = 0\n"
"\n"
);

	fprintf(fp , "%s", "\n");
	version_string(fp);
	fprintf(fp , "\n%s", 
        "Report bugs to " REPORT_BUGS "\n");
	return 0;
}

void
process_pax_command(GB * G, char * command, char * type, int sw_e_num)
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

static
void
abort_script(SWICOL * swicol, GB * G, int fd)
{
	if (G->g_target_did_abortM)
		return;
        swicol_set_master_alarm(swicol);
	/* wrong to send this here
	if (G->in_shls_looperM)
		swicol_send_loop_trailer(swicol, fd);
	*/
        /* swpl_send_abort(swicol, fd, G->g_swi_event_fd, ""); */
	sw_e_msg(G, "Executing abort_script() ... \n");
	G->g_target_did_abortM = 1;
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
		fprintf(stderr, "%s\n", swverid_print(swverid, tmp));
	}
	strob_close(tmp);
	return 0;
}

static
int
does_have_sw_selections(VPLOB * swspecs)
{
	return
	vplob_get_nstore(swspecs) > 0 ? 1 : 0;
}

static
void
critical_region_begin(void)
{
	swgp_signal_block(SIGINT, (sigset_t *)NULL);
}

static
void
critical_region_end(void)
{
	swgp_signal_unblock(SIGINT, (sigset_t *)NULL);
}


static
void
safe_sig_handler(int signum)
{
	E_DEBUG("");

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
			sw_e_msg(G, "Caught SIGTERM\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGINT:
			if (G->g_do_progressmeter) alarm(0);
			/* sw_e_msg(G, "Caught SIGINT\n"); */
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGPIPE:
			if (G->g_do_progressmeter) alarm(0);
			sw_e_msg(G, "Caught SIGPIPE\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
	}
}

static
void
main_sig_handler(int signum)
{
	int fd;
	int loggersig = SIGABRT;

	switch(signum) {
		case SIGTERM:
		case SIGINT:
		case SIGPIPE:
			if (G->g_running_signalsetM) return;
			sw_e_msg(G, "Executing handler for signal %d.\n", G->g_psignalM);
			if (G->g_do_progressmeter) alarm(0);
			swgp_signal_block(SIGTERM, (sigset_t *)NULL);
			swgp_signal_block(SIGINT, (sigset_t *)NULL);
			G->g_psignalM = signum;
			if (G->g_swicolM) {
				/* need to shutdown the target script */
				sw_e_msg(G, "Executing abort\n");
				abort_script(G->g_swicolM, G, G->g_target_fdar[1]);
				raise(SIGUSR1);
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
			/* swgp_signal_block(SIGPIPE, (sigset_t *)NULL); */
			swlib_tty_atexit();
			fd = G->g_target_fdar[0];
			if (fd >= 0) close(fd);
			fd = G->g_target_fdar[1];
			if (fd >= 0) close(fd);
			swc_gf_xformat_close(G, G->g_xformat);
			swlib_kill_all_pids(G->g_pid_array +
						SWC_PID_ARRAY_LEN, 
						G->g_pid_array_len, 
						SIGTERM, 
						G->g_verboseG);
			swlib_wait_on_all_pids(G->g_pid_array, 
						G->g_pid_array_len, 
						G->g_status_array, 
						0, 
						G->g_verboseG);
			swc_shutdown_logger(G, loggersig);
			switch(signum) {
				case SIGTERM:
				case SIGINT:
					/* FIXME per spec */
					LCEXIT(SW_FAILED_ALL_TARGETS); 
					break;
			}
			break;
	}
}

static
void
apply_usage_restrictions(char * source_type, GB * G,
			char * current_arg)
{
	int val;
}

static
int
sc_selections_check(GB * G, struct extendedOptions * opta,  SWICAT_SC * sc)
{
	STROB * tmp;
	int ret;
	int i;
	SWICAT_SR * sr;

	tmp = strob_open(100);
	ret = 0;
	i = 0;
	while ((sr=vplob_val(sc->srM, i++)) != NULL) {
		if (strlen(swicat_sr_form_catalog_path(sr, "", tmp)) > 0) {
			ret++;
		}
	}

	strob_close(tmp);
	return ret;
}


/* Main Program */

int
main(int argc, char **argv)
{
	int c;
	int i;
	int j;
	int ret;
	int optret = 0;
	int target_fdar[3];
	int source_fdar[3];
	int target_stdio_fdar[3];
	int source_stdio_fdar[3];
	int save_stdio_fdar[3];
	int targetlist_fd = -1;
	int target_nhops;
	int main_optind = 0;
	int num_remains;
	int e_argc = 1;
	int w_argc = 1;
	int swi_event_fd = -1;
	pid_t ss_pid = 0;
	pid_t target_write_pid = 0;
	pid_t target_pump_pid = 0;
	int is_target_local_regular_file = 0;
	char * optionname;
	SWI_DISTDATA * distdataO;

	char * opt_option_files = (char*)NULL;
	char * response_img;

	char * wopt_shell_command = (char*)(NULL);
	char * wopt_enforce_all_signatures;
	char * wopt_sig_level;
	int    wopt_no_defaults = 0;
	int    wopt_kill_hop_threshhold = 2;  /* 2 */
	char * wopt_no_getconf;
	char * wopt_noscripts;
	int opt_loglevel;

	char * wopt_forward_agent = (char*)CHARTRUE;
	char * wopt_no_remote_kill = (char*)NULL;
	char * wopt_local_pax_write_command = (char*)NULL;
	char * wopt_remote_pax_write_command = (char*)NULL;
	char * wopt_local_pax_read_command = (char*)NULL;
	char * wopt_remote_pax_read_command = (char*)NULL;
	char * wopt_local_pax_remove_command = (char*)NULL;
	char * wopt_remote_pax_remove_command = (char*)NULL;
	char * wopt_ssh_options = (char*)NULL;
	int    wopt_with_hop_pids = 1;  /* internal test control */
	char * wopt_show_options;
	char * wopt_show_options_files;
	char * wopt_allow_ambig;
	char * wopt_blocksize = "5120";
	char wopt_pgm_mode[30];
	
	char * eopt_autoselect_dependencies;
	char * eopt_autoselect_dependents;
	char * eopt_allow_incompatible;
	char * eopt_allow_multiple_versions;
	char * eopt_ask;
	char * eopt_enforce_dependencies;
	char * eopt_installed_software_catalog;
	char * eopt_logfile;
	char * eopt_loglevel;
	char * eopt_reconfigure;
	char * eopt_verbose;
	char * eopt_select_local;
	char * eopt_enforce_scripts;

	int opt_preview = 0;
	int nmatches;
	int optionEnum;
	int devel_no_fork_optimization = 0; /* reject the no fork optimization */
	int local_stdin = 0;
	int do_extension_options_via_goto = 0;
	int fverfd;
	int target_loop_count = 0;
	int target_success_count = 0;
	int * p_do_progressmeter;
	int make_master_raw = 1;
	int is_local_stdin_seekable = 0;
	int tmpret;
	int tmp_status;
	int nullfd;
	int testnum = 0;
	int f_retval = 0;	/* protocol, internal errors */
	int s_retval = 0;	/* target, usage and content  error2 */
	int found_ambig = 0;
	int target_file_size;
	int target_array_index = 0;
	int select_array_index = 0;
	int stdin_in_use = 0;
	int use_no_getconf;
	int swevent_fd = STDOUT_FILENO;
	int local_stdout_in_use = 0;
	int reqs_failed = 0;
	int rp_status;
	int did_skip;
	int total_skipped = 0;
	struct extendedOptions * opta = optionsArray;
	struct termios	*login_orig_termiosP = NULL;
	struct winsize	*login_sizeP = NULL;
	unsigned long int statbytes;
	
	char * tty_opt = NULL;
	char * target_fork_type;
	char * target_script_pid = (char*)NULL;
	char * tmpcharp = (char*)NULL;
	char * exoption_arg;
	char * exoption_value;
	char * cl_target_target = (char*)NULL;
	char * new_cl_target_target = (char*)NULL;
	char * xcl_target_target = (char*)NULL;
	char * cl_target_selections = (char*)NULL;
	char * target_path = (char*)NULL;
	char * catalog_entry_directory;
	char * pax_read_command_key = (char*)NULL;
	char * pax_write_command_key = (char*)NULL;
	char * pax_remove_command_key = (char*)NULL;
	char * tmp_working_arg = (char*)NULL;
	char * working_arg = (char*)NULL;
	char * current_arg = (char*)NULL;
	char * system_defaults_files = (char*)NULL;
	char cwd[1024];
	char * remote_shell_command = REMOTE_SSH_BIN;
	char * remote_shell_path;

	STRAR * target_cmdlist;
	STRAR * target_tramp_list;
	
	CPLOB * w_arglist;
	CPLOB * e_arglist;

	VPLOB * swspecs;
	VPLOB * pre_reqs;
	VPLOB * ex_reqs;
	
	STROB * btmp;
	STROB * btmp2;
	STROB * target_current_dir;
	STROB * target_control_message;
	STROB * target_start_message;
	STROB * target_catalog_message;
	STROB * target_access_message;
	STROB * source_line_buf;
	STROB * target_line_buf;
	STROB * isc_script_buf;
	STROB * tmpcommand;
	STROB * target_catalog_dir;

	SHCMD * target_sshcmd[2];
	SHCMD * source_sshcmd[2];
	SHCMD * kill_sshcmd[2];
	SHCMD * target_kill_sshcmd[2];

	FILE * fver;

	sigset_t * ssh_fork_blockmask;
	sigset_t * fork_blockmask;
	sigset_t * fork_defaultmask;
	sigset_t * currentmask;

	SWLOG * swlog;
	SWICOL * swicol;
	SWUTS * uts;
	SWICAT_SL * sl;
	SWICAT_REQ * req;
	SWICAT_SC * sc;
	SWICAT_SR * sr;

	struct option main_long_options[] =
             {
		{"distribution", 0, 0, 'd'},
		{"selections-file", 1, 0, 'f'},
		{"alternate-catalog-root", 0, 0, 'r'},
		{"target-file", 1, 0, 't'},
		{"source", 1, 0, 's'},
		{"defaults-file", 1, 0, 'X'},
		{"index-format", 0, 0, 'v'},  /* this requires disambiguation with -v */
		{"verbose", 0, 0, 'V'},
		{"extension-option", 1, 0, 'W'},
		{"extended-option", 1, 0, 'x'},
		{"version", 0, 0, 'Y'},
		{"help", 0, 0, '\012'},
		{"noop", 0, 0, 154},
		{"show-options", 0, 0, 158},
		{"show-options-files", 0, 0, 159},
		{"testnum", 1, 0, 164},
		{"debug-verbose", 0, 0, 165},
		{"force-locks", 0, 0, 167},
		{"remote-shell", 1, 0, 172},
		{"local-pax-write-command", 1, 0, 175},
		{"remote-pax-write-command", 1, 0, 176},
		{"pax-write-command", 1, 0, 177},
		{"remote-pax-read-command", 1, 0, 178},
		{"local-pax-read-command", 1, 0, 179},
		{"pax-read-command", 1, 0, 180},
		{"no-defaults-files", 0, 0, 181},
		{"pax-command", 1, 0, 182},
		{"no-remote-kill", 0, 0, 184},
		{"no-getconf", 0, 0, 185},
		{"use-getconf", 0, 0, 186},
		{"shell-command", 1, 0, 188},
		{"ssh-options", 1, 0, 195},
		{"debug-events", 0, 0, 196},
		{"source-script-name", 1, 0, 198},
		{"target-script-name", 1, 0, 199},
		{"swi-debug-name", 1, 0, 200},
		{"debug-task-scripts", 0, 0, 201},
		{"enable-ssh-agent-forwarding", 0, 0, 202},
		{"A", 0, 0, 202},
		{"disable-ssh-agent-forwarding", 0, 0, 203},
		{"a", 0, 0, 203},
		{"noscripts", 0, 0, 204},	
		{"no-scripts", 0, 0, 204},	
		{"remote-pax-remove-command", 1, 0, 205},
		{"local-pax-remove-command", 1, 0, 206},
		{"pax-remove-command", 1, 0, 207},
		{"allow-ambig", 0, 0, 208},
		{"sig-level", 1, 0, 209},
		{"postinstall", 0, 0, 210},
		{"send-environment", 0, 0, 211},
		{"swbis_no_getconf", 		1, 0, 218},
		{"swbis-no-getconf", 		1, 0, 218},
		{"swbis_shell_command", 	1, 0, 219},
		{"swbis-shell-command", 	1, 0, 219},
		{"swbis_no_remote_kill", 	1, 0, 220},
		{"swbis-no-remote-kill", 	1, 0, 220},
		{"swbis_local_pax_write_command", 1, 0, 222},
		{"swbis-local-pax-write-command", 1, 0, 222},
		{"swbis_remote_pax_write_command", 1, 0, 223},
		{"swbis-remote-pax-write-command", 1, 0, 223},
		{"swbis_local_pax_read_command", 1, 0, 224},
		{"swbis-local-pax-read-command", 1, 0, 224},
		{"swbis_remote_pax_read_command", 1, 0, 225},
		{"swbis-remote-pax-read-command", 1, 0, 225},
		{"swbis_local_pax_remove_command", 1, 0, 226},
		{"swbis-local-pax-remove-command", 1, 0, 226},
		{"swbis_remote_pax_remove_command", 1, 0, 227},
		{"swbis-remote-pax-remove-command", 1, 0, 227},
		{"swbis_enforce_all_signatures", 	1, 0, 228},
		{"swbis-enforce-all-signatures", 	1, 0, 228},
		{"swbis_sig_level", 1, 0, 229},
		{"swbis-sig-level", 1, 0, 229},
		{"swbis_forward_agent", 	1, 0, 236},
		{"swbis-forward-agent", 	1, 0, 236},
		{0, 0, 0, 0}
             };

	struct option posix_extended_long_options[] =
             {
		{"allow_incompatible", 		1, 0, 140},
		{"allow_mutiple_versions",	1, 0, 141},
		{"ask",				1, 0, 142},
		{"autoselect_dependencies", 	1, 0, 143},
		{"autoselect_dependents", 	1, 0, 144},
		{"installed_software_catalog",	1, 0, 145},
		{"enforce_dependencies", 	1, 0, 146},
		{"logfile", 			1, 0, 147},
		{"loglevel", 			1, 0, 148},
		{"reconfigure",		 	1, 0, 149},
		{"verbose", 			1, 0, 150},
		{"enforce_scripts", 		1, 0, 151},
               {0, 0, 0, 0}
             };

	struct option std_long_options[] =
             {
		{"preview", 0, 0, 'p'},
		{"unconfigure", 0, 0, 'u'},
		{"selections-file", 1, 0, 'f'},
		{"target-file", 1, 0, 't'},
		{"extended-option", 1, 0, 'x'},
		{"defaults-file", 1, 0, 'X'},
		{"verbose", 0, 0, 'V'},
		{"extension-option", 1, 0, 'W'},
		{"help", 0, 0, '\012'},
		{0, 0, 0, 0}
             };

	struct ugetopt_option_desc main_help_desc[] =
             {
	{"", "", "Preview only by showing information to stdout"},
	{"", "", "unconfigure, run the un<script>"},
	{"", "FILE","Take software selections from FILE."},
	{"", "FILE", "Specify a FILE containing a list of targets." },
	{"", "option=value", "Specify posix extended option."},
	{"", "FILE[ FILE2 ...]", "Specify files that override \n"
	"        system option defaults. Specify empty string to disable \n"
	"        option file reading."},
	{"", "", "increment verbose level"},
	{"", "optionarg", "Specify implementation extension option."},
	{"help", "", "Show this help to stdout (impl. extension)."},
	{0, 0, 0}
	};

	/*  End of Decalarations.  */
	
	swevent_set_verbose(SWC_VERBOSE_3);
	swlib_set_swbis_verbose_threshold(SWC_VERBOSE_3);

	strcpy(wopt_pgm_mode, "");
	G = &g_blob;
	gb_init(G);
	G->g_target_fdar = target_fdar;
	G->g_source_fdar = source_fdar;
	G->g_save_fdar = save_stdio_fdar;
	G->g_verbose_threshold = SWC_VERBOSE_1;
	G->optaM = opta;
	
	uxfio_devnull_open("/dev/null", O_RDWR, 0);  /* initiallize the null fd */

	pre_reqs = NULL;  /* used for testing, not by a published option */
	ex_reqs = NULL;   /* used for testing, not by a published option */

	target_fdar[0] = -1;
	target_fdar[1] = -1;
	target_fdar[2] = -1;

	source_fdar[0] = -1;
	source_fdar[1] = -1;
	source_fdar[2] = -1;

	source_stdio_fdar[0] = (STDIN_FILENO);
	source_stdio_fdar[1] = (STDOUT_FILENO);
	source_stdio_fdar[2] = (STDERR_FILENO);

	target_stdio_fdar[0] = (STDIN_FILENO);
	target_stdio_fdar[1] = (STDOUT_FILENO);
	target_stdio_fdar[2] = (STDERR_FILENO);

	save_stdio_fdar[0] = dup(STDIN_FILENO);
	save_stdio_fdar[1] = dup(STDOUT_FILENO);
	save_stdio_fdar[2] = dup(STDERR_FILENO);
	
	G->g_vstderr = stderr;

	G->g_main_sig_handler = main_sig_handler;
	G->g_safe_sig_handler = safe_sig_handler;

	target_fork_type = G->g_fork_pty_none;	

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
	currentmask = &G->g_currentmask;
	ssh_fork_blockmask = &G->g_ssh_fork_blockmask;
	sigemptyset(currentmask);
	sigemptyset(fork_blockmask);
	sigemptyset(ssh_fork_blockmask);
	sigaddset(fork_blockmask, SIGALRM);

	sigemptyset(fork_defaultmask);
	sigaddset(fork_defaultmask, SIGINT);
	sigaddset(fork_defaultmask, SIGPIPE);

	/* block SIGINT in ssh, this seems to be required to
	   avoid stranded ``bash -s'' processes on the remote target
	   host, ssh is sent SIGTERM below if signal handling is in progress */	
	sigaddset(ssh_fork_blockmask, SIGINT);
	
	target_current_dir = strob_open(10);
	source_line_buf = strob_open(10);
	target_line_buf = strob_open(10);
	target_catalog_dir = strob_open(10);

	nullfd = G->g_nullfd;

	swc0_create_parser_buffer();
	
	target_sshcmd[1] = (SHCMD*)NULL;
	source_sshcmd[1] = (SHCMD*)NULL;
	kill_sshcmd[1] = (SHCMD*)NULL;
	target_kill_sshcmd[1] = (SHCMD*)NULL;
	
	kill_sshcmd[0] = (SHCMD*)NULL;
	source_sshcmd[0] = shcmd_open();
	target_sshcmd[0] = (SHCMD*)NULL;
	target_kill_sshcmd[0] = shcmd_open();

	btmp = strob_open(10);		/* General use String object. */
	btmp2 = strob_open(10);		/* General use String object. */

	tmpcommand = strob_open(10);
	w_arglist = cplob_open(1);	/* Pointer list object. */
	e_arglist = cplob_open(1);	/* Pointer list object. */
	swicol = swicol_create();
	G->g_swicolM = NULL;
	uts = swuts_create();
	initExtendedOption();

	if (getcwd(cwd, sizeof(cwd) - 1) == NULL) {
		sw_e_msg(G, "%s\n", strerror(errno));
		LCEXIT(1);
	}
	cwd[sizeof(cwd) - 1] = '\0';

	/* Set the compiled-in defaults for the extended options.  */

	eopt_allow_incompatible          = CHARFALSE;
	eopt_allow_multiple_versions     = CHARFALSE; 
	eopt_ask                         = CHARFALSE; 
	eopt_autoselect_dependencies     = CHARFALSE; 
	eopt_autoselect_dependents       = CHARFALSE; 
	eopt_enforce_dependencies        = CHARFALSE; 
	eopt_enforce_scripts             = CHARTRUE; 
	eopt_installed_software_catalog  = "var/lib/swbis/catalog/";
	eopt_logfile                     = "/var/log/swconfig.log";
	eopt_loglevel                    = "0";
	eopt_reconfigure                 = CHARFALSE;
	eopt_select_local		 = CHARFALSE;
	eopt_verbose			 = "1";

	wopt_no_getconf			= CHARTRUE;
	wopt_no_remote_kill		= CHARFALSE;
	wopt_shell_command		= "detect";
	wopt_local_pax_write_command 	= "tar"; 
	wopt_remote_pax_write_command 	= "tar";
	wopt_local_pax_read_command 	= "tar";
	wopt_remote_pax_read_command 	= "tar";
	wopt_local_pax_remove_command 	= "tar";
	wopt_remote_pax_remove_command 	= "tar";
	wopt_allow_ambig 		= CHARFALSE;
	wopt_noscripts 			= CHARFALSE;
	wopt_enforce_all_signatures	= CHARFALSE;
	wopt_sig_level			= "0";
	wopt_show_options = NULL;
	wopt_show_options_files = NULL;

	set_opta_initial(opta, SW_E_allow_incompatible, CHARFALSE);
	set_opta_initial(opta, SW_E_allow_multiple_versions, CHARFALSE);
	set_opta_initial(opta, SW_E_ask, CHARFALSE);
	set_opta_initial(opta, SW_E_autoselect_dependencies, CHARFALSE);
	set_opta_initial(opta, SW_E_autoselect_dependents, CHARFALSE);
	set_opta_initial(opta, SW_E_enforce_dependencies, CHARFALSE);
	set_opta_initial(opta, SW_E_enforce_scripts, eopt_enforce_scripts);
	set_opta_initial(opta, SW_E_installed_software_catalog, eopt_installed_software_catalog);
	set_opta_initial(opta, SW_E_logfile, eopt_logfile);
	set_opta_initial(opta, SW_E_loglevel, eopt_loglevel);
	set_opta_initial(opta, SW_E_reconfigure, eopt_reconfigure);
	set_opta_initial(opta, SW_E_select_local, eopt_select_local);
	set_opta_initial(opta, SW_E_verbose, eopt_verbose);


	set_opta_initial(opta, SW_E_swbis_no_getconf, wopt_no_getconf);
	set_opta_initial(opta, SW_E_swbis_remote_shell_client,
 				          remote_shell_command);
	set_opta_initial(opta, SW_E_swbis_shell_command, wopt_shell_command);
	set_opta_initial(opta, SW_E_swbis_no_remote_kill, wopt_no_remote_kill);
	set_opta_initial(opta, SW_E_swbis_local_pax_write_command,
					wopt_local_pax_write_command);
	set_opta_initial(opta, SW_E_swbis_remote_pax_write_command,
					wopt_remote_pax_write_command);
	set_opta_initial(opta, SW_E_swbis_local_pax_read_command,
					wopt_local_pax_read_command);
	set_opta_initial(opta, SW_E_swbis_remote_pax_read_command,
					wopt_remote_pax_read_command);
	set_opta_initial(opta, SW_E_swbis_local_pax_remove_command,
					wopt_local_pax_remove_command); 
	set_opta_initial(opta, SW_E_swbis_remote_pax_remove_command,
					wopt_remote_pax_remove_command); 
	set_opta_initial(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
	set_opta_initial(opta, SW_E_swbis_ignore_scripts, wopt_noscripts);
	
	set_opta_initial(opta, SW_E_swbis_enforce_all_signatures, wopt_enforce_all_signatures);
	set_opta_initial(opta, SW_E_swbis_sig_level, wopt_sig_level);

	cplob_add_nta(w_arglist, strdup(argv[0]));
	cplob_add_nta(e_arglist, strdup(argv[0]));

	while (1) {
		int option_index = 0;

		c = ugetopt_long(argc, argv, "Auf:t:pvs:X:x:W:", 
					main_long_options, &option_index);
		if (c == -1) break;

		switch (c) {
		case 'A':
			wopt_forward_agent = CHARTRUE;
			set_opta(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
			break;
		case 'u':
			G->g_do_unconfigM = 1;
			break;
		case 'p':
			opt_preview = 1;
			G->g_opt_previewM = 1;
			break;
		case 'v':
			G->g_verboseG++;
			eopt_verbose = (char*)malloc(12);
			snprintf(eopt_verbose, 11, "%d", G->g_verboseG);
			eopt_verbose[11] = '\0';
			set_opta(opta, SW_E_verbose, eopt_verbose);
			free(eopt_verbose);
			break;
		case 'f':
			/* Process the selection file */
			if (swc_process_swoperand_file(swlog,
				"selections", optarg, &stdin_in_use,
				&select_array_index, G->g_selectfd_array))
			{
				LCEXIT(1);
			}
			break;
		case 't':
			/* Process the target file */
			if (swc_process_swoperand_file(swlog,
				"target", optarg, &stdin_in_use,
				&target_array_index, G->g_targetfd_array))
			{
				LCEXIT(1);
			}
			break;
		case 'W':
			swc0_process_w_option(btmp, w_arglist, optarg, &w_argc);
			break;
		case 'x':
			exoption_arg = strdup(optarg);
			SWLIB_ALLOC_ASSERT(exoption_arg != NULL);
			/*  Parse the extended option and add to pointer list
			    for later processing.  */
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
					strob_strcpy(btmp, "--");
					strob_strcat(btmp, t);
					strob_strcat(btmp, "=");
					strob_strcat(btmp, exoption_value);
					cplob_add_nta(e_arglist, 
						strdup(strob_str(btmp)));
					e_argc++;

					*np = '=';
					t = strob_strtok(etmp, NULL, " ");
				}
				strob_close(etmp);
			}
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
               case 'Y':
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
			sw_e_msg(G, "Try `swconfig --help' for more information.\n");
			LCEXIT(1);
		 	break;
               default:
			if (c >= 154 && c <= WOPT_LAST) { 
				/*  This provides the ablility to specify 
				    extension options by using the 
				    --long-option syntax (i.e. without using 
				    the -Woption syntax)  */
				do_extension_options_via_goto = 1;
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
		case 140:
			eopt_allow_incompatible = strdup(optarg);
			set_opta_boolean(opta, SW_E_allow_incompatible, eopt_allow_incompatible);
			break;
		case 141:
			eopt_allow_multiple_versions = strdup(optarg);
			set_opta_boolean(opta, SW_E_allow_multiple_versions, eopt_allow_multiple_versions);
			break;
		case 142:
			eopt_ask = strdup(optarg);
			set_opta_boolean(opta, SW_E_ask, eopt_ask);
			break;
		case 143:
			eopt_autoselect_dependencies = strdup(optarg);
			set_opta_boolean(opta, SW_E_autoselect_dependencies, eopt_autoselect_dependencies);
			break;
		case 144:
			eopt_autoselect_dependents = strdup(optarg);
			set_opta_boolean(opta, SW_E_autoselect_dependents, eopt_autoselect_dependents);
			break;

		case 145:
			eopt_installed_software_catalog = strdup(optarg);
			set_opta(opta, SW_E_installed_software_catalog, eopt_installed_software_catalog);
			break;
		case 146:
			eopt_enforce_dependencies = strdup(optarg);
			set_opta_boolean(opta, SW_E_enforce_dependencies, optarg);
			break;
		case 151:
			eopt_enforce_scripts = strdup(optarg);
			set_opta_boolean(opta, SW_E_enforce_scripts, optarg);
			break;
		case 147:
			eopt_logfile = strdup(optarg);
			set_opta(opta, SW_E_logfile, eopt_logfile);
			SWLIB_ALLOC_ASSERT(eopt_logfile != NULL);
			break;
		case 148:
			eopt_loglevel = strdup(optarg);
			opt_loglevel = swlib_atoi(optarg, NULL);
			G->g_loglevel = opt_loglevel;
			G->g_logspec.loglevelM = opt_loglevel;
			set_opta(opta, SW_E_loglevel, eopt_loglevel);
			SWLIB_ALLOC_ASSERT(eopt_loglevel != NULL);
			break;
		case 149:
			eopt_reconfigure = strdup(optarg);
			set_opta_boolean(opta, SW_E_reconfigure, eopt_reconfigure);
			break;
		case 150:
			eopt_verbose = strdup(optarg);
			G->g_verboseG = swlib_atoi(eopt_verbose, NULL);
			set_opta(opta, SW_E_verbose, eopt_verbose);
			free(eopt_verbose);
			break;
		default:
			sw_e_msg(G, "error processing extended option\n");
			sw_e_msg(G, "Try `swconfig --help' for more information.\n");
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

	/*  Now run the Implementation extension options (-W) through getopt.  */
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
		case 154: /* noop */
			break;
		case 158:
			wopt_show_options = CHARTRUE;
			break;
		case 159:
			wopt_show_options_files = CHARTRUE;
			break;
		case 164:
			/* Development testing controls.  */
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
		case 167:
			G->g_force_locks = 1;
			break;
		case 172:
			remote_shell_command = strdup(optarg);
			set_opta(opta, SW_E_swbis_remote_shell_client, remote_shell_command);
			break;
		case 175:
			wopt_local_pax_write_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_write_command, "write",
						SW_E_swbis_local_pax_write_command);
			break;
		case 176:
			wopt_remote_pax_write_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_write_command, "write",
						SW_E_swbis_remote_pax_write_command);
			break;
		case 177:
			wopt_local_pax_write_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_write_command, "write",
						SW_E_swbis_local_pax_write_command);

			wopt_remote_pax_write_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_write_command, "write",
						SW_E_swbis_remote_pax_write_command);
			break;
		case 178:
			wopt_remote_pax_read_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_read_command, "read",
						SW_E_swbis_remote_pax_read_command);
			break;
		case 179:
			wopt_local_pax_read_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_read_command, "read",
						SW_E_swbis_local_pax_read_command);
			break;
		case 180:
			wopt_local_pax_read_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_read_command, "read",
						SW_E_swbis_local_pax_read_command);
			wopt_remote_pax_read_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_read_command, "read",
						SW_E_swbis_remote_pax_read_command);
			break;
		case 181:
			wopt_no_defaults = 1;
			break;
		case 182:
			wopt_local_pax_write_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_write_command, "write",
						SW_E_swbis_local_pax_write_command);
			wopt_remote_pax_write_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_write_command, "write",
						SW_E_swbis_remote_pax_write_command);
			wopt_local_pax_read_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_read_command, "read",
						SW_E_swbis_local_pax_read_command);
			wopt_remote_pax_read_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_read_command, "read",
						SW_E_swbis_remote_pax_read_command);
			wopt_local_pax_remove_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_remove_command, "remove",
						SW_E_swbis_local_pax_remove_command);
			wopt_remote_pax_remove_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_remove_command, "remove",
						SW_E_swbis_remote_pax_remove_command);
			break;
		case 184:
			wopt_no_remote_kill = CHARTRUE;
			set_opta(opta, SW_E_swbis_no_remote_kill, CHARTRUE);
			break;
		case 185:
			wopt_no_getconf = CHARTRUE;
			set_opta(opta, SW_E_swbis_no_getconf, CHARTRUE);
			break;
		case 186:
			wopt_no_getconf = CHARFALSE;
			set_opta(opta, SW_E_swbis_no_getconf, CHARFALSE);
			break;
		case 188:
			wopt_shell_command = strdup(optarg);
			swc_check_shell_command_key(G, wopt_shell_command);
			set_opta(opta, SW_E_swbis_shell_command, wopt_shell_command);
			break;
		case 195:
			wopt_ssh_options = strdup(optarg);
			break;
		case 196:
			G->g_do_debug_events = 1;
			break;
		case 198:
			G->g_source_script_name = strdup(optarg);
			break;
		case 199:
			G->g_source_script_name = strdup(optarg);
			G->g_target_script_name = (char*)NULL;
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
			wopt_remote_pax_remove_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_remove_command, "remove",
						SW_E_swbis_remote_pax_remove_command);
			break;
		case 206:
			wopt_local_pax_remove_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_remove_command, "remove",
						SW_E_swbis_local_pax_remove_command);
			break;
		case 207:
			wopt_local_pax_remove_command = strdup(optarg);
			process_pax_command(G, wopt_local_pax_remove_command, "remove",
						SW_E_swbis_local_pax_remove_command);
			wopt_remote_pax_remove_command = strdup(optarg);
			process_pax_command(G, wopt_remote_pax_remove_command, "remove",
						SW_E_swbis_remote_pax_remove_command);
			break;
		case 208:
			wopt_allow_ambig = CHARTRUE;
			break;
		case 209:
			optionEnum = getEnumFromName("swbis_sig_level", opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			break;
		case 210:
			G->g_config_postinstallM = 1;
			break;
		case 211:
			G->g_send_envM = 1;
			break;
		case 218:
		case 219:
		case 220:
		case 222:
		case 223:
		case 224:
		case 225:
		case 226:
		case 227:
		case 228:
		case 229:
		case 236:
			optionname = getLongOptionNameFromValue(main_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			if (c == 229) {
				/* Sanity check on sig_level */
				int xres;
				swlib_atoi(optarg, &xres);
				if (xres != 0) {
					sw_e_msg(G, "bad value detected for extended option: %s\n", optionname);
					LCEXIT(1);
				}
			}
			set_opta(opta, optionEnum, optarg);
			if (swextopt_get_status()) {
				sw_e_msg(G, "bad value detected for extended option: %s\n", optionname);
				LCEXIT(1);
			}
			break;
		default:
			sw_e_msg(G, "error processing implementation extension option : %s\n",
				cplob_get_list(w_arglist)[w_option_index]);
		 	exit(1);
		break;
		}
		if (do_extension_options_via_goto == 1) {
			do_extension_options_via_goto = 0;
			goto gotoStandardOptions;
		}
	}

	optind = main_optind;

	system_defaults_files = initialize_options_files_list(NULL);

	if (opt_preview) {
		G->g_verbose_threshold = SWC_VERBOSE_1;
	}

	/* Show the options to stdout.  */

	if (swc_is_option_true(wopt_show_options_files)) { 
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

	eopt_installed_software_catalog = get_opta_isc(opta, SW_E_installed_software_catalog);

	eopt_select_local		= get_opta(opta, SW_E_select_local);
	eopt_verbose			= get_opta(opta, SW_E_verbose);
		G->g_verboseG = swlib_atoi(eopt_verbose, NULL);
	
	wopt_no_remote_kill		= swbisoption_get_opta(opta, 
							SW_E_swbis_no_remote_kill);
	wopt_no_getconf			= swbisoption_get_opta(opta, 
							SW_E_swbis_no_getconf);
	wopt_shell_command		= swbisoption_get_opta(opta, 
							SW_E_swbis_shell_command);
	eopt_enforce_scripts		= get_opta(opta, SW_E_enforce_scripts);

	remote_shell_command		= swbisoption_get_opta(opta, 
							SW_E_swbis_remote_shell_client);
	wopt_local_pax_write_command	= swbisoption_get_opta(opta, 	
					SW_E_swbis_local_pax_write_command);
	wopt_remote_pax_write_command	= swbisoption_get_opta(opta, 
					SW_E_swbis_remote_pax_write_command);
	wopt_local_pax_read_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_local_pax_read_command);
	wopt_remote_pax_read_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_remote_pax_read_command);
	wopt_local_pax_remove_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_local_pax_remove_command);
	wopt_remote_pax_remove_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_remote_pax_remove_command);
	wopt_enforce_all_signatures	= swbisoption_get_opta(opta, SW_E_swbis_enforce_all_signatures);
	wopt_sig_level			= swbisoption_get_opta(opta, SW_E_swbis_sig_level);
	wopt_forward_agent		= swbisoption_get_opta(opta, SW_E_swbis_forward_agent);
	wopt_noscripts			= swbisoption_get_opta(opta, SW_E_swbis_ignore_scripts);

	E_DEBUG("");
	if (swc_is_option_true(wopt_show_options)) { 
		E_DEBUG("");
		swextopt_writeExtendedOptions(STDOUT_FILENO, opta, SWC_U_G);
		if (G->g_verboseG > 4) {
			debug_writeBooleanExtendedOptions(STDOUT_FILENO, opta);
		}
		LCEXIT(0);
	}

	swc_set_shell_dash_s_command(G, wopt_shell_command);
	
	if (*remote_shell_command == '/') {
		remote_shell_path = remote_shell_command;
	} else {
		remote_shell_path = shcmd_find_in_path(getenv("PATH"), remote_shell_command);
	}
	
	use_no_getconf = swc_is_option_true(wopt_no_getconf);	

	/* Configure the standard I/O usages.  */

	fver = G->g_vstderr;
	fverfd = STDERR_FILENO;
	
	if (G->g_verboseG == 0 /* && opt_preview == 0 */) {
		fver = fopen("/dev/null", "r+");
		if (!fver) LCEXIT(1);
		G->g_vstderr = fver;
		fverfd = nullfd;
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
	}

	swparse_set_do_not_warn_utf();
	if (G->g_verboseG >= SWC_VERBOSE_4) {
		swparse_unset_do_not_warn_utf();
	}

	if (G->g_verboseG >= SWC_VERBOSE_7) {
		swlib_set_verbose_level(G->g_verboseG);
	}
	
	/* Set the terminal setting and ssh tty option.  */
	
	tty_opt = "-T";
	login_orig_termiosP = NULL;
	login_sizeP = NULL;
	
	/* Set the signal handlers.  */

	swgp_signal(SIGINT, safe_sig_handler);
	swgp_signal(SIGPIPE, safe_sig_handler);
	swgp_signal(SIGTERM, safe_sig_handler);
	swgp_signal(SIGUSR1, safe_sig_handler);

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *          Process the Software Selections
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	swspecs = vplob_open();

	ret = swc_process_selection_files(G, swspecs);

	if (ret == 0)
		if (argv[optind]) {
			if (*(argv[optind]) != '@') {
				/* Must be a software selection.  */
				ret = swc_process_selection_args(swspecs, argv, argc,
								&optind);
			}
		}

	if (ret) {
		/* Software selection error */
		sw_e_msg(G, "error processing selections\n");
		LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
	}

   	current_arg = swc_get_next_target(argv, argc, &optind, 
						G->g_targetfd_array, 
						"/", /* XX FIXME */
						&num_remains);

	swlex_set_input_policy(SWLEX_INPUT_UTF_TR);

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *           Loop over the targets.
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	E_DEBUG("");

	while (
		current_arg &&
		local_stdin == 0 &&
		G->g_signal_flag == 0
		)
	{
		E_DEBUG2("current_arg=[%s]", current_arg);
		distdataO = swi_distdata_create();
		/*
		swgp_signal(SIGINT, safe_sig_handler);
		swgp_signal(SIGPIPE, safe_sig_handler);
		swgp_signal(SIGTERM, safe_sig_handler);
		swgp_signal(SIGUSR1, safe_sig_handler);
		*/
		total_skipped = 0;
		statbytes = 0;
		found_ambig = 0;
		s_retval = 0;
		f_retval = 0;
		G->g_target_did_abortM = 0;
		target_tramp_list = strar_open();
		target_sshcmd[0] = shcmd_open();
		target_cmdlist = strar_open();
		target_control_message = strob_open(10);
		target_start_message = strob_open(10);
		target_catalog_message = strob_open(10);
		target_access_message = strob_open(10);
		cl_target_target = NULL;
		cl_target_selections = NULL;
		working_arg = strdup(current_arg);

		/* Parse the target sofware_selections and targets.  */

		if (target_loop_count == 0) {

			/* This branch really only checks for an invalid
			   syntax and only for the first target, it is mostly
			   just a sanity check  */

			tmp_working_arg = strdup(working_arg);
			swc_parse_soc_spec(
				tmp_working_arg, 
				&cl_target_selections, 
				&xcl_target_target
				);

			if (xcl_target_target == NULL) {
				cl_target_target = strdup("/" /* XX FIXME */);
			} else {
				cl_target_target = strdup(xcl_target_target);
			}

			if (cl_target_selections) {
				/* Selections are not supported here.  They are applied
				   globally and processed before entering the target processing
				   loop. */

				sw_e_msg(G, "software selection not valid when specified with a specific target.\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
		} else {

			/* subsequent args are targets, the same software
                           selection applies.  */
			cl_target_target = strdup(working_arg);
		}

		new_cl_target_target = swc_convert_multi_host_target_syntax(G, cl_target_target);
		free(cl_target_target);
		cl_target_target = new_cl_target_target;
		G->g_cl_target_targetM = strdup(cl_target_target);

		/* Parse the target string and set the commands to invoke */

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
				wopt_ssh_options,
				swc_is_option_true(wopt_forward_agent));
		if (G->g_target_terminal_host == NULL) {
			G->g_target_terminal_host = strdup("localhost");
		}

		SWLIB_ASSERT(target_nhops >= 0);

		/* Set the archive writing utility */
		if (pax_write_command_key == (char*)NULL) {
			if (target_nhops < 1) {
				/* Local host */
				pax_write_command_key = wopt_local_pax_write_command;
			} else {
				/* Remote host */
				pax_write_command_key = wopt_remote_pax_write_command;
			}		
		}

		/* Set the archive writing utility */
		if (pax_remove_command_key == (char*)NULL) {
			if (target_nhops < 1) {
				/* Local host */
				pax_remove_command_key = wopt_local_pax_remove_command;
			} else {
				/* Remote host */
				pax_remove_command_key = wopt_remote_pax_remove_command;
			}		
		}

		/* Do a sanity check on the target path
		   reading from stdin on a remote host is not supported.
			   Reset the the default target */

		if (xcl_target_target == NULL && target_nhops >= 1 && 
					strcmp(cl_target_target, "-") == 0) {

			/* FIXME?? silently change the target to something that makes
			   sense */
			cl_target_target = strdup(".");
		}

		/* Another sanity checks on the target path */
		target_path = swc_validate_targetpath(
					target_nhops, 
					tmpcharp, 
					"/" /* XX FIXME */, cwd, "target");

		SWLIB_ASSERT(target_path != NULL);
		E_DEBUG2("target_path is [%s]", target_path);
		
		/* More policy and sanity checks */
		apply_usage_restrictions(NULL, G, target_path);

		if (strcmp(target_path, "-") == 0 /* && target_nhops >= 1 */ ) {
			/* stdout/stdin not a valid target */
			sw_e_msg(G, "invalid target spec\n");
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}

		/* Policy and sanity checks for use of standard output */

		 /* Set the informational Event messages fd */
		local_stdout_in_use = 0;
		if (local_stdout_in_use) {
			swevent_fd = STDERR_FILENO; 
		} else {
			/* always stdout for swconfig */
			swevent_fd = STDOUT_FILENO; 
		}
		G->g_swevent_fd = swevent_fd;

		/* Establish the logger process and stderr redirection. It is
		   the logger process that fields the events from the [remote] target
		   process.  */

		if (
			target_loop_count == 0 &&      /* First Target Only */
			local_stdin == 0
		) {
			/* The same logger process is used for multiple targets */

			E_DEBUG("");
			G->g_logger_pid = swc_fork_logger(G, source_line_buf,
					target_line_buf, swc_get_stderr_fd(G),
					swevent_fd, &G->g_logspec, (int*)(NULL)/*&G->g_s_efd*/, &G->g_t_efd,
					G->g_verboseG,  &swi_event_fd );
			SWLIB_ASSERT((int)(G->g_logger_pid) > 0);

			/* Record the file descriptors of all the newly created plumbing */

			G->g_swi_event_fd = swi_event_fd;
			target_stdio_fdar[2] = G->g_t_efd;
			/* source_stdio_fdar[2] = G->g_s_efd; */
			sw_d_msg(G, "logger_pid is %d\n", G->g_logger_pid);
		}

		swlib_doif_writef(G->g_verboseG,
			SWC_VERBOSE_2 + SWCONFIG_UPVERBOSE, 
			&G->g_logspec, swevent_fd,
			"SWBIS_TARGET_BEGINS for %s\n", current_arg);

		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		pax_read_command_key = wopt_local_pax_read_command;
		pax_write_command_key = wopt_local_pax_write_command;
		pax_remove_command_key = wopt_local_pax_remove_command;

		sw_d_msg(G, "target_fork_type : %s\n", target_fork_type);

		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		/* Set the current target_path into the task script
		   protocol object */

		swicol_set_targetpath(swicol, target_path);
		swicol_set_task_debug(swicol, G->g_do_task_shell_debug);

		/* Make the target piping (which is really the source to read) */

		if (local_stdin == 0) {
			/* Remote target */

			E_DEBUG("remote target");
			if (swlib_is_sh_tainted_string(target_path)) {
				SWLIB_FATAL("tainted path");
			}

			ss_pid = swc_run_ssh_command(G,
				target_sshcmd,
				target_cmdlist,
				target_path,
				0 /*opt_preview */,
				target_nhops,
				target_fdar,
				target_stdio_fdar,
				login_orig_termiosP,
				login_sizeP, 
				&target_pump_pid, 
				target_fork_type, 
				make_master_raw, 
				(sigset_t*)ssh_fork_blockmask, 
				devel_no_fork_optimization /* reject the no fork optimization */,
							   /* It does not work with swconfig's plumbing */
				G->g_verboseG,
				&target_file_size,
				use_no_getconf,
				&is_target_local_regular_file,
				wopt_shell_command,
				swc_get_stderr_fd(G),
				&G->g_logspec);
			
			if (ss_pid < 0) {
				/* Fatal */
				sw_e_msg(G, "ssh command failed\n");
				LCEXIT(sw_exitval(G,
					target_loop_count, 
					target_success_count));
			}

			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

			if (is_target_local_regular_file) {
				/* is_target_local_regular_file  was set above */
				/* See if the local file is seekable */

				if (lseek(target_fdar[0],
						(off_t)0, SEEK_SET) != 0) {
					/* sanity check */
					sw_e_msg(G, "lseek internal error on stdio_fdar[0]\n");
					f_retval = 1;
					/* goto next target, this one failed */
					goto TARGET1;
					goto ENTER_WITH_FAILURE;
				}
			} 

			/* Record the pid */
			swc_record_pid(ss_pid, 
				G->g_pid_array, 
				&G->g_pid_array_len, 
				G->g_verboseG);

			if (target_pump_pid > 0) {
				/* This is the child of slave 
				   pty process, zero if there 
				   isn't one.  */
				swc_record_pid(target_pump_pid, 
					G->g_pid_array, 
					&G->g_pid_array_len, 
					G->g_verboseG);
			}
			
			if (isatty(target_fdar[0])) {
				if (swlib_tty_raw(target_fdar[0]) < 0) {
					sw_e_msg(G, "tty_raw error : target_fdar[0]\n");
					LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
				}
			}
			/* End of (local_stdin == 0) */
		} else {
			/* Non-remote target, i.e. the localhost
			   Here is the target piping for local stdin */
			E_DEBUG("");

			ss_pid = 0;
			target_fdar[0] = target_stdio_fdar[0];
			target_fdar[1] = target_stdio_fdar[1];
			target_fdar[2] = target_stdio_fdar[2];
		}

		E_DEBUG("");
		if (is_local_stdin_seekable  || is_target_local_regular_file) {
			E_DEBUG("");
			G->g_is_seekable = 1;
		} else {
			E_DEBUG("");
			G->g_is_seekable = 0;
		}

		/*
		 * = = = = = = = = = = = = = = = = = = = = =
		 *     Run the target script
		 * = = = = = = = = = = = = = = = = = = = = =
		 */

		/* Write the target script (to read the source). */

		/* if local_stdin == 1, then this routine returns 0 doing nothing */

		E_DEBUG("");
		target_write_pid = swc_run_source_script(G,
					1 /* don't avert a fork(3), always fork */, 
					/* more conditions for averting fork(3) */
				local_stdin || is_target_local_regular_file,
					/* The file descriptor to write to */
					target_fdar[1],
					target_path,
					target_nhops, 
					pax_write_command_key,
					fork_blockmask,
					fork_defaultmask,
					G->g_target_terminal_host,
					wopt_blocksize,
					SWC_VERBOSE_4,
					/* Here is the script to run */
					swconfig_write_source_copy_script2
					);

		/* target_write_pid will be:
			< 0 for error
			0  when local_stdin is set
		        > 0 when local_stdin is not set */

		/* Signal Check */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		sw_d_msg(G, "source write pid: %d\n", target_write_pid);


		/*
		 * = = = = = = = = = = = = = = = = = = = = =
		 *  Make sure the target script is running
		 * = = = = = = = = = = = = = = = = = = = = =
		 */

		if (target_write_pid > 0) {
			E_DEBUG("");
			sw_d_msg(G, "waiting on target script pid\n");

			/* Wait on the target script  */
			ret = swc_wait_on_script(G, target_write_pid, "target");

			if (ret != 0) {
				/* error */
				sw_e_msg(G, "write_scripts waitpid : exit value = %d\n", ret);
				/* Signal Check */
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				LCEXIT(
					sw_exitval(G, target_loop_count,	
						target_success_count));
			}
			sw_d_msg(G, "wait() on source script succeeded.\n");

			/* Read the start message and the pid */
			ret = swc_read_start_ctl_message(G,
						target_fdar[0],
						target_start_message,
						target_tramp_list,
						G->g_verboseG,
						&target_script_pid,
						"source");

			if (ret < 0) {
				/*
				 * This is (may be) the failure path for ssh authentication
				 * failure.
				 */
		
				/*
				 * See if the ssh command has finished.
				 */
				if (swlib_wait_on_pid_with_timeout(ss_pid, &tmp_status, 0 /*flags*/, 1/*verbose_level*/, 3 /* Seconds */) > 0) {
					/*
					 * Store the result in the status array.
					 */
					SWLIB_ASSERT(
						swlib_update_pid_status(ss_pid, tmp_status,
							G->g_pid_array, G->g_status_array, G->g_pid_array_len) == 0);
				}
				E_DEBUG("ABCD");
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

				/* Error, start message indicated failure */
				LC_RAISE(SIGTERM);
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
			}

			sw_d_msg(G, "start_ctl_message(loc=start): %s\n", strob_str(target_start_message));

			/*
			 * ===============================
			 * read the pwd current directory message
			 * from the target
			 * MSG_129 
			 * ===============================
			 */

			ret = swc_read_target_message_129(G, target_fdar[0], target_current_dir, G->g_verboseG, "target pwd");
			if (ret < 0) {
				sw_e_msg(G, "SW_INTERNAL_ERROR for target %s in file %s at line %d: message _129 failed\n",
						current_arg, __FILE__, __LINE__);
				goto ENTER_WITH_FAILURE;
			}

			/* Now adjust the target_path to be an absolute path
			   if it is not already */

			if (*target_path != '/') {
				target_path = swc_make_absolute_path(strob_str(target_current_dir), target_path);
				swicol_set_targetpath(swicol, target_path);
			}

			if (swlib_is_sh_tainted_string(target_path)) {
				SWLIB_FATAL("tainted path");
			}
	
			if (target_nhops >= wopt_kill_hop_threshhold) {

				/* Construct the remote kill vector if warranted  */
				E_DEBUG("");
				G->g_killcmd = NULL;
				sw_d_msg(G, "Running swc_construct_newkill_vector\n");
				if ((ret=swc_construct_newkill_vector(
					target_kill_sshcmd[0], 
					target_nhops,
					target_tramp_list, 
					target_script_pid, 
					G->g_verboseG)) < 0) 
				{
					target_kill_sshcmd[0] = NULL;
					sw_e_msg(G, "target kill command not constructed"
					"(ret=%d), maybe a shell does not "
					"have PPID.\n", ret);
				}
				G->g_target_kmd = target_kill_sshcmd[0];
			}
	
			/* Signal Check */
			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

			/* Read the leading control message from the 
			   output of the source script. */

			if ( 	local_stdin == 0 &&
				1
			) {
				/* Read the control message from the target.  */

				E_DEBUG("");

				sw_d_msg(G, "reading target script control messages\n");

				ret = swc_read_target_ctl_message(G, target_fdar[0], 
					target_control_message, G->g_verboseG, "source");

				if (ret < 0) {
					/* Fatal */
					sw_d_msg(G, "read_target_ctl_message error (loc=source_start)\n");
					main_sig_handler(SIGTERM);
					LCEXIT(sw_exitval(G,
						target_loop_count,
						target_success_count));
				}

				sw_l_msg(G, SWC_VERBOSE_5, "Got source control message [%s]\n",
					strob_str(target_control_message));
			} else {
				/* FIXME?? when is this path ever taken?? */
				/* strob_strcpy(target_control_message, ""); */
				E_DEBUG("");
				sw_d_msg(G, "No source control message expected\n");
				sw_d_msg(G, "Opps, here unexpectedly\n");
				LCEXIT(1);	
			}
			E_DEBUG("");

			/* Read the SW_SOURCE_ACCESS_BEGIN or
			   the SW_SOURCE_ACCESS_ERROR or
			   the SW_SPECIAL_MODE_BEGINS message */

			sw_d_msg(G, "reading source script access messages\n");
			
			E_DEBUG("");
			ret = swc_read_target_ctl_message(G, target_fdar[0],
				target_access_message, G->g_verboseG, "access");

			if (ret < 0) {
				E_DEBUG("");
				sw_d_msg(G, "read_target_ctl_message error"
					" (loc=target_access)\n");
				
				/* Go to the next target */
				goto ENTER_WITH_FAILURE2;
			}

			/* Analyze the source. Check the event value, Make
				sure it is SW_SOURCE_ACCESS_BEGINS */
			if (G->g_do_cleanshM == 0) {
				if ((ret=swevent_get_value(
						swevent_get_events_array(),
						strob_str(target_access_message))) !=
						SW_SOURCE_ACCESS_BEGINS) 
				{
					/* Source access error */
					E_DEBUG("");
	
					sw_e_msg(G, "source access error: ret=%d :%s\n", ret, 
						strob_str(target_access_message));
	
					/* Go to the next target */
					goto ENTER_WITH_FAILURE;
				}
			}	
			E_DEBUG("");
		} else if (target_write_pid == 0) {
			E_DEBUG("");
			/*
			 * Fork did not happen.
			 * This is an error for swconfig 
			 */
			/* This happens when local_stdin is set.  */
			main_sig_handler(SIGTERM);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
			;
		} else {
			/* Fatal error */
			E_DEBUG("");
			sw_e_msg(G, "fatal internal error. fork error.\n");
			main_sig_handler(SIGTERM);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}

		/*
		 * = = = = = = = = = = = = = = = = = = = = =
		 *     Selection Phase Begins Here
		 * = = = = = = = = = = = = = = = = = = = = =
		 */

		G->g_swicolM = swicol;
		G->g_running_signalusr1M = 0;
		G->g_running_signalsetM = 0;

		E_DEBUG2("target_ctl_message: [%s]", strob_str(target_control_message));
		isc_script_buf = NULL;
		if ( target_write_pid > 0 &&
		     strob_strstr(target_control_message, SWBIS_SOURCE_CTL_ARCHIVE)
		) {
			/* The target is a tarball
			   This is an error or a no-op for swconfig */
			E_DEBUG("");
			abort_script(swicol, G, target_fdar[1]);
        		swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
			f_retval = 1;
			E_DEBUG("");
		} else if ( target_write_pid > 0 &&
			strob_strstr(target_control_message, SWBIS_SOURCE_CTL_CLEANSH) != NULL
		) {
			E_DEBUG("do cleansh");
			;
		} else if ( target_write_pid > 0 &&
			strob_strstr(target_control_message, SWBIS_SOURCE_CTL_DIRECTORY) != NULL
		) {
			/*
			 * Normal case for swconfig
			 */

			E_DEBUG("");
			apply_usage_restrictions("dir", G, "");
			isc_script_buf = strob_open(10);

			/* 
			 * Get the uts attributes from the target host
			 * SWBIS_TS_uts task shell
			 */
			ret = swpl_get_utsname_attributes(G, swicol, uts, target_fdar[1], G->g_swi_event_fd);
			if (ret != 0) {
				abort_script(swicol, G, target_fdar[1]);
        			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
				f_retval++;
			}

			/* Send a gratuitous task shell that does nothing
			   successfully, this is here as a test only. */

			ret = 0;
			if (f_retval == 0)
				ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": guarding 123");
			if (ret != 0) {
				abort_script(swicol, G, target_fdar[1]);
        			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
				f_retval++;
				raise(SIGINT);
			}

			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

			E_DEBUG("");
			if (f_retval == 0) {
				response_img = swpl_get_samepackage_query_response(G, swicol, target_path,
					target_fdar[0], target_fdar[1], opta, swspecs,
					&f_retval, 0 /* make_dummy_response */);
				if (response_img == NULL) {
					abort_script(swicol, G, target_fdar[1]);
       		 			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
					f_retval++;
				}
			} else {
				raise(SIGINT);
				response_img = NULL;
			}

			E_DEBUG("");
			
			if (f_retval == 0)
				 swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": guarding 1234");

			/* Now parse the response */
			sl = NULL;		
			req = swpl_analyze_samepackage_query_response(G, response_img, &sl);
			if (req == NULL) {
				raise(SIGINT);
			}

			/* Now make judgements about the selections according to the
			   Selection Phase spec requirements */

			E_DEBUG("");
			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
			if (G->g_do_debug_events)
				swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

			i = 0;

			/*
			 * --------------------------------------------
			 * Now loop over the selections
			 * --------------------------------------------
			 */

			E_DEBUG("");
			G->in_shls_looperM = 1;

			/* loop over the queries, which are actually the whitespace delimited
			   software selections from the command line */

			if (! does_have_sw_selections(swspecs)) {
				E_DEBUG("No selections given on command line");
				/* per spec, If no selections are given, do nothing */
				sw_e_msg(G, "SW_SELECTION_NOT_FOUND for nil (empty) selection list\n");
				s_retval ++;
			}

			while (
				/* s_retval == 0 && */
				f_retval == 0 &&
				(sc=vplob_val(sl->scM, i++)) != NULL &&
				G->g_signal_flag == 0 &&
				1
			) {
				j = 0;
				E_DEBUG("");
				nmatches = sc_selections_check(G, opta, sc);
				if (nmatches == 0) {
					strob_sprintf(btmp2, 0,
						"SW_SELECTION_NOT_FOUND on %s: %s: status=1\n",
							cl_target_target, sc->sqM->swspec_stringM);
					sw_e_msg(G, strob_str(btmp2));
					s_retval++;
					continue;
				} 

				/* Loop over the responses to a query */

				while(
					f_retval == 0 &&
					(sr=vplob_val(sc->srM, j++)) &&
					G->g_signal_flag == 0 &&
					1
				) {
					critical_region_begin();
					E_DEBUG("");
					catalog_entry_directory = swicat_sr_form_catalog_path(sr,
									eopt_installed_software_catalog, NULL);

					E_DEBUG2("catalog_entry_directory=%s", catalog_entry_directory);
					if (nmatches > 1 && swc_is_option_true(wopt_allow_ambig) == 0) {
						/* don't allow ambiguaous selections (e.g. wildcards) unless
						   specifically allowed via the --allow-ambig option */
						swicat_sr_form_swspec(sr, btmp2);
						sw_e_msg(G, "SW_SELECTION_NOT_FOUND_AMBIG: %s: status=1\n",
							strob_str(btmp2));
						found_ambig = 1;
						s_retval++;
						continue;
					}
					rp_status = 1;
					E_DEBUG2("signal_flag=%d", G->g_signal_flag);
					E_DEBUG2("SWEVERID: %s", swverid_show_object_debug(sr->swspecM, NULL, "sr->swspecM:"));
					E_DEBUG2("SWEVERID LOCATION: %s", swverid_get_verid_value(sr->swspecM, SWVERID_VERIDS_LOCATION, 1));

					did_skip = 0;
					ret = swconfig_looper_sr_payload(G, target_path, swicol,
								sr,
								target_fdar[1],
								target_fdar[0],
								&rp_status,
								uts, &did_skip);

					if (did_skip) total_skipped++;

					/* rp_status is the script return status
					   ret, if non-zero is a protocol or internal error */

					E_DEBUG2("signal_flag=%d", G->g_signal_flag);
					E_DEBUG2("swconfig_looper_sr_payload returned %d", ret);
					if (rp_status != 0) {
						/* error, but continuation possible */
						/*
						sw_e_msg(G, "SW_INTERNAL_ERROR: %s:%d status=%d\n",
							__FILE__, __LINE__, rp_status);
						*/
						s_retval++;
					}
					E_DEBUG2("signal_flag=%d", G->g_signal_flag);
					if (ret < 0) {
						/* error that is incompatible with continuing */
						/* raise(SIGINT); */
						f_retval++;
						break;
					}
					E_DEBUG2("signal_flag=%d", G->g_signal_flag);
					critical_region_end();
					swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				}  /* loop over responses */
				E_DEBUG("");
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				if (swicol_get_master_alarm_status(swicol)) {
					break;
				}
				E_DEBUG("");
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
			} /* loop over software selections */
			
			E_DEBUG2("f_retval=%d", f_retval);

			/* this terminates the shls_looper() loop */
			ret = swicol_send_loop_trailer(swicol, target_fdar[1]);
			G->in_shls_looperM = 0;

			E_DEBUG("");
			if (ret != 0) {
				E_DEBUG("");
				abort_script(swicol, G, target_fdar[1]);
				E_DEBUG("");
        			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
				E_DEBUG("");
				f_retval++;
			}
			E_DEBUG("");

			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

			E_DEBUG("");
			ret = 0;
			if (1 || f_retval == 0) {
				/* this need to be executed even if the looper exited with error */
				ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": guarding 89");

			}
			E_DEBUG("");
			if (ret != 0) {
				abort_script(swicol, G, target_fdar[1]);
        			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
				f_retval++;
			}

			E_DEBUG("");
			if (s_retval) f_retval ++;

			if (sl)
				swicat_sl_delete(sl);
		} else if ( target_write_pid > 0 ) {
			/* 
			 * unexpected internal error
			 */
			E_DEBUG("");
			sw_e_msg(G, "internal error at line %d\n", __LINE__);
			abort_script(swicol, G, target_fdar[1]);
        		swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
			f_retval = 1;
		} else {
			/*
			 * local stdin, error for swconfig
			 */
			E_DEBUG("");
			swgp_signal(SIGINT, main_sig_handler);
			sw_e_msg(G, "operation on stdin not supported\n");
			abort_script(swicol, G, target_fdar[1]);
        		swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
			f_retval = 1;
		}
		E_DEBUG("");
		G->g_swicolM = NULL;

		if (isc_script_buf) {
			strob_close(isc_script_buf);
			isc_script_buf = NULL;
		}

		/* Signal Check */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		E_DEBUG("");

		if (G->g_verboseG >= SWC_VERBOSE_8) {
			sw_d_msg(G, "target_fdar[0] = %d\n", target_fdar[0]);
			sw_d_msg(G, "target_fdar[1] = %d\n", target_fdar[1]);
			sw_d_msg(G, "target_fdar[2] = %d\n", target_fdar[2]);
		}
			
		E_DEBUG("");
		swpl_tty_raw_ctl(G, 2);

		E_DEBUG("");
		if (strstr(strob_str(target_control_message), 
			"SW_SOURCE_ACCESS_ERROR :") != (char*)NULL) {
			E_DEBUG("");
			/* Error */	
			sw_e_msg(G, "SW_SOURCE_ACCESS_ERROR : []\n"
				);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G,
				target_loop_count, 
				target_success_count));
		}
		E_DEBUG("");

		/* Signal Check */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

ENTER_WITH_FAILURE:
		E_DEBUG("");
		swicol_rpsh_wait_for_event(swicol, (STROB *)NULL, G->g_swi_event_fd, SWI_MAIN_SCRIPT_ENDS);
ENTER_WITH_FAILURE2:

		E_DEBUG("");
		if (kill_sshcmd[0]) {
			shcmd_close(kill_sshcmd[0]);
			kill_sshcmd[0] = (SHCMD*)NULL;
		}

		if (target_kill_sshcmd[0]) {
			shcmd_close(target_kill_sshcmd[0]);
			target_kill_sshcmd[0] = (SHCMD*)NULL;
		}

		/* Now close down  */
			
		E_DEBUG("");

		/* Signal Check */
		E_DEBUG("");
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		close(target_fdar[0]);
		if (target_fdar[1] != STDOUT_FILENO) 
			close(target_fdar[1]);

		if (G->g_signal_flag) {
			swlib_kill_all_pids(G->g_pid_array +
				SWC_PID_ARRAY_LEN, 
				G->g_pid_array_len, 
				SIGTERM, 
				G->g_verboseG);
		}
		E_DEBUG("");
		swlib_wait_on_all_pids_with_timeout(
				G->g_pid_array, 
				G->g_pid_array_len, 
				G->g_status_array, WNOHANG /*waitpid flags*/, 
				G->g_verboseG - 2, -3 /* after 3 seconds, kill*/);
		
		swlib_wait_on_all_pids_with_timeout(
				G->g_pid_array, 
				G->g_pid_array_len, 
				G->g_status_array, 0 /*waitpid flags*/, 
				G->g_verboseG - 2, 0);

		E_DEBUG("");

		ret = swc_analyze_status_array(G->g_pid_array,
					G->g_pid_array_len, 
					G->g_status_array,
					G->g_verboseG - 2);
		if (f_retval == 0 && ret ) f_retval++;

		/* Now re-Initialize to prepare for the next target. */

		E_DEBUG("");
		G->g_pid_array_len = 0;

		if (tmp_working_arg) free(tmp_working_arg);
		tmp_working_arg = NULL;

		if (working_arg) free(working_arg);
		working_arg = NULL;

		if (cl_target_target) free(cl_target_target);
		cl_target_target = NULL;
		
		if (cl_target_selections) free(cl_target_selections);
		cl_target_selections = NULL;

		if (target_path) free(target_path);
		target_path = NULL;

TARGET1:
		E_DEBUG("");
		G->g_pid_array_len = 0;

		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		strar_close(target_tramp_list);
		shcmd_close(target_sshcmd[0]);
		strar_close(target_cmdlist);
		strob_close(target_control_message);
		strob_close(target_start_message);
		strob_close(target_catalog_message);
		strob_close(target_access_message);
		strob_close(target_current_dir);

		if (target_script_pid) free(target_script_pid);
		target_script_pid = NULL;

		target_loop_count++;
		if (f_retval == 0 && total_skipped == 0) target_success_count++;
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_2 + SWCONFIG_UPVERBOSE,
				&G->g_logspec, f_retval ? swc_get_stderr_fd(G) : swevent_fd,
			"SWBIS_TARGET_ENDS for %s: status=%d\n",
						current_arg,
						f_retval);
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		free(current_arg);
   		current_arg = swc_get_next_target(argv, 
						argc, 
						&optind, 
						G->g_targetfd_array,
						"/" /* XX FIXME */, &num_remains);
		swi_distdata_delete(distdataO);
		if (found_ambig)
		sw_e_msg(G, "try --allow-ambig to operate on multiple selections\n");
	} /* target loop */
	E_DEBUG("");

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, 
			&G->g_logspec, swc_get_stderr_fd(G), "Finished processing targets\n");
	E_DEBUG("");
	if (targetlist_fd >= 0) close(targetlist_fd);
	E_DEBUG("");
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, &G->g_logspec, swc_get_stderr_fd(G), 
		"exiting at %s:%d with value %d\n",
		__FILE__, __LINE__,
		sw_exitval(G, target_loop_count, target_success_count));

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_6, 
			&G->g_logspec, swc_get_stderr_fd(G), "SWI_NORMAL_EXIT: status=%d\n",
		sw_exitval(G, target_loop_count, target_success_count));

	E_DEBUG("");
	swutil_close(swlog);
	E_DEBUG("");
	close(G->g_swevent_fd);
	close(G->g_swi_event_fd);
	f_retval = swc_shutdown_logger(G, 0);

	E_DEBUG("");
	if (f_retval) return 1;
	if (reqs_failed) return 3;
	return(sw_exitval(G, target_loop_count, target_success_count));	
}
