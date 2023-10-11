/* swconfig_lib.c -- swconfig routines.

 Copyright (C) 2009,2010 James H. Lowe, Jr. 
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
#include "swconfig_lib.h"

static
void
looper_abort(GB * G, SWICOL * swicol, int ofd)
{
	swpl_send_abort(swicol, ofd, G->g_swi_event_fd,  SWBIS_TS_Abort);
}

static
char *
determine_script_to_run(GB * G)
{
	if (G->g_do_unconfigM) {
		if (G->g_config_postinstallM) {
			return SW_A_unpostinstall;
		} else {
			return SW_A_unconfigure;
		}
	} else {
		if (G->g_config_postinstallM) {
			return SW_A_postinstall;
		} else {
			return SW_A_configure;
		}
	}
}

static
void
construct_execution_phase_script_frag1(GB * G, CISF_BASE * base, char * scriptname,
		SWI * swi, STROB * buf, int do_reconfigure, int current_return_code,
		char * parent_tag, STROB * config_msg) 
{
	SWI_CONTROL_SCRIPT * script;
	STROB * tmp;
	STROB * tmp0;
	STROB * tmp2;
	STROB * tmp3;

	E_DEBUG("Entering");
	script = swi_xfile_get_control_script_by_tag(base->ixfileM, scriptname);
	if (!script) {
		/*  no script */
		E_DEBUG("leaving");
		return;
	}

	tmp = strob_open(20);
	tmp0 = strob_open(20);
	tmp2 = strob_open(20);
	tmp3 = strob_open(20);

	E_DEBUG("");
	if (parent_tag && strlen(parent_tag)) {
		E_DEBUG("");
		strob_sprintf(tmp0, 0, "%s.%s", parent_tag, base->ixfileM->baseM.b_tagM);
	} else {
		E_DEBUG("");
		strob_sprintf(tmp0, 0, "%s", base->ixfileM->baseM.b_tagM);
	}
	strob_strcpy(tmp, "");
	strob_sprintf(tmp, STROB_DO_APPEND,
		"%s %s", strob_str(tmp0), script->baseM.b_tagM);
	strob_sprintf(tmp3, STROB_NO_APPEND, SW_A_SCRIPT_ID "=%d", script->sidM);

	if (
		current_return_code == SWI_RESULT_UNDEFINED ||
		do_reconfigure ||
		(
			G->g_do_unconfigM &&
			current_return_code != SWI_RESULT_UNDEFINED
		)
	) {
		if (G->g_opt_previewM) {
			E_DEBUG("preview mode");
			strob_strcpy(tmp2, strob_str(tmp));
			strob_strcat(tmp2, ": "  SWEVENT_ATT_STATUS_PREVIEW);
			strob_sprintf(buf, STROB_DO_APPEND,
			CSHID
			" %s\n"
			" %s\n"
			" %s\n",
			TEVENT(2, (swi->verboseM), SW_CONTROL_SCRIPT_BEGINS, strob_str(tmp)),
			TEVENT(2, (swi->verboseM), SWI_MSG, strob_str(tmp3)),
			TEVENT(2, (swi->verboseM), SW_CONTROL_SCRIPT_ENDS, strob_str(tmp2))
			);
		} else {
			E_DEBUG("not preview mode");
			swpl2_construct_control_script(G, base, buf, swi, scriptname, parent_tag);
		}
	} else {
		/* already configured */
		/* give the ALREADY CONFIGURED event */
		E_DEBUG("already configured");
		strob_strcpy(tmp2, strob_str(tmp));
		strob_sprintf(tmp2, STROB_DO_APPEND,  ": " SWEVENT_STATUS_PFX "%d", current_return_code);
		strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		" %s\n"
		" %s\n"
		,
		TEVENT(2, swi->verboseM, SWI_MSG, strob_str(tmp3)),
		TEVENT(2, swi->verboseM, SW_ALREADY_CONFIGURED, strob_str(tmp2))
		);
		strob_sprintf(config_msg, STROB_DO_APPEND, "SW_ALREADY_CONFIGURED: %s\n", strob_str(tmp2));
	}
	strob_close(tmp);
	strob_close(tmp0);
	strob_close(tmp2);
	strob_close(tmp3);
	E_DEBUG("leaving");
}

