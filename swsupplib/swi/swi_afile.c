/* swi_afile.c -- POSIX attribute and control file object. 

   Copyright (C) 2005 Jim Lowe
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swheader.h"
#include "swheaderline.h"
#include "ugetopt_help.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "atomicio.h"
#include "swicat.h"
#include "swi_base.h"
#include "swi_afile.h"

#include "debug_config.h"
#ifdef SWSWINEEDDEBUG
#define SWSWI_E_DEBUG(format) SWBISERROR("SWI DEBUG: ", format)
#define SWSWI_E_DEBUG2(format, arg) SWBISERROR2("SWI DEBUG: ", format, arg)
#define SWSWI_E_DEBUG3(format, arg, arg1) \
			SWBISERROR3("SWI DEBUG: ", format, arg, arg1)
#else
#define SWSWI_E_DEBUG(arg)
#define SWSWI_E_DEBUG2(arg, arg1)
#define SWSWI_E_DEBUG3(arg, arg1, arg2)
#endif 

typedef struct  {
	int did_itM;
	char * tagM;
} G_CONTROL_SCRIPT;

static
G_CONTROL_SCRIPT pp_g_scripts[] = {
		{0, "checkinstall"},
		{0, "preinstall"}, 
		{0, "postinstall"}, 
		{0, "verify"},
		{0, "fix"},
		{0, "checkremove"}, 
		{0, "preremove"},  
		{0, "postremove"},
		{0, "configure"},
		{0, "unconfigure"}, 
		{0, "request"},    
		{0, "unpreinstall"}, 
		{0, "unpostinstall"}, 
		{0, "space"},        
		{0, (char*)NULL}
		};


static
void
make_case_pattern_for_unused_tags(STROB * buf)
{
	int is_first = 1;
	G_CONTROL_SCRIPT * script_ent;
			
	strob_sprintf(buf, STROB_DO_APPEND, "\t\t");
	script_ent = pp_g_scripts;
	while ((script_ent)->tagM) {
		if (script_ent->did_itM == 0) {
			if (is_first == 0)
				strob_sprintf(buf, STROB_DO_APPEND, "|");
			strob_sprintf(buf, STROB_DO_APPEND,
				"%s", script_ent->tagM);
			is_first = 0;
		} else {
			script_ent->did_itM = 0;
		}
		script_ent++;
	}
	strob_sprintf(buf, STROB_DO_APPEND, ")\n");
	strob_sprintf(buf, STROB_DO_APPEND, 
		"\t\t\techo \"$0: no ${SWBCS_SCRIPT_TAG} script for ${SWBCS_MATCH}\" 1>&2\n"
		"\t\t\texit " SWBIS_STATUS_COMMAND_NOT_FOUND "\n"
		"\t\t\t;;\n"
		);
}

static
int
set_is_ieee_control_script(char * pathname, int do_set)
{
	char * base;
	G_CONTROL_SCRIPT * script_ent;

	base = swlib_basename(NULL, pathname);
	swi_com_assert_pointer((void*)base, __FILE__, __LINE__);
	script_ent = pp_g_scripts;
	while ((script_ent)->tagM) {
		if (strcmp(base, (script_ent)->tagM) == 0) {
			if (do_set) {
				(script_ent)->did_itM = 1;
			}
			return 1;
		}
		script_ent++;
	}
	return 0;
}


static
char *
get_attribute_from_INFO_object(SWI_CONTROL_SCRIPT * control_script, char *attr_name)
{
	SWI_XFILE * xfile;
	SWHEADER * h;
	SWHEADER_STATE state;
	char * val;

	xfile = control_script->swi_xfileM;
	if (!xfile) return NULL;
	h = xfile->info_headerM;
	if (!h) return NULL;
	swheader_store_state(h, &state);
	swheader_reset(h);
	swheader_set_current_offset_p(h, &(xfile->INFO_header_indexM));
	swheader_set_current_offset_p_value(h, control_script->INFO_offsetM);
	val = swheader_get_single_attribute_value(h, attr_name);
	swheader_restore_state(h, &state);
	return val;
}

void
swi_scripts_delete(SWI_SCRIPTS * s)
{
	int i;
	for(i=0; i<SWI_MAX_OBJ; i++) {
		if (s->swi_coM[i])
			swi_control_script_delete(s->swi_coM[i]);
	}
	free(s);
}

SWI_SCRIPTS *
swi_scripts_create(void)
{
	SWI_SCRIPTS * s = (SWI_SCRIPTS *)malloc(sizeof(SWI_SCRIPTS));
	SWSWI_E_DEBUG("");
	swi_com_assert_pointer((void*)s, __FILE__, __LINE__);
	swiInitListOfObjects((void**)(s->swi_coM));
	return s;
}

void
swi_file_member_delete(SWI_FILE_MEMBER * s)
{
	if (s->refcountM > 1) {
		(s->refcountM)--;
		return;
	}
	if (s->pathnameM) free(s->pathnameM);
	if (s->dataM) free(s->dataM);
	free(s);
}

SWI_FILE_MEMBER *
swi_file_member_create(void)
{
	SWI_FILE_MEMBER * s = (SWI_FILE_MEMBER *)malloc(
					sizeof(SWI_FILE_MEMBER));
	swi_com_assert_pointer((void*)s, __FILE__, __LINE__);
	swi_vbase_init(s, SWI_I_TYPE_AFILE, NULL, NULL);
	s->refcountM = 1;
	s->pathnameM = NULL;
	s->lenM  = -1;
	s->dataM = NULL;
	return s;
}

void
swi_control_script_delete(SWI_CONTROL_SCRIPT * s)
{
	if (s->baseM.b_tagM) free(s->baseM.b_tagM);
	if (s->afileM)
		swi_file_member_delete(s->afileM);
	free(s);
}

SWI_CONTROL_SCRIPT *
swi_control_script_create(void)
{
	static int id = 0;

	SWI_CONTROL_SCRIPT * s;
	SWSWI_E_DEBUG("");
	s = (SWI_CONTROL_SCRIPT *)malloc(sizeof(SWI_CONTROL_SCRIPT));
	swi_com_assert_pointer((void*)s, __FILE__, __LINE__);

	swi_vbase_init(s, SWI_I_TYPE_SCRIPT, NULL, NULL);
	s->sidM = ++id;
	s->afileM = NULL; /* swi_file_member_create(); */
	s->swi_xfileM = (void*)NULL; 
	s->INFO_offsetM = -1; /* unset */ 
	s->resultM = SWI_RESULT_UNDEFINED;
	return s;
}

