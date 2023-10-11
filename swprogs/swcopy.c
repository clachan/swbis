/*  swcopy.c -- The swbis swcopy utility.

 Copyright (C) 2003,2004,2005,2006,2008 Jim Lowe
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

#define FILENEEDDEBUG 1 /* controls the E_DEBUG() statements */
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "swlib.h"
#include "vplob.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swi.h"
#include "swcommon.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swutilname.h"
#include "swicol.h"
#include "swevents.h"
#include "swutillib.h"
#include "atomicio.h"
#include "swmain.h"  /* this file should be included by the main program file */
#include "swproglib.h"
#include "globalblob.h"

#define DEVNULL			"/dev/null"
#define PROGNAME		"swcopy"
#define SW_UTILNAME		PROGNAME
#define SWPARSE_AT_LEVE		0
#define SWPROG_VERSION 		SWCOPY_VERSION
#define VERBOSE_LEVEL2		2
#define VERBOSE_LEVEL3		3

#define KILL_PID		SWSSH_TRACK_PID

#define WOPT_LAST				236
#define MAX_CONTROL_MESG_LEN			2000
#define SWC_PID_ARRAY_LEN			30

static char progName[] = PROGNAME;
static char * CHARTRUE = "True";
static char * CHARFALSE = "False";
static GB g_blob;
static GB * G;

static 
void 
version_string(FILE * fp)
{
	fprintf(fp,  
"%s (swbis) version " SWCOPY_VERSION "\n",
		progName);
}

static
void 
version_info(FILE * fp)
{
	version_string(fp);
	/* fprintf(fp,  "\n"); */
	fprintf(fp,  "%s",
"Copyright (C) 2003,2004,2005,2006,2007 Jim Lowe\n"
"Portions are copyright 1985-2000 Free Software Foundation, Inc.\n"
"This software is distributed under the terms of the GNU General Public License\n"
"and comes with NO WARRANTY to the extent permitted by law.\n"
"See the file named COPYING for details.\n");
}

static
UINFORMAT *
get_uinformat(XFORMAT * xformat)
{
	if (xformat_get_swvarfs(xformat))
		return
		swvarfs_get_uinformat(xformat_get_swvarfs(xformat));
	else 
		return NULL;
}

static
int
is_target_path_a_directory(char * path)
{
	if (strcmp(path, ".") == 0 ||
		strlen(path) == 0 ||
		path[strlen(path) -1] == '/' ||
		path[strlen(path) -1] == '.' 
	) return 1;
	return 0;
}

static
int
does_have_sw_selections(VPLOB * swspecs)
{
	return vplob_get_nstore(swspecs) > 0 ? 1 : 0;
}


static
int 
usage(FILE * fp, 
		char * pgm, 
		struct option  lop[], 
		struct ugetopt_option_desc helpdesc[])
{
	
fprintf(fp, "%s",
"swbis 'swcopy' copies a directory or serial archive from a source location\n"
"to a target location.  The source and target may be different hosts.\n"
"A source distribution must be a valid posix package unless the --no-audit\n"
"option is used.\n"
);
	
	fprintf(fp, "%s",
"\n"
"Usage: swcopy [-p] [-f file] [-s source] [-x option=value] [-X options_file]\\\n"
"              [-W option[=value]] [-t targetfile] [impl_specific_options]\\\n"
"                 [software_selections][@target]\n"
);

fprintf(fp, "%s",
"\n"
"Examples:\n"
"       Copy the  distribution foo.tar.gz at 192.168.1.10 :\n"
"             swcopy -s foo.tar.gz @192.168.1.10:/root\n"
"\n"
"       Read a archive on stdin, audit it and write to stdout :\n"
"             swcopy -s - @- \n"
"\n"
"       Read a archive on stdin, audit it and install it in ./ :\n"
"             swcopy -s - @. \n"
"       Decompress/decrypt all layers of compression/encryption of a\n"
"       tar archive to stdout:\n"
"             swcopy --extract -s- @- \n"
);

fprintf(fp, "%s",
"\n"
"If no options are given, an archive is read from the default source directory.\n"
"and installed on the default target directory.\n"
"\n"
"By default, without external influences of the swdefaults file, swcopy\n"
"reads an archive on stdin, decodes and audits it, and writes the archive\n"
"to stdout. Use the --no-audit option to copy arbitrary data.\n"
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
"   --show-options  Show the extended options and exit\n"
"   --show-options-files   Show the options files that will be parsed.\n"
"   --login      login to the target, used for testing ssh.\n"
"   --allow-rpm   Allow translation and copying of RPMs as a posix package.\n"
"   --unrpm       Turns on --allow-rpm and --audit.\n"
"   --to-sw       same as --unrpm.\n"
"\n"
"   Archive Treatment Options:\n"
"\n"
"   --extract   Force installation as tar archive\n"
"   --no-extract  Force installation as a regular file\n"
"   --create   Force create as tar archive\n"
"   --defacto-layout  Enforce a constant leading path in\n"
"                   package archive. (not implemented)\n"
"\n"
"   Compression/Encryption Options:\n"
"\n"
"   --uncompress  Leave output archive uncompressed\n"
"   --symmetric\n"
"   --encrypt-for-recipient NAME\n"
"   --xz          Compress output using the xz utility\n"
"   --lzma        Compress output using the lzma utility\n"
"   --gzip        Compress output using the gzip utility\n"
"   --bzip2       Compress output using the bzip2 utility\n"
"\n"
"   Misc Options:\n"
"\n"
"   --noop       Legal option with no effect\n"
"   --no-audit    Do not audit the archive, just copy it.\n"
"   --audit       Do audit the archive.\n"
"   --block-size  Same as -b N, where N is octets.\n"
"\n"
"  Tar Reading/Writing Utility Selection\n"
"\n"
"   --pax-command={tar|pax|star|gtar}\n"
"   --pax-read-command={tar|pax|star|gtar}\n"
"   --local-pax-read-command={tar|pax|star|gtar}\n"
"   --remote-pax-read-command={tar|pax|star|gtar}\n"
"              pax  = \"pax -r\"\n"
"              star = \"star xpUf -\"\n"
"              tar  = \"tar xpf -\"\n"
"              gtar = \"tar xpf - --overwrite\"\n"
"   --pax-write-command={tar|pax|star|gtar}\n"
"   --local-pax-write-command={tar|pax|star|swbistar}\n"
"   --remote-pax-write-command={tar|pax|star|swbistar}\n"
"              pax  = \"pax -w\"\n"
"              star = \"star cf - -H ustar\"\n"
"              tar  = \"tar cf -\"\n"
"              gtar = \"gtar cf -\"\n"
"              swbistar = \"" SWBISLIBEXECDIR "/swbis/swbistar\"\n"
" 	The default is \"tar\"\n"
"\n"
"  Target and Remote connection options\n"
"\n"
"   -A,--A,--enable-ssh-agent-forwarding\n"
"      --a,--disable-ssh-agent-forwarding\n"
"   --remote-shell=NAME  NAME is the client \"rsh\" or \"ssh\"(default).\n"
"   --shell-command={detect|posix|sh|bash|ksh}  detect is the default\n"
"   --no-getconf  Don't use the getconf utility to set the PATH variable on the\n"
"               target host. This sets the remote command to be '/bin/sh -s'\n"
"               instead of the default \"PATH=`getconf PATH` sh -s\"\n"
"   --use-getconf  This causes getconf to be used.  Opposite to --no-getconf.\n"
"   --progress-bar-fd={1|2}  1 (stdout) is the default\n"
"   --no-pty      Disable pseudo-tty allocation (default).\n"
"   --pty         Force pseudo-tty allocation (really never used).\n"
"\n"
"  Verbosity and Debugging options\n"
"\n"
"   --quiet-progress      disable progress bar.\n"
"   --show-progress-bar   enable progress bar.\n"
"   --no-defaults  Do not read any defaults files.\n"
"   --no-remote-kill  Do not execute a kill command. Normally a kill command\n"
"               is issued when signals TERM, PIPE, and INT are received for\n"
"               targets where nhops > 1.\n"
"   --target-script-name=NAME  write the internal stdin script to NAME.\n"
"   --source-script-name=NAME  write the internal stdin script to NAME.\n"
"\n"
"   The following are mainly for development testing:\n"
"\n"
"   testnum={1|0}  Test selection mode.\n"
"   pty-fork-version={no-pty|pty|pty2}\n"
);
      
	swc_helptext_target_syntax(fp);

fprintf(fp, "%s",
"\n"
"Copying Semantics:\n"
"Similar to scp(1) and cp(1) except: non-existing directories in a target\n"
"are created, and a source directory cannot be renamed (although a regular\n"
"file can be), and a single source file is specified for one or more targets\n"
"instead of vice-versa for scp(1) and cp(1).\n"
"\n"
"If  a  target  directory  on the host does not exist it will be created\n"
"using mkdir -p using the file creation mask of the  originating  swcopy\n"
"process.  A trailing slash in the target spec signifies that the last\n"
"path component should be a directory.\n"
"A source spec that is a directory  will be created on the target as a\n"
"directory with the same name.\n"  
"If the source spec is standard input then the existence of directories\n"
"in the target host and/or a trailing  slash  in the target spec path\n"
"determine if the source file will be created as a regular file or read\n"
"as a portable archive.\n"
"If the source spec is a regular file, the source basename will be used as\n"
"the basename in the target if the last target path component is a directory\n"
"or ends in a slash '/', otherwise, the  target basename is the last path\n"
"component of the target spec.  The implementation option --extract biases\n"
"these rules to install using the archive reading command (e.g.  pax -r).\n"
"swcopy does not modify the fileset state attribute during transit.\n"
"swcopy does not merge distributions, serial archives are overwritten and\n"
"directories are copied into by the default operation of the archive reading\n"
"utility specified in the defaults files, which is pax if no defaults file\n"
"is read.\n"
);

fprintf(fp, "%s",
"\n"
"POSIX Extended Options:\n"
"        File : <libdir>/swbis/swdefaults\n"
"               ~/.swdefaults\n"
"        Override values in defaults file.\n"
"        Syntax : -x option=option_argument [-x ...]\n"
"            or : -x \"option=option_argument  ...\"\n"
"   autoselect_dependencies     = false      # Not Implemented\n"
"   compress_files              = false      # Not Implemented\n"
"   compression_type            = none       # Not Implemented\n"
"   distribution_source_directory   = -\n"
"   distribution_target_directory   = -\n"
"   enforce_dependencies        = false      # Not Implemented\n"
"   enforce_dsa                 = false       # Not Implemented\n"
"   logfile                     = /var/lib/sw/swcopy.log #Not Implemented\n"
"   loglevel                    = 0          # Not Implemented\n"
"   recopy                      = false      # Not Implemented\n"
"   select_local		= false      # Not Implemented\n"
"   uncompress_files		= false      # Not Implemented\n"
"   verbose			= 1\n"
"\n"
"Swbis Extended Options:\n"
"   These can be accessed with the -x option.\n"
"        File : <libdir>/swbis/swbisdefaults\n"
"               ~/.swbis/swbisdefaults\n"
"  swcopy.swbis_no_getconf		 = false # getconf is used\n"
"  swcopy.swbis_shell_command		 = detect\n"
"  swcopy.swbis_no_remote_kill		 = false\n"
"  swcopy.swbis_quiet_progress_bar       = false\n"
"  swcopy.swbis_no_audit                 = false\n"
"  swcopy.swbis_local_pax_write_command  = pax\n"
"  swcopy.swbis_remote_pax_write_command = pax\n"
"  swcopy.swbis_local_pax_read_command   = pax\n"
"  swcopy.swbis_remote_pax_read_command  = pax\n"
"  swcopy.swbis_allow_rpm                = false\n"
"  swcopy.swbis_remote_shell_client      = ssh\n"
"  swcopy.swbis_forward_agent            = true\n"
"   # Choices that swcopy supports : {pax|tar|gtar|star}\n"
);
fprintf(fp , "%s", "\n");
	version_string(fp);
	fprintf(fp , "\n%s", 
        "Report bugs to " REPORT_BUGS "\n"
	);
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
		fprintf(stderr, "%s\n", swverid_print(swverid, tmp));
	}
	strob_close(tmp);
	return 0;
}

static
char *
get_target_script_pid(STROB * control_message)
{
	char *s;
	char *p;
	char *ret;
	char *ed;

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

static
int
cowardly_check(char * sourcepath,  char * targetpath)
{
	struct stat st1, st2;
	STROB * tmp;
	STROB * tmp2;
	char *c = NULL;
	char *s;

	if (strcmp(targetpath, "-") == 0 || 
		strcmp(sourcepath, "-") == 0) {
		return 0;
	}

	tmp = strob_open(10);
	tmp2 = strob_open(10);
	
	if (strcmp(targetpath, sourcepath) == 0)
		return 1;

	swlib_dirname(tmp, targetpath);
	strob_strcpy(tmp2, sourcepath);
	swlib_squash_trailing_slash(strob_str(tmp2));
	if (strcmp(strob_str(tmp), strob_str(tmp2)) == 0)
		return 1;

	swlib_dirname(tmp, sourcepath);
	strob_strcpy(tmp2, targetpath);
	swlib_squash_trailing_slash(strob_str(tmp2));
	if (strcmp(strob_str(tmp), strob_str(tmp2)) == 0)
		return 1;

	c = strdup(sourcepath);
	if (strcmp(targetpath, ".") == 0) {
		s = strrchr(c, '/');
		if (!s) 
			return 0;
		*s = '\0';
	}
	if (stat(c, &st1) < 0) 
		return 0;
	if (stat(targetpath, &st2) < 0) 
		return 0;
	if (st1.st_ino == st2.st_ino) {
		return 1;
	}
	free(c);
	strob_close(tmp);
	return 0;
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
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec,
					swc_get_stderr_fd(G), "tty_raw error\n");
		}
	}
}

static
void
safe_sig_handler(int signum)
{
	switch(signum) {
		case SIGTERM:
			if (G->g_do_progressmeter) alarm(0);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, &G->g_logspec,
					swc_get_stderr_fd(G), "Caught SIGTERM\n");
			G->g_signal_flag = signum;
			break;
		case SIGINT:
			if (G->g_do_progressmeter) alarm(0);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, &G->g_logspec,
						swc_get_stderr_fd(G), "Caught SIGINT\n");
			G->g_signal_flag = signum;
			break;
		case SIGPIPE:
			if (G->g_do_progressmeter) alarm(0);
			swlib_doif_writef(G->g_verboseG,
					SWC_VERBOSE_IDB, &G->g_logspec, swc_get_stderr_fd(G), 
							"Caught SIGPIPE\n");
			G->g_signal_flag = signum;
			break;
	}
}

