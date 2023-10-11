/*  swproglib.c -- general purpose routines common among the sw<utiltities>

 Copyright (C) 2006,2007,2008,2009,2010 Jim Lowe
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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "strob.h"
#include "cplob.h"
#include "vplob.h"
#include "swlib.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swcommon0.h"
#include "swcommon.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "etar.h"
#include "strar.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swevents.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "swinstall.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swicat.h"
#include "swicat_e.h"
#include "strar.h"
#include "swi.h"
#include "atomicio.h"
#include "swutilname.h"
#include "swproglib.h"
#include "shlib.h"
#include "swgpg.h"
#include "ls_list.h"
#include "fnmatch_u.h"

static SCAR * g_script_array[SCAR_ARRAY_LEN+1];

static
void
select_error_codes(char * script_tag, int * event_error_value, int * event_warning_value)
{

	/*
	 * script_tag is either preinstall, postinstall, configure, or
	 * postremove, preremove
	 */

	if (
		strcmp(script_tag, SW_A_postinstall) == 0 ||
		strcmp(script_tag, SW_A_postremove) == 0 ||
		0
	) {
		*event_error_value = SW_POST_SCRIPT_ERROR;
		*event_warning_value = SW_POST_SCRIPT_WARNING;
	} else if (
		strcmp(script_tag, SW_A_preinstall) == 0 ||
		strcmp(script_tag, SW_A_preremove) == 0 ||
		0
	) {
		*event_error_value = SW_PRE_SCRIPT_ERROR;
		*event_warning_value = SW_PRE_SCRIPT_WARNING;
	} else if (
		strcmp(script_tag, SW_A_configure) == 0 ||
		strcmp(script_tag, SW_A_unconfigure) == 0 ||
		0
	) {
		*event_error_value = SW_CONFIGURE_ERROR;
		*event_warning_value = SW_CONFIGURE_WARNING;
	} else {
		*event_error_value = SW_CONFIGURE_ERROR;
		*event_warning_value = SW_CONFIGURE_ERROR;
		SWLIB_INTERNAL("");
	}
}

static
void
make_padding_for_login_shell(int blocks, STROB * buf)
{
	int i,j,k;
	for (k=0; k<blocks; k++) {
		for (j=0; j<=7; j++) {
			for (i=0; i<=6; i++) {
				strob_sprintf(buf, STROB_DO_APPEND, "########");
			}
			strob_sprintf(buf, STROB_DO_APPEND, "#######\n");
		}
	}
}

static
int
environ_exclude(char * varname)
{
	char * s;
	int retval;
	int ret;
	char * pat;
	char * exc;
	STROB * patlist;
	const char exclude_list[] = ":_:PATH:LD_LIBRARY_PATH:IFS:LD_PRELOAD:"
				":SHELL:HOME:LANG:USER:TMPDIR:TERM:TERMCAP:TZ:MAIL:XAUTHORITY:"
				":LOGNAME:HOME:BASH_ENV:CDPATH:DISPLAY:HOSTNAME:HISTSIZE:"
				":SSH_ASKPASS:SSH_AUTH_SOCK:SSH_CONNECTION:SSH_ORIGINAL_COMMAND:SSH_TTY:"
				":SSH_AGENT_PID:SSH_AGENT_PID:"
				":GPG_AGENT_INFO:PWD:LS_COLORS:WINDOWID:"
				":G_BROKEN_FILENAMES:LESSOPEN:SHLVL:ENV:OLDPWD:"
				"";
	const char pattern_list[] = ":CVS*:SSH*:JMM*:GPG*:JX_*";

	retval = 0;
	if (strchr(varname, ':')) return 1;

	exc = (char*)exclude_list;
	while ((s=strstr(exc, varname))) {
		if (*(s-1) == ':' && *(s+strlen(varname)) == ':') {
			retval = 1;
			break;
		}
		exc = s+strlen(varname);
	}

	if (retval) return 1;
	
	patlist = strob_open(32);
	pat = strob_strtok(patlist, (char*)pattern_list, ":\r\n");
	while(pat && strlen(pat)) {
		ret = fnmatch(pat, varname, 0);
		if (ret == 0) {
			retval = 1;
			break;
		}
		pat = strob_strtok(patlist, NULL, ":\r\n");
	}
	strob_close(patlist);
	return retval;
}

static
void
abort_script(SWICOL * swicol, GB * G, int fd)
{
	swicol_set_master_alarm(swicol);
	swpl_send_abort(swicol, fd, G->g_swi_event_fd, "");
}

static
int
set_ls_list_flags(void)
{
	int ls_verbose;
	ls_verbose = 0;
	ls_verbose &= ~(LS_LIST_VERBOSE_WITH_MD5|LS_LIST_VERBOSE_WITH_SHA1|LS_LIST_VERBOSE_WITH_SHA512);
	ls_verbose &= ~(LS_LIST_VERBOSE_WITH_SIZE|LS_LIST_VERBOSE_WITH_ALL_DATES);
	return ls_verbose;
}

static
void
merge_name_id(int * ls_verbose, int available_attributes)
{
	int x;
	x = *ls_verbose;
	if (available_attributes & LS_LIST_VERBOSE_WITH_SYSTEM_IDS) {
		x |= LS_LIST_VERBOSE_WITH_SYSTEM_IDS;
	}
	if (available_attributes & LS_LIST_VERBOSE_WITH_SYSTEM_NAMES) {
		x |= LS_LIST_VERBOSE_WITH_SYSTEM_NAMES;
	}
	*ls_verbose = x;
}

static
void
apply_location_to_path(STROB * buf, struct new_cpio_header * file_hdr, char * location)
{
	strob_strcpy(buf, location);
	swlib_squash_leading_dot_slash(strob_str(buf));
	swlib_squash_all_leading_slash(strob_str(buf));
	swlib_unix_dircat(buf, ahsStaticGetTarFilename(file_hdr));
	ahsStaticSetTarFilename(file_hdr, strob_str(buf));
}

static
int
determine_ls_list_flags(int attr, struct new_cpio_header * file_hdr, int check_contents)
{

	FILE_DIGS * digs;
	int ls_verbose;

	ls_verbose = 0;
	E_DEBUG("");

	if (attr & TARU_UM_UID && attr & TARU_UM_OWNER) {
		E_DEBUG("Has owner and uid");
		ls_verbose |= LS_LIST_VERBOSE_WITH_SYSTEM_IDS;
		ls_verbose |= LS_LIST_VERBOSE_WITH_SYSTEM_NAMES;
	} else if (attr & TARU_UM_UID) {
		E_DEBUG("Has uid only");
		ls_verbose |= LS_LIST_VERBOSE_WITH_SYSTEM_IDS;
		ls_verbose &= ~LS_LIST_VERBOSE_WITH_SYSTEM_NAMES;
	} else if (attr & TARU_UM_OWNER) {
		E_DEBUG("Has owner only");
		ls_verbose &= ~LS_LIST_VERBOSE_WITH_SYSTEM_IDS;
		ls_verbose |= LS_LIST_VERBOSE_WITH_SYSTEM_NAMES;
	}

	if (check_contents == 0) {
		ls_verbose &= ~(LS_LIST_VERBOSE_WITH_MD5|LS_LIST_VERBOSE_WITH_SHA1|LS_LIST_VERBOSE_WITH_SHA512);
		ls_verbose &= ~(LS_LIST_VERBOSE_WITH_SIZE);
	}

	digs = file_hdr->digsM;
	if (digs) {
		if (digs->do_md5 == DIGS_ENABLE_ON ) {
			E_DEBUG("Turning on md5");
			ls_verbose |= LS_LIST_VERBOSE_WITH_MD5;
		}
		if (digs->do_sha1 == DIGS_ENABLE_ON ) {
			E_DEBUG("Turning on sha1");
			ls_verbose |= LS_LIST_VERBOSE_WITH_SHA1;
		}
		if (digs->do_sha512 == DIGS_ENABLE_ON ) {
			E_DEBUG("Turning on sha512");
			ls_verbose |= LS_LIST_VERBOSE_WITH_SHA512;
		}
	} else {
		E_DEBUG("Turning off digests");
		ls_verbose &= ~(LS_LIST_VERBOSE_WITH_MD5|LS_LIST_VERBOSE_WITH_SHA1|LS_LIST_VERBOSE_WITH_SHA512);
	}
	return ls_verbose;
}


static
int
determine_here_document_stop_word_bystr(STROB * here_payload, STROB * stopbuf)
{
	char * ret_t;
	char * ret;
	int n;
	n = 0;

	E_DEBUG("");
	do {
		strob_set_length(stopbuf, L_tmpnam+1);
		ret_t = tmpnam(strob_str(stopbuf));
		if (ret_t == NULL) return 2;
		ret = strob_strstr(here_payload, strob_str(stopbuf));
		if (ret == NULL) {
			E_DEBUG("");
			return 0; /* unique character sequence found */
		}	
		n++;
	} while (ret == 0 && n < 10);
	E_DEBUG("");
	return 1; /* unique character sequence not found */
}


static
int
determine_here_document_stop_word(SWI_FILELIST * fl, STROB * stopbuf)
{
	int ret;
	STROB * payload;
	payload = strob_open(100);
	swpl_print_file_list_to_buf(fl, payload);
	ret = determine_here_document_stop_word_bystr(payload, stopbuf);
	strob_close(payload);	
	return ret;
}


static
int
write_control_script_code_fragment(GB * G, SWI * swi, SWI_CONTROL_SCRIPT * script, char * swspec_tags, STROB * buf)
{
	STROB * tmp;
	STROB * tmp2;
	STROB * tmp3;
	
	tmp = strob_open(100);
	tmp2 = strob_open(100);
	tmp3 = strob_open(100);

	strob_sprintf(tmp, STROB_DO_APPEND,
		"%s %s", swspec_tags, script->baseM.b_tagM);

	strob_strcpy(tmp2, strob_str(tmp));
	if (G->g_opt_previewM) {
		strob_strcat(tmp2, ": "  SWEVENT_ATT_STATUS_PREVIEW);
	} else {
		strob_strcat(tmp2, ": " SWEVENT_STATUS_PFX "$sw_retval");
	}
	
	strob_sprintf(tmp3, STROB_DO_APPEND, SW_A_SCRIPT_ID "=%d", script->sidM);

	strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"# Start of swpl_write_control_script_code_fragment\n"
		"	cscf_opt_preview=\"%d\"\n"
		"	cd \"%s\" || exit 44\n"
		"	cd \"%s\" || exit 45\n"
		"	script_control_val=$?\n"
		"	case \"$swbis_ignore_scripts\" in yes) script_control_val=_ignore_;; esac\n"
		"	case \"$cscf_opt_preview\" in 1) script_control_val=_preview_;; esac\n"
		"	case $script_control_val in  # Case_swproglib.c_002\n"
		"		0)\n"
		"			%s\n"
		"			%s\n"
		"			" SWBIS_PGM_SH " " SW_A_CONTROL_SH " \"%s\" \"%s\"\n"
		"			sw_retval=$?\n"
		"			%s\n"
		"			;;\n"
		"		_preview_)\n"
		"			%s\n"
		"			%s\n"
		"			sw_retval=0\n"
		"			%s\n"
		"			;;\n"
		"		_ignore_)\n"
		"			sw_retval=0\n"
		"			;;\n"
		"		*)\n"
		"			sw_retval=" SWBIS_STATUS_COMMAND_NOT_FOUND "\n"
		"			;;\n"
		"	esac  # Case_swproglib.c_002\n"
		"# End of swpl_write_control_script_code_fragment\n",
		G->g_opt_previewM,
		swi->swi_pkgM->target_pathM,
		swi->swi_pkgM->catalog_entryM,
		TEVENT(2, (swi->verboseM), SW_CONTROL_SCRIPT_BEGINS, strob_str(tmp)),
		TEVENT(2, (swi->verboseM), SWI_MSG, strob_str(tmp3)),
		swspec_tags,
		script->baseM.b_tagM,
		TEVENT(2, (swi->verboseM), SW_CONTROL_SCRIPT_ENDS, strob_str(tmp2)),
		TEVENT(2, (swi->verboseM), SW_CONTROL_SCRIPT_BEGINS, strob_str(tmp)),
		TEVENT(2, (swi->verboseM), SWI_MSG, strob_str(tmp3)),
		TEVENT(2, (swi->verboseM), SW_CONTROL_SCRIPT_ENDS, strob_str(tmp2))
		);
	
	strob_close(tmp);
	strob_close(tmp2);
	strob_close(tmp3);
	return 0;
}

static
int
construct_script_cases(GB * G, STROB * buf, SWI * swi, 
		SWI_CONTROL_SCRIPT * script, 
		char * tagspec,
		char * script_tag,
		int error_event_value,
		int warning_event_value)
{
	int ret;
	struct extendedOptions * opta;
	int script_retcode;
	int swi_retcode;
	STROB * tmp2;
	STROB * tmp3;

	tmp2= strob_open(48);
	tmp3 = strob_open(48);
	opta = swi->optaM;
	SWLIB_ASSERT(script != NULL);

	/*
	 * write main script fragment
	 */
	E_DEBUG("");

	ret = write_control_script_code_fragment(G, swi, script, tagspec, buf);
	SWLIB_ASSERT(ret == 0);

	/*
	 * write some shell code that monitors
	 * return status of the script
	 */

	if (0 && strcmp(swlib_utilname_get(), SW_UTN_CONFIG) == 0) {
		/* disable this */
		/* enforce_script will be used by swconfig in this
		   implementation */

		/* the enforce_scripts option is not used by swconfig
		   therefore avoid using it if the utility is 'swconfig' */
		script_retcode = SW_WARNING;
		swi_retcode = 0;
	} else {
		E_DEBUG("");
		if ( swextopt_is_option_true(SW_E_enforce_scripts, opta)) {
			E_DEBUG("SW_E_enforce_scripts is true");
			script_retcode = SW_ERROR;
			swi_retcode = 1;
		} else {
			E_DEBUG("SW_E_enforce_scripts is false");
			script_retcode = SW_WARNING;
			swi_retcode = 0;
		}
	}

	strob_sprintf(tmp2, 0, "%s: status=$sw_retval", script_tag);
	strob_sprintf(tmp3, 0, "%s: status=$script_retval", script_tag);

	strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"# Start of construct_script_cases\n"
		"	case \"$sw_retval\" in\n"
		"		" SWBIS_STATUS_COMMAND_NOT_FOUND ")\n"
		"			# Special return code of control.sh which means\n"
		"			# no script for the specified control_script tag\n"
		"			sw_retval=0\n"
		"			;;\n"
					
		"		%d)\n"
		"			sw_retval=0\n"
		"			;;\n"
					/* Do nothing */
		"		%d)\n"
		"			%s\n"
		"			sw_retval=0\n"
		"			;;\n"
	
		"		%d)\n"
		"			script_retval=%d\n"
		"			%s\n"
		"			sw_retval=\"%d\"\n"
		"			;;\n"
		"		*)\n"
		"			sw_retval=1\n"
		"			;;\n"
		"	esac\n"
		"# End of construct_script_cases\n" ,
	SW_SUCCESS,
	SW_WARNING,
	TEVENT(2, (swi->verboseM), warning_event_value, strob_str(tmp2)),
	SW_ERROR,
	script_retcode,
	TEVENT(2, (swi->verboseM), error_event_value, strob_str(tmp3)),
	swi_retcode
	);	
	strob_close(tmp2);
	strob_close(tmp3);
	E_DEBUG("");
	return 0;
}

static
int
form_entry_names(GB * G, SWICAT_E * e, char * iscpath, STROB * tmp_epath, STROB * tmp_depath)
{
	char * epath; 
	char * depath;
	char * s_ret1; 
	char * s_ret2; 
	int retval;
	
	retval = 0;

	E_DEBUG("Entering");
	E_DEBUG2("iscpath=[%s]", iscpath);
	s_ret1 = swicat_e_form_catalog_path(e, tmp_epath, iscpath, SWICAT_ACTIVE_ENTRY);
	epath = strob_str(tmp_epath);
	E_DEBUG2("epath=[%s]", epath);

	s_ret2 = swicat_e_form_catalog_path(e, tmp_depath, iscpath, SWICAT_DEACTIVE_ENTRY);
	depath = strob_str(tmp_depath);
	E_DEBUG2("dpath=[%s]", depath);

	if (swlib_check_clean_path(epath) != 0) {
		sw_e_msg(G, "tainted path name: %s\n", epath);
		strob_strcpy(tmp_epath, "");
		s_ret1 = NULL;
	}
	
	if (swlib_check_clean_path(depath) != 0) {
		sw_e_msg(G, "tainted path name: %s\n", depath);
		strob_strcpy(tmp_depath, "");
		s_ret2 = NULL;
	}

	if (s_ret1 == NULL || s_ret2 == NULL) {
		return -1;
	}

	return 0;
}

static
int
common_remove_catalog_entry(GB * G, SWICOL * swicol, int ofd, char * epath, char * depath, char * task_id)
{
	STROB * btmp;
	STROB * dir2;
	int retval;
	int ret;
	
	retval = 0;
	btmp = strob_open(32);
	dir2 = strob_open(32);

	strob_strcpy(dir2, epath);
	swlib_unix_dirtrunc_n(dir2, 2);
	
	if (strcmp(task_id, SWBIS_TS_remove_catalog_entry) == 0) {
		strob_sprintf(btmp, 0,
			CSHID
			"	# Remove all old catalog entries\n"
			" 	(cd \"%s\" && find . -type d -name '_[0-9]*' -exec rm -fr {} \\; 2>/dev/null ) </dev/null \n",
			strob_str(dir2)
		);
	}

	strob_sprintf(btmp, 1,
		CSHID
		"	rm -fr \"%s\" 1>&2\n"
		"	mv -f \"%s\" \"%s\" 1>&2\n"
		"	sw_retval=$?\n"
		" 	(cd \"%s\" && rmdir * 2>/dev/null ) </dev/null \n"
		"	case $sw_retval in 0) ;; *) sw_retval=2 ;; esac\n"
		"	dd count=1 bs=512 of=/dev/null 2>/dev/null\n",
		depath,
		epath,
		depath,
		strob_str(dir2)
		);

	ret = swicol_rpsh_task_send_script2 (
		swicol,
		ofd, 		/* file descriptor */
		512,		/* null dummy block */
		swicol->targetpathM, /* i.e. stay in the target path */
		strob_str(btmp),/* The actual task script */
		task_id
		);
	if (ret < 0) {
		retval++;
		goto error_out;
	}

	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret < 0) {
		retval++;
		goto error_out;
	}

	ret = swicol_rpsh_task_expect(swicol, G->g_swi_event_fd,
			SWICOL_TL_9 /*time limit*/);

	if (ret < 0) {
		retval = ret;
		SWLIB_INTERNAL("");
		goto error_out;
	}

	if (ret > 0) {
		retval = ret;
	}

	error_out:

	strob_close(btmp);
	strob_close(dir2);
	return retval;
}

static int
alter_catalog_entry(int remove_restore, GB * G, SWICOL * swicol, SWICAT_E * e, int ofd, char * iscpath)
{
	STROB * tmp;
	STROB * tmp2;
	char * epath; 
	char * depath;
	int retval;
	int ret;
	
	retval = 0;
	tmp = strob_open(32);
	tmp2 = strob_open(32);

	ret = form_entry_names(G, e, iscpath, tmp, tmp2);
	if (ret)
		return 1;

	epath = strob_str(tmp);
	depath = strob_str(tmp2);
	if (remove_restore == 0)
		ret = common_remove_catalog_entry(G, swicol, ofd, epath, depath, SWBIS_TS_remove_catalog_entry);
	else
		ret = common_remove_catalog_entry(G, swicol, ofd, depath, epath, SWBIS_TS_restore_catalog_entry);

	if (ret)
		return 2;

	strob_close(tmp);
	strob_close(tmp2);
	return retval;
}

static
int
parse_ls_ld_output(SWI * swi, char * ls_output)
{
	char * line;
	char * value;
	char * attribute;
	int ret;
	STROB * tmp;
	char * s;
	mode_t mode;
	char * xp;
	char * s_mode;
	char * owner;
	char * group;
	
	/*
	The line to parse looks something like this

	swinstall: swicol: 315:ls_ld=drwxr-x--- jhl/other 0 2008-10-30 20:46 var/lib/swbis/catalog/
	*/

	E_DEBUG2("ls output = [%s]", ls_output);
	ret = 0;
	tmp = strob_open(10);
	line = strob_strtok(tmp, ls_output, "\r\n");
	while (line) {
		ret = swevent_parse_attribute_event(line, &attribute, &value);
		if (ret == 0) {
			E_DEBUG("");
			if (strcmp(attribute, "ls_ld") == 0) {
				/*
				fprintf(stderr, "attribute=[%s]\n", attribute);	
				fprintf(stderr, "value=[%s]\n", value);	
				*/
				/* Now parse the value to get the perms, owner, and group
				   The value looks like
				GNU tar:  drwxr-x--- jhl/other 0 2008-10-30 20:46 var/lib/swbis/catalog/
					--or--
				STAR:     0 drwxr-s---  jhl/other Dec 14 20:53 2010 var/lib/swbis/catalog/

				bsdtar:   drwxr-s---  0  jhl  other  0 Dec 14 20:53 2010 var/lib/swbis/catalog/
				 */
				E_DEBUG("");

				E_DEBUG2("value=[%s]", value);
				/* Step over white space and numbers, 'star' places the size first
				   in  ``tar tvf -`` listing */
				E_DEBUG("");
				xp = value;
				while (xp && *xp && isspace((int)*xp)) xp++;
				while (xp && *xp && isdigit((int)*xp)) xp++;
				value = xp;
				E_DEBUG("");

				s_mode = strob_strtok(tmp, value, " ");
				if (!s_mode) return -1;
				s = strob_strtok(tmp, NULL, " ");
				if (!s) return -2;
				owner = s;
				E_DEBUG("");

				/* In bsdtar this could be a single digit (link count?) */
				if (strlen(s) == 1 && isdigit((int)(*s))) {
					/* Step over the number */
					s = strob_strtok(tmp, NULL, " ");
					owner = s;
				}
				E_DEBUG("");

				s = strchr(s, '/');
				if (!s) {
					/* must have space separated owner and group.
					   bsdtar does this */
					s = strob_strtok(tmp, NULL, " ");
					if (!s) return -4;
					group = s;
				} else {
					*s = '\0';
					s++;
					group = s;
				}

				mode = swlib_filestring_to_mode(s_mode, 0);

				if (strlen(owner) && strlen(group) && mode > 0666) {
					E_DEBUG2("owner=%s", owner);
					E_DEBUG2("group=%s", group);
					E_DEBUG2("mode=%s", s_mode);
					swi->swi_pkgM->installed_catalog_ownerM = strdup(owner);
					swi->swi_pkgM->installed_catalog_groupM = strdup(group);
					swi->swi_pkgM->installed_catalog_modeM = mode;
				} else {
					E_DEBUG2("FAILED:  owner=%s", owner);
					E_DEBUG2("FAILED:  group=%s", group);
					E_DEBUG2("FAILED:  mode=%s", s_mode);
					return -5;
				}

				/* Test point
				taru_mode_to_chars(mode, buf, sizeof(buf), '\0');
				fprintf(stderr, "[%s] [%s] [%s] [%s]\n", owner, group, s_mode, buf);
				*/
				ret = 0;
				break;
			} else {
				E_DEBUG("");
				fprintf(stderr, "bad message: %s\n", attribute);
				ret = -1;
				break;
			}
		} else {
			/* this happens */
			E_DEBUG("");
			ret = 0;
		}
		E_DEBUG("");
		line = strob_strtok(tmp, NULL, "\r\n");
	}
	E_DEBUG("");
	strob_close(tmp);
	return ret;
}

