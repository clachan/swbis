/*  swlist.c -- The swbis catalog listing utility.

 Copyright (C) 2006,2007,2010 Jim Lowe
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
#include "swutillib.h"
#include "swmain.h"
#include "globalblob.h"
#include "swproglib.h"
#include "swlist_lib.h"
#include "swlist.h"
#include "swverify_lib.h"

#define SWLIST_UPVERBOSE	1
#define PROGNAME                "swlist"
#define SW_UTILNAME             PROGNAME
#define SWPROG_VERSION          SWLIST_VERSION
#define WOPT_LAST		238   /* Highest option number */

static char progName[] = PROGNAME;
static char * CHARTRUE = "True";
static char * CHARFALSE = "False";

static GB g_blob;   /* Global blob of utility independent stuff */
static GB * G;      /* pointer to Global blob */

static BLIST g_blist;  /* Global blob of swlist specific option settings */
static BLIST * BL;      /* Pointer to (BLIST)(g_blist)  */

static int n_msg_SIGPIPE = 0;

struct blist_level blist_levels_i[] = {
	{
		0, /* is set*/  
		SW_LEVEL_V_FILE,  /* must be 0 */
		SW_A_file
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_CONTROL_FILE, /* 1 */
		SW_A_control_file
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_FILESET, /* 2 */
		SW_A_fileset
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_SUBPRODUCT, /* 3 */
		SW_A_subproduct
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_PRODUCT, /* 4 */
		SW_A_product
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_BUNDLE, /* 5 */
		SW_A_bundle
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_DISTRIBUTION, /* 6 */
		SW_A_distribution
	},
	{
		0, /* is set*/  
		SW_LEVEL_V_HOST, /* 7 */
		SW_A_host
	},
	{
		0,
		0,
		(char*)NULL
	}
};

static
void 
version_string(FILE * fp)
{
        fprintf(fp,   
"%s (swbis) version " SWLIST_VERSION "\n",
                progName);
}

static
void 
version_info(FILE * fp)
{
	version_string(fp);
	fprintf(fp,  "%s",
"Copyright (C) 2007,2008,2010 Jim Lowe\n"
"Portions are copyright 1985-2000 Free Software Foundation, Inc.\n"
"This software is distributed under the terms of the GNU General Public License\n"
"and comes with NO WARRANTY to the extent permitted by law.\n"
"See the file named COPYING for details.\n");

}

static
void
write_attr(char * name, char * value, int index_format)
{
	if (! value) value = "\"\"";
	if (index_format == 0)
		fprintf(stdout, "%s\n", value);
	else
		fprintf(stdout, "    %s %s\n", name, value);
}

static
int 
usage(FILE * fp, 
		char * pgm, 
		struct option  lop[], 
		struct ugetopt_option_desc helpdesc[])
{
	
fprintf(fp, "%s",
"swlist lists information in the software catalog\n"
);
	
	fprintf(fp, "%s",
"\n"
"Usage: swlist [-d|-r] [-v] [-a attribute] [-c catalog] [-f file] [-l level]\\\n"
"[-t targetfile] [-x option=value] [-X options_file] [-W option[=value]]\\\n"
"      [software_selections][@target]\n"
);

fprintf(fp, "%s",
"\n"
" Options:\n"
);
	ugetopt_print_help(fp, pgm, lop, helpdesc);

fprintf(fp , "%s",

"\n"
" Basic Mode Control:\n"
"\n"
"      --products   list installed products\n"
"      --files      list files as known from the the installed catalog\n" 
"      --system     list files as installed in the file system\n" 
"      --directory  list directories in the installed catalog\n" 
"\n"
" Examples:\n"
"    swlist --show-options\n"
"    swlist --show-options-files\n"
"    swlist --files @-  # list the payload of a package on stdin\n"
"    swlist --files foo @ 192.168.1.2 # List installed files in the foo package\n"
"    swlist --products @ 192.168.1.2 # List installed products\n"
"    swlist -c - @/ | tar tvf - # write tar archive of installed catalog\n"
"    swlist -x one-liner=products emacs\\*,r>20 @/ # List products matching\n"
"                                                 #  emacs\\*,r>20\n"
"    swlist -x one-liner=files @- <foo.tar.gz  # List payload files\n"
"    swlist -x verbose=2 -x one-liner=files @- <foo.tar.gz  # List payload files verbosely\n"

"\n"
" Implementation Extension Options:\n"
"    Syntax : -W option[=option_argument] [-W ...]\n"
"        or   -W option[=option_argument],option...\n"
"        or   --option[=option_argument]\n"
"\n"
"      --show-options      Show the extended options and exit\n"
"      --show-options-files   Show the options files that will be parsed.\n"
"\n"
"  Operational Options:\n"
"     --force-locks  ignore locking, and remove existing lock.\n"
"     --sig-level=N  N=0,1, or 2,.. number of sigs required\n"
"     --noop              Legal option with no effect\n"
"     --no-defaults-file        Do not read any defaults files.\n"
"\n"
"  Target and Remote connection options\n"
"\n"
"      --remote-shell=NAME  NAME is the client \"rsh\" or \"ssh\"(default).\n"
"      --no-remote-kill     Do not execute a kill command. Normally a kill\n"
"                        command is issued when signals TERM, PIPE, and INT\n"
"                        are received for targets where nhops > 1.\n"
"      --no-getconf      Don't use the getconf utility to set the PATH variable\n"
"                        on the target host.\n"
"      --use-getconf     This causes getconf to be used.\n"
"      --shell-command={detect|posix|sh|bash|ksh}  detect is the default\n"
"      --ssh-options=options      single letter ssh options. e.g. --ssh-options=1\n"
"   -A,--A,--enable-ssh-agent-forwarding\n"
"      --a,--disable-ssh-agent-forwarding\n"
"\n"
" Debugging Controls:\n"
"\n"
"      --debug-events             show the internal events listing to stderr\n"
"      --source-script-name=NAME  write the internal stdin script to NAME.\n"
"      --target-script-name=NAME  write the internal stdin script to NAME.\n"
"      --debug-task-scripts       write each task shell to a file in /tmp\n"
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
"            or : -W \"option=option_argument  ...\"\n"
"   distribution_target_directory   = -	     # Stdin\n"
"   installed_software_catalog  = var/lib/swbis/catalog\n"
"   one_liner                   = files|products\n"
"   select_local		= false    # Not Implemented\n"
"   verbose			= 1\n"
"\n"
" Swbis Extended Options:\n"
"        File : <libdir>/swbis/swbisdefaults\n"
"               ~/.swbis/swbisdefaults\n"
"        Command Line Syntax : -x option=option_argument\n"
"  swlist.swbis_no_getconf		 = true # getconf is used\n"
"  swlist.swbis_shell_command	         = detect\n"
"  swlist.swbis_no_remote_kill	         = false\n"
"  swlist.swbis_any_format               = false\n"
"  swlist.swbis_forward_agent            = true\n"
"\n"
);

	fprintf(fp , "%s", "\n");
	version_string(fp);
	fprintf(fp , "\n%s", 
        "Report bugs to " REPORT_BUGS "\n");
	return 0;
}

void
critical_region_begin(void)
{
	swgp_signal_block(SIGINT, (sigset_t *)NULL);
}

void
critical_region_end(void)
{
	swgp_signal_unblock(SIGINT, (sigset_t *)NULL);
}

static
int
show_all_sw_selections(VPLOB * swspecs, int fd)
{
	int i;
	SWVERID * swverid;
	STROB * tmp;
	STROB * buf;
	tmp = strob_open(1);
	buf = strob_open(1);
	i=0;
	while ((swverid=vplob_val(swspecs, i++)) != NULL) {
		swlib_writef(fd, buf, "%s\n", swverid_print(swverid, tmp));
	}
	strob_close(tmp);
	strob_close(buf);
	return i-1;
}