static
void
main_sig_handler(int signum)
{
	int fd;
	switch(signum) {
		case SIGTERM:
		case SIGINT:
		case SIGPIPE:
			swlib_doif_writef(G->g_verboseG,
					SWC_VERBOSE_IDB, &G->g_logspec, swc_get_stderr_fd(G),
					"Executing handler for signal %d.\n",
					 signum);
			G->g_signal_flag = 0;
			if (G->g_do_progressmeter) alarm(0);
			swgp_signal_block(SIGTERM, (sigset_t *)NULL);
			swgp_signal_block(SIGINT, (sigset_t *)NULL);
			/* swgp_signal_block(SIGPIPE, (sigset_t *)NULL); */
			swlib_tty_atexit();
			fd = G->g_target_fdar[0]; if (fd >= 0) close(fd);
			fd = G->g_target_fdar[1]; if (fd >= 0) close(fd);
			fd = G->g_source_fdar[0]; if (fd >= 0) close(fd);
			fd = G->g_source_fdar[1]; if (fd >= 0) close(fd);
			swc_gf_xformat_close(G, G->g_xformat);
			swlib_kill_all_pids(G->g_pid_array + SWC_PID_ARRAY_LEN, 
						G->g_pid_array_len, 
						SIGTERM, 
						G->g_verboseG);
			swlib_wait_on_all_pids(G->g_pid_array, 
						G->g_pid_array_len, 
						G->g_status_array, 
						0, 
						G->g_verboseG);
			swc_shutdown_logger(G, SIGABRT);
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
checkpathname(char * s) {
	/* do nothing, squash the warning */
	if (s || !s) return 0;
	return 0;
}

static
int
swc_write_target_copy_script(
		int ofd, 
		char * fp_targetpath, 
		STROB * control_message, 
		char * sourcepath,
		STRAR * sourcepath_list,
		int delaytime,
		int keep_old_files,
		int nhops,
		int vlv,
		int do_extract,
		int no_extract,
		char * pax_read_command_key,
		char * hostname,
		char * blocksize
		)
{
	int retval = 0;
	int ret;
	int is_target_trailing_slash;
	int is_serial_archive_file;
	int cmd_verbose = 0;
	char * pax_read_command;
	STROB * buffer;
	STROB * buffer_new;
	STROB * tmp;
	STROB * tmp1;
	STROB * tmp2;
	STROB * tmpexit;
	STROB * to_devnull;
	STROB * set_vx;
	STROB * subsh, * subsh2;
	char * xx;
	char * target_dirname, *source_basename, *target_basename;
	char * targetpath;
	char * x_targetpath;
	char * x_basename;
	char * x_targetdir;
	char umaskbuf[10];

	SWLIB_ASSERT(fp_targetpath != NULL);
	targetpath = strdup(fp_targetpath);

	if (strstr(targetpath, "..") == targetpath) { return 0; }
	if (strstr(sourcepath, "..") == sourcepath) { return 0; }
	if (strstr(targetpath, "../")) { return 0; }
	if (strstr(sourcepath, "../")) { return 0; }
	if (swlib_is_sh_tainted_string(targetpath)) { return 0; }
	if (swlib_is_sh_tainted_string(sourcepath)) { return 0; }
	
	buffer = strob_open(1);
	buffer_new = strob_open(64);
	tmp = strob_open(10);
	tmp1 = strob_open(10);
	tmp2 = strob_open(10);
	tmpexit = strob_open(10);
	to_devnull = strob_open(10);
	set_vx = strob_open(10);
	subsh = strob_open(10);
	subsh2 = strob_open(10);

	if (vlv <= SWC_VERBOSE_0 && keep_old_files == 0) {
		strob_strcpy(to_devnull, "2>/dev/null");
	}

	if (vlv >= SWC_VERBOSE_5 && keep_old_files == 0) {
		cmd_verbose = SWC_VERBOSE_1;
	}
	
	if (vlv >= SWC_VERBOSE_SWIDB) {
		strob_strcpy(set_vx, "set -vx\n");
	}

	pax_read_command = swc_get_pax_read_command(G->g_pax_read_commands,
					pax_read_command_key, 
					cmd_verbose, 
					keep_old_files, DEFAULT_PAX_R);

	if ((strstr(strob_str(control_message), 
			SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY)
		 || do_extract) && no_extract == 0
	) {
		/* The source was a directory, unpack it with pax. */
		is_serial_archive_file = 0;
	} else {
		/* Default */
		is_serial_archive_file = 1;
	}

	swc_print_umask(umaskbuf, sizeof(umaskbuf));

	is_target_trailing_slash = (strlen(targetpath) && 
				targetpath[strlen(targetpath) - 1] == '/');

	if (strlen(targetpath) && strcmp(sourcepath, "-") == 0 && 
						is_target_trailing_slash) {
		/* This handles the case of stdin source and a target with
		  a trailing / which will be interpreted to mean unpack the
		  serial_archive file with pax in that directory.  */
		is_serial_archive_file = 0;
	}
	swlib_doif_writef(vlv, SWC_VERBOSE_IDB, &G->g_logspec,  
		swc_get_stderr_fd(G), 
			"swc_write_target_copy_script : source_type [%s]\n",
						 strob_str(control_message));

	swlib_doif_writef(vlv, SWC_VERBOSE_IDB, &G->g_logspec,
		swc_get_stderr_fd(G), 
		"swc_write_target_copy_script"
		" : is_serial_archive_file = [%d]\n",
					 is_serial_archive_file);

	/* Squash the trailing slash.  */
	swlib_squash_double_slash(targetpath);
	if (strlen(targetpath) > 1) {
		if (targetpath[strlen(targetpath) - 1] == '/' ) {
			targetpath[strlen(targetpath) - 1] = '\0';
		}
	}

	swlib_dirname(tmp, targetpath);
	target_dirname = strdup(strob_str(tmp));
	swlib_basename(tmp, sourcepath);
	source_basename = strdup(strob_str(tmp));
	swlib_basename(tmp, targetpath);
	target_basename = strdup(strob_str(tmp));

	if (is_serial_archive_file == 0) {
		/* Unpack the archive using pax
		   The source control string was 
		   SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY */
		x_targetpath = targetpath;
		
		/* Sanity check */
		if (checkpathname(x_targetpath)) {
			strob_sprintf(tmpexit, 0, "exit 2;\n");
		}
		retval = 2;

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
		"%s\n" /* Session Begins */
		"%s\n" /* Analysis Begins */
		"%s"
		"%s"
		"	sw_retval=0\n"
		"%s"
		"	umask %s\n"
		"	blocksize=\"%s\"\n"
		"	xtarget=\"%s\"\n"
		"	wcwd=`pwd`\n"
		"	if test -p \"$xtarget\" -o -b \"$xtarget\" -o -c \"$xtarget\"; then\n"
		"\t\t%s\n" /* Analysis Ends */
		"\t\t%s\n" /* Execution Begins */
		"		dd ibs=512 obs=\"$blocksize\" of=\"$xtarget\" %s\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
		"		%s\n" /* Execution Ends */
		"		if test \"$sw_retval\" != \"0\"; then\n"
		"			 dd count=1 of=/dev/null 2>/dev/null\n"
		"		fi\n"
		"	else\n"
		"		cd \"$xtarget\" 2>/dev/null\n"
		"		sw_retval=$?\n"
		"		if test \"$xtarget\" = '.'; then\n"
		"			sw_retval=1\n"
		"		else\n"
		"			if test $sw_retval !=  \"0\"; then\n"
		"				mkdir -p \"$xtarget\"\n"
		"				sw_retval=$?\n"
		"			else\n"
		"				 cd \"$wcwd\"; sw_retval=1;\n"
		"			fi\n"
		"		fi\n"
		"		if test \"$sw_retval\" = \"0\"; then\n"
		"\t\t\t%s\n"
		"		fi\n"
		"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
		"		%s\n" /* Analysis Ends *//* Execution Begins */
		"		cd \"$xtarget\" && %s && %s\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
		"		%s\n" /* Execution Ends */
		"		if test \"$sw_retval\" != \"0\"; then\n"
		"			 dd count=1 of=/dev/null 2>/dev/null;\n"
		"		fi\n"
		"	fi\n"
		"sleep %d\n"
		"%s\n" /* Session Ends */
		"%s\n"
		,
		TEVENT(2, vlv, SW_SESSION_BEGINS, ""),
		TEVENT(2, vlv, SW_ANALYSIS_BEGINS, ""),
		swicol_subshell_marks(subsh, "target", 'L', nhops, vlv),
		strob_str(tmpexit),
		strob_str(set_vx),
		umaskbuf,
		blocksize,
		x_targetpath, 
		TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
		TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
		strob_str(to_devnull),
		TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval"),
		TEVENT(2, vlv, SW_SOC_CREATED, x_targetpath),
		TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
		TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
		pax_read_command,
		TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval"),
		delaytime,
		TEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
		swicol_subshell_marks(subsh2, "target", 'R', nhops, vlv)
		);

		xx = strob_str(buffer_new);
		ret = atomicio((ssize_t (*)(int, void *, size_t))write, ofd, xx, strlen(xx));
		if (ret != (int)strlen(xx)) {
			return 1;
		}

	} else {
		/* Copy as a archive file (regular file).
		   The source control string was 
		   *not* SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY */
		
		if ( strcmp(sourcepath, "-") == 0 &&
			(strcmp(fp_targetpath, ".") == 0 || 
			is_target_trailing_slash)
		  )  {
			/* This causes the archive not to be compressed
			   because pax is going to be used.  */
			retval = 2;
		} else {
			/* Normal, recompress if req'd to preserve the
			   original compression state.  */
			retval = 1;
		}
		if (swlib_is_sh_tainted_string(target_basename)) { return 0; }
		if (swlib_is_sh_tainted_string(source_basename)) { return 0; }
		if (swlib_is_sh_tainted_string(target_dirname)) { return 0; }
		if (swlib_is_sh_tainted_string(targetpath)) { return 0; }

		if (strcmp(sourcepath, "-") == 0) 
		{
			x_basename = target_basename;
			x_targetdir = target_dirname;
			strob_sprintf(tmp1, 0, 
			"	elif test -d '%s'; then\n"
			"		%s\n"  /* ANALYSIS ENDS */
			"		cd '%s' && %s && %s\n"
			"		sw_retval=$?\n"
			"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
			"\t\t%s\n"
			,
			targetpath,
			TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
			targetpath, 
			TEVENT(2, vlv,  SW_EXECUTION_BEGINS, ""),
			pax_read_command,
			TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval")
			);

			strob_sprintf(tmp2, 0, 
			"	elif test -d '%s'; then\n"
			"		%s\n"  /* ANALYSIS ENDS */
			"		cd '%s' && %s && dd of='%s' %s\n"
			"		sw_retval=$?\n"
			"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
			"		case \"$sw_retval\" in 0);;\n"
			"			*)\n"
			"\t\t%s\n"
			"			;;\n"
			"		esac\n"
			"\t\t%s\n"
			,
			x_targetdir, 
			TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
			x_targetdir, 
			TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
			x_basename,
			strob_str(to_devnull),
			TEVENT(2, vlv, SW_ACCESS_DENIED, ""),
			TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval")
			);
		} else {
			x_basename = source_basename;
			x_targetdir = targetpath;

			if (!is_target_trailing_slash) {
			strob_sprintf(tmp2, 0, 
			"	elif test -d '%s'; then\n"
			"		%s\n"  /* ANALYSIS ENDS */
			"		cd '%s' && %s && dd of='%s' %s\n"
			"		sw_retval=$?\n"
			"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
			"		case \"$sw_retval\" in 0);;\n"
			"			*)\n"
			"\t\t%s\n"
			"			;;\n"
			"		esac\n"
			"\t\t%s\n"
			,
			target_dirname,
			TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
			target_dirname,
			TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
			target_basename,
			strob_str(to_devnull),
			TEVENT(2, vlv, SW_ACCESS_DENIED, ""),
			TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval")
			);
			} else {
				strob_sprintf(tmp2, 0,  "");
			}

			strob_sprintf(tmp1, 0, 
			"	elif test -d '%s'; then\n"
			"		%s\n"  /* ANALYSIS ENDS */
			"		cd '%s' && %s && dd of='%s' %s\n"
			"		sw_retval=$?\n"
			"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
			"\t\t%s\n"
			,
			x_targetdir, 
			TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
			x_targetdir,
			TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
			x_basename,
			strob_str(to_devnull),
			TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval")
			);
		}

		/*
		* Sanity check
		*/
		if (	swc_checkbasename(target_basename) ||
			swc_checkbasename(x_basename) 
		) {
			strob_sprintf(tmpexit, 0, "exit 2;\n");
		}
		if (swlib_is_sh_tainted_string(targetpath)) { return 0; }
		if (swlib_is_sh_tainted_string(x_targetdir)) { return 0; }
		if (swlib_is_sh_tainted_string(x_basename)) { return 0; }

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
		"	echo " SWBIS_TARGET_CTL_MSG_125 ": " KILL_PID "\n"
		"%s\n"
		"%s\n"
		"%s"
		"%s"
		"	sw_retval=0\n"
		"%s"
		"	umask %s\n"
		"	blocksize=\"%s\"\n"
		"	targetpath=\"%s\"\n"
		"	xtargetdir=\"%s\"\n"
		"	xbasename=\"%s\"\n"
		"	wcwd=`pwd`\n"
		"	if test -p \"$targetpath\" -o -b \"$targetpath\" -o -c \"$targetpath\"; then\n"
		"\t\t%s\n"  /* Analysis ends */
		"\t\t%s\n"
		"		dd obs=\"$blocksize\" of=\"$targetpath\" %s\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
		"		if test $sw_retval != \"0\"; then\n"
		"			dd count=1 of=/dev/null 2>/dev/null\n"
		"\t\t%s\n"
		"		fi\n"
		"\t\t%s\n"
		"%s"   /* strob_str(tmp1) */
		"%s"   /* strob_str(tmp2) */
		"	else\n"
		"		cd \"$xtargetdir\" 2>/dev/null\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
		"		if test \"$xtargetdir\" = '.'; then\n"
		"			sw_retval=1\n"
		"		else\n"
		"			if test $sw_retval != \"0\"; then\n"
		"				mkdir -p \"$xtargetdir\"\n"
		"				sw_retval=$?\n"
		"			else\n"
		"				cd \"$wcwd\"; sw_retval=1\n"
		"			fi\n"
		"		fi\n"
		"		if test $sw_retval = \"0\"; then\n"
		"			%s\n"  /* SOC_CREATED */
		"		fi\n"
		"\t\t%s\n"  /* ANALYSIS_ENDS */
		"		cd \"$xtargetdir\" && %s && dd of=\"$xbasename\" %s\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) ;; *) sw_retval=1; ;; esac\n"
		"		if test $sw_retval != \"0\"; then\n"
		"\t\t%s\n" /* ACCESS_DENIED */
		"		fi\n"
		"\t\t%s\n"
		"	fi\n"
		"sleep %d\n"
		"%s\n"
		"%s\n"
		,
		TEVENT(2, vlv, SW_SESSION_BEGINS, ""),
		TEVENT(2, vlv, SW_ANALYSIS_BEGINS, ""),
		swicol_subshell_marks(subsh, "target", 'L', nhops, vlv),
		strob_str(tmpexit),
		strob_str(set_vx),
		umaskbuf,
		blocksize,
		targetpath,
		x_targetdir,
		x_basename, 
		TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
		TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
		strob_str(to_devnull),
		TEVENT(2, vlv, SW_ACCESS_DENIED, ""),
		TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval"),
		strob_str(tmp1),
		strob_str(tmp2),
		TEVENT(2, vlv, SW_SOC_CREATED, x_targetdir),
		TEVENT(2, vlv, SW_ANALYSIS_ENDS, "status=0"),
		TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
		strob_str(to_devnull),
		TEVENT(2, vlv, SW_ACCESS_DENIED, ""),
		TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval"),
		delaytime,
		TEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
		swicol_subshell_marks(subsh2, "target", 'R', nhops, vlv)
		);

		xx = strob_str(buffer_new);
		ret = atomicio((ssize_t (*)(int, void *, size_t))write, ofd, xx, strlen(xx));
		if (ret != (int)strlen(xx)) {
			return 1;
		}
	}
	free(source_basename);
	free(target_dirname);
	free(target_basename);
	strob_close(tmp);
	strob_close(tmp1);
	strob_close(tmp2);
	strob_close(tmpexit);
        if (G->g_target_script_name) {
                swlib_tee_to_file(G->g_target_script_name, -1, strob_str(buffer_new), -1, 0);
        }
	strob_close(buffer);
	strob_close(buffer_new);
	strob_close(to_devnull);
	strob_close(set_vx);

	if (ret <= 0) retval = 0;
	swlib_doif_writef(vlv, SWC_VERBOSE_IDB, 
		&G->g_logspec, swc_get_stderr_fd(G), 
			"swc_write_target_copy_script : retval = [%d]\n",
				retval);
	/*
	   retval:
	 	0 : Error
	 	1 : OK
	 	2 : OK   targetpath is "." or unpack script is being used.
	 		 or target path ends in a "/" */
	return retval;
}