static
int
i_construct_analysis_script(GB * G, char * script_name, STROB * buf, SWI * swi, SWI_CONTROL_SCRIPT ** p_script)
{
	int ret;
	SWI_PRODUCT * prod;
	SWI_CONTROL_SCRIPT * script;
	struct extendedOptions * opta;
	int checkscript_retcode;
	int swi_retcode;
	int check_def_retcode;

	opta = swi->optaM;

	prod = swi_package_get_product(swi->swi_pkgM, 0 /* FIXME The first product */);
	if (swi_product_has_control_file(prod, script_name)) {
	
		strob_sprintf(buf, STROB_DO_APPEND,
		"	cd \"%s\"\n",
		swi->swi_pkgM->target_pathM);
	
		/*
		 * Write the code for the check script
		 */
	
		script = swi_product_get_control_script_by_tag(prod, script_name);
		SWLIB_ASSERT(script != NULL);
		ret = write_control_script_code_fragment(G, swi, script, prod->p_baseM.b_tagM, buf);
		SWLIB_ASSERT(ret == 0);

		/*
		 * write some shell code that monitors
		 * return status of the check script
		 */

		if (swextopt_is_option_true(SW_E_enforce_scripts, opta)) {
			checkscript_retcode = SW_ERROR;
			swi_retcode = 1;
		} else {
			checkscript_retcode = SW_WARNING;
			swi_retcode = 0;
		}

		if (G->g_force) {
			check_def_retcode = 0;
		} else {
			check_def_retcode = SW_DESELECT;
		}

		*p_script = script;
		strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"case \"$sw_retval\" in\n"
		"	%d)\n"   /* SW_SUCCESS */
		"		sw_retval=0\n"
		"		;;\n"
				/* Do nothing */
		"	%d)\n"  /* SW_WARNING */
		"		sw_retval=0\n"
		"		%s\n"
		"		;;\n"

		"	%d)\n"  /* SW_ERROR */
		"		script_retval=%d\n"
		"		sw_retval=%d\n"
		"		%s\n"
		"		sw_retval=0\n"
		"		;;\n"
		"	%d)\n"  /* SW_DESELECT */
		"		script_retval=%d\n"
		"		sw_retval=%d\n"
		"		%s\n"
		"		sw_retval=0\n"
		"		;;\n"
		"	*)\n"
		"		sw_retval=4\n" /* Big error */
		"		;;\n"
		"esac\n"
		"dd of=/dev/null 2>/dev/null\n",

		SW_SUCCESS,
		SW_WARNING,
		TEVENT(2, (swi->verboseM), SW_CHECK_SCRIPT_WARNING, "status=$sw_retval"),
		SW_ERROR,
		checkscript_retcode,
		swi_retcode,
		TEVENT(2, (swi->verboseM), SW_CHECK_SCRIPT_ERROR, "status=$script_retval"),
		SW_DESELECT,
		SW_DESELECT,
		check_def_retcode,
		TEVENT(2, (swi->verboseM), SW_CHECK_SCRIPT_EXCLUDE, "status=$sw_retval")
		);	
	} else {
		*p_script = NULL;
		strob_sprintf(buf, STROB_DO_APPEND, "sw_retval=0\n");
	}
	return 0;
}
	
static
char *
construct_binsh_name(char * shell)
{
	if (strcmp(shell, SH_A_sh) == 0) {
		return SWBIS_PGM_BIN_SH;
	} else if (strcmp(shell, SH_A_bash) == 0) {
		return SWBIS_PGM_BIN_BASH;
	} else if (strcmp(shell, SH_A_ksh) == 0) {
		return SWBIS_PGM_BIN_KSH;
	} else if (strcmp(shell, SH_A_mksh) == 0) {
		return SWBIS_PGM_BIN_MKSH;
	} else {
		fprintf(stderr, "%s: invalid selection in construct_binsh_name(), defaulting to sh\n",
				swlib_utilname_get());
		return SWBIS_PGM_BIN_SH;
	};
}

void
swpl_write_chdir_catalog_fragment(STROB * tmp, char * catalog_path, char * id_str)
{
	strob_sprintf(tmp, STROB_DO_APPEND,
		CSHID
		"cd \"%s\"\n"
		"case \"$?\" in\n"
		"	0)\n"
		"	;;\n"
		"	*)\n"
		"	echo error: cd \"%s\" failed for task \"%s\" 1>&2\n"
		"	exit 1\n"
		"	;;\n"
		"esac\n"
		, catalog_path, catalog_path, id_str);
}

void
swpl_print_file_list_to_buf(SWI_FILELIST * fl, STROB * buf)
{
	char * name;
	int ix;
	ix = 0;
	while ((name=swi_fl_get_path(fl, ix++)) != NULL) {
		strob_sprintf(buf, STROB_DO_APPEND, "%s\n", name);
	}
}

void
swpl_set_detected_catalog_perms(SWI * swi, ETAR * etar, int tar_type)
{
	mode_t mode;
	if (swi->swi_pkgM->installed_catalog_ownerM)
		etar_set_uname(etar, swi->swi_pkgM->installed_catalog_ownerM);
	if (swi->swi_pkgM->installed_catalog_groupM)
		etar_set_gname(etar, swi->swi_pkgM->installed_catalog_groupM);
	mode = swi->swi_pkgM->installed_catalog_modeM;
	if (mode > 0) {
		if (tar_type == REGTYPE) {
			mode &= ~(S_IXUSR|S_IXGRP|S_IXOTH|S_ISVTX|S_ISUID|S_ISGID);
		}
		etar_set_mode_ul(etar, (unsigned int)(mode));
	}
}

int
swpl_send_null_task(SWICOL * swicol, int ofd, int event_fd, char * msgtag, int retcode)
{
	int ret;
	STROB * tmp;
	tmp = strob_open(10);
	strob_sprintf(tmp, 0, "sw_retval=%d", retcode);
	ret = swpl_send_null_task2(swicol, ofd, event_fd, msgtag, strob_str(tmp));
	strob_close(tmp);
	return ret;
}

int
swpl_send_null_task2(SWICOL * swicol, int ofd, int event_fd, char * msgtag, char * status_expression)
{
	int ret = 0;
	STROB * tmp;
	STROB * ts_msg;

	ts_msg = strob_open(10);
	tmp = strob_open(10);


	strob_strcat(ts_msg, msgtag);
	E_DEBUG("");
	/*
	 * Here is the minimal task scriptlet that throws an error
	 */
	strob_sprintf(tmp, 0,
		CSHID
		"# sw_retval=%%d\n"
		"%s\n"
		"sleep 0\n"
		"dd bs=512 count=1 of=/dev/null 2>/dev/null\n"
		"case $sw_retval in 0) exit 0 ;; *) exit $sw_retval;; esac\n"
		, status_expression);
	E_DEBUG("");

	/*
	 * Send the script into stdin of the POSIX shell
	 * invoked with the '-s' option.
	 */
	E_DEBUG("");
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd, 
		512,  
		".",
		strob_str(tmp), strob_str(ts_msg));
	E_DEBUG2("ret=%d", ret);

	if (ret == 0) {
		/*
		 * Now send the stdin payload which must be
		 * exactly stdin_file_size bytes long.
		 */
		
		/* 
		 * Send the payload
		 */
		ret = etar_write_trailer_blocks(NULL, ofd, 1);
		if (ret < 0) {
			fprintf(stderr, "%s: swpl_get_utsname_attributes(): etar_write_trailer_blocks(): ret=%d\n",
				swlib_utilname_get(), ret);
		}

		/*
		 * Reap the events from the event pipe.
		 */
		E_DEBUG2("ret=%d",ret);
		if (ret > 0)
			ret = swicol_rpsh_task_expect(swicol,
				event_fd, SWICOL_TL_12);

		E_DEBUG2("ret=%d",ret);
		/* swicol_show_events_to_fd(swicol, STDERR_FILENO, -1); */	
	}
	strob_close(ts_msg);
	strob_close(tmp);
	swicol_clear_task_idstring(swicol);
	E_DEBUG2("ret=%d",ret);
	return ret;
}

void
swpl_init_header_root(ETAR * etar)
{
	etar_set_typeflag(etar, REGTYPE);
	etar_set_mode_ul(etar, (unsigned int)(0640));
	etar_set_uid(etar, 0);
	etar_set_gid(etar, 0);
	etar_set_uname(etar, "root");
	etar_set_gname(etar, "root");
}

void
swpl_scary_init_script_array(void)
{
	int i;
	for (i=0; i<SCAR_ARRAY_LEN; i++) {
		g_script_array[i] = (SCAR*)NULL;
	}
}

SWI_CONTROL_SCRIPT *
swpl2_find_by_id(int id, CISFBA * bav, CISF_BASE ** bpp)
{
	int i;
	VPLOB * v;
	CISF_BASE * b;
	SWI_CONTROL_SCRIPT * sc;

	v = bav->base_arrayM;
	i = 0;
	while ((b=(CISF_BASE*)vplob_val(v, i++))) {
		E_DEBUG2("index=%d", i);
		E_DEBUG2("id = %d", id);
		sc = swi_xfile_get_control_script_by_id(b->ixfileM, id);
		if (sc) {
			E_DEBUG2("Found id = %d", id);
			if (bpp)
				*bpp = b;
			return sc;
		}
	}
	E_DEBUG("Returning NULL");
	if (bpp)
		*bpp = NULL;
	return NULL;
}

SCAR **
swpl_get_script_array(void)
{
	return g_script_array;
}

SCAR *
swpl_scary_create(char * tag, char * tagspec, SWI_CONTROL_SCRIPT * script)
{
	SCAR * scary;
	scary = (SCAR *)malloc(sizeof(SCAR));
	if (!scary) return scary;
	scary->tagM = strdup(tag);		/* e.g. postinstall or preinstall */
	scary->tagspecM = strdup(tagspec);      /* <product_tag> or <product_tag>.<fileset_tag> */
	scary->scriptM = script;                /* pointer to the script object */
	return scary;
}

void
swpl_scary_delete(SCAR * scary)
{
	free(scary->tagM);
	free(scary->tagspecM);
	free(scary);
}

SCAR *
swpl_scary_add(SCAR ** array, SCAR * script)
{
	int i;
	for (i=0; i<SCAR_ARRAY_LEN; i++) {
		if (array[i] == (SCAR*)NULL) {
			array[i] = script;
			return script;
		}
	}
	return NULL;
}

void
swpl2_cisf_base_array_add(CISF_PRODUCT * cisf, CISF_BASE * b)
{
	VPLOB * v = cisf->cbaM->base_arrayM;
	vplob_add(v, b);
}

CISF_BASE * 
swpl2_cisf_base_array_get(CISF_PRODUCT * cisf, int ix)
{
	CISF_BASE * b;
	VPLOB * v = cisf->cbaM->base_arrayM;
	b = (CISF_BASE*)vplob_val(v, ix);
	return b;
}

void
swpl_scary_show_array(SCAR ** array)
{
	int i;
	STROB * tmp;
	tmp = strob_open(18);
	for (i=0; i<SCAR_ARRAY_LEN; i++) {
		if (array[i] == (SCAR*)NULL) {
			break;
		} else {
			swlib_writef(STDERR_FILENO, tmp, "%d tag=[%s] tagspec=[%s] script=[%p]\n",
				i, array[i]->tagM,
				array[i]->tagspecM,
				array[i]->scriptM);
		}
	}
	strob_close(tmp);
	return;
}

SWI_CONTROL_SCRIPT *
swpl_scary_find_script(SCAR ** array, char * tag, char * tagspec)
{
	int i;
	STROB * tmp;
	SWI_CONTROL_SCRIPT * retval = NULL;

	tmp = strob_open(18);
	for (i=0; i<SCAR_ARRAY_LEN; i++) {
		if (array[i] == (SCAR*)NULL) {
			break;
		} else {
			if (
				strcmp(array[i]->tagM, tag) == 0 &&
				strcmp(array[i]->tagspecM, tagspec) == 0 &&
				1
			) {
				retval = array[i]->scriptM;
				break;
			}
		}
	}
	strob_close(tmp);
	return retval;
}

int
swpl_retrieve_files(GB * G, SWI * swi, SWICOL * swicol, SWICAT_E * e, SWI_FILELIST * file_list,
	int ofd, int ifd, int archive_fd, char * pax_write_command_key, int ls_fd, int ls_verbose, FILE_DIGS * digs)
{
	TARU * taru;
	STROB * scriptbuf;
	STROB * shell_lib_buf;
	int ret;

	scriptbuf = strob_open(300);
	shell_lib_buf = strob_open(300);

	E_DEBUG("");
	strob_sprintf(scriptbuf, STROB_DO_APPEND, "rm_retval=1\n");
	strob_sprintf(scriptbuf, STROB_DO_APPEND, "sw_retval=1\n");
			
	/* Every task script read stdin a block from even if it doesn't
           need data */	

	/* Add the requisite dd bs=512 count=1 of=/dev/null */
	E_DEBUG("");
	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"dd bs=512 count=1 of=/dev/null 2>/dev/null\n"
		"sw_retval=$?\n"
		);

	E_DEBUG("");

	swpl_make_verify_command_script_fragment(G, scriptbuf,
			file_list, pax_write_command_key, ls_fd < 0);

	E_DEBUG("");
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd,
		512,
		".",
		strob_str(scriptbuf),
		SWBIS_TS_retrieve_files_archive);

	if (ret != 0) {
		/* Set master_alarm ?? */
		swicol_set_master_alarm(swicol);
		return -1;
	}

	E_DEBUG("");
	/* Send the gratuitous data block because 'count=0' is busted for
	   some 'dd' programs */

	E_DEBUG("");
	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret <= 0) {
		return -1;
	}

	/* Now we have to read the archive */

	E_DEBUG("");
	taru = taru_create();
	E_DEBUG("");

	/*
 	 * Set the tar header policy to decode pax headers
 	 * This is required.
 	 */
	taru_set_tar_header_policy(taru, "pax", NULL);

	/* here, the archive is read.  This is a tar archive of the
	   package files as lifted from the file system */

	ret = taru_process_copy_out(taru, ifd, archive_fd,
			NULL, NULL, arf_ustar, ls_fd, ls_verbose, (intmax_t*)NULL, digs);

	E_DEBUG("");
	taru_delete(taru);
	if (ret < 0) {
		swicol_set_master_alarm(swicol);
	}

	E_DEBUG("");
	ret = swicol_rpsh_task_expect(swicol, G->g_swi_event_fd, SWICOL_TL_12);

	E_DEBUG("");
	strob_close(scriptbuf);
	strob_close(shell_lib_buf);
	E_DEBUG("");
	return ret;
}

void
swpl2_audit_cisf_bases(GB * G, SWI * swi, CISF_PRODUCT * cisf)
{
	int fileset_index;
	CISF_FILESET * f;

	E_DEBUG("entering");
	cplob_shallow_reset((CPLOB*)(cisf->cbaM->base_arrayM));
	E_DEBUG("add top level");
	swpl2_cisf_base_array_add(cisf, (void*)(&(cisf->cisf_baseM)));

	/*
	 * Loop over the filesets
	 */

	fileset_index = 0;
	f = (CISF_FILESET*)vplob_val(cisf->isetsM, fileset_index++);
	while(f) {
		E_DEBUG2("GOT f fileset_index=%d\n", fileset_index);
		E_DEBUG("add fileset\n");
		swpl2_cisf_base_array_add(cisf, (void*)(&(f->cisf_baseM)));
		f = (CISF_FILESET*)vplob_val(cisf->isetsM, fileset_index++);
	}
	E_DEBUG("leaving");
}

void
swpl_audit_execution_scripts(GB * G, SWI * swi, SCAR ** scary_scripts)
{
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	SWI_CONTROL_SCRIPT * product_xxinstall_script;
	SWI_CONTROL_SCRIPT * fileset_xxinstall_script;
	STROB * tagspec;
	SCAR * scary_script;
	int fileset_index;
	char ** ts;
	char * tag;
	char * execution_scripts[] = {
			SW_A_checkinstall,
			SW_A_postinstall,
			SW_A_preinstall,
			SW_A_configure,
			SW_A_checkremove,
			SW_A_preremove,
			SW_A_postremove,
			SW_A_unconfigure,
			SW_A_unpostinstall,
			SW_A_unpreinstall,
			SW_A_request,
			(char*)(NULL) };

	/* SWBIS_DEBUG_PRINT(); */
	tagspec = strob_open(32);
	swpl_scary_init_script_array();
	
	/*
	 * FIXME, allow more than one product
	 */

	product = swi_package_get_product(swi->swi_pkgM, 0 /* The first product */);
	ts = execution_scripts;
	for (;tag=(*ts),*ts != (char*)NULL; ts++) {	
		product_xxinstall_script = swi_product_get_control_script_by_tag(product, tag);
		if (product_xxinstall_script) {
			scary_script = swpl_scary_create(tag, product->p_baseM.b_tagM, product_xxinstall_script);
			swpl_scary_add(scary_scripts, scary_script);
		}
	}

	/*
	 * Loop over the filesets
	 */
	/* SWBIS_DEBUG_PRINT(); */

	fileset_index = 0;
	fileset = swi_product_get_fileset(product, fileset_index++);
	while(fileset) {	
		strob_sprintf(tagspec, STROB_NO_APPEND, "%s.%s", product->p_baseM.b_tagM, fileset->baseM.b_tagM);
		ts = execution_scripts;
		for (;tag=(*ts),*ts != (char*)NULL; ts++) {	
			fileset_xxinstall_script = swi_xfile_get_control_script_by_tag(fileset, tag);
			if (fileset_xxinstall_script) {
				scary_script = swpl_scary_create(tag, strob_str(tagspec), fileset_xxinstall_script);
				swpl_scary_add(scary_scripts, scary_script);
			}
		}
		fileset = swi_product_get_fileset(product, fileset_index++);
	}
	strob_close(tagspec);
}


int
swpl_write_case_block(SWI * swi, STROB * buf, char * tag)
{
	STROB * tmp;
	tmp = strob_open(100);

	swicat_write_script_cases(swi, tmp, tag);

	strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"# generated by swpl_write_case_block\n"
		"	%s)\n"
		"%s"
		"		;;\n",
		tag, strob_str(tmp));

	strob_close(tmp);
	return 0;
}

uintmax_t
swpl_get_whole_block_size(uintmax_t size)
{
	uintmax_t ret;
	if (size == 0) return 0;

	if (size % TARRECORDSIZE)
		ret = ((size / TARRECORDSIZE) + 1) * TARRECORDSIZE;
	else
		ret = ((size / TARRECORDSIZE)) * TARRECORDSIZE;
	return ret;
}

int 
swpl_write_session_options_file(STROB * buf, SWI * swi)
{
	strob_strcpy(buf, "");
	swextopt_writeExtendedOptions_strob(buf, (struct extendedOptions *)(swi->optaM), swi->swc_idM, 1);
	return strob_strlen(buf);
}

int
swpl_write_single_file_tar_archive(SWI * swi, int ofd, char * name, char * data, AHS * ahs)
{
	ETAR * etar;
	int ret;
	int retval = 0;

	etar = etar_open(swi->xformatM->taruM->taru_tarheaderflagsM);
	etar_init_hdr(etar);
	swpl_init_header_root(etar);
	etar_set_size(etar, strlen(data));
	if (etar_set_pathname(etar, name)) SWLIB_FATAL("name too long");
	swpl_set_detected_catalog_perms(swi, etar, REGTYPE);
	if (ahs) {
		etar_set_uname(etar, ahs_get_tar_username(ahs));
		etar_set_gname(etar, ahs_get_tar_groupname(ahs));
		etar_set_mode_ul(etar, (unsigned long)ahs_get_mode(ahs));
	}
	etar_set_chksum(etar);

	/*
	 * Emit the header 
	 */
	ret = etar_emit_header(etar, ofd);
	if (ret < 0) {
		fprintf(stderr, "%s: swpl_write_single_file_tar_archive(): etar_emit_header(): ret=%d\n",
			swlib_utilname_get(), ret);
		return ret;
	}
	retval += ret;

	/*
	 * Emit the file data
	 */
	ret = etar_emit_data_from_buffer(etar, ofd, data, strlen(data));
	if (ret < 0) {
		fprintf(stderr, "%s: swpl_write_single_file_tar_archive(): etar_emit_data_from_buffer(): ret=%d\n",
			swlib_utilname_get(), ret);
		return ret;
	}
	retval += ret;

	/*
	 * Emit the trailer blocks
	 */	
	ret = etar_write_trailer_blocks(NULL, ofd, 2);
	if (ret < 0) {
		fprintf(stderr, "%s: swpl_write_single_file_tar_archive(): etar_write_trailer_blocks(): ret=%d\n",
			swlib_utilname_get(), ret);
		return ret;
	}
	retval += ret;

	etar_close(etar);
	return retval;
}

