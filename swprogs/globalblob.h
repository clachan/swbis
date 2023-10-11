/*  globalblob.h -- The main routine globals in a struct */
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/


#ifndef globalblob_h_200604
#define globalblob_h_200604

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
#include "swcommon0.h"
#include "progressmeter.h"
#include "swevents.h"
#include "swi.h"
#include "swicol.h"
#include "swutillib.h"
#include "xformat.h"

typedef struct {
	struct extendedOptions * optaM;
	int 	g_stdout_testfd;
	int 	g_fail_loudly; 
	int 	g_verbose_threshold;  /* threshold for printing a message */ 
	int 	g_verboseG;      /* current set verbosity. */
	int 	g_signal_flag;   /* POSIX signal number */
	int 	g_t_efd;         /* file descriptor, target stderr */
	int 	g_s_efd;         /* file descriptor, source stderr */
	pid_t 	g_logger_pid;    /* pid_t of logger process */
	int 	g_swevent_fd;    /* file descriptor, read by the (SWI*) module */
	FILE * 	g_vstderr;
	pid_t 	g_pid_array[SWC_PID_ARRAY_LEN + SWC_PID_ARRAY_LEN];
	int 	g_status_array[SWC_PID_ARRAY_LEN];
	int 	g_targetfd_array[SWC_TARGET_FD_ARRAY_LEN];
	int 	g_selectfd_array[SWC_TARGET_FD_ARRAY_LEN];
	struct timespec g_io_req;
	SHCMD * g_target_kmd;
	SHCMD * g_source_kmd;
	SHCMD * g_killcmd;
	int 	g_pid_array_len;
	int 	g_nullfd;
	int 	g_stderr_fd;
	int * 	g_target_fdar;
	int * 	g_source_fdar;
	int * 	g_save_fdar;
	char * g_fork_pty;
	char * g_fork_pty2;
	char * g_fork_pty_none;
	sigset_t g_fork_defaultmask;
	sigset_t g_fork_blockmask;
	sigset_t g_ssh_fork_blockmask;
	sigset_t g_currentmask;
	int 	g_do_progressmeter;
	struct sw_logspec g_logspec;
	int 	g_loglevel;
	int 	g_meter_fd;
	XFORMAT * g_xformat;
	int 	g_swi_event_fd;
	SWLOG * g_swlog;
	struct g_pax_write_command * g_pax_write_commands;
	struct g_pax_read_command * g_pax_read_commands;
	struct g_pax_remove_command * g_pax_remove_commands;
	char * g_source_script_name;
	char * g_target_script_name;
	char * g_swi_debug_name;
  	void (*g_main_sig_handler)(int signum);
  	void (*g_safe_sig_handler)(int signum);
	int g_is_seekable;
	int g_do_debug_events;
	char * g_target_terminal_host;
	char * g_source_terminal_host;
	char * g_sh_dash_s;  /* i.e. `sh -s'   or  `bash -s' */ 
	int g_opt_alt_catalog_root;  /* utility  -r option */
	int g_do_task_shell_debug;
	int g_do_distribution;
	int g_do_installed_software;
	int g_master_alarm;
	int g_noscripts;
	SWI * g_swi;
	int e_in_control_scriptM;
	int devel_verboseM;
	int g_to_stdout;
	int g_force;
	int g_force_locks;
	int in_shls_looperM;
	SWICOL * g_swicolM;
	int g_psignalM;
	int g_running_signalsetM;
	int g_running_signalusr1M;
	int g_target_did_abortM;
	int g_save_stderr_fdM;
	int g_opt_previewM;
	int g_do_cleanshM;
	char g_gpufieldM[30];
	int g_gpuflagM;
	int g_do_createM; 		/* used by swcopy */
	int g_do_extractM; 		/* used by swcopy */
	STRAR * g_sourcepath_listM;	 /* used by swcopy */
	int g_no_extractM;	 	/* used by swcopy */
	int g_do_show_linesM; 		/* used by swverify */
	char * g_catalog_info_nameM; 	/* used by swverify */
	char * g_system_info_nameM; 	/* used by swverify */
	int g_justdbM; 			/* operate on installed catalog only */
	int g_do_dereferenceM;
	int g_do_hard_dereferenceM;
	int g_ignore_slack_installM;
	int g_do_unconfigM;
	int g_config_postinstallM;
	int g_send_envM;
	int g_no_summary_reportM;	/* used by swverify */
	int g_output_formM;		/* used by swverify */
	int g_no_prelinkM;		/* used by swverify */
	char * g_cl_target_targetM;
	int g_no_script_chdirM;
	int g_prelink_fdM;		/* used by swverify */
	char * g_prelink_fd_memM;	/* used by swverify */
} GB;

#define G_opta (G->optaM)
#define GFP_GET(K) gb_fparams_get(G, K)
#define GFP_ADD(K, V) gb_fparams_add(G, K, V)

#define G_is_in_control_script(G) (G->e_in_control_scriptM != 0)
#define G_set_is_in_control_script(G, a) G->e_in_control_scriptM = a

void gb_fparams_init(GB * G);
void gb_fparams_delete(GB * G);
GB * gb_create(void);
void gb_delete(GB* G);
void gb_init(GB * G);
int gb_fparams_add(GB * G, char * kw, char * val);
char * gb_fparams_get(GB * G, char * kw);

#endif