static
pid_t
run_target_script(
		int local_stdout,
		int target_pipe,
		char * source_path,
		char * target_path,
		STROB * source_control_message,
		int keep_old_files,
		int nhops,
		int do_extract,
		int no_extract,
		char * pax_read_command_key,
		sigset_t *fork_blockmask,
		sigset_t *fork_defaultmask,
		char * hostname,
		char * blocksize
		) 
{
	pid_t write_pid;

	if (local_stdout) {
		/* fork not required.  */
		return (0);
	}
	
	write_pid = swndfork(fork_blockmask, fork_defaultmask);
	if (write_pid == 0) {
		int ret_target = 1;
		/* Write the target scripts.  */
		ret_target = swc_write_target_copy_script(
			target_pipe, 
			target_path,
			source_control_message,
			source_path,
			NULL, 
			SWC_SCRIPT_SLEEP_DELAY, 
			keep_old_files, 
			nhops, 
			G->g_verboseG,
			do_extract, 
			no_extract, 
			pax_read_command_key,
			hostname,
			blocksize);
		/* 0 is error
		  1 or 2 is OK and carry meaning.  */
		if (ret_target < 0) ret_target = 0;
		_exit(ret_target);
	} else if (write_pid < 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"fork error : %s\n", strerror(errno));
	}
	return write_pid;
}

static
int
transform_output(
	VPLOB * compression_layers,
	int target_fdar1,
	sigset_t *fork_blockmask,
	sigset_t *fork_defaultmask,
	uintmax_t * pstatbytes
	)
{

	/* FIXME: to get the progressmeter to give correct stats for
	  auditted compressed archives, then pstatbytes must be updated
	  here based on the output of the compressor program.  */

	int retfd;
	int pv[2];
	pid_t pid;		

	pid = 0;
	pv[0] = -1;
	pv[1] = -1;

	if (
		compression_layers != NULL
	) {
		pipe(pv);
		pid = fork();
		if (pid == 0) {
			int cmdret;
			SHCMD ** command_vector;
			SHCMD ** cmdv;
			SHCMD * last_command;
			close(pv[1]);
			command_vector = (SHCMD**)vplob_get_list(compression_layers);
			shcmd_set_srcfd(command_vector[0], pv[0]);
			last_command = shcmd_get_last_command(command_vector);
			SWLIB_ASSERT(last_command != NULL);
			shcmd_set_dstfd(last_command, target_fdar1);
			shcmd_cmdvec_exec(command_vector);
			cmdret = shcmd_cmdvec_wait2(command_vector);

			/*  Now check the exit status of every command
			    to avoid a false indication of success. */

			cmdv = command_vector;
			while (*cmdv && cmdret == 0) {
				cmdret = shcmd_get_exitval(*cmdv);
				if (cmdret == SHCMD_UNSET_EXITVAL) cmdret = 127;
				cmdv++;
			}
			_exit(cmdret);
		} else if (pid > 0) {
			/* parent */
			close(target_fdar1);
			close(pv[0]);
			retfd = pv[1];
		} else {
			exit(1); // fork failed;
		}
	} else {
		retfd = target_fdar1;
	}
	return retfd;
}

static
int
run_audit(SWI * swi,
	int target_fd1,
	int source_fd0,
	char * source_path,
	int do_extract,
	int no_extract,
	int otarallow,
	int compression_off,
	uintmax_t * pstatbytes,
	int do_progressmeter,
	int source_file_size,
	char * working_arg,
	sigset_t *fork_blockmask,
	sigset_t *fork_defaultmask,
	struct extendedOptions * opta
	)
{
	int tfd1;
	int gpipe[2];
	int ztype;
	int ret;
	char * command = NULL;
	VPLOB * compression_layers = NULL;
	SHCMD * compressor;
	XFORMAT * xformat;
	UINFORMAT * uin;
	void (*alarm_handler)(int) = NULL;
	sigset_t pendmask;
	off_t file_size = (off_t)source_file_size;

	gpipe[0] = -1;
	gpipe[1] = -1;

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, &G->g_logspec, swc_get_stderr_fd(G),
					"run audit: start setup_xformat\n");

	xformat = swi->xformatM;
	/* uin = swi->uinformatM; SAME as get_uinformat(xformat) ??? */

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
				"run audit: finished setup_xformat\n");
	uin = get_uinformat(xformat); 
	ztype = uinfile_get_ztype(uin);

	if (compression_off == 0 && do_extract == 0) {
		if (ztype == UINFILE_COMPRESSED_GZ) {
			file_size = 0;  /* If compressed the progressmeter %complete does not
					   work */
			command = "gzip";
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gzip");
			shcmd_add_arg(compressor, "--force");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);

		} else if (ztype == UINFILE_COMPRESSED_BZ2 ) {
			file_size = 0;  /* If compressed the progressmeter %complete does not
					   work */
			command = "bzip2";

			compressor = shcmd_open();
			shcmd_add_arg(compressor, "bzip2");
			shcmd_add_arg(compressor, "--force");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);

		} else if (ztype == UINFILE_COMPRESSED_NA || 
					ztype == UINFILE_COMPRESSED_NOT) {
			command = "";
		} else {
			/*
			* Don't know how to recompress
			*/
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G), 
				"warning: archive not re-compressed\n");
			command = "";
		}
		if (strlen(command)) {
			swlib_doif_writef(G->g_verboseG,
					SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G),
				"run audit: starting transform_output\n");
			tfd1 = transform_output(compression_layers, 
					target_fd1,
					fork_blockmask,
					fork_defaultmask, pstatbytes
					);
			gpipe[1] = tfd1;
			swlib_doif_writef(G->g_verboseG,
					SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
				"run audit: finished transform_output\n");
		} else {
			tfd1= target_fd1;
		}
	} else {
		tfd1= target_fd1;
	}

	SWLIB_ASSERT(xformat != (XFORMAT*)NULL);
	xformat_set_ofd(xformat, tfd1);
	if (do_progressmeter) {
		start_progress_meter(G->g_meter_fd, working_arg,
					file_size, pstatbytes);
		swgp_signal_block(SIGALRM, (sigset_t *)NULL);
		alarm_handler = update_progress_meter;
	}
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
			"run audit: starting swlib_audit_distribution\n");

	ret = swlib_audit_distribution(xformat, 0, tfd1, 
			pstatbytes, 
			&G->g_signal_flag, 
			alarm_handler);

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
			"run audit: finished swlib_audit_distribution\n");
	if (do_progressmeter) {
		swgp_signal_unblock(SIGALRM, (sigset_t *)NULL);
        	sigpending(&pendmask);
		if (sigismember(&pendmask, SIGALRM)) {
			update_progress_meter(SIGALRM);
		}
		stop_progress_meter();
	}
	if (gpipe[1] >= 0) close(gpipe[1]);
	return ret;
}
				
static
int 
wait_on_script(pid_t write_pid, char * location)
{
	/*
	* wait on the write_script process and check for errors.
	*/
	pid_t ret;
	int status;
	ret = waitpid(write_pid, &status, 0);
	if (ret < 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"%s scripts waitpid : %s\n", 
			location, strerror(errno));
		return -1;
	}
	if (WIFEXITED(status)) {
		return (int)(WEXITSTATUS(status));
	} else {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"%s script had abnormal exit.\n", 
			location);
		return -2;
	}
}

static
STRAR *
construct_sourcepath_list(STRAR * source_spec_list, char * cwd, int * retval)
{
	STRAR * list;
	int ret;
	int n;
	int n_abs;
	int n_rel;
	char * target;
	char * selections;
	char * spec;
	char * path;

	n_abs = 0;
	n_rel = 0;
	*retval = 0;
	list = strar_open();
	n = 0;
	E_DEBUG("");
	while((spec=strar_get(source_spec_list, n)) != NULL) {
		E_DEBUG("");
		swc_parse_soc_spec(spec, &selections, &target);
		if (selections) {
			/* do not allow selections with multiple source specs */
			return NULL;
		}
		/* fprintf(stderr, "target=[%s]\n", target); */
		ret = swssh_determine_target_path(target, &path);
		if (ret) {
			/* fprintf(stderr, "swcopy: bad path for a multiple source context: %s\n", path); */
			/* not a good path */
			return NULL;
		}
		/* fprintf(stderr, "path1=[%s]  ret=%d\n", path, ret); */

		/* FIXME??  enforce that the paths must either be all 
			absolute or all relative */

		E_DEBUG("");
		if (*path == '/')
			n_abs++;
		else
			n_rel++;

		path = swc_validate_targetpath(
				0, 
				path, 
				"/", cwd, "target");

		if (n_abs && n_rel)
			return NULL;

		/* fprintf(stderr, "path=[%s]\n", path); */
		strar_add(list, path);
		n++;
		E_DEBUG("");
	}
	E_DEBUG("");
	return list;
}