int
swpl_load_single_status_value (GB * G, SWI * swi, int ofd, 
	int event_fd, char * id_str, int status_msg)
{
	int ret;
	STROB * tmp;
	STROB * data;
	
	ret = 0;
	tmp = strob_open(10);
	data = strob_open(10);

	/* make up one 512 byte block of nuls with
	   the formated integer at the beginning 
	   of the 512 block */

	strob_memset(data, '\0', 513);
	strob_sprintf(data, 0, "%d\n", status_msg);
	
	/* Here is the task scriptlet to read a value
	   and exit with its status */

	strob_sprintf(tmp, 0,
		"# Read the status from the the management host\n"
		"# and exit with its value.  This is used to \n"
		"# convey an error from the management host to the\n"
		"# target host\n"
		"MHOST_STATUS=$(dd bs=512 count=1);\n"
		"case \"$MHOST_STATUS\" in\n"
		"	0)\n"
		"		sw_retval=0\n"
		"		;;\n"
		"	*)\n"
		"		sw_retval=1\n"
		"		;;\n"
		"esac\n"
		);

	/*  Now send the task script */

	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 			/* file descriptor */
		1,			/* one 512 byte block */
		"/", 			/* path does not matter for this script */
		strob_str(tmp),		/* The actual task script */
		id_str			/* Id string */
		);

	if (ret == 0) {
		ret = atomicio((ssize_t (*)(int, void *, size_t))write,
			ofd, (void*)(strob_str(data)), 512);
		if (ret == 512)
			ret = 0;
		else
			ret = 2;
	}  else {
		ret = 1;
		/* internal error */
	}
	strob_close(data);
	strob_close(tmp);
	return ret;
}

int
swpl_load_single_file_tar_archive (GB * G,
	SWI * swi,
	int ofd, 
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str,
	char * name,
	char * data, AHS * ahs
	)
{
	int ret;
	unsigned long int stdin_file_size;
	STROB * tmp;
	STROB * namebuf;
	STROB * tmpv;

	tmp = strob_open(10);
	tmpv = strob_open(10);
	namebuf = strob_open(10);

	if (G->g_verboseG > SWC_VERBOSE_4) {
		strob_strcpy(tmpv, "");
	} else {
		strob_strcpy(tmpv, "1>/dev/null 2>&1");
	}

	/*
	 * Compute the data size that will be sent to the stdin of the 'sh -s'
	 * process on the target host.
	 */

	stdin_file_size = \
		swpl_get_whole_block_size((unsigned long int)strlen(data)) +  /* data plue tar block padding */
		TARRECORDSIZE + /* ustar header */
		TARRECORDSIZE + /* trailer block */
		TARRECORDSIZE + /* trailer block */
		0;
	/*
	 * Here is the task scriptlet to load the installed file.
	 */
	
	strob_sprintf(tmp, STROB_DO_APPEND,
			CSHID
		"cd \"%s\"\n"
		"case \"$?\" in\n"
		"	0)\n"
		"	;;\n"
		"	*)\n"
		"	echo error: cd \"%s\" failed for task \"%s\" 1>&2\n"
		"	exit 1\n"
		"	;;\n"
		"esac\n"
		, catalog_path, catalog_path, id_str);

	strob_sprintf(tmp, STROB_DO_APPEND,
			CSHID
		"dd 2>/dev/null | (%s %s; sw_retval=$?; dd of=/dev/null 2>/dev/null; exit $sw_retval) \n"
		"sw_retval=$?\n"	/* This line is required */
		"# dd of=/dev/null 2>/dev/null\n"
		,
		pax_read_command, strob_str(tmpv)
		);

	/*
	 * Now send the task script to the target host 'sh -s'
	 */

	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 			/* file descriptor */
		stdin_file_size,	/* script stdin file length */
		swi->swi_pkgM->target_pathM,
		strob_str(tmp),		/* The actual task script */
		id_str			/* Id string */
		);
	
	if (ret == 0) {
		/*
		 * Now send the data on the same stdin.
		 * In this case it is a tar archive with one file member
		 */

		ret = swpl_write_single_file_tar_archive(swi, ofd, name, data, ahs);
		if (ret <= 0) {
			SWBIS_ERROR_IMPL();
			return -1;
		}

		/*
		 * Assert what must be true.
		 */

		if (ret != (int)stdin_file_size) {
			fprintf(stderr, "ret=%d  stdin_file_size=%lu\n", ret, stdin_file_size);
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}
	
		/*
		 * Reap the events from the event pipe.
		 */
		ret = swicol_rpsh_task_expect(swi->swicolM,
					event_fd,
				SWICOL_TL_12 /*time limit*/);
		if (swi->debug_eventsM)
			swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, -1);

		if (ret < 0) {
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}
	}
	/*
	 * close and return
	 */
	strob_close(tmpv);
	strob_close(tmp);
	strob_close(namebuf);
	return ret;
}

int
swpl_write_tar_installed_software_index_file (GB * G, SWI * swi, int ofd,
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str, char *sw_a_installed, AHS * ahs
	)
{
	int ret = 0;
	STROB * name;
	STROB * data;

	name = strob_open(100);
	data = strob_open(100);

	swicat_isf_installed_software(data, swi);

	/*
	 * catalog_entryM = [var/lib/swbis/catalog/swbis/swbis/0.000/0]
	 */

	strob_strcpy(name, sw_a_installed);

	ret = swpl_load_single_file_tar_archive(G,
		swi,
		ofd, 
		catalog_path,
		pax_read_command,
		alt_catalog_root,
		event_fd,
		id_str,
		strob_str(name),
		strob_str(data), ahs);

	strob_close(name);
	strob_close(data);
	return ret;
}

int
swpl_construct_configure_script(GB * G, CISF_PRODUCT * cisf, STROB * buf, SWI * swi, int do_configure)
{
	int ret;
	E_DEBUG("");
	strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"	# Start of code generated by swpl_construct_configure_script \n"
		"		ssv_do_configure=%d\n"
		"		case \"$ssv_do_configure\" in\n"
		"		1)\n",
			do_configure);	

	ret = swpl_construct_script(G, cisf, buf, swi, SW_A_configure);

	strob_sprintf(buf, STROB_DO_APPEND,
		"		;;\n"
		"		esac\n"
		"	# End of code generated by swpl_construct_configure_script \n"
	);
	E_DEBUG("");
	return 0;
}

int
swpl2_construct_control_script(GB * G, CISF_BASE * cisf_base, STROB * buf, SWI * swi, char * script_tag, char * parent_tag)
{
	STROB * tmp;
	STROB * toap;
	SWI_CONTROL_SCRIPT * xx_script;
	char * target_path;
	struct extendedOptions * opta;
	int event_error_value;
	int event_warning_value;

	E_DEBUG("START");
	opta = swi->optaM;
	tmp = strob_open(20);
	toap = strob_open(20);

	select_error_codes(script_tag, &event_error_value, &event_warning_value);

	E_DEBUG2("script_tag=%s", script_tag);
	xx_script = swi_xfile_get_control_script_by_tag(cisf_base->ixfileM, script_tag);
	E_DEBUG("");

	if (
		!xx_script
	) {
		/*  no script */
		return 0;
	}
	E_DEBUG("");

	strob_sprintf(toap, STROB_DO_APPEND,
		CSHID
		"# Begin code generated by swproglib.c:swpl_construct_control_script\n");

	E_DEBUG("");
	if (swi->swi_pkgM->target_pathM) {
		target_path = swi->swi_pkgM->target_pathM;
	} else {
		target_path = ".";
	}

	E_DEBUG("");
	if (parent_tag && strlen(parent_tag)) {
		strob_sprintf(tmp, 0, "%s.%s", parent_tag, cisf_base->ixfileM->baseM.b_tagM);
	} else {
		strob_sprintf(tmp, 0, "%s", cisf_base->ixfileM->baseM.b_tagM);
	}
	strob_sprintf(toap, STROB_DO_APPEND, "\tcd \"%s\"\n", target_path);

	E_DEBUG("");
	construct_script_cases(G, toap, swi,
		xx_script,
		strob_str(tmp), script_tag,
		event_error_value,
		event_warning_value);

	E_DEBUG("");
	strob_sprintf(toap, STROB_DO_APPEND,
		"# End of code generated by swproglib.c:swpl_construct_control_script\n");

	strob_sprintf(buf, STROB_DO_APPEND, "%s", strob_str(toap));

	strob_close(toap);
	strob_close(tmp);
	return 0;
}

int
swpl_construct_script(GB * G, CISF_PRODUCT * cisf, STROB * buf, SWI * swi, char * script_tag)
{
	STROB * tmp;
	STROB * toap;
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	SWI_CONTROL_SCRIPT * product_xxinstall_script;
	SWI_CONTROL_SCRIPT * fileset_xxinstall_script;
	char * target_path;
	struct extendedOptions * opta;
	int product_ix;
	int fileset_ix;
	int event_error_value;
	int event_warning_value;

	E_DEBUG("START");
	opta = swi->optaM;
	tmp = strob_open(20);
	toap = strob_open(20);

	product_ix = cisf->cisf_baseM.cf_indexM;
	fileset_ix = ((CISF_FILESET*)vplob_val(cisf->isetsM, 0))->cisf_baseM.cf_indexM;

	/* Remove this check when multiple products/filesets become supported */
	if (fileset_ix != 0 || product_ix != 0) {
		fprintf(stderr, "***  ERROR in cisf object\n");	
		product_ix = fileset_ix = 0;
	}

	/*
	 * script_tag is either preinstall, postinstall, configure, or
	 * postremove, preremove
	 */
	select_error_codes(script_tag, &event_error_value, &event_warning_value);

	product = swi_package_get_product(swi->swi_pkgM, product_ix /* The first product */);
	product_xxinstall_script = swi_product_get_control_script_by_tag(product, script_tag);

	fileset = swi_product_get_fileset(product, fileset_ix /* the first one */);
	fileset_xxinstall_script = swi_xfile_get_control_script_by_tag(fileset, script_tag);

	if (
		!product_xxinstall_script &&
		!fileset_xxinstall_script
	) {
		/*
		 * no script
		 */
		/* strob_sprintf(toap, STROB_DO_APPEND, "sw_retval=0\n"); */
		return 0;
	}

	/*
	 * FIXME -- need to set the state to "transient"
	 */

	/*
	 * write code to run the product {pre|post}install script
	 */
	if (swi->swi_pkgM->target_pathM) {
		target_path = swi->swi_pkgM->target_pathM;
	} else {
		target_path = ".";
	}
	
	if (product_xxinstall_script) {
		/*
		 * construct the product script's execution
		 */
		strob_sprintf(toap, STROB_DO_APPEND, 
			CSHID
			"# Start of code generated by swproglib.c:swpl_construct_script\n",
			"cd \"%s\"\n",
			target_path);
		construct_script_cases(G, toap, swi,
			product_xxinstall_script,
			product->p_baseM.b_tagM, script_tag,
			event_error_value,
			event_warning_value);
	}
	
	E_DEBUG("");
	if (fileset_xxinstall_script) {
		strob_sprintf(toap, STROB_DO_APPEND,
			CSHID
		"	case \"$sw_retval\" in  # Case_construct_script_003\n"
		"	0)\n"
		);
		strob_sprintf(tmp, 0, "%s.%s", product->p_baseM.b_tagM, fileset->baseM.b_tagM);

		strob_sprintf(toap, STROB_DO_APPEND, "\tcd \"%s\"\n", target_path);
		construct_script_cases(G, toap, swi,
			fileset_xxinstall_script,
			strob_str(tmp), script_tag,
			event_error_value,
			event_warning_value);
		strob_sprintf(toap, STROB_DO_APPEND,
		"	;;\n"
		"	esac # Case_construct_script_003\n"
		);
	}

	E_DEBUG("");
	strob_sprintf(toap, STROB_DO_APPEND,
		"# End of code generated by swproglib.c:swpl_construct_script\n");

	strob_sprintf(buf, STROB_DO_APPEND, "%s", strob_str(toap));

	E_DEBUG("");
	strob_close(toap);
	strob_close(tmp);
	return 0;
}

int
swpl_construct_analysis_script(GB * G, char * script_name, STROB * buf, SWI * swi, SWI_CONTROL_SCRIPT ** p_script)
{
	return i_construct_analysis_script(G, script_name,  buf, swi, p_script);
}

int
swpl_compare_name(char * name1, char * name2, char * att, char * filename)
{
	int ret;
	if (strlen(name1) == 0 || strlen(name2) == 0) return 0;
	ret = swlib_dir_compare(name1, name2, 1);
	if (ret) {
		if (swlib_compare_8859(name1, name2) != 0) {
			fprintf(stderr, "%s: attribute mismatch: %s: att=%s: storage=[%s] INFO=[%s]\n",
				swlib_utilname_get(), filename, att,
				name1, name2);
		} else {
			ret = 0;
		}
	}
	return ret ? 1 : 0;
}

void
swpl_safe_check_pathname(char * s)
{
	if (swlib_is_sh_tainted_string(s)) {
		SWLIB_FATAL("tainted string");
	}
}

void
swpl_sanitize_pathname(char * s)
{
	swlib_squash_all_leading_slash(s);
}

char * 
swpl_get_attribute(SWHEADER * header, char * att, int * len)
{
	char * line;
	char * value;
        line = swheader_get_attribute(header, att, NULL);
	value = swheaderline_get_value(line, len);
	return value;
}

void
swpl_enforce_one_prod_one_fileset(SWI * swi)
{
	if (
		(swi->swi_pkgM->swi_coM[0]) == NULL || 
		(swi->swi_pkgM->swi_coM[0]->swi_coM[0]) == NULL
	) 
	{
		/*
		 * No product or no fileset
		 */
		fprintf(stderr, "%s: currently,  only one (1) products/filesets.\n",
			swlib_utilname_get());
		SWI_internal_fatal_error();
	}
	
	if (
		swi->swi_pkgM->swi_coM[1] ||
		swi->swi_pkgM->swi_coM[0]->swi_coM[1]
	) {
		/*
		 * More than one product or fileset
		 */
		fprintf(stderr, "%s: currently, multiple products/filesets not yet supported\n",
			swlib_utilname_get());
		SWI_internal_fatal_error();
	}	
	return;
}

int
swpl_does_have_prod_postinstall(SWI * swi)
{
	int ret;
	SWI_PRODUCT * prod;
	prod = swi_package_get_product(swi->swi_pkgM, 0 /* The first product */);
	ret = swi_product_has_control_file(prod, SW_A_postinstall);
	return ret;
}

int
swpl_get_fileset_file_count(SWHEADER * infoheader)
{
	int count = 0;
	char * next_line;
	swheader_reset(infoheader);
	next_line = swheader_get_next_object(infoheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
	while (next_line){
		count++;
		next_line = swheader_get_next_object(infoheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
	}
	return count++;
}

int
swpl_write_out_signature_member(SWI * swi, struct tar_header * ptar_hdr,
		struct new_cpio_header * file_hdr,
		int ofd, int signum,
		int *package_ret, char * installer_sig)
{
	int ifd;
	XFORMAT * xformat = swi->xformatM;
	int retval = 0;
	int ret;
	unsigned long filesize;
	unsigned long package_filesize;
	char * sig;
	int siglen;
	STROB * tmp = strob_open(16);
	ETAR * etar;
	
	ifd = xformat_get_ifd(xformat);
	etar = etar_open(swi->xformatM->taruM->taru_tarheaderflagsM);
	etar_init_hdr(etar);

	/*
	 * Set the basic tar header fields
	 */
	swpl_init_header_root(etar);

	/*
	 * This reads the signature tar header from the package.
	 */
	ret = uxfio_read(ifd, (void*)(ptar_hdr), TARRECORDSIZE);
	if (ret != TARRECORDSIZE) {
		SWI_internal_fatal_error();
	}
	
	/* If this is the installer sig then seek backwards since there
	   really is not a archive member in the package for this signature */
	 
	if (installer_sig && strlen(installer_sig)) {
		ret = uxfio_lseek(ifd, -TARRECORDSIZE, SEEK_CUR);
		if (ret < 0) {
			SWI_internal_fatal_error();
		}
	}

	/*
	 * Determine the filesize of the signature file
	 * It damn well should be 512 or 1024
	 */
	taru_otoul(ptar_hdr->size, &filesize);
	package_filesize = filesize + TARRECORDSIZE;
	*package_ret = package_filesize;

	/*
	 * Do a sanity check on the size.
	 */
	if (filesize != 512 && filesize != 1024) {
		fprintf(stderr, "filesize=%d\n", (int)filesize);
		SWI_internal_fatal_error();
	}

	/*
	 * allocate some temporary memory for this
	 */
	sig = malloc((size_t)filesize);
	if (!sig) SWI_internal_fatal_error();

	if (installer_sig && strlen(installer_sig)) {
		memset(sig, (int)'\n', (size_t)filesize);
		memcpy(sig, installer_sig, strlen(installer_sig));
	} else {
		/*
		 * This reads the sigfile archive member data
		 */	
		ret = uxfio_read(ifd, (void*)(sig), (size_t)filesize);
		if ((int)ret != (int)filesize) {
			SWI_internal_fatal_error();
		}
	}
	
	sig[filesize-1] = '\0';
	siglen = strlen(sig);
	/*
	 * The package contains a sigfile padded with NULs.
	 * We will only install the ascii bytes which means the
	 * the sig file in the installed catalog will be shorter in
	 * length than in the package.
	 */

	etar_set_size(etar, siglen);
	
	/*
	 * set the mode of the catalog top level directory
	 *    was:  etar_set_mode_ul(etar, (unsigned int)(0640));
	 */
	swpl_set_detected_catalog_perms(swi, etar, REGTYPE);

	strob_strcpy(tmp, SWINSTALL_INCAT_NAME);
	swlib_unix_dircat(tmp, SWINSTALL_CATALOG_TAR);
	swlib_squash_all_leading_slash(strob_str(tmp));  /* FIXME */
	if (signum <= 1) {
		strob_sprintf(tmp, 1, ".sig");
	} else {
		strob_sprintf(tmp, 1, ".sig%d", signum);
	}

	if (etar_set_pathname(etar, strob_str(tmp))) SWLIB_FATAL("name too long");
	etar_set_chksum(etar);

	ret = uxfio_unix_safe_write(ofd, (void*)(etar_get_hdr(etar)), TARRECORDSIZE);
	if (ret != TARRECORDSIZE) {
		SWI_internal_fatal_error();
	}
	retval += ret;

	if (siglen <= 512)
		filesize = 512;
	else if (siglen <= 1024)
		filesize = 1024;
	else
		SWI_internal_fatal_error();
	
	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
			ofd, (void*)(sig), filesize);
	if (ret != (int)filesize) {
		fprintf(stderr, "%s: swpl_write_out_signature_member(): ret=%d filesize=%d\n",
			swlib_utilname_get(), ret, (int)filesize);
		if (ret < 0)
			fprintf(stderr, "%s: swpl_write_out_signature_member(): %s\n",
				swlib_utilname_get(), strerror(errno));
		SWI_internal_fatal_error();
	}
	retval += ret;

	etar_close(etar);
	strob_close(tmp);
	free(sig);
	return retval;
}

int
swpl_write_out_all_signatures(SWI * swi, struct tar_header * ptar_hdr,
		struct new_cpio_header * file_hdr,
		int ofd, int startoffset,
		int endoffset, unsigned long filesize)
{
	int curpos;
	int ifd;
	int ret = 0;
	int sig_number;
	int current = 0;
	int package_current = 0;
	int package_ret = 0;
	int sig_block_length = endoffset - startoffset;
	XFORMAT * xformat = swi->xformatM;
	
	ifd = xformat_get_ifd(xformat);
	curpos = uxfio_lseek(ifd, 0L, SEEK_CUR);
	if (curpos < 0) {
		SWI_internal_fatal_error();
	}

	if (sig_block_length < 1024) {
		/*
		* FIXME: tar format specific.
		* Sanity check.
		*/
		return -1;
	}

	/*
	 * Seek to the beginning of the signature archive members.
	 */

	if (uxfio_lseek(ifd, startoffset, SEEK_SET) < 0) {
		SWI_internal_fatal_error();
	}

	/*
	 * all the sigfiles are the same length, filesize.
	 * sig_number is the number of signatures present, this
	 * needs to be determined because it is decremented to
	 * '1'.  This means the last signature in the package
	 * will have the name 'catalog.tar.sig', the next to last
	 * will have the name 'catalog.tar.sig2', and so on.
	 */

	sig_number = sig_block_length / (int)(filesize + TARRECORDSIZE);

	/* fprintf(stderr, "eraseme filesize=%d sig_number=%d\n", (int)filesize, sig_number); */

	/*
	 * do a sanity check
	 */

	if (sig_block_length % (int)(filesize + TARRECORDSIZE)) {
		SWI_internal_fatal_error();
	}

	if (swi->swi_pkgM->installer_sigM && sig_number >= 1) {
		sig_number++;
		ret = swpl_write_out_signature_member(swi, ptar_hdr,
				file_hdr, ofd,
				sig_number--, &package_ret,
				swi->swi_pkgM->installer_sigM);
		if (ret < 0) {
			return ret;
		}
		current += ret;
	}

	/*
	 * Loop to write out all the signatures.
	 */

	while (package_current < sig_block_length) {
		if (sig_number <= 0) {
			/* Sanity check */
			SWI_internal_fatal_error();
		}
		ret = swpl_write_out_signature_member(swi, ptar_hdr,
				file_hdr, ofd,
				sig_number--, &package_ret, NULL);
		if (ret < 0) {
			return ret;
		}
		package_current += package_ret;
		current += ret;
	}

	/*
	 * Now restore the old offset.
	 */
	if (uxfio_lseek(ifd, curpos, SEEK_SET) < 0) {
		SWI_internal_fatal_error();
	}
	return current;
}

int
swpl_write_catalog_data(SWI * swi, int ofd, int sig_block_start, int sig_block_end)
{
	int curpos;
	int ifd;
	int iend;
	int amount;
	XFORMAT * xformat = swi->xformatM;
	SWI_PACKAGE * package = swi->swi_pkgM;
	int retval;
	int ret;

	E_DEBUG("START");
	ifd = xformat_get_ifd(xformat);
	curpos = uxfio_lseek(ifd, 0L, SEEK_CUR);

	E_DEBUG2("current position = [%d]", curpos);

	if (curpos < 0) {
		SWI_internal_fatal_error();
		return -1;
	}

	E_DEBUG2("ifd=%d", ifd);
	if (uxfio_lseek(ifd, package->catalog_start_offsetM, SEEK_SET) < 0) {
		SWI_internal_fatal_error();
		return -1;
	}

	E_DEBUG("");
	if (sig_block_start > 0) {
		iend = sig_block_start;
	} else {
		iend = package->catalog_end_offsetM;
	}
	amount = iend - package->catalog_start_offsetM;

	E_DEBUG("");
	E_DEBUG2("amount before signature [%d]", amount);
	if (swlib_pump_amount(ofd, ifd, amount) != amount) {
		SWI_internal_fatal_error();
		return -1;
	}
	retval = amount;
	E_DEBUG("");
	if (sig_block_start > 0) {
		/* 
		 * skip the signatures and write the remaining.
		 */
		amount = sig_block_end - sig_block_start;
		if (uxfio_lseek(ifd, amount, SEEK_CUR) < 0) {
			SWBIS_ERROR_IMPL();
			SWI_internal_fatal_error();
		}
		E_DEBUG2("amount = sig_block_end - sig_block_start  [%d]", amount);
		amount = package->catalog_end_offsetM - sig_block_end;
		if (swlib_pump_amount(ofd, ifd, amount) != amount) {
			SWBIS_ERROR_IMPL();
			SWI_internal_fatal_error();
			return -1;
		}
		E_DEBUG2("remaining after signature [%d]", amount);
		retval += amount;
	} else {
		/*
		 * done
		 */
		;
	}
	if (uxfio_lseek(ifd, curpos, SEEK_SET) < 0) {
		SWBIS_ERROR_IMPL();
		SWI_internal_fatal_error();
	}

	E_DEBUG("");
	/*
	 * Now write 2 NUL trailer blocks
	 */
	E_DEBUG2("retval before trailers [%d]", retval);
	ret = etar_write_trailer_blocks(NULL, ofd, 2);
	if (ret < 0) {
		fprintf(stderr, "%s: swpl_write_catalog_data(): etar_write_trailer_blocks(): ret=%d\n",
			swlib_utilname_get(), ret);
		return ret;
	}
	retval += ret;
	E_DEBUG("");
	return retval;
} 

int
swpl_send_abort(SWICOL * swicol, int ofd, int event_fd, char * msgtag)
{
	int ret = 0;
	ret = swpl_send_null_task(swicol, ofd, event_fd, msgtag, SW_ERROR);
	return ret;
}

int
swpl_report_status(SWICOL * swicol, int ofd, int event_fd)
{
	int ret = 0;
	ret = swpl_send_null_task2(swicol, ofd, event_fd, SWBIS_TS_report_status, "sw_retval=$rp_status");
	return ret;
}

int
swpl_send_success(SWICOL * swicol, int ofd, int event_fd, char * msgtag)
{
	int ret = 0;
	ret = swpl_send_null_task(swicol, ofd, event_fd, msgtag, SW_SUCCESS);
	return ret;
}

int
swpl_send_nothing_and_wait(SWICOL * swicol, int ofd, int event_fd, char * msgtag, int tl, int retcode)
{
	int ret = 0;
	STROB * tmp;

	E_DEBUG("START");
	tmp = strob_open(10);

	/*
	 * Here is the minimal task scriptlet
	 */
	strob_sprintf(tmp, 0,
		"sw_retval=%d\n"
		"dd of=/dev/null 2>/dev/null\n",
		retcode
		);

	/*
	 * Send the script into stdin of the POSIX shell
	 * invoked with the '-s' option.
	 */
	E_DEBUG("");
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd, 
		512,  
		".", /* does not matter */
		strob_str(tmp), msgtag
		);

	E_DEBUG("");
	if (ret == 0) {
		/*
		 * Now send the stdin payload which must be
		 * exactly stdin_file_size bytes long.
		 */
	
		/* 
		 * Send the payload
		 */
		E_DEBUG("");
		ret = etar_write_trailer_blocks(NULL, ofd, 1);
		if (ret < 0) {
			fprintf(stderr, "%s: send_nothing_and_wait(): etar_write_trailer_blocks(): ret=%d\n",
				swlib_utilname_get(), ret);
		}
		/*
		 * Reap the events from the event pipe.
		 */
		E_DEBUG("");
		ret = swicol_rpsh_task_expect(swicol,
				event_fd,
				tl /*time limit*/);
		if (ret < 0) {
			SWLIB_INTERNAL("");
		}
	}
	strob_close(tmp);
	E_DEBUG("");
	return ret;
}

