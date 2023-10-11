/* swremove_lib.c -- swremove routines.

 Copyright (C) 2007,2010 James H. Lowe, Jr. 
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
#include "ahs.h"
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
#include "swi.h"
#include "swutilname.h"
#include "globalblob.h"
#include "swproglib.h"
#include "swlist_lib.h"
#include "shlib.h"
#include "swgpg.h"
#include "swicat.h"
#include "swicat_e.h"

#define SWREMOVE_ARG_MAX_THRESHOLD 800

static
int
make_remove_glob_list_for(int do_dir, GB * G, SWI * swi, STROB * buf, SWI_FILELIST * fl, int do_remove_volatile, int do_rename_volatile)
{
	STROB * rm_tmp;
	char * sx;
	int type;
	int ix;
	int action;

	rm_tmp = strob_open(32);
	ix = 0;
	while ((sx=swi_fl_get_path(fl, ix)) != NULL) {
		if (G->devel_verboseM) {
			SWLIB_INFO4("INFO; file=[%s] type=[%c] is_volatile=[%c]", sx,
					(int)swi_fl_get_type(fl, ix),
					(int)swi_fl_is_volatile(fl, ix));
		}

		if (G->g_justdbM == 1) {
			action = 0;
		} else if (swi_fl_is_volatile(fl, ix))	{
			/* apply volatile policy */
			if (do_remove_volatile == 1) {
				/* keep going, just do it */
				action = 1;
			} else if (do_remove_volatile == 0) {
				/* skip it */
				action = 0;
			} else {
				/* rename it */
				action = 2;
			}
		} else {
			action = 1;
		}

		if (action == 1) {
			/* remove it */
			type = swi_fl_get_type(fl, ix);
			if (do_dir == 0 && type != DIRTYPE) {
				strob_sprintf(rm_tmp, STROB_DO_APPEND, " \"%s\"", sx);
				if (strob_strlen(rm_tmp) > SWREMOVE_ARG_MAX_THRESHOLD) {
					strob_sprintf(buf, STROB_DO_APPEND, "rm -f %s\n", strob_str(rm_tmp));
					strob_sprintf(buf, STROB_DO_APPEND, "case $? in 0) ;; *) rm_retval=$? ;; esac\n");
					strob_strcpy(rm_tmp, "");
				}
			} else if (do_dir && type == DIRTYPE) {
				if (strcmp(sx, "/") == 0 || strcmp(sx, "./") == 0) {
					; /* skip */
				} else {
					swlib_squash_embedded_dot_slash(sx);
					strob_sprintf(rm_tmp, STROB_DO_APPEND, " \"%s\"", sx);
				}
				E_DEBUG2("HAVE dir to remove: %s", sx);
				if (strob_strlen(rm_tmp) > SWREMOVE_ARG_MAX_THRESHOLD) {
					E_DEBUG2("AT SWREMOVE_ARG_MAX_THRESHOLD with %s\n", sx);
					strob_sprintf(buf, STROB_DO_APPEND, "rmdir %s\n", strob_str(rm_tmp));
					strob_sprintf(buf, STROB_DO_APPEND, "case $? in 0) ;; *) rm_retval=$? ;; esac\n");
					E_DEBUG2("HAVE rmdir command args: %s", strob_str(rm_tmp));
					strob_strcpy(rm_tmp, "");
				}
			}
		} else if (action == 0) {
			/* skip it */
			;
		}  else {
			/* rename it */
			strob_sprintf(buf, STROB_DO_APPEND, "mv -f \"%s\" \"%s%s\"\n",
				sx, sx, SWREMOVE_VOLATILE_SUFFIX);

			/* Do not trap this error, just let it go
			strob_sprintf(buf, STROB_DO_APPEND, "case $? in 0) ;; *) rm_retval=$? ;; esac\n");
			*/
		}
		ix++;
	}

	if (do_dir == 0) {
		if (strob_strlen(rm_tmp) > 0)
			strob_sprintf(buf, STROB_DO_APPEND, "\nrm -f %s\n", strob_str(rm_tmp));
	} else {
		if (strob_strlen(rm_tmp) > 0)
			strob_sprintf(buf, STROB_DO_APPEND, "\nrmdir %s\n", strob_str(rm_tmp));
	}
	
	strob_close(rm_tmp);
	return 0;
}

static
void
looper_abort(GB * G, SWICOL * swicol, int ofd)
{
	swpl_send_abort(swicol, ofd, G->g_swi_event_fd,  SWBIS_TS_Abort);
}

static
int
show_file_list(GB * G, SWI_FILELIST * fl)
{
	char * sx;
	int ix;

	swi_fl_qsort_forward(fl);
	ix = 0;
	while ((sx=swi_fl_get_path(fl, ix)) != NULL) {
		sw_l_msg(G, SWC_VERBOSE_1, "%s\n", sx);
		ix++;
	}
	return 0;
}

