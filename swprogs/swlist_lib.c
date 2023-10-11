/* swlist_lib.c -- swlist routines.

 Copyright (C) 2006,2007,2008,2010 James H. Lowe, Jr. 
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
#include "ls_list.h"

static
void
looper_abort(GB * G, SWICOL * swicol, int ofd)
{
	swpl_send_abort(swicol, ofd, G->g_swi_event_fd,  SWBIS_TS_Abort);
}

static
char *
get_attribute_from_installed(BLIST * BL, SWICAT_E * e, char * object, char * keyword)
{
	int parse_status;
	int swlex_inp;
	char * parser_line;
	SWHEADER_STATE state_wid;
	SWHEADER * isf_header;
	SWVERID * wid;

	E_DEBUG("");
	if (BL->isf_headerM) {
		isf_header = BL->isf_headerM;	
		swheader_store_state(isf_header, &state_wid);
		swheader_reset(isf_header);
	} else {
		swlex_inp = swlex_get_input_policy();
		swlex_set_input_policy(SWLEX_INPUT_UTF_TR);
		isf_header = swicat_e_isf_parse(e, &parse_status, NULL);
		swlex_set_input_policy(swlex_inp);
		if (parse_status)
			return NULL;
		swheader_store_state(isf_header, &state_wid);
		BL->isf_headerM = isf_header;
	}
	wid = swverid_open(object, (char*)(NULL));
	swverid_get_object_by_swverid(isf_header, wid,  (int*)NULL);
	parser_line = swheader_get_attribute_in_current_object(isf_header, keyword, NULL, NULL);
	swheader_restore_state(isf_header, &state_wid);
	return parser_line;
}

static
int
show_nopen(void)
{
	int ret;
	ret = open("/dev/null",O_RDWR);
	if (ret<0)
		fprintf(stderr, "fcntl error: %s\n", strerror(errno));
	close(ret);
	return ret;
}
	
static
void
write_attributes(BLIST * BL, STROB * buf, SWHEADER * swheader, int index)
{
	char * line;
	int level;
	char * keyword;
	char * value;

	E_DEBUG("");
	swheader_store_state(swheader, NULL);
	swheader_set_current_offset(swheader, index);
	while((line=swheader_get_next_attribute(swheader))) {
		E_DEBUG("");
		keyword = swheaderline_get_keyword(line);
		value = swheaderline_get_value(line, (int *)NULL);
		level = swheaderline_get_level(line);
		swdef_write_keyword_to_buffer(buf, keyword, level, SWPARSE_MD_TYPE_ATT);
		swdef_write_value_to_buffer(buf, value);
	}
	swheader_restore_state(swheader, NULL);
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

		"				tar cbf 1 - \"$catalog_entry_dir\" 2>/dev/null\n"
		"				sw_retval=$?\n"
		"				# echo \"here is the swspec string: [$swspec_string]\" 1>&2\n"

		"				rp_status=$sw_retval\n"
		"				# rp_status=8\n"
		"				shls_bashin2 \"" SWBIS_TS_report_status "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"				ret=$sw_retval\n"
		"				swexec_status=$sw_retval\n"
		"				case $sw_retval in 0) %s ;; *) ;; esac\n" /* clear to send */
	
		"				shls_bashin2 \"" SWBIS_TS_Catalog_unpack "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"
	
		"				shls_bashin2 \"" SWBIS_TS_retrieve_files_archive "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"
		
		"				case $swexec_status in\n"
		"			"	xSTR(SWP_RP_STATUS_NO_GNU_TAR) ") swexec_status=0\n"
		"				;;\n"
		"				esac\n"

		"				shls_bashin2 \"" SWBIS_TS_Catalog_dir_remove "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s;; *) sleep 2; shls_false_;; esac\n"
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

static
int
write_index_format_from_iscat(GB * G, SWI * swi, SWICAT_E * e, BLIST * BL, int ofd, int ls_verbose)
{
	int retval;
	int index;
	int parse_status;
	int swlex_inp;
	int ret;
	char * next_line;
	char * keyword;
	char * value;
	char * hline;
	char * dist_description;
	char * dist_title;
	
	SWHEADER_STATE info_state1;
	SWHEADER_STATE state1;
	SWHEADER_STATE state_dist_wid;
	SWHEADER * global_dist_index_header;
	SWHEADER * info_header;
	SWHEADER * isf_header;
	SWI_PRODUCT * current_product;
	SWI_XFILE * current_fileset;
	STROB * ls_line;
	STROB * buf;
	SWVERID * dist_wid;

	E_DEBUG("");
	retval = 0;
	ls_line = strob_open(60);
	buf = strob_open(60);

	/* FIXME: Eventually support multiple products and filesets  */

	swpl_enforce_one_prod_one_fileset(swi);

	/* Get the current product and fileset, for now, the first ones */

	current_product = swi_package_get_product(swi->swi_pkgM, 0 /* the first one */);
	current_fileset = swi_product_get_fileset(current_product, 0 /* the first one */);

	global_dist_index_header = swi_get_global_index_header(swi);

	/* Loop over the INFO file definitions, these are found
		in the SWHEADER for the fileset */

	info_header = swi_get_fileset_info_header(swi, 0, 0); /* FIXME */
	swheader_store_state(info_header, &info_state1);
	swheader_reset(info_header);

	/* Create the SWHEADER of the INSTALLED file
	*/
	swlex_inp = swlex_get_input_policy();
	swlex_set_input_policy(SWLEX_INPUT_UTF_TR);

	E_DEBUG("");
	isf_header = swicat_e_isf_parse(e, &parse_status, NULL);
	swlex_set_input_policy(swlex_inp);

	swheader_reset(global_dist_index_header);
	dist_wid = swverid_open(SW_A_distribution, (char*)(NULL));
	swverid_get_object_by_swverid(global_dist_index_header, dist_wid,  (int*)NULL);
	swheader_store_state(global_dist_index_header, &state_dist_wid);

	dist_description = swheader_get_attribute_in_current_object(global_dist_index_header, SW_A_description, NULL, NULL);
	swheaderline_write_to_buffer(buf,  dist_description);

	dist_title = swheader_get_attribute_in_current_object(global_dist_index_header, SW_A_title, NULL, NULL);
	swheaderline_write_to_buffer(buf,  dist_title);

	if (!isf_header) {
		E_DEBUG("HERE");
		sw_e_msg(G, "error parsing INSTALLED file\n");
		return -1;
	} 

	/* write the installed_software information in INDEX in format */
	
	E_DEBUG("");
	while ((next_line=swheader_get_next_object(isf_header, (int)UCHAR_MAX, (int)UCHAR_MAX))) {

		E_DEBUG2("next_line=%s", next_line);
		if (swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ) {
			/* Sanity check */
			SWBIS_IMPL_ERROR_DIE(1);
		}
		keyword = swheaderline_get_keyword(next_line);
		value = swheaderline_get_value(next_line, (int*)(NULL));
		swheaderline_write (next_line, ofd);
		strob_strcpy(buf, "");
		
		index = swheader_get_current_offset(isf_header);
		write_attributes(BL, buf, isf_header, index);
		uxfio_write(ofd, strob_str(buf), strob_strlen(buf));
       	}

	/* write the INFO in index format */

	while ((next_line=swheader_get_next_object(info_header, (int)UCHAR_MAX, (int)UCHAR_MAX))) {

		E_DEBUG2("next_line=%s", next_line);
		if (swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ) {
			/* Sanity check */
			SWBIS_IMPL_ERROR_DIE(1);
		}
		keyword = swheaderline_get_keyword(next_line);
		value = swheaderline_get_value(next_line, (int*)(NULL));
		swheaderline_write (next_line, ofd);
		strob_strcpy(buf, "");
		
		index = swheader_get_current_offset(info_header);
		write_attributes(BL, buf, info_header, index);
		uxfio_write(ofd, strob_str(buf), strob_strlen(buf));
       	}


	swheader_reset(global_dist_index_header);
	swheader_restore_state(info_header, &info_state1);
	strob_close(buf);
	strob_close(ls_line);
	return retval;
}