static
char *
get_script_result(CISF_BASE * base, char * scriptname)
{
	char * result;
	SWI_CONTROL_SCRIPT * control_script;
	SWI_XFILE * xfile;
	xfile = base->ixfileM;
	control_script = swi_xfile_get_control_script_by_tag(xfile, scriptname);
	if (control_script != NULL) {
		result = swi_control_script_posix_result(control_script);
		return result;
	} else {
		return NULL;
	}
}

static
void
construct_execution_phase_script(GB * G, CISF_PRODUCT * cisf, SWI * swi, STROB * buf, STROB * config_msg) 
{
	int current_return_code;
	int i;
	int do_reconfigure;
	CISF_BASE * product_base;
	CISF_BASE * cisf_base;
	char * current_result;
	char * parent_tag;
	char * scriptname;

	E_DEBUG("");

	do_reconfigure = swextopt_is_option_true(SW_E_reconfigure, G->optaM);
	scriptname = determine_script_to_run(G);

	strob_sprintf(buf, STROB_DO_APPEND,
		"( # subshell 001\n"
			CSHID
	);

	E_DEBUG("");
	/* FIXME */
	/* don't assume it the first product ???, use sw selections */

	product_base = swpl2_cisf_base_array_get(cisf, 0); /* this is the product */

	/* Need to determine if the product is already configured */
	current_result = get_script_result(product_base, scriptname);

	/* 
	 * Construct the script fragment for the product
	 */
	if (current_result) {
		E_DEBUG2("has current result [%s]", current_result);
		current_return_code = swi_control_script_get_return_code(current_result);
		construct_execution_phase_script_frag1(G, product_base, scriptname,
			swi, buf, do_reconfigure, current_return_code, NULL, config_msg);
	} else {
		/* OK, product must not have this script */
		E_DEBUG2("no script [%s]", current_result);
		;
	}

	/* 
	 * Now loop over the filesets
	 */

	E_DEBUG("looping over filesets");
	parent_tag = product_base->ixfileM->baseM.b_tagM;
	i = 1;	
	E_DEBUG2("i=%d", i);
	while ((cisf_base = swpl2_cisf_base_array_get(cisf, i++)) != NULL) {
		E_DEBUG2("in fileset i=%d", i);

		current_result = get_script_result(cisf_base, scriptname);
		if (current_result) {
			E_DEBUG2("current result = %s", current_result);
			current_return_code = swi_control_script_get_return_code(current_result);
			construct_execution_phase_script_frag1(G, cisf_base, scriptname,
				swi, buf, do_reconfigure, current_return_code, parent_tag, config_msg);
		} else {
			/* OK, fileset must not have this script */
			E_DEBUG("no script in this fileset");
			;
		}
	}

	E_DEBUG("");
	strob_sprintf(buf, STROB_DO_APPEND,
		"# FIXME: why isn't sw_retval being set\n"
		"sw_retval=\"${sw_retval:=0}\"\n"
		"exit \"$sw_retval\"\n"
		") 1</dev/null # subshell 001\n"
		"sw_retval=$?\n"
	);

	strob_sprintf(buf, STROB_DO_APPEND,
		"dd bs=512 count=1 of=/dev/null 2>/dev/null\n"
		);
}

static
int
do_run_configure(GB * G, SWI * swi, char * target_path,
		SWHEADER * isf_header, CISF_PRODUCT * cisf,
		SWICOL * swicol, SWICAT_E * e, int ofd, STROB * config_msg)
{
	STROB * buf;
	int ret;

	E_DEBUG("");
	buf = strob_open(300);

	construct_execution_phase_script(G, cisf, swi, buf, config_msg); 

	ret = swicol_rpsh_task_send_script2(
		swicol,
		ofd,
		512,
		target_path, /* WAS "."*/
		strob_str(buf),
		SWBIS_TS_run_configure);

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

		"				shls_bashin2 \"" SWBIS_TS_Load_INSTALLED_transient "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_INSTALLED_transient ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"
		
		"				shls_bashin2 \"" SWBIS_TS_run_configure "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_run_configure  ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		CSHID
		"				shls_bashin2 \"" SWBIS_TS_Catalog_dir_remove "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Catalog_dir_remove ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_Load_INSTALLED_90 "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_INSTALLED_90 ";; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_make_live_INSTALLED  "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s " SWBIS_TS_make_live_INSTALLED ";; *) sleep 2; shls_false_;; esac\n"
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
void
merge_cisf_update_times(char * create_time, char * mod_time, SWI_BASE * base)
{
	uintmax_t ctime;
	uintmax_t mtime;
	E_DEBUG("");
	if (create_time) {
		ctime = strtoumax(create_time, NULL, 10);
		base->create_timeM = (time_t)ctime;
	}
	if (mod_time) {
		mtime = strtoumax(mod_time, NULL, 10);
		base->mod_timeM = (time_t)mtime;
	}
}