static
int
make_remove_command_script_fragment(GB * G, CISF_PRODUCT * cisf, SWI * swi, STROB * buf, SWI_FILELIST * fl)
{
	STROB * suffix_tmp;
	STROB * rm_tmp;
	STROB * rmdir_tmp;
	char * remove_volatile;	
	int do_remove_volatile;
	int do_rename_volatile;

	/* Tag every file in a volatile directory as being volatile itself
	   FIXME ??? is this the correct thing to do */

	swpl_tag_volatile_files(fl);

	/* Get the volatile file policy from the options */

	remove_volatile = swbisoption_get_opta(G->optaM, SW_E_swbis_remove_volatile);
	do_rename_volatile = 0;
	if (swextopt_is_value_true(remove_volatile)) {
		do_remove_volatile = 1;
	} else if (swextopt_is_value_false(remove_volatile)) {
		do_remove_volatile = 0;
	} else if (strcasecmp(remove_volatile, "rename") == 0) {
		do_remove_volatile = 2;
	} else {
		sw_e_msg(G, "bad value for swbis_remove_volatile [%s]\n", remove_volatile);
		do_remove_volatile = 2;
	}
	
	suffix_tmp = strob_open(32);
	rm_tmp = strob_open(32);
	rmdir_tmp = strob_open(32);

	strob_strcpy(buf, "");
	swi_fl_qsort_reverse(fl);

	strob_sprintf(buf, STROB_DO_APPEND,
		"( # subshell 001_remove\n"
		"sw_retval=0\n"
		);

	swpl_construct_script(G, cisf, buf, swi, SW_A_preremove);

	strob_sprintf(buf, STROB_DO_APPEND,
	"exit \"$sw_retval\"\n"
	") 1</dev/null # subshell 001_remove\n"
	"sw_retval=$?\n"
	);

	strob_sprintf(buf, STROB_DO_APPEND, "rm_retval=0\n");

	make_remove_glob_list_for(0 /* not DIRTYPE */, G, swi, buf, fl, do_remove_volatile, do_rename_volatile);
	make_remove_glob_list_for(1 /* now the DIRTYPE */, G, swi, buf, fl, do_remove_volatile, do_rename_volatile);
	
	strob_sprintf(buf, STROB_DO_APPEND,
		"( # subshell 002_remove\n"
		);

	swpl_construct_script(G, cisf, buf, swi, SW_A_postremove);

	strob_sprintf(buf, STROB_DO_APPEND,
		"exit \"$sw_retval\"\n"
		") 1</dev/null # subshell 002_remove\n"
		"sw_retval=$?\n"
		);

	strob_close(suffix_tmp);
	strob_close(rm_tmp);
	strob_close(rmdir_tmp);
	return 0;
}

static
int
remove_files(GB * G, char * target_path, CISF_PRODUCT * cisf, SWI * swi, SWICOL * swicol, SWICAT_E * e, SWI_FILELIST * file_list, int ofd)
{
	STROB * buf;
	int ret;

	E_DEBUG("");
	buf = strob_open(300);

	strob_sprintf(buf, STROB_DO_APPEND, "rm_retval=0\n");

	make_remove_command_script_fragment(G, cisf, swi, buf, file_list);

	/* Add the requisite dd bs=512 count=1 of=/dev/null */
		
	strob_sprintf(buf, STROB_DO_APPEND,
		"dd bs=512 count=1 of=/dev/null 2>/dev/null\n"
		"sw_retval=$rm_retval\n"
		);

	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd,
		512,
		target_path,  /* WAS "."*/
		strob_str(buf),
		SWBIS_TS_remove_files);
	if (ret != 0) {
		return -1;
	}

	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret <= 0) {
		return -1;
	}

	ret = swicol_rpsh_task_expect(swicol, G->g_swi_event_fd, SWICOL_TL_10);
	E_DEBUG2("swicol_rpsh_task_expect returned %d", ret);

	strob_close(buf);
	return ret;
}

static
char *
looper_locked_region(STROB * buf, int vlv)
{
	strob_sprintf(buf, 0,
		CSHID
		"				# -----------------------------\n"
		"				# The locked region begins here\n"
		"				# -----------------------------\n"
		"				export rp_status\n"

		"				#\n"
		"				# The code for selection processing can begin here\n"
		"				#\n"

		"				read swspec_string\n"                  /* SWBIS_DT_0001 */
		"				%s\n" /* SW_SELECTION_EXECUTION_BEGINS */

		"				# echo Processing Now \"$catentdir\" 1>&2\n"
		"				echo \"$catentdir\"\n"			/* SWBIS_DT_0002 */
		
		"				test -d \"$catalog_entry_dir\" 2>/dev/null\n"
		"				case $? in \n"
		"					0) tar cbf 1 - \"$catalog_entry_dir\" 2>/dev/null; ;;\n"
		"					*) dd if=/dev/zero count=2 2>/dev/null; ;;\n"
		"				esac\n"

		"				sw_retval=$?\n"
		"				# echo \"here is the swspec string: [$swspec_string]\" 1>&2\n"

		"				rp_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_report_status "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_report_status ";; *) shls_false_;; esac\n"
		"				ret=$sw_retval\n"
		"				swexec_status=$sw_retval\n"
		"				case $sw_retval in 0) %s ;; *) ;; esac\n" /* clear to send */
	
		CSHID
		"				shls_bashin2 \"" SWBIS_TS_Catalog_unpack "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Catalog_unpack  ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_Analysis_002 "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Analysis_002 ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_remove_files "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_remove_files  ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		CSHID
		"				shls_bashin2 \"" SWBIS_TS_Catalog_dir_remove "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Catalog_dir_remove ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_remove_catalog_entry "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_remove_catalog_entry ";; *) sleep 2; shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				%s\n" /* SW_SELECTION_EXECUTION_ENDS */
		,
/*_% */		TEVENT(2, vlv,  SW_SELECTION_EXECUTION_BEGINS, "$swspec_string"),
/*_% */		TEVENT(2, -1, SWI_TASK_CTS, "Clear to Send: status=0"),
/*_% */		TEVENT(2, vlv,  SW_SELECTION_EXECUTION_ENDS, "${swspec_string}: status=$sw_retval")
		);
	return strob_str(buf);
}