static
int
ls_fileset_from_iscat(SWI * swi, int ofd, int ls_verbose)
{
	int retval;
	SWHEADER_STATE state1;
	SWHEADER * indexheader;
	SWHEADER * infoheader;
	SWI_PRODUCT * current_product;
	SWI_XFILE * current_fileset;
	char * next_line;
	char * keyword;
	AHS * info_ahs2;
	int ret;
	STROB * ls_line;

	E_DEBUG("");
	retval = 0;
	info_ahs2 = ahs_open();
	ls_line = strob_open(60);

	/* FIXME: Eventually support multiple products and filesets  */

	swpl_enforce_one_prod_one_fileset(swi);

	/* Get the current product and fileset, for now, the first ones */

	current_product = swi_package_get_product(swi->swi_pkgM, 0 /* the first one */);
	current_fileset = swi_product_get_fileset(current_product, 0 /* the first one */);

	indexheader = swi_get_global_index_header(swi);

	/* Loop over the INFO file definitions, these are found
		in the SWHEADER for the fileset */

	infoheader = swi_get_fileset_info_header(swi, 0, 0); /* FIXME */
	swheader_store_state(infoheader, NULL);
	swheader_reset(infoheader);

        while ((next_line=swheader_get_next_object(infoheader, (int)UCHAR_MAX, (int)UCHAR_MAX))) {

		E_DEBUG2("next_line=%s", next_line);
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

		ahs_init_header(info_ahs2);
		swheader_store_state(infoheader, &state1);
		ret = swheader_fileobject2filehdr(infoheader, ahs_vfile_hdr(info_ahs2));
		if (ret) {
			SWLIB_FATAL("");
		}
		swheader_restore_state(infoheader, &state1);

		/*  Here is some example code for accessing and
			printing the file attributes */
		/******
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
		******/
		taru_print_tar_ls_list(ls_line, ahs_vfile_hdr(info_ahs2), ls_verbose);
		uxfio_unix_atomic_write(ofd, strob_str(ls_line), strob_strlen(ls_line));
        }

	swheader_restore_state(infoheader, NULL);
	ahs_close(info_ahs2);
	strob_close(ls_line);
	return retval;
}

static
void
write_annex_date_attribute (GB * G, SWICAT_E * e, BLIST * BL, char * cl_target_target, int attr_index, char * installed_kw, char * attr_name)
{
	char * line;
	char * value;
	unsigned long int uli;
	char buf[30];
	int result;
	time_t tm;

	E_DEBUG("");
	line = get_attribute_from_installed(BL, e, SW_A_product, installed_kw);
	if (!line) goto error_out;
	value = swheaderline_get_value(line, (int *)NULL);
	if (!value) goto error_out;
    	uli = swlib_atoul(value, &result, (char**)NULL);
	tm = (time_t)uli;
	/* ctime_r(&tm, buf); */
        strncpy(buf, asctime(localtime(&tm)), sizeof(buf) - 1);
        buf[sizeof(buf)-1] = '\0';
	swlib_tr(buf, (int)'\0', (int)'\r');
	swlib_tr(buf, (int)'\0', (int)'\n');
	swlist_write_attr2(BL, cl_target_target, attr_name, buf, attr_index);
	E_DEBUG("");
	return;
	error_out:
	sw_e_msg(G, "error parsing INSTALLED file while searching for attribute: %s\n", SW_A_create_time);

}

static
int
write_annex_attributes (GB * G, SWICAT_E * e, BLIST * BL, char * cl_target_target, SWICAT_SR * sr)
{
	int attr_index;
	char * attr;
	char * line;
	char * value;

	attr_index = 0;
	while ((attr=swlist_blist_get_next_attr(BL, attr_index, &attr_index)) != NULL) {
		if (strcmp(attr, SW_A_create_date) == 0) {
			write_annex_date_attribute (G, e, BL, cl_target_target, attr_index, SW_A_create_time, SW_A_create_date);
		} 

		if (strcmp(attr, SW_A_mod_date) == 0) {
			write_annex_date_attribute (G, e, BL, cl_target_target, attr_index, SW_A_mod_time, SW_A_mod_date);
		} 

		if (strcmp(attr, SW_A_software_spec) == 0) {
			swlist_write_attr2(BL, cl_target_target, SW_A_software_spec, swverid_print(sr->swspecM, NULL), attr_index);
		} 
		attr_index++;
	}
	return 0;
}