void
swi_add_script(SWI_SCRIPTS * thisisit, SWI_CONTROL_SCRIPT * v)
{
	SWSWI_E_DEBUG("ENTERING");
	swiAddObjectToList((void**)(thisisit->swi_coM), (void *)v);
	SWSWI_E_DEBUG("LEAVING");
}

int
swi_control_script_get_return_code(char * posix_result)
{
	if (strcasecmp(posix_result, SW_RESULT_NONE) == 0) {
		return SWI_RESULT_UNDEFINED ;
	} else if (strcasecmp(posix_result, SW_RESULT_SUCCESS) == 0) {
		return SWI_RESULT_SUCCESS;
	} else if (strcasecmp(posix_result, SW_RESULT_WARNING) == 0) {
		return SWI_RESULT_WARNING;
	} else if (strcasecmp(posix_result, SW_RESULT_FAILURE) == 0) {
		return SWI_RESULT_FAILURE;
	} else {
		/* internal error */
		fprintf(stderr, "%s: unrecognized  POSIX result string: [%s]\n", swlib_utilname_get(), posix_result); 
		return SWI_RESULT_FAILURE;
	}
}

char *
swi_control_script_posix_result(SWI_CONTROL_SCRIPT * s)
{
	if (s->resultM == SWI_RESULT_UNDEFINED || s->resultM < 0) {
		return SW_RESULT_NONE;
	} else if (s->resultM == SW_SUCCESS) {
		return SW_RESULT_SUCCESS;
	} else if (s->resultM == SW_ERROR) {
		return SW_RESULT_FAILURE;
	} else if (s->resultM == SW_WARNING) {
		return SW_RESULT_WARNING;
	} else if (s->resultM == SW_NOTE) {
		return SW_RESULT_WARNING;
	} else {
		fprintf(stderr, "%s: internal error: bad result code: [%d]\n", swlib_utilname_get(), s->resultM); 
		return SW_RESULT_FAILURE;
	}
}