static
int
merge_cisf_process_product(CISF_PRODUCT * cisf_product, SWHEADER * swheader)
{
	char * next_line;
	char * tag;
	char * object_keyword;
	char * mod_time;
	char * state;
	char * create_time;
	char * result;
	int script_return_code;
	int retval;
	int pix;
	SWI * swi;
	SWHEADER_STATE at_xfile;
	SWI_CONTROL_SCRIPT * control_script;
	CISF_FILESET * cisf_fileset;
	SWI_XFILE * fileset;
	SWI_XFILE * current_xfile;

	E_DEBUG("");
	retval = 0;
	swi = cisf_product->swiM;


	/* loop over the filesets in the product */
	swheader_restore_state(swheader, cisf_product->cisf_baseM.atM);

	/* update the create_time and mod_time from INSTALLED */
	create_time = swheader_get_single_attribute_value(swheader, SW_A_create_time);
	mod_time = swheader_get_single_attribute_value(swheader, SW_A_mod_time);

	merge_cisf_update_times(create_time, mod_time, &(cisf_product->productM->p_baseM));
	E_DEBUG("");

	current_xfile = cisf_product->productM->xfileM;
	SWLIB_ASSERT(current_xfile != NULL);

	/* Loop over the objects of the INSTALLED file */

	while ((next_line=swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX )) ) {
		E_DEBUG2("next_line=%s\n", next_line);
		object_keyword = swheaderline_get_keyword(next_line);

		if (swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ) {
			/* Sanity check */
			SWBIS_IMPL_ERROR_DIE(1);
		} else {
			/* All is well, we are located in the attributes of 
			   installed product */
			;
		}
		
		/* Merge the create_time and mod_time from the product object */

		create_time = swheader_get_single_attribute_value(swheader, SW_A_create_time);
		mod_time = swheader_get_single_attribute_value(swheader, SW_A_mod_time);
		merge_cisf_update_times(create_time, mod_time, &(cisf_product->cisf_baseM.ixfileM->baseM));

		if (strcmp(object_keyword, SW_A_product) == 0) {
			E_DEBUG("");
			/* STOP */
			/* don't go into the next product */
			/* There should not be a second product in the INSTALLED file */
			break;
		} else if (strcmp(object_keyword, SW_A_fileset) == 0) {

			/* We need to get the state amd mod_time and create_time from the
			   INSTALLED file so when the INSTALLED file is rewritten (overwritten)
			   this info is not lost */

			E_DEBUG("in fileset");
			swheader_store_state(swheader, &at_xfile);
			cisf_fileset = swpl_cisf_fileset_create();
			tag = swheader_get_single_attribute_value(swheader, SW_A_tag);
			E_DEBUG2("Found fileset: tag = [%s]", tag);
			fileset = swi_find_fileset_by_swsel(cisf_product->productM, tag, &pix);
			current_xfile = fileset;
			cisf_fileset->cisf_baseM.ixfileM = fileset;
			cisf_fileset->cisf_baseM.cf_indexM = pix;
			swheader_state_copy(cisf_fileset->cisf_baseM.atM, &at_xfile);
			vplob_add(cisf_product->isetsM, cisf_fileset);
			create_time = swheader_get_single_attribute_value(swheader, SW_A_create_time);
			mod_time = swheader_get_single_attribute_value(swheader, SW_A_mod_time);
			state = swheader_get_single_attribute_value(swheader, SW_A_state);
			if (state) {
				E_DEBUG("");
				strncpy(fileset->stateM, state, sizeof(fileset->stateM) -1);
			} else {
				E_DEBUG("state attribute not found");

			}
			fileset->stateM[sizeof(fileset->stateM)-1] = '\0';
			merge_cisf_update_times(create_time, mod_time, &(cisf_fileset->cisf_baseM.ixfileM->baseM));
		} else if (strcmp(object_keyword, SW_A_control_file) == 0) {

			/* We need to get the return codes from the INSTALLED file so when
			   the INSTALLED file is rewritten (overwritten) this info is not lost */

			E_DEBUG("in control file");

			/* find this control_file in the current_xfile object */

			tag = swheader_get_single_attribute_value(swheader, SW_A_tag);
			result = swheader_get_single_attribute_value(swheader, SW_A_result);
			E_DEBUG2("Found control script: tag = [%s]", tag);
			E_DEBUG3("Control script: tag = [%s], result=[%s]", tag, result);
			control_script = swi_xfile_get_control_script_by_tag(current_xfile, tag);

			/* this should never fail */
			SWLIB_ASSERT(control_script != NULL);
			script_return_code = swi_control_script_get_return_code(result);
			control_script->resultM = script_return_code;	
		} else if (strcmp(object_keyword, SW_A_bundle) == 0) {
			/* not supported at this time */
			;
		} else if (strcmp(object_keyword, SW_A_subproduct) == 0) {
			/* not supported at this time */
			;
		} else {
			/* fatal error */
			/* There should *NOT* be any other objects in the INSTALLED file */
			E_DEBUG("");
			SWLIB_ASSERT(0);
		}
	}
	E_DEBUG("");
	return retval;
}