int
swpl_send_signature_files(SWI * swi, int ofd,
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd,
	struct tar_header * ptar_hdr,
	int sig_block_start, int sig_block_end,
	unsigned long filesize)
{
	int ret = 0;
	int padamount;
	int stdin_file_size;
	STROB * tmp;
	int sig_block_length;
	struct new_cpio_header * file_hdr;
	struct tar_header * tar_hdr;

	E_DEBUG("START");
	tar_hdr = (struct tar_header *)malloc(TARRECORDSIZE+1);
	tmp = strob_open(10);
	file_hdr = taru_make_header();

	memcpy((void*)tar_hdr, (void*)ptar_hdr, TARRECORDSIZE);
	sig_block_length = sig_block_end - sig_block_start;

	stdin_file_size = sig_block_length +
			TARRECORDSIZE +  /* Trailer block */
			TARRECORDSIZE;   /* Trailer block */

	if (swi->swi_pkgM->installer_sigM && strlen(swi->swi_pkgM->installer_sigM)) {
		stdin_file_size += (filesize + TARRECORDSIZE);
	}

	/*
	 * Here is the task scriptlet.
	 */
	strob_sprintf(tmp, STROB_DO_APPEND,
		CSHID
		"cd \"%s\"\n"
		"case \"$?\" in\n"
		"	0)\n"
		"	;;\n"
		"	*)\n"
		"	echo error: cd \"%s\" failed in routine \"%s\" 1>&2\n"
		"	exit 1\n"
		"	;;\n"
		"esac\n"
		, catalog_path, catalog_path, "__send_signature_files");

	strob_sprintf(tmp, STROB_DO_APPEND,
		CSHID
		"dd 2>/dev/null | %s\n"
		"sw_retval=$?\n"
		"dd of=/dev/null 2>/dev/null\n"
		,
		pax_read_command
		);

	/*
	 * Send the script into stdin of the POSIX shell
	 * invoked with the '-s' option.
	 */
	/* SWBIS_TS_Load_signatures */
	/* swicol_set_task_idstring(swi->swicolM, SWBIS_TS_Load_signatures); */
	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 
		stdin_file_size,
		swi->swi_pkgM->target_pathM,
		strob_str(tmp), SWBIS_TS_Load_signatures
		);

	if (ret == 0) {
		/*
		 * Now send the stdin payload which must be
		 * exactly stdin_file_size bytes long.
		 */
	
		ret = swpl_write_out_all_signatures(swi, tar_hdr, file_hdr,
			ofd, sig_block_start, sig_block_end, filesize);

		if (ret < 0) {
			SWBIS_ERROR_IMPL();
			return -1;
		}

		if (ret > (stdin_file_size - TARRECORDSIZE - TARRECORDSIZE)) {
			/*
			* Sanity check
			*/
			SWBIS_ERROR_IMPL();
			return -1;
		}

		/*
		 * Now pad the remaining output which will be at least
		 * 2 * 512 bytes long.
		 */
		padamount = stdin_file_size - ret;

		ret = swlib_pad_amount(ofd, padamount);
		if (ret < 0 || ret != padamount) {
			SWBIS_ERROR_IMPL();
			return -1;
		}

		/*
		 * Reap the events from the event pipe.
		 */
		ret = swicol_rpsh_task_expect(swi->swicolM,
				event_fd,
				SWICOL_TL_12 /*time limit*/);
		if (swi->debug_eventsM)
			swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, -1);
	}
	taru_free_header(file_hdr);
	strob_close(tmp);
	free(tar_hdr);
	return 0; /* ret; */
}

int
swpl_common_catalog_tarfile_operation(SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd,
	char * script, char * id_str)
{
	int ret = 0;
	STROB * script_buf;

	E_DEBUG("START");
	script_buf = strob_open(10);

	/*
	 * add a chdir operation to the front of this script
	 * to chdir into the catalog diretory
	 */

	strob_sprintf(script_buf, STROB_DO_APPEND,
			CSHID
		"cd \"%s\"\n"
		"case \"$?\" in\n"
		"	0)\n"
		"	;;\n"
		"	*)\n"
		"	echo error: cd \"%s\" failed for task \"%s\" 1>&2\n"
		"	exit 1\n"
		"	;;\n"
		"esac\n"
		, catalog_path, catalog_path, id_str);

	/*
	 * Now append the caller's script
	 */

	strob_sprintf(script_buf, STROB_DO_APPEND, "\n%s\n", script);

	/*
	 * Now send the script.
	 */

	/* swicol_set_task_idstring(swi->swicolM, id_str); */
	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 
		512,  /* Some dd's can't read zero bytes */
		swi->swi_pkgM->target_pathM,
		strob_str(script_buf),
		id_str);
		
	if (ret == 0) {
		/* 
		 * Now Send the payload 
		 * which in this case is just a gratuitous 512 bytes.
		 * This is needed because some dd(1) on some Un*xes can't
		 * read zero blocks.
		 */
		ret = etar_write_trailer_blocks(NULL, ofd, 1);
		if (ret < 0) {
			fprintf(stderr, "%s: swpl_common_catalog_tarfile_operation(): etar_write_trailer_blocks(): ret=%d\n",
				swlib_utilname_get(), ret);
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}

		/*
		 * Now reap the events from the event pipe.
		 */
		ret = swicol_rpsh_task_expect(swi->swicolM,
				event_fd,
				SWICOL_TL_12 /*time limit*/);

		if (swi->debug_eventsM)
			swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, -1);
	}
	strob_close(script_buf);
	return ret;
}

void
swpl_update_state_by_cisf_base(CISF_BASE * cfb, char * state)
{
	time(&(cfb->ixfileM->baseM.mod_timeM));
	swi_xfile_set_state(cfb->ixfileM, state);
}

void
swpl_update_fileset_state(SWI * swi, char * swsel, char * state)
{

	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	int fileset_index = 0;

	/*
	 * FIXME, supprt software selections
	 */
	product = swi_package_get_product(swi->swi_pkgM, 0 /* The first product */);
	fileset = swi_product_get_fileset(product, fileset_index++);
	while(fileset) {	
		swi_xfile_set_state(fileset, state);
		fileset = swi_product_get_fileset(product, fileset_index++);
	}
}
		
void
swpl2_update_fileset_state(CISF_BASE * cisf_base, SWI_CONTROL_SCRIPT * script, int status)
{
	SWI_XFILE * xfile;
	char * script_tag;

	if (cisf_base->typeidM == CISF_ID_PRODUCT) {
		/* a product does not have a state attribute */
		return;
	}

	xfile = cisf_base->ixfileM;
	script_tag = script->baseM.b_tagM;

	if (strcmp(script_tag, SW_A_configure) == 0) {
		if (status == 0)
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_CONFIGURED);
		else
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_CORRUPT);

	} else if (strcmp(script_tag, SW_A_postinstall) == 0) {
		if (status == 0)
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_INSTALLED);
		else
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_CORRUPT);

	} else if (
		strcmp(script_tag, SW_A_preinstall) == 0 ||
		strcmp(script_tag, SW_A_preremove) == 0
	) {
		if (status == 0)
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_TRANSIENT);
		else
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_CORRUPT);
	} else if (strcmp(script_tag, SW_A_unconfigure) == 0) {
		if (status == 0)
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_INSTALLED);
		else
			swpl_update_state_by_cisf_base(cisf_base, SW_STATE_CORRUPT);
	} else {
		fprintf(stderr, "bad script tag in swpl2_update_fileset_state\n");
	}
}

int
swpl2_update_execution_script_results(SWI * swi, SWICOL * swicol, CISF_PRODUCT * cisf)
{
	char * ununtag;
	STROB * buf;
	STROB * tmp;
	SWI_CONTROL_SCRIPT * script;
	SWI_CONTROL_SCRIPT * ununscript;
	CISF_BASE * cisf_base;
	int event_index = -1;
	int event_start_index;
	int status;
	int script_id;
	int result;
	char * message;
	char * id_msg;
	char * s;
	int num_processed;
	
	E_DEBUG("START");
	num_processed = 0;
	buf = strob_open(100);
	tmp = strob_open(100);

	E_DEBUG("");
	event_start_index = 0;

	/*
	 * loop over the events
	 */
	message = swicol_rpsh_get_event_message(swicol, SW_CONTROL_SCRIPT_BEGINS,
				event_start_index, &event_index);
	event_index++;
	while (message) {
		/* fprintf(stderr, "message=[%s]\n", message); */
		E_DEBUG2("message=%s", message);

		/* 
		 * Find the script_id from the SWI_MSG event
		 */
		id_msg = swicol_rpsh_get_event_message(swicol, SWI_MSG, event_index, &event_index);
		SWLIB_ASSERT(id_msg != NULL);
		s = strstr(id_msg, SW_A_SCRIPT_ID "=");
		SWLIB_ASSERT(s != NULL);
		s += strlen(SW_A_SCRIPT_ID "=");
		script_id = swlib_atoi(s, &result);
		SWLIB_ASSERT(result == 0);
	
		/* 
		 * Now find the SWI_CONTROL_SCRIPT object that belongs to this script_id
		 */
		E_DEBUG2("Looking for script id [%d]\n", script_id);
		script = swpl2_find_by_id(script_id, cisf->cbaM, &cisf_base);
		SWLIB_ASSERT(script != NULL);
		E_DEBUG2("Found script id [%d]\n", script_id);

		status = swicol_rpsh_get_event_status(swicol, (char*)(NULL),
                		SW_CONTROL_SCRIPT_ENDS, event_index, &event_index);
		SWLIB_ASSERT(status >= 0);

		/* Now, if the script is unconfigure we need to set the result of the
		   configure script to UNSET */

		/* So find the configure script */

		if (cisf_base && script) {
			if (strcmp(script->baseM.b_tagM, SW_A_unconfigure) == 0) {
				ununtag =  SW_A_configure;
			} else if (strcmp(script->baseM.b_tagM, SW_A_unpostinstall) == 0) {
				ununtag =  SW_A_postinstall;
			} else {
				ununtag = NULL;
			}
			if (ununtag) {
				ununscript = swi_xfile_get_control_script_by_tag(cisf_base->ixfileM, ununtag);
				if (ununscript) {
					ununscript->resultM = SWI_RESULT_UNDEFINED;
				}
			}
		}
		script->resultM = status;
		num_processed++;

		swpl2_update_fileset_state(cisf_base, script, status);

		E_DEBUG2("script result=%d", status);
		message = swicol_rpsh_get_event_message(swicol,
				SW_CONTROL_SCRIPT_BEGINS, event_index+1, &event_index);
	}
	strob_close(tmp);
	strob_close(buf);
	return num_processed;
}

/* If the filesets have no script, but the product does, refer the
   fileset state from the product script results */
int
swpl2_normalize_configure_script_results(SWI * swi, char * script_name, CISF_PRODUCT * cisf)
{
	int i;
	int product_result;
	CISF_BASE * base;
	SWI_CONTROL_SCRIPT * product_script;
	SWI_CONTROL_SCRIPT * script;
	SWI_XFILE * xfile;

	E_DEBUG("ENTERING");
	i = 0;
	base = swpl2_cisf_base_array_get(cisf, i++);
	if (!base || base->typeidM != CISF_ID_PRODUCT) {
		/* error */
		E_DEBUG("error 1");
		return 1;
	}

	xfile = base->ixfileM; /* actually PFILES */

	/* script_name should only be 'configure' or 'unconfigure' */

	product_script = swi_xfile_get_control_script_by_tag(xfile, script_name);

	if (!product_script) {
		/* This is OK, if there is no product script then
		   there is no need to refer the result into the fileset */
		E_DEBUG("returning 0");
		return 0;	 
	}
	
	product_result = product_script->resultM;	
	E_DEBUG2("product_result=%d", product_result);

	/* i equals 1 here, this is the first fileset */
	while((base=swpl2_cisf_base_array_get(cisf, i)) != NULL) {
		E_DEBUG("GOT fileset");
		if (base->typeidM == CISF_ID_PRODUCT) {
			/* error, should not happen */
			E_DEBUG("error 2");
			return 2;	
		} else if (base->typeidM == CISF_ID_FILESET) {
			xfile = base->ixfileM;
			if (!xfile) {
				E_DEBUG("error 4");
				return 4;
			}
			script = swi_xfile_get_control_script_by_tag(xfile, script_name);
			if (script == NULL) {
				E_DEBUG2("updating fileset result to product_result=%d", product_result);
				swpl2_update_fileset_state(base, product_script, product_result);
			} else {
				/* do not refer the product result to this fileset */
				;
			}
		} else {
			/* error, should not happen */
			E_DEBUG("error 3");
			return 3;	
		}
		i++;
	}
	E_DEBUG2("Leaving i=%d", i);
	return 0;
}

int
swpl_update_execution_script_results(SWI * swi, SWICOL * swicol, SCAR ** array)
{
	STROB * buf;
	STROB * tmp;
	SWI_CONTROL_SCRIPT * script;
	int event_index = -1;
	int event_start_index;
	int status;
	char * savechp;
	char * tag;
	char * tagspec;
	char * message;
	char * id_msg;
	
	E_DEBUG("START");
	buf = strob_open(100);
	tmp = strob_open(100);

	E_DEBUG("");
	/* event_start_index = swicol->event_indexM; */
	event_start_index = 0;

	E_DEBUG2("event_start_index=%d", event_start_index);

	/*
	 * loop over the events
	 */
	message = swicol_rpsh_get_event_message(swicol, SW_CONTROL_SCRIPT_BEGINS,
				event_start_index, &event_index);
	event_index++;
	while (message) {
		/* fprintf(stderr, "message=[%s]\n", message);
			*/
		E_DEBUG2("message=%s", message);
		tagspec = message;
		tag = strchr(message, (int)(' '));
		SWLIB_ASSERT(tag != NULL);
		savechp = tag;
		*tag = '\0';
		tag++;
		/* fprintf(stderr, "tag=[%s] tagspec=[%s]\n", tag, tagspec);
			*/
		/*
		 * Now find the script that belongs to this tag and tagspec
		 */
		script = swpl_scary_find_script(array, tag, tagspec);
		E_DEBUG2("script tag=%s", tag);
		E_DEBUG2("script tagspec=%s", tagspec);
		SWLIB_ASSERT(script != NULL);

		id_msg = swicol_rpsh_get_event_message(swicol, SWI_MSG, event_index, &event_index);

		status = swicol_rpsh_get_event_status(swicol, (char*)(NULL),
                		SW_CONTROL_SCRIPT_ENDS, event_index, &event_index);
		SWLIB_ASSERT(status >= 0);
		script->resultM = status;
		E_DEBUG2("script result=%d", status);
		message = swicol_rpsh_get_event_message(swicol,
				SW_CONTROL_SCRIPT_BEGINS,
				event_index+1,
				&event_index);
		*savechp = (int)(' ');		
	}

	strob_close(tmp);
	strob_close(buf);
	return 0;
}

int
swpl_unpack_catalog_tarfile(GB * G, SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd)
{
	int ret = 0;
	STROB * tmp;
	STROB * tmpv;

	E_DEBUG("START");
	tmp = strob_open(10);
	tmpv = strob_open(10);

	if (G->g_verboseG >= SWC_VERBOSE_6) {
		strob_strcpy(tmpv, "");
	} else {
		strob_sprintf(tmpv, 0, "1>/dev/null");
	}

	strob_sprintf(tmp, 0,
		CSHID
		"#set -vx\n"
		"# echo unpacking catalog %s 1>&2\n"
		"dd of=/dev/null 2>/dev/null\n"
		"# pwd 1>&2\n"
		"cd  " SWINSTALL_INCAT_NAME  " || exit 1\n"
		"%s %s <catalog.tar %s\n"
		"sw_retval=$?\n"
		,
		swi->exported_catalog_prefixM,
		pax_read_command,
		swi->exported_catalog_prefixM,
		strob_str(tmpv)
		);

	E_DEBUG("");
	ret = swpl_common_catalog_tarfile_operation(swi, ofd, catalog_path,
				pax_read_command, alt_catalog_root,
				event_fd, strob_str(tmp),
				SWBIS_TS_Catalog_unpack);
	E_DEBUG("");
	strob_close(tmp);
	strob_close(tmpv);
	return ret;
}

/** swpl_remove_catalog_directory - remove the unpacked catalog from catalog
 *
 */

int
swpl_remove_catalog_directory(SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd)
{
	int ret = 0;
	STROB * tmp;
	STROB * path;
	STROB * tmptok;
	STROB * rmdir_command;
	char * token;
	char * start;

	E_DEBUG("START");
	path = strob_open(10);
	tmp = strob_open(10);
	tmptok = strob_open(10);
	rmdir_command = strob_open(10);

	/*
	 * perform some sanity checks on swi->exported_catalog_prefixM
	 * It should have the force of <path>/catalog
	 */

	strob_strcpy(path, swi->exported_catalog_prefixM);
	if (swlib_check_clean_relative_path(strob_str(path))) return 1;
	swlib_squash_trailing_slash(strob_str(path));
	if (strstr(strob_str(path), SW_A_catalog) == NULL) return 2;

	/*
	 * OK , swi->exported_catalog_prefixM is now qualified as sane
	 */

	/*
	 * swi->exported_catalog_prefixM is typically :  <path>/catalog
	 * therefore we must execute:
	 *     rmdir <path>
	 *  If there is more than one leading path component
	 *  then they must be rmdir'ed individually
	 *  
	 *  If the exported catalog path is 
	 *      aaaa/bbbb/cccc/catalog    then
	 *  form a commmand string to remove aaaa/bbbb/cccc
         *  using rmdir since it is safer to user than rm -fr
	 *  The command will look like this:
	 *     rmdir aaaa/bbbb/cccc && rmdir aaaa/bbbb && rmdir aaaa
	 */

	strob_strcpy(rmdir_command, "");
	if (strchr(strob_str(path), '/')) {
		start = strob_str(path);
		token = strrchr(start, '/');	
		while(token) {
			*token = '\0';
			strob_sprintf(rmdir_command, 1, " && rmdir \"%s\"", start); 
			token = strrchr(start, '/');	
		}
	}

	strob_sprintf(tmp, 0,
		CSHID
		"cd " SWINSTALL_INCAT_NAME " || exit 1\n"
		"/bin/rm -fr \"%s\" %s\n"
		"sw_retval=$?\n"
		"#echo rmdir command is [\"%s\"] 1>&2\n"
		"#echo rm command is [\"%s\"] 1>&2\n"
		"dd of=/dev/null 2>/dev/null\n"
		,
		swi->exported_catalog_prefixM,
		strob_str(rmdir_command),
		strob_str(rmdir_command),
		swi->exported_catalog_prefixM
		);


	ret = swpl_common_catalog_tarfile_operation(swi, ofd, catalog_path,
				pax_read_command, alt_catalog_root,
				event_fd, strob_str(tmp),
				SWBIS_TS_Catalog_dir_remove);
	strob_close(tmp);
	strob_close(path);
	strob_close(rmdir_command);
	strob_close(tmptok);
	return ret;
}