int
swlist_is_annex_attribute(char * name)
{
	if (
		strcmp(name, SW_A_mod_date) &&
		strcmp(name, SW_A_create_date) &&
		strcmp(name, SW_A_software_spec)
	)
		return 0;
	else
		return 1;
}


void
swlist_write_attr2(BLIST * BL, char * host, char * name, char * value, int attr_index)
{
	static STROB * buffer;

	if (!buffer) buffer = strob_open(20);
	if (! value) value = "\"\"";
	if (swlist_blist_level_is_set(BL, SW_LEVEL_V_HOST)) {
		swlib_writef(STDOUT_FILENO, buffer, "%s: %s: %s\n", host, name, value);
	} else {
		swlib_writef(STDOUT_FILENO, buffer, "%s: %s\n", name, value);
	}
	swlist_blist_mark_as_processed(BL, attr_index);
}

int
swlist_write_source_copy_script2(GB * G,
	int ofd, 
	char * sourcepath, 
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
	char * pgm_mode;
	char * dirname;
	char * basename;
	char * pax_write_command;
	char * opt_force_lock;
	char * opt_allow_no_lock;
	char * xx;
	STROB * buffer;
	STROB * buffer_new;
	STROB * shell_lib_buf;
	STROB * is_archive_msg;
	STROB * is_directory_msg;
	STROB * subsh;
	STROB * subsh2;
	STROB * tmp;
	STROB * set_vx;
	STROB * to_devnull;
	STROB * isc_msg;
	STROB * locked_region;
	STROB * looper_routine;
	int vlv;
	char * debug_task_shell;
	
	pgm_mode = G->g_gpufieldM;
	basename = (char*)NULL;
	dirname = (char*)NULL;
	buffer = strob_open(1);
	buffer_new = strob_open(1);
	to_devnull = strob_open(10);
	is_archive_msg = strob_open(1);
	is_directory_msg = strob_open(1);
	set_vx = strob_open(10);
	tmp = strob_open(10);
	subsh = strob_open(10);
	subsh2 = strob_open(10);
	isc_msg = strob_open(10);
	shell_lib_buf = strob_open(10);
	locked_region = strob_open(100);
	looper_routine = strob_open(100);

	vlv = G->g_verboseG;
	if (G->g_do_task_shell_debug == 0) {
		debug_task_shell="";
	} else {
		debug_task_shell="x";
	}

	/* Sanity checks on sourcepath */
	if (
		strstr(sourcepath, "..") == sourcepath ||
		strstr(sourcepath, "../") ||
		swlib_is_sh_tainted_string(sourcepath) ||
		0
	) { 
		return 1;
	}
	swlib_squash_double_slash(sourcepath);

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

	/* Split sourcepath into a basename and leading direcctory parts */
	
	/* leading directories */
	swlib_dirname(tmp, sourcepath);
	dirname = strdup(strob_str(tmp));
	if (swlib_is_sh_tainted_string(dirname)) { return 1; }

	/* basename which may be a directory */
	swlib_basename(tmp, sourcepath);
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

	/* Determine what archive writing utility to use */
	
	pax_write_command = swc_get_pax_write_command(G->g_pax_write_commands,
						pax_write_command_key,
						G->g_verboseG, DEFAULT_PAX_W);


	/* Make a nice message for the SOURCE_ACCESS_BEGINS event when
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
	/* ret = swlib_writef(ofd, buffer,  */
									/* ism_begin */
		"IFS=\"`printf ' \\t\\n'`\"\n"
		"trap '/bin/rm -f ${LOCKPATH}.lock; exit 1' 1 2 15\n"
		"echo " SWBIS_TARGET_CTL_MSG_125 ": " KILL_PID "\n"
		"echo " SWBIS_TARGET_CTL_MSG_129 ": \"`pwd | head -1`\"\n"
		"export LOCKENTRY\n"
		CSHID
		"LOCKPATH=\"\"\n"
		"%s\n"  			/* SEVENT: SW_SESSION_BEGINS */
		"%s" 				/* swicol_subshell_marks */
		"wcwd=\"`pwd`\"\n"
		"export lock_did_lock\n"
		"export opt_force_lock\n"
		"lock_did_lock=\"\"\n"
		"opt_force_lock=\"%s\"\n"
		"opt_allow_no_lock=\"%s\"\n"
		"export PATH\n"
		"PATH=`getconf PATH`:$PATH\n"
		"export swutilname\n"
		"swutilname=swlist\n"
		"%s\n"			/* shls_bashin2 from shell_lib.sh */
		"%s\n"			/* shls_false_ from shell_lib.sh */
		"%s\n"			/* shls_looper from  shell_lib.sh */
		"%s\n"			/* shls_looper_payload from shell_lib.sh */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* shls_make_dir_absolute */
		"%s" 			/* set statement for verbosity */
		"pgm_mode=\"%s\"\n"
		"blocksize=\"%s\"\n"
		"dirname=\"%s\"\n"
		"basename=\"%s\"\n"
		"targetpath=\"%s\"\n"
		"sourcepath=\"%s\"\n"

		"#\n"
		"# make sourcepath absolute\n"
		"#\n"
		"case \"$sourcepath\" in\n"
		"/*)\n"
		";;\n"
		"*)\n"
		"mda_pwd=\"$wcwd\"\n"
		"mda_target_path=\"$sourcepath\"\n"
		"shls_make_dir_absolute\n"
		"sourcepath=\"$mda_target_path\"\n"
		"# sourcepath is now absolute\n"
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
		"# END of code to make absolute\n"
		"#\n"

		"sw_targettype=unset\n"
		"sw_retval=0\n"
		"sb__delaytime=%d\n"
		"export sh_dash_s\n"
		"d_sh_dash_s=\"%s\"\n"
		"case \"$5\" in PSH=*) eval \"$5\";; *) unset PSH ;; esac\n"
		"sh_dash_s=\"${PSH:=$d_sh_dash_s}\"\n"
		"#sh_dash_s=\"/bin/bash -s\"\n"
		"debug_task_shell=\"%s\"\n"
		"export LOCKPATH\n"
		"#\n"
		"%s\n"    /* <<<--- shls_looper_payload */
		"swexec_status=0\n"

		/* Here, in this first if-then statement we will classify and
		   type-test the target path */
									/* ism_test_begin */
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

		/* Here is where the real work begins, the target has been
			typed and tested, the management host has been
			notified via the "is_archive_msg" and "is_directory_msg"
			and the $sw_targettype var has been set.  */

		CSHID
		"case \"$sw_targettype\" in\n"
		"	\"regfile\")\n"
				/* Just dd the file back to management host */
		"		%s\n"
		"		%s\n"
		"		dd ibs=\"$blocksize\" obs=512 if=\"$sourcepath\" %s;\n"
		"		sw_retval=$?\n"
		"		%s\n"
		"		;;\n"
		"	\"dir\")\n"
				/* This case is, for swlist, the case for
				   querying installed software located at
				   <dirname>/<basename> */
		"		%s\n"
		"		%s\n"
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

		"shls_bashin2 \"" SWBIS_TS_Get_iscs_listing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Get_iscs_listing  ";; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		"shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Do_nothing ";; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		"		case \"$pgm_mode\" in\n"
		"		" SWLIST_PMODE_FILE "|" SWLIST_PMODE_INDEX "|" SWLIST_PMODE_ATT ")\n"	
		"			shls_looper \"$sw_retval\"\n"
		"			sw_retval=$?\n"
		"			swexec_status=$sw_retval\n"
		"			shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"			sw_retval=$?\n"
		"			case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"			sw_retval=$?\n"
		"			swexec_status=$sw_retval\n"
		"			;;\n"
		"		esac\n"
		"		%s\n"
		"		;;\n"
		"	\"unset\")\n"
					/* This is an error */
		"		sw_retval=1\n"
		"		;;\n"
		"	*)\n"
					/* This is an error */
		"		sw_retval=1\n"
		"		;;\n"
		"esac\n"

		"if test \"$sw_retval\" != \"0\"; then\n"
		"	 sb__delaytime=0;\n"
		"fi\n"
		"sleep \"$sb__delaytime\"\n"
		"%s\n"
		"%s\n"
		"%s\n"
		,
									/* ism_begin */
/*_% */		SEVENT(2, vlv, SW_SESSION_BEGINS, ""),
/*_% */		swicol_subshell_marks(subsh, "install_target", 'L', nhops, vlv),
/*_% */		opt_force_lock,
/*_% */		opt_allow_no_lock,
/*_% 5 */	shlib_get_function_text_by_name("shls_bashin2", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_false_", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_looper", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_looper_payload", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("lf_make_lockfile_name", shell_lib_buf, NULL),
/*_% 10 */	shlib_get_function_text_by_name("lf_make_lockfile_entry", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("lf_test_lock", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("lf_remove_lock", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_make_dir_absolute", shell_lib_buf, NULL),
/*_% */		strob_str(set_vx),
/*_% */		pgm_mode,
/*_% */		blocksize,
/*_% */		dirname,
/*_% */		basename,
/*_% */		sourcepath,
/*_% */		sourcepath,
/*_% 19 */	delaytime,
		swc_get_default_sh_dash_s(G),
/*_% */		debug_task_shell,					/* debug_task_shell */	
/*_% */		strob_str(looper_routine),
									/* ism_test_begin */
									/* ism_d1 */
/*_%*/		strob_str(is_directory_msg),
									/* ism_a1 */
/*_% */		strob_str(is_archive_msg),
									/* access_error */
/*_% */		SEVENT(2, vlv, SW_SOURCE_ACCESS_ERROR, basename),
/*_% */		SEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, basename),
									/* chdir_failed */
/*_% */		SEVENT(2, vlv, SW_SOURCE_ACCESS_ERROR, dirname),
/*_% */		SEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, dirname),
									/* ism_else_fail */
/*_% */		SEVENT(2, vlv,  SW_SOURCE_ACCESS_ERROR, sourcepath),
/*_% */		SEVENT(1, -1, SW_SOURCE_ACCESS_ERROR, sourcepath),

								/* case: sw_targettype=regfile */
/*_% */		SEVENT(1, -1, SW_SOURCE_ACCESS_BEGINS, ""),
/*_% */		SEVENT(2, vlv, SW_SOURCE_ACCESS_BEGINS, ""),
/*_% */		strob_str(to_devnull),
/*_% */		SEVENT(2, vlv, SW_SOURCE_ACCESS_ENDS, "status=0"),

								/* case: sw_targettype=dir */

/*_% */		SEVENT(1, -1, SW_SOURCE_ACCESS_BEGINS, strob_str(isc_msg)),
/*_% */		SEVENT(2, vlv, SW_SOURCE_ACCESS_BEGINS, strob_str(isc_msg)),
/*_% */		SEVENT(2, vlv, SW_SOURCE_ACCESS_ENDS, "status=$sw_retval"),
									/* ism_end */
/*_% */		SEVENT(2, -1, SWI_MAIN_SCRIPT_ENDS, "status=0"),
/*_% */		SEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
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
	strob_close(buffer);
	strob_close(buffer_new);
	strob_close(is_archive_msg);
	strob_close(is_directory_msg);
	strob_close(set_vx);
	strob_close(to_devnull);
	strob_close(subsh);
	strob_close(subsh2);
	strob_close(isc_msg);
	strob_close(shell_lib_buf);
	strob_close(locked_region);
	strob_close(looper_routine);
	/*
	 * 0 is OK
	 * !0 is error
	 */
	return !(ret > 0);
}

int
swlist_has_annex_attribute(BLIST * BL)
{
	char * attr;
	int attr_index;
	
	E_DEBUG("");
	attr_index = 0;
	while ((attr=swlist_blist_get_next_attr(BL, attr_index, &attr_index)) != NULL) {
		if (swlist_is_annex_attribute(attr)) {
			E_DEBUG("returning 1");
			return 1;
		}
		attr_index++;
	}
	E_DEBUG("returning 0");
	return 0;
}

SWI *
swlist_list_create_swi(GB * G,
	struct extendedOptions * opta,
	VPLOB * swspecs,
	char * target_path
	)
{
	int ret;
	SWI * swi;
	int dup_of_target_fdar0;

	swi = swi_create();
	swi->verboseM = G->g_verboseG;
	swi_set_utility_id(swi, SWC_U_L);
	dup_of_target_fdar0 = dup(G->g_target_fdar[0]);
	ret = swi_do_decode(swi,
		G->g_swlog,
		G->g_nullfd,  /* target fd */
		dup_of_target_fdar0,  /* source fd */
		target_path,
		NULL /* source_path */,
		swspecs,
		G->g_target_terminal_host,
		opta,
		G->g_is_seekable,
		G->g_do_debug_events,
		G->g_verboseG,
		&G->g_logspec,
		-1 /* open flags, use default */);

	if (ret != 0) {
		return NULL;
	}
	return swi;
}

int
swlist_list_distribution_archive(
	GB * G,
	struct extendedOptions * opta,
	VPLOB * swspecs,
	char * target_path,
	int target_nhops,
	int efd
	)
{
	SWI * swi;
	int dup_of_target_fdar0;
	int ret;
	int retval;
	SWI_DISTDATA * distdataO;
	char * tmp_s;
	char * pax_read_command_key = "tar";
	FILE * fver;

	E_DEBUG("");
	fver = G->g_vstderr;
	retval = 0;
	swi = swi_create();
	swi_set_utility_id(swi, SWC_U_L);
	distdataO = swi_distdata_create();

	E_DEBUG("into  swi_do_decode");
	dup_of_target_fdar0 = dup(G->g_target_fdar[0]);
	ret = swi_do_decode(swi,
		G->g_swlog,
		G->g_nullfd,  /* target fd */
		dup_of_target_fdar0,  /* source fd */
		target_path,
		NULL /* source_path */,
		swspecs,
		G->g_target_terminal_host,
		opta,
		G->g_is_seekable,
		G->g_do_debug_events,
		G->g_verboseG,
		&G->g_logspec,
		-1 /* open flags, use default */);

	E_DEBUG("");

	if (ret) {
		E_DEBUG("");
		return -1;
	}

	E_DEBUG("");
	G->g_xformat = swi->xformatM;

	swi->opt_alt_catalog_rootM = G->g_opt_alt_catalog_root;
	swi->swi_pkgM->target_pathM = strdup(target_path);
	swi->swi_pkgM->installed_software_catalogM =
		strdup(get_opta_isc(opta, SW_E_installed_software_catalog));

	if (swlib_check_clean_path(swi->swi_pkgM->installed_software_catalogM)) {
		SWLIB_FATAL("tainted path");
	}
		
	swicol_set_targetpath(swi->swicolM, target_path);
	swicol_set_delaytime(swi->swicolM, SWC_SCRIPT_SLEEP_DELAY);
	swicol_set_nhops(swi->swicolM, target_nhops);
	swicol_set_verbose_level(swi->swicolM, G->g_verboseG);

	/* Validate some basic information about the package.  */
	ret = swi_distdata_resolve(swi, distdataO, 1 /*enforce swinstall policy*/);
	if (ret) {
		swlib_doif_writef(G->g_verboseG,
			G->g_fail_loudly,
			&G->g_logspec, swc_get_stderr_fd(G),
			"error decoding source INDEX file: swbis_code=%d\n", ret);

			/* Close and return */
			G->g_xformat = NULL;
			swi_delete(swi);
			return -2;
	}

	swi->distdataM = distdataO;

	if (G->g_verboseG >= SWC_VERBOSE_8) {
		tmp_s = swi_dump_string_s(swi, "swlist: (SWI*)");
		fprintf(fver, "%s\n", tmp_s);
	}

	if (G->g_swi_debug_name) {
		swc_write_swi_debug(swi, G->g_swi_debug_name);
	}

	/* Use the swinstall routine setup in "preview mode"
	   to list the distribution */


	if (0) {
		/* dead code */
		E_DEBUG("");
		/* This lists from the storage section */
		/* 
		ret = swinstall_arfinstall(G,
				swi,
				G->g_nullfd,
				efd, / * G->g_t_efd, * /
				&G->g_signal_flag,
				target_path,
				"",
				swspecs,
				SWI_SECTION_BOTH,
				-1,
				1, / * opt_preview * /
				pax_read_command_key,
				G->g_opt_alt_catalog_root,
				G->g_save_fdar[2],
				opta,
				&G->g_logspec,
				0, / *keep_old_files* /
				NULL / * prorgess meter * /);
		*/	
		E_DEBUG2("swinstall_arfinstall returned %d", ret);
		if (ret) retval = -3;
	} else {
		/* This lists from the catalog INFO file */
		ret = ls_fileset_from_iscat(swi,
				STDOUT_FILENO,
				swpl_determine_tar_listing_verbose_level(swi));
		if (ret) retval = -3;
	}


	/* We need to exhaust the file descriptor or dd(1) in the target script will 
	   with a non-zero exit value */
			 
	swlib_pipe_pump(G->g_nullfd,  dup_of_target_fdar0);

	G->g_xformat = NULL;
	swi_delete(swi);
	E_DEBUG("");
	return retval;
}

extern struct blist_level *  blist_levels_i;

void
blist_init(BLIST * blist)
{
	blist->attr_levelsM = strar_open();
	blist->attributesM = strar_open();
	blist->attr_flagsM = strob_open(12);
	blist->do_get_create_dateM = 0;
	blist->do_get_mod_dateM = 0;
	blist->do_get_software_specM = 0;
	blist->catalogM = NULL;	
	blist->levelsM = blist_levels_i; /* from swlist.h */
	blist->index_format_is_setM = 0;
	blist->level_is_setM = 0;
	blist->has_soc_attributesM = 0;
	blist->has_uts_attributesM = 0;
	blist->has_annex_attributesM = 0;
	blist->isf_headerM = NULL;
}

int
swlist_looper_sr_payload(GB * G, BLIST * BL, char * target_path, char * cl_target_target, SWICOL * swicol,
		SWICAT_SR * sr, int ofd, int ifd, int * p_rp_status,
		SWUTS * uts, char * pax_write_command_key, int wopt_file_sys, char * wopt_pgm_mode)
{
	int retval;
	int rstatus;
	int ret;
	int do_skip_entry;
	int ls_verbose;
	int result;
	int sig_level;
	int swlex_inp;
	char * pax_read_command;
	char * epath;
	char * installed_software_catalog;
	char * catalog_entry_directory;
	STROB * tmp;
	STROB * btmp;
	STROB * btmp2;
	STROB * swspec_string;
	SWICAT_E * ee;
	SWI * swi;
	SWI_FILELIST * file_list;
	SWGPG_VALIDATE * swgpg;
	int archive_fd;
	int no_permission = 0;
	
	archive_fd = -1;
	do_skip_entry = 0;
	retval = 0;
	rstatus = 0;
	tmp = strob_open(100);
	btmp = strob_open(100);
	btmp2 = strob_open(100);
	swspec_string = strob_open(100);
	swi = NULL;
	file_list = NULL;
	swgpg = NULL;
	ee = NULL;

	sig_level = swlib_atoi(get_opta(G->optaM, SW_E_swbis_sig_level), &result);
	if (result) {
		sig_level = 0;
	}

	pax_read_command = swc_get_pax_read_command(G->g_pax_read_commands,
		"tar", G->g_verboseG >= SWC_VERBOSE_3,
		0 /*keep_old_files*/, DEFAULT_PAX_R);
	E_DEBUG("");
	installed_software_catalog = get_opta_isc(G->optaM, SW_E_installed_software_catalog);
	catalog_entry_directory = swicat_sr_form_catalog_path(sr, installed_software_catalog, NULL);

	E_DEBUG2("catalog_entry_directory = [%s]", catalog_entry_directory);

	swicat_sr_form_swspec(sr, swspec_string);

	E_DEBUG("");
	E_DEBUG2("wopt_pgm_mode is %s", wopt_pgm_mode);
	*p_rp_status = 0;
	E_DEBUG("setting *p_rp_status = 0");
	if (strlen(catalog_entry_directory) == 0) {
		/* return with no error
		   this happens for `the empty response` to a query */
		E_DEBUG("error");
		return 1;
	}

	/* fprintf(stderr, "ENTRY: [%s]\n", catalog_entry_directory); */

	strob_sprintf(btmp, 0, "%s\n", catalog_entry_directory);
	E_DEBUG2("servicing looper: writing for shls_looper_payload arg0: [%s]", strob_str(btmp));

	/* write the catalog entry directory which becomes arg1 to
	   the shls_looper_payload() routine */

	E_DEBUG("");
	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
			ofd,
			strob_str(btmp),
			strob_strlen(btmp)
			);
					
	if (swicol_get_master_alarm_status(swicol) != 0 ||
		 ret != (int)strob_strlen(btmp)
	) {
		/* error */
		E_DEBUG("error");
		E_DEBUG2("ret != (int)strob_strlen(btmp) = [%d]", ret != (int)strob_strlen(btmp));
		sw_e_msg(G, "error from atomicio: ret=%d\n", ret);
		return -1;
	}

	/* here is a gratuitous task shell that does nothing */

	E_DEBUG("");
	ret = swpl_send_success(swicol, ofd, G->g_swi_event_fd,
			SWBIS_TS_check_loop);
	if (ret != 0) {
		sw_e_msg(G, "error from swpl_send_success()\n");
		E_DEBUG("error");
		rstatus = 1;
		retval = -1;  /* protocol internal error */
		goto error_out;
	}

	/* Make a session lock */

	E_DEBUG2("target_path=%s", target_path);
	ret = swpl_session_lock(G, swicol, target_path, ofd, G->g_swi_event_fd);
	E_DEBUG2("swpl_session_lock: ret=%d", ret);
	sw_d_msg(G, "swpl_session_lock returned [%d]\n", ret);

	E_DEBUG2("swpl_session_lock returned %d", ret);
	swlib_squash_trailing_vnewline(strob_str(btmp));	
	if (ret < 0) {
		/* Internal error */
		E_DEBUG2("error ret=%d", ret);
		sw_d_msg(G, "swpl_session_lock: lock fail for %s, ret=%d\n", strob_str(btmp), ret);
		sw_e_msg(G, "error from swpl_session_lock: status=%d\n", ret);
		rstatus = 1;
		retval = -4;
		goto error_out;
	} else if (ret > 0) {
		/* session in progress, or no access to make lock */
		E_DEBUG2("error ret=%d", ret);
		sw_e_msg(G, "swpl_session_lock lock failed for %s, ret=%d\n", strob_str(btmp), ret);
		/* sw_d_msg(G, "swpl_session_lock lock failed for %s, ret=%d\n", strob_str(btmp), ret); */
		rstatus = SWP_RP_STATUS_NO_LOCK;
		retval = 1;
		goto error_out;
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

	E_DEBUG("");
	swgp_read_line(ifd, btmp2, DO_APPEND);
	if (G->devel_verboseM)
		fprintf(stderr, "rp_status is %d\n", ret);

	/* Create the catalog entry control structure */

	E_DEBUG("");
	ee = swicat_e_create();

	/* Now read an archive of the installed catalog entry on
	   file descriptor ifd */

	E_DEBUG("");
	ret = swicat_e_open_entry_tarball(ee, ifd);
	if (ret != 0) {
		sw_e_msg(G, "error opening catalog entry tarball, ret=%d\n", ret);
		rstatus = 1;
		retval = -2;  /* protocol internal error */
		looper_abort(G, swicol, ofd);
		goto error_out;
	}

	/* ee->entry_prefixM contains the installed_software_catalog
	  attribute already prefixed */

	E_DEBUG("");
	epath = swicat_e_form_catalog_path(ee, btmp, NULL, SWICAT_ACTIVE_ENTRY);
	if (G->devel_verboseM)
		SWLIB_INFO2("catalog entry (active) path = %s", epath);
	
	E_DEBUG("");
	epath = swicat_e_form_catalog_path(ee, btmp, NULL, SWICAT_DEACTIVE_ENTRY);
	if (G->devel_verboseM)
		SWLIB_INFO2("catalog entry path (inactive) path = %s", epath);

	E_DEBUG("");
	epath = swicat_e_form_catalog_path(ee, btmp, NULL, SWICAT_ACTIVE_ENTRY);

	/* epath is now a clean relative path to such as:
		var/lib/swbis/catalog/emacs/emacs/21.2.1/0  */

	E_DEBUG("");
	ret = swicat_e_reset_fd(ee);
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
	
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	E_DEBUG("");
	swgpg = swgpg_create();
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG("");

	ret = 0;
	if (sig_level >= 0)
		ret = swicat_e_verify_gpg_signature(ee, swgpg);
	if (ret != 0) {
		rstatus = 3;
		retval = 0;
		sw_e_msg(G, "error from swicat_e_verify_gpg_signature, ret=%d\n", ret);
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	/* Now interpret the verification results according to extended option
	   requirements */

	E_DEBUG("");
	ret = 0;
	if (sig_level >= 0)
		ret = swpl_signature_policy_accept(G, swgpg, (G->g_verboseG > 0) ? G->g_verboseG-1:0, strob_str(swspec_string));
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG2("ret=%d", ret);
	if (ret != 0) {
		/* Bad signature or not enough good signatures
		   Can't use the blob to make a removal file list
		   because were assuming its tainted. */
		E_DEBUG("sig level not satisfied");
		rstatus = SWP_RP_STATUS_NO_INTEGRITY;
		retval = 0;
		do_skip_entry = 1;
	} else {
		/* Signatures OK, or don't care */
		;
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	/*
	 * If we're here its OK to decode the catalog.tar file to create (SWI*)swi
	 * because its authenticated, or we don't care.
	 */

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG("");

	swlex_inp = swlex_get_input_policy();
	E_DEBUG("");
	swlex_set_input_policy(SWLEX_INPUT_UTF_TR);
	E_DEBUG("");

	swi = swicat_e_open_swi(ee);
	E_DEBUG("");
	swlex_set_input_policy(swlex_inp);
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	if (swi == NULL) {
		/* swi might be NULL due to file system permission,
		   handle NULL gracefully */
		do_skip_entry = 1;
		no_permission = SWP_RP_STATUS_NO_PERMISSION;
		sw_e_msg(G, "catalog read error, possible permission denied: %s/%s\n", target_path, epath);
	} else {
		swi->verboseM = G->g_verboseG;
		swi_set_utility_id(swi, SWC_U_L);
		swi->swi_pkgM->catalog_entryM = strdup(epath); /* FIXME, this must set explicitly */
		swi->swi_pkgM->target_pathM = strdup(target_path); /* FIXME, this must set explicitly */
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());


	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	if (swi) swi->optaM = G->optaM;
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	error_out_and_report:

	/* SWBIS_TS_report_status */

	/* The purpose of this is solely to provide an opportunity for
	the remote script to report a problem and to re-verify script-data
	sychronization. */
	
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	ret = swpl_report_status(swicol, ofd, G->g_swi_event_fd);
	if (ret) {
		E_DEBUG("error at TS_report_status");
		rstatus = 6;
		retval = -4;
		goto error_out;
	}
	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	/* wait for clear-to-send */

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	ret = swicol_rpsh_wait_cts(swicol, G->g_swi_event_fd);
	if (ret) {
		E_DEBUG("error");
		rstatus = -1;
		retval = -5;
		goto error_out;
	}

	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	/* Make the file list for the product from the catalog metadata */
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	if (swi) { 
		file_list = swicat_e_make_file_list(ee, uts, swi);
	} else {
		E_DEBUG("file_list is NULL");
		file_list = NULL;
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	if (swi && file_list == NULL) {
		E_DEBUG("file_list is NULL");
		rstatus = 7;
		retval = 0;
		do_skip_entry = 1;
	}
	
	if (
		G->g_opt_previewM  ||
		G->g_verboseG > SWC_VERBOSE_3 ||
		G->devel_verboseM ||
		0
	) {
		if (file_list) swpl_show_file_list(G, file_list);
	}

	if (G->devel_verboseM)
		SWLIB_INFO2("%s", strob_str(btmp));

	skip:

	/* SWBIS_TS_Catalog_unpack */
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		E_DEBUG("skipping at swpl_unpack_catalog_tarfile");
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
		E_DEBUG("at swpl_unpack_catalog_tarfile");
		ret = swpl_unpack_catalog_tarfile(G, swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd);
		if (ret != 0) {
			E_DEBUG("error in swpl_unpack_catalog_tarfile");
			rstatus = -3;
			retval = -9;
			goto error_out;
		}
	}

	/* SWBIS_TS_retrieve_files_archive */
	ls_verbose = G->g_verboseG > SWC_VERBOSE_1 ? LS_LIST_VERBOSE_L1 : LS_LIST_VERBOSE_L0;
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_FILE) != 0 ||
		0
	) {
		E_DEBUG("NOT running swpl_retrieve_files");
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_retrieve_files_archive,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			E_DEBUG("");
			rstatus = -4;
			retval = -10;
			goto error_out;
		}
	} else {
		E_DEBUG("running swpl_retrieve_files");
		/* archive_fd = STDOUT_FILENO; */
		archive_fd = G->g_nullfd;
		SWLIB_ASSERT(archive_fd > 0);

		E_DEBUG("");
		if (wopt_file_sys) {
			/* List files as installed in the file system */
			FILE_DIGS * digs;
			digs = taru_digs_create();
			taru_digs_init(digs, DIGS_ENABLE_ON, 0);

			ret = swpl_retrieve_files(G, swi, swicol, ee, file_list,
				ofd, ifd, archive_fd,
				pax_write_command_key,
				STDOUT_FILENO,
				ls_verbose, digs);
		} else {
			ret = swpl_retrieve_files(G, swi, swicol, ee, file_list,
				ofd, ifd, archive_fd,
				pax_write_command_key,
				-1, -1, (FILE_DIGS*)NULL);
		}
		E_DEBUG2("LOWEST FD=%d", show_nopen());
		E_DEBUG("");
		if (ret < 0) {
			E_DEBUG("");
			rstatus = 8;
			retval = -11;
			goto error_out;
		} else if (ret > 0) {
			if (ret == SWP_RP_STATUS_NO_GNU_TAR) {
				sw_e_msg(G, "GNU tar or pax is required for this operation but appears missing.\n");
			}
			rstatus = 9;
		}
	}
	E_DEBUG("");
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG2("wopt_pgm_mode is %s", wopt_pgm_mode);

	if (
		swlist_has_annex_attribute(BL) && /* has create_date create_time or software_spec */
		BL->index_format_is_setM == 0
		/* swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_INDEX) != 0 */
	) {
		/* annex attributes and not '-v' mode */
		write_annex_attributes (G, ee, BL, cl_target_target, sr);
		ret = 0;

	} else if (swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_INDEX) == 0) {
		E_DEBUG2("LOWEST FD=%d", show_nopen());
		E_DEBUG("SWLIST_PMODE_INDEX");
		/* sw_e_msg(G, "software definition file format (-v mode) mode unsupported\n"); */

		ret = write_index_format_from_iscat(G, swi, ee, BL, STDOUT_FILENO, ls_verbose);
		if (ret != 0) {
			sw_e_msg(G, "error writing index format\n");
		}
	} else if (swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_FILE) == 0) {
		E_DEBUG2("LOWEST FD=%d", show_nopen());
		E_DEBUG("SWLIST_PMODE_FILE");
		if (wopt_file_sys) {
			E_DEBUG("List files from file system (above)");
			/* see swpl_retrieve_files() invocation above */
			;
		} else {
			/* List file from the installed catalog */
		
			/* swpl_show_file_list(G, file_list); */
			if (do_skip_entry == 0) {
				E_DEBUG("List files from installed catalog");
				ret = ls_fileset_from_iscat(swi, STDOUT_FILENO, ls_verbose);
			} else {
				;
				E_DEBUG("skipping listing files from installed catalog");
			}
		}
	} else if (swpl_test_pgm_mode(wopt_pgm_mode, SWLIST_PMODE_ATT) == 0) {
		sw_e_msg(G, "listing attributes not supported yet\n");
	} else {
		/* program internal errro */
		/* never happens */
		sw_e_msg(G, "unhandled mode in swlist_lib.c\n");
	}

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	archive_fd = -1;

	E_DEBUG("");

	/* SWBIS_TS_Catalog_dir_remove
	   This removes the .../export/ directory
	   for example: var/lib/swbis/catalog/foo/foo/1.1/0/export */

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
			E_DEBUG("swpl_send_nothing_and_wait(SWBIS_TS_Catalog_dir_remove) returned error");
			retval = -12;
			goto error_out;
		}
        } else {
		E_DEBUG("Remove catalog dir");
                /* TS_Catalog_dir_remove */
                ret = swpl_remove_catalog_directory(swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd);
		if (ret != 0) {
			E_DEBUG("swpl_remove_catalog_directory returned error");
			retval = -13;
			goto error_out;
		}
	}

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG("");
	error_out:
	E_DEBUG("");

	strob_close(tmp);
	strob_close(btmp);
	strob_close(btmp2);
	strob_close(swspec_string);
	E_DEBUG("");
	if (file_list)
		swi_fl_delete(file_list);

	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);
	if (rstatus) {
		*p_rp_status = rstatus;
		E_DEBUG2("setting *p_rp_status = %d", rstatus);
	}
	if (no_permission)
		*p_rp_status = no_permission;
	if (swi) swi_delete(swi);
	if (swgpg) swgpg_delete(swgpg);
	if (ee) swicat_e_delete(ee);
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	E_DEBUG("");
	return retval;
}