static
void
set_pgm_mode(GB * G, BLIST * BL, char * pgm_mode, char * one_liner, int opt_index_file_format)
{
	E_DEBUG2("pgm_mode is %s", pgm_mode);
	E_DEBUG2("one_liner is %s", one_liner);
	E_DEBUG2("opt_index_file_format is %d", opt_index_file_format);
	if (
		opt_index_file_format
	) {
		strcpy(pgm_mode, SWLIST_PMODE_INDEX);
		E_DEBUG2("pgm_mode is %s\n", pgm_mode);
		return;
	}
	if (BL->catalogM != NULL) {
		strcpy(pgm_mode, SWLIST_PMODE_CAT);
		E_DEBUG2("pgm_mode is %s\n", pgm_mode);
		return;
	}

	if (strlen(pgm_mode) > 0) {
		/* already set by option */
		E_DEBUG2("pgm_mode is [already] %s\n", pgm_mode);
		return;
	}
	if (strstr(one_liner, SWLIST_PMODE_PROD)) {
		strcpy(pgm_mode, SWLIST_PMODE_PROD);
	} else if (strstr(one_liner, SWLIST_PMODE_FILE)) {
		strcpy(pgm_mode, SWLIST_PMODE_FILE);
	} else if (strstr(one_liner, SWLIST_PMODE_DIR)) {
		strcpy(pgm_mode, SWLIST_PMODE_DIR);
	} else {
		strcpy(pgm_mode, SWLIST_PMODE_PROD);
	}
	E_DEBUG2("pgm_mode is %s", pgm_mode);
}

static
int
is_index_file_format(char * pgm_mode, int opt_alt_verbose, int explicitly_set)
{
	/* The -v option has overloaded meaning */

	/* determine the meaning of a non-zero value of 
	   opt_alt_verbose, it can either mean "-x verbose=N" or
	   it can mean "INDEX file format" per POSIX spec. */

	if (opt_alt_verbose == 0)
		return 0;

	if (
		(
		strcmp(pgm_mode, SWLIST_PMODE_PROD) == 0 ||
		strcmp(pgm_mode, SWLIST_PMODE_DIR) == 0 ||
		strcmp(pgm_mode, SWLIST_PMODE_FILE) == 0 ||
		strcmp(pgm_mode, SWLIST_PMODE_CAT) == 0 ||
		strcmp(pgm_mode, SWLIST_PMODE_DEP1) == 0 ||
		strcmp(pgm_mode, SWLIST_PMODE_TEST1) == 0 ||
		0
		) && explicitly_set 
	) {
		/* opt_alt_verbose if set, means verbose */	
		return 0;
	} else {
		if (opt_alt_verbose)
			/* means INDEX file format */	
			return 1;
		else
			return 0;
	}
}