int
swpl_get_utsname_attributes(GB * G, SWICOL * swicol, SWUTS * uts, int ofd, int event_fd)
{
	int ret;
	STROB * tmp = strob_open(10);
	STROB * tmp1 = strob_open(10);

	E_DEBUG("START");
	strob_sprintf(tmp, 0,
			CSHID
			"sw_retval=0\n"
			"%s\n" /* shls_config_guess() */
			"%s\n" /* shls_run_config_guess() */
			"%s\n"
			"%s\n"
			"%s\n"
			"%s\n"
			"%s\n"
			"dd of=/dev/null 2>/dev/null\n"
			"exit $sw_retval\n"
			"%s"		/* nil line */
			,
			shlib_get_function_text_by_name("shls_config_guess", tmp1, NULL),
			shlib_get_function_text_by_name("shls_run_config_guess", tmp1, NULL),
			TEVENT(2, 1, SWI_ATTRIBUTE, SW_A_machine_type "=\"$(uname -m)\""),
			TEVENT(2, 1, SWI_ATTRIBUTE, SW_A_os_name      "=\"$(uname -s)\""),
			TEVENT(2, 1, SWI_ATTRIBUTE, SW_A_os_release   "=\"$(uname -r)\""),
			TEVENT(2, 1, SWI_ATTRIBUTE, SW_A_os_version   "=\"$(uname -v)\""),
			TEVENT(2, 1, SWI_ATTRIBUTE, SW_A_architecture "=\"$(shls_run_config_guess)\""),
			"");

	/* TS_uts_query */
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd, 
		512,
		".",
		strob_str(tmp), SWBIS_TS_uts
		);

	if (ret == 0) {
		ret = etar_write_trailer_blocks(NULL, ofd, 1);
		if (ret < 0) {
			fprintf(stderr, "%s: swpl_get_utsname_attributes(): etar_write_trailer_blocks(): ret=%d\n",
				swlib_utilname_get(), ret);
		}

		ret = swicol_rpsh_task_expect(swicol,
			event_fd,
			SWICOL_TL_100 /*timelimit*/);
			/* 100 seconds needed for cygwin hosts */
	
		/*
		 * print the events to memory
		 */
		swicol_print_events(swicol, tmp, swicol->event_indexM);

		/*
		 * Parse and Store the uts attributes
		 */
		swuts_read_from_events(/* swi->target_version_idM->swutsM */ uts, strob_str(tmp));

		E_DEBUG2("utsname=[%s]", strob_str(tmp));

		if (ret != 0) {
			/*
			 * Examine the events in swicol->evemt_listM to determine
			 * what action to take
			 */
			if (ret == 2) {
				swlib_doif_writef(G->g_verboseG,  1,
					(struct sw_logspec *)(NULL), STDERR_FILENO,
					"SW_RESOURCE_ERROR on target host:  %d second time limit expired\n", 5);
			}
			ret = -1;
		}
	}
	strob_close(tmp);
	strob_close(tmp1);
	return ret;
}

int
swpl_get_catalog_perms(GB * G, SWI * swi, int ofd, int event_fd)
{
	int ret;
	SWICOL * swicol;
	STROB * tmp;
	
	E_DEBUG("START");
	swicol = swi->swicolM;
	tmp = strob_open(10);

	strob_sprintf(tmp, 0,
			CSHID
			"sw_retval=0\n"
			"ISC=\"%s\"\n"
			"LSO=`tar chf - \"$ISC\" 2>/dev/null | tar tvf - 2>/dev/null | head -1`\n"
			"%s\n"
			"dd of=/dev/null 2>/dev/null\n"
			"exit $sw_retval\n"
			"%s"		/* nil line */
			,
			get_opta_isc(G->optaM, SW_E_installed_software_catalog),
			TEVENT(2, 1, SWI_ATTRIBUTE, "ls_ld=\"$LSO\""),
			"");

	/* TS_get_catalog_perms */
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd, 
		512,
		swi->swi_pkgM->target_pathM,
		strob_str(tmp), SWBIS_TS_get_catalog_perms
		);

	if (ret == 0) {
		ret = etar_write_trailer_blocks(NULL, ofd, 1);
		if (ret < 0) {
			fprintf(stderr, "%s: get_catalog_perms(): etar_write_trailer_blocks(): ret=%d\n",
				swlib_utilname_get(), ret);
		}

		ret = swicol_rpsh_task_expect(swicol,
			event_fd,
			SWICOL_TL_100 /*timelimit*/);
			/* 100 seconds needed for cygwin hosts */
		
		if (ret != 0) {
			/* FIXME, write error message */
			return -1;
		}
	
		/*
		 * print the events to memory
		 */
		swicol_print_events(swicol, tmp, swicol->event_indexM);
		/* fprintf(stderr, "%s\n", strob_str(tmp)); */

		/*
		 * Parse and Store the ls -ld output into swi
		 */

		ret = parse_ls_ld_output(swi, strob_str(tmp));
		if (ret != 0) {
			swi->swi_pkgM->installed_catalog_ownerM = strdup("root");
			swi->swi_pkgM->installed_catalog_groupM = strdup("root");
			swi->swi_pkgM->installed_catalog_modeM = 0755;
			fprintf(stderr, "%s: warning: (swpl_get_catalog_perms) parsing (retrieval) of tar listing failed: ret=%d\n",
				swlib_utilname_get(), ret);
			fprintf(stderr, "%s: warning: root/root (0755) will be used for catalog ownerships\n", swlib_utilname_get());
			ret = 0;
		}
	} else {
		ret = -3;
	}
	strob_close(tmp);
	return ret;
}

int
swpl_determine_tar_listing_verbose_level(SWI * swi)
{
	int taru_ls_verbose_level;

	E_DEBUG("START");
	if (swi->swc_idM == SWC_U_I /* swinstall */) {
		if (swi->verboseM >= SWC_VERBOSE_3) {
			E_DEBUG("");
			taru_ls_verbose_level = LS_LIST_VERBOSE_L1;
		} else {
			E_DEBUG("");
			taru_ls_verbose_level = LS_LIST_VERBOSE_L0;
		}
	} else if (swi->swc_idM == SWC_U_L /* swlist */) {
		if (swi->verboseM >= SWC_VERBOSE_2) {
			E_DEBUG("");
			taru_ls_verbose_level = LS_LIST_VERBOSE_L1;
		} else {
			E_DEBUG("");
			taru_ls_verbose_level = LS_LIST_VERBOSE_L0;
		}
	} else {
		E_DEBUG("");
		taru_ls_verbose_level = LS_LIST_VERBOSE_L0;
	}
	E_DEBUG2("[%d]", taru_ls_verbose_level);
	return taru_ls_verbose_level;
}

int
swpl_assert_all_file_definitions_installed(SWI * swi, SWHEADER * infoheader)
{
	int ret;
	char * next_line;
	char * keyword;
	char * missing_name;
	
	E_DEBUG("START");
	ret = 0;
	swheader_store_state(infoheader, NULL);
	swheader_reset(infoheader);
	while((next_line=swheader_get_next_object(infoheader, (int)UCHAR_MAX, (int)UCHAR_MAX))) {
		keyword = swheaderline_get_keyword(next_line);
		SWLIB_ASSERT(keyword != NULL);
		if (strcmp(keyword, SW_A_control_file) == 0) {
			continue;
		}
		if (swheaderline_get_flag1(next_line) == 0) {
			missing_name = swpl_get_attribute(infoheader, SW_A_path, NULL);
			swlib_doif_writef(swi->verboseM, SWC_VERBOSE_1,
				(struct sw_logspec *)(NULL), STDERR_FILENO,
				"warning: missing from storage section: %s\n", missing_name);
			ret++;
		}
	}
	swheader_restore_state(infoheader, NULL);
	return ret;
}

VPLOB *
swpl_get_same_revision_specs(GB * G, SWI * swi, int product_number, char * location)
{
	char * revision;
	char * tag;
	VPLOB * swspecs;
	SWI_PRODUCT * product;
	SWHEADER * global_index;
	SWVERID * swverid;
	STROB * tmp;

	E_DEBUG("START");
	tmp = strob_open(10);
	swspecs = vplob_open(); 

	if (location && strlen(location) == 0) location = NULL;

	product = swi_package_get_product(swi->swi_pkgM, product_number/* The first product */);
	SWLIB_ASSERT(product != NULL);
	
	/* Get the global INDEX access header object */
	global_index = swi_get_global_index_header(swi);
	swheader_store_state(global_index, NULL);
	swheader_reset(global_index);

	swheader_set_current_offset(global_index, product->p_baseM.header_indexM);

	tag = swheader_get_single_attribute_value(global_index, SW_A_tag);
	SWLIB_ASSERT(tag != NULL);

	revision = swheader_get_single_attribute_value(global_index, SW_A_revision);
	SWLIB_ASSERT(revision != NULL);
	
	/* First, LT */
	strob_sprintf(tmp, STROB_NO_APPEND, "%s,r<%s", tag, revision);
	if (location)
		strob_sprintf(tmp, STROB_DO_APPEND, ",l=%s", location);
	swverid = swverid_open(NULL, strob_str(tmp));
	vplob_add(swspecs, swverid);	

	/* Second,  EQ */
	strob_sprintf(tmp, STROB_NO_APPEND, "%s,r==%s", tag, revision);
	if (location)
		strob_sprintf(tmp, STROB_DO_APPEND, ",l=%s", location);
	swverid = swverid_open(NULL, strob_str(tmp));
	vplob_add(swspecs, swverid);	

	/* Third, GT */
	strob_sprintf(tmp, STROB_NO_APPEND, "%s,r>%s", tag, revision);
	if (location)
		strob_sprintf(tmp, STROB_DO_APPEND, ",l=%s", location);
	swverid = swverid_open(NULL, strob_str(tmp));
	vplob_add(swspecs, swverid);	

	/* Fouth, EQ but any location */
	strob_sprintf(tmp, STROB_NO_APPEND, "%s,r==%s", tag, revision);
	swverid = swverid_open(NULL, strob_str(tmp));
	vplob_add(swspecs, swverid);	

	swheader_restore_state(global_index, NULL);
	strob_close(tmp);
	return swspecs;
}

VPLOB *
swpl_get_dependency_specs(GB * G, SWI * swi, char * requisite_keyword, int product_number, int fileset_number)
{
	char * depspec;
	char * value;
	VPLOB * swspecs;
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	SWHEADER * global_index;
	SWVERID * swverid;
	STROB * tmp;
	STROB * tmp2;
	char ** xlist;
	char ** list;
	
	E_DEBUG("START");
	tmp = strob_open(10);
	tmp2 = strob_open(10);

	/* Get the dependency specs from the fileset per spec */

	swspecs = vplob_open(); /* List object that will contain a
				   list of SWVERID objects */

	product = swi_package_get_product(swi->swi_pkgM, product_number/* The first product */);
	SWLIB_ASSERT(product != NULL);

	fileset = swi_product_get_fileset(product, fileset_number);
	SWLIB_ASSERT(fileset != NULL);

	/* Now get the dependencies which are stored in the 'prerequisites' attrbute */

	/* Get the global INDEX access header object */
	global_index = swi_get_global_index_header(swi);
	swheader_store_state(global_index, NULL);
	swheader_reset(global_index);

	/* Now set the offset of the relevant part of the global INDEX file */
	swheader_set_current_offset(global_index, fileset->baseM.header_indexM);

	xlist = swheader_get_attribute_list(global_index, requisite_keyword, (int*)NULL);
	list = xlist;	
	while (*list) {
		value = swheaderline_get_value(*list, NULL);
		/* Now token-ize this as space delimeted */
		
		/* The data is in escaped internal form, hence we must
		   (de)expand the "\\n" (for example) sequences to turn them
		   into the real char (a '\n' for example */
		swlib_expand_escapes((char **)(NULL), NULL, value, tmp);

		depspec = strob_strtok(tmp, strob_str(tmp), " \n\r\t"); /* FIXME supported quoted spaces ??? */
		while(depspec) {
			swverid = swverid_open(NULL, depspec);
			if (swverid == NULL) {
				sw_e_msg(G, "error processing software spec: %s\n", depspec);
			} else {
				SWVERID * next;
				next = swverid;
				vplob_add(swspecs, next);	
				while((next=swverid_get_alternate(next))) {
					vplob_add(swspecs, next);	
				}
				swverid_disconnect_alternates(swverid);
				/*
				strob_strcpy(tmp2, "");
				swverid_show_object_debug(swverid, tmp2, "");
				fprintf(stderr, "%s\n", strob_str(tmp2));
				*/
			}
			depspec = strob_strtok(tmp, NULL, " \n\r\t");
		}
		list++;
	}

	free(xlist);
	strob_close(tmp);
	strob_close(tmp2);
	swheader_restore_state(global_index, NULL);
	return swspecs;
}

int
swpl_get_catalog_entries(GB * G,
			VPLOB * swspecs,
			VPLOB * pre_swspecs,
			VPLOB * co_swspecs,
			VPLOB * ex_swspecs,
			char * target_path,
			SWICOL * swicol,
			int target_fd0,
			int target_fd1,
			struct extendedOptions * opta,
			char * pgm_mode)
{
	STROB * isc_script_buf;
	int aa_fd;
	int ret;
	int retval;
	
	E_DEBUG("START");
	retval = 0;
	isc_script_buf = strob_open(10);

	/* This section writes the product_tag revision vendor_tag
	   to  stdout */

	/* write the task script into a buffer */
	if (swpl_test_pgm_mode(pgm_mode, SWLIST_PMODE_PROD) == 0) {
		swicat_write_isc_script(isc_script_buf, G, swspecs, NULL, NULL, NULL, SWICAT_FORM_P1);
	} else if (swpl_test_pgm_mode(pgm_mode, SWLIST_PMODE_DIR) == 0) {
		swicat_write_isc_script(isc_script_buf, G, swspecs, NULL, NULL, NULL, SWICAT_FORM_DIR1);
	} else if (swpl_test_pgm_mode(pgm_mode, SWLIST_PMODE_DEP1) == 0) {
		swicat_write_isc_script(isc_script_buf, G, NULL, pre_swspecs, NULL, ex_swspecs, SWICAT_FORM_DEP1);
	} else {
		sw_e_msg(G, "bad internal mode: %s\n", pgm_mode);
		swicat_write_isc_script(isc_script_buf, G, NULL, pre_swspecs, NULL, ex_swspecs, SWICAT_FORM_DEP1);
	}
	
	/* Now send the SWBIS_TS_Get_iscs_listing task script */
	/* swicol_set_task_idstring(swicol, SWBIS_TS_Get_iscs_listing); */


	swicol->needs_synct_eoaM = 1;

	ret = swicol_rpsh_task_send_script2(swicol,
		target_fd1,			/* file descriptor of the task shell's stdin */
		512,				/* data size, here just a gratuitous block of NULs */
		target_path,			/* Directory to run in */
		strob_str(isc_script_buf),	/* The actual script */
		SWBIS_TS_Get_iscs_listing);
			
	swicol->needs_synct_eoaM = 0;
	if (ret != 0) {
		E_DEBUG("");
		fprintf(stderr, "swicol_rpsh_task_send_script2 error\n");
		retval = -1;
		return -1;
	}

	/* Send 1 block to satisfy the task shell, all task shells
	   get atleast 1 block because some dd()'s can't read zero (0)
	   block */
	E_DEBUG("");
	ret = etar_write_trailer_blocks(NULL, target_fd1, 1);
	if (ret < 0) {
		fprintf(stderr, "etar_write_trailer_blocks error\n");
		retval = -2;
		return retval;
	}

	/* Open a mem file just to collect the output and
	   run a sanity check */
	aa_fd = swlib_open_memfd();	

	/* Now read the output from the target process */
	E_DEBUG("");
	ret = swlib_synct_suck(aa_fd, target_fd0);
	E_DEBUG("");

	if (ret < 0) {
		/* This can happen for good reasons such as 'no such file'
		   as well as internal error bad reasons */
		/* fprintf(stderr, "swlib_synct_suck error\n"); */
		E_DEBUG("");
		retval = -3;
		return retval;
	}

	/* Now reap the task shell events */
	ret = swicol_rpsh_task_expect(swicol, G->g_swi_event_fd, SWICOL_TL_30);
	if (ret != 0) {
		if (ret < 0) {
			SWLIB_INTERNAL("");
		}
		retval = -4;
		sw_e_msg(G, "swicol_rpsh_task_expect failed: status=%d\n", ret);
		return retval;
	}
	E_DEBUG("");
	uxfio_lseek(aa_fd, 0, SEEK_SET);
	swicat_squash_null_bytes(aa_fd);
	uxfio_lseek(aa_fd, 0, SEEK_SET);

	retval = aa_fd;
	strob_close(isc_script_buf);
	return retval;
}

int
swpl_do_list_catalog_entries2(GB * G,
			VPLOB * swspecs,
			VPLOB * pre_swspecs,
			VPLOB * co_swspecs,
			VPLOB * ex_swspecs,
			char * target_path,
			SWICOL * swicol,
			int target_fd0,
			int target_fd1,
			struct extendedOptions * opta,
			char * pgm_mode)
{
	int retval;
	int memfd;
	char * memtext;

	E_DEBUG("START");
	retval = 0;
	memfd = swpl_get_catalog_entries(G,
			swspecs,
			pre_swspecs,
			co_swspecs,
			ex_swspecs,
			target_path,
			swicol,
			target_fd0,
			target_fd1,
			opta,
			pgm_mode);

	if (memfd < 0) return memfd;
	uxfio_lseek(memfd, 0, SEEK_SET);

	/* Now write the response from the target to local stdout */
	if (
		swpl_test_pgm_mode(pgm_mode, SWLIST_PMODE_PROD) == 0 ||
		swpl_test_pgm_mode(pgm_mode, SWLIST_PMODE_DIR) == 0 ||
		G->g_verboseG >= SWC_VERBOSE_3 ||
		0
	) {
		swlib_pipe_pump(STDOUT_FILENO, memfd);
	}

	if (swpl_test_pgm_mode(pgm_mode, SWLIST_PMODE_DEP1) == 0) {
		SWICAT_REQ  * req;
		int ret;
		/* Analyze the dependencies */
	
		req = swicat_req_create();
		/* Now get the NUL terminated text image */
		memtext = uxfio_get_fd_mem(memfd, (int *)NULL);

		ret = swicat_req_analyze(G, req, memtext, NULL);
		if (ret < 0 || ret > 0) {
			sw_e_msg(G, "internal error analyzing dependencies, ret=%d\n", ret);
		} else {
			;
			/* no error, dependencies successfully
			   queried independent of query result */
		}
		if (
			swicat_req_get_pre_result(req) != 0 ||
			swicat_req_get_ex_result(req) != 0 ||
			0
		) {
			/* Failed dependency */
			retval = 1;
			swicat_req_print(G, req);
		}
		swicat_req_delete(req);
	}
	uxfio_close(memfd);
	return retval;
}

int
swpl_test_pgm_mode(char * pgm_mode, char * test_mode)
{
	if (strcmp(test_mode, pgm_mode) == 0) {
		return 0;
	} else {
		return 1;
	}
}

int
swpl_get_catalog_tar(GB * G,
	SWI * swi,
	char * target_path,
	VPLOB * upgrade_specs,
	int target_fd0,
	int target_fd1)
{
	char * path;
	struct stat st;
	int oflags;
	SWVARFS * swvarfs;
	STROB * isc_script_buf;
	struct extendedOptions * opta;
	int ret;
	int cfd;

	E_DEBUG("");
	opta = swi->optaM;

	isc_script_buf = strob_open(100);

	/* write a tar archive of the selection that will be upgraded because a 
	   removal of the fileset file and catalog entry is required */

	/* Note: the following function is used in the 'swlist -c -' operation,
	   we reuse it here,  this will get the tarblob catalog entries of all
	   the specs in the list (VPLOB *) upgrade_specs  */

	E_DEBUG("");
	swicat_write_isc_script(isc_script_buf, G, upgrade_specs,
				NULL, NULL, NULL, SWICAT_FORM_TAR1);

	/* swicol_set_task_idstring(swi->swicolM, SWBIS_TS_Get_iscs_entry); */

	E_DEBUG("");
	ret = swicol_rpsh_task_send_script2(swi->swicolM,
		target_fd1,
		512,          
		target_path,
		strob_str(isc_script_buf),   
		SWBIS_TS_Get_iscs_entry);

	E_DEBUG("");
	if (ret != 0) {
		E_DEBUG("");
		swicol_set_master_alarm(swi->swicolM);
		/* sw_e_msg(G, "swicol: fatal: %s:%d ret=%d\n", __FILE__, __LINE__, ret); */
	}

	if (ret == 0) {
		E_DEBUG("");
		ret = etar_write_trailer_blocks(NULL, target_fd1, 1);
	} else {
		E_DEBUG("");
		ret = -1;
	}

	E_DEBUG("");
	cfd = swlib_open_memfd();
	if (swicol_get_master_alarm_status(swi->swicolM) == 0 && ret == 512) {
		TARU * taru = taru_create();
		E_DEBUG("");
		ret = taru_process_copy_out(taru, target_fd0, cfd,
					NULL, NULL, arf_ustar, -1, -1, (intmax_t*)NULL, NULL);
		taru_delete(taru);
		if (ret < 0) {
			return -1;
		}
	} else {
		E_DEBUG("");
		return -2;
	}
	E_DEBUG("");
	ret = swicol_rpsh_task_expect(swi->swicolM, G->g_swi_event_fd, SWICOL_TL_30);
	if (uxfio_lseek(cfd, (off_t)0, SEEK_SET) < 0) {
		E_DEBUG("");
		return -3;
	}

	E_DEBUG("");
	if (uxfio_lseek(cfd, (off_t)0, SEEK_SET) < 0) {
		E_DEBUG("");
		return -4;
	}
	/*
	swlib_tee_to_file("/tmp/_catalog1.tar", cfd, NULL, -1, 0);
	*/

	E_DEBUG("");
	if (uxfio_lseek(cfd, (off_t)0, SEEK_SET) < 0) {
		return -5;
	}
	oflags = 0;

	E_DEBUG("");
	swvarfs = swvarfs_opendup(cfd, oflags, (mode_t)0);
	while ((path=swvarfs_get_next_dirent(swvarfs, &st)) != NULL && strlen(path)) {
		E_DEBUG("");
		if (G->devel_verboseM)
			fprintf(stderr, "<debug>: path=[%s]\n", path);
	}

	E_DEBUG("");
	swvarfs_dirent_reset(swvarfs);
	E_DEBUG("");

	while ((path=swvarfs_get_next_dirent(swvarfs, &st)) != NULL && strlen(path)) {
		if (G->devel_verboseM)
			fprintf(stderr, "<debug> path=[%s]\n", path);
	}
	E_DEBUG("");
	swvarfs_close(swvarfs);
	/* uxfio_close(cfd); */
	strob_close(isc_script_buf);
	E_DEBUG2("returning %d", cfd);
	return cfd;
}