void
swlist_blist_attr_add(BLIST * BL, char * level, char * attr)
{
	int n;
	n = strar_num_elements(BL->attributesM);
	if (!level)
		strar_add(BL->attr_levelsM, SW_A_host);
	else
		strar_add(BL->attr_levelsM, level);
	strar_add(BL->attributesM, attr);
	strob_chr_index(BL->attr_flagsM, n, '\x00');
}

char * 
swlist_blist_get_next_attr(BLIST * BL, int start, int * px)
{
	char * ret;
	int f;
	int i;
	int n;
	i = start;

	n = strar_num_elements(BL->attributesM);
	do {
		f = strob_get_char(BL->attr_flagsM, i++);
	} while (f && i < n);
	if (!f) {
		ret = strar_get(BL->attributesM, --i);
		if (px) *px = i;
	} else {
		ret = NULL;
	}
	return ret;
}

void
swlist_blist_mark_as_processed(BLIST * BL, int index)
{
	strob_chr_index(BL->attr_flagsM, index, '\x01');
}

void
swlist_blist_clear_all_as_processed(BLIST * BL)
{
	int i;
	int n;
	i = 0;
	n = strar_num_elements(BL->attributesM);
	do {
		strob_chr_index(BL->attr_flagsM, i++, '\x00');
	} while (i < n);
	if (BL->isf_headerM) {
		swheader_close(BL->isf_headerM);
		BL->isf_headerM = NULL;
	}
}

int
swlist_blist_level_is_set(BLIST * BL, int level)
{
	struct blist_level * list;
	list = BL->levelsM;
	return list[level].lv_is_setM;
}