int
swremove_write_source_copy_script2(GB * G,
	int ofd, 
	char * targetpath, 
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
	char * dirname;
	char * basename;
	char * pax_write_command;
	char * opt_force_lock;
	char * opt_allow_no_lock;
	char * ignore_scripts;
	char * c_do_cleansh;
	char * xx;
	STROB * locked_region;
	STROB * looper_routine;
	STROB * buffer;
	STROB * buffer_new;
	STROB * shell_lib_buf;
	STROB * is_archive_msg;
	STROB * is_directory_msg;
	STROB * is_task_msg;
	STROB * subsh;
	STROB * subsh2;
	STROB * tmp;
	STROB * set_vx;
	STROB * to_devnull;
	STROB * isc_msg;
	int vlv;
	char * debug_task_shell;
	
	basename = (char*)NULL;
	dirname = (char*)NULL;
	buffer = strob_open(100);
	buffer_new = strob_open(100);
	to_devnull = strob_open(100);
	is_archive_msg = strob_open(100);
	is_directory_msg = strob_open(100);
	is_task_msg = strob_open(100);
	set_vx = strob_open(100);
	tmp = strob_open(100);
	subsh = strob_open(100);
	subsh2 = strob_open(100);
	isc_msg = strob_open(100);
	shell_lib_buf = strob_open(100);
	locked_region = strob_open(100);
	looper_routine = strob_open(100);

	vlv = G->g_verboseG;
	if (G->g_do_task_shell_debug == 0) {
		debug_task_shell="";
	} else {
		debug_task_shell="x";
	}
	
	if (G->g_do_cleanshM) {
		c_do_cleansh = "yes";
	} else {
		c_do_cleansh = "";
	}

	/* Sanity checks on targetpath */
	if (
		strstr(targetpath, "..") == targetpath ||
		strstr(targetpath, "../") ||
		swlib_is_sh_tainted_string(targetpath) ||
		0
	) { 
		return 1;
	}
	swlib_squash_double_slash(targetpath);

	/* assemble the the script verbosity controls */

	if (vlv >= SWC_VERBOSE_SWIDB) {
		strob_strcpy(set_vx, "set -vx\n");
	}
	
	if (vlv <= verbose_threshold ) {
		strob_strcpy(to_devnull, "2>/dev/null");
	}

	if (G->g_force_locks) {
		opt_allow_no_lock = "true";
		opt_force_lock = "true";
	} else {
		opt_allow_no_lock = "";
		opt_force_lock = "";
	}
	
	if (swextopt_is_option_true(SW_E_swbis_ignore_scripts, G->optaM)) {
		ignore_scripts="yes";
	} else {
		ignore_scripts="no";
	}

	/* Split targetpath into a basename and leading direcctory parts */
	
	/* leading directories */
	swlib_dirname(tmp, targetpath);
	dirname = strdup(strob_str(tmp));
	if (swlib_is_sh_tainted_string(dirname)) { return 1; }

	/* basename which may be a directory */
	swlib_basename(tmp, targetpath);
	basename = strdup(strob_str(tmp));      
	if (swlib_is_sh_tainted_string(basename)) { return 1; }

	/* This is a sanity check required by the assumption of the
	   code that reads this control message. */
	if (strchr(basename, '\n') || 
		strlen(basename) > MAX_CONTROL_MESG_LEN - 
			strlen(SWBIS_SWINSTALL_SOURCE_CTL_ARCHIVE ":")
	) {
		return 1;
	}

	/* these are the control messages that tell the management host
           whether the target path is a regular file or directory */
	strob_sprintf(is_archive_msg, 0, 
		"echo " "\"" SWBIS_SWINSTALL_SOURCE_CTL_ARCHIVE ": %s\"", basename);

	strob_sprintf(is_directory_msg, 0, 
		"echo " "\""SWBIS_SWINSTALL_SOURCE_CTL_DIRECTORY ": %s\"", basename);
	
	strob_sprintf(is_task_msg, 0, 
		"echo " "\""SWBIS_SWINSTALL_SOURCE_CTL_CLEANSH ": %s\"", basename);

	/* Determine what archive writing utility to use */
	
	pax_write_command = swc_get_pax_write_command(G->g_pax_write_commands,
						pax_write_command_key,
						G->g_verboseG, DEFAULT_PAX_W);


	/* Make a message for the SOURCE_ACCESS_BEGINS event when
	   reading the catalog */

	strob_sprintf(isc_msg, 0, "installed catalog at %s", get_opta_isc(G->optaM, SW_E_installed_software_catalog));


	/* Make a final adjustment if the path is '/', FIXME */
	if (
		strcmp(basename, "/") == 0 &&
		strcmp(dirname, "/") == 0
	) {
		free(basename);
		basename = strdup(".");	
	}


	looper_locked_region(locked_region, vlv);
	swpl_looper_payload_routine(looper_routine,  vlv, strob_str(locked_region));

	strob_strcpy(buffer_new, "");
	if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), "detect") == 0) {
		swpl_bashin_detect(buffer_new);
	} else if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), "posix") == 0) {
		swpl_bashin_posixsh(buffer_new);
	} else {
		swpl_bashin_testsh(buffer_new, get_opta(G->optaM, SW_E_swbis_shell_command));
	}

	/* Now here is the script */
	strob_sprintf(buffer_new, STROB_DO_APPEND,
		"IFS=\"`printf ' \\t\\n'`\"\n"
		"trap '/bin/rm -f ${LOCKPATH}.lock; exit 1' 1 2 15\n"
		"echo " SWBIS_TARGET_CTL_MSG_125 ": " KILL_PID "\n"
		"echo " SWBIS_TARGET_CTL_MSG_129 ": \"`pwd | head -1`\"\n"
		CSHID
		"export LOCKENTRY\n"
		"LOCKPATH=\"\"\n"
		"%s\n"  			/* SEVENT: SW_SESSION_BEGINS */
		"%s" 				/* swicol_subshell_marks */
		"wcwd=\"`pwd`\"\n"
		"export lock_did_lock\n"
		"export opt_force_lock\n"
		"export swbis_ignore_scripts\n"
		"export do_cleansh\n"
		"lock_did_lock=\"\"\n"
		"opt_force_lock=\"%s\"\n"
		"opt_allow_no_lock=\"%s\"\n"
		"export PATH\n"
		"PATH=`getconf PATH`:$PATH\n"
		"swbis_ignore_scripts=\"%s\"\n"
		"export swutilname\n"
		"swutilname=swremove\n"
		"do_cleansh=\"%s\"\n"
		"%s\n"				/* shls_bashin2 from shell_lib.sh */
		"%s\n"				/* shls_false_ from shell_lib.sh */
		"%s\n"				/* shls_looper from  shell_lib.sh */
		"%s\n"				/* shls_looper_payload from shell_lib.sh */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* shls__cleansh routine */
		"%s\n"			/* shls_cleansh routine */
		"%s\n"			/* shls_make_dir_absolute */
		"%s" 			/* set statement for verbosity */
		"export opt_to_stdout\n"
		"opt_to_stdout=\"\"\n"
		"blocksize=\"%s\"\n"
		"dirname=\"%s\"\n"
		"basename=\"%s\"\n"
		"targetpath=\"%s\"\n"
		"sw_targettype=unset\n"
		"sw_retval=0\n"
		"sb__delaytime=%d\n"
		"export sh_dash_s\n"
		"d_sh_dash_s=\"%s\"\n"
		"case \"$5\" in PSH=*) eval \"$5\";; *) unset PSH ;; esac\n"
		"sh_dash_s=\"${PSH:=$d_sh_dash_s}\"\n"
		"debug_task_shell=\"%s\"\n"
		"swexec_status=0\n"
		"export LOCKPATH\n"

		"#\n"
		"# make targetpath absolute\n"
		"#\n"
		"case \"$targetpath\" in\n"
		"/*)\n"
		";;\n"
		"*)\n"
		"mda_pwd=\"$wcwd\"\n"
		"mda_target_path=\"$targetpath\"\n"
		"shls_make_dir_absolute\n"
		"targetpath=\"$mda_target_path\"\n"
		"# targetpath is now absolute\n"
		";;\n"
		"esac\n"
		"#\n"
		"# END of code to make absolute\n"
		"#\n"

		"#\n"
		"# make dirname absolute\n"
		"#\n"
		"case \"$dirname\" in\n"
		"/*)\n"
		";;\n"
		"*)\n"
		"mda_pwd=\"$wcwd\"\n"
		"mda_target_path=\"$dirname\"\n"
		"shls_make_dir_absolute\n"
		"dirname=\"$mda_target_path\"\n"
		"# dirname is now absolute\n"
		";;\n"
		"esac\n"
		"#\n"
		"# END of code to make dirname absolute\n"
		"#\n"

		"# Here is the override of the shls_looper_payload function\n"
		"#\n"
		"%s\n"    /* <<<--- shls_looper_payload looper_routine */
		
		/* Here, in this first if-then statement we will classify and
		   type-test the target path */

		CSHID
		"case \"$do_cleansh\" in\n"
		"	yes)\n"
		"		%s\n"    /* Send is cleansh message */
		"		shls_cleansh\n"
		"		# set sw_retval to cause a fall through\n"
		"		sw_targettype=special\n"
		"		sw_retval=1\n"
		"		%s\n" /* SPECIAL_MODE_BEGINS */
		"		%s\n" /* SPECIAL_MODE_BEGINS */
		"		;;\n"
		"	*)\n"
		"	# Here is the normal usage\n"
		"if test -e \"$dirname\"; then\n"
		"	cd \"$dirname\"\n"
		"	sw_retval=$?\n"
		"	case $sw_retval in\n"
		"	0)\n"
				/* chdir succeeded */
		"		if test -d \"$basename\" -a -r \"$basename\"; then\n"
									/* ism_d1 */

						/* This must be a installation root with an 
						   installed_software catalog at the expected
						   location which is :
							<path>/<installed_software catalog>/  */
		"			sw_targettype=dir\n"
		"			%s\n"                           /* Send is_directory message */
		"		elif test -e \"$basename\" -a -r \"$basename\"; then\n"
									/* ism_a1 */
		"			sw_targettype=regfile\n"
		"			%s\n"                           /* Send is_archive message */
		"		else\n"
									/* access_error */
					/* error */
		"			sw_targettype=error\n"
		"			%s\n"
		"			%s\n"
		"		fi\n"
		"		;;\n"
		"	*)\n"
				/* chdir_failed */
		"		sw_targettype=error\n"
		"		%s\n"
		"		%s\n"
		"		;;\n"
		"	esac\n"
		"else\n"
									/* ism_else_fail */
			/* Bad file name or no access ... */
		"	%s\n"
		"	%s\n"
		"fi\n"
		";;\n"
		"esac    # do_cleansh \n"

		/* Here is where the real work begins, the target has been
			typed and tested, the management host has been
			notified via the "is_archive_msg" and "is_directory_msg"
			and the $sw_targettype var has been set.  */

		CSHID
		"case \"$sw_targettype\" in\n"
		"	\"regfile\")\n"
				/* This is an error for swremove */
				/* throw an error */
		"		sw_retval=1\n"
		"		;;\n"
		"	\"dir\")\n"
		"		%s\n" /* SOURCE_ACCESS_BEGINS */
		"		%s\n" /* SOURCE_ACCESS_BEGINS */
		"		swexec_status=0\n"

		CSHID
		"shls_bashin2 \"" SWBIS_TS_uts "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_uts ";; *) shls_false_;; esac\n"
		"sw_retval=$?\n"

		"shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Do_nothing ";; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		CSHID
		"shls_bashin2 \"" SWBIS_TS_Get_iscs_listing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Get_iscs_listing ";; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		"shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Do_nothing ";; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		/* Loop over the selections */
		"		shls_looper \"$sw_retval\"\n"
		"		sw_retval=$?\n"
		"		swexec_status=$sw_retval\n"

		"		shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Do_nothing  ";; *) shls_false_;; esac\n"
		"		sw_retval=$?\n"
		"		swexec_status=$sw_retval\n"

		"		%s\n"
		"		;; # case dir\n"
		"	\"unset\")\n"
					/* This is an error */
		"		sw_retval=1\n"
		"		;;\n"
		"	*)\n"
					/* This is an error */
		"		sw_retval=1\n"
		"		;;\n"
		"esac  # case sw_targettype\n"

		CSHID
		"if test \"$sw_retval\" != \"0\"; then\n"
		"	 sb__delaytime=0;\n"
		"fi\n"
		"sleep \"$sb__delaytime\"\n"
		"%s\n"
		"%s\n"
		"%s\n"
		,
									/* ism_begin */