static
int
check_source_specs(STRAR * source_spec_list, VPLOB * source_swspecs)
{
	int ret;
	int n_src;
	int i;
	char * x;
	char * spec;
	STROB * tmp;
	char ** margv;
	int margc;
	int mind;
	STRAR * args;

	args = NULL;
	/* If there are more than one source spec, impose the restriction that
	   it not contain a software selection part */

	i=0;
	while ((spec=strar_get(source_spec_list, i++)) != NULL) {
		E_DEBUG2("SPEC: %s", spec);
		;
	}

	n_src = i;
	if (n_src > 1) {
		/* this is the case of multiple '-s optarg' given from the
		   command line, this has the restriction that none contain
		   a software selection */
		;
	}

	tmp = strob_open(24);
	i = 0;
	while ((spec=strar_get(source_spec_list, i++)) != NULL) {
		/* for each spec,  parse it by white space */
		E_DEBUG2("SPEC2: %s", spec);
		if (args) strar_close(args);
		args = strar_open();
		x = strob_strtok(tmp, spec, " \t");
		while(x) {
			strar_add(args, x);	
			E_DEBUG2("SPEC2: TOK X: %s", spec);
			x = strob_strtok(tmp, NULL, " \t");
		}
		margc = strar_num_elements(args);
		mind = 0;
		margv = args->listM->list;
		ret = swc_process_selection_args(source_swspecs, margv, margc, &mind);
		if (ret) {
			/* error */
			return 1;
		}
	}

	/* return 0 if only one software spec _or_ there are multiple software
	   specs with no selection spec */

	if (args) strar_close(args);
	strob_close(tmp);
	return 0; 
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
	int c;
	int n;
	int ret;
	int tmpret;
	int optret = 0;
	int source_fdar[3];
	int target_fdar[3];
	int source_stdio_fdar[3];
	int target_stdio_fdar[3];
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
	int src_num_with_target;
	int src_num_with_swspec;
	char * tmp_s;
	char * optionname;
	int  optionEnum;
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
	char * wopt_no_audit;
	char * wopt_shell_command;
	int wopt_no_defaults = 0;
	int wopt_login = 0;
	int wopt_no_pty = 0;
	int wopt_pty = 0;
	int wopt_kill_hop_threshhold = 2;  /* 2 */
	int wopt_do_extract = 0;
	int wopt_no_extract = 0;
	int wopt_do_create = 0;
	int wopt_otarallow = 0;
	char * wopt_no_getconf;
	char * wopt_forward_agent;
	char * wopt_no_remote_kill;
	char * wopt_quiet_progress_bar;
	char * wopt_show_progress_bar;
	int is_seekable = 0;
	int wopt_with_hop_pids = 1;
	int devel_no_fork_optimization = 1;
	int local_stdout = 0;
	int local_stdin = 0;
	int do_extension_options_via_goto = 0;
	int tmp_status;
	int ts_waitret;
	int ts_statusp;
	int fverfd;
	int wopt_uncompress = 0;
	int target_loop_count = 0;
	int target_success_count = 0;
	int * p_do_progressmeter;
	int make_master_raw = 1;
	int is_local_stdin_seekable = 0;
	int nullfd;
	int opt_preview = 0;
	int testnum = 0;
	int retval = 0;
	int source_file_size;
	int target_file_size;
	uintmax_t statbytes;
	int target_array_index = 0;
	int select_array_index = 0;
	int stdin_in_use = 0;
	int use_no_getconf;
	int swevent_fd = STDOUT_FILENO;
	struct extendedOptions * opta = optionsArray;
	struct termios	login_orig_termiosO, *login_orig_termiosP = NULL;
	struct winsize	login_sizeO, *login_sizeP = NULL;
	
	char * xcmd;
	char * wopt_blocksize = "5120";
	char * wopt_show_options = NULL;
	char * wopt_show_options_files = NULL;
	char * wopt_pty_fork_type = NULL;
	int  wopt_keep_old_files = 0;

	char * eopt_autoselect_dependencies;
	char * eopt_compress_files;
	char * eopt_compression_type;
	char * eopt_distribution_target_directory;
	char * eopt_distribution_source_directory;
	char * eopt_enforce_dependencies;
	char * eopt_enforce_dsa;
	char * eopt_logfile;
	char * eopt_loglevel;
	char * eopt_recopy;
	char * eopt_select_local;
	char * eopt_uncompress_files;
	char * eopt_verbose;
	char * tty_opt = NULL;
	char * target_command_context = "target";
	char * source_fork_type;
	char * target_fork_type;
	char * pty_fork_type;
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

	int wopt_debug_events= 0; /* Set to 1 to turn on some more debugging */
	char * pax_read_command_key = (char*)NULL;
	char * pax_write_command_key = (char*)NULL;
	char * wopt_local_pax_write_command = (char*)NULL;
	char * wopt_remote_pax_write_command = (char*)NULL;
	char * wopt_local_pax_read_command = (char*)NULL;
	char * wopt_remote_pax_read_command = (char*)NULL;
	char * wopt_allow_rpm = (char*)NULL;
	char * working_arg = (char*)NULL;
	char * current_arg = (char*)NULL;
	char * opt_option_files = (char*)NULL;
	char * system_defaults_files = (char*)NULL;
	char * source_path = (char*)NULL;
	char cwd[1024];
	char * remote_shell_command = REMOTE_SSH_BIN;
	char * remote_shell_path;
	char * target_terminal_host = (char*)NULL;
	char * source_terminal_host = (char*)NULL;

	STRAR * target_cmdlist;
	STRAR * source_cmdlist;
	STRAR * target_tramp_list;
	STRAR * source_spec_list;
	STRAR * sourcepath_list;
	STRAR * source_tramp_list;
	STRAR * source_s_optargs;
	VPLOB * target_swspecs; /* List of software selections */
	VPLOB * source_swspecs; /* List of software selections */
	STROB * tmp;
	STROB * source_control_message;
	STROB * target_control_message;
	STROB * target_start_message;
	STROB * source_start_message;
	STROB * source_access_message;
	STROB * source_line_buf;
	STROB * target_line_buf;
	STROB * tmpcommand;
	CPLOB * w_arglist;
	CPLOB * e_arglist;
	XFORMAT * xformat = NULL;
	SWI * swi = (SWI*)NULL;
	SHCMD * target_sshcmd[2];
	SHCMD * source_sshcmd[2];
	SHCMD * kill_sshcmd[2];
	SHCMD * target_kill_sshcmd[2];
	SHCMD * source_kill_sshcmd[2];
	UINFORMAT * uinformat = NULL;
	FILE * fver;
	sigset_t * fork_blockmask;
	sigset_t * fork_defaultmask;
	sigset_t * currentmask;
	SWLOG * swlog;
	VPLOB * compression_layers;  /* array of SHCMD objects */
	SHCMD * compressor;

	struct option std_long_options[] =
             {
		{"selections-file", 1, 0, 'f'},
		{"preview", 0, 0, 'p'},
		{"block-size", 1, 0, 'b'},
		{"target-file", 1, 0, 't'},
		{"source", 1, 0, 's'},
		{"defaults-file", 1, 0, 'X'},
		{"verbose", 0, 0, 'v'},
		{"extension-option", 1, 0, 'W'},
		{"extended-option", 1, 0, 'x'},
		{"version", 0, 0, 'V'},
		{"help", 0, 0, '\012'},
               {0, 0, 0, 0}
             };
	
	struct option main_long_options[] =
             {
		{"selections-file", 1, 0, 'f'},
		{"preview", 0, 0, 'p'},
		{"block-size", 1, 0, 'b'},
		{"target-file", 1, 0, 't'},
		{"source", 1, 0, 's'},
		{"enable-agent-forwarding", 0, 0, 'A'},
		{"defaults-file", 1, 0, 'X'},
		{"verbose", 0, 0, 'v'},
		{"extension-option", 1, 0, 'W'},
		{"extended-option", 1, 0, 'x'},
		{"version", 0, 0, 'V'},
		{"help", 0, 0, '\012'},
                {"symmetric", 0, 0, 151},
                {"encrypt-for-recipient", 1, 0, 150},
		{"xz", 0, 0, 152},
		{"lzma", 0, 0, 153},
		{"noop", 0, 0, 154},
		{"show-options", 0, 0, 158},
		{"show-options-files", 0, 0, 159},
		{"no-audit", 0, 0, 160},
		{"login", 0, 0, 161},
		{"no-pty", 0, 0, 162},
		{"pty-fork-version", 1, 0, 163},
		{"testnum", 1, 0, 164},
		{"gzip", 0, 0, 165},
		{"bzip2", 0, 0, 166},
		{"extract", 0, 0, 167},
		{"defacto-layout", 0, 0, 168},
		{"pty", 0, 0, 169},
		{"keep-old-files", 0, 0, 170},
		{"uncompress", 0, 0, 171},
		{"remote-shell", 1, 0, 172},
		{"quiet-progress", 0, 0, 173},
		{"audit", 0, 0, 174},
		{"local-pax-write-command", 1, 0, 175},
		{"remote-pax-write-command", 1, 0, 176},
		{"pax-write-command", 1, 0, 177},
		{"remote-pax-read-command", 1, 0, 178},
		{"local-pax-read-command", 1, 0, 179},
		{"pax-read-command", 1, 0, 180},
		{"no-defaults", 0, 0, 181},
		{"pax-command", 1, 0, 182},
		{"show-progress-bar", 0, 0, 183},
		{"no-remote-kill", 0, 0, 184},
		{"no-getconf", 0, 0, 185},
		{"use-getconf", 0, 0, 186},
		{"progress-bar-fd", 1, 0, 187},
		{"shell-command", 1, 0, 188},
		{"allow-rpm", 0, 0, 189},
		{"unrpm", 0, 0, 190},
		{"source-script-name", 1, 0, 191},
		{"target-script-name", 1, 0, 192},
		{"swi-debug-name", 1, 0, 193}, 
		{"create", 0, 0, 194},
		{"no-extract", 0, 0, 195},
		{"to-swbis", 0, 0, 196},
		{"enable-ssh-agent-forwarding", 0, 0, 202},
		{"A", 0, 0, 202}, /* i.e. --A */
		{"a", 0, 0, 203}, /* i.e. --a */
		{"disable-ssh-agent-forwarding", 0, 0, 203},
		{"swbis_no_getconf", 		1, 0, 207},
		{"swbis-no-getconf", 		1, 0, 207},
		{"swbis_shell_command", 	1, 0, 208},
		{"swbis-shell-command", 	1, 0, 208},
		{"swbis_no_remote_kill", 	1, 0, 209},
		{"swbis-no-remote-kill", 	1, 0, 209},
		{"swbis_quiet_progress_bar", 	1, 0, 210},
		{"swbis-quiet-progress-bar", 	1, 0, 210},
		{"swbis_no_audit", 		1, 0, 211},
		{"swbis-no-audit", 		1, 0, 211},
		{"swbis_local_pax_write_command", 1, 0, 212},
		{"swbis-local-pax-write-command", 1, 0, 212},
		{"swbis_remote_pax_write_command", 1, 0, 213},
		{"swbis-remote-pax-write-command", 1, 0, 213},
		{"swbis_local_pax_read_command", 1, 0, 214},
		{"swbis-local-pax-read-command", 1, 0, 214},
		{"swbis_remote_pax_read_command", 1, 0, 215},
		{"swbis-remote-pax-read-command", 1, 0, 215},
		{"swbis_allow_rpm", 		1, 0, 216},
		{"swbis-allow-rpm", 		1, 0, 216},
		{"swbis_forward_agent", 	1, 0, 217},
		{"swbis-forward-agent", 	1, 0, 217},
		{0, 0, 0, 0}
             };
	
	struct option posix_extended_long_options[] =
             {
		{"autoselect_dependencies", 	1, 0, 230   },
		{"compress_files", 		1, 0, 231   },
		{"compression_type", 		1, 0, 232   },
		{"distribution_target_directory", 1, 0, 233 },
		{"distribution_source_directory", 1, 0, 234 },
		{"enforce_dependencies", 	1, 0, 235   },
		{"enforce_dsa", 		1, 0, 236   },
		{"logfile", 			1, 0, 237   },
		{"loglevel", 			1, 0, 238   },
		{"recopy", 			1, 0, 239   },
		{"select_local", 		1, 0, 240   },
		{"uncompress_files", 		1, 0, 241   },
		{"verbose", 			1, 0, 242   },
               {0, 0, 0, 0}
             };
     
	/* -------------------------------------------------
 
	struct option impl_extensions_long_options[] =
             {
		{"block-size", 1, 0, 'b'},
		{"noop", 0, 0, 154},
		{"show-options", 0, 0, 158},
		{"show-options-files", 0, 0, 159},
		{"no-audit", 0, 0, 160},
		{"login", 0, 0, 161},
		{"no-pty", 0, 0, 162},
		{"pty-fork-version", 1, 0, 163},
		{"testnum", 1, 0, 164},
                {"symmetric", 0, 0, 151},
                {"encrypt-for-recipient", 1, 0, 150},
		{"xz", 0, 0, 152},
		{"lzma", 0, 0, 153},
		{"gzip", 0, 0, 165},
		{"bzip2", 0, 0, 166},
		{"extract", 0, 0, 167},
		{"defacto-layout", 0, 0, 168},
		{"pty", 0, 0, 169},
		{"keep-old-files", 0, 0, 170},
		{"uncompress", 0, 0, 171},
		{"remote-shell", 1, 0, 172},
		{"quiet-progress", 0, 0, 173},
		{"audit", 0, 0, 174},
		{"local-pax-write-command", 1, 0, 175},
		{"remote-pax-write-command", 1, 0, 176},
		{"pax-write-command", 1, 0, 177},
		{"remote-pax-read-command", 1, 0, 178},
		{"local-pax-read-command", 1, 0, 179},
		{"pax-read-command", 1, 0, 180},
		{"no-defaults", 0, 0, 181},
		{"pax-command", 1, 0, 182},
		{"show-progress-bar", 0, 0, 183},
		{"no-remote-kill", 0, 0, 184},
		{"no-getconf", 0, 0, 185},
		{"use-getconf", 0, 0, 186},
		{"progress-bar-fd", 1, 0, 187},
		{"shell-command", 1, 0, 188},
		{"allow-rpm", 0, 0, 189},
		{"unrpm", 0, 0, 190},
		{"target-script-name", 1, 0, 191},
		{"source-script-name", 1, 0, 192},
		{"swi-debug-name", 1, 0, 193},
		{"create", 0, 0, 194},
		{"no-extract", 0, 0, 195},
		{0, 0, 0, 0}
             };
      
	------------------------------------------------------- */

 
	struct ugetopt_option_desc main_help_desc[] =
             {
	{"", "FILE","Take software selections from FILE."},
	{"", "", "Preview only by showing information to stdout"},
	{"block-size", "N", "specify block size N octets, 0<N<=32256\n"
	"          default size is 5120. (implementation extension)."},
	{"", "FILE", "Specify a FILE containing a list of targets."
	},
	{"", "source", "Specify the file, directory, or '-' for stdin.\n"
	"                     source may have target syntax."
	},
	{"", "FILE[ FILE2 ...]", "Specify files that override \n"
	"        system option defaults. Specify empty string to disable \n"
	"        option file reading."
	},
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
	{"", "option[=value]", "Specify implementation extension option."},
	{"", "option=value", "Specify posix extended option."},
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
	G = &g_blob;
	gb_init(G);
	pid_array = G->g_pid_array;
	status_array = G->g_status_array;
	p_pid_array_len = &G->g_pid_array_len;
	pty_fork_type = G->g_fork_pty2;
        G->g_main_sig_handler = main_sig_handler;
	G->g_safe_sig_handler = safe_sig_handler;
	G->optaM = opta;
	compression_layers = NULL;
	
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
	G->g_selectfd_array[0] = -1;
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
	
	G->g_vstderr = stderr;
	nullfd = G->g_nullfd;

	G->g_target_fdar = target_fdar;
	G->g_source_fdar = source_fdar;

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
		
	source_s_optargs = strar_open();
	target_tramp_list = strar_open();
	source_tramp_list = strar_open();
	source_spec_list = strar_open();

	tmp = strob_open(10);		/* General use String object. */
	tmpcommand = strob_open(10);
	w_arglist = cplob_open(1);	/* Pointer list object. */
	e_arglist = cplob_open(1);	/* Pointer list object. */

	initExtendedOption();

	if (getcwd(cwd, sizeof(cwd) - 1) == NULL) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G), 
			"%s\n", strerror(errno));
		LCEXIT(1);
	}
	cwd[sizeof(cwd) - 1] = '\0';

	/*
	* Set the compiled-in defaults for the extended options.
	*/
	eopt_autoselect_dependencies	= CHARFALSE;
	eopt_compress_files		= CHARFALSE;
	eopt_compression_type		= "none";
	eopt_distribution_target_directory = "-";
	eopt_distribution_source_directory = "-";
	eopt_enforce_dependencies	= CHARFALSE;
	eopt_enforce_dsa		= CHARFALSE;
	eopt_logfile			= "/var/lib/sw/swcopy.log";
	eopt_loglevel			= "0";
	eopt_recopy			= CHARFALSE;
	eopt_select_local		= CHARFALSE;
	eopt_uncompress_files		= CHARFALSE;
	eopt_verbose			= "1";
	wopt_no_getconf			= CHARTRUE;
	wopt_no_remote_kill		= CHARFALSE;
	wopt_quiet_progress_bar		= CHARFALSE;
	wopt_show_progress_bar		= CHARFALSE;
	wopt_no_audit			= CHARTRUE;
	wopt_shell_command		= "detect";
	wopt_local_pax_write_command 	= "tar";
	wopt_remote_pax_write_command 	= "tar";
	wopt_local_pax_read_command 	= "tar";
	wopt_remote_pax_read_command 	= "tar";
	wopt_allow_rpm			= CHARTRUE;
	wopt_forward_agent		= CHARTRUE;

	wopt_show_options = NULL;
	wopt_show_options_files = NULL;

	set_opta_initial(opta, SW_E_autoselect_dependencies, 
					eopt_autoselect_dependencies);
	set_opta_initial(opta, SW_E_compress_files, eopt_compress_files);
	set_opta_initial(opta, SW_E_compression_type, eopt_compression_type);
	set_opta_initial(opta, SW_E_distribution_source_directory, 
					eopt_distribution_source_directory);
	set_opta_initial(opta, SW_E_distribution_target_directory, 	
					eopt_distribution_target_directory);
	set_opta_initial(opta, SW_E_enforce_dependencies, 
					eopt_enforce_dependencies);
	set_opta_initial(opta, SW_E_enforce_dsa, eopt_enforce_dsa);
	set_opta_initial(opta, SW_E_logfile, eopt_logfile);
	set_opta_initial(opta, SW_E_loglevel, eopt_loglevel);
	set_opta_initial(opta, SW_E_recopy, eopt_recopy);
	set_opta_initial(opta, SW_E_select_local, eopt_select_local);
	set_opta_initial(opta, SW_E_uncompress_files, eopt_uncompress_files);
	set_opta_initial(opta, SW_E_verbose, eopt_verbose);

	set_opta_initial(opta, SW_E_swbis_no_getconf, wopt_no_getconf);
	set_opta_initial(opta, SW_E_swbis_shell_command, wopt_shell_command);
	set_opta_initial(opta, SW_E_swbis_remote_shell_client, remote_shell_command);
	set_opta_initial(opta, SW_E_swbis_no_remote_kill, wopt_no_remote_kill);
	set_opta_initial(opta, SW_E_swbis_quiet_progress_bar,
					wopt_quiet_progress_bar);
	set_opta_initial(opta, SW_E_swbis_no_audit, wopt_no_audit);
	set_opta_initial(opta, SW_E_swbis_local_pax_write_command,
					wopt_local_pax_write_command);
	set_opta_initial(opta, SW_E_swbis_remote_pax_write_command,
					wopt_remote_pax_write_command);
	set_opta_initial(opta, SW_E_swbis_local_pax_read_command,
					wopt_local_pax_read_command);
	set_opta_initial(opta, SW_E_swbis_remote_pax_read_command,
					wopt_remote_pax_read_command);
	set_opta_initial(opta, SW_E_swbis_allow_rpm, wopt_allow_rpm);

	cplob_add_nta(w_arglist, strdup(argv[0]));
	cplob_add_nta(e_arglist, strdup(argv[0]));

	while (1) {
		int option_index = 0;

		c = ugetopt_long(argc, argv, "b:f:t:pVvs:X:x:W:A", 
					main_long_options, &option_index);
		if (c == -1) break;

		switch (c) {
		case 'p':
			opt_preview = 1;
			break;
		case 'v':
			G->g_verboseG++;
			eopt_verbose = (char*)malloc(12);
			snprintf(eopt_verbose, 12, "%d", G->g_verboseG);
			eopt_verbose[11] = '\0';
			set_opta(opta, SW_E_verbose, eopt_verbose);
			break;
		case 'f':
			/*
			 * Process the selections file
			 */
			if (swc_process_swoperand_file(swlog,
				"selections", optarg, &stdin_in_use,
				&select_array_index, G->g_selectfd_array))
			{
				LCEXIT(1);
			}
			/*
			selections_filename = strdup(optarg);
			SWLIB_ALLOC_ASSERT(selections_filename != NULL);
			*/
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
				if (n <= 0 || n > 65537) {
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
			strar_add(source_s_optargs, optarg);
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
						swlib_doif_writef(G->g_verboseG, 
							G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
						"invalid extended arg : %s\n", 
							optarg);
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
			swlib_doif_writef(G->g_verboseG,
				G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
				"Try `swcopy --help' for more information.\n");
			LCEXIT(1);
		 	break;
               default:
			if (c >= 150 && c <= WOPT_LAST) { 
				/*
				*  This provides the ablility to specify 
				*  extension options by using the 
				*  --long-option syntax (i.e. without using 
				*  the -Woption syntax) .
				*/
				do_extension_options_via_goto = 1;
				goto gotoExtensionOptions;
gotoStandardOptions:
				;
			} else {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"invalid args.\n");
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
		
		case 230:
			swc_set_boolean_x_option(opta, 
				SW_E_autoselect_dependencies, 
				optarg, 
				&eopt_autoselect_dependencies);
			break;
		case 231:
			swc_set_boolean_x_option(opta, 
				SW_E_compress_files, 
				optarg, 
				&eopt_compress_files);
			break;
		case 232:
			swc_set_boolean_x_option(opta, 
				SW_E_compression_type, 
				optarg, 
				&eopt_compression_type);
			break;
		case 233:
			eopt_distribution_target_directory = strdup(optarg);
			set_opta(opta, 
				SW_E_distribution_target_directory, 
				eopt_distribution_target_directory);
			SWLIB_ALLOC_ASSERT
				(eopt_distribution_target_directory != NULL);
			break;
		case 234:
			eopt_distribution_source_directory = strdup(optarg);
			set_opta(opta, 
				SW_E_distribution_source_directory, 
				eopt_distribution_source_directory);
			SWLIB_ALLOC_ASSERT
				(eopt_distribution_source_directory != NULL);
			break;
		case 235:
			swc_set_boolean_x_option(opta, 
				SW_E_enforce_dependencies, 
				optarg, 
				&eopt_enforce_dependencies);
			break;

		case 236:
			swc_set_boolean_x_option(opta, 
				SW_E_enforce_dsa, 
				optarg, 
				&eopt_enforce_dsa);
			break;
		case 237:
			eopt_logfile = strdup(optarg);
			set_opta(opta, SW_E_logfile, eopt_logfile);
			SWLIB_ALLOC_ASSERT(eopt_logfile != NULL);
			break;

		case 238:
			eopt_loglevel = strdup(optarg);
			opt_loglevel = swlib_atoi(optarg, NULL);
			G->g_loglevel = opt_loglevel;
			G->g_logspec.loglevelM = opt_loglevel;
			set_opta(opta, SW_E_loglevel, eopt_loglevel);
			SWLIB_ALLOC_ASSERT(eopt_loglevel != NULL);
			break;

		case 239:
			swc_set_boolean_x_option(opta, 
					SW_E_recopy, 
					optarg, 
					&eopt_recopy);
			break;
		case 240:
			swc_set_boolean_x_option(opta, 
				SW_E_select_local, 
				optarg, 
				&eopt_select_local);
			break;
		case 241:
			swc_set_boolean_x_option(opta, 
				SW_E_uncompress_files, 
				optarg, 
				&eopt_uncompress_files);
			break;

		case 242:
			eopt_verbose = strdup(optarg);
			G->g_verboseG = swlib_atoi(eopt_verbose, NULL);
			set_opta(opta, SW_E_verbose, eopt_verbose);
			break;
		default:
			swlib_doif_writef(G->g_verboseG,
				G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
				"error processing extended option\n");
			swlib_doif_writef(G->g_verboseG,
				G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
				"Try `swcopy --help' for more information.\n");
		 	LCEXIT(1);
               break;
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
		case 154:
			break;
		case 158:
			wopt_show_options = CHARTRUE;
			break;
		case 159:
			wopt_show_options_files = CHARTRUE;
			break;
		case 160:
			wopt_no_audit = CHARTRUE;
			set_opta(opta, SW_E_swbis_no_audit, CHARTRUE);
			break;
		case 174:
			wopt_no_audit = CHARFALSE;
			set_opta(opta, SW_E_swbis_no_audit, CHARFALSE);
			break;
		case 161: /* Login */
			pty_fork_type = G->g_fork_pty2;
			swutil_x_mode = 0;
			wopt_no_audit = CHARTRUE;
			wopt_login = 1;
			make_master_raw = 0;
			local_stdout = 1;
			break;
		case 162: /* No pty */
			wopt_pty_fork_type = G->g_fork_pty_none;
			wopt_no_pty = 1;
			break;
		case 163:
			if (strcmp(optarg, G->g_fork_pty2) == 0) {
				pty_fork_type = G->g_fork_pty2;
			} else if (strcmp(optarg, G->g_fork_pty) == 0) {
				pty_fork_type = G->g_fork_pty;
			} else {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"invalid option arg : [%s]\n", optarg);
				LCEXIT(1);
			}
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
			E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gzip");
			shcmd_add_arg(compressor, "-f");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 166:
			E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "bzip2");
			shcmd_add_arg(compressor, "-f");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 150: /* encrypt for recipent */
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gpg");
			shcmd_add_arg(compressor, "--use-agent");
			shcmd_add_arg(compressor, "-e");
			if (strlen(optarg)) {
				shcmd_add_arg(compressor, "-r");
				shcmd_add_arg(compressor, optarg);
			}
			shcmd_add_arg(compressor, "-o");
			shcmd_add_arg(compressor, "-");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 151: /* symmetric */
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gpg");
			shcmd_add_arg(compressor, "--use-agent");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-o");
			shcmd_add_arg(compressor, "-");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 152:
			E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "xz");
			shcmd_add_arg(compressor, "-f");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 153:
			E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "lzma");
			shcmd_add_arg(compressor, "-f");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 167:
			wopt_no_extract = 0;
			G->g_no_extractM = 0;
			wopt_do_extract = 1;
			G->g_do_extractM = 1;
			break;
		case 168:
			wopt_otarallow = 1;
			break;
		case 169: /* Pty */
			wopt_pty_fork_type = G->g_fork_pty2;
			pty_fork_type = G->g_fork_pty2;
			wopt_pty = 1;
			break;
		case 170:
			wopt_keep_old_files = 1;
			break;
		case 171:
			wopt_uncompress = 1;
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
			xcmd = swc_get_pax_write_command(G->g_pax_write_commands, wopt_remote_pax_write_command,
							G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax write command: %s \n", 
					wopt_remote_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_remote_pax_write_command,
						wopt_remote_pax_write_command);
			break;
		case 175:
			wopt_local_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(G->g_pax_write_commands,
					wopt_local_pax_write_command, G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax write command: %s \n",
						wopt_local_pax_write_command);
				LCEXIT(1);
			}
			set_opta(opta, SW_E_swbis_local_pax_write_command,
					wopt_local_pax_write_command);
			break;
		case 177:
			wopt_local_pax_write_command = strdup(optarg);
			wopt_remote_pax_write_command = strdup(optarg);
			xcmd = swc_get_pax_write_command(G->g_pax_write_commands,
						wopt_local_pax_write_command, G->g_verboseG, (char*)NULL);
			if (xcmd == NULL) {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
					&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax write command: %s \n",
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
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
					&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax read command: %s \n",
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
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax read command: %s \n", 
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
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax read command: %s \n",
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
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
					&G->g_logspec, swc_get_stderr_fd(G),
					"illegal pax read command: %s \n",
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
			wopt_allow_rpm = CHARTRUE;
			set_opta(opta, SW_E_swbis_allow_rpm, CHARTRUE);
			break;
		case 196:
		case 190:
			wopt_allow_rpm = CHARTRUE;
			set_opta(opta, SW_E_swbis_allow_rpm, CHARTRUE);
			wopt_no_audit = CHARFALSE;
			set_opta(opta, SW_E_swbis_no_audit, CHARFALSE);
			break;
		case 191:
			G->g_source_script_name = strdup(optarg);
			break;
		case 192:
			G->g_target_script_name = strdup(optarg);
			break;
		case 193:
			G->g_swi_debug_name = strdup(optarg);
			break;
		case 194:
			wopt_do_create = 1;
			G->g_do_createM = 1;
			break;
		case 195:
			wopt_do_extract = 0;
			G->g_do_extractM = 1;
			wopt_no_extract = 1;
			G->g_no_extractM = 1;
			break;
		case 202:
			wopt_forward_agent = CHARTRUE;
			set_opta(opta, SW_E_swbis_forward_agent, wopt_forward_agent);
			break;
		case 203:
			wopt_forward_agent = CHARFALSE;
			set_opta(opta, SW_E_swbis_forward_agent, CHARFALSE);
			break;
		case 207:
		case 208:
		case 209:
		case 210:
		case 211:
		case 212:
		case 213:
		case 214:
		case 215:
		case 216:
		case 217:
			E_DEBUG("running with getLongOptionNameFromValue");
			optionname = getLongOptionNameFromValue(main_long_options , c);
			E_DEBUG2("option name from getLongOptionNameFromValue is [%s]", optionname);
			SWLIB_ASSERT(optionname != NULL);
			E_DEBUG2("running getEnumFromName with name [%s]", optionname);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, optionEnum, optarg);
			break;
		default:
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
					&G->g_logspec, swc_get_stderr_fd(G),
				"error processing implementation extension option\n");
		 	exit(1);
		break;
		}
		if (do_extension_options_via_goto == 1) {
			do_extension_options_via_goto = 0;
			goto gotoStandardOptions;
		}
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = =
	 *  End of Command line options processing
	 * = = = = = = = = = = = = = = = = = = = =
	 */
	
	optind = main_optind;
	
	system_defaults_files = initialize_options_files_list(NULL);
	
	/*
	 * = = = = = = = = = = = = = = = = = = = =
	 *  Show the options file names to stdout.
	 * = = = = = = = = = = = = = = = = = = = =
	 */
	if (wopt_show_options_files) { 
		/*
		 * This shows the filename of the defaults that
		 * would be used.
		 */
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
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
						&G->g_logspec, swc_get_stderr_fd(G),
						"defaults file error\n");
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
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"defaults file error\n");
		LCEXIT(1);
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *  Reset the option values to pick up the values from 
	 *  the defaults file(s).
	 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */
	eopt_autoselect_dependencies	= get_opta(opta, 
						SW_E_autoselect_dependencies);
	eopt_compress_files		= get_opta(opta, SW_E_compress_files);
	eopt_uncompress_files		= get_opta(opta, SW_E_uncompress_files);
	eopt_compression_type		= get_opta(opta, SW_E_compression_type);
	eopt_distribution_target_directory = get_opta(opta, 
					SW_E_distribution_target_directory);
	eopt_distribution_source_directory = get_opta(opta, 
					SW_E_distribution_source_directory);
	eopt_enforce_dependencies	= get_opta(opta, SW_E_enforce_dependencies);
	eopt_enforce_dsa		= get_opta(opta, SW_E_enforce_dsa);
	eopt_logfile			= get_opta(opta, SW_E_logfile);
	eopt_loglevel			= get_opta(opta, SW_E_loglevel);
			G->g_loglevel = swlib_atoi(eopt_loglevel, NULL);
			G->g_logspec.loglevelM = G->g_loglevel;
			opt_loglevel = G->g_loglevel;
	eopt_recopy			= get_opta(opta, SW_E_recopy);
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
	wopt_no_audit			= swbisoption_get_opta(opta, 
						SW_E_swbis_no_audit);
	wopt_local_pax_write_command	= swbisoption_get_opta(opta, 	
					SW_E_swbis_local_pax_write_command);
	wopt_remote_pax_write_command	= swbisoption_get_opta(opta, 
					SW_E_swbis_remote_pax_write_command);
	wopt_local_pax_read_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_local_pax_read_command);
	wopt_remote_pax_read_command	= swbisoption_get_opta(opta, 
						SW_E_swbis_remote_pax_read_command);

	if (wopt_show_options) { 
		/*
		 * show the options to stdout
		 */
		swextopt_writeExtendedOptions(STDOUT_FILENO, opta, SWC_U_C);
		if (G->g_verboseG > 4) {
			debug_writeBooleanExtendedOptions(STDOUT_FILENO, opta);
		}
		LCEXIT(0);
	}

	/*
	 * All the options are now set from all sources.
	 */
	
	if (swc_is_option_true(wopt_show_progress_bar)) wopt_quiet_progress_bar = CHARFALSE;
	
	if (*remote_shell_command == '/') {
		remote_shell_path = remote_shell_command;
	} else {
		remote_shell_path = shcmd_find_in_path(getenv("PATH"), remote_shell_command);
	}

	use_no_getconf = swc_is_option_true(wopt_no_getconf);	
	
	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 *   Now process the source specs given by the '-s' optargs
	 *   (which was deferred  until the defaults files were read).
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	src_num_with_target = 0;
	src_num_with_swspec = 0;
	n = 0;	
	tmp_s = strar_get(source_s_optargs, n++);
	while (tmp_s) {
		soc_spec_source = strdup(tmp_s);
		if (
			strchr(tmp_s, '@') == NULL &&
			strlen(soc_spec_source) > 0 &&
			*soc_spec_source != '/' &&
			strchr(soc_spec_source, ':') == NULL
		) {
			E_DEBUG2("P3: parsing as software spec: %s", soc_spec_source);
			/* Here, assume the unadorned name is a software spec
			   with no target, hence,
			    append the default distribution source directory */

			src_num_with_swspec++;
			strob_strcpy(tmp, soc_spec_source);
			strob_strcat(tmp, "@");
			strob_strcat(tmp, get_opta(opta, SW_E_distribution_source_directory));
			soc_spec_source = strdup(strob_str(tmp));
		} else if (strchr(tmp_s, '@') == NULL) {
			E_DEBUG2("P3: parsing as target: %s", soc_spec_source);
			/* Its not a software spec, and the '@' is not present,
			   but it is a target, hence we need to add it because the
			   later routines expect it */

			src_num_with_target++;
			strob_strcpy(tmp, "");
			strob_strcat(tmp, "@");
			strob_strcat(tmp, soc_spec_source);
			soc_spec_source = strdup(strob_str(tmp));
		} else {
			E_DEBUG("P3: doing nothing");
			src_num_with_target++;
			;
		}
		/* FIXME, here we may want to attach the default distribution_source_directory */
		strar_add(source_spec_list, soc_spec_source);
		SWLIB_ALLOC_ASSERT(soc_spec_source != NULL);
		tmp_s = strar_get(source_s_optargs, n++);
	}
	
	if (src_num_with_target && src_num_with_swspec) {
		/* Impose a retriction of what is supported */
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"error: unsupported combination of multiple source specs\n");
		LCEXIT(1);
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 *   Configure the standard I/O usages.
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	if (opt_preview) {
		fver = stdout;
		fverfd = STDOUT_FILENO;
	} else {
		fver = G->g_vstderr;
		fverfd = STDERR_FILENO;
	}
	
	if (G->g_verboseG == 0 && !opt_preview) {
		fver = fopen("/dev/null", "r+");
		if (!fver) LCEXIT(1);
		G->g_vstderr = fver;
		fverfd = nullfd;
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
	}

	if (G->g_verboseG >= SWC_VERBOSE_7) {
		swlib_set_verbose_level(SWC_VERBOSE_7);
	}
	
	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 *   Set up the logger spec
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	swc_initialize_logspec(&G->g_logspec, eopt_logfile, G->g_loglevel);

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 * initialize the swutil object members
	 * = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	swlog->swu_efdM = swc_get_stderr_fd(G);
	swlog->swu_logspecM = &G->g_logspec;
	swlog->swu_verboseM = G->g_verboseG;
	swlog->swu_fail_loudlyM = G->g_fail_loudly;

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *   Set the terminal setting and ssh tty option.
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */
	
	if (
		/* (wopt_pty) || */
		(wopt_login && opt_preview == 0 && isatty(STDIN_FILENO))
	) {
		tty_opt = "-t";
		target_command_context = "login";
		login_orig_termiosP = &login_orig_termiosO;
		login_sizeP = &login_sizeO;
		if (tcgetattr(STDIN_FILENO, login_orig_termiosP) < 0)
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
						"tcgetattr error on stdin");
		if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) login_sizeP) < 0)
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
							"TIOCGWINSZ error");
		tty_raw_ctl(1);
		if (atexit(swlib_tty_atexit) < 0)
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
							"atexit error");
	} else {
		tty_opt = "-T";
		login_orig_termiosP = NULL;
		login_sizeP = NULL;
	}
	
	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *        Set the signal handlers.
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	swgp_signal(SIGINT, safe_sig_handler);
	swgp_signal(SIGPIPE, safe_sig_handler);
	swgp_signal(SIGTERM, safe_sig_handler);

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *     Process the source software spec
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	source_swspecs = vplob_open();
	ret = check_source_specs(source_spec_list, source_swspecs);
	if (ret != 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"error: invalid or unsupported source specs\n");
		LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
	} 

	/* SOURCE:  &xcl_source_target <--- soc_spec_source */
	
	#ifdef FILENEEDDEBUG
	E_DEBUG("Running show_all_sw_selections for the source");
	show_all_sw_selections(source_swspecs); 
	E_DEBUG("END Running show_all_sw_selections for the source");
	#endif

	if (opt_preview) swssh_deactivate_sanity_check();
	E_DEBUG2("Running swc_parse_soc_spec: [%s]", soc_spec_source);
	if (swc_parse_soc_spec(soc_spec_source, &cl_source_selections, 
		&xcl_source_target) != 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
		"error parsing source target spec: %s\n", soc_spec_source);
		LCEXIT(1);
	}

	if (	cl_source_selections &&
		strcmp(cl_source_selections, "-") &&
		strcmp(cl_source_selections, ".") &&
		1
	) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
		"software selection feature for the source is not yet supported: %s @ %s \n"
		"perhaps try prefixation of a '@', such as:\n"
		"   -s @ %s@%s \n",
		cl_source_selections, xcl_source_target,
		cl_source_selections, xcl_source_target );
		LCEXIT(1);
	}

	if (xcl_source_target == NULL) {
		cl_source_target = strdup(eopt_distribution_source_directory);
	} else {
		strob_strcpy(tmp, "");	
		ret = swextopt_combine_directory(tmp,
			xcl_source_target,
			eopt_distribution_source_directory);
		if (ret) {
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"software source specification is ambiguous: %s\n", soc_spec_source);
			LCEXIT(1);
		}
		cl_source_target = strdup(strob_str(tmp));
	}

	/*
	 * Do a sanity check on the source.
	 * If it does not have an embedded ':' require that
	 * it end in a ':' (thus signifying a hostname) or
	 * begin with a '/' indicating a pathname.
	 */
	if (
		(
			strcmp(cl_source_target, "-")  &&
			*cl_source_target != ':'
		) &&
		(
			strlen(cl_source_target) == 0 ||
			(
				strchr(cl_source_target, ':') == NULL &&
				(
					(*cl_source_target != '/') &&
					(cl_source_target[strlen(cl_source_target)-1] != ':')
				)
			)
		)
	) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"invalid source spec : %s\n", cl_source_target);
		LCEXIT(1);
	}

	/* SOURCE:  &tmpcharp  <--- cl_source_target  */

	source_nhops = swssh_parse_target(source_sshcmd[0],
				source_kill_sshcmd[0],
				cl_source_target,
				remote_shell_path,
				remote_shell_command,
				&tmpcharp,
				&source_terminal_host,
				tty_opt, wopt_with_hop_pids, NULL, 1 /* do set -A */);
	SWLIB_ASSERT(source_nhops >= 0);


	/* SOURCE:  &source_path  <--- &tmpchar */

	source_path = swc_validate_targetpath(
				source_nhops, 
				tmpcharp, 
				eopt_distribution_source_directory, cwd, "source");


	SWLIB_ASSERT(source_path != NULL);

	if (source_nhops >= 1 && strcmp(source_path, "-") == 0) {
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
			&G->g_logspec, swc_get_stderr_fd(G),
			"remote stdin is not supported\n");
		LCEXIT(1);
	}

	if (strcmp(source_path, "-") == 0) { 
		if (stdin_in_use) {
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"invalid usage of stdin\n");
			LCEXIT(1);
		}
		local_stdin = 1; 
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *      Set the Serial archive write command.
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	if (pax_write_command_key == (char*)NULL) {
		if (source_nhops < 1) {
			pax_write_command_key = wopt_local_pax_write_command;
		} else {
			pax_write_command_key = wopt_remote_pax_write_command;
		}		
	}

	/*
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 *    Process the Software Selections of the Target
	 * = = = = = = = = = = = = = = = = = = = = = = = = = 
	 */

	target_swspecs = vplob_open(); /* List object containing list of SWVERID objects */
	ret = 0;
	if (argv[optind]) {
		if (*(argv[optind]) != '@') {
			/*
			* Must be a software selection.
			*/
			ret = swc_process_selection_args(target_swspecs, argv, argc, &optind);
		}
	}

	if (ret)
	{
		/*
		 * Software selection error
		 */
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
			&G->g_logspec, swc_get_stderr_fd(G),
				"error processing selections\n");
		LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
	}


	if (does_have_sw_selections(target_swspecs) > 0) {
		/*
		 * Software selections not supported yet.
		 */
		swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
			&G->g_logspec, swc_get_stderr_fd(G),
		"software selections not implemented: %s\n", swverid_print((SWVERID*)(vplob_val(target_swspecs, 0)), tmp));
			LCEXIT(sw_exitval(G, target_loop_count, 
					target_success_count));
	}
	
	#ifdef FILENEEDDEBUG 
	E_DEBUG("Running show_all_sw_selections for the target");
	show_all_sw_selections(target_swspecs); 
	E_DEBUG("END Running show_all_sw_selections for the target");
	#endif


	/* Process the list of source paths for example
		swcopy -s /usr -s /etc @ /dev/tape
	*/

	sourcepath_list = NULL;
	if (strar_num_elements(source_spec_list) > 1) {
		E_DEBUG("source_spec_list > 1");
		sourcepath_list = construct_sourcepath_list(source_spec_list, cwd, &ret);
		if (
			sourcepath_list == NULL ||
			swc_make_multiplepathlist(sourcepath_list, (STROB *)NULL, (STROB *)NULL) == NULL
		) {
			/* error */
			E_DEBUG("error");
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"bad source specs for a multiple source context\n");
			LCEXIT(sw_exitval(G, target_loop_count, target_success_count));
		}

		/* Force creation of tar archive */
		wopt_do_create = 1;
		G->g_do_createM = 1;
		G->g_sourcepath_listM = sourcepath_list;
		devel_no_fork_optimization = 0;
		E_DEBUG("");
	} else {
		G->g_sourcepath_listM = NULL;
	}

	/*
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 *           Loop over the targets.
	 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
	 */

   	current_arg = swc_get_next_target(argv, argc, &optind, 
					G->g_targetfd_array, 
					get_opta(opta, SW_E_distribution_target_directory),
					&num_remains);
	while (current_arg) {
		E_DEBUG("");
		swgp_signal(SIGINT, safe_sig_handler);
		swgp_signal(SIGPIPE, safe_sig_handler);
		swgp_signal(SIGTERM, safe_sig_handler);
		statbytes = 0;
		retval = 0;
		target_sshcmd[0] = shcmd_open();
		target_cmdlist = strar_open();
		source_cmdlist = strar_open();
		source_control_message = strob_open(10);
		target_control_message = strob_open(10);
		target_start_message = strob_open(10);
		source_start_message = strob_open(10);
		source_access_message = strob_open(10);
		cl_target_target = NULL;
		cl_target_selections = NULL;
		if (target_loop_count > 0 && local_stdin) {
			/*
			* Cant send stdin to multiple targets.
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"copying stdin to multilpe targets is not supported\n");
			retval = 1;
			goto TARGET1;
			*/
		}
		working_arg = strdup(current_arg);
		/* Target Spec --- */

		/*
		 * Parse the target sofware_selections and targets.
		 */

		E_DEBUG("");
		soc_spec_target = strdup(working_arg);
		if (target_loop_count == 0) {
			if (swc_parse_soc_spec(soc_spec_target, &cl_target_selections, 
				&xcl_target_target) != 0) {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
					&G->g_logspec, swc_get_stderr_fd(G),
					"error parsing target spec: %s\n", soc_spec_target);
				LCEXIT(1);
			}

			if (xcl_target_target == NULL) {
				cl_target_target = strdup(eopt_distribution_target_directory);
			} else {
				cl_target_target = strdup(xcl_target_target);
			}

			/*
			 * Selections are not supported here.  They are applied
			 * globally and processed before entering the target processing
			 * loop.
			 */
	
			if (cl_target_selections) {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
			"software selection not valid when specified with a specific target.\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
		} else {
			/*
			* subsequext args are targets, the same
			* software selection applies.
			*/
			cl_target_target = strdup(soc_spec_target);
		}

		E_DEBUG("");
		new_cl_target_target = swc_convert_multi_host_target_syntax(G, cl_target_target);
		free(cl_target_target);
		cl_target_target = new_cl_target_target;

		E_DEBUG("");
		E_DEBUG2("cl_target_target is [%s]", cl_target_target);
		/*
		 * combine with distribution_target_directory according to POSIX
		 */
		strob_strcpy(tmp, "");	
		ret = swextopt_combine_directory(tmp,
				cl_target_target,
				eopt_distribution_target_directory);
		if (ret) {
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
			"software target specification is ambiguous: %s\n", soc_spec_target);
			LCEXIT(1);
		}
		free(cl_target_target);
		cl_target_target = strdup(strob_str(tmp));

		E_DEBUG("");
		target_nhops = swssh_parse_target(target_sshcmd[0],
				target_kill_sshcmd[0],
				cl_target_target,
				remote_shell_path,
				remote_shell_command,
				&tmpcharp,
				&target_terminal_host,
				tty_opt, wopt_with_hop_pids, NULL, 1 /* do set -A */);
		SWLIB_ASSERT(target_nhops >= 0);
		
		E_DEBUG2("xcl_target_target is [%s]", xcl_target_target);
		E_DEBUG2("tmpcharp is [%s]", tmpcharp);
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
					eopt_distribution_target_directory, cwd, "target");
		SWLIB_ASSERT(target_path != NULL);

		E_DEBUG2("target_path is [%s]", target_path);
		if (wopt_login) {
			/*
			* A "-" target trips the ssh to /bin/cat.
			* We don't want that in the case of a 
			* interactive login, so set the target path
			* to a slash.
			*/ 
			target_path = strdup("/");
		}


		E_DEBUG("");
		
		if (
			strcmp(target_path, "-") == 0 &&
			target_nhops >= 1 &&
			strcmp(source_path, "-") == 0 
			)
		{
			/*
			* This would echo keystrokes through the pipeline to stdout
			* on a *remote machine*.  Since in not a valid concept
			* we will use it to mean something else, namely
			*    read a list of files on stding and write
			*    an archive of those files on stdout.
			* That is:
			*     pax -w       --or-- 
			*     tar cf - --files-from=-
			*/
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
						"feature not supported yet\n");
			LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
		} else if (
			strcmp(target_path, "-") == 0 &&
			target_nhops >= 1
			)
		{
			target_path = strdup(".");
			/* Was fatal error 2008-01-19
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
						"invalid target spec\n");
			LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			*/
		}


		E_DEBUG("");
		if (strcmp(target_path, "-") == 0) {
			local_stdout = 1;
		}
		if (local_stdout) swevent_fd = -1;
		G->g_swevent_fd = swevent_fd;

		E_DEBUG("");
		/*
		 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
		 *   Establish the logger process and stderr redirection.
		 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		if (target_loop_count == 0 && opt_preview == 0)
			G->g_logger_pid = swc_fork_logger(G, source_line_buf,
					target_line_buf, swc_get_stderr_fd(G),
					swevent_fd, &G->g_logspec, &G->g_s_efd, &G->g_t_efd,
					G->g_verboseG, (int*)(NULL));
		target_stdio_fdar[2] = G->g_t_efd;
		source_stdio_fdar[2] = G->g_s_efd;

		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_3, 
			&G->g_logspec, G->g_t_efd, /* swc_get_stderr_fd(G), */
			"SWBIS_TARGET_BEGINS for %s\n", current_arg);
	
		if (target_nhops && source_nhops) {
			/*
			* disallow remote source *and* target if either is
			* a double hop.  But allow it if both are single
			* hops.
			*/
			if (target_nhops + source_nhops >= 3) {
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
		"simultaneous remote source and target is not supported.\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
		}	

		/*
		* Set the fork type according to usage mode and user options.  
		* Only use pty when required.
		* If nhops is 1 then pty use seems not to be required.
		*/
		E_DEBUG("");
		if ((target_nhops + source_nhops) == 1) {
			/*
			* pseudo tty not needed
			*/
			if (wopt_pty || wopt_login) {
				pty_fork_type = G->g_fork_pty2;
			} else {
				pty_fork_type = G->g_fork_pty_none;
			}
		}

		E_DEBUG("");
		if (source_nhops == 0 || wopt_no_pty || source_nhops > 1) {
			source_fork_type =  G->g_fork_pty_none;
			if (wopt_pty_fork_type) 
				source_fork_type = wopt_pty_fork_type;
		} else {
			source_fork_type = pty_fork_type;
			if (wopt_pty_fork_type) 
				source_fork_type = wopt_pty_fork_type;
		}

		E_DEBUG("");
		if (wopt_login) {
			target_fork_type = pty_fork_type;
			if (wopt_pty_fork_type) 
				target_fork_type = wopt_pty_fork_type;
		} else if (target_nhops == 0 || 
				wopt_no_pty || 
					target_nhops > 1) {
			target_fork_type = G->g_fork_pty_none;
			if (wopt_pty_fork_type) 
				target_fork_type = wopt_pty_fork_type;
		} else {
			target_fork_type = pty_fork_type;
			if (wopt_pty_fork_type) 
				target_fork_type = wopt_pty_fork_type;
		}
		if (target_nhops && source_nhops) {
			target_fork_type = G->g_fork_pty_none;
			source_fork_type =  G->g_fork_pty_none;
		}

		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		E_DEBUG("");
		if ((target_nhops + source_nhops) == 0 &&
						wopt_do_extract == 0) {
			if (cowardly_check(source_path,  target_path)) {
				swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
			"cowardly refusing to overwrite the source\n");
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
		}

		E_DEBUG("");
		if (pax_read_command_key == (char*)NULL) {
			if (target_nhops < 1) {
				pax_read_command_key = 
					wopt_local_pax_read_command;
			} else {
				pax_read_command_key = 
					wopt_remote_pax_read_command;
			}		
		}

		E_DEBUG("");
		if (opt_preview) {
			/*
			 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
			 *                  Preview mode.
			 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
			 */
			if (
				cl_source_target[strlen(cl_source_target) - 1]
				== '/'
			)
			{
				/*
				* Hack to allow for easy testing of both cases:
				*	SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY
				*		and
				*	SWBIS_SWCOPY_SOURCE_CTL_ARCHIVE
				*/
				strob_strcpy(source_control_message, 
					SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY
					);
			} else {
				strob_strcpy(source_control_message, 
					SWBIS_SWCOPY_SOURCE_CTL_ARCHIVE
					);
			}
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
					"Preview Mode\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
					"local_stdin = %d\n", local_stdin);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"local_stdout = %d\n", local_stdout);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target Preview\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: soc spec : [%s]\n", working_arg);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: target : [%s]\n", cl_target_target);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: nhops: %d\n", target_nhops);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: path: [%s]\n", target_path);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: source is %s\n", strob_str(source_control_message));
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Target: fork type: %s\n", target_fork_type);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: pax_read_command: %s\n", swc_get_pax_read_command(G->g_pax_read_commands,
								pax_read_command_key, 
								G->g_verboseG >= SWC_VERBOSE_3, 
								wopt_keep_old_files, DEFAULT_PAX_R));
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: Command Args: ");
		swc_do_preview_cmd(G, "swcopy: Target: Kill Command Args: ", fver,
			target_path, 
			target_sshcmd[0],
			target_kill_sshcmd[0],
			cl_target_target,
			target_cmdlist,
			target_nhops, 1,
			use_no_getconf, wopt_shell_command);
		fflush(fver);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Target: script starts here:\n");
		if (local_stdout == 0 && G->g_verboseG > SWC_VERBOSE_5) {
			swc_write_target_copy_script(
				fverfd, 
				target_path, 
				source_control_message, 
				source_path, 
				sourcepath_list, 
				SWC_SCRIPT_SLEEP_DELAY, 
				wopt_keep_old_files, 	
				target_nhops, 
				G->g_verboseG,
				wopt_do_extract,
				wopt_no_extract,
				pax_read_command_key,
				target_terminal_host,
				wopt_blocksize);
		}
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: End of Target script:\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Target: End of Target Preview\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Source Preview:\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Source: soc spec: [%s]\n",
					cl_source_target);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Source: nhops: %d\n", source_nhops);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Source: path: [%s]\n", source_path);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Source: fork type: %s\n", source_fork_type);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
				"Source: pax_write_command: %s\n", 
				swc_get_pax_write_command(G->g_pax_write_commands, pax_write_command_key, 
						G->g_verboseG, DEFAULT_PAX_W));
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Source: Command Args: ");
		swc_do_preview_cmd(G, "swcopy: Source: Kill Command Args: ", fver,
			source_path, 
			source_sshcmd[0], 
			source_kill_sshcmd[0], 
			cl_source_target, 
			source_cmdlist, 
			source_nhops,
			target_loop_count == 0,
			use_no_getconf, wopt_shell_command);
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Source: command ends here:\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Source: script starts here:\n");
		if (local_stdin == 0 && G->g_verboseG > SWC_VERBOSE_5) {
			swc_write_source_copy_script(G,
				fverfd, 
				source_path,
				SWCOPY_DO_SOURCE_CTL_MESSAGE,
				SWC_VERBOSE_0, 
				SWC_SCRIPT_SLEEP_DELAY, 
				source_nhops, 
				pax_write_command_key, 
				source_terminal_host,
				wopt_blocksize
				);
		}
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Source: End of Source script:\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"Source: End of Source Preview\n");
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, NULL, fverfd,
			"End of Preview Mode\n");
		goto TARGET1;
		} /* end of preview */

		E_DEBUG("");
		/*
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 *   Do the real copy.
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
				"target_fork_type : %s\n",
					target_fork_type);
		swlib_doif_writef(G->g_verboseG,
				SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
					"source_fork_type : %s\n",
					source_fork_type);

		E_DEBUG("");
		/*
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 * Make the source piping.
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
		if (local_stdin == 0) {
			/*
			* Remote Source, and not stdin
			*/
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
				(sigset_t*)fork_blockmask, 
				devel_no_fork_optimization,
				G->g_verboseG,
				&source_file_size,
				use_no_getconf,
				&is_source_local_regular_file,
				wopt_shell_command,
				swc_get_stderr_fd(G),
				&G->g_logspec);
			swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
			E_DEBUG2("is_source_local_regular_file=%d", is_source_local_regular_file);
	
			/* if there are multiple source specs, unset is_source_local_regular_file */
			if (G->g_sourcepath_listM) {
				/* disable optimization when multiple source
				   specs are used */
				is_source_local_regular_file = 0;
			}
			if (is_source_local_regular_file) {
				if (lseek(source_fdar[0],
						(off_t)0, SEEK_SET) != 0) {
					swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
				"lseek internal error on stdio_fdar[0]\n");
					retval = 1;
					goto TARGET1;
				}
			}

			if (ss_pid < 0) {
				/*
				* Fatal
				*/		
				LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
			}

			if (ss_pid > 0) { 
				swc_record_pid(ss_pid, 
					G->g_pid_array, 
					&G->g_pid_array_len, 
					G->g_verboseG);
			}

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
					swlib_doif_writef(G->g_verboseG, 
						G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
					"tty_raw error : source_fdar[0]\n");
					LCEXIT(
					sw_exitval(G, target_loop_count, 
						target_success_count));
				}
			}
		} else {
			/*
			* here is the source piping for local stdin.
			*/ 
			ss_pid = 0;
			source_fdar[0] = source_stdio_fdar[0];
			source_fdar[1] = source_stdio_fdar[1];
			source_fdar[2] = source_stdio_fdar[2];

			if (target_loop_count > 0) {
				dup2(save_stdio_fdar[0], source_fdar[0]);
				is_local_stdin_seekable = 
					(lseek(source_fdar[0],
						(off_t)0,
						SEEK_CUR) >= 0);
				if (is_local_stdin_seekable) {
					/*
					swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
			"Warning: multiple targets using stdin is not functional.\n");
					*/
					if (lseek(source_fdar[0],
							0, SEEK_SET) != 0) {
						swlib_doif_writef(G->g_verboseG,
							G->g_fail_loudly,
							&G->g_logspec, swc_get_stderr_fd(G),
					"lseek internal error on source_fdar[0]\n");
						retval = 1;
						goto TARGET1;
					} else {
						;
					}
				} else {
					/*
					* can't have multiple targets when the
					* source is not not seekable
					*/
					swlib_doif_writef(G->g_verboseG,
						G->g_fail_loudly,
						&G->g_logspec, swc_get_stderr_fd(G),
						"source is not seekable,"
						" multiple targets not allowed\n");
					retval = 1;
					goto TARGET1;
				}
			}
		}

		E_DEBUG("");
		if (wopt_login && target_nhops == 0) {
			swlib_tty_atexit();
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
				&G->g_logspec, swc_get_stderr_fd(G), 
				"invalid host argument : [%s]\n",
				target_path);
			LCEXIT(
				sw_exitval(G, target_loop_count, 
					target_success_count));
		}	
		
		E_DEBUG("");
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
					source_terminal_host,
					wopt_blocksize,
					SWC_VERBOSE_0,
					swc_write_source_copy_script);
		swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);

		E_DEBUG("");
		if (source_write_pid > 0) {
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, 
				&G->g_logspec, swc_get_stderr_fd(G), 
				"waiting on source script pid\n");
			/*
			 * ==========================
			 * Wait on the source script.
			 * ==========================
			 */
			E_DEBUG("");
			if ((ret=wait_on_script(source_write_pid, 
						"source")) != 0) {
				/*
			 	 * ====================================
				 * non zero is error for source script.
				 * ====================================
				 */
				swlib_doif_writef(G->g_verboseG, 
					G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
				"write_scripts waitpid : exit value = %d\n",
					ret );
				swc_check_for_current_signals(G, __LINE__, wopt_no_remote_kill);
				LCEXIT(
					sw_exitval(G, target_loop_count,	
						target_success_count));
			}
				
			swlib_doif_writef(G->g_verboseG, 
				SWC_VERBOSE_IDB, 
				&G->g_logspec, swc_get_stderr_fd(G), 
				"wait() on source script succeeded.\n");

			E_DEBUG("");
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
				swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);
				swlib_doif_writef(G->g_verboseG, 1, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"read_start_ctl_message error"
					" (loc=start)\n");
				LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
			}
			
			E_DEBUG("");
			if (source_nhops >= wopt_kill_hop_threshhold) {
				/*
				* Construct the remote kill vector.
				*/
				G->g_killcmd = NULL;
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
					&G->g_logspec, swc_get_stderr_fd(G), 
				"Running swc_construct_newkill_vector\n");
				if ((ret=swc_construct_newkill_vector(
					source_kill_sshcmd[0], 
					source_nhops,
					source_tramp_list, 
					source_script_pid, 
					G->g_verboseG)) < 0) 
				{
					source_kill_sshcmd[0] = NULL;
					swlib_doif_writef(G->g_verboseG, 
							G->g_fail_loudly, 
							&G->g_logspec, swc_get_stderr_fd(G),
					"source kill command not constructed"
					"(ret=%d), maybe a shell does not "
					"have PPID.\n", ret);
				}
				G->g_source_kmd = source_kill_sshcmd[0];
			}
			swc_check_for_current_signals(G, __LINE__, 
					wopt_no_remote_kill);

			/*
			 * ===================================
			 * Read the leading control message from the 
			 * output of the source script.
			 * This is how the target script knows to
			 * create a file or directory archive.
			 * ===================================
			 */
			E_DEBUG("");
			if ( SWCOPY_DO_SOURCE_CTL_MESSAGE ) {
				/*
				* Read the control message 
				* from the remote source.
				*/
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
					&G->g_logspec, swc_get_stderr_fd(G),
				"reading source script control messages\n"
								);

				if (swc_read_target_ctl_message(G,
					source_fdar[0], 
					source_control_message,
					G->g_verboseG, "source") < 0) 
				{
					swlib_doif_writef(G->g_verboseG, 
						SWC_VERBOSE_IDB,
							&G->g_logspec, swc_get_stderr_fd(G),
						"read_target_ctl_message error"
						" (loc=source_start)\n");
					LCEXIT(sw_exitval(G, 
						target_loop_count,
						target_success_count));
				}
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
					&G->g_logspec, swc_get_stderr_fd(G),
					"Got source control message [%s]\n",
					strob_str(source_control_message));
			} else {
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, 
					&G->g_logspec, swc_get_stderr_fd(G), 
				"No source control message expected\n");
			}

			E_DEBUG("");
			/*
			 * ===================================
			 * Read the SW_SOURCE_ACCESS_BEGIN or
			 * the SW_SOURCE_ACCESS_ERROR message
			 * ===================================
			 */	

			E_DEBUG("");
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
				&G->g_logspec, swc_get_stderr_fd(G),
				"reading source script access messages\n");
			if (swc_read_target_ctl_message(G, source_fdar[0], 
				source_access_message,
				G->g_verboseG, "source") < 0
			) 
			{
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, 
						&G->g_logspec, swc_get_stderr_fd(G),
					"read_target_ctl_message error"
					" (loc=source_access)\n");
				sleep(1); /* this allows some error messages
					     to appear */
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G, 
					target_loop_count,
					target_success_count));
			}
			
			E_DEBUG("");
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
				swlib_doif_writef(G->g_verboseG, 1, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"source access error: ret=%d :%s\n",
					ret, 
					strob_str(source_access_message));
				swc_shutdown_logger(G, 0);
				LCEXIT(sw_exitval(G, 
					target_loop_count,
					target_success_count));
			}
		} else if (source_write_pid == 0) {
			E_DEBUG("");
			/* 
			 * ====================================
			 * fork did not happen.
			 * This is Normal, source script not required. 
			 * ====================================
			 */
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
				&G->g_logspec, swc_get_stderr_fd(G), 
				"No fork required for source control script\n"
				);
		} else {
			/*
			* error
			*/
			swlib_doif_writef(G->g_verboseG, 1, &G->g_logspec, swc_get_stderr_fd(G),
				"fatal internal error. fork error.\n");
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
		}

		E_DEBUG("");
		/*
		 * ++++++++++++++++++++++++++++++++++ 
		 * Make the target piping.
		 * ++++++++++++++++++++++++++++++++++ 
		 */
		swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);
		if (local_stdout == 0 || 
			wopt_login || 
			target_nhops >= 1) {
			/*
			* Remote target or login mode.
			*/
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
				(sigset_t*)fork_blockmask,
				0 /*devel_no_fork_optimization*/,
				G->g_verboseG,
				&target_file_size,
				use_no_getconf,
				(int*)(NULL),
				wopt_shell_command,
				swc_get_stderr_fd(G),
				&G->g_logspec);
			swc_check_for_current_signals(G, __LINE__,
						wopt_no_remote_kill);
			if (ts_pid < 0) {
				swlib_doif_writef(G->g_verboseG, 
					G->g_fail_loudly, 
					&G->g_logspec, swc_get_stderr_fd(G),
					"fork error : %s\n", strerror(errno));
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G, target_loop_count, 
						target_success_count));
			}
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
	
			if (wopt_login == 0 || target_nhops >= 1) {
				if (isatty(target_fdar[0]))
				if (swlib_tty_raw(target_fdar[0]) < 0) {
					swlib_doif_writef(G->g_verboseG, 
							G->g_fail_loudly,	
							&G->g_logspec, swc_get_stderr_fd(G),
					"tty_raw error : target_fdar[0]\n");
					swc_shutdown_logger(G, SIGABRT);
					LCEXIT(sw_exitval(G, 
						target_loop_count,
						target_success_count));
				}
			}
		} else {
			/*
			* here is the target piping for local stdout.
			*/ 
			ts_pid = 0;
			target_fdar[0] = target_stdio_fdar[0];
			target_fdar[1] = target_stdio_fdar[1];
			target_fdar[2] = target_stdio_fdar[2];
		}
			
		/*
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 * Write the target script.
		 * +++++++++++++++++++++++++++++++++++++++++++++++++ 
		 */
		target_write_pid = run_target_script(
					local_stdout, 
					target_fdar[1],
					source_path,
					target_path,
					source_control_message,
					wopt_keep_old_files, 
					target_nhops,
					wopt_do_extract, 
					wopt_no_extract, 
					pax_read_command_key,
					fork_blockmask,
					fork_defaultmask,
					target_terminal_host,
					wopt_blocksize
					);
		swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);

		if (target_write_pid > 0) {
			/*
			 * ==========================
			 * Wait on the target script.
			 * ==========================
			 */
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
						&G->g_logspec, swc_get_stderr_fd(G), 
					"waiting on target script pid\n");
			if ((ret=wait_on_script(
					target_write_pid, "target")) <= 0) {
				/*
				 * ================================
				 * zero is error for target script.
				 * ================================
				 */
				swlib_doif_writef(G->g_verboseG, 
					G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
					"write_scripts waitpid : exit value"
					" = %d which is an error.\n",
						 ret );
				LC_RAISE(SIGTERM);
				swc_check_for_current_signals(G, __LINE__,
							wopt_no_remote_kill);
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G, 
						target_loop_count, 
						target_success_count));
			}
			if (ret == 1) {
				/*
				 * =============================
				 * Normal compression condition.
				 * =============================
				 */
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2,
					&G->g_logspec, swc_get_stderr_fd(G), 
					"target script returned %d\n", ret);
			} else if (ret == 2) {
				/*
				 * ===================================
				 * Don't re-compress
				 * send to target script uncompressed.
				 * ===================================
				 */
				wopt_uncompress = 1;
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2,
					&G->g_logspec, swc_get_stderr_fd(G), 
					"target script returned %d\n", ret);
			}
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, 
				&G->g_logspec, swc_get_stderr_fd(G), 
				"wait() on target script succeeded.\n");
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
				 * This is the failure path for ssh authentication
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
					if (G->g_signal_flag)
						swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1,
						&G->g_logspec, swc_get_stderr_fd(G),
						"SW_ILLEGAL_STATE_TRANSISTION for target %s: signum=%d\n",
						current_arg, G->g_signal_flag);
					else 
						swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1,
						&G->g_logspec, swc_get_stderr_fd(G),
						"SW_INTERNAL_ERROR for target %s in file %s at line %d\n",
						current_arg, __FILE__, __LINE__);
				}
				/*
				 * Continue with next target.
				 */
				goto ENTER_WITH_FAILURE;
			}

			if (target_nhops >= wopt_kill_hop_threshhold /*2*/ ) {
				/*
				 * ==============================
				 * Construct the clean-up command 
				 * to shutdown remote script.
				 * ==============================
				 */ 
				G->g_killcmd = NULL;
				if ((ret=swc_construct_newkill_vector(
					target_kill_sshcmd[0], 
					target_nhops,
					target_tramp_list, 
					target_script_pid, 
					G->g_verboseG)) < 0) 
				{
					target_kill_sshcmd[0] = NULL;
					swlib_doif_writef(G->g_verboseG, 
						G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
				"target kill command not constructed (ret=%d)"
				", maybe a shell does not have PPID.\n",
								ret);
				}
				G->g_target_kmd = target_kill_sshcmd[0];
			}
		} else if (target_write_pid == 0) {
			/* fork did not happen.
			   Normal, source script not required.  */
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB,
				&G->g_logspec, swc_get_stderr_fd(G), 
				"No fork required for target control script\n"
				);
		} else {
			/*
			 * error
			 */
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly,
				&G->g_logspec, swc_get_stderr_fd(G),
				"fatal internal error. target script"
				" fork error.\n");
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, 
				target_loop_count, 
				target_success_count));
		}

		swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);

		if (ts_pid && 
			waitpid(ts_pid, &tmp_status, WNOHANG) > 0) {
			swc_shutdown_logger(G, SIGABRT);
			LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
		}
				
		if ((
			local_stdout == 0  ||
			G->g_meter_fd == STDERR_FILENO
		    ) &&
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

		if (G->g_verboseG >= SWC_VERBOSE_IDB2) {
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"target_fdar[0] = %d\n", target_fdar[0]);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"target_fdar[1] = %d\n", target_fdar[1]);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"target_fdar[2] = %d\n", target_fdar[2]);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"source_fdar[0] = %d\n", source_fdar[0]);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"source_fdar[1] = %d\n", source_fdar[1]);
			swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
				&G->g_logspec, swc_get_stderr_fd(G),
				"source_fdar[2] = %d\n", source_fdar[2]);
		}
			
		/* Here is the ladder of expected Usages.  */
		tty_raw_ctl(2);
		ret = 0;
		if (swutil_x_mode && swc_is_option_true(wopt_no_audit) == 0) {
			/*  Normal Default Usage
			    Decode and Audit the posix package.  */
			if (strstr(strob_str(source_control_message), 
				"SW_SOURCE_ACCESS_ERROR :") != 
				(char*)NULL) {
				/*
				* Error
				*/	
				swlib_doif_writef(G->g_verboseG, 
					G->g_fail_loudly, &G->g_logspec, swc_get_stderr_fd(G),
					"SW_SOURCE_ACCESS_ERROR : []\n"
					);
				swc_shutdown_logger(G, 0);
				LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
			}

			swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);

			if (swi) {
				xformat_close(swi->xformatM);
				swi_delete(swi);
				if (uxfio_lseek(source_fdar[0], 0, SEEK_SET) != 0) {
					fprintf(stderr, "%s: uxfio_lseek error: %s : %d\n",
						swlib_utilname_get(),  __FILE__, __LINE__);
					exit(2);
				}
			}

			E_DEBUG("into do_decode");
			swgp_signal(SIGINT, main_sig_handler);
			swi = swi_create();
			SWLIB_ASSERT(swi != NULL);
			swi_set_utility_id(swi, SWC_U_C);

			ret = swi_do_decode(swi, swlog, G->g_nullfd,
				dup(source_fdar[0]),
				target_path,
				source_path,
				target_swspecs,
				target_terminal_host,
				opta,
				is_seekable,
				wopt_debug_events,
				G->g_verboseG,
				&G->g_logspec,
				-1 /* open flags, use default */);
			G->g_xformat = swi->xformatM;
			E_DEBUG("out of do_decode");
	
			if (ret) {
				swlib_doif_writef(G->g_verboseG,
					G->g_fail_loudly,
					&G->g_logspec, swc_get_stderr_fd(G),
					"error decoding source\n");
				main_sig_handler(SIGTERM);
				swc_shutdown_logger(G, SIGABRT);
				LCEXIT(sw_exitval(G, 
					target_loop_count, 
					target_success_count));
			}

			E_DEBUG("into run_audit");
			
			/* The file postition should be zero, (at begininning) */
			ret = run_audit(swi, target_fdar[1],
				source_fdar[0],
				source_path,
				wopt_do_extract,
				wopt_no_extract,
				wopt_otarallow, 
				wopt_uncompress, 
				&statbytes,
				*p_do_progressmeter,
				source_file_size,
				working_arg,
				fork_blockmask,
				fork_defaultmask,
				opta
				);
			E_DEBUG("finished run_audit");
			E_DEBUG2("cur pos = %d", (int)lseek(source_fdar[0], 0, SEEK_CUR));
			/*
			if (G->g_signal_flag == SIGPIPE) {
				G->g_signal_flag = 0;
			}
			*/
			swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);
			kill_sshcmd[0] = (SHCMD*)NULL;
		} else if ( (wopt_login) ) {
			/* Interactive Login -OR- remote /bin/cat.
			   Communicate with stdin and stdout, 
			   which may be the keyboard and terminal.  */
			xformat = (XFORMAT*)NULL;
			ret = swgp_stdioPump(
				STDOUT_FILENO, 
				target_fdar[0], 
				target_fdar[1], 
				STDIN_FILENO, 
				0, /* verbose */
				(G->g_verboseG < 3), 
				ts_pid, 
				&ts_waitret, 
				&ts_statusp, 
				&G->g_signal_flag,
				&statbytes);
			swc_check_for_current_signals(G, __LINE__,
						wopt_no_remote_kill);
			swlib_tty_atexit();
		} else if (swutil_x_mode && swc_is_option_true(wopt_no_audit)) {
			/* No Audit Mode
			   Used to copy arbitrary files.  */
			int ugpipe[2];
			int gpipe[2];
			int tfd1;
			int sfd1;
			int local_stdout_fd;
			int remote_stdout_fd;
			int noad_nullfd = -1;
			int pump_source_pid;
			xformat = (XFORMAT*)NULL;
				
			ugpipe[0] = -1;
			gpipe[1] = -1;

			/*
			 *  -+ Optimization.+- Obsolete if star/tar verbosity
			 *			is redirected to fd=2
			 * ====================
			 * For non tar/pax transfers set remote_stdout_fd
			 * and local_stdout_fd to the /dev/null fd.
			 * This causes a factor of 5 speed up in
			 * transferring files.   The actual remote stdout
			 * is needed to see the verbose output of tar on the
			 * remote side.
			 *
			 * Detemine if the source file type is an directory or
			 * regular file and use this to base a optimization 
			 * decision on.
			 *
			 * Note:
			 * Even though redir'ing the verbose listing to fd=2
			 * breaks tar/star convention, it is consistent with
			 * pax(1) and the IEEE Std 1003.1-2001 pax manual page
			 * from the Open Group.
			 */

			if (/*Disabled*/ 
			    0 && (strstr(strob_str(source_control_message), 
				SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY) || 
				wopt_do_extract) && G->g_verboseG >= 3) 
			{
				/* Turned of, Now dead code, pump stdout
				   seems to cut performance by half. */
				/*
				 * Its a directory. Pump stdout so we can see
				 * the tar's verbosity.
				 */
				local_stdout_fd = STDOUT_FILENO;
				remote_stdout_fd = target_fdar[0];
			} else {
				/*
				 * Its not a directory.
				 * This make the stdioPump five (5) times
				 * faster.
				 */
				noad_nullfd = open("/dev/null", O_RDWR, 0);
				remote_stdout_fd = noad_nullfd;
				local_stdout_fd = noad_nullfd;
			}

			swc_check_for_current_signals(G, __LINE__,
						wopt_no_remote_kill);
			
			if (   wopt_uncompress || 
			       /* is_target_path_a_directory(target_path) || */
		 	       wopt_do_extract
			) {
				int xfg;
				
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
						&G->g_logspec, swc_get_stderr_fd(G), 
				"no_audit mode: wopt_uncompress = %d\n"
				"no_audit mode: is_target_path_a directory = %d\n"
				"no_audit mode: wopt_do_extract = %d\n",
					wopt_uncompress,
					is_target_path_a_directory(target_path),
					wopt_do_extract);

				xfg = UINFILE_DETECT_FORCEUNIXFD |
						UINFILE_DETECT_ARBITRARY_DATA;
				swgp_signal(SIGINT, main_sig_handler);
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
						&G->g_logspec, swc_get_stderr_fd(G), 
				"no_audit mode: starting uinfile_opendup\n");
				sfd1 = uinfile_opendup(
						source_fdar[0],
						(mode_t)(0), 
						&uinformat, 
						xfg);
				swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB2, 
						&G->g_logspec, swc_get_stderr_fd(G), 
						"no_audit mode: "
						"finished uinfile_opendup\n");
				swgp_signal(SIGINT, safe_sig_handler);
				if (sfd1 < 0) {
					swlib_doif_writef(G->g_verboseG, 
						G->g_fail_loudly, 
						&G->g_logspec, swc_get_stderr_fd(G),
					"error from uinfile_opendup\n");
					swc_shutdown_logger(G, SIGABRT);
					LCEXIT(sw_exitval(G, 
						target_loop_count, 
						target_success_count));
				}
				ugpipe[0] = sfd1;
			} else {
				sfd1= source_fdar[0];
			}

			if (compression_layers != NULL) {
				swlib_doif_writef(G->g_verboseG,
						SWC_VERBOSE_IDB2, 
						&G->g_logspec, swc_get_stderr_fd(G), 
						"no_audit mode: "
						"starting transform_output\n"
						);
				tfd1 = transform_output(
						compression_layers,
						target_fdar[1],
						fork_blockmask,
						fork_defaultmask, (uintmax_t*)(NULL)
						); 
				swlib_doif_writef(G->g_verboseG,
						SWC_VERBOSE_IDB2,
						&G->g_logspec, swc_get_stderr_fd(G), 
				"no_audit mode: finished transform_output\n"
						);
				gpipe[1] = tfd1;
			} else {
				tfd1= target_fdar[1];
			}

			if (*p_do_progressmeter)
				start_progress_meter(G->g_meter_fd, working_arg, 
						(off_t)source_file_size,
						&statbytes);

			if (source_fdar[0] != STDIN_FILENO) {
				pump_source_pid =  ss_pid;
			} else {
				pump_source_pid =  ts_pid;
			}
			swlib_doif_writef(G->g_verboseG,
				SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
				"no_audit mode: starting swgp_stdioPump\n");
			ret = swgp_stdioPump(
				tfd1, 
				sfd1, 
				local_stdout_fd,
					/*STDOUT_FILENO or noad_nullfd*/
				remote_stdout_fd,
					/*target_fdar[0] or noad_nullfd*/
				G->g_verboseG >= 10,
				(G->g_verboseG < SWC_VERBOSE_IDB),
				pump_source_pid,
				&ts_waitret, 
				&ts_statusp, 
				&G->g_signal_flag,
				&statbytes);
			swlib_doif_writef(G->g_verboseG,
				SWC_VERBOSE_IDB2, &G->g_logspec, swc_get_stderr_fd(G), 
				"no_audit mode: finished swgp_stdioPump\n"
									);
			if (noad_nullfd >= 0) close(noad_nullfd);
			if (ugpipe[0] >= 0) close(ugpipe[0]);
			if (gpipe[1] >= 0) close(gpipe[1]);
			if (uinformat) { 
				uinfile_close(uinformat); uinformat = NULL; 
			}
			if (*p_do_progressmeter)
				stop_progress_meter();
			if (G->g_signal_flag == SIGPIPE)
				G->g_signal_flag = 0;
			swc_check_for_current_signals(G, __LINE__,
						wopt_no_remote_kill);
		} else {
			/*
			    Invalid Usage */
			ret = -1;
			swlib_doif_writef(G->g_verboseG, G->g_fail_loudly, 
				&G->g_logspec, swc_get_stderr_fd(G),
				PROGNAME ": internal error,"
				"invalid usage\n");
		}