void
swpl_tty_raw_ctl(GB * G, int c)
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

char *
swpl_get_samepackage_query_response(GB * G, SWICOL * swicol,
			char * target_path,
			int target_fd0,
			int target_fd1,
			struct extendedOptions * opta,
			VPLOB * packagespecs,
			int * p_retval,
			int make_dummy_response)
{
	int uxfio_fd;
	char * mem;

	E_DEBUG("START");
	E_DEBUG("calling swpl_get_catalog_entries");
	uxfio_fd = swpl_get_catalog_entries(G, (VPLOB*)NULL,
				packagespecs,
				(VPLOB*)NULL,
				(VPLOB *)NULL,
				target_path,
				swicol,
				target_fd0,
				target_fd1,
				opta,
				SWLIST_PMODE_DEP1);

	if (uxfio_fd < 0 ) {
		E_DEBUG("");
		return NULL;
	} else if (uxfio_fd < 0) {
		/* error, abort the target script */
		E_DEBUG("");
		abort_script(swicol, G, target_fd1);
		(*p_retval)++;
		return NULL;
	}
	/* The sync'ing dd on the target host injects blocks of NULs,
	   these must be squashed */
	E_DEBUG("");
	swicat_squash_null_bytes(uxfio_fd);

	/* Obtain a new image of the file */
	E_DEBUG("");
	uxfio_lseek(uxfio_fd, 0, SEEK_SET);
	mem = swi_com_new_fd_mem(uxfio_fd, (int*)NULL);

	E_DEBUG("");
	if (0 && G->devel_verboseM && 0)
		fprintf(stderr, "[[%s]]\n", mem);

	uxfio_close(uxfio_fd);
	/* FIXME, possible memory leak */
	E_DEBUG("END");
	return mem;
}

SWICAT_REQ *
swpl_analyze_samepackage_query_response(GB * G, char * response_image, SWICAT_SL ** p_sl)
{
	int ret;
	SWICAT_REQ  * req;
	SWICAT_SL * sl;

	E_DEBUG("START");
	if (response_image == NULL) return NULL;
	req = swicat_req_create();

	/* analyze the image and make the SWICAT_SL object */

	ret = swicat_req_analyze(G, req, response_image, p_sl);
	sl = *p_sl;
	if (ret < 0 || ret > 0) {
		sw_e_msg(G, "internal error from swicat_req_analyze, ret=%d\n", ret);
		return NULL;
	} else {
		;
	}
	return req;
}

char *
swpl_shellfrag_session_lock(STROB * buf, int vlv)
{
	
	/*
	 * implicit external variable interface
	 *
	 * Read Only:
	 *   LOCKPATH
	 *   LOCKENTRY
	 *   swexec_status   {0|1}
	 *   opt_allow_no_lock   {""|yes}
	 *   sw_retval
	 *   sh_dash_s
         *
	 * Write: 
	 *
	 *
	 */

	strob_sprintf(buf, 0,
	CSHID
	"#\n"
	"#  <<<<----- here's the code to lock a session \n"
	"#\n"
	"lock_did_lock=\"\"\n"
	"shls_bashin2 \"" SWBIS_TS_make_locked_session "\"\n"
	"sw_retval=$?\n"
	" # sw_retval:\n"
	" #   0: OK, proceed by invoking bash -s\n"
	" #   1: Not OK, either commanded abort or internal error\n"
	"lock_status=0\n"
	"case $sw_retval in\n"
	"	0) \n"
	"	# Ok to make test for lock\n"
	"	$sh_dash_s\n"
	"	sw_retval=$?\n"
	"	lock_status=$sw_retval\n"
	"	;;\n"
	"	# 2)  Dead Code, we never get here\n"
	"	# sw_retval=1\n"
	"	# opt_allow_no_lock=\"\"\n"
	"	# lock_status=2\n"
	"	# ;;\n"
	"	*)\n"
	"	sw_retval=1\n"
	"	opt_allow_no_lock=\"\"\n"
	"	lock_status=" SWBIS_STATUS_COMMAND_NOT_FOUND "\n"
	"	;;\n"
	"esac\n"
	"swexec_status=$sw_retval\n"
	"case \"$lock_status\" in\n"
	"0)\n"
	"       # Got Lock\n"
	"	lock_did_lock=true\n"
	"	case \"$opt_to_stdout\" in\n"
	"	True)\n"
	"		lock_did_lock=\"\"\n"
	"		;;\n"
	"	*)\n"
	"		%s\n"  /* SW_SOC_LOCK_CREATED */
	"		;;\n"
	"	esac\n"
	"	;;\n"
	"2) # Read-Only Access \n"
	"	case \"$opt_allow_no_lock\" in	\"\") ;; *) lock_status=0 ;; esac\n"
	"	case \"$opt_allow_no_lock\" in\n"
	"	*)\n"
	"		%s\n" /* SW_SOC_IS_READ_ONLY */
	"		%s\n" /* SW_SOC_LOCK_FAILURE */
	"		sw_retval=0\n"
	"		swexec_status=$sw_retval\n"
	"		;;\n"
	"	esac\n"
	"	;;\n"
	"1) # Lock failed \n"
	"	case \"$opt_allow_no_lock\" in\n"
	"	\"\")\n"
	"		%s\n" /* SW_CONFLICTING_SESSION_IN_PROGRESS */
	"		%s\n" /* SW_SOC_LOCK_FAILURE */
	"		;;\n"
	"	*)\n"
	"		%s\n" /* SW_CONFLICTING_SESSION_IN_PROGRESS */
	"		%s\n" /* SW_SOC_LOCK_FAILURE */
	"		lock_status=0\n"
	"		sw_retval=0\n"
	"		swexec_status=$sw_retval\n"
	"		;;\n"
	"	esac\n"
	"	;;\n"
	SWBIS_STATUS_COMMAND_NOT_FOUND ")\n"
	"	# do nothing \n"
	"	;;\n"
	"*)\n"
	"	%s\n" /* SW_SOC_LOCK_FAILURE */
	"	;;\n"
	"esac\n"
	"case \"$opt_force_lock\" in \"\") ;; *) lock_did_lock=true; ;; esac\n"
	"#\n"
	"#  <<<<----- here ends the code to lock a session \n"
	"#\n",
	TEVENT(2, vlv, SW_SOC_LOCK_CREATED, "lockpath=$LOCKPATH: status=$sw_retval"),
	TEVENT(2, vlv, SW_SOC_IS_READ_ONLY, "lockpath=$LOCKPATH: status=1"),
	TEVENT(2, vlv, SW_SOC_LOCK_FAILURE, "lockpath=$LOCKPATH: status=1"),
	TEVENT(2, vlv, SW_CONFLICTING_SESSION_IN_PROGRESS, "lockpath=${LOCKPATH}: status=1"),
	TEVENT(2, vlv, SW_SOC_LOCK_FAILURE, "lockpath=$LOCKPATH: status=1"),
	TEVENT(2, vlv, SW_CONFLICTING_SESSION_IN_PROGRESS, "lockpath=$LOCKPATH: status=2"),
	TEVENT(2, vlv, SW_SOC_LOCK_FAILURE, "lockpath=$LOCKPATH: status=2"),
	TEVENT(2, vlv, SW_SOC_LOCK_FAILURE, "lockpath=$LOCKPATH: status=$lock_status")
	);
	return strob_str(buf);
}

char *
swpl_shellfrag_session_unlock(STROB * buf, int vlv)
{
	strob_sprintf(buf, 0,
	CSHID
	"case \"$lock_did_lock\" in\n"
	"       \"\") ;;\n"
	"       *)\n" 
	"       lf_remove_lock \"$opt_force_lock\"\n"
	"       %s\n"  /* SW_SOC_LOCK_REMOVED */
	"       ;;\n"  
	"esac\n",
	TEVENT(2, vlv, SW_SOC_LOCK_REMOVED, "status=$?")
	);
	return strob_str(buf);
}

int
swpl_session_lock(GB * G, SWICOL * swicol, char * target_path, int ofd, int event_fd)
{
	int ret;
	STROB * tmp;
	STROB * shell_lib_buf;

	E_DEBUG("");
	E_DEBUG2("target_path=[%s]", target_path);
	shell_lib_buf = strob_open(24);
	tmp = strob_open(24);

	/* swicol_set_task_idstring(swicol, SWBIS_TS_make_locked_session); */
	strob_sprintf(tmp, 0,
		CSHID
		"%s\n"
		"%s\n"
		"%s\n"
		"%s\n"
		"%s\n"
		"lf_make_lock </dev/null\n"
		"sw_retval=$?\n"
		"dd count=1 bs=512 of=/dev/null 2>/dev/null\n"
		,
		shlib_get_function_text_by_name("lf_make_lockfile_name", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("lf_make_lockfile_entry", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("lf_append_lockfile_entry", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("lf_test_lock", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("lf_make_lock", shell_lib_buf, NULL)
		);

	E_DEBUG("");
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd, 
		512, 			/* stdin_file_size */
		target_path, 		/* directory to run in*/
		strob_str(tmp),  	/* the task script */
		SWBIS_TS_make_locked_session
		);

	if (ret != 0) {
		E_DEBUG2("swicol_rpsh_task_send_script2 returned %d", ret);
		return -1;
	}

	E_DEBUG("");
	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret < 0) {
		E_DEBUG("error from etar_write_trailer_blocks");
		SWBIS_ERROR_IMPL();
		SWI_internal_error();
		return -1;
	}

	E_DEBUG("");
	ret = swicol_rpsh_task_expect(swicol,
				event_fd,
				SWICOL_TL_12 /*time limit*/);

	E_DEBUG2("ret=%d", ret);
	if (ret < 0) {
		E_DEBUG2("lock returning %d", ret);
		return -1;
	} else if (ret > 0 && G->g_force_locks == 0) {
		E_DEBUG2("lock returning %d", ret);
		return ret;
	}

	strob_close(tmp);
	strob_close(shell_lib_buf);
	return 0;
}

char *
swpl_looper_payload_routine(STROB * buf, int vlv, char * locked_region)
{
	STROB * lockfrag_buffer;
	STROB * unlockfrag_buffer;

	lockfrag_buffer = strob_open(164);
	unlockfrag_buffer = strob_open(164);

	strob_sprintf(buf, 0,
		CSHID
		"shls_looper_payload()\n"
		"{\n"
		"	catalog_entry_dir=\"$1\"\n"
		"	case \"$targetpath\" in \"\") return 1;; esac\n"
		"	cd \"$targetpath\"\n"
		"	case $? in\n"
		"		0)\n"
		"			test -d \"$catalog_entry_dir\" ||\n"
		"			echo error: \"'\" \"$catalog_entry_dir\" \"'\" is not a directory 1>&2 ||\n"
		"			return 1\n"
		"			shls_bashin2 \"" SWBIS_TS_check_loop "\"\n"
		"			sw_retval=$?\n"
		"			case $sw_retval in 0) $sh_dash_s;; *) return 1;; esac\n"
		"			sw_retval=$?\n"
		"			swexec_status=$sw_retval\n"
		"			catentdir=\"$catalog_entry_dir\"\n"
		"			# remove the last two directory components\n"
		"			catentdir=\"${catentdir%%/*}\"\n"
		"			catentdir=\"${catentdir%%/*}\"\n"
		"			LOCKPATH=\"$catentdir\"\n"
		"			LOCKENTRY=$$\n"
		"			# set the lock_status to an error\n"
		"			lock_status=3\n"
		"			lock_did_lock=\"\"\n"
		"			# the locking code gets put inline here\n"
		"			%s\n"	/* <---- swpl_shellfrag_session_lock() */
		"			case \"$lock_status\" in\n"
		"			0)\n"
		"				%s\n" /* locked region */
		"			;;\n"
		"			esac\n"
		"			%s\n"	/* <---- swpl_shellfrag_session_unlock() */
		"			return $swexec_status\n"
		"		;;\n"
		"		*)\n"
		"			return 1\n"
		"		;;\n"
		"	esac\n"
		"	#echo $1>>/tmp/looper.out\n"
		"}\n",
/*_% */		swpl_shellfrag_session_lock(lockfrag_buffer, vlv),
		locked_region,
/*_% */		swpl_shellfrag_session_unlock(unlockfrag_buffer, vlv)
		);
	strob_close(lockfrag_buffer);
	strob_close(unlockfrag_buffer);
	return strob_str(buf);
}

/**
 * swpl_signature_policy_accept - analyze gpg status line
 *
 * Return 0 if authenticity meets requiment, non-zero if not
 */

int
swpl_signature_policy_accept(GB * G, SWGPG_VALIDATE * w, int verbose_level, char * swspec_string)
{
	int retval;
	int n;
	int num_of_sigs;
	int ngood;
	int ret;
	int gotta_have_all;
	struct extendedOptions * opta;
	char * blob;
	int result;
	int sig_level;
	int sig_fd;
	int logger_fd;
	int status_fd;
	int not_confirmed_msg_fd;

	opta = G->optaM;
	swgpg_set_status_array(w); /* harmless if already done so do it again */

	sig_level = swlib_atoi(get_opta(opta, SW_E_swbis_sig_level), &result);
	if (result) {
		return -1;
	}

	gotta_have_all = swextopt_is_option_true(SW_E_swbis_enforce_all_signatures, opta);

	n = 0;
	num_of_sigs = swgpg_get_number_of_sigs(w);	

	ngood = 0;
	while((blob=strar_get(w->list_of_status_blobsM, n)) != NULL) {
		sig_fd = -1;
		status_fd = -1;
		logger_fd = -1;
		ret = swgpg_get_status(w, n);
		switch (ret) {
			case SWGPG_SIG_VALID:
				ngood++;
				if (verbose_level >= SWC_VERBOSE_2) {
					logger_fd = STDOUT_FILENO;
				}
				if (verbose_level >= SWC_VERBOSE_3) {
					status_fd = STDOUT_FILENO;
				}
				break;
			default:
				logger_fd = STDERR_FILENO;
				if (verbose_level >= SWC_VERBOSE_3)
					status_fd = STDERR_FILENO;
				break;
		}
		swgpg_show(w, n, sig_fd, status_fd, logger_fd);
		n++;
	}
	
	retval = 1;
	if (sig_level == 0 && !gotta_have_all) {
		retval = 0;
	} else if (sig_level > 0 && !gotta_have_all) {
		if (ngood >= sig_level) {
			retval = 0;
		}
	} else if ( sig_level < 0 /* same as gotta_have_all */) {
		/*
 		 * Must have all and at least one.
 		 */
		if (
			ngood == num_of_sigs &&
			ngood > 0 &&
			1
		) {
			retval = 0;
		}
	} else if (sig_level > 0 && gotta_have_all) {
		if (
			ngood >= sig_level &&
			ngood == num_of_sigs &&
			ngood > 0 &&
			1
		) {
			retval = 0;
		}
	}

	if (strcmp(swlib_utilname_get(), "swlist") == 0) {
		not_confirmed_msg_fd = STDERR_FILENO;
	} else {
		not_confirmed_msg_fd = STDOUT_FILENO;
	}

	if (retval) {
		swc_stderr_fd2_set(G, STDERR_FILENO);
		sw_l_msg(G, SWC_VERBOSE_1, "%s at sig_level=%d for %s: %d sigs, %d good: status=1\n",
			swevent_code(SW_ISC_INTEGRITY_NOT_CONFIRMED), sig_level, swspec_string, num_of_sigs, ngood);
		swc_stderr_fd2_restore(G);
	} else {
		if (sig_level == 0 && num_of_sigs != ngood) {
			sw_l_msg(G, SWC_VERBOSE_1, "Warning: %s at sig_level=%d for %s: %d sigs, %d good: status=2\n",
				swevent_code(SW_ISC_INTEGRITY_NOT_CONFIRMED), sig_level, swspec_string, num_of_sigs, ngood);
		} else if (
				(sig_level > 0 && ngood >= sig_level) ||
				(sig_level == 0 && num_of_sigs > 0 && num_of_sigs == ngood) ||
				0
		) {
			swc_stderr_fd2_set(G, not_confirmed_msg_fd);
			swc_stderr_fd2_set(G, STDOUT_FILENO); /* ??? */
			sw_l_msg(G, SWC_VERBOSE_2, "%s at sig_level=%d for %s: %d sigs, %d good: status=0\n",
				swevent_code(SW_ISC_INTEGRITY_CONFIRMED), sig_level, swspec_string, num_of_sigs, ngood);
			swc_stderr_fd2_restore(G);
		} else if (
				(sig_level == 0 && num_of_sigs == 0) ||
				0
		) {
			swc_stderr_fd2_set(G, not_confirmed_msg_fd);
			sw_l_msg(G, SWC_VERBOSE_1, "%s at sig_level=%d for %s: %d sigs, %d good: status=0\n",
				swevent_code(SW_ISC_INTEGRITY_NOT_CONFIRMED), sig_level, swspec_string, num_of_sigs, ngood);
			swc_stderr_fd2_restore(G);
		} else {
			swc_stderr_fd2_set(G, not_confirmed_msg_fd);
			sw_l_msg(G, SWC_VERBOSE_1, "%s at sig_level=%d for %s: %d sigs, %d good: status=0\n",
				swevent_code(SW_ISC_INTEGRITY_NOT_CONFIRMED), sig_level, swspec_string, num_of_sigs, ngood);
			swc_stderr_fd2_restore(G);
		}
	}
	return retval;
}

/**
 * swpl_remove_catalog_entry - remove the [entire] catalog entry
 *  @e : entry to remove
 *
 * To remove an entry, remove the inactive name, then move the 
 * entry to the inactive name
 */

int
swpl_remove_catalog_entry(GB * G, SWICOL * swicol, SWICAT_E * e, int ofd, char * iscpath)
{
	int retval;
	retval = alter_catalog_entry(0 /*remove_restore*/, G, swicol, e, ofd, iscpath);
	return retval;
}

int
swpl_restore_catalog_entry(GB * G, SWICOL * swicol, SWICAT_E * e, int ofd, char * iscpath)
{
	int retval;
	retval = alter_catalog_entry(1 /*remove_restore*/, G, swicol, e, ofd, iscpath);
	return retval;
}

char *
swpl_rename_suffix(STROB * buf)
{
	/*
	time_t t;
	struct tm * m;
	t = time(NULL);
	m = localtime(&t);
	strob_sprintf(buf, 0, "swbis_%04d%02d%02dT%02d%02d%02dZ",
			m->tm_year+1900, m->tm_mon + 1, m->tm_mday, m->tm_hour, m->tm_min, m->tm_sec);
	*/
	/* No, I don't like this, just use "swbisold" as the suffix */

	strob_sprintf(buf, 0, "%s", "swbisold");
	return strob_str(buf);
}

void
swpl_tag_volatile_files(SWI_FILELIST * fl)
{
	int ix;
	char * sx;
	int is_volatile;
	char * prefix;
	int type;	

	swi_fl_qsort_forward(fl);
	ix = 0;
	prefix = NULL;
	while ((sx=swi_fl_get_path(fl, ix)) != NULL) {
		type = swi_fl_get_type(fl, ix),
		is_volatile = swi_fl_is_volatile(fl, ix);
		if (prefix == NULL) {
			if (type == DIRTYPE && is_volatile) {
				/* tag every file in this directory as volatile */
				if (prefix) free(prefix);
				prefix = strdup(sx);
			}
		} else {
			if (strstr(sx, prefix) == sx) {
				/* contained in current directory */
				swi_fl_set_is_volatile(fl, (char*)NULL, ix, 1);
			} else {
				/* out of the directory */
				if (prefix) free(prefix);
				prefix = NULL;
			}
		}
		ix++;
	}
	if (prefix) free(prefix);
}

int
swpl_show_file_list(GB * G, SWI_FILELIST * fl)
{
	char * name;
	int ix;

	E_DEBUG("");
	swi_fl_qsort_forward(fl);
	ix = 0;
	while ((name=swi_fl_get_path(fl, ix)) != NULL) {
		sw_l_msg(G, SWC_VERBOSE_1, "%s\n", name);
		ix++;
	}
	E_DEBUG("");
	return 0;
}

int
swpl_make_here_document_source(GB * G, STROB * scriptbuf, STROB * here_payload)
{
	STROB * stop_word;
	int ret;
	
	stop_word = strob_open(32);

	ret = determine_here_document_stop_word_bystr(here_payload, stop_word);
	if (ret != 0) {
		/* this really, really should never happen */
		exit(12);
	}

	E_DEBUG("");

	/* Start of hear document */
	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"(\n"
		"cat <<'%s'\n",
		strob_str(stop_word));

	/* payload of here document */
	strob_sprintf(scriptbuf, STROB_DO_APPEND, "%s", strob_str(here_payload));

	/* End of here document */
	E_DEBUG("");
	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"%s\n"
		")",
		strob_str(stop_word));

	strob_close(stop_word);
	return 0;
}

