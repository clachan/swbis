/* globalblob.c -- All the main routine globals in a struct

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

#include "swuser_config.h"
#include "globalblob.h"
#include "swcommon.h"
#include "pax_commands.h"

GB *
gb_create(void) {
	GB * gb;
	gb = malloc (sizeof(GB));
	gb_init(gb);
	return gb;
}

void
gb_delete(GB * G) {
	return;
}

void
gb_init(GB * G) {
	int i;
	
	G->g_stdout_testfd = -1; 
	G->g_fail_loudly = -1;
	G->g_verbose_threshold = SWC_VERBOSE_4;  /* default for most utiltiies */
	G->g_verboseG = 1; 
	G->g_signal_flag = 0; 
	G->g_t_efd = -1;
	G->g_s_efd = -1;
	G->g_logger_pid = 0;
	G->g_swevent_fd = -1;
	G->g_vstderr = (FILE*)NULL;

	/* G->g_pid_array[SWC_PID_ARRAY_LEN + SWC_PID_ARRAY_LEN]; */
	for (i=0; i<SWC_PID_ARRAY_LEN + SWC_PID_ARRAY_LEN; i++) {
		G->g_pid_array[i] = 0;
	}
	/* G->g_status_array[SWC_PID_ARRAY_LEN]; */
	for (i=0; i<SWC_PID_ARRAY_LEN; i++) {
		G->g_status_array[i] = 0;
	}
	/* G->g_targetfd_array[SWC_TARGET_FD_ARRAY_LEN]; */
	for (i=0; i<SWC_TARGET_FD_ARRAY_LEN; i++) {
		G->g_targetfd_array[i] = -1;
	}

	/* G->g_selectfd_array[SWC_TARGET_FD_ARRAY_LEN]; */
	for (i=0; i<SWC_TARGET_FD_ARRAY_LEN; i++) {
		G->g_selectfd_array[i] = -1;
	}

	G->g_io_req.tv_sec = 0;
	G->g_io_req.tv_nsec = 0;

	G->g_target_kmd = (SHCMD*)NULL;
	G->g_source_kmd = (SHCMD*)NULL;
	G->g_killcmd = (SHCMD*)NULL;
	G->g_pid_array_len = 0;
	G->g_nullfd = open("/dev/null", O_RDWR, 0);
	if (G->g_nullfd < 0) { 
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
			&G->g_logspec, swc_get_stderr_fd(G), "Can't open /dev/null\n");
		exit(1);
	}

	G->g_stderr_fd = -1;
	G->g_target_fdar = (int*)(NULL);
	G->g_source_fdar = (int*)(NULL);
	G->g_save_fdar = (int*)(NULL);
	G->g_fork_pty = SWFORK_PTY;
	G->g_fork_pty2 = SWFORK_PTY2;
	G->g_fork_pty_none = SWFORK_NO_PTY;
	sigemptyset(&(G->g_fork_defaultmask));
	sigemptyset(&(G->g_fork_blockmask));
	sigemptyset(&(G->g_ssh_fork_blockmask));
	sigemptyset(&(G->g_currentmask));
	G->g_do_progressmeter = 0;
	/* G->g_logspec = {-1, 0, -1 }; */
	G->g_logspec.logfdM = -1;
	G->g_logspec.loglevelM = 0;
	G->g_logspec.fail_loudlyM = -1;
	G->g_loglevel = 0;
	G->g_meter_fd = STDOUT_FILENO;
	G->g_xformat = NULL;        
	G->g_swi_event_fd = -1;    
	G->g_swlog = NULL;       
	G->g_pax_read_commands = g_pax_read_commands;
	G->g_pax_write_commands = g_pax_write_commands;
	G->g_pax_remove_commands = g_pax_remove_commands;
	G->g_source_script_name = (char*)NULL;
	G->g_target_script_name = (char*)NULL;
	G->g_swi_debug_name = (char*)NULL;
	G->g_is_seekable = 0;
	G->g_do_debug_events = 0;
	G->g_target_terminal_host = (char*)NULL;
	G->g_source_terminal_host = (char*)NULL;
	G->g_sh_dash_s = (char*)NULL;
	G->g_opt_alt_catalog_root = 0;
	G->optaM = (struct extendedOptions *)NULL;
	G->g_do_task_shell_debug = 0;
	G->g_do_distribution = 0;
	G->g_do_installed_software = 0;
	G->g_master_alarm = 0;
	G->g_noscripts = 0;
	G->g_swi = NULL;
	G->e_in_control_scriptM = 0;
	G->devel_verboseM = 0;
	G->g_to_stdout = 0;
	G->g_force = 0;
	G->g_force_locks = 0;
	G->in_shls_looperM = 0;
	G->g_swicolM = NULL;
	G->g_main_sig_handler = (void (*)(int))NULL;
	G->g_safe_sig_handler = (void (*)(int))NULL;
	G->g_psignalM = 0;
	G->g_running_signalsetM = 0;
	G->g_running_signalusr1M = 0;
	G->g_target_did_abortM = 0;
	G->g_save_stderr_fdM = 0;
	G->g_opt_previewM = 0;
	G->g_do_cleanshM = 0;
	G->g_gpuflagM = 0;
	G->g_do_createM = 0;
	G->g_do_extractM = 0;
	G->g_sourcepath_listM = NULL;
	G->g_no_extractM = 0;
	G->g_do_show_linesM = 0;
	G->g_catalog_info_nameM = NULL;
	G->g_system_info_nameM = NULL;
	G->g_justdbM = 0;
	G->g_do_dereferenceM = 0;
	G->g_do_hard_dereferenceM = 0;
	G->g_ignore_slack_installM = 0;
	*(G->g_gpufieldM) = '\0';
	G->g_do_unconfigM = 0;
	G->g_config_postinstallM = 0;
	G->g_send_envM = 0;
	G->g_no_summary_reportM = 0;
	G->g_no_prelinkM = 0;     	/* prelink check is enabled by default */
	G->g_output_formM = 0;
	G->g_cl_target_targetM = NULL;
	G->g_no_script_chdirM = 0;
	G->g_prelink_fdM = -1;
	G->g_prelink_fd_memM = NULL;
}
