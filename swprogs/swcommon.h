/* swcommon.h - Second lowest level common header file.
  
   Copyright (C) 2003-2004 James H. Lowe, Jr.  <jhlowe@acm.org>
  
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

#ifndef swcommon_200212j_h
#define swcommon_200212j_h

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vplob.h"
#include "usgetopt.h"
#include "swcommon0.h"
#include "swextopt.h"
#include "swutillib.h"
#include "swverid.h"
#include "globalblob.h"
#include "swi.h"

#define KILL_PID		"$$"

   void swc_copyright_info(FILE * fp);

   int initExtendedOption(void);

   void debug_writeBooleanExtendedOptions(int ofd, struct extendedOptions * opta);

 
   char * initialize_options_files_list(char * usethis);

   int swc_do_preview_cmd(GB * G, char * prefix, FILE * fver, char * targetpath,
			SHCMD * sshcmd, SHCMD * kill_sshcmd, char * cl_target, 
			STRAR * cmdlist, int nhops, int count,
			int opt_no_getconf, char * shellname);

   int swc_form_command_args(GB * G, char * context, char * targetpath,
			STRAR * target_cmdlist, int opt_preview, int nhops,
			int opt_no_getconf, char * shellname);

   int swc_parse_soc_spec(char * arg, char ** selections, char ** target);

   int swc_open_filename(char * sourcefilename, int * open_errorp);

   char * swc_validate_targetpath(int nhops, char * targetpath,
					char * default_target, char * cwd, char * context);

   int swc_pump_line(int ofd, int ifd);

   pid_t swc_run_ssh_command(GB * G,
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
		char * fork_type, int make_master_raw, 
		sigset_t * ignoreset,
		int no_fork_ok,
		int verbose_level,
		int * source_file_size,
		int opt_no_getconf,
		int * is_local_seekable_file,
		char * landing_shell,
		int error_fd,
		struct sw_logspec * logspec
		);

   int swc_analyze_status_array(pid_t * pid, int num, int * status, int debug);

   int swc_do_run_kill_cmd(SHCMD * killcmd, SHCMD *, SHCMD *, int verbose_level);

   int swc_checkbasename(char * s);

   void swc_record_pid(pid_t pid, pid_t * p_pid_array, int * p_pid_array_len,
							int verbose_level);
   void swc_set_sig_ign_by_mask(sigset_t * ignoremask);

   char * swc_get_next_target(char ** argvector, int uargc, int * poptind,
				int * targetsfd, char * default_target, int * rem);
/*
   mode_t swc_get_umask(void);
*/
   char * swc_print_umask(char * buf, size_t len);

   int swc_construct_newkill_vector(SHCMD * kmd, int nhops, STRAR * tramp_list,
				 char * script_pid, int verbose_lvel);
   
   char * swc_get_terminal_host(SHCMD * sshcmd, int nhops);

   pid_t swc_fork_logger(GB * G, STROB *, STROB *, int efd, int ofd, struct sw_logspec *,
			int * s_efd, int * t_efd, int verboselevel, int * pfd_array);

   int swc_process_selection_args(VPLOB * , char ** argvector, int uargc, int * poptind);

   int swc_process_selection_files(GB * G, VPLOB * swspec_list);

   void swc_flush_logger(GB * G, STROB * slinebuf, STROB * tlinebuf, int efd,
			int ofd, struct sw_logspec *, int verboselevel);

   char * swc_get_pax_read_command(struct g_pax_read_command g_pax_read_commands[],
			char * keyid, int verbose_level, 
			int keep, char * paxdefault);

   char * swc_get_pax_write_command(struct g_pax_write_command g_pax_write_commands[], char * keyid,
				 int verbose_level, char * paxdefault);
   
   char * swc_get_pax_remove_command(struct g_pax_remove_command g_pax_remove_commands[], char * keyid,
				 int verbose_level, char * paxdefault);

   int swc_open_logfile(char * logfile);
   void swc_initialize_logspec(struct sw_logspec * logspec, char * logfile, int log_level);
   
   int swc_process_swoperand_file(SWLOG * swutil, char * type_string, char * filename,
			int * p_stdin_in_use, int * p_array_index, int * fd_array);

void
swc_set_boolean_x_option(struct extendedOptions * opta, 
		enum eOpts nopt, 
		char * arg, 
		char ** optionp);

int swc_write_source_copy_script(GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv,
			int delaytime, int nhops, char * pax_write_command_key, char * hostname, char * blocksize);

int swc_read_target_ctl_message(GB * G, int fd, STROB * control_message, int vlevel, char * loc);
char * swc_get_target_script_pid(GB * G, STROB * control_message);
int swc_read_start_ctl_message(GB * G, int fd, STROB * control_message, STRAR * tramp_list, int vlevel, 
	char ** pscript_pid, char * loc); 

int swc_tee_to_file(char * name, char * buf);
void swc_gf_xformat_close(GB * G, XFORMAT * xformat);

int swc_get_stderr_fd(GB * G);

int swc_shutdown_logger(GB * G, int signum);

void swc_set_stderr_fd(GB * G, int fd);
void swc_lc_raise(GB * G, char * file, int line, int signo);
void swc_lc_exit(GB * G, char * file, int line, int status);
int swc_is_option_true(char * s);
int sw_exitval(GB * G, int count, int suc);
void swc_check_for_current_signals(GB * G, int lineno, char * no_remote_kill);


pid_t swc_run_source_script(GB * G,
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
		int (*func_write_source_copy_script)
			(
				GB * G,
				int ofd,
				char * sourcepath,
				int do_get_file_type,
				int vlv,
				int delaytime,
				int nhops,
				char * pax_write_command_key,
				char * hostname,
				char * blocksize
			)
		); 
void swc_helptext_target_syntax(FILE * fp);
int swc_wait_on_script(GB * G, pid_t write_pid, char * location);
void swc_set_shell_dash_s_command(GB * G, char * wopt_shell_command);
int swc_write_swi_debug(SWI * swi, char * filename);
char * swc_convert_multi_host_target_syntax(GB * G, char * target_spec);
int sw_e_msg(GB * G, char * format, ...);
int sw_l_msg(GB * G, int write_at_level, char * format, ...);
int sw_d_msg(GB * G, char * format, ...);
int sw_msg(GB * G, int fd, char * format, ...);
void swc_stderr_fd2_set(GB * G, int fd);
void swc_stderr_fd2_restore(GB * G);
void swc_process_pax_command(GB * G, char * command, char * type, int sw_e_num);
void swc_check_all_pax_commands(GB * G);
void swc_check_shell_command_key(GB * G, char * cmd);
char * swc_get_default_sh_dash_s(GB * G);
STROB * swc_make_multiplepathlist(STRAR * sourcepath_listM, STROB * rootdir, STROB * firstpath);
int swc_read_target_message_129(GB * G, int fd, STROB * cdir, int vlevel, char * loc);
char * swc_make_absolute_path(char * target_current_dir, char * target_path);


#endif