static
int
merge_cisf(SWI * swi, SWHEADER * swheader, CISF_PRODUCT * cisf_product)
{
	char * next_line;
	char * tag;
	char * number;
	char * keyword;
	int retval;
	int ix;
	SWHEADER_STATE state1;
	SWHEADER_STATE at_product;
	SWI_PRODUCT * product;

	E_DEBUG("");
	retval = 0;
	swheader_store_state(swheader, &state1);
	swheader_reset(swheader);
	cisf_product->swiM = swi;
	
	/* swheader_print_header(isf_header); */
	/* fprintf(stderr, "xxstatus is %d\n", xxstatus); */

	/* Loop over the objects in the SW_A_INSTALLED file */

	E_DEBUG("");
	while ((next_line=swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX))) {
		E_DEBUG2("next_line=%s\n", next_line);
		if (swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ) {
			/* Sanity check */
			return -1;
			SWBIS_IMPL_ERROR_DIE(1);
		}
		keyword = swheaderline_get_keyword(next_line);

		if (strcmp(keyword, SW_A_product) == 0) {
			swheader_store_state(swheader, &at_product);
			tag = swheader_get_single_attribute_value(swheader, SW_A_tag);
			number = swheader_get_single_attribute_value(swheader, SW_A_number);
			/* find this product in the index file */
			product = swi_find_product_by_swsel(swi->swi_pkgM, tag, number, &ix);
			if (! product) {
				/* error, not found */
				retval = 1;
				break;
			}
			swheader_restore_state(swheader, &at_product);
			cisf_product->cisf_baseM.cf_indexM = ix;
			cisf_product->productM = product;
			ix = 0;
			while (
				swi->swi_pkgM->swi_coM[ix] != NULL && 
				swi->swi_pkgM->swi_coM[ix] != product
			) {
				ix++;
			}
			SWLIB_ASSERT(swi->swi_pkgM->swi_coM[ix] != NULL);
			cisf_product->cisf_baseM.cf_indexM = ix;
			swheader_state_copy(cisf_product->cisf_baseM.atM, &at_product);
			
			/* Now update the filesets and product scripts */

			retval = merge_cisf_process_product(cisf_product, swheader);

			/* This test was OK on 07JAN2010
			swprogs/swconfig --no-defa  acpid @ /tmp/acpid_test_jhl4  2>&1 |
			diff /tmp/acpid_test_jhl4/var/lib/swbis/catalog/acpid/acpid/1.0.8/0/INSTALLED - 
			*/

			/*
			{
			STROB * xx = strob_open(32);
			swicat_isf_installed_software(xx, swi);
			fprintf(stderr, "%s\n", strob_str(xx));
			strob_close(xx);
			}
			*/

			break;	/* only process one product */
		}
	}
error_out:
	E_DEBUG("");
	swheader_restore_state(swheader, &state1);
	return retval;
}