int
swpl_make_verify_command_script_fragment(GB * G,
		STROB * scriptbuf,
		SWI_FILELIST * fl,
		char * pax_write_command_key, int be_silent)
{
	int ret;
	int check_volatile;
	int is_volatile;
	STROB * stop_word;
	STROB * shell_lib_buf;
	char * name;
	int ix;
	
	shell_lib_buf = strob_open(24);

	/* Tag every file in a volatile directory as being volatile itself
	   FIXME ??? is this the correct thing to do */

	E_DEBUG("");
	swpl_tag_volatile_files(fl);
	
	stop_word = strob_open(32);

	ret = determine_here_document_stop_word(fl, stop_word);
	if (ret != 0) {
		/* this really should never happen */
		exit(12);
	}

	E_DEBUG("");

	/* sort the files in forward direction */
	swi_fl_qsort_forward(fl);

	strob_sprintf(scriptbuf, STROB_DO_APPEND, "rm_retval=0\n");

	/* Construct the here document of filenames that feed
	   GNU tar's stdin to make an archive */
	/* Something like this:
		(
		cat <<'/tmp/End'
		a
		c
		/tmp/End
		) | tar  cf - --no-recursion --files-from=-
	*/

	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"%s\n"
		"%s\n"
		"%s\n",
		shlib_get_function_text_by_name("shls_check_for_gnu_tar", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("shls_check_for_recent_gnu_gtar", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("shls_check_for_gnu_gtar", shell_lib_buf, NULL)
		);

	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"%s\n"
		"%s\n",
		shlib_get_function_text_by_name("shls_missing_which", shell_lib_buf, NULL),
		shlib_get_function_text_by_name("shls_write_files_ar", shell_lib_buf, NULL)
		);


	/*
 	 * thw following test has the the effect of requiring that GNU tar or pax
 	 * is present.
 	 */

	if (strcmp(pax_write_command_key, "gtar") == 0 ||
	    strcmp(pax_write_command_key, "pax") == 0) {
		strob_sprintf(scriptbuf, STROB_DO_APPEND,
			CSHID
			"check_has_gnu_gtar=0\n"
			"check_has_gnu_tar=0\n"
			);
	} else if (strcmp(pax_write_command_key, "tar") == 0) {
		/* Check to see if tar is GNU tar */
		
		strob_sprintf(scriptbuf, STROB_DO_APPEND,
			CSHID
			"shls_check_for_gnu_tar\n"
			"check_has_gnu_tar=$?\n");

		/*
 		 * just to be accomodative of systems that have GNU tar as gtar,
 		 * add this check even though the correct thing to do is fail.
 		 */

		strob_sprintf(scriptbuf, STROB_DO_APPEND,
			CSHID
			"shls_check_for_gnu_gtar\n"
			"check_has_gnu_gtar=$?\n");

	} else if (strcmp(pax_write_command_key, "detect") == 0) {
		/* Use GNU tar if tar is GNU tar, or use pax
		   this is handled below */
		strob_sprintf(scriptbuf, STROB_DO_APPEND,
			CSHID
			"check_has_gnu_tar=0\n");
	} else {
		strob_sprintf(scriptbuf, STROB_DO_APPEND,
			CSHID
			"check_has_gnu_tar=0\n");
	}

	/*
 	 * check_has_gnu_tar=0 means yes, has either pax or GNU tar
 	 * Anything is not acceptable and the script will abort.
 	 */

	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"\n"
		"GTAR=/\n"
		"case $check_has_gnu_gtar in\n"
		"	0)\n"
		"	GTAR=gtar\n"
		"	;;\n"
		"	*)\n"
		"	;;\n"
		"esac\n"
		"\n"
		"case $check_has_gnu_tar in\n"
		"	0)\n"
		"	GTAR=tar\n"
		"	;;\n"
		"	*)\n"
		"	case $check_has_gnu_gtar in\n"
		"		0)\n"
		"		;;\n"
		"		*)\n"
		"		# Neither gtar or GNU tar was found, fail now\n"
		"		# simulate the response of GNU tar to an empty list\n"
		"		dd count=2 bs=512 if=/dev/zero 2>/dev/null \n"
		"		exit " xSTR(SWP_RP_STATUS_NO_GNU_TAR) "\n"
		"		;;\n"
		"		esac\n"
		"	;;\n"
		"esac\n"
		);

	/* Start of hear document */
	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"(\n"
		"cat <<'%s'\n",
		strob_str(stop_word));

	/* priint each file */
	E_DEBUG("");
	ix = 0;
	
	if (strcmp(swlib_utilname_get(),  SWUTIL_NAME_SWVERIFY) == 0) {
		check_volatile = swextopt_is_option_true(SW_E_check_volatile, G->optaM);
	} else {
		check_volatile = 0;
	}

	while ((name=swi_fl_get_path(fl, ix)) != NULL) {
		if (G->devel_verboseM) {
			SWLIB_INFO3("INFO; file=[%s] type=[%c]", name, (char)swi_fl_get_type(fl, ix));
		}
		/* Check the volatile flag */
		is_volatile = 0;	
		if (check_volatile == 0) {
			/* exclude volatile */
			is_volatile = swi_fl_is_volatile(fl, ix);
		}
		if (is_volatile == 0)
			strob_sprintf(scriptbuf, STROB_DO_APPEND, "%s\n", name);
		ix++;
	}

	/* End of here document */
	E_DEBUG("");
	strob_sprintf(scriptbuf, STROB_DO_APPEND,
		"%s\n"
		") |",
		strob_str(stop_word));


	E_DEBUG("");
	/* write the tar writing command */
	if (
		G->g_do_dereferenceM ||
		strcmp(pax_write_command_key, "tar") == 0 ||
		strcmp(pax_write_command_key, "gtar") == 0 ||
		0
	) {
		do_tar_key: /* goto label */
		/*
 		 * If here, then tar is really GNU tar,  and the -H pax format works.
 		 */

		/* GTAR has been set above by the tests */

		strob_sprintf(scriptbuf, STROB_DO_APPEND,
			"gtar_hdref=\"\"\n"
			"case \"%s\" in\n"
			"yes)\n"
			"	shls_check_for_recent_gnu_gtar\n"
			"	case $? in\n"
			"		0)\n"
			"		gtar_hdref=\"--hard-dereference\"\n"
			"		;;\n"
			"		*)\n"
			"		echo \"swinstall: warning: tar option ''--hard-derefence'' is not available\" 1>&2\n"
			"		;;\n"
			"	esac\n"
			";;\n"
			"esac\n"
			"case \"$GTAR\" in\n"
			"	/)\n"
			"	# FATAL error GNU tar is missing\n"
			"	# simulate the response of GNU tar to an empty list\n"
			"	dd count=2 bs=512 if=/dev/zero 2>/dev/null \n"
			"	exit " xSTR(SWP_RP_STATUS_NO_GNU_TAR) "\n"
			"	;;\n"
			"esac\n"
			" $GTAR %scf - -b 1 -H pax %s $gtar_hdref  --no-recursion --files-from=- %s\n",
			(G->g_do_hard_dereferenceM == 0 ? "": "yes"),
			(char*)(G->g_verboseG > SWC_VERBOSE_2 ? "v" : ""),
			(G->g_do_dereferenceM == 0 ? "": "--dereference"),
			(be_silent ? "2>/dev/null" : ""));
	} else if (
		strcmp(pax_write_command_key, "pax") == 0 ||
		0
	) {
		/*
 		 * Pax is specified
 		 */
		strob_sprintf(scriptbuf, STROB_DO_APPEND, " pax -d -w -b 512\n");
	} else if (
		strcmp(pax_write_command_key, "detect") == 0 ||
		0
	) {
		/*
		 * write code to use either pax or tar, making sure it is GNU tar
		 */

		strob_sprintf(scriptbuf, STROB_DO_APPEND,
				" shls_write_files_ar\n"
				CSHID
				"# Support controlled bailout in the event \n"
				"# neither GNU tar or pax is present \n"
				"case $? in 126) exit " xSTR(SWP_RP_STATUS_NO_GNU_TAR) ";; esac\n"
				);
	} else {
		goto do_tar_key;
	}

	/* FIXME, sw_retval is set above only, and does not report
	   the exit status of the primary reading command */

	E_DEBUG("");
	strob_sprintf(scriptbuf, STROB_DO_APPEND,
			"# FIXME, sw_retval should be set here\n"
			"### Not used   retrieve_retval=$?\n"
			CSHID
			);

	E_DEBUG("");
	strob_close(shell_lib_buf);
	strob_close(stop_word);
	return 0;
}

int
swpl_ls_fileset_from_iscat(SWI * swi, int ofd, int ls_verbose, int * available_attributes, int skip_volatile, int check_contents)
{
	int retval;
	SWHEADER_STATE state1;
	SWHEADER * indexheader;
	SWHEADER * infoheader;
	SWI_PRODUCT * current_product;
	SWI_XFILE * current_fileset;
	struct new_cpio_header * file_hdr;
	char * next_line;
	char * directory;
	char * next_attr;
	char * keyword;
	char * value;
	int has_md5;
	int has_sha1;
	int has_sha512;
	AHS * info_ahs2;
	int ret;
	STROB * ls_line;
	STROB * tmp2;
	int file_attribute_usage;
	FILE_DIGS * digs;

	E_DEBUG("");

	has_md5 = 0;
	has_sha1 = 0;
	has_sha512 = 0;
	tmp2 = strob_open(30);

	file_attribute_usage = 0;
	retval = 0;
	info_ahs2 = ahs_open();
	ls_line = strob_open(60);

	if (check_contents == 0) {		
		ls_verbose &= ~(LS_LIST_VERBOSE_WITH_MD5|LS_LIST_VERBOSE_WITH_SHA1|LS_LIST_VERBOSE_WITH_SHA512);
		ls_verbose &= ~(LS_LIST_VERBOSE_WITH_SIZE);
	}

	/* FIXME: Eventually support multiple products and filesets  */

	swpl_enforce_one_prod_one_fileset(swi);

	/* Get the current product and fileset, for now, the first ones */

	current_product = swi_package_get_product(swi->swi_pkgM, 0 /* the first one */);
	current_fileset = swi_product_get_fileset(current_product, 0 /* the first one */);

	indexheader = swi_get_global_index_header(swi);
	swheader_set_current_offset(indexheader, current_product->p_baseM.header_indexM);
	directory = swheader_get_single_attribute_value(indexheader, SW_A_directory);
	directory = swlib_attribute_check_default(SW_A_product, SW_A_directory, directory);

	/* Loop over the INFO file definitions, these are found
		in the SWHEADER for the fileset */

	infoheader = swi_get_fileset_info_header(swi, 0, 0); /* FIXME */
	swheader_store_state(infoheader, NULL);
	swheader_reset(infoheader);

	file_hdr = ahs_vfile_hdr(info_ahs2);
	taru_init_header_digs(file_hdr);
	digs = file_hdr->digsM;

	taru_digs_init(digs, check_contents ? 1 : 0, DIGS_DO_POISON);

        while ((next_line=swheader_get_next_object(infoheader, (int)UCHAR_MAX, (int)UCHAR_MAX))) {

		E_DEBUG2("next_line=%s\n", next_line);
		if (swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ) {
			/* Sanity check */
			SWBIS_IMPL_ERROR_DIE(1);
		}

		keyword = swheaderline_get_keyword(next_line);

		/* skip control files */

		SWLIB_ASSERT(keyword != NULL);
		if (strcmp(keyword, SW_A_control_file) == 0) {
			continue;
		}

		taru_init_header(file_hdr);
		file_hdr->digsM = digs;
		taru_digs_init(digs, check_contents ? 1 : 0, DIGS_DO_POISON);
		
		swheader_store_state(infoheader, &state1);

		ret = swheader_fileobject2filehdr(infoheader, file_hdr);
		if (ret) {
			SWLIB_FATAL("");
		}

		if (swi->swi_pkgM->locationM) {
			swlib_apply_location(tmp2, ahsStaticGetTarFilename(file_hdr), swi->swi_pkgM->locationM, directory);
			ahsStaticSetTarFilename(file_hdr, strob_str(tmp2));
		}

		if (file_attribute_usage == 0) {
			/* first time */
			file_attribute_usage = file_hdr->usage_maskM;
		} else {
			/* construct the minimum attributes that all the files have */
			file_attribute_usage &= file_hdr->usage_maskM;
		}

		/* file_attribute_usage are the TARU_UM_* flags and it returns
		   the policy in terms of the LS_LIST_VERBOSE flags  */
		*available_attributes |= determine_ls_list_flags(file_attribute_usage, file_hdr, check_contents);
		/* fprintf(stderr, "=========== %d\n", *available_attributes); */

		/* The contents of the INFO object are first detected and set in the
		   DIGS object in the file_hdr object, then they are transferred to
		   the *available_attributes flags by determine_ls_list_flags() */
		
		if (check_contents) {
			if (*available_attributes & LS_LIST_VERBOSE_WITH_MD5) { has_md5 = 1; }
			if (*available_attributes & LS_LIST_VERBOSE_WITH_SHA1) { has_sha1 = 1; }
			if (*available_attributes & LS_LIST_VERBOSE_WITH_SHA512) { has_sha512 = 1; }
			
			/* If any files have a digest then assume all of them do */

			if (has_md5) ls_verbose |= LS_LIST_VERBOSE_WITH_MD5;
			if (has_sha1) ls_verbose |= LS_LIST_VERBOSE_WITH_SHA1;
			if (has_sha512) ls_verbose |= LS_LIST_VERBOSE_WITH_SHA512;
		}

		merge_name_id(&ls_verbose, *available_attributes);

		swheader_restore_state(infoheader, &state1);

		/*  Here is some example code for accessing and
			printing the file attributes */

		if (0) {
			/* example code */
			value = swheaderline_get_value(next_line, (int*)(NULL));
			swlib_doif_writef(swi->verboseM, SWC_VERBOSE_1,
				(struct sw_logspec *)(NULL), STDERR_FILENO,
				"%s %s\n", keyword, value?value:"");
			while((next_attr=swheader_get_next_attribute(infoheader))) {
				keyword = swheaderline_get_keyword(next_attr);
				value = swheaderline_get_value(next_attr, (int*)(NULL));
				swlib_doif_writef(swi->verboseM, SWC_VERBOSE_1,
					(struct sw_logspec *)(NULL), STDERR_FILENO,
					"%s %s\n", keyword, value?value:"");
			}	
		}

		/* construct the string to print */

		ls_verbose |= LS_LIST_VERBOSE_PREPEND_DOTSLASH; 
		ls_verbose |= LS_LIST_VERBOSE_LINKNAME_PLAIN; 
		taru_print_tar_ls_list(ls_line, file_hdr, ls_verbose);

		/* Now print the line */
		if (skip_volatile && file_hdr->usage_maskM & TARU_UM_IS_VOLATILE) {
			;
		} else {
			if (ofd >= 0) {
				uxfio_unix_atomic_write(ofd, strob_str(ls_line), strob_strlen(ls_line));
			}
		}
        }

	swheader_restore_state(infoheader, NULL);
	/* taru_init_header(file_hdr); */
	/* taru_digs_delete(digs); */
	ahs_close(info_ahs2);
	strob_close(ls_line);
	strob_close(tmp2);
	return retval;
}

void
swpl_bashin_posixsh(STROB * buf)
{
	char * prog;
	char * swbis;
	char * binsh;

	binsh = "sh";
	prog = swlib_utilname_get();
	if (strcmp(prog, "swremove") == 0) {
		/* this if-else is here so when using the --cleansh option
 		   the process is disguised */
		prog = "/";  /* assumes that "/" is harmless to execute */
		swbis = "/";
	} else {
		swbis = "/_swbis";
	}

	strob_sprintf(buf, STROB_DO_APPEND,
	"{\n"
	"export XXX_PKG\n"
	"export XXX_PGM\n"
	"export XXX_SH\n"
	"export PATH\n"
	"XXX_PKG=\"%s\"\n"
	"XXX_PGM=\"%s\"\n"
	"XPATH=`getconf PATH`\n"
	"case $? in\n"
	"0)\n"
	"PATH=\"$XPATH\"\n"
	";;\n"
	"*)\n"
	"PATH=/bin:/usr/bin\n"
	"echo \"swbis: $XXX_PGM: getconf not found (required by --shell-command=posix option)\" 1>&2\n"
	"echo \"swbis: $XXX_PGM: fatal, exiting with status 125\" 1>&2\n"
	"exit 125\n"
	";;\n"
	"esac\n"
	"xxx_missing_which() {\n"
	"# Prefix: xxabb\n"
	"xxabb_pgm=\"$1\"\n"
	"xxabb_name=`which $xxabb_pgm 2>/dev/null`\n"
	"test -f \"$xxabb_name\" -o -h \"$xxabb_name\"\n"
	"case \"$?\" in\n"
	"0) echo \"$xxabb_name\"; return 0; ;;\n"
	"*) echo \"/\"; return 1; ;;\n"
	"esac\n"
	"return 0\n"
	"}\n"
	"XXX_SH=`xxx_missing_which sh`\n"
	"test -x \"$XXX_SH\" &&\n"
	"{\n"
	"\"$XXX_SH\" -s 1>/dev/null 2>&1 <<HERE\n"
	"(read A; exit 0);\n"
	"/\n"
	"HERE\n"
	"case $? in\n"
	"0)\n"
	"exec \"$XXX_SH\" -s  \"$XXX_PKG\" /\"_$XXX_PGM\" / / PSH=\"\\\"\"$XXX_SH\" -s\\\"\"\n"
	"exit 121\n"
	";;\n"
	"esac\n"
	"}\n"
	"echo \"swbis: $XXX_PGM: a POSIX shell in PATH=getconf PATH is not suitable on host `hostname`\" 1>&2\n"
	"echo \"swbis: $XXX_PGM: fatal, exiting with status 124\" 1>&2\n"
	"exit 124\n"
	"}\n",
		swbis, prog
	);
	make_padding_for_login_shell(16, buf); /* needs to be 16 to support ash as the
						 account holders login shell because the
						 ash read() size is 8192 bytes */
}

void
swpl_bashin_testsh(STROB * buf, char * shell)
{
	char * prog;
	char * swbis;
	char * binsh;

	binsh = construct_binsh_name(shell);

	prog = swlib_utilname_get();
	if (strcmp(prog, "swremove") == 0) {
		/* this if-else is here so when using the --cleansh option
 		   the process is disguised */
		prog = "/";  /* assumes that "/" is harmless to execute */
		swbis = "/";
	} else {
		swbis = "/_swbis";
	}

	strob_sprintf(buf, STROB_DO_APPEND,
	"{\n"
	"export XXX_PKG\n"
	"export XXX_PGM\n"
	"export XXX_SH\n"
	"XXX_PKG=\"%s\"\n"
	"XXX_PGM=\"%s\"\n"
	"XXX_SH=\"%s\"\n"
	"test -x $XXX_SH &&\n"
	"{\n"
	"$XXX_SH -s 1>/dev/null 2>&1 <<HERE\n"
	"(read A; exit 0);\n"
	"/\n"
	"HERE\n"
	"case $? in\n"
	"0)\n"
	"exec $XXX_SH -s  $XXX_PKG /_$XXX_PGM / / PSH=\"\\\"\"$XXX_SH\" -s\\\"\"\n"
	"exit 121\n"
	";;\n"
	"esac\n"
	"}\n"
	"echo \"swbis: $XXX_PGM: the specified shell is not suitable on host `hostname`\" 1>&2\n"
	"echo \"swbis: $XXX_PGM: fatal, exiting with status 126\" 1>&2\n"
	"exit 126\n"
	"}\n",
		swbis, prog, binsh
	);

	make_padding_for_login_shell(16, buf); /* needs to be 16 to support ash as the
						 account holders login shell because the
						 ash read() size is 8192 bytes */
}

void
swpl_bashin_detect(STROB * buf)
{
	char * prog;
	char * swbis;

	/* New and Improved July 2014 */
	
	/* Here is the code to detect a shell that complies with
	the POSIX prescribed handling of STDIN (which is not to
	read ahead but rather read 1 byte at a time for the
	single command or compound command).  To get around shells
	that don't do this, initially we pad with 1024 bytes of "####..."
	which assumes that the shell is improperly gobbling only
	1024 bytes at a time, then we test and exec the POSIX shell.


	ASIDE: the original ash shell gobbles 8kb per read() call,
	IMHO this is terrible and does seem to be outside the mainstream
	of traditional /bin/sh behaviour, systems with ash as /bin/sh are
	broken as far as swbis interoperability is concerned. (Note, dash
	has the read() size reduced, dash is supported by swbis).
        For example the heirloom shell gobbles stdin at 128 bytes per read()
        ash and (unpatched) dash at 8192 and Debian dash at 512. Older
	versions of the Korn shell read at 512 bytes per read().
         */

	/* test for shell  with required POSIX feature
		(echo "(sed -e s/a/d/);"; echo /)  | ksh -s 1>/dev/null 2>&1; echo $?
	or better, for example
		$ (echo "(read A; exit 0)"; echo /) | /bin/ash -s; echo $?
		/: permission denied
		126
		$ (echo "(read A; exit 0)"; echo /) | /bin/sh -s; echo $?
		0
	*/

	prog = swlib_utilname_get();
	if (strcmp(prog, "swremove") == 0) {
		/* this if-else is here so when using the --cleansh option
 		   the process is disguised */
		prog = "/";  /* assumes that "/" is harmless to execute */
		swbis = "/";
	} else {
		swbis = "/_swbis";
	}

	strob_sprintf(buf, STROB_DO_APPEND,
	"{\n"
	"export XXX_PKG\n"
	"export XXX_PGM\n"
	"export XXX_SH\n"
	"XXX_PKG=\"%s\"\n"
	"XXX_PGM=\"%s\"\n"
	"XXX_SH=/bin/bash\n"
	"test -x $XXX_SH &&\n"
	"{\n"
	"$XXX_SH -s 1>/dev/null 2>&1 <<HERE\n"
	"(read A; exit 0);\n"
	"/\n"
	"HERE\n"
	"case $? in\n"
	"0)\n"
	"exec $XXX_SH -s  $XXX_PKG /_$XXX_PGM / / PSH=\"\\\"\"$XXX_SH\" -s\\\"\"\n"
	"exit 121\n"
	";;\n"
	"esac\n"
	"}\n"
	"XXX_SH=/bin/ksh\n"
	"test -x $XXX_SH &&\n"
	"{\n"
	"$XXX_SH -s 1>/dev/null 2>&1 <<HERE\n"
	"(read A; exit 0);\n"
	"/\n"
	"HERE\n"
	"case $? in\n"
	"0)\n"
	"exec $XXX_SH -s  $XXX_PKG /_$XXX_PGM / / PSH=\"\\\"\"$XXX_SH\" -s\\\"\"\n"
	"exit 121\n"
	";;\n"
	"esac\n"
	"}\n"
	"XXX_SH=/usr/xpg4/bin/sh\n"
	"test -x $XXX_SH &&\n"
	"{\n"
	"$XXX_SH -s 1>/dev/null 2>&1 <<HERE\n"
	"(read A; exit 0);\n"
	"/\n"
	"HERE\n"
	"case $? in\n"
	"0)\n"
	"exec $XXX_SH -s  $XXX_PKG /_$XXX_PGM / / PSH=\"\\\"\"$XXX_SH\" -s\\\"\"\n"
	"exit 121\n"
	";;\n"
	"esac\n"
	"}\n"
	"XXX_SH=/bin/sh\n"
	"test -x $XXX_SH &&\n"
	"{\n"
	"$XXX_SH -s 1>/dev/null 2>&1 <<HERE\n"
	"(read A; exit 0);\n"
	"/\n"
	"HERE\n"
	"case $? in\n"
	"0)\n"
	"exec $XXX_SH -s  $XXX_PKG /_$XXX_PGM / / PSH=\"\\\"\"$XXX_SH\" -s\\\"\"\n"
	"exit 121\n"
	";;\n"
	"esac\n"
	"}\n"
	"echo \"swbis: $XXX_PGM: no suitable shell found on host `hostname`\" 1>&2\n"
	"echo \"swbis: $XXX_PGM: fatal, exiting with status 126\" 1>&2\n"
	"exit 126\n"
	"}\n",
		swbis, prog
	);

	make_padding_for_login_shell(16, buf); /* needs to be 16 to support ash as the
						 account holders login shell because the
						 ash read() size is 8192 bytes */
}

