/* swssh.c  --  routines to run a command via multilple ssh hops.
 */

/*
   Copyright (C) 2003,2004,2006 James H. Lowe, Jr. <jhlowe@acm.org>
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
*/

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <utime.h>
#include "swgp.h"
#include "strob.h"
#include "swssh.h"
#include "swfork.h"
#include "shcmd.h"
#include "uxfio.h"
#include "swlib.h"
#include "strob.h"
#include "strar.h"

#define TARGET_DELIM "@@"

static int g_active_flag = 1;
static int g_did_get_command = 0;


static
int
is_posix_shell_command(char *s)
{
	int retval = 0;
	if (strstr(s, "getconf") && strstr(s, "PATH")) {
		/*
		* Try to determine if this is the 
		* SWSSH_POSIX_SHELL_COMMAND arg
		*/
		char * sp = strdup(s);
		char * p = sp;

		/* disable the test below, it is currently broken */
		return 1;

		while(p && *p) {
			if (*p == '\\')
				memmove(p, p+1, strlen(p+1) + 1);
			else
				p++;
		}	
		if (strcmp(sp, SWSSH_POSIX_SHELL_COMMAND) == 0) {
			/*
			* It really is the SWSSH_POSIX_SHELL_COMMAND
			* shell meta characters are allowed in this case.
			*/
			if (g_did_get_command) {
				/*
				* Sanity error, Only allow finding this once.
				* Maybe the user is up to something bad.
				*/
				fprintf(stderr, "internal error in swssh\n");
				exit(2);
			}
			g_did_get_command = g_active_flag;
			retval = 1;
		} else {
			retval = 0;
		}
		free(sp);
	} else {
		/*
		* Zero (0) is safe;
		*/
		retval = 0;
	}
	return retval;
}

static
int
bail_on_taint(char *s) {
	if (swlib_is_sh_tainted_string(s)) {
		fprintf(stderr, "arg contains shell meta characters [%s]\n", s);
		exit(2);
	}
	return 0;
}

static
char **
safe_shcmd_add_arg(SHCMD * cmd, char *s)
{
	/*
	* The only arg that is allowed to have
	* shell metacharacters is the
	* SWSSH_POSIX_SHELL_COMMAND arg.
	*/
	if (!cmd) return NULL;
	if (is_posix_shell_command(s) == 0) {
		bail_on_taint(s);
	}
	return shcmd_add_arg(cmd, s);
}

static
void
clean_leading_comm_at_sign(char ** ptar)
{
	if (*ptar == NULL) return;
	if ((**ptar) == '@') (*ptar)++;
	while ((**ptar) == '\x20') (*ptar)++;
}

static
int
cat_escapes(STROB * command, int nhops)
{
	int nbackslashes = 0;

	if (nhops == 0) { nbackslashes = 0; }
	else if (nhops == 1) { nbackslashes = 0; }
	else if (nhops == 2) { nbackslashes = 1; }
	else if (nhops == 3) { nbackslashes = 3; }
	else if (nhops == 4) { nbackslashes = 7; }
	else {
		return -1;
	}

	{
		int i = 0;
		for (i=0; i<nbackslashes; i++) {
			strob_strcat(command, "\\");
		}		
	}
	return 0;
}

static
int
form_intermediate_msg(STROB * command, char * host, int nhops)
{
	strob_sprintf(command, 0,
		"echo 10%d Intermediate Host:%s:" SWSSH_TRACK_PID ";",
						nhops, host);
	swssh_protect_shell_metacharacters(command, nhops,
						SWBIS_TAINTED_CHARS);
	return 0;
}

static
void
add_host_kill(int doimsg, SHCMD * cmd, char * host, int cmdcount)
{
	char * s;
	char * tok;
	STROB * t;
	STROB * tmp;

	if (!doimsg) return;
	t = strob_open(10);
	tmp = strob_open(10);

	strob_sprintf(tmp, 0, "kill " SWC_KILL_PID_MARK " ;", cmdcount, host);
	swssh_protect_shell_metacharacters(tmp, cmdcount, SWBIS_TAINTED_CHARS);

	strob_strcpy(t, strob_str(tmp));
	s = strob_str(t);
	while((tok = strob_strstrtok(t, s, " "))) {
		s = NULL;
		shcmd_add_arg(cmd, tok);
	}
	strob_close(tmp);
	strob_close(t);
}

static
void
add_host_message(int doimsg, SHCMD * cmd, char * host, int cmdcount)
{
	char * s;
	char * tok;
	STROB * t;
	STROB * tmp;

	if (!doimsg) return;
	t = strob_open(10);
	tmp = strob_open(10);
	form_intermediate_msg(tmp, host, cmdcount);
	strob_strcpy(t, strob_str(tmp));
	s = strob_str(t);
	while((tok = strob_strstrtok(t, s, " "))) {
		s = NULL;
		shcmd_add_arg(cmd, tok);
	}
	strob_close(tmp);
	strob_close(t);
}