ENTER_WITH_FAILURE:

		if (kill_sshcmd[0]) {
			shcmd_close(kill_sshcmd[0]);
			kill_sshcmd[0] = (SHCMD*)NULL;
		}
		if (xformat) {
			swc_gf_xformat_close(G, xformat);
			xformat = (XFORMAT*)NULL;
		}

		/* Now close down. */

		swc_check_for_current_signals(G, __LINE__,
					wopt_no_remote_kill);
		if (wopt_login == 0) {
			close(target_fdar[0]);
			if (target_fdar[1] != STDOUT_FILENO) 
				close(target_fdar[1]);
			if (source_fdar[0] != STDIN_FILENO)
				close(source_fdar[0]);
			if (source_fdar[1] != STDOUT_FILENO) 
				close(source_fdar[1]);
		}

		swlib_wait_on_all_pids(
				pid_array, 
				*p_pid_array_len, 
				status_array, 0 /*waitpid flags*/, 
				G->g_verboseG - 2);

		if (retval == 0) {
			retval = swc_analyze_status_array(pid_array,
						*p_pid_array_len, 
						status_array,
						G->g_verboseG - 2);
		}
		if (retval == 0 && ret ) retval++;
		/* Now re-Initialize.  */
		*p_pid_array_len = 0;
		free(soc_spec_target);
		free(working_arg);
		working_arg = NULL;
		soc_spec_target = NULL;
		cl_target_target = NULL;
		cl_target_selections = NULL;
		target_path = NULL;
		/* End real copy, not a preview */