int
swpl_check_package_signatures(GB * G, SWI * swi, int *p_num_checked)
{
	int i;
	int retval;
	int ret;
	char * s;
	int signed_bytes_fd;
	SWGPG_VALIDATE * swgpg;
	STROB * gpg_status;
	int sig_block_start;
	int sig_block_end;
	CPLOB * archive_files;
	SWI_FILE_MEMBER * afile;
	int num_checked;

	if (p_num_checked) *p_num_checked = 0;
	num_checked = 0;
	retval = 0;
	swgpg = swgpg_create();
	gpg_status = strob_open(33);
	archive_files = swi->swi_pkgM->dfilesM->archive_filesM;

	swi_examine_signature_blocks(swi->swi_pkgM->dfilesM, &sig_block_start, &sig_block_end);
	signed_bytes_fd = swlib_open_memfd();
	swpl_write_catalog_data(swi, signed_bytes_fd, sig_block_start, sig_block_end);

	i = 0;
	while ((afile = (SWI_FILE_MEMBER *)cplob_val(archive_files, i++))) {
		if (
			(s=strstr(afile->pathnameM, "/" SW_A_signature)) &&
			(*(s + strlen("/" SW_A_signature)) == '\0')
		) {
			/* Got a signature
			fprintf(stderr, "GOT a sig [%s]\n", afile->pathnameM); */

			uxfio_lseek(signed_bytes_fd, 0L, SEEK_SET);
			strob_memset(gpg_status, (int)('\0'), 500);
			ret = swgpg_run_gpg_verify(swgpg, signed_bytes_fd, afile->dataM, G->g_verboseG, gpg_status);
			/* FIXME for some reason swgpg_run_gpg_verify returns non-zero */

			ret = swgpg_determine_signature_status(strob_str(gpg_status), -1);
			if (ret != 0 || G->g_verboseG >= SWC_VERBOSE_3) {
				fprintf(stderr, "%s", strob_str(gpg_status));
			}
			if (ret == 0) {
				retval++;
			}	
			num_checked++;
		}
	}
	if (p_num_checked) *p_num_checked = num_checked;
	uxfio_close(signed_bytes_fd);
	swgpg_delete(swgpg);
	strob_close(gpg_status);
	return retval;
}

char *
swpl_make_package_signature(GB * G, SWI * swi)
{
	int retval;
	char * sig;
	int signed_bytes_fd;
	int sig_block_start;
	int sig_block_end;
	CPLOB * archive_files;
	char * gpg_name;
	char * gpg_path;
	SHCMD * sigcmd;
	int signer_status;

	retval = 0;
	archive_files = swi->swi_pkgM->dfilesM->archive_filesM;

	/*
	 * always use agent and environment variables
	 * get gpg_name and gpg_path from the environment
	 * GNUPGHOME and GNUPGNAME
	 */
	gpg_name = getenv("GNUPGNAME");
	gpg_path = getenv("GNUPGHOME");

	E_DEBUG("");
	if (!gpg_name || !gpg_path) {
		/* FIXME message here */
		if (!gpg_name)
			fprintf(stderr, "%s: GNUPGNAME not set\n", swlib_utilname_get());
		if (!gpg_path)
			fprintf(stderr, "%s: GNUPGHOME not set\n", swlib_utilname_get());
		return NULL;
	}

	E_DEBUG("");
	sigcmd = swgpg_get_package_signature_command("GPG", gpg_name, gpg_path, SWGPG_SWP_PASS_AGENT);

	E_DEBUG("");
	swi_examine_signature_blocks(swi->swi_pkgM->dfilesM, &sig_block_start, &sig_block_end);
	E_DEBUG("");
	signed_bytes_fd = swlib_open_memfd();
	E_DEBUG("");
	swpl_write_catalog_data(swi, signed_bytes_fd, sig_block_start, sig_block_end);
	E_DEBUG("");
	uxfio_lseek(signed_bytes_fd, 0L, SEEK_SET);

	E_DEBUG("");
	sig = swgpg_get_package_signature(sigcmd, &signer_status,
		SWGPG_SWP_PASS_AGENT, NULL, signed_bytes_fd, 0, NULL);

	/*
	if (sig)
		fprintf(stderr, "%s", sig);
	*/
	E_DEBUG("");
	uxfio_close(signed_bytes_fd);
	E_DEBUG("");
	return sig;
}

char *
swpl_make_environ_transfer_image(STROB * buf)
{
	char ** list;
	char * att;
	char * varname;	
	char * value;
	char * q;
	STROB * tmp;
	char * v;

	tmp = strob_open(30);
	strob_strcpy(buf, "");
	list = environ;
	
	while((att = *(list++)) != NULL) {
		q = NULL;
		q = strchr(att, '=');
		if (!q) continue;
		*q = '\0';
		value = q+1;
		varname = att;
		if (environ_exclude(varname))
			goto HERE_reject_var;	

		if (strpbrk(varname, SWBIS_WS_TAINTED_CHARS SWBIS_TAINTED_CHARS "-:;"))
			goto HERE_reject_var;	
			
		if (strpbrk(value, "`")) /* backtick */
			goto HERE_reject_var;

		/* Now escape " and $ chars */

		v = value;
		strob_strcpy(tmp, "");
		while(*v) {
			if (*v == '"') {
				strob_strcat(tmp, "\\\"");
			} else if (*v == '$') {
				strob_strcat(tmp, "\\$");
			} else {
				strob_charcat(tmp, *v);
			}
			v++;
		}
		
		/* Now with the '"' and '$' escaped and tainted chars
		   checked the following shell expression should be safe

			VAR=${VAR="<value>"} */

		strob_sprintf(buf, DO_APPEND, "export %s\n%s=${%s=\"%s\"}\n",
			varname, varname, varname, strob_str(tmp));

		HERE_reject_var:
		*q = '=';
	}
	strob_close(tmp);
	return strob_str(buf);
}

int
swpl_run_check_script(GB * G, char * script_name, char * id_string, SWI * swi, int ofd, int event_fd, int * p_check_status)
{
	int ret;
	STROB * buf;
	SWI_CONTROL_SCRIPT * check_script;
	int stdin_file_size;
	int this_index;
	int script_status;
	char * check_message;

	buf = strob_open(400);

	ret = swpl_construct_analysis_script(G, script_name, buf, swi, &check_script);
	SWLIB_ASSERT(ret == 0);
	
	stdin_file_size = 512;

	swicol_set_task_idstring(swi->swicolM, id_string);
	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 
		512,
		swi->swi_pkgM->target_pathM,
		strob_str(buf),
		id_string
		);

	if (ret == 0) {
		/* 
		 * Send the payload, in this case, it is a gratuitous block of nuls.
		 */
		ret = etar_write_trailer_blocks(NULL, ofd, 1);

		if (ret < 0) return -1;

		/*
		 * wait for the script to finish
		 */
		ret = swicol_rpsh_task_expect(swi->swicolM,	
				event_fd,
				SWICOL_TL_9 /*timelimit*/);
		if (ret < 0) return -1;

		/*
		 * this shows only the events for this task script
		 */	
		if (swi->debug_eventsM)
			swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, -1);
	
		check_message = swicol_rpsh_get_event_message(swi->swicolM,
       	         			SW_CONTROL_SCRIPT_BEGINS,
					-1, 
					&this_index);

		*p_check_status = -1;  /* not set */
		if (check_message) {
			/*
			 * Perform a sanity check on the message
			 */
	
			SWLIB_ASSERT(strstr(check_message, script_name) != NULL);
	
			/*
			 * record the result
			 */
			script_status = swicol_rpsh_get_event_status(swi->swicolM, NULL, SW_CONTROL_SCRIPT_ENDS, this_index, &this_index);
			check_script->resultM = script_status;
			*p_check_status = script_status;
		} else {
			/*
			 * This is OK
			 * There must not have been a check script
			 */
			;
		}
	}
	strob_close(buf);
	return ret;
}

CISF_PRODUCT *
swpl_cisf_product_create(SWI_PRODUCT * product)
{
	CISF_PRODUCT * x;
	E_DEBUG("");
	x = (CISF_PRODUCT *)malloc(sizeof(CISF_PRODUCT));
	x->swiM = NULL;
	x->productM = NULL;

	swpl_cisf_init_base(&(x->cisf_baseM));
	(x->cisf_baseM).atM = swheader_state_create();
	(x->cisf_baseM).typeidM = CISF_ID_PRODUCT;
	(x->cisf_baseM).ixfileM = product->xfileM;
	/* FIXME, need to copy all of product->pbaseM */
	x->cisf_baseM.ixfileM->baseM.b_tagM = strdup(product->p_baseM.b_tagM);
	x->isetsM = vplob_open();
	x->cbaM = malloc(sizeof(CISFBA));
	x->cbaM->base_arrayM = vplob_open();
	return x;
}

void
swpl_cisf_init_base(CISF_BASE * x)
{
	x->ixfileM = NULL;
	x->atM = NULL;
	x->cf_indexM = -1;
	x->typeidM = -1;
}

CISF_FILESET *
swpl_cisf_fileset_create(void)
{
	CISF_FILESET * x;
	E_DEBUG("");
	x = (CISF_FILESET *)malloc(sizeof(CISF_FILESET));
	swpl_cisf_init_base(&(x->cisf_baseM));
	(x->cisf_baseM).atM = swheader_state_create();
	(x->cisf_baseM).typeidM = CISF_ID_FILESET;
	return x;
}

void
swpl_cisf_product_delete(CISF_PRODUCT * x)
{
	swheader_state_delete(x->cisf_baseM.atM);
	vplob_close(x->isetsM);
	free(x);
}

void
swpl_cisf_fileset_delete(CISF_FILESET * x)
{
	swheader_state_delete(x->cisf_baseM.atM);
	free(x);
}

void
swpl_cisf_init_single_single(CISF_PRODUCT * x, SWI * swi)
{
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	CISF_FILESET * xf;
	product = swi_package_get_product(swi->swi_pkgM, 0 /* The first product */);
	fileset = swi_product_get_fileset(product, 0);
	x->productM = product;
	x->cisf_baseM.cf_indexM = 0;
	x->cisf_baseM.ixfileM = product->xfileM;

	if ((xf=vplob_val(x->isetsM, 0)) == NULL) {
		xf = swpl_cisf_fileset_create();
		vplob_add(x->isetsM, xf);
	} else {
		;
	}
	xf->cisf_baseM.ixfileM = fileset;
	xf->cisf_baseM.cf_indexM = 0;
}

int
swpl_run_make_installed_live(GB * G, SWI * swi, int ofd, char * target_path)
{
	int ret;
	STROB * tmp;

	tmp = strob_open(32);
	swicol_set_task_idstring(swi->swicolM, SWBIS_TS_make_live_INSTALLED);
	strob_sprintf(tmp, 0,
		"cd \"%s\"\n"
		"case \"$?\" in\n"
		"	0)\n"
		"		sw_retval=0\n"
		"		;;\n"
		"	*)\n"
		"		sw_retval=1\n"
		"		;;\n"
		"esac\n"

		"case \"$sw_retval\" in\n"
		"	0)\n"
		"		mv -f "	SW_A__INSTALLED " " SW_A_INSTALLED "\n"
		"		sw_retval=$?\n"
		"		;;\n"
		"esac\n"
		"dd count=1 bs=512 of=/dev/null 2>/dev/null\n"
		,
		swi->swi_pkgM->catalog_entryM);

	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 
		512, 			/* stdin_file_size */
		target_path, 		/* directory to run in*/
		strob_str(tmp),  	/* the task script */
		SWBIS_TS_make_live_INSTALLED
	);

	if (ret != 0) {
		strob_close(tmp);
		return -1;
	}

	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret < 0) {
		strob_close(tmp);
		SWBIS_ERROR_IMPL();
		SWI_internal_error();
		return -1;
	}

	ret = swicol_rpsh_task_expect(swi->swicolM,
				G->g_swi_event_fd,
				SWICOL_TL_8 /*time limit*/);
	if (ret != 0) {
		strob_close(tmp);
		return -1;
	}
	strob_close(tmp);
	return 0;
}

int
swpl_run_check_overwrite(GB * G, SWI_FILELIST * fl, SWI * swi, int ofd, char * target_path, int fp_keep_fd)
{
	int ret;
	int retval;
	int keep_fd;
	STROB * tmp;
	STROB * stopbuf;
	STROB * payload;
	STROB * scriptbuf;
	int target_fd0;

	tmp = strob_open(32);
	stopbuf = strob_open(32);
	payload = strob_open(100);
	scriptbuf = strob_open(100);
	target_fd0 = G->g_target_fdar[0];

	retval = 0;

	swpl_print_file_list_to_buf(fl, payload);

	ret = swpl_make_here_document_source(G, tmp, payload);
	if (ret) retval++;

	swicol_set_task_idstring(swi->swicolM, SWBIS_TS_check_OVERWRITE);

	/* thses two subshells are closed by the
	   shell code generated in swlib_append_synct_eof() */
	strob_sprintf(scriptbuf,  STROB_DO_APPEND,
		"sw_retval=0\n"
		"(\n"
		"(\n"
	);
		
	strob_sprintf(scriptbuf,  STROB_DO_APPEND,
		"dd count=1 bs=512 of=/dev/null 2>/dev/null\n"
		"# result=\"\"\n"
		"IFS=\"`printf '\\n\\t'`\"\n"
		"for file in `\n"
		"%s"  /* this is the file list */
		"`\n"
		"do\n"
		"	if test -e \"$file\"; then\n"
		"		# result=exist\n"
		"		echo \"$file\"\n"
		"	fi\n"
		"#%s\n"
		"done\n"
		,
		strob_str(tmp),
		swevent_code(SW_FILE_EXISTS)
		);

	swlib_append_synct_eof(scriptbuf);

        swi->swicolM->needs_synct_eoaM = 1;
	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 
		512, 			/* stdin_file_size */
		target_path, 		/* directory to run in*/
		strob_str(scriptbuf),  	/* the task script */
		SWBIS_TS_check_OVERWRITE
	);
	swi->swicolM->needs_synct_eoaM = 0;

	if (ret != 0) {
		E_DEBUG("");
		strob_close(tmp);
		return -1;
	}

	E_DEBUG("");
	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret < 0) {
		E_DEBUG("");
		strob_close(tmp);
		SWBIS_ERROR_IMPL();
		SWI_internal_error();
		return -2;
	}

	/* Open a mem file */
	if (fp_keep_fd < 0)
		keep_fd = swlib_open_memfd();
	else
		keep_fd = fp_keep_fd;

	/* Now read the output from the target process */
	E_DEBUG("");
	ret = swlib_synct_suck(keep_fd, target_fd0);
	E_DEBUG("");

	if (ret < 0) {
		E_DEBUG2("swlib_synct_suck ret=%d", ret);
		retval = -3;
	}

	E_DEBUG("");
	uxfio_lseek(keep_fd, 0, SEEK_SET);
	swicat_squash_null_bytes(keep_fd);
	uxfio_lseek(keep_fd, 0, SEEK_SET);

	E_DEBUG("");

	/* Now reap the task shell events */
	ret = swicol_rpsh_task_expect(swi->swicolM,
				G->g_swi_event_fd,
				SWICOL_TL_500 /*time limit*/);
	E_DEBUG("");
	if (ret != 0) {
		E_DEBUG("");
		retval = -4;
	}

	if (fp_keep_fd < 0)
		swlib_close_memfd(keep_fd);

	E_DEBUG("");
	strob_close(tmp);
	strob_close(stopbuf);
	strob_close(payload);
	E_DEBUG("");
	return retval;
}

int
swpl_run_get_prelink_filelist(GB * G, SWICOL * swicol, int ofd, int * pfp_prelink_fd)
{
	int ret;
	int retval;
	int prelink_fd;
	STROB * tmp;
	STROB * scriptbuf;
	int target_fd0;
	char * target_path = "/";

	tmp = strob_open(32);
	scriptbuf = strob_open(100);
	target_fd0 = G->g_target_fdar[0];

	retval = 0;
	strob_sprintf(tmp,  STROB_DO_APPEND,
		"PATH=/usr/sbin:/usr/bin:/sbin:\"$PATH\"\n"
		"missing_which() {\n"
		"# Prefix: xxa\n"
		"xxa_pgm=\"$1\"\n"
		"xxa_name=`which $xxa_pgm 2>/dev/null`\n"
		"test -f \"$xxa_name\" -o -h \"$xxa_name\"\n"
		"case \"$?\" in\n"
		"0) echo \"$xxa_name\"; return 0; ;;\n"
		"*) echo \"/\"; return 1; ;;\n"
		"esac\n"
		"return 0\n"
		"}\n"
		"PRELINK=`missing_which prelink`\n"
		"case \"$?\" in\n"
		"0)\n"
		"\"$PRELINK\" -v -p |\n"
		"grep -v '^ ' |\n"
		"sed -e 's/[(\[].*//' |\n"
		"sed -e 's/:$//' |\n"
		"sed -e 's/[ \t]*$//' |\n"
		"sort |\n"
		"dd 2>/dev/null\n"
		";;\n"
		"*)\n"
		";;\n"
		"esac\n"
	);

	swicol_set_task_idstring(swicol, SWBIS_TS_Get_prelink_filelist);

	/* thses two subshells are closed by the
	   shell code generated in swlib_append_synct_eof() */
	strob_sprintf(scriptbuf,  STROB_DO_APPEND,
		"sw_retval=0\n"
		"(\n"
		"(\n"
	);
		
	strob_sprintf(scriptbuf,  STROB_DO_APPEND,
		/*
 		 * This script reads no data, but every script must
 		 * read at least one (1) block, here is that read
 		 */
		"dd count=1 bs=512 of=/dev/null 2>/dev/null\n"
		"IFS=\"`printf ' \\t\\n'`\"\n"
		"%s"  /* this is the script fragment to generate the prelink list */
		"\n"
		,
		strob_str(tmp)
		);

	/*
 	 * This routine closes the two sub-shell parentheses above
 	 */

	swlib_append_synct_eof(scriptbuf);

        swicol->needs_synct_eoaM = 1;
	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd, 
		512, 			/* stdin_file_size */
		target_path, 		/* directory to run in*/
		strob_str(scriptbuf),  	/* the task script */
		SWBIS_TS_Get_prelink_filelist
	);
	swicol->needs_synct_eoaM = 0;

	if (ret != 0) {
		E_DEBUG("");
		strob_close(tmp);
		*pfp_prelink_fd = -1;
		return -1;
	}

	E_DEBUG("");

	/*
 	 * send one (1) block of NULS (scripts that read no data read at least one block
 	 */
	ret = etar_write_trailer_blocks(NULL, ofd, 1);
	if (ret < 0) {
		E_DEBUG("");
		strob_close(tmp);
		SWBIS_ERROR_IMPL();
		SWI_internal_error();
		*pfp_prelink_fd = -1;
		return -2;
	}

	/*
	 * Open a mem file
	 */
	prelink_fd = swlib_open_memfd();

	/* 
 	 * Now read the output from the target process
 	 */
	E_DEBUG("");
	ret = swlib_synct_suck(prelink_fd, target_fd0);
	E_DEBUG("");

	if (ret < 0) {
		E_DEBUG2("swlib_synct_suck ret=%d", ret);
		retval = -3;
	}

	E_DEBUG("");
	uxfio_lseek(prelink_fd, 0, SEEK_SET);
	swicat_squash_null_bytes(prelink_fd);
	uxfio_lseek(prelink_fd, 0, SEEK_SET);

	E_DEBUG("");

	/*
 	 * Now reap the task shell events
 	 */
	ret = swicol_rpsh_task_expect(swicol,
				G->g_swi_event_fd,
				SWICOL_TL_500 /*time limit*/);
	E_DEBUG("");
	if (ret != 0) {
		E_DEBUG("");
		retval = -4;
	}

	E_DEBUG("");
	strob_close(tmp);
	E_DEBUG("");

	if (retval == 0) {
		*pfp_prelink_fd = prelink_fd;
	} else {
		swlib_close_memfd(prelink_fd);
		*pfp_prelink_fd = -1;
	}
	return retval;
}

void
swpl_agent_fail_message(GB *G, char * current_arg, int status)
{
	swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, &G->g_logspec, swc_get_stderr_fd(G),
		"SW_AGENT_INITIALIZATION_FAILED for target %s: status=%d\n",
		current_arg,
		status);
	if (status == 255) {
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, &G->g_logspec, swc_get_stderr_fd(G),
			"possible cause: invalid authentication credentials\n");
	} else if (status == 127) {
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, &G->g_logspec, swc_get_stderr_fd(G),
			"possible cause: the specified shell not found\n");
	} else if (status == 126) {
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, &G->g_logspec, swc_get_stderr_fd(G),
			"swbis is unable to operate because the specified or alternate shell program (sh)\n"
			"%s: conforming to POSIX 1003.1 at locations /bin/bash, /bin/ksh, /bin/mksh,\n"
			"%s: /usr/xpg4/bin/sh, or /bin/sh was not found (or found and not suitable).\n",
			swlib_utilname_get(),
			swlib_utilname_get());
	} else if (status == 124) {
		swlib_doif_writef(G->g_verboseG, SWC_VERBOSE_1, &G->g_logspec, swc_get_stderr_fd(G),
			"swbis is unable to operate because the shell found in PATH returned by\n"
			"%s: getconf was found not conforming to POSIX 1003.1\n",
			swlib_utilname_get());
	}
}