/*_% */		TEVENT(2, vlv, SW_SESSION_BEGINS, ""),
/*_% */		swicol_subshell_marks(subsh, "install_target", 'L', nhops, vlv),
/*_% */		opt_force_lock,
/*_% */		opt_allow_no_lock,
/*_% */		ignore_scripts,
/*_% */		c_do_cleansh,
/*_% */		shlib_get_function_text_by_name("shls_bashin2", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_false_", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_looper", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_looper_payload", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_make_lockfile_name", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_make_lockfile_entry", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_test_lock", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_remove_lock", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls__cleansh", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls_cleansh", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls_make_dir_absolute", shell_lib_buf, NULL),
/*_% */		strob_str(set_vx),
/*_% */		blocksize,
/*_% */		dirname,
/*_% */		basename,
/*_% */		targetpath,
/*_% */		delaytime,
/*_% */		swc_get_default_sh_dash_s(G),
/*_% */		debug_task_shell,
/*_% */		strob_str(looper_routine),
/*_% */		strob_str(is_task_msg),
/*_% */		TEVENT(1, -1, SW_SPECIAL_MODE_BEGINS, "cleansh"),
/*_% */		TEVENT(2, vlv, SW_SPECIAL_MODE_BEGINS, "cleansh"),
/*_% */		strob_str(is_directory_msg),
/*_% */		strob_str(is_archive_msg),
/*_% */		TEVENT(2, vlv, SW_SOURCE_ACCESS_ERROR, basename),
/*_% */		TEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, basename),
/*_% */		TEVENT(2, vlv, SW_SOURCE_ACCESS_ERROR, dirname),
/*_% */		TEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, dirname),
/*_% */		TEVENT(2, vlv,  SW_SOURCE_ACCESS_ERROR, targetpath),
/*_% */		TEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, targetpath),
/*_% */		TEVENT(1, -1, SW_SOURCE_ACCESS_BEGINS, strob_str(isc_msg)),
/*_% */		TEVENT(2, vlv, SW_SOURCE_ACCESS_BEGINS, strob_str(isc_msg)),
/*_% */		TEVENT(2, vlv, SW_SOURCE_ACCESS_ENDS, "status=$sw_retval"),
/*_% */		TEVENT(2, -1, SWI_MAIN_SCRIPT_ENDS, "status=0"),
/*_% */		TEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
/*_% */		swicol_subshell_marks(subsh2, "install_target", 'R', nhops, vlv)
		);

	xx = strob_str(buffer_new);
	ret = atomicio((ssize_t (*)(int, void *, size_t))write, ofd, xx, strlen(xx));
	if (ret != (int)strlen(xx)) {
		return 1;
	}

	free(basename);
	free(dirname);
	strob_close(tmp);
	if (G->g_source_script_name) {
		swlib_tee_to_file(G->g_source_script_name, -1, xx, -1, 0);
	}

	strob_close(locked_region);
	strob_close(looper_routine);
	strob_close(buffer_new);
	strob_close(buffer);
	strob_close(is_archive_msg);
	strob_close(is_directory_msg);
	strob_close(is_task_msg);
	strob_close(set_vx);
	strob_close(to_devnull);
	strob_close(subsh);
	strob_close(subsh2);
	strob_close(isc_msg);
	strob_close(shell_lib_buf);
	/*
	 * 0 is OK
	 * !0 is error
	 */
	return !(ret > 0);
}