static
void
abort_script(SWICOL * swicol, GB * G, int fd)
{
	if (G->g_target_did_abortM)
		return;
        swicol_set_master_alarm(swicol);
	if (G->in_shls_looperM) {
		; /* not applicable in swremove */
	}
	sw_e_msg(G, "Executing abort_script() ... \n");
	G->g_target_did_abortM = 1;
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
			sw_e_msg(G, "Caught SIGTERM\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGINT:
			if (G->g_do_progressmeter) alarm(0);
			sw_e_msg(G, "Caught SIGINT\n");
			G->g_signal_flag = signum;
			if (G->g_swicolM) swicol_set_master_alarm(G->g_swicolM);
			break;
		case SIGPIPE:
			if (G->g_do_progressmeter) alarm(0);
			if (n_msg_SIGPIPE == 0) {
				sw_e_msg(G, "Caught SIGPIPE\n");
				n_msg_SIGPIPE++;
			}
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
			sw_e_msg(G, "Executing handler for signal %d.\n", signum);
			swgp_signal_block(SIGTERM, (sigset_t *)NULL);
			swgp_signal_block(SIGINT, (sigset_t *)NULL);
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
			}
		case SIGUSR1:
			G->g_signal_flag = 0;
			if (G->g_do_progressmeter) alarm(0);
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
int
apply_usage_restrictions(char * source_type, GB * G,
			char * current_arg)
{
	int val;

	val = 0;
	if (source_type) {
		E_DEBUG2("source_type=%s", source_type);
		;
	}
	if (source_type && strstr(source_type, "dir")) {
		E_DEBUG("");
		if (G->g_do_distribution == 1) {
			E_DEBUG("");
			val = 1;
		}
	} else if (source_type && strstr(source_type, "archive")) {
		E_DEBUG("");
		if (G->g_do_installed_software == 1) {
			E_DEBUG("");
			val = 1;
		}
	} else {
		/* Check the current arg */
		E_DEBUG2("current_arg=[%s]", current_arg);
		if (
			( strcmp(current_arg, "-") == 0 &&
			G->g_do_installed_software == 1
			) ||
			( strcmp(current_arg, "/") == 0 &&
			G->g_do_distribution == 1
			)
		) {
			E_DEBUG("");
			val = 1;
		}
	}

	if (val) {
		sw_e_msg(G, "invalid usage or specified target type is unsupported\n");
		/* main_sig_handler(SIGTERM);
		swc_shutdown_logger(G, SIGABRT);
		*/
		/* LCEXIT(1); */
	}

	return val;
}

static
int
process_level_option(BLIST * BL, char * level_name)
{
	struct blist_level * list;
	list = blist_levels_i;
	while((*list).lv_nameM) {
		if (strcmp((*list).lv_nameM, level_name) == 0 ) {
			(*list).lv_is_setM = 1;
			if (strcmp(level_name, SW_A_host))
				BL->level_is_setM ++;
			return 0;
		}
		list++;
	}
	return 1;
}

/* Main Program */

int
main(int argc, char **argv)
{
	int c;
	int ret;
	int uret;
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
	int ism;
	int no_of_selections_processed;
	int jsm;
	pid_t ss_pid = 0;
	pid_t source_write_pid = 0;
	pid_t target_pump_pid = 0;
	int is_target_local_regular_file = 0;
	char * optionname = NULL;
	SWI_DISTDATA * distdataO;

	int opt_level_is_set = 0;
	int opt_alt_verbose = 0;
	int opt_index_file_format = 0;  /* -v option */
	char * attr;
	int attr_index;
	char * opt_catalog_dest_dir = (char *)NULL;   /* -c option */
	char * opt_option_files = (char*)NULL;

	char * wopt_shell_command = (char*)(NULL);
	char * wopt_any_format = (char*)(NULL);
	int    wopt_no_defaults = 0;
	int    wopt_kill_hop_threshhold = 2;  /* 2 */
	char * wopt_no_getconf = (char*)NULL;
	char * wopt_forward_agent = (char*)CHARTRUE;
	char * wopt_no_remote_kill = (char*)NULL;
	char * wopt_local_pax_write_command = (char*)NULL;
	char * wopt_remote_pax_write_command = (char*)NULL;
	char * wopt_local_pax_read_command = (char*)NULL;
	char * wopt_remote_pax_read_command = (char*)NULL;
	char * wopt_ssh_options = (char*)NULL;
	int    wopt_with_hop_pids = 1;  /* internal test control */
	char * wopt_show_options = NULL;
	char * wopt_show_options_files = NULL;
	char * wopt_blocksize = "5120";
	char * wopt_sig_level;
	char * wopt_enforce_all_signatures;
	char wopt_pgm_mode[30];
	char wopt_pgm_mode_explicitly_set = 0;
	int wopt_file_sys = 0;

	char * eopt_distribution_target_directory;
	char * eopt_installed_software_catalog;
	char * eopt_one_liner;
	char * eopt_select_local;
	char * eopt_verbose;
	char * response_img;

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
	int f_retval = 0;
	int s_retval = 0;
	int rp_status;
	int target_file_size;
	unsigned long int statbytes;
	int target_array_index = 0;
	int select_array_index = 0;
	int stdin_in_use = 0;
	int use_no_getconf;
	int swevent_fd = STDOUT_FILENO;
	int local_stdout_in_use = 0;
	int reqs_failed = 0;
	int nmatches;
	int uts_attr_mode;
	int num_cmd_line_swspecs;
	struct extendedOptions * opta = optionsArray;
	struct termios	*login_orig_termiosP = NULL;
	struct winsize	*login_sizeP = NULL;
	
	char * xcmd;
	char * tty_opt = NULL;
	char * target_fork_type;
	char * target_script_pid = (char*)NULL;
	char * tmpcharp = (char*)NULL;
	char * exoption_arg;
	char * exoption_value;
	char * catalog_entry_directory;
	char * cl_target_target = (char*)NULL;
	char * new_cl_target_target = (char*)NULL;
	char * xcl_target_target = (char*)NULL;
	char * cl_target_selections = (char*)NULL;
	char * target_path = (char*)NULL;

	char * pax_read_command_key = (char*)NULL;
	char * pax_write_command_key = (char*)NULL;
	char * tmp_working_arg = (char*)NULL;
	char * working_arg = (char*)NULL;
	char * current_arg = (char*)NULL;
	char * system_defaults_files = (char*)NULL;
	char cwd[1024];
	char * remote_shell_command = REMOTE_SSH_BIN;
	char * remote_shell_path;
	char * current_opt_level = NULL;

	STRAR * target_cmdlist;
	STRAR * target_tramp_list;
	VPLOB * swspecs;
	VPLOB * pre_reqs;
	VPLOB * ex_reqs;
	STROB * tmp;
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
	CPLOB * w_arglist;
	CPLOB * e_arglist;
	SHCMD * target_sshcmd[2];
	SHCMD * source_sshcmd[2];
	SHCMD * kill_sshcmd[2];
	SHCMD * target_kill_sshcmd[2];
	FILE * fver;
	sigset_t * fork_blockmask;
	sigset_t * fork_defaultmask;
	sigset_t * currentmask;
	SWVERID * swverid;
	SWLOG * swlog;
	SWICOL * swicol;
	SWUTS * uts;
	SWICAT_SL * sl;
	SWICAT_REQ * req;
	SWICAT_SC * sc;
	SWICAT_SR * sr;

	struct option main_long_options[] =
             {
		{"attribute", 1, 0, 'a'},
		{"catalog", 1, 0, 'c'},
		{"distribution", 0, 0, 'd'},
		{"selections-file", 1, 0, 'f'},
		{"level", 1, 0, 'l'},
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
		{"any-format", 0, 0, 200},
		{"swi-debug-name", 1, 0, 201},
		{"debug-task-scripts", 0, 0, 202},
		{"enable-ssh-agent-forwarding", 0, 0, 203},
		{"A", 0, 0, 203},
		{"disable-ssh-agent-forwarding", 0, 0, 204},
		{"a", 0, 0, 204},
		{"products", 0, 0, 205},
		{"files", 0, 0, 206},
		{"test1", 0, 0, 207},
		{"deps", 0, 0, 208}, 
		{"dependencies", 0, 0, 208}, 
		{"prerequisite", 1, 0, 209},
		{"exrequisite", 1, 0, 210}, 
		{"directory-form", 0, 0, 211},
		{"sig-level", 1, 0, 212},
		{"system", 0, 0, 213},
		{"swbis_no_getconf", 		1, 0, 218},
		{"swbis-no_getconf", 		1, 0, 218},
		{"swbis_shell_command", 	1, 0, 219},
		{"swbis-shell_command", 	1, 0, 219},
		{"swbis_no_remote_kill", 	1, 0, 220},
		{"swbis-no-remote-kill", 	1, 0, 220},
		{"swbis_local_pax_write_command", 1, 0, 222},
		{"swbis-local-pax-write_command", 1, 0, 222},
		{"swbis_remote_pax_write_command", 1, 0, 223},
		{"swbis-remote-pax-write_command", 1, 0, 223},
		{"swbis_local_pax_read_command", 1, 0, 224},
		{"swbis-local-pax-read-command", 1, 0, 224},
		{"swbis_remote_pax_read_command", 1, 0, 225},
		{"swbis-remote-pax-read-command", 1, 0, 225},
		{"swbis_forward_agent", 	1, 0, 236},
		{"swbis-forward-agent", 	1, 0, 236},
		{"swbis_sig_level", 1, 0, 237},
		{"swbis-sig-level", 1, 0, 237},
		{"swbis_enforce_all_signatures", 1, 0, 238},
		{"swbis-enforce-all-signatures", 1, 0, 238},
		{0, 0, 0, 0}
             };

	struct option posix_extended_long_options[] =
             {
		{"verbose", 			1, 0, 216},
		{"installed_software_catalog",	1, 0, 217},
		{"one_liner",		 	1, 0, 221},
		{"distribution_target_directory", 1, 0, 232},
               {0, 0, 0, 0}
             };
	
	struct option std_long_options[] =
             {
		{"attribute", 1, 0, 'a'},
		{"catalog", 1, 0, 'c'},
		{"distribution", 0, 0, 'd'},
		{"selections-file", 1, 0, 'f'},
		{"level", 1, 0, 'l'},
		{"installed-software", 0, 0, 'r'},
		{"target-file", 1, 0, 't'},
		{"extended-option", 1, 0, 'x'},
		{"defaults-file", 1, 0, 'X'},
		{"verbose", 0, 0, 'V'},
		{"extension-option", 1, 0, 'W'},
		{"index-format", 0, 0, 'v'},
		{"help", 0, 0, '\012'},
               {0, 0, 0, 0}
             };
       
	struct ugetopt_option_desc main_help_desc[] =
             {
	{"", "", "specifies which attributes to list"},
	{"", "DIR", "export the catalog to stdout, use - for stdout"},
	{"", "", "causes swlist to operate on a distribution"},
	{"", "FILE","Take software selections from FILE."},
	{"", "LEVEL", "specify the software level to list\n"},
	{"", "", "causes swlist to operate on installed software\n"
	"          located at an alternate root."},
	{"", "FILE", "Specify a FILE containing a list of targets." },
	{"", "option=value", "Specify posix extended option."},
	{"", "FILE[ FILE2 ...]", "Specify files that override \n"
	"        system option defaults. Specify empty string to disable \n"
	"        option file reading."
	},
	{"", "", "increment verbose level"},
	{"", "optionarg", "Specify implementation extension option."},
	{"", "", "List in INDEX file format"},
	{"help", "", "Show this help to stdout (impl. extension)."},
	{0, 0, 0}
	};

	/*  End of Decalarations.  */
	
	strcpy(wopt_pgm_mode, "");
	G = &g_blob;
	gb_init(G);
	G->g_target_fdar = target_fdar;
	G->g_source_fdar = source_fdar;
	G->g_save_fdar = save_stdio_fdar;
	G->g_verbose_threshold = SWC_VERBOSE_1;
	G->optaM = opta;

	uxfio_devnull_open("/dev/null", O_RDWR, 0);  /* initiallize the null fd */

	BL = &g_blist;
	blist_init(BL);
	BL->levelsM = blist_levels_i;

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
	sigemptyset(currentmask);
	sigemptyset(fork_blockmask);
	sigaddset(fork_blockmask, SIGALRM);
	sigemptyset(fork_defaultmask);
	sigaddset(fork_defaultmask, SIGINT);
	sigaddset(fork_defaultmask, SIGPIPE);
	
	source_line_buf = strob_open(10);
	target_line_buf = strob_open(10);
	target_current_dir = strob_open(10);

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

	tmp = strob_open(10);		/* General use String object. */
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

	eopt_distribution_target_directory = "/";
	eopt_installed_software_catalog = "var/lib/swbis/catalog/";

	eopt_one_liner			= SWLIST_PMODE_PROD; 
	eopt_select_local		= CHARFALSE;
	eopt_verbose			= "1";
	wopt_no_getconf			= "true";
	wopt_no_remote_kill		= CHARFALSE;
	wopt_shell_command		= "detect";
	wopt_local_pax_write_command 	= "detect"; 
	wopt_remote_pax_write_command 	= "detect";
	wopt_local_pax_read_command 	= "tar";
	wopt_remote_pax_read_command 	= "tar";
	wopt_sig_level			= "0";
	wopt_enforce_all_signatures	= CHARFALSE;

	wopt_show_options = NULL;
	wopt_show_options_files = NULL;

	set_opta_initial(opta, SW_E_distribution_target_directory,
			eopt_distribution_target_directory);
	set_opta_initial(opta, SW_E_installed_software_catalog,
			eopt_installed_software_catalog);
	set_opta_initial(opta, SW_E_one_liner, eopt_one_liner);
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
	set_opta_initial(opta, SW_E_swbis_any_format, wopt_any_format);
	set_opta_initial(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
	set_opta_initial(opta, SW_E_swbis_enforce_all_signatures, wopt_enforce_all_signatures);
	set_opta_initial(opta, SW_E_swbis_sig_level, wopt_sig_level);

	cplob_add_nta(w_arglist, strdup(argv[0]));
	cplob_add_nta(e_arglist, strdup(argv[0]));

	while (1) {
		int option_index = 0;

		c = ugetopt_long(argc, argv, "Aa:c:df:t:rl:Vvs:X:x:W:", 
					main_long_options, &option_index);
		if (c == -1) break;

		switch (c) {
		case 'a':
			if (swuts_is_uts_attribute(optarg)) {
				BL->has_uts_attributesM++;
				strcpy(wopt_pgm_mode, SWLIST_PMODE_UTS_ATT);
			} else if (swlist_is_annex_attribute(optarg)) {
				BL->has_annex_attributesM++;
				strcpy(wopt_pgm_mode, SWLIST_PMODE_ATT);
			} else {
				BL->has_soc_attributesM++;
				strcpy(wopt_pgm_mode, SWLIST_PMODE_ATT);
			}
			swlist_blist_attr_add(BL, current_opt_level, optarg);
			break;
		case 'A':
			wopt_forward_agent = CHARTRUE;
			set_opta(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
			break;
		case 'c':
			opt_catalog_dest_dir = strdup(optarg);
			BL->catalogM = strdup(optarg);
			if (strcmp(opt_catalog_dest_dir, "-") != 0) {
				sw_e_msg(G, "only stdout is supported for the -c option, specified by a dash '-'\n");
				LCEXIT(1);
			}
			break;
		case 'd':
			G->g_do_distribution = 1;
			break;
		case 'l':
			opt_level_is_set = 1;
			if (current_opt_level) free(current_opt_level);
			current_opt_level = strdup(optarg);
			if (process_level_option(BL, optarg)) {
				/* error */
				sw_e_msg(G, "invalid level: %s \n", optarg);
				LCEXIT(1);
			}
			break;
		case 'r':
			G->g_do_installed_software = 1;
			G->g_opt_alt_catalog_root = 1;
			break;
		case 'V':
			G->g_verboseG++;
			eopt_verbose = (char*)malloc(12);
			snprintf(eopt_verbose, 11, "%d", G->g_verboseG);
			eopt_verbose[11] = '\0';
			set_opta(opta, SW_E_verbose, eopt_verbose);
			free(eopt_verbose);
			break;
		case 'v':  /* -v is the posix option for
				'use_index_file_format' for output */
			opt_alt_verbose++;
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
			swc0_process_w_option(tmp, w_arglist, optarg, &w_argc);
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
			sw_e_msg(G, "Try `swlist --help' for more information.\n");
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
		
		case 232:
			eopt_distribution_target_directory = strdup(optarg);
			set_opta(opta, 
				SW_E_distribution_target_directory, 
				eopt_distribution_target_directory);
			SWLIB_ALLOC_ASSERT
				(eopt_distribution_target_directory != NULL);
			break;
		case 216:
			eopt_verbose = strdup(optarg);
			G->g_verboseG = swlib_atoi(eopt_verbose, NULL);
			set_opta(opta, SW_E_verbose, eopt_verbose);
			free(eopt_verbose);
			break;
		case 217:
			eopt_installed_software_catalog = strdup(optarg);
			set_opta(opta, SW_E_installed_software_catalog, eopt_installed_software_catalog);
			break;
		case 221:
			optionname = getLongOptionNameFromValue(posix_extended_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			break;
		default:
			sw_e_msg(G, "error processing extended option\n");
			sw_e_msg(G, "Try `swlist --help' for more information.\n");
		 	LCEXIT(1);
               break;
               }
	}
	optind = 1;
	optarg =  NULL;

	/*  Now run the Implementation extension options (-W) through getopt.  */
	while (1) {
		int w_option_index = 1;
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
		case 167:
			G->g_force_locks = 1;
			break;
		case 172:
			remote_shell_command = strdup(optarg);
			set_opta(opta, SW_E_swbis_remote_shell_client, remote_shell_command);
			break;
		case 175:
			wopt_local_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(
					G->g_pax_write_commands, wopt_local_pax_write_command,
							 G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax write command: %s \n",
						wopt_local_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_write_command,
					wopt_local_pax_write_command);
			break;
		case 176:
			wopt_remote_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(
				G->g_pax_write_commands, wopt_remote_pax_write_command, G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				sw_e_msg(G, "illegal pax write command: %s \n",
					wopt_remote_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_remote_pax_write_command,
						wopt_remote_pax_write_command);
			break;
		case 177:
			wopt_local_pax_write_command = strdup(optarg);
			wopt_remote_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(
					G->g_pax_write_commands, wopt_local_pax_write_command,
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
				sw_e_msg(G, "illegal pax read command: %s \n", 
						wopt_local_pax_read_command);
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
			wopt_any_format = CHARTRUE;
			set_opta(opta, SW_E_swbis_any_format, "true");
			break;
		case 201:
			G->g_swi_debug_name = strdup(optarg);
			break;
		case 202:
			G->g_do_task_shell_debug = 1;
			break;
		case 203:
			wopt_forward_agent = CHARTRUE;
			set_opta(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
			break;
		case 204:
			wopt_forward_agent = CHARFALSE;
			set_opta(opta, SW_E_swbis_forward_agent, CHARFALSE);
			break;
		case 205:
			strcpy(wopt_pgm_mode, SWLIST_PMODE_PROD);
			wopt_pgm_mode_explicitly_set = 1;
			break;
		case 206:
			strcpy(wopt_pgm_mode, SWLIST_PMODE_FILE);
			wopt_pgm_mode_explicitly_set = 1;
			wopt_file_sys = 0;
			break;
		case 207:
			strcpy(wopt_pgm_mode, SWLIST_PMODE_TEST1);
			wopt_pgm_mode_explicitly_set = 1;
			break;
		case 208:
			strcpy(wopt_pgm_mode, SWLIST_PMODE_DEP1);
			wopt_pgm_mode_explicitly_set = 1;
			break;
		case 209:
			if (pre_reqs == NULL) pre_reqs = vplob_open();
			{
				SWVERID * id = swverid_open(NULL, optarg);
				if (id == NULL) {
					sw_e_msg(G, "bad software spec: %s\n", optarg);
					exit(1);
				}
				vplob_add(pre_reqs, id);
			}
			break;
		case 210:
			if (ex_reqs == NULL) ex_reqs = vplob_open();
			{
				SWVERID * id = swverid_open(NULL, optarg);
				if (id == NULL) {
					sw_e_msg(G, "bad software spec: %s\n", optarg);
					exit(1);
				}
				vplob_add(ex_reqs, id);
			}
			break;
		case 211:
			strcpy(wopt_pgm_mode, SWLIST_PMODE_DIR);
			wopt_pgm_mode_explicitly_set = 1;
			break;
		case 212:
			optionEnum = getEnumFromName("swbis_sig_level", opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			break;
		case 213:
			strcpy(wopt_pgm_mode, SWLIST_PMODE_FILE);
			wopt_pgm_mode_explicitly_set = 1;
			wopt_file_sys = 1;
			break;

		case 219:
		case 222:
		case 223:
		case 224:
		case 225:
		case 237:
			optionname = getLongOptionNameFromValue(main_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			break;

		case 238:
		case 220:
		case 218:
		case 236:
			optionname = getLongOptionNameFromValue(main_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			E_DEBUG2("option name is [%s]", optionname);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta_boolean(opta, optionEnum, optarg);
			break;

		default:
			sw_e_msg(G, "error processing implementation extension option : %s\n",
				cplob_get_list(w_arglist)[w_option_index]);
		 	exit(1);
		break;
		}

		if (swextopt_get_status()) {
			sw_e_msg(G, "bad value detected for extended option: %s\n", optionname);
			LCEXIT(1);
		}

		if (do_extension_options_via_goto == 1) {
			do_extension_options_via_goto = 0;
			goto gotoStandardOptions;
		}
	}

	optind = main_optind;

	system_defaults_files = initialize_options_files_list(NULL);

	/* Show the options to stdout.  */

	if (wopt_show_options_files) { 
		swextopt_parse_options_files(opta, 
			system_defaults_files, 
			progName, 1 /* not req'd */,  1 /* show_only */);
		swextopt_parse_options_files(opta, 
			opt_option_files, 
			progName,
		 	1 /* not req'd */,  1 /* show_only */);
		LCEXIT(0);
	}

	/*  Read the system defaults files and home directory copies
	    if HOME is set.  */

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
	
	/*  Read the defaults files given with the -X option.  */

	optret += swextopt_parse_options_files(opta, 
		opt_option_files, 
		progName, 	
		1 /* req'd */, 0 /* not show_only */);
	if (optret) {
		sw_e_msg(G, "defaults file error\n");
		LCEXIT(1);
	}

	/*  Reset the option values to pick up the values from 
	    the defaults file(s). */

	eopt_distribution_target_directory = get_opta(opta, SW_E_distribution_target_directory);
	eopt_installed_software_catalog = get_opta_isc(opta, SW_E_installed_software_catalog);
	eopt_select_local		= get_opta(opta, SW_E_select_local);
	eopt_one_liner			= get_opta(opta, SW_E_one_liner);
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
	wopt_local_pax_write_command	= swbisoption_get_opta(opta, 	
					SW_E_swbis_local_pax_write_command);
	wopt_remote_pax_write_command	= swbisoption_get_opta(opta, 
					SW_E_swbis_remote_pax_write_command);
	wopt_local_pax_read_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_local_pax_read_command);
	wopt_remote_pax_read_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_remote_pax_read_command);
	wopt_sig_level			= swbisoption_get_opta(opta, SW_E_swbis_sig_level);

	if (is_index_file_format(wopt_pgm_mode, opt_alt_verbose, wopt_pgm_mode_explicitly_set)) {
		E_DEBUG("");
		opt_index_file_format = 1;
		BL->index_format_is_setM = 1;
	} else {
		E_DEBUG("");
		if (opt_alt_verbose) {
			E_DEBUG("");
			G->g_verboseG += opt_alt_verbose;
		}
		eopt_verbose = (char*)malloc(12);
		snprintf(eopt_verbose, 11, "%d", G->g_verboseG);
		eopt_verbose[11] = '\0';
		set_opta(opta, SW_E_verbose, eopt_verbose);
		free(eopt_verbose);
	}

	set_pgm_mode(G, BL, wopt_pgm_mode, eopt_one_liner, opt_index_file_format);

	if (wopt_show_options) { 
		swextopt_writeExtendedOptions(STDOUT_FILENO, opta, SWC_U_L);
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

	/* Process the Software Selections */

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

	num_cmd_line_swspecs = show_all_sw_selections(swspecs, nullfd);


	/* ----------------------------- */
	/* ----------------------------- */
	/* Loop over the targets.  */
	/* ----------------------------- */
	/* ----------------------------- */

   	current_arg = swc_get_next_target(argv, argc, &optind, 
						G->g_targetfd_array, 
					eopt_distribution_target_directory,
						&num_remains);
	E_DEBUG3("current_arg=[%s] eopt_distribution_target_directory=[%s]", current_arg, eopt_distribution_target_directory);
	E_DEBUG2("num_cmd_line_swspecs is %d", num_cmd_line_swspecs);
	E_DEBUG("");

	while (current_arg && local_stdin == 0) {

		E_DEBUG2("current_arg=[%s]", current_arg);
		E_DEBUG2("Lowest fd is now [%d]", swgp_find_lowest_fd(0));

		distdataO = swi_distdata_create();
		swgp_signal(SIGINT, safe_sig_handler);
		swgp_signal(SIGPIPE, safe_sig_handler);
		swgp_signal(SIGTERM, safe_sig_handler);
		statbytes = 0;
		f_retval = 0;
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

		swlist_blist_clear_all_as_processed(BL);
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
				cl_target_target = strdup(eopt_distribution_target_directory);
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
			if (*working_arg == '@')
				cl_target_target = strdup(working_arg+1);
			else
				cl_target_target = strdup(working_arg);
		}
		E_DEBUG2("NOW cl_target_target = %s", cl_target_target);

		new_cl_target_target = swc_convert_multi_host_target_syntax(G, cl_target_target);
		free(cl_target_target);
		cl_target_target = new_cl_target_target;
		G->g_cl_target_targetM = strdup(cl_target_target);

		/* Parse the target string and set the commands to invoke */

		if (G->g_target_terminal_host) free(G->g_target_terminal_host);
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
		SWLIB_ASSERT(target_nhops >= 0);
		E_DEBUG2("Target nhops is %d", target_nhops);

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

		/* Do a sanity check on the target path */

		if (xcl_target_target == NULL && target_nhops >= 1 && 
					strcmp(cl_target_target, "-") == 0) {
			/* reading from stdin on a remote host is not supported.
			   Reset the the default target */

			/* FIXME?? silently change the target to something that makes
			   sense */
			cl_target_target = strdup(".");
		}

		/* Do some sanity checks on the target path */

		target_path = swc_validate_targetpath(
					target_nhops, 
					tmpcharp, 
					eopt_distribution_target_directory, cwd, "target");

		SWLIB_ASSERT(target_path != NULL);
		E_DEBUG2("target_path is [%s]", target_path);
		
		/* More policy and sanity checks */

		E_DEBUG("1");
		apply_usage_restrictions(NULL, G, target_path);
		E_DEBUG("");
		if (strcmp(target_path, "-") == 0 && target_nhops >= 1) {
			/* error cannot write to stdout except on localhost */
			sw_e_msg(G, "invalid target spec\n");
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}

		/* More policy and sanity checks for use of standard input */

		if (strcmp(target_path, "-") == 0) { 
			if (stdin_in_use) {
				/* error */
				sw_e_msg(G, "invalid usage of stdin\n");
				LCEXIT(1);
			}
			local_stdin = 1; 
		}

		/* Policy and sanity checks for use of standard output */

		local_stdout_in_use = 1;
		if (
			BL->index_format_is_setM == 1 ||
			strar_num_elements(BL->attributesM) > 0
		) { 
			local_stdout_in_use = 0;
		}
		
		 /* Set the informational Event messages fd */

		if (local_stdout_in_use) {
			swevent_fd = STDERR_FILENO; 
		} else {
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
			SWC_VERBOSE_3 + SWLIST_UPVERBOSE, 
			&G->g_logspec, swevent_fd,
			"SWBIS_TARGET_BEGINS for %s\n", current_arg);

		/* Signal Check */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		pax_read_command_key = wopt_local_pax_read_command;
		pax_write_command_key = wopt_local_pax_write_command;

		sw_d_msg(G, "target_fork_type : %s\n", target_fork_type);

		/* Signal Check */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		/* Set the current target_path into the task script
		   protocol object */

		swicol_set_targetpath(swicol, target_path);
		swicol_set_task_debug(swicol, G->g_do_task_shell_debug);

		/* Make the target piping (which is really the source to read) */

		critical_region_begin();
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
				(sigset_t*)fork_blockmask, 
				devel_no_fork_optimization /* reject the no fork optimization */,
							   /* It does not work with swlist's plumbing */
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
			E_DEBUG2("ss_pid=%d", (int)ss_pid);

			/* Signal Check */
			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

			/* is_target_local_regular_file  was set above */

			if (is_target_local_regular_file) {
				/* See if the local file is seekable */

				if (lseek(target_fdar[0],
						(off_t)0, SEEK_SET) != 0) {
					/* sanity check */
					sw_e_msg(G, "lseek internal error on stdio_fdar[0]\n");
					f_retval = 1;
					/* goto next target, this one failed */
					E_DEBUG("goto TARGET1");
					goto TARGET1;
					E_DEBUG("goto ENTER_WITH_FAILURE");
					goto ENTER_WITH_FAILURE;
				}
			} 

			/* Record the pid */
			swc_record_pid(ss_pid, 
				G->g_pid_array, 
				&G->g_pid_array_len, 
				G->g_verboseG);
			E_DEBUG("AB");

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
			E_DEBUG("********* Not a remote target");

			ss_pid = 0;
			target_fdar[0] = target_stdio_fdar[0];
			target_fdar[1] = target_stdio_fdar[1];
			target_fdar[2] = target_stdio_fdar[2];
		}

		critical_region_end();
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		E_DEBUG("");
		if (is_local_stdin_seekable  || is_target_local_regular_file) {
			E_DEBUG("");
			G->g_is_seekable = 1;
		} else {
			E_DEBUG("");
			G->g_is_seekable = 0;
		}

		/* Write the target script (to read the source). */

		/* if local_stdin == 1, then this routine returns 0 doing nothing */

		/* G->g_gpuflag1 conveys the mode into the target script */

		strncpy(G->g_gpufieldM, wopt_pgm_mode, sizeof(G->g_gpufieldM));
		E_DEBUG("");
		source_write_pid = swc_run_source_script(G,
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
					SWC_VERBOSE_4, /* verbose threshhold for common events */
					/* Here is the script to run */
					swlist_write_source_copy_script2
					);

		/* source_write_pid will be:
			< 0 for error
			0  when local_stdin is set
		        > 0 when local_stdin is not set */

		/* Signal Check */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		sw_d_msg(G, "source write pid: %d\n", source_write_pid);

		if (source_write_pid > 0) {
			E_DEBUG("source_write_pid > 0");
			sw_d_msg(G, "waiting on source script pid\n");

			/* Wait on the target script  */
			ret = swc_wait_on_script(G, source_write_pid, "target");

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
				/* Error, start message indicated failure */
				E_DEBUG("ABC");

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
				E_DEBUG("ABCDE");
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
				/* FIXME, should be bigger error than this */
				E_DEBUG("goto ENTER_WITH_FAILURE");
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

			/* Now target_path is absolute */

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
				/* Read the control message from the remote source.  */

				E_DEBUG("");

				sw_d_msg(G, "reading source script control messages\n");

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
			   the SW_SOURCE_ACCESS_ERROR message */

			sw_d_msg(G, "reading source script access messages\n");
			
			E_DEBUG("");
			ret = swc_read_target_ctl_message(G, target_fdar[0],
				target_access_message, G->g_verboseG, "source");

			if (ret < 0) {
				E_DEBUG("");
				sw_d_msg(G, "read_target_ctl_message error"
					" (loc=target_access)\n");
				
				/* Go to the next target */
				E_DEBUG("goto ENTER_WITH_FAILURE");
				goto ENTER_WITH_FAILURE;
			}

			/* Analyze the source. Check the event value, Make
				sure it is SW_SOURCE_ACCESS_BEGINS */
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
				E_DEBUG("goto ENTER_WITH_FAILURE");
				goto ENTER_WITH_FAILURE;
			}
			E_DEBUG("");
		} else if (source_write_pid == 0) {
			E_DEBUG("source_write_pid == 0");
			/* Fork did not happen.
			   This happens when local_stdin is set.  */
			;
		} else {
			/* Fatal error */
			E_DEBUG("");
			sw_e_msg(G, "fatal internal error. fork error.\n");
			main_sig_handler(SIGTERM);
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		/* Now, there are three (3) possibilities on what the script
		   will do:
			1) List a distribution tar archive file
			   Send back a distribution tar archive because the
			   target was a regular file.
			2) List Installed Software
			   Send back tar archive containing a the installed
			   software catalog because the target was a directory
			   and an ISC (InstalledSoftwareCatalog) was found at
			   the expected location of
				<target_path>/<installed_software_catalog>/<tag>/<tag>
			   `tag' is from the software spec on the command line e.g.
				`swlist <tag> @ 192.168.1.2:<target_path>'
			3) List a distribution directory, this must be specifically
			   commanded by use of the '-d' option, otherwise when
			   'target_path' is a directory it is assumed to be
			   installed software.  */

		isc_script_buf = NULL;

		G->g_running_signalusr1M = 0;
		G->g_running_signalsetM = 0;

		if (    source_write_pid > 0 &&
			strob_strstr(target_control_message, SWBIS_SOURCE_CTL_ARCHIVE)
		) {
			/* The target is a tarball, Decode the package catalog section.
			   target_fdar[0] contains a distribution, i.e. a tarball  */
			E_DEBUG("");
			if (swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_TEST1) == 0) {
				VPLOB * xx_specs;
				SWI * xx_swi;
				E_DEBUG("");
				/*
				 *	Scratch test area, access with the --test1 option 
				 */
				xx_swi = swlist_list_create_swi(G, opta, swspecs, target_path);
				xx_specs = swpl_get_dependency_specs(G, xx_swi, SW_A_prerequisites, 0, 0);
				show_all_sw_selections(xx_specs, 2);
			} else {
				E_DEBUG("2");
				apply_usage_restrictions("archive", G, "");
				E_DEBUG("");
				swgp_signal(SIGINT, main_sig_handler);
				ret = swlist_list_distribution_archive(G,
						opta,
						swspecs,
						target_path,
						target_nhops,
						G->g_t_efd);
				if (ret) {
					E_DEBUG("");
					sw_e_msg(G, "error decoding source\n");
					/* FIXME: go to the next target */
					main_sig_handler(SIGTERM);
					swc_shutdown_logger(G, SIGABRT);
					LCEXIT(sw_exitval(G,
						target_loop_count, 
						target_success_count));
				}
			}
			E_DEBUG("");
		} else if ( source_write_pid > 0 &&
			strob_strstr(target_control_message, SWBIS_SOURCE_CTL_DIRECTORY) != NULL
		) {
			/* The target is a directory, In this case, we list
			   installed software at the target path which is a directory */

			/* The cases that are handled by this section are:
				1) List attributes from the catalog
				   according to the one_liner option.
				   If value of one_liner is:
					files  : tar-like listing		
					products : product version vendor_tag listing

				2) If -c option is active, list the catalog
				   selections to stdout according to the -c DIR option.
				   The catalog is exported as a tar archive to stdout.  */

	
			/* Right Now, the remote script is waiting (at a "bash -s") for a
			   task script to be sent to its standard input.
			   This task script will get the catalog according to the
			   software selections */

			/** DEBUG: show_all_sw_selections(swspecs, 2); */
			E_DEBUG("3");
			uret = apply_usage_restrictions("dir", G, "");
			E_DEBUG("");

			isc_script_buf = strob_open(10);
		
			G->g_swicolM = swicol;

			/* Handle the SWBIS_TS_uts task shell */
			ret = swpl_get_utsname_attributes(G, swicol, uts, target_fdar[1], G->g_swi_event_fd);
			if (ret != 0 || uret != 0) {
				swicol_set_master_alarm(swicol);
				swicol_set_task_idstring(swicol, SWBIS_TS_Abort);
				ret = swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
				f_retval++;
				E_DEBUG("goto ENTER_WITH_FAILURE");
				goto ENTER_WITH_FAILURE;
			} else {

				/* Send a gratuitous task shell that does nothing
				   successfully, this is here as a test only. */

				ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": testing 123");
			}
			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
			uts_attr_mode = (
				num_cmd_line_swspecs == 0 &&
				BL->has_soc_attributesM == 0 &&
				BL->has_annex_attributesM == 0 &&
				BL->has_uts_attributesM > 0 &&
				BL->level_is_setM == 0  /* i.e. a level not given on cmd line */
			);

			E_DEBUG2("uts_attr_mode is %d", uts_attr_mode);

			if ( uts_attr_mode) {

				/* This is the special mode that displays the UTS attributes
				   of the host machine instead of the attributes from the package.
				   This is an implementation extension. */

				attr_index = 0;
				E_DEBUG("ELSE LADDER");
				E_DEBUG("at while");
				while ((attr=swlist_blist_get_next_attr(BL, attr_index, &attr_index)) != NULL) {
					/* attr = strar_get(BL->attributesM, 0); */
					E_DEBUG3("attr=%s attr_index=%d", attr, attr_index);
					SWLIB_ASSERT(attr != NULL);
					if (strcmp(attr, SW_A_architecture) == 0) {
						E_DEBUG("");
						/* write the config.guess'ed triplet to stdout,
						   it should be sitting in uts->arch_tripletM */
						swlist_write_attr2(BL, cl_target_target, SW_A_architecture, uts->arch_tripletM, attr_index);
					} 
	
					if (strcmp(attr, SW_A_os_name) == 0) {
						swlist_write_attr2(BL, cl_target_target, SW_A_os_name, uts->sysnameM, attr_index);
					} 
	
					if (strcmp(attr, SW_A_os_version) == 0) {
						swlist_write_attr2(BL, cl_target_target, SW_A_os_version, uts->versionM, attr_index);
					} 
	
					if (strcmp(attr, SW_A_os_release) == 0) {
						swlist_write_attr2(BL, cl_target_target, SW_A_os_release, uts->releaseM, attr_index);
					} 
	
					if (strcmp(attr, SW_A_machine_type) == 0) {
						swlist_write_attr2(BL, cl_target_target, SW_A_machine_type, uts->machineM, attr_index);
					} 
					attr_index++;
				}


				ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
					SWBIS_TS_Get_iscs_listing ": not needed");
				if (ret != 0) {
					swicol_set_master_alarm(swicol);
					sw_e_msg(G, "swicol: fatal: %s:%d\n", __FILE__, __LINE__);
				}
				
				if (f_retval == 0)
					ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": testing 1234");

				E_DEBUG("");

			} else if (swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_CAT) == 0) {

				/* This is the code path for the -c DIR option.
				   Write a tar archive of the catalog to stdout */

				E_DEBUG("ELSE LADDER");
				E_DEBUG("swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_CAT) == 0");

				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

				swicat_write_isc_script(isc_script_buf, G, swspecs, NULL, NULL, NULL, SWICAT_FORM_TAR1);
				ret = swicol_rpsh_task_send_script2(swicol,
					target_fdar[1],			/* file descriptor of the task shell's stdin */
					512,				/* data size, here just a gratuitous block of NULs */
					target_path,			/* Directory to run in */
					strob_str(isc_script_buf),	/* The actual script */
					SWBIS_TS_Get_iscs_listing);
				if (ret != 0) {
					swicol_set_master_alarm(swicol);
					sw_e_msg(G, "swicol: fatal: %s:%d\n", __FILE__, __LINE__);
				}

				/* Every task script has to be fed at least 1 block, this is a 
				   concession to dd's which can't read 0 blocks successfully */
				if (ret == 0) {
					ret = etar_write_trailer_blocks(NULL, target_fdar[1], 1);
				}
				/* Now translate the tar output to stdout */
				E_DEBUG("");
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				E_DEBUG2("ret=%d", ret);

				if (swicol_get_master_alarm_status(swicol) == 0 && ret == 512) {
					TARU * taru = taru_create();

					swgp_signal(SIGINT, main_sig_handler);
					E_DEBUG("");
					ret = taru_process_copy_out(taru, target_fdar[0], STDOUT_FILENO,
							NULL/*defer*/, NULL, arf_ustar, -1, -1, (intmax_t*)NULL, NULL);
					swgp_signal(SIGINT, safe_sig_handler);
					E_DEBUG("");
					taru_delete(taru);
					if (ret < 0) {
						swicol_set_master_alarm(swicol);
						sw_e_msg(G, "error translating archive to stdout\n");
					}
				}
				E_DEBUG("");
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				/* Now reap the task shell events */
				E_DEBUG("");
				ret = swicol_rpsh_task_expect(swicol, G->g_swi_event_fd, SWICOL_TL_4);
				E_DEBUG("done");
				
				if (f_retval == 0)
					ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": testing 1234");

			} else if (
				BL->has_soc_attributesM > 0 ||
				BL->has_annex_attributesM > 0 ||
				swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_FILE) == 0 ||
				swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_ATT) == 0 ||
				swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_INDEX) == 0
			) {
				E_DEBUG("ELSE LADDER");
				E_DEBUG2("wopt_pgm_mode is [%s]", wopt_pgm_mode);
				E_DEBUG("SWLIST_PMODE_FILE");
				/* This section will list the files in installed software */	

				if (! does_have_sw_selections(swspecs)) {
					E_DEBUG("No selections given on command line");
					/* Per spec, If no selections are given,
					   process all selections. Hence we need to add
					   as swverid for "*" */
					swverid = swverid_open(NULL, "*");
					if (swverid == NULL) {
						/* This should never happen */
						SWLIB_FATAL("");
					} else {
						E_DEBUG("Adding * as default selection");
						vplob_add(swspecs, (void*)swverid);
					}
				}

				E_DEBUG("");
				if (f_retval == 0) {
					response_img = swpl_get_samepackage_query_response(G, swicol, target_path,
						target_fdar[0], target_fdar[1], opta, swspecs,
						&f_retval, 0 /* make_dummy_response */);
					if (response_img == NULL) {
						E_DEBUG("");
						abort_script(swicol, G, target_fdar[1]);
       			 			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
						f_retval++;
					}
				} else {
					E_DEBUG("error");
					raise(SIGINT);
					response_img = NULL;
				}

				/* send another dummy taak */
				/* This seems to be important for remote operations.
				   It may have a benificial side effect of making the
				   ``read line'' in shell_lib.sh shls_looper actually receive
				    what was written by atomicio() in swlist_looper_sr_payload() */
				if (f_retval == 0)
					ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": testing 1234");

				/* Now parse the response */
				sl = NULL;
				req = swpl_analyze_samepackage_query_response(G, response_img, &sl);
				if (req == NULL) {
					E_DEBUG("error");
					raise(SIGINT);
				}
	
				/* Now make judgements about the selections according to the
				   Selection Phase spec requirements */
	
				E_DEBUG("");
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				if (G->g_do_debug_events)
					swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);


				/* ----------------------------------------*/	
				/* ----------------------------------------*/	
				/*
				 * Now loop over the selections
				 */
				/* ----------------------------------------*/	
				/* ----------------------------------------*/	
			
				E_DEBUG("Looping over selections");
				G->in_shls_looperM = 1;
				ism = 0;
				no_of_selections_processed = 0;
	
				/* loop over the queries, which are actually the whitespace delimited
				   software selections from the command line */

				E_DEBUG("Loop over selections");
				while (
					/* s_retval == 0 && */
					f_retval == 0 &&
					(sc=vplob_val(sl->scM, ism++)) != NULL &&
					G->g_signal_flag == 0 &&
					1
				) {
					E_DEBUG2("START OF SELECTION LOOP for %s", sc->sqM->swspec_stringM);
					jsm = 0;
					no_of_selections_processed++;
					nmatches = sc_selections_check(G, opta, sc);
					E_DEBUG("while (sc=vplob_val(sl->scM, ism++)");
					if (nmatches == 0) {
						E_DEBUG("SW_SELECTION_NOT_FOUND");
						strob_sprintf(btmp2, 0,
							"SW_SELECTION_NOT_FOUND on %s: %s: status=1\n",
								cl_target_target, sc->sqM->swspec_stringM);
						sw_e_msg(G, strob_str(btmp2));
						s_retval++;
						continue;
					} 

					/* ----------------------------------------*/	
					/* ----------------------------------------*/	
					/* Loop over the responses to a selection query */
					/* ----------------------------------------*/	
					/* ----------------------------------------*/	
	
					while(
						f_retval == 0 &&
						(sr=vplob_val(sc->srM, jsm++)) &&
						G->g_signal_flag == 0 &&
						1
					) {
						swlist_blist_clear_all_as_processed(BL);
						critical_region_begin();
						E_DEBUG2("START OF RESPONSE for %s", swverid_print(sr->swspecM, NULL));
						E_DEBUG("");
						catalog_entry_directory = swicat_sr_form_catalog_path(sr,
										eopt_installed_software_catalog, NULL);
						E_DEBUG("");
						E_DEBUG2("catalog_entry_directory=%s", catalog_entry_directory);
						rp_status = 1;
						E_DEBUG("");
						E_DEBUG2("signal_flag=%d", G->g_signal_flag);
						ret = swlist_looper_sr_payload(G, BL, target_path, cl_target_target, swicol,
									sr,
									target_fdar[1],
									target_fdar[0],
									&rp_status,
									uts,
									pax_write_command_key,
									wopt_file_sys, wopt_pgm_mode);

						/* rp_status is the script return status
						   ret, if non-zero is a protocol or internal error */
	
						E_DEBUG2("signal_flag=%d", G->g_signal_flag);
						E_DEBUG2("swc_looper_sr_payload returned %d", ret);
						if (rp_status != 0) {
							/* error, but continuation possible */
							E_DEBUG("");
							if (
								rp_status != SWP_RP_STATUS_NO_INTEGRITY &&
								rp_status != SWP_RP_STATUS_NO_LOCK &&
								rp_status != SWP_RP_STATUS_NO_PERMISSION
							) {
								sw_e_msg(G, "SW_INTERNAL_ERROR: %s:%d rp_status=%d\n",
									__FILE__, __LINE__, rp_status);
							}
							s_retval++;
							}
						E_DEBUG2("signal_flag=%d", G->g_signal_flag);
						if (ret < 0) {
							/* error that is incompatible with continuing */
							E_DEBUG("");
							raise(SIGINT);
							f_retval++;
							break;
						}
						E_DEBUG2("signal_flag=%d", G->g_signal_flag);
						critical_region_end();
						swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
						E_DEBUG("END OF RESPONSE");
					}  /* loop over responses */
					E_DEBUG("");
					swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
					if (swicol_get_master_alarm_status(swicol)) {
						break;
					}
					E_DEBUG("");
					swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
					E_DEBUG2("END OF SELECTION LOOP for %s", sc->sqM->swspec_stringM);
					/* swevent_s_arr_reset(); */
				} /* loop over software selections */
				E_DEBUG("finished looping over selections");
				E_DEBUG2("f_retval=%d", f_retval);
	
				/* this terminates the shls_looper() loop */
				ret = swicol_send_loop_trailer(swicol, target_fdar[1]);
				G->in_shls_looperM = 0;
	
				if (ret != 0) {
					abort_script(swicol, G, target_fdar[1]);
       	 				swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
					f_retval++;
				}

				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				ret = 0;

				if (f_retval == 0) {
					ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
							SWBIS_TS_Do_nothing ": testing 89");
				}

				if (ret != 0) {
					abort_script(swicol, G, target_fdar[1]);
       		 			swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
					f_retval++;
				}

				if (s_retval) {
					f_retval ++;
				}

				if (sl) {
					swicat_sl_delete(sl);
				}
			} else if (
				swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_PROD) == 0 ||
				swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_DIR) == 0 ||
				swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_DEP1) == 0 
			) {
				E_DEBUG("ELSE LADDER");
				E_DEBUG("SWLIST_PMODE_PROD SWLIST_PMODE_DIR SWLIST_PMODE_DEP1");
				ret = swpl_do_list_catalog_entries2(G,
						swspecs, pre_reqs, NULL, ex_reqs,
						target_path,
						swicol,
						target_fdar[0], target_fdar[1],
						opta, wopt_pgm_mode);
				if (ret > 0) {
					/* dependencies succesfully queried and
					   not satisfied */
					reqs_failed = 1;
					ret = 0;
				} else if (ret < 0) {
					reqs_failed = 1;
				} else {
					reqs_failed = 0;
				}

				if (f_retval == 0)
					ret = swpl_send_success(swicol, target_fdar[1], G->g_swi_event_fd,
						SWBIS_TS_Do_nothing ": testing 1234");

			} else {
				E_DEBUG("ELSE LADDER DEFAULT");
				E_DEBUG("");
				sw_e_msg(G, "unsupported mode\n");
				swicol_set_master_alarm(swicol);
				swicol_set_task_idstring(swicol, SWBIS_TS_Get_iscs_listing);
				abort_script(swicol, G, target_fdar[1]);
				ret = swpl_send_abort(swicol, target_fdar[1], G->g_swi_event_fd, "");
			}
			if (G->g_do_debug_events)
				swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

		} else if ( source_write_pid > 0 ) {
			/* unexpected internal error */
			E_DEBUG("");
			sw_e_msg(G, "internal error at line %d\n", __LINE__);
			LCEXIT(1);
		} else {
			/* This happens when local_stdin is set which
			   should occur when a package is read from standard
			   input, the logger process does not exist for this
			   case. */

			E_DEBUG("");
			swgp_signal(SIGINT, main_sig_handler);

			/* Note: this is the special case for a package on stdin.
				In this case the logger process does not exist hence
				G->g_t_efd is not set, hence we use stdout instead  */
			if (swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_TEST1) == 0) {
				VPLOB * xx_specs;
				SWI * xx_swi;
				E_DEBUG("");
				/* Scratch test area, access with the --test1 option */
				xx_swi = swlist_list_create_swi(G, opta, swspecs, target_path);
				xx_specs = swpl_get_dependency_specs(G, xx_swi, SW_A_prerequisites, 0, 0);
				show_all_sw_selections(xx_specs, 2);
			} else {
				E_DEBUG("");
				ret = swlist_list_distribution_archive(G,
							opta,
							swspecs,
							target_path,
							target_nhops,
							target_fdar[1] /* G->g_t_efd*/ );
				E_DEBUG2("swlist_list_distribution_archive ret=[%d]", ret);
				if (ret) {
					E_DEBUG("");
					sw_e_msg(G, "error decoding source\n");
					main_sig_handler(SIGTERM);
					swc_shutdown_logger(G, SIGABRT);
					LCEXIT(1);
				}
			}
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
		tty_raw_ctl(2);

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
		E_DEBUG("");