static
void
parse_host_port(char * host, SHCMD * cmd, SHCMD * kmd)
{
	char * u;
	char * s;
	if ((u=strchr(host, '_')) == NULL) {		
		safe_shcmd_add_arg(kmd, host);
		safe_shcmd_add_arg(cmd, host);
	} else {
		int status;
		s = u;
		s++;		
		swlib_atoi(s, &status);
		if (status == 0) {
			*u = '\0';
			safe_shcmd_add_arg(kmd, "-p");
			safe_shcmd_add_arg(cmd, "-p");
			safe_shcmd_add_arg(kmd, s);
			safe_shcmd_add_arg(cmd, s);
			safe_shcmd_add_arg(kmd, host);
			safe_shcmd_add_arg(cmd, host);
			*u = '_';
		} else {
			/* probably a illegal host name */
			safe_shcmd_add_arg(kmd, host);
			safe_shcmd_add_arg(cmd, host);
		}
	}
}

void
swssh_deactivate_sanity_check(void)
{
	g_active_flag = 0;
}

void
swssh_reset_module(void)
{
	g_did_get_command = 0;
}

char *
swssh_landing_command(char * shellname, int opt_no_getconf)
{
	if (shellname) {
		if (strcmp(shellname, "bash") == 0) {
			return SWSSH_BASH_SHELL_COMMAND;	
		} else if (strcmp(shellname, SH_A_sh) == 0) {
			return SWSSH_SH_SHELL_COMMAND;	
		} else if (strcmp(shellname, SH_A_ash) == 0) {
			return SWSSH_ASH_SHELL_COMMAND;	
		} else if (strcmp(shellname, SH_A_ksh) == 0) {
			return SWSSH_KSH_SHELL_COMMAND;	
		} else if (strcmp(shellname, SH_A_mksh) == 0) {
			return SWSSH_MKSH_SHELL_COMMAND;	
		} else if (strcmp(shellname, SH_A_dash) == 0) {
			return SWSSH_DASH_SHELL_COMMAND;	
		} else if (strcmp(shellname, "posix") == 0) {
			return SWSSH_POSIX_SHELL_COMMAND;	
		} else if (strcmp(shellname, "detect") == 0) {
			return SWSSH_SYSTEM_SHELL_COMMAND;	
		} else {
			return SWSSH_SYSTEM_SHELL_COMMAND;	
		}
	} else {
		if (opt_no_getconf == 0)
			return SWSSH_POSIX_SHELL_COMMAND;	
		else
			return SWSSH_SYSTEM_SHELL_COMMAND;	
	}
}

int
swssh_protect_shell_metacharacters(STROB * command, int nhops, char * taints) {
	STROB * tmp = strob_open(1);
	char ns[2];
	char * s;

	strob_strcpy(tmp, strob_str(command));
	s = strob_str(tmp);
	strob_strcpy(command, "");
	ns[1] = '\0';
	while (*s) {
		if (strchr(taints, (int)(*s))) {
			cat_escapes(command, nhops);
		}
		ns[0] = *s;
		strob_strcat(command, ns);	
		s++;
	}
	strob_close(tmp);
	return 0;
}

int
swssh_determine_target_path(char * target, char ** path)
{
	/*
	* parse the path name at the end of the target.
	*/
	char * ds;
	char * p;
	
	if (*target == ':' || *target == '/' || *target == '.' || *target == '-' ) {
		*path = target;
		target = target + strlen(*path);
	} else {
		ds = target;
		p = strrchr(ds, ':');
		if (p) {
			(*path) = ++p;
		} else if (!p) {
			if (ds && (*ds == '/' || *ds == '.' ||
							*ds == '-')) {
				*path = ds;
			}
		} else {
			;
		}
		if (*path) {
			*path = target + strlen(target);
			if (strstr(*path, TARGET_DELIM)) {
				fprintf(stderr,
				"target path not at terminal host\n");
				return  -1;
			} else {
				return  -1;
			}
		}
	}
	return 0;
}