int
swremove_looper_sr_payload(GB * G, char * target_path, SWICOL * swicol,
		SWICAT_SR * sr, int ofd, int ifd, int * p_rp_status, SWUTS * uts)
{
	int retval;
	int rstatus;
	int ret;
	int do_skip_entry;
	int checkremove_status;
	int do_check_abort;
	int do_enforce_scripts;
	char * pax_read_command;
	char * epath;
	char * installed_software_catalog;
	char * catalog_entry_directory;
	STROB * btmp;
	STROB * btmp2;
	STROB * swspec_string;
	SWICAT_E * e;
	SWI * swi;
	SWI_FILELIST * file_list;
	SWGPG_VALIDATE * swgpg;
	CISF_PRODUCT * cisf;
	SWI_PRODUCT * current_product;
	
	do_skip_entry = 0;
	retval = 0;
	rstatus = 0;
	btmp = strob_open(100);
	btmp2 = strob_open(100);
	swspec_string = strob_open(100);
	swi = NULL;
	file_list = NULL;
	swgpg = NULL;
	e = NULL;
	checkremove_status = -1; /* unset value */

	do_enforce_scripts = swextopt_is_option_true(SW_E_enforce_scripts, G->optaM);
	pax_read_command = swc_get_pax_read_command(G->g_pax_read_commands,
		"tar", G->g_verboseG >= SWC_VERBOSE_3,
		0 /*keep_old_files*/, DEFAULT_PAX_R);
	E_DEBUG("");
	installed_software_catalog = get_opta_isc(G->optaM, SW_E_installed_software_catalog);
	catalog_entry_directory = swicat_sr_form_catalog_path(sr, installed_software_catalog, NULL);

	E_DEBUG2("catalog_entry_directory = [%s]", catalog_entry_directory);

	swicat_sr_form_swspec(sr, swspec_string);

	*p_rp_status = 0;
	if (strlen(catalog_entry_directory) == 0) {
		/* return with no error
		   this happens for `the empty response` to a query */
		return 1;
	}

	/* fprintf(stderr, "ENTRY: [%s]\n", catalog_entry_directory); */

	strob_sprintf(btmp, 0, "%s\n", catalog_entry_directory);
	E_DEBUG2("servicing looper: writing: [%s]", strob_str(btmp));

	/* write the catalog entry directory which becomes arg1 to
	   the shls_looper_payload() routine */

	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
			ofd,
			strob_str(btmp),
			strob_strlen(btmp)
			);
					
	if (swicol_get_master_alarm_status(swicol) != 0 ||
		 ret != (int)strob_strlen(btmp)
	) {
		/* error */
		sw_e_msg(G, "error from atomicio\n");
		return -1;
	}

	/* here is a gratuitous task shell that does nothing */

	E_DEBUG("");
	ret = swpl_send_success(swicol, ofd, G->g_swi_event_fd,
			SWBIS_TS_check_loop);
	if (ret != 0) {
		sw_e_msg(G, "error from swpl_send_success()\n");
		return -2;
	}

	/* Make a session lock */

	E_DEBUG("");
	ret = swpl_session_lock(G, swicol, target_path, ofd, G->g_swi_event_fd);
	sw_d_msg(G, "swpl_session_lock returned [%d]\n", ret);

	E_DEBUG2("swpl_session_lock returned %d", ret);
	swlib_squash_trailing_vnewline(strob_str(btmp));	
	if (ret < 0) {
		/* Internal error */
		sw_d_msg(G, "swpl_session_lock: lock fail for %s, ret=%d\n", strob_str(btmp), ret);
		sw_e_msg(G, "error from swpl_session_lock: status=%d\n", ret);
		return -4;
	} else if (ret > 0) {
		/* session in progress, or no access to make lock */
		sw_e_msg(G, "swpl_session_lock lock failed for %s, ret=%d\n", strob_str(btmp), ret);
		/* sw_d_msg(G, "swpl_session_lock lock failed for %s, ret=%d\n", strob_str(btmp), ret); */
		*p_rp_status = 1;
		return 1;
	} 

	/*
	 * ---------------------------------------------
	 * If we're here we got the lock
	 * ---------------------------------------------
	 */
	
	/* Here is where the real work begins
	   Perform the required actions, in accord with
	   the contents of shls_looper_payload() shell routine */

	E_DEBUG("");
	
	/*
	 * Send a line of text to use in the status messages.
	 */

	/* SWBIS_DT_0001 */
	swgp_write_as_echo_line(ofd, strob_str(swspec_string));

	/*
	 * read the line echo'ed in the remote script
	 */

	/* SWBIS_DT_0002 */
	swgp_read_line(ifd, btmp2, DO_APPEND);

	if (G->devel_verboseM)
		fprintf(stderr, "rp_status is %d\n", ret);

	e = swicat_e_create();
	ret = swicat_e_open_entry_tarball(e, ifd);
	if (ret != 0) {
		sw_e_msg(G, "error opening catalog entry tarball, ret=%d\n", ret);
		rstatus = 1;
		retval = -2;  /* protocol internal error */
		looper_abort(G, swicol, ofd);
		goto error_out;
	} else if (ret == SWICAT_RETVAL_NULLARCHIVE) {
		/* this is the case for a missing catalog
		   entry which may happen */
		do_skip_entry = 1;
		goto GL_skip;
	} else {
		; /* Normal path */
	}

	/* e->entry_prefixM has the installed_software_catalog
	  path already prefixed */
	epath = swicat_e_form_catalog_path(e, btmp, NULL, SWICAT_ACTIVE_ENTRY);
	if (G->devel_verboseM)
		SWLIB_INFO2("catalog entry (active) path = %s", epath);
	
	epath = swicat_e_form_catalog_path(e, btmp, NULL, SWICAT_DEACTIVE_ENTRY);
	if (G->devel_verboseM)
		SWLIB_INFO2("catalog entry path (inactive) path = %s", epath);

	epath = swicat_e_form_catalog_path(e, btmp, NULL, SWICAT_ACTIVE_ENTRY);

	/* epath is now a clean relative path to such as:
		var/lib/swbis/catalog/emacs/emacs/21.2.1/0  */

	ret = swicat_e_reset_fd(e);
	if (ret < 0) {
		rstatus = 2;
		retval = -3;
		sw_e_msg(G, "error reseting swicat_e object, ret=%d\n", ret);
		looper_abort(G, swicol, ofd);
		goto error_out;
	}

	/* Now obtain the verification status for each signature */
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	swgpg = swgpg_create();
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	E_DEBUG("");

	ret = swicat_e_verify_gpg_signature(e, swgpg);
	if (ret != 0) {
		rstatus = 3;
		retval = 0;
		sw_e_msg(G, "error from swicat_e_verify_gpg_signature, ret=%d\n", ret);
	}

	/* Now interpret the verification results according to extended option
	   requirements */

	ret = swpl_signature_policy_accept(G, swgpg, G->g_verboseG, strob_str(swspec_string));
	if (ret != 0) {
		/* Bad signature or not enough good signatures
		   Can't use the blob to make a removal file list
		   because were assuming its tainted. */
		rstatus = SWP_RP_STATUS_NO_INTEGRITY;
		retval = 0;
		do_skip_entry = 1;
	} else {
		/* Signatures OK, or don't care */
		;
	}

	/*
	 * If we're here its OK to decode the catalog.tar file to create (SWI*)swi
	 * because its authenticated integrity or we don't care.
	 */

	swi = swicat_e_open_swi(e);
	if (swi == NULL) {
		/* swi might be NULL due to file system permission,
		   handle NULL gracefully */
		do_skip_entry = 1;
		sw_e_msg(G, "catalog read error, possible permission denied: %s/%s\n", target_path, epath);
	} else {
		swi->swi_pkgM->catalog_entryM = strdup(epath); /* FIXME, this must set explicitly */
		swi->swi_pkgM->target_pathM = strdup(target_path); /* FIXME, this must set explicitly */
		swi->optaM = G->optaM;
	}

	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	/* Create the CISF_PRODUCT object */
	current_product = swi_package_get_product(swi->swi_pkgM, D_ZERO /* FIXME, assumes the first one */);
	cisf = swpl_cisf_product_create(current_product);

	/*
	 * FIXME
	 * Initialize the CISF object according to assumptions
	 * of one product and one fileset  FIXME
	 */	

	/* FIXME, this should be based on INSTALL, not the single
	   product and single fileset assumption */
	swpl_cisf_init_single_single(cisf, swi);

	GL_skip:

	/* SWBIS_TS_report_status
	The purpose of this is solely to provide an opportunity for
	the remote script to report a problem and to re-verify script-data
	sychronization. */
	
	ret = swpl_report_status(swicol, ofd, G->g_swi_event_fd);
	*p_rp_status = ret;
	E_DEBUG2("rp_status is %d", ret);
	if (ret) {
		rstatus = 6;
		retval = -4;
		goto error_out;
	}
	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	/* wait for clear-to-send */

	ret = swicol_rpsh_wait_cts(swicol, G->g_swi_event_fd);
	*p_rp_status = ret;
	E_DEBUG2("rp_status is %d", ret);
	if (ret) {
		rstatus = -1;
		retval = -5;
		goto error_out;
	}

	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	if (swi) 
		file_list = swicat_e_make_file_list(e, uts, swi);
	else
		file_list = NULL;

	if (swi && file_list == NULL) {
		/* empty package, no files */
		rstatus = 7;
		retval = 0;
		goto error_out;
	}
	
	if (
		G->g_opt_previewM  ||
		G->g_verboseG > SWC_VERBOSE_3 ||
		G->devel_verboseM ||
		0
	) {
		if (file_list) show_file_list(G, file_list);
	}

	if (G->devel_verboseM)
		SWLIB_INFO2("%s", strob_str(btmp));

	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_Catalog_unpack,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			E_DEBUG("");
			rstatus = -2;
			retval = -8;
			goto error_out;
		}
	} else {
		ret = swpl_unpack_catalog_tarfile(G, swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd);
		if (ret != 0) {
			E_DEBUG("");
			rstatus = -3;
			retval = -9;
			goto error_out;
		}
	}

	/* Analysis Phase and checremove script */
	/* SWBIS_TS_Analysis_002 */

	E_DEBUG("HERE");
	do_check_abort = 0;
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_Analysis_002,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			E_DEBUG("");
			rstatus = -4;
			retval = -10;
			goto error_out;
		}
	} else {
		/*
		 * TS_analysis_002
		 */
		E_DEBUG("HERE");
		ret = swpl_run_check_script(G, SW_A_checkremove, SWBIS_TS_Analysis_002, swi, ofd, G->g_swi_event_fd, &checkremove_status);
		if (ret < 0) {
			/* internal error */
			retval++;
			E_DEBUG2("retval = %d", retval);
		} else {
			/* process the checkremove enforcement policy 
			   based on the script exit status */
			
			if (checkremove_status == SW_SUCCESS) {
				;	
			} else if (checkremove_status == SW_WARNING) {
				;
			} else if (checkremove_status == SW_ERROR) {
				if (do_enforce_scripts) {
					do_check_abort = 1;
				}
			} else if (checkremove_status == SW_DESELECT) {
				/* even if do_enforce_scripts == 0, enforce this status
				   as meaning deselect, only if forced override */
				if (G->g_force == 0) {
					do_check_abort = 1;
				}
			} else if (checkremove_status == -1) {
				/* OK, probably no checkremove script */
				;
			} else {
				/* unrecognized status */
				sw_e_msg(G, "unrecognized exit status for checkremove: %d\n", checkremove_status);
				retval++;
			}

		}
	}

	/* SWBIS_TS_remove_files */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		do_check_abort ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_remove_files,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			rstatus = -7;
			retval = -10;
			goto error_out;
		}
	} else {
		E_DEBUG("remove files");
		ret = remove_files(G, target_path, cisf, swi, swicol, e, file_list, ofd);
		if (ret < 0) {
			/* internal error */
			E_DEBUG("remove files: internal error");
			rstatus = -8;
			retval = -11;
			goto error_out;
		} else if (ret > 0) {
			/* script error */
			E_DEBUG("remove files: script error");
			rstatus = 9;
			goto error_out;
		}
	}

	/* SWBIS_TS_Catalog_dir_remove
	   This removes the .../export/name-version  directory
	   i.e. the opposite of Catalog_unpack */

	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_Catalog_dir_remove,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			rstatus = -10;
			retval = -12;
			goto error_out;
		}
        } else {
                /* TS_Catalog_dir_remove */
                ret = swpl_remove_catalog_directory(swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd);
		if (ret != 0) {
			rstatus = -11;
			retval = -13;
			goto error_out;
		}
	}

	/* SWBIS_TS_remove_catalog_entry */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		do_check_abort ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_remove_catalog_entry,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			retval = -13;
		}
	} else {
		ret = swpl_remove_catalog_entry(G, swicol, e, ofd, NULL);
		if (ret < 0) {
			/* internal error */
			E_DEBUG("remove catalog: internal error");
			rstatus = -12;
			retval = -14;
			goto error_out;
		} else if (ret > 0) {
			/* script error */
			E_DEBUG("remove catalog: script error");
			rstatus = 10;
		}
	}

	error_out:

	strob_close(btmp);
	strob_close(btmp2);
	strob_close(swspec_string);
	if (file_list)
		swi_fl_delete(file_list);

	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);
	E_DEBUG2("rp_status is now rstatus: %d", rstatus);
	if (rstatus)
		*p_rp_status = rstatus;
	if (swi) swi_delete(swi);
	if (swgpg) swgpg_delete(swgpg);
	if (e) swicat_e_delete(e);
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	return retval;
}