int
swconfig_write_source_copy_script2(GB * G,
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
	char * local_env;
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
	STROB * env_buf;
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
	env_buf = strob_open(100);

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

	if (G->g_send_envM)
		local_env = swpl_make_environ_transfer_image(env_buf);
	else
		local_env = "";
	
	/* Now here is the script */
	strob_sprintf(buffer_new, STROB_DO_APPEND,
		"trap '/bin/rm -f ${LOCKPATH}.lock; exit 1' 1 2 15\n"
		"echo " SWBIS_TARGET_CTL_MSG_125 ": " KILL_PID "\n"
		"echo " SWBIS_TARGET_CTL_MSG_129 ": \"`pwd | head -1`\"\n"
		CSHID
		"%s\n"
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
		"swutilname=swconfig\n"
		"do_cleansh=\"%s\"\n"
		"%s\n"			/* shls_bashin2 from shell_lib.sh */
		"%s\n"			/* shls_false_ from shell_lib.sh */
		"%s\n"			/* shls_looper from  shell_lib.sh */
		"%s\n"			/* shls_looper_payload from shell_lib.sh */
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
				/* This is an error for swconfig */
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
		local_env,
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
	strob_close(env_buf);
	/*
	 * 0 is OK
	 * !0 is error
	 */
	return !(ret > 0);
}

int
swconfig_looper_sr_payload(GB * G, char * target_path, SWICOL * swicol,
		SWICAT_SR * sr, int ofd, int ifd, int * p_rp_status, SWUTS * uts, int * did_skip)
{
	int retval;
	int rstatus;
	int ret;
	int do_skip_entry;
	int checkremove_status;
	int do_check_abort;
	int xxstatus;
	int result;
	int sig_level;
	char * pax_read_command;
	char * epath;
	char * script_name;
	char * installed_software_catalog;
	char * catalog_entry_directory;
	STROB * xxtmp;
	STROB * btmp;
	STROB * btmp2;
	STROB * config_msg;
	STROB * swspec_string;
	AHS * isf_ahs;
	SWICAT_E * e;
	SWI * swi;
	SWGPG_VALIDATE * swgpg;
	SWHEADER * isf_header;
	CISF_PRODUCT * cisf_product = NULL;
	SWI_PRODUCT * current_product = NULL;
 	
	do_skip_entry = 0;
	retval = 0;
	rstatus = 0;
	xxtmp = strob_open(100);
	btmp = strob_open(100);
	btmp2 = strob_open(100);
	swspec_string = strob_open(100);
	config_msg = strob_open(100);
	swi = NULL;
	swgpg = NULL;
	e = NULL;
	isf_ahs = ahs_open();

	checkremove_status = -1; /* unset value */

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
	E_DEBUG("");
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
	E_DEBUG("");

	/* e->entry_prefixM has the installed_software_catalog
	  path already prefixed */
	epath = swicat_e_form_catalog_path(e, btmp, NULL, SWICAT_ACTIVE_ENTRY);
	if (G->devel_verboseM)
		SWLIB_INFO2("catalog entry (active) path = %s", epath);
	
	E_DEBUG("");
	epath = swicat_e_form_catalog_path(e, btmp, NULL, SWICAT_DEACTIVE_ENTRY);
	if (G->devel_verboseM)
		SWLIB_INFO2("catalog entry path (inactive) path = %s", epath);

	E_DEBUG("");
	epath = swicat_e_form_catalog_path(e, btmp, NULL, SWICAT_ACTIVE_ENTRY);

	/* epath is now a clean relative path to such as:
		var/lib/swbis/catalog/emacs/emacs/21.2.1/0  */

	E_DEBUG("");
	ret = swicat_e_reset_fd(e);
	if (ret < 0) {
		rstatus = 2;
		retval = -3;
		sw_e_msg(G, "error reseting swicat_e object, ret=%d\n", ret);
		looper_abort(G, swicol, ofd);
		goto error_out;
	}

	E_DEBUG("");
	/* Now obtain the verification status for each signature */
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	E_DEBUG("");
	swgpg = swgpg_create();
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	E_DEBUG("");
	ret = 0;
	if (sig_level >= 0)
		ret = swicat_e_verify_gpg_signature(e, swgpg);
	if (ret != 0) {
		rstatus = 3;
		retval = 0;
		sw_e_msg(G, "error from swicat_e_verify_gpg_signature, ret=%d\n", ret);
	}

	/* Now interpret the verification results according to extended option
	   requirements */

	E_DEBUG("");
	ret = 0;
	if (sig_level >= 0)
		ret = swpl_signature_policy_accept(G, swgpg, G->g_verboseG, strob_str(swspec_string));
	E_DEBUG("");
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
	 * because its authenticated or we don't care.
	 */

	E_DEBUG("");
	swi = swicat_e_open_swi(e);
	if (swi == NULL) {
		/* swi might be NULL due to file system permission,
		   handle NULL gracefully */
		do_skip_entry = 1;
		sw_e_msg(G, "catalog read error: %s/%s\n", target_path, epath);
	} else {
		swi->swi_pkgM->catalog_entryM = strdup(epath); /* FIXME, this must set explicitly */
		swi->swi_pkgM->target_pathM = strdup(target_path); /* FIXME, this must set explicitly */
		swi->swi_pkgM->installed_software_catalogM = strdup(get_opta_isc(G->optaM, SW_E_installed_software_catalog));
		ret = swicat_e_find_attribute_file(e, SW_A_location, xxtmp, NULL);
		if (ret) {
			swi->swi_pkgM->locationM  = strdup("/");
		} else {
			swi->swi_pkgM->locationM  = strdup(strob_str(xxtmp));
		}
	}

	E_DEBUG("");
	/* Now map the sections of the INSTALLED file to
	   the (SWI*) object, this is done by 
	   swpl_cisf_<*> routines and stored in the
	   (CISF_PRODUCT*) object. */

	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	if (swi) swi->optaM = G->optaM;
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	
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
		do_skip_entry = 1;
		goto error_out;
	}
	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	/* wait for clear-to-send */

	ret = swicol_rpsh_wait_cts(swicol, G->g_swi_event_fd);
	*p_rp_status = ret;
	E_DEBUG2("rp_status is %d", ret);
	if (ret) {
		E_DEBUG("error");
		rstatus = -1;
		retval = -5;
		goto error_out;
	}

	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	if (swi) {
		/* Now we must merge the state, creat_time, mod_time and scripts
		   results from INSTALLED into the (SWI*)swi object so that
		   when INSTALLED is updated it will not loose information */
	
		/* swheader_print_header(isf_header); */
		/* fprintf(stderr, "xxstatus is %d\n", xxstatus); */
		
		E_DEBUG("HERE");
	
		/* Create the CISF_PRODUCT object
		*/	
		E_DEBUG("HERE");
		current_product = swi_package_get_product(swi->swi_pkgM, D_ZERO /* the first one */);
		cisf_product = swpl_cisf_product_create(current_product);
	
		E_DEBUG("HERE");
		/*  Now apply selection, which for now is to assume one product and one fileset */
		swpl_cisf_init_single_single(cisf_product, swi);
		
		E_DEBUG("HERE");
		/* Now make the array of the base structure for the list of filesets in cisf_product */
		swpl2_audit_cisf_bases(G, swi, cisf_product);
	} else {
		;
	}

	E_DEBUG("HERE");
	/* Create the SWHEADER of the INSTALLED file
	*/
	isf_header = swicat_e_isf_parse(e, &xxstatus, isf_ahs);
	/* FIXME, check xxstatus */

	E_DEBUG("HERE");
	if (isf_header) {
		E_DEBUG("HERE");

		/* Merge the INSTALLED file from the installed_catalog into the (SWI*)object */

		ret = merge_cisf(swi, isf_header, cisf_product);

		/* fprintf(stderr, "location %s\n", swi->swi_pkgM->locationM); */

		/* This is a test */  /* swicat_write_installed_software(swi, swi->nullfdM); */
		E_DEBUG("HERE");
		if (ret) {
			sw_e_msg(G, "error reading and merging the INSTALLED file\n");
			do_skip_entry = 1;
		}
	} else {
		E_DEBUG("HERE");
		sw_e_msg(G, "error parsing INSTALLED file\n");
		do_skip_entry = 1;
	}

	if (G->devel_verboseM)
		SWLIB_INFO2("%s", strob_str(btmp));
	
	E_DEBUG("HERE");

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
			rstatus = -3;
			retval = -9;
			goto error_out;
		}
	}

	/* Analysis Phase */
	/* SWBIS_TS_Analysis_002 */

	E_DEBUG("HERE");
	do_check_abort = 0;
	ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd, SWBIS_TS_Analysis_002,
		SWICOL_TL_8, SW_SUCCESS);
	if (ret != 0) {
		E_DEBUG("");
		rstatus = -4;
		retval = -10;
		goto error_out;
	}

	/* Need to set the product or fileset state to transient */
	
	/* SWBIS_TS_Load_INSTALLED_transient */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		do_check_abort ||
		0
	) {
		E_DEBUG("");
		E_DEBUG("SWBIS_TS_Load_INSTALLED_transient, NULL");
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_Load_INSTALLED_transient,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			rstatus = -5;
			retval = -11;
			goto error_out;
		}
	} else {
		/* write the installed file */
		E_DEBUG("SWBIS_TS_Load_INSTALLED_transient");
		ret = swpl_write_tar_installed_software_index_file(G, swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd,
			SWBIS_TS_Load_INSTALLED_transient, SW_A_INSTALLED, NULL);
		if (ret != 0) {
			rstatus = -6;
			retval = -12;
			goto error_out;
		}
	}

	/* SWBIS_TS_run_configure */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		do_check_abort ||
		0
	) {
		E_DEBUG("run configure: doing nothing");
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_run_configure,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			rstatus = -7;
			retval = -13;
			goto error_out;
		}
	} else {
		E_DEBUG("run configure: really running it");
		ret = do_run_configure(G, swi, target_path, isf_header, cisf_product, swicol, e, ofd, config_msg);
		if (ret < 0) {
			/* internal error */
			E_DEBUG("internal error");
			rstatus = -8;
			retval = -14;
			goto error_out;
		} else if (ret > 0) {
			/* script error */
			E_DEBUG("script error");
			rstatus = 9;
			goto error_out;
		}
		if (strob_strlen(config_msg)) {
			/* sw_l_msg(G, SWC_VERBOSE_1, "%s", strob_str(config_msg)); */
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
		E_DEBUG("SWBIS_TS_Catalog_dir_remove NULL");
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_Catalog_dir_remove,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			rstatus = -10;
			retval = -15;
			goto error_out;
		}
        } else {
                /* TS_Catalog_dir_remove */
		E_DEBUG("SWBIS_TS_Catalog_dir_remove");
                ret = swpl_remove_catalog_directory(swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd);
		if (ret != 0) {
			rstatus = -12;
			retval = -16;
			goto error_out;
		}
	}

	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		E_DEBUG("SWBIS_TS_Load_INSTALLED_90 NULL");
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
			SWBIS_TS_Load_INSTALLED_90,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			rstatus = -13;
			retval = -17;
			goto error_out;
		}
        } else {
                /* SWBIS_TS_Load_INSTALLED_90 */

		E_DEBUG("SWBIS_TS_Load_INSTALLED_90");
		ret = swpl2_update_execution_script_results(swi, swicol, cisf_product);
		if (ret == 0) {
			/* this means there was no script executed which
			   happens when there is no script to execute,
			   just issue a warning */
			sw_e_msg(G, "warning: no script executed\n");
		}

		script_name = determine_script_to_run(G);
		if (
			strcmp(script_name, SW_A_configure) == 0 ||
			strcmp(script_name, SW_A_unconfigure) == 0 ||
			0
		) {
			E_DEBUG2("running normalize_configure_script_results: %s", script_name);
			swpl2_normalize_configure_script_results(swi, script_name, cisf_product);
		}

		ret = swpl_write_tar_installed_software_index_file(G, swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd,
			SWBIS_TS_Load_INSTALLED_90, SW_A__INSTALLED, isf_ahs);
		if (ret != 0) {
			rstatus = -14;
			retval = -18;
			goto error_out;
		}
	}

	/* SWBIS_TS_make_live_INSTALLED */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
			SWBIS_TS_make_live_INSTALLED,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			retval = -1;
			goto error_out;
		}
        } else {
		ret = swpl_run_make_installed_live(G, swi, ofd, target_path);
		if (ret != 0) {
			rstatus = -15;
			retval = -19;
			goto error_out;
		}
	}

	E_DEBUG("BEFORE error_out");
	error_out:
	E_DEBUG("AFTER error_out");
	if (do_skip_entry)
		*did_skip = 1;
	strob_close(xxtmp);
	strob_close(config_msg);
	strob_close(btmp);
	strob_close(btmp2);
	strob_close(swspec_string);
	ahs_close(isf_ahs);

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
