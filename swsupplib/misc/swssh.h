#ifndef swssh_h_20031006
#define swssh_h_20031006

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include "strar.h"
#include "shcmd.h"


#define SWSSH_POSIX_SHELL_COMMAND     "PATH=`getconf PATH` sh -s"
#define SWSSH_SYSTEM_SHELL_COMMAND    "/bin/sh -s"
#define SWSSH_SH_SHELL_COMMAND        "/bin/sh -s"
#define SWSSH_BASH_SHELL_COMMAND      "/bin/bash -s"
#define SWSSH_KSH_SHELL_COMMAND      "/bin/ksh -s"
#define SWSSH_MKSH_SHELL_COMMAND      "/bin/mksh -s"
#define SWSSH_ASH_SHELL_COMMAND      "/bin/ash -s"   /* not supported */
#define SWSSH_DASH_SHELL_COMMAND      "/bin/dash -s"
#define SWSSH_DETECT_POISON_DEFAULT   "exit 1"     /* poison default when auto-detect is on */

/* NOT USED #define SWSSH_SSH_BIN "/usr/bin/ssh" */
#define SWC_KILL_PID_MARK "_"
#define SWSSH_TRACK_PID "$PPID"

int swssh_parse_target(SHCMD * cmd, SHCMD * killcmd, 
			char * target, char * ssh_command, char * remote_ssh_command,
			char ** path, char ** last_host, char * tty_option, int do_imsg,
			char * sshoption, int do_forward_auth_agent);
int swssh_run_ssh_cmd(SHCMD * shcmd, int verbose, int outfd, int infd);
int swssh_assemble_ssh_cmd(SHCMD * shcmd, STRAR * cmdlist, STRAR * delimlist, int nhops);
int swssh_protect_shell_metacharacters(STROB * command, int nhops, char * taints);
void swssh_reset_module(void);
char * swssh_landing_command(char * shellname, int opt_no_getconf);
void swssh_deactivate_sanity_check(void);
int swssh_determine_target_path(char * target, char ** path);

#endif