TARGET1:
		G->g_pid_array_len = 0;

		swc_check_for_current_signals(G, __LINE__,
						wopt_no_remote_kill);
		shcmd_close(target_sshcmd[0]);
		strar_close(target_cmdlist);
		strar_close(source_cmdlist);
		strob_close(source_control_message);
		strob_close(target_control_message);
		strob_close(target_start_message);
		strob_close(source_start_message);
		strob_close(source_access_message);
		target_loop_count++;
		if (retval == 0) target_success_count++;

		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_3,
			&G->g_logspec, retval ? swc_get_stderr_fd(G) : swevent_fd,
			"SWBIS_TARGET_ENDS for %s: status=%d\n",
						current_arg,
						retval);

		swc_check_for_current_signals(G, __LINE__,
						wopt_no_remote_kill);
		free(current_arg);
   		current_arg = swc_get_next_target(argv, 
						argc, 
						&optind, 
						G->g_targetfd_array,
				get_opta(opta, SW_E_distribution_target_directory),
						&num_remains);
	} /* target loop */
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, 
			&G->g_logspec, swc_get_stderr_fd(G), "Finished processing targets\n");
	if (targetlist_fd >= 0) close(targetlist_fd);
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_IDB, &G->g_logspec, swc_get_stderr_fd(G), 
		"exiting at %s:%d with value %d\n",
		__FILE__, __LINE__,
		sw_exitval(G, target_loop_count, target_success_count));

	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_6, 
			&G->g_logspec, swc_get_stderr_fd(G) /*g_t_efd*/, "SWI_NORMAL_EXIT: status=%d\n",
		sw_exitval(G, target_loop_count, target_success_count));

	retval = swc_shutdown_logger(G, 0);
	if (retval) return(3);
	return(sw_exitval(G, target_loop_count, target_success_count));	
}