int
swi_afile_write_script_cases(SWI_SCRIPTS * scripts, STROB * buf, char * installed_isc_path)
{
	int i;
	char * lslash;
	char * name;
	char * interpreter;
	G_CONTROL_SCRIPT * script_ent;
	SWI_CONTROL_SCRIPT * control_script;
	STROB * vintbuf;

	vintbuf = strob_open(10);

	/*
	 * initialize the list that keeps track of the
	 * scripts that exist
	 */
	script_ent = pp_g_scripts;
	while ((script_ent)->tagM) {
		(script_ent)->did_itM = 0;
		script_ent++;
	}

	strob_sprintf(buf, STROB_DO_APPEND,
		"		case \"${SWBCS_SCRIPT_TAG}\" in\n"
		);
			

	SWLIB_ASSERT(installed_isc_path != NULL);

	/*
	 * Loop over all the scripts
	 */

	for(i=0; i<SWI_MAX_OBJ; i++) {
		script_ent = pp_g_scripts;
		control_script = scripts->swi_coM[i];
		if (control_script) {
			/*
			 * Mark the tag as being used
			 */
			set_is_ieee_control_script(control_script->baseM.b_tagM, 1);

			/*
			 * Now write the shell case statement fragment
			 */

			/*
			 * Get the interpreter attribure from the software definition in
			 * the INFO file.
			 */

			interpreter = get_attribute_from_INFO_object(control_script, SW_A_interpreter);	
			if (
				interpreter == NULL
			) {
				interpreter = "";
			}

			/*
			 * Sanity check, Ignore gargbage 
			 */

			if (strlen(interpreter) && swlib_check_clean_path(interpreter)) {
				interpreter = "";
			}

			if (strlen(interpreter)) {
				strob_sprintf(vintbuf, STROB_NO_APPEND, "INTERPRETER=\"%s\"\n", interpreter);
			} else {
				strob_strcpy(vintbuf, "");
			}


			lslash = strrchr(control_script->afileM->pathnameM, '/');
			SWLIB_ASSERT(lslash != NULL);
			*lslash = '\0';
			name = lslash+1;

			strob_sprintf(buf, STROB_DO_APPEND,
				"\t\t%s)\n"
				"\t\t\texport SW_CONTROL_TAG=\"%s\"\n"
				"\t\t\t%s\n"
				"\t\tcase \"$SW_CATALOG\" in\n"
				"\t\t\t  # User specified SW_CATALOG as really, really absolute\n"
				"\t\t\t/*) export SW_CONTROL_DIRECTORY=\"${SW_CATALOG}/%s/%s\"\n"
				"\t\t\t;;\n"
				"\t\t\t  # Normal case\n"
				"\t\t\t*) export SW_CONTROL_DIRECTORY=\"${SW_ROOT_DIRECTORY}/${SW_CATALOG}/%s/%s\"\n"
				"\t\t\t;;\n"
				"\t\tesac\n"
				"\t\t\tSWBCS_SCRIPT_NAME=\"%s\"\n"
				"\t\t\t;;\n",
				control_script->baseM.b_tagM,
				control_script->baseM.b_tagM,
				strob_str(vintbuf),
				SWINSTALL_INCAT_NAME,
				control_script->afileM->pathnameM,
				SWINSTALL_INCAT_NAME,
				control_script->afileM->pathnameM,
				name
				);
			*lslash = '/';
		} else {
			break;
		}
	}
	
	/*
	 * Now form the cases that are not used.
	 */
	make_case_pattern_for_unused_tags(buf);

	strob_sprintf(buf, STROB_DO_APPEND,
		"		*)\n"
		"			echo \"$0: invalid tag\" 1>&2\n"
		"			exit 1\n"
		"			;;\n"
		);

	strob_sprintf(buf, STROB_DO_APPEND,
		"		esac\n"
		);

	strob_close(vintbuf);
	return 0;
}

int
swi_afile_is_ieee_control_script(char * pathname)
{
	int ret;
	ret = set_is_ieee_control_script(pathname, 0);
	return ret;
}