ENTER_WITH_FAILURE:
		if (swicol_get_master_alarm_status(swicol) == 0 && source_write_pid > 0 ) {
			E_DEBUG("");
			swicol_rpsh_wait_for_event(swicol, (STROB *)NULL, G->g_swi_event_fd, SWI_MAIN_SCRIPT_ENDS);
		}

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

		/* Now close down  */
			
		E_DEBUG("");

		/* Signal Check */
		E_DEBUG("");
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		close(target_fdar[0]);
		E_DEBUG("");
		if (target_fdar[1] != STDOUT_FILENO) 
			close(target_fdar[1]);

		if (G->g_target_did_abortM || G->g_signal_flag) {
			swlib_kill_all_pids(G->g_pid_array +
				SWC_PID_ARRAY_LEN,
				G->g_pid_array_len,
				SIGTERM,
				G->g_verboseG);
			sleep(1);
			swlib_kill_all_pids(G->g_pid_array +
				SWC_PID_ARRAY_LEN,
				G->g_pid_array_len,
				SIGKILL,
				G->g_verboseG);
		}

		E_DEBUG("");
		swlib_wait_on_all_pids_with_timeout(
				G->g_pid_array, 
				G->g_pid_array_len, 
				G->g_status_array, 0 /*waitpid flags*/, 
				G->g_verboseG - 2, -3);
		E_DEBUG("");

		if (f_retval == 0) {
			f_retval = swc_analyze_status_array(G->g_pid_array,
						G->g_pid_array_len, 
						G->g_status_array,
						G->g_verboseG - 2);
		}
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

		if (target_script_pid) free(target_script_pid);
		target_script_pid = NULL;

		target_loop_count++;
		if (f_retval == 0) target_success_count++;
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_3 + SWLIST_UPVERBOSE,
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
					eopt_distribution_target_directory, &num_remains);
		swi_distdata_delete(distdataO);
		/* swevent_s_arr_reset(); */
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

	/* swevent_s_arr_delete(); */
	strob_close(tmp);
	strob_close(btmp2);
	E_DEBUG("");
	swutil_close(swlog);
	E_DEBUG("");
	close(G->g_swevent_fd);
	close(G->g_swi_event_fd);
	if (G->g_logger_pid > 0) {
		f_retval = swc_shutdown_logger(G, 0);
		if (f_retval) exit(1);
	}
	if (reqs_failed) exit(3);
	exit(sw_exitval(G, target_loop_count, target_success_count));	
}