int
swssh_parse_target(SHCMD * cmd, 
			SHCMD * kmd, 
			char * target, 
			char * ssh_command, 
			char * fp_remote_ssh_command, 
			char ** path, 
			char ** terminal_host, 
			char * tty_option, 
			int do_imsg,
			char * sshoption,
			int do_forward_auth_agent)
{
	int cmdcount = 0;
	int icmdcount = 0;
	int is_multihop = 0;
	int nhops;
	STROB * tmp = strob_open(10);
	STROB * tmp2 = strob_open(10);
	char * remote_ssh_command;
	char * p;
	char * s;
	char * commat;
	char * user;
	char * host;
	char * parent_host = NULL;
	char * tok;
	char * is_ssh;

	clean_leading_comm_at_sign(&target);

	*terminal_host = (char*)NULL;
	*path = (char*)NULL;

	if (ssh_command == NULL) ssh_command = "ssh";
	is_ssh  = strstr(ssh_command, "ssh");


	strob_strcpy(tmp, fp_remote_ssh_command);

	s = strstr(strob_str(tmp), "//"); 
	if (0 && s == strob_str(tmp)) {
		/* disabled */
		/* FIXME */
		/*
		* Hack!! If the pathname begins with "//"
		* then leave it alone.  This is an unpublished
		* feature that may be useful for testing different
		* versions of ssh.
		*/
		;
	} else {
		/*
		* strip the pathname part.
		* The intermediate and terminal ssh invocations are not
		* not absolute paths.
		*/	
		swlib_basename(tmp, fp_remote_ssh_command);
	}
	remote_ssh_command = strdup(strob_str(tmp));

	/* ret = swssh_determine_target_path(target, path); */
	{
		/*
		* parse the path name at the end of the target.
		*/
		char * ds;
		
		if (*target == ':' || *target == '/' || *target == '.' || *target == '-' ) {
			*path = target;
			target = target + strlen(*path);
		} else {
			ds = target;
			p = strrchr(ds, ':');
			if (p) {
				(*path) = ++p;
			} else if (!p) {
				if (ds && (*ds == '/' || *ds == '.' ||
								*ds == '-')) {
					*path = ds;
				}
			} else {
				;
			}
			if (*path) {
				if (strstr(*path, TARGET_DELIM)) {
					fprintf(stderr,
					"target path not at terminal host\n");
					return  -1;
				}
			}
		}
	}

	strob_strcpy(tmp, target);
	s = strob_str(tmp);
	nhops = 0;
	while((tok = strob_strstrtok(tmp, s, TARGET_DELIM))) {
		nhops++;
		s = NULL;
	}
	
	if (nhops > 1) {
		is_multihop = 1;
	}

	strob_strcpy(tmp, target);
	s = strob_str(tmp);
	while((tok = strob_strstrtok(tmp, s, TARGET_DELIM))) {
		user = (char*)NULL;
		host = (char*)NULL;
		*terminal_host = (char*)NULL;
		
		s = NULL;
		commat = strchr(tok, '@');
		if (commat) {
			user = tok;
			host = commat + 1;
			*commat = '\0';
		} else {
			host = tok;
		}	

		if (strchr(host, (int)':')) {
			*strchr(host, (int)':') = '\0';
		}

		if (strlen(host)) {
			*terminal_host = strdup(host);
			if (cmdcount == 0) {
				safe_shcmd_add_arg(kmd, ssh_command);
				safe_shcmd_add_arg(cmd, ssh_command);
			} else {
				add_host_message(do_imsg, cmd,
						parent_host, cmdcount);
				add_host_kill(do_imsg, kmd, parent_host,
								cmdcount);
				safe_shcmd_add_arg(kmd, remote_ssh_command);
				safe_shcmd_add_arg(cmd, remote_ssh_command);
			}
			parent_host = strdup(host);
			if (is_ssh) {
				if (tty_option) {
					safe_shcmd_add_arg(cmd, tty_option);
					safe_shcmd_add_arg(cmd, tty_option);
					safe_shcmd_add_arg(kmd, tty_option);
					safe_shcmd_add_arg(kmd, tty_option);
				}
				if (is_multihop) {
					if (do_forward_auth_agent) {
						if (cmdcount+1 < nhops) {
							safe_shcmd_add_arg(cmd, "-A");
							safe_shcmd_add_arg(kmd, "-A");
						} else {
							;
							/* don't apply -A to the last host
							   because it is not needed there */
						}
					}
				}
				if (sshoption && strlen(sshoption)) {
					strob_strcpy(tmp2, "-");
					strob_strcat(tmp2, sshoption);
					safe_shcmd_add_arg(kmd, strob_str(tmp2));	
					safe_shcmd_add_arg(cmd, strob_str(tmp2));	
				}
			}
			cmdcount++;
		} else if (strlen(host) == 0 && icmdcount == 0) {
			/*
			* localhost, empty host, user specified.
			*/
			safe_shcmd_add_arg(cmd, "/bin/sh");
		} else {
			/*
			* error.
			*/
			return -1;
		}

		if (user) {
			safe_shcmd_add_arg(cmd, "-l");
			safe_shcmd_add_arg(cmd, user);
			safe_shcmd_add_arg(kmd, "-l");
			safe_shcmd_add_arg(kmd, user);
		}
		
		if (strlen(host)) {
			parse_host_port(host, cmd, kmd);
			/*
			safe_shcmd_add_arg(kmd, host);
			safe_shcmd_add_arg(cmd, host);
			*/
		} else if (strlen(host) == 0 && icmdcount == 0) {
			safe_shcmd_add_arg(cmd, "-c");
		}	
		icmdcount++;
	}
	if (icmdcount == 0) {
		safe_shcmd_add_arg(cmd, "/bin/sh");
		safe_shcmd_add_arg(cmd, "-c");
	}
	free(remote_ssh_command);
	strob_close(tmp);
	strob_close(tmp2);
	return cmdcount;
}

int
swssh_assemble_ssh_cmd(SHCMD * shcmd, STRAR * cmdlist,
					STRAR * delimlist, int nhops)
{
	char * cmd1;
	int i = 0;

	g_did_get_command = 0;
	while ((cmd1=strar_get(cmdlist, i))) {
		safe_shcmd_add_arg(shcmd, cmd1);
		i++;
	}
	return 0;
}
