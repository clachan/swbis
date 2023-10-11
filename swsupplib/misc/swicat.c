/* swicat.c -- installed software catalog routines
   
   Copyright (C) 2004,2006,2007,2010 Jim Lowe
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

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
#include "swi.h"
#include "swi_xfile.h"
#include "swinstall.h"
#include "atomicio.h"
#include "ls_list.h"
#include "swverid.h"
#include "swevents.h"
#include "shlib.h"
#include "swicat.h"
#include "swicat_s.h"

/*  Query/Response Format made by swpg_get_catalog_entries()
Q0:P:swbis,r<0.585.20070324a
R0:P:...:
Q1:P:swbis,r==0.585.20070324a
R1:P:BP.:swbis:swbis                    r=0.585.20070324a       v=GNU   i=0   
Q2:P:swbis,r>0.585.20070324a
R2:P:...:
*/

static
SWICAT_PRE_SET *
swicat_pre_set_create(void)
{
	SWICAT_PRE_SET * pre_set = malloc(sizeof(SWICAT_PRE_SET));
	pre_set->preM = strob_open(12);
	pre_set->pre_fullfilledM = strar_open();
	return pre_set;	
}


static
SWICAT_EX_SET *
swicat_ex_set_create(void)
{
	SWICAT_EX_SET * ex_set = malloc(sizeof(SWICAT_EX_SET));
	ex_set->exM = strob_open(12);
	ex_set->ex_violatedM = strar_open();
	return ex_set;
}

static
void
swicat_pre_set_delete(SWICAT_PRE_SET * pre_set)
{
	strob_close(pre_set->preM);
	strar_close(pre_set->pre_fullfilledM);
}

static
char *
get_req_type(char * line)
{
	char * s;
	s = line;
	s = strchr(s, ':');
	if (!s) return s;
	s++;
	return s;
}

static
char *
check_requisite_response(char * line)
{
	char * s;
	s = line;
	/* Example	
	R1:P:BP.:abiword:abiword                r=1.0.2         v=6
		and an empty (non matching) response looks like:
	R1:P:...:
	*/
	s = strchr(s, ':'); if (!s) return s;	
	s++;
	s = strchr(s, ':'); if (!s) return s;	
	s++;
	s = strchr(s, ':'); if (!s) return s;	
	s++;
	s = strchr(s, ':'); if (!s) return s;	
	s++;
	return s;
}

static
void
add_path_code(STROB * buf)
{
	/* 
	 * aa="${aa:="$bb"}"
	 */
	strob_sprintf(buf, STROB_DO_APPEND,
	CSHID
	"export SWPATH\n"
	"gpath=$(getconf PATH)\n"
	"case \"$gpath\" in\n"
	"	\"\")\n"
	"		SWPATH=\"$PATH\"\n"
	"		;;\n"
	"	*)\n"
	"		SWPATH=\"${gpath}:${PATH}\"\n"
	"		;;\n"
	"esac\n"
	);
}

static		
char *
evaluate_query(GB * G, SWICAT_REQ * req, char * spec, STROB * text, char * first_line, int * err, SWICAT_SL * swicat_sl)
{
	char * line;
	char * s;
	int ret;
	char * type;
	STROB * tmp;
	int num;
	char * resp;
	SWICAT_PRE_SET * pre_set;
	SWICAT_EX_SET * ex_set;
	SWICAT_SC * sc;

	sc = NULL;	
	pre_set = NULL;
	ex_set = NULL;
	*err = 0;
	tmp = strob_open(12);
	line = first_line;
	s = line;
	s++;
	num = swlib_atoi(s, &ret);
	if (ret != 0) {
		*err = 1;	
		return NULL;
	}

	if (swicat_sl) {
		SWICAT_SQ * sq = swicat_sq_create();
		sc = swicat_sc_create();
		swicat_sq_parse(sq, line);
		swicat_sc_set_sq(sc, sq);
		swicat_sl_add_sc(swicat_sl, sc);
	}	
	
	type = get_req_type(line);
	if (type == NULL) {
		*err = 1;
		return NULL;
	}

	switch (*type) {
		case 'P':
			pre_set = swicat_pre_set_create();
			strob_strcpy(pre_set->preM, spec);
			vplob_add(req->pre_set_arrayM, pre_set);
			break;
		case 'E':
			ex_set = swicat_ex_set_create();
			strob_strcpy(ex_set->exM, spec);
			vplob_add(req->ex_set_arrayM, ex_set);
			break;
		case 'C':
			break;
		default:
			*err = 1;
			return NULL;
			break;			
	}

	line = strob_strtok(text, NULL, "\r\n");
	while(line) {
		E_DEBUG2("line=[%s]", line);
		if (swicat_sl) {
			SWICAT_SR * sr = swicat_sr_create();
			swicat_sr_parse(sr, line);	
			swicat_sc_add_sr(sc, sr);
		}
		resp = check_requisite_response(line);
		/* for exrequistes NULL is good,
		   for prerequistes NULL is bad */
		E_DEBUG2("resp=[%s]", resp);
		switch (*type) {
			case 'P':  /* prerequisites */
				if (resp == NULL) {
					strar_add(req->failed_preM, spec);
					req->pre_resultM = 1;
				} else {
					strar_add(pre_set->pre_fullfilledM, resp);
				}
				break;
			case 'E':  /* exrequisites */
				if (resp != NULL) {
					strar_add(ex_set->ex_violatedM, resp);
					req->ex_resultM = 1;
				} else {
					strar_add(req->passed_exM, spec);
				}
				break;
			case 'C':  /* corequisites */
				if (resp == NULL) {
					strar_add(req->failed_coM, spec);
					req->co_resultM = 1;
				}
				break;
		}

		line = strob_strtok(text, NULL, "\r\n");
		if (line && swlib_atoi(line+1, &ret) == num && ret == 0) {
			continue;
		} else {
			break;
		}
	}
	strob_close(tmp);	
	return line;
}

void
shpat_to_regex(STROB * buf, char * shell_pattern)
{
	char * s;
	int i;
	
	i = 0;
	strob_strcpy(buf, "");
	s = shell_pattern;
	strob_chr_index(buf, i++, (int)('^'));
	while (*s) {
		if (*s == '?') {
			strob_chr_index(buf, i++, (int)('.'));
		} else if (*s == '+') {
			strob_chr_index(buf, i++, (int)('\\'));
			strob_chr_index(buf, i++, (int)('+'));
		} else if (*s == '*') {
			strob_chr_index(buf, i++, (int)('.'));
			strob_chr_index(buf, i++, (int)('*'));
		} else {
			strob_chr_index(buf, i++, (int)(*s));
		}
		s++;
	}
	strob_chr_index(buf, i++, (int)('$'));
}

int
write_query_blob(GB * G, SWVERID * swverid, STROB * buf, STROB * tmp, int query_num, char * req_type)
{
	/*
	 * A query blob is a block of shell variable assignments
	 * similar to the example in ../shell_lib/test_isc_03.sh

		echo SWBIS_SOC_SPEC1="\"swbis\""
		echo SWBIS_SOC_SPEC2="\".*\""
		echo SWBIS_VENDOR_TAG="\".*\""
		echo SWBIS_REVISION_SPEC="\"0.600\""
		echo SWBIS_REVISION_RELOP="\">\""
		echo SWBIS_QUALIFIER_SPEC="\".*\""
		echo SWBIS_LOCATION_SPEC="\".*\""
		echo SWBIS_INSTANCE=0
	 */
	int i;
	char * s;
	char * awkregex;
	char * tmpval;
	struct VER_ID * verid;

	strob_sprintf(buf, 1, "SW_SOC_SPEC=\"%s\"\n", swverid->source_copyM);

	strob_sprintf(buf, 1, 
			"SWBIS_UTIL_NAME=\"%s\"\n"
			"SWBIS_CL_TARGET_TARGET=\"%s\"\n",
			swlib_utilname_get(),
			(G->g_cl_target_targetM != NULL ? G->g_cl_target_targetM: "(unset)")
			);
	
	/* set the defaults to include everything */
	strob_sprintf(buf, 1, "SWBIS_SOC_SPEC1=\".*\"\n");
	strob_sprintf(buf, 1, "SWBIS_SOC_SPEC2=\".*\"\n");

	i = 0;
	while ((s = cplob_val(swverid->taglistM, i))) {
		if (i < 2) {
			strob_strcpy(tmp, s);
			/* Now translate the shell pattern matching chars
			   to awk regex patterns. */
			shpat_to_regex(tmp, s);
			awkregex = strob_str(tmp);	
			strob_sprintf(buf, 1, "SWBIS_SOC_SPEC%d=\"%s\"\n", i+1, awkregex);
		} else {
			/* If the swspec is bundle.prod.fileset
			   i.e. more the 2, then just use the first two
			   because the third has to be a fileset or subproduct
			   and these are not matched by method used here. */
			;	
		}
		i++;
	}

	/* Now set the vendor tag */

	verid = swverid_get_verid(swverid, SWVERID_VERIDS_VENDOR_TAG, 1);
	if (verid == NULL) {
		strob_sprintf(buf, 1, "SWBIS_VENDOR_TAG=\".*\"\n");
	} else {
		tmpval = verid->valueM;
		shpat_to_regex(tmp, tmpval);
		strob_sprintf(buf, 1, "SWBIS_VENDOR_TAG=\"%s\"\n", strob_str(tmp));
	}

	if (req_type) 
		strob_sprintf(buf, 1, "SWBIS_REQ_TYPE=\"%s\"\n", req_type);

	if (query_num >= 0)
		strob_sprintf(buf, 1, "SWBIS_QUERY_NUM=\"%d\"\n", query_num);
	
	/* Now set the revsion and revision relop */

	verid = swverid_get_verid(swverid, SWVERID_VERIDS_REVISION, 1);
	if (verid == NULL) {
		strob_sprintf(buf, 1, "SWBIS_REVISION_SPEC=\".*\"\n");
	} else {
		tmpval = verid->valueM;
		if (strcmp(tmpval, "*") == 0) {
			strob_sprintf(buf, 1, "SWBIS_REVISION_SPEC=\".*\"\n");
		} else {
			strob_strcpy(tmp, tmpval);
			strob_sprintf(buf, 1, "SWBIS_REVISION_SPEC=\"%s\"\n", strob_str(tmp));
		}
		strob_sprintf(buf, 1, "SWBIS_REVISION_RELOP=\"%s\"\n", verid->rel_opM);
	}


	/* Now set the second revsion and revision relop */
	verid = swverid_get_verid(swverid, SWVERID_VERIDS_REVISION, 2);
	if (verid == NULL) {
		;
	} else {
		/* A second revision such as in a bracketed range */
		tmpval = verid->valueM;
		if (strcmp(tmpval, "*") == 0) {
			strob_sprintf(buf, 1, "SWBIS_REVISION2_SPEC=\".*\"\n");
		} else {
			strob_strcpy(tmp, tmpval);
			strob_sprintf(buf, 1, "SWBIS_REVISION2_SPEC=\"%s\"\n", strob_str(tmp));
		}
		strob_sprintf(buf, 1, "SWBIS_REVISION2_RELOP=\"%s\"\n", verid->rel_opM);
	}

	verid = swverid_get_verid(swverid, SWVERID_VERIDS_CATALOG_INSTANCE, 1);
	if (verid == NULL) {
		tmpval = "*";
	} else {
		tmpval = verid->valueM;
	}	
	shpat_to_regex(tmp, tmpval);
	strob_sprintf(buf, 1, "SWBIS_INSTANCE=\"%s\"\n", strob_str(tmp));

	verid = swverid_get_verid(swverid, SWVERID_VERIDS_LOCATION, 1);
	if (verid == NULL) {
		tmpval = "*";
	} else {
		tmpval = verid->valueM;
	}	
	if (strcmp(tmpval, "/") == 0 || strlen(tmpval) == 0) {
		strob_sprintf(buf, 1, "SWBIS_LOCATION_SPEC=\"^/$|^$\"\n");
	} else {
		shpat_to_regex(tmp, tmpval);
		strob_sprintf(buf, 1, "SWBIS_LOCATION_SPEC=\"%s\"\n", strob_str(tmp));
	}

	verid = swverid_get_verid(swverid, SWVERID_VERIDS_QUALIFIER, 1);
	if (verid == NULL) {
		strob_sprintf(buf, 1, "SWBIS_QUALIFIER_SPEC=\".*\"\n");
	} else {
		tmpval = verid->valueM;
		shpat_to_regex(tmp, tmpval);
		strob_sprintf(buf, 1, "SWBIS_QUALIFIER_SPEC=\"%s\"\n", strob_str(tmp));
	}

	return 0;
}

static
void
write_shell_assign1(STROB * buf, char * varname, char * value)
{
	/* 
	 * aa="${aa:="$bb"}"
	 */
	strob_sprintf(buf, STROB_DO_APPEND, "%s=\"${%s:=\"%s\"}\"\n", varname, varname, value);
}


static
void
write_swbis_env_vars(STROB * buf, SWI * swi)
{
	return;
}

static
void
form_absolute_control_dir(STROB * tmp, SWI * swi, char * control_script_pkg_dir, char ** script_name)
{
	char * s;

	strob_strcpy(tmp, swi->swi_pkgM->target_pathM);
	swlib_unix_dircat(tmp, swi->swi_pkgM->catalog_entryM);
	swlib_unix_dircat(tmp, SWINSTALL_INCAT_NAME);
	swlib_unix_dircat(tmp, control_script_pkg_dir);

	/*
	 * Now make some sanity checks
	 */
	swlib_squash_trailing_slash(strob_str(tmp));
	swlib_squash_double_slash(strob_str(tmp));
	if (swlib_check_clean_absolute_path(strob_str(tmp)))
		SWLIB_FATAL("internal error: form_absolute_control_dir-1");

	/*
	 * Now take the dirname of this.
	 */
	s = strrchr(strob_str(tmp), '/');
	if (s) {
		*s = '\0';
		s++;
		*script_name = s;
	} else {
		SWLIB_FATAL("internal error: form_absolute_control_dir-2");
	}
	return;
}

static
void 
write_state(STROB * buf, SWI_XFILE * xx, int do_if_active)
{
	if (do_if_active && !(xx->baseM.is_activeM)) return;
	if (strcmp(xx->stateM, SW_STATE_UNSET)) {
		strob_sprintf(buf, STROB_DO_APPEND,  "    " SW_A_state " %s\n", xx->stateM);
	}
	return;
}

static
void 
write_times(STROB * buf, SWI_BASE * xx, int do_if_active)
{
	time_t caltime;
	
	/*if (do_if_active && !(xx->is_activeM)) return;
	*/
	caltime = (time_t)(xx->create_timeM);
	strob_sprintf(buf, STROB_DO_APPEND, "    " SW_A_create_time " %lu # %s", (unsigned long)(xx->create_timeM), ctime(&caltime));
	caltime = (time_t)(xx->mod_timeM);
	strob_sprintf(buf, STROB_DO_APPEND, "    " SW_A_mod_time " %lu # %s", (unsigned long)(xx->mod_timeM), ctime(&caltime));
}

static
void
add_env_entry_localdefault(char * name, char * value, STROB * buf)
{
	char * s;
	if (value) {
		s = value;
	} else {
		s = getenv(name);
	}
	if (!s) s="";
	write_shell_assign1(buf, name, s);
}

static
void
add_env_entry(char  * name, char * value, STROB * buf)
{
	char * s;

	if (value) {
		s = value;
	} else {
		s = getenv(name);
	}
	if (s) {
		strob_sprintf(buf, STROB_DO_APPEND, "%s=\"%s\"\n", name, s); 
		strob_sprintf(buf, STROB_DO_APPEND, "export %s\n", name); 
	}
}

static
int
write_dependency_assertion_script(SWI * swi, SWVERID * dep_spec, STROB * buf)
{
	return -1;
}

static
int
squash_null_block(char * start, int len)
{
	char * first_null;
	char * s;
	int done;
	int nn;

	if (len <= 0) return 0;
	first_null = start + strlen(start);
	s = first_null;
	nn = 0;
	while (
		(s - start) < len &&
		*s == '\0'
	) {
		nn++;
		s++;
	}
	done = (int)(s - start);
	if ( done >= len) {
		/* finished, the whole file is NUL clean */
		return 0;
	}
	memmove(first_null, s, ((char *)(start+len)) - s);
	memset(start + (len - nn), '\0', nn);
	return nn;
}

static
void
write_attributes(STROB * buf, SWHEADER * swheader, int index)
{
	char * line;
	int level;
	char * keyword;
	char * value;

	swheader_store_state(swheader, NULL);
	swheader_set_current_offset(swheader, index);
	while((line=swheader_get_next_attribute(swheader))) {
		keyword = swheaderline_get_keyword(line);
		value = swheaderline_get_value(line, (int *)NULL);
		level = swheaderline_get_level(line);
		swdef_write_keyword_to_buffer(buf, keyword, level, SWPARSE_MD_TYPE_ATT);
		swdef_write_value_to_buffer(buf, value);
	}
	swheader_restore_state(swheader, NULL);
}

void
write_isf_excludes(STROB * ibuf, SWI * swi)
{
	int fd;
	int n;
	char * s;
	STROB * buf;
	fd = swi->pending_file_conflicts_fdM;
	if (fd < 0) return;
	buf = strob_open(32);
	uxfio_lseek(fd, 0, SEEK_SET);
	while ((n=swgp_read_line(fd, (STROB*)buf, DO_NOT_APPEND)) > 0) {
		s = strob_str(buf);
		while(swlib_squash_trailing_char(s, '\n') == 0);
		strob_sprintf(ibuf, 0, SW_A_excluded_from_install " " "\"%s\"\n", s);
	}
	strob_close(buf);
}

int
swicat_write_installed_software(SWI * swi, int ofd)
{
	/*
	* Write the installed software file
	*
	* Example:

	installed_software
	path /
	catalog var/lib/swbis/catalog

	product
	create_time 1059628779
	mod_time 1059629009
	all_filesets BIN DOC
	filesets BIN DOC
	create_time 1059628779
	mod_time 1059629009
	tag foo

	control_file
	tag postinstall
	result 0

	fileset
	create_time 1059628779
	mod_time 1059629009
	tag BIN
	state installed

	fileset
	tag DOC
	create_time 1059628779
	mod_time 1059629009
	state installed
	*/
	STROB * buf = strob_open(10);
	int ret;
	swicat_isf_installed_software(buf, swi);
	ret = atomicio((ssize_t (*)(int, void *, size_t))write,
				ofd, (void*)(strob_str(buf)), strob_strlen(buf));
	strob_close(buf);
	return ret;
}

int
swicat_isf_all_scripts(STROB * buf, SWI_SCRIPTS * xx, int do_if_active)
{
	int i = 0;
	while(i < SWI_MAX_OBJ && xx->swi_coM[i]) {
		swicat_isf_control_script(buf, xx->swi_coM[i], do_if_active);
		i++;
	}
	return 0;
}

int
swicat_isf_control_script(STROB * buf, SWI_CONTROL_SCRIPT * xx, int do_if_active)
{
	strob_sprintf(buf, STROB_DO_APPEND, SW_A_control_file "\n");
	strob_sprintf(buf, STROB_DO_APPEND, "\t" SW_A_tag " %s\n", xx->baseM.b_tagM);
	strob_sprintf(buf, STROB_DO_APPEND, "\t" SW_A_result " %s\n", swi_control_script_posix_result(xx));
	if (xx->resultM >= 0)
		strob_sprintf(buf, STROB_DO_APPEND, "\t" SW_A_return_code " %d\n", xx->resultM);
	return 0;
}

int
swicat_isf_fileset(SWI * swi, STROB * buf, SWI_XFILE * xfile, int do_if_active)
{
	strob_sprintf(buf, STROB_DO_APPEND, SW_A_fileset "\n");
	strob_sprintf(buf, STROB_DO_APPEND, "    " SW_A_location " %s\n", swi->swi_pkgM->locationM ? swi->swi_pkgM->locationM:"/");
	write_attributes(buf, SWI_INDEX_HEADER(swi), xfile->baseM.header_indexM);
	write_state(buf, xfile, do_if_active);
	write_times(buf, (SWI_BASE *)&(xfile->baseM), 1 /* do_if_active */);
	swicat_isf_all_scripts(buf, xfile->swi_scM, do_if_active);
	write_isf_excludes(buf, swi);
	strob_sprintf(buf, STROB_DO_APPEND, "\n");
	return 0;
}

int
swicat_isf_product(SWI * swi, STROB * buf, SWI_PRODUCT * prod, int do_if_active)
{
	int i = 0;
	SWHEADER * swheader;

	swheader = SWI_INDEX_HEADER(swi);
	strob_sprintf(buf, STROB_DO_APPEND, SW_A_product "\n");
	strob_sprintf(buf, STROB_DO_APPEND, SW_A_location " %s\n", swi->swi_pkgM->locationM ? swi->swi_pkgM->locationM:"/");
		
	write_attributes(buf, SWI_INDEX_HEADER(swi), prod->p_baseM.header_indexM);

	write_times(buf, (SWI_BASE *)&(prod->p_baseM), 1 /* do_if_active */);

	/* ------------------------------------------------------ */
	/* ------------------------------------------------------ */
	/* ------------------------------------------------------ */
	/*
	{
		swheader_store_state(swheader, NULL);
		swheader_set_current_offset(swheader, prod->p_baseM.header_indexM);
		while((line=swheader_get_next_attribute(swheader)))
			swheaderline_write_debug(line, STDERR_FILENO);
		swheader_restore_state(swheader, NULL);
	}
	*/
	/* ------------------------------------------------------ */
	/* ------------------------------------------------------ */
	/* ------------------------------------------------------ */

	swicat_isf_all_scripts(buf, prod->xfileM->swi_scM, do_if_active);
	while(i < SWI_MAX_OBJ && prod->swi_coM[i]) {
		swicat_isf_fileset(swi, buf, prod->swi_coM[i], do_if_active);
		i++;
	}
	return 0;
}

int
swicat_isf_installed_software(STROB * buf, SWI * swi)
{
	SWI_PRODUCT * prod;

	prod = swi->swi_pkgM->swi_coM[0];
	strob_sprintf(buf, STROB_DO_APPEND, SW_A_installed_software "\n");
	swicat_isf_product(swi, buf, prod, 1);
	if (swi->swi_pkgM->swi_coM[1]) {
		fprintf(stderr, "multiple products not supported\n");
		return 1;
	}
	return 0;
}

int
swicat_env(STROB * buf, SWI * swi, char * control_script_pkg_dir, char * tag)
{
	STROB * tmp = strob_open(10);
	char * tmp_s;

	add_env_entry("LC_CTYPE", "", buf);
	add_env_entry("LC_MESSAGES", "", buf);
	add_env_entry("LC_TIME", "", buf);
	add_env_entry("TZ", "", buf);
	add_env_entry("LANG", "C", buf);
	add_env_entry("LC_ALL", "C", buf);

	if (swi->swi_pkgM->target_pathM) {
		add_env_entry("SW_ROOT_DIRECTORY", swi->swi_pkgM->target_pathM, buf);
	} else {
		fprintf(stderr, "warning: swi->swi_pkgM->target_pathM is null, using /\n");
		add_env_entry("SW_ROOT_DIRECTORY", "/", buf);
	}

	add_env_entry("SW_PATH", "$PATH", buf);

	/*
	 * SWBIS_CATALOG_ENTRY is what is after the <installed_software_catalog> path
	 * fragment
	 */

	SWLIB_ASSERT(strlen(swi->swi_pkgM->installed_software_catalogM));
	tmp_s = strstr(swi->swi_pkgM->catalog_entryM, swi->swi_pkgM->installed_software_catalogM);

	SWLIB_ASSERT(tmp_s != NULL);
	tmp_s += strlen(swi->swi_pkgM->installed_software_catalogM);
	while(*tmp_s == '/') tmp_s++;

	add_env_entry("SWBIS_CATALOG_ENTRY", tmp_s, buf);

	/*
	 * Wrong.. SW_CATALOG same as installed_software.catalog and is a 
	 * Wrong.. a relative path (It is not the installed_software_catalog option value)
	 */
	/* Actually, SW_CATALOG is really the path such as, for example:

		 var/lib/swbis/catalog/foo/foo/1.1/0/

	   prior to version 1.12, the value of SW_CATALOG was set by the following line:
		add_env_entry("SW_CATALOG", swi->swi_pkgM->installed_software_catalogM, buf);
	*/

	add_env_entry("SW_CATALOG", swi->swi_pkgM->catalog_entryM, buf);

	SWLIB_ASSERT(swi->swi_pkgM->locationM != NULL);
	add_env_entry("SW_LOCATION", swi->swi_pkgM->locationM, buf);
	
	add_path_code(buf);

	if (*(swi->swi_pkgM->installed_software_catalogM) == '/') {
		/* The installed_software_catalog was specified as 
		   an absolute path */
		add_env_entry("SW_SESSION_OPTIONS", "${SW_CATALOG}/" SW_A_session_options , buf);
	} else {
		add_env_entry("SW_SESSION_OPTIONS", "${SW_ROOT_DIRECTORY}/${SW_CATALOG}/" SW_A_session_options , buf);
	}


	/* add SW_SOFTWARE_SPEC */

	/* FIXME, don't assume first product */
	/* FIXME , add tag for fileset when writing the fileset */
	
	/* This is a provisional value for  SW_SOFTWARE_SPEC at the top
	   of the file.  It is defined again before each script invocation
	   so as to reflect the tag of the fileset */

	strob_strcpy(tmp, "");
	swverid_print(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM, tmp);
	add_env_entry("SW_SOFTWARE_SPEC", strob_str(tmp), buf);

	strob_strcat(buf, ". \"${SW_SESSION_OPTIONS}\"\n");	

	strob_close(tmp);
	return 0;
}

int
swicat_make_options_file(STROB * buf)
{
	strob_strcpy(buf, "");	
	return 0;
}

void
swicat_write_auto_comment(STROB * buf, char * filename)
{
        time_t caltime;
        time(&caltime);
        strob_sprintf(buf, STROB_DO_APPEND,
                "# %s\n"
                "# Automatically generated by %s version %s on %s"
                "#\n",
                filename,
		swlib_utilname_get(), SWBIS_RELEASE,
		ctime(&caltime));
}

void
swicat_construct_controlsh_taglist(SWI * swi, char * sw_selections, STROB * list)
{
	int i;
	int j;
	SWI_PACKAGE * package;
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	STROB * tmp;

	tmp = strob_open(100);
	strob_strcpy(list, "");

	package = swi->swi_pkgM;
	for(i=0; i<SWI_MAX_OBJ; i++) {
		product = package->swi_coM[i];
		if (product) {
			SWLIB_ASSERT(product->p_baseM.b_tagM != NULL);
			strob_strcpy(tmp, product->p_baseM.b_tagM);
			if (swlib_check_clean_path(product->p_baseM.b_tagM)) SWLIB_ASSERT(0);
			strob_sprintf(list, STROB_DO_APPEND, "%s", strob_str(tmp));
			for(j=0; j<SWI_MAX_OBJ; j++) {
				fileset = product->swi_coM[j];
				if (fileset) {
					SWLIB_ASSERT(fileset->baseM.b_tagM != NULL);
					if (swlib_check_clean_path(fileset->baseM.b_tagM)) SWLIB_ASSERT(0);
					strob_strcpy(tmp, product->p_baseM.b_tagM);
					strob_strcat(tmp, ".");
					strob_strcat(tmp, fileset->baseM.b_tagM);
					strob_sprintf(list, STROB_DO_APPEND, " %s", strob_str(tmp));
				} else break;
			}
		} else break;
		strob_sprintf(list, STROB_DO_APPEND, " ");
	}
	strob_close(tmp);
}

int
swicat_write_script_cases(SWI * swi, STROB * buf, char * sw_selection)
{
	int ret;
	char * product_tag;
	char * fileset_tag;
	SWVERID * swverid_spec;
	SWVERID * swverid;
	CPLOB * taglist;
	SWI_PRODUCT * product;
	SWI_XFILE * xfile;
	SWI_SCRIPTS * scripts;
	STROB * tmp;


	tmp = strob_open(32);
	/*
	 * the first step is to find the object implied by
	 * sw_selection.  sw_selection has the form:
	 *     <prod_tag>.<fileset_tag>
	 */

	E_DEBUG2("sw_selection=[%s]", sw_selection);
	/* E_DEBUG2("[%s]", swi_dump_string_s(swi, "AAAAA")); */

	swverid = swverid_open(SW_A_product, NULL);
	taglist = swverid_u_parse_swspec(swverid, sw_selection);
	E_DEBUG2("swverid_print = [%s]", swverid_show_object_debug(swverid, NULL, ""));

	/* NOT
	 * do a sanity check,
	 * only 2 dot delimited fields are allowed.
	 *	
	if (cplob_get_nused(taglist) > 3) {
		SWLIB_ASSERT(0);
	}		
	*/

	product_tag = cplob_val(taglist, 0);
	fileset_tag = cplob_val(taglist, 1);  /* may be NUL */

	/* FIXME
	 * Assumes one fileset
	 */

	product = swi_find_product_by_swsel(swi->swi_pkgM, product_tag, NULL, (int*)NULL);
	SWLIB_ASSERT(product != NULL);   /* fatal */

	if (fileset_tag) {
		xfile = swi_find_fileset_by_swsel(product, fileset_tag, NULL);
	} else {
		/*
		 * the scripts reside in the pfiles object
		 */
		xfile = product->xfileM;
	}
	SWLIB_ASSERT(xfile != NULL);   /* fatal */
	scripts = xfile->swi_scM;
	SWLIB_ASSERT(scripts != NULL);   /* fatal */

	/* make a copy of the swverid to avoid harming the original */

	swverid_spec = swverid_copy(SWI_GET_PRODUCT(swi, 0)->p_baseM.swveridM);

	if (fileset_tag) {
		/* add fileset_tag to the '.' deimited list */
		cplob_add_nta(swverid_spec->taglistM, strdup(fileset_tag));
	}
	
	swverid_print(swverid_spec, tmp);

	/*
	 * OK, we now have the scripts for the product or fileset
	 * Loop over them and write the shell code fragment for each
	 * POSIX script.
	 */
	strob_sprintf(buf, STROB_DO_APPEND,
		"\t#\n"
		"\t# write script cases here\n"
		"\t#\n");
	
	add_env_entry("SW_SOFTWARE_SPEC", strob_str(tmp), buf);
	
	ret = swi_afile_write_script_cases(scripts,  buf, swi->swi_pkgM->installed_software_catalogM);
	ret = 0;
	SWLIB_ASSERT(ret == 0);   /* fatal */

	swverid_close(swverid);
	swverid_close(swverid_spec);
	cplob_close(taglist);
	strob_close(tmp);
	return 0;
}


int
swicat_r_get_installed_catalog(GB * G, VPLOB * swspecs, char * target_path)
{

	/* the main target script should now be waiting at a "bash -s" command.
	   We need to write a task script that gets the portion of installed
	   software catalog */

        return -1;
}

/*
 * Write the task script for getting a list of catalog_entries or
 * a tar archive of those entries based on the software selections
 * in swspecs.
 *
 * Return:
 *	0: success, script_buf has script
 *     -1: error
 */

int
swicat_write_isc_script(STROB * script_buf,
		GB * G,
		VPLOB * swspecs,
		VPLOB * swspecs_pre,
		VPLOB * swspecs_co,
		VPLOB * swspecs_ex,
		char * list_vform)
{
	int query_num;
	int i;
	SWVERID * swverid;
	STROB * tmp;
	STROB * shell_lib_buf;
	char * list_lform;
	char * isc_path;

	tmp = strob_open(1);
	shell_lib_buf = strob_open(1);
	strob_strcpy(script_buf, "");

	isc_path = get_opta_isc(G->optaM, SW_E_installed_software_catalog);

	if (strcmp(list_vform, SWICAT_FORM_TAR1) == 0) {
		/* Create a tar archive of the selections */

		/* lie, set the form so that a list of directories will be
		   printed to be used by a archive creation function as the list of
		   directories to archive */
		list_lform = SWICAT_FORM_DIR1;
	} else {
		/* Use the form specified by the argument */
		list_lform = list_vform;
	}

	strob_sprintf(script_buf, STROB_DO_APPEND,
		CSHID
		"export sw_retval\n"
		"export SW_SOC_SPEC\n"
		"export SWBIS_SOC_SPEC1\n"
		"export SWBIS_SOC_SPEC2\n"
		"export SWBIS_VENDOR_TAG\n"
		"export SWBIS_REVISION_SPEC\n"
		"export SWBIS_REVISION_RELOP\n"
		"export SWBIS_REVISION2_SPEC\n"
		"export SWBIS_REVISION2_RELOP\n"
		"export SWBIS_LOCATION_SPEC\n"
		"export SWBIS_QUALIFIER_SPEC\n"
		"export SWBIS_INSTANCE\n"
		"export SWBIS_QUERY_NUM\n"
		"export SWBIS_LIST_FORM\n"
		"export SWBIS_REQ_TYPE\n"
		"export SWBIS_UTIL_NAME\n"
		"export SWBIS_CL_TARGET_TARGET\n"
		"%s\n"
		"%s\n"
		"%s\n",
		shlib_get_function_text_by_name("shls_false_", tmp, NULL),
		shlib_get_function_text_by_name("shls_apply_socspec", tmp, NULL),
		shlib_get_function_text_by_name("shls_get_verid_list", tmp, NULL)
		);
			
	/* put shell functions here needed for writing the tar format here */
	if (strcmp(list_vform, SWICAT_FORM_TAR1) == 0) {
		strob_sprintf(script_buf, 1,
			CSHID
			"%s\n"
			"%s\n"
			"%s\n"
			"%s\n"
			,
			shlib_get_function_text_by_name("shls_false_", shell_lib_buf, NULL),
			shlib_get_function_text_by_name("shls_check_for_gnu_tar", shell_lib_buf, NULL),
			shlib_get_function_text_by_name("shls_missing_which", shell_lib_buf, NULL),
			shlib_get_function_text_by_name("shls_write_cat_ar", shell_lib_buf, NULL)
			);
	}

	/* Now loop over swspecs and make a query blob
	   for each swspec.  A query blob is a block of
	   shell variables that form the interface to the
	   selection routines from shell_lib/shell_lib.sh */

	/* Beginning subshell */

	if (isc_path)
		strob_sprintf(script_buf, 1, "cd \"%s\"\n", isc_path);
	else
		strob_sprintf(script_buf, 1, "(exit 1);\n");

	strob_sprintf(script_buf, 1, 
	"case $? in\n"
	"	0)\n"
	);

	E_DEBUG("");
	strob_sprintf(script_buf, 1, 
			"sw_retval=0\n"
			"(\n"		/* subshell */
			);


	/* Set the LIST_FORM variable. This set the output form */
	strob_sprintf(script_buf, 1, 
			"SWBIS_LIST_FORM=\"%s\"\n",
			list_lform);

	/* Following line reads the block of NULs for this task
	   the a block of NULs is sent is that some dd's can't successfully
	   read 0 blocks, hence all task scripts read atleast 1 block,
	   even task scripts that require no input. */

	strob_sprintf(script_buf, 1, "dd bs=512 count=1 of=/dev/null 2>/dev/null\n");

	i=0;
	strob_sprintf(script_buf, 1, "(\n");		/* subshell */

	if (swspecs) {
		/* write listing or query blob */
		E_DEBUG("");
		if (vplob_get_nstore(swspecs) <= 0) {
			/* add a "*" spec */
			E_DEBUG("");
			swverid = swverid_open(NULL, "*");
			if (swverid == NULL) {
				return -1;
			} else {
				vplob_add(swspecs, (void*)swverid);
			}
		}

		E_DEBUG("Looping over swspecs");
		while ((swverid=vplob_val(swspecs, i++)) != NULL) {
			E_DEBUG2("in swpec number [%d]", i);
			write_query_blob(G, swverid, script_buf, tmp, -1, NULL);
	
			/* Here is the command that reads the shell variables */
			strob_sprintf(script_buf, 1,
				"shls_get_verid_list | shls_apply_socspec\n"
				"\n");
		}
	} else if (swspecs_pre || swspecs_co || swspecs_ex) {
		/* write requisite checks blobs */
		E_DEBUG("");
		query_num = 0;
		i = 0;
		if (swspecs_pre) {
			E_DEBUG("have swspecs_pre");
			while ((swverid=vplob_val(swspecs_pre, i++)) != NULL) {
				E_DEBUG2("in swpec number [%d]", i);
				write_query_blob(G, swverid, script_buf, tmp, query_num, SWICAT_REQ_TYPE_P);
				strob_sprintf(script_buf, 1, "shls_get_verid_list | shls_apply_socspec\n" "\n");
				query_num++;
			}
		}
		i=0;
		if (swspecs_co) {
			E_DEBUG("have swspecs_co");
			while ((swverid=vplob_val(swspecs_co, i++)) != NULL) {
				E_DEBUG2("in swpec number [%d]", i);
				write_query_blob(G, swverid, script_buf, tmp, query_num, SWICAT_REQ_TYPE_C);
				strob_sprintf(script_buf, 1, "shls_get_verid_list | shls_apply_socspec\n" "\n");
				query_num++;
			}
		}	
		i=0;
		if (swspecs_ex) {
			E_DEBUG("have swspecs_ex");
			while ((swverid=vplob_val(swspecs_ex, i++)) != NULL) {
				E_DEBUG2("in swpec number [%d]", i);
				write_query_blob(G, swverid, script_buf, tmp, query_num, SWICAT_REQ_TYPE_E);
				strob_sprintf(script_buf, 1, "shls_get_verid_list | shls_apply_socspec\n" "\n");
				query_num++;
			}
		}
		if (query_num == 0) {
			E_DEBUG("have no blobs whatsoever");
			/* here NO query blobs were written, which means we owe the script syntax
				a command (symmetric with 
					shls_get_verid_list | shls_apply_socspec

				Use dd if=/dev/null 2>/dev/null
			*/
			strob_sprintf(script_buf, 1, "dd if=/dev/null 2>/dev/null\n");
		}
	} else {
		/* This should never happen */
		sw_e_msg(G, "internal error in swicat_write_isc_script()\n");
		;
		E_DEBUG("");

	}

	/* echo TRAILER!!! as a terminator so that a reading process on the
	   management host knows when to stop reading. */

	E_DEBUG("");
	if (strcmp(list_vform, SWICAT_FORM_TAR1) == 0) {
		strob_sprintf(script_buf, 1,
			")\n"
			") | shls_write_cat_ar\n");
	} else {
		swlib_append_synct_eof(script_buf);
	}

	strob_sprintf(script_buf, 1, 
	";;\n"
	"	*)\n"
	"	echo \"internal error: bad installed_software_catalog path in swicat_write_isc_script\"\n"
	"	sw_retval=1\n"
	"	;;\n"
	"esac\n"
	);

	strob_close(tmp);
	strob_close(shell_lib_buf);
	return 0;
}

int
swicat_squash_null_bytes(int fd)
{
	int ret;
	off_t len;
	off_t current_pos;
	char * mem;
	int nn;
	off_t newlen;

	/* squash interjected NULs caused by dd used with
	   the sync conversion such as dd obs=512 conv=sync */

	current_pos = uxfio_lseek(fd, 0, SEEK_CUR);
        SWLIB_ASSERT(current_pos >= 0);
	len = uxfio_lseek(fd, (off_t)0, SEEK_END);
        SWLIB_ASSERT(len >= 0);
	mem = (char*)uxfio_get_fd_mem(fd, (int *)NULL);
        SWLIB_ASSERT(mem != NULL);

	nn = 0;
	newlen = len;
	while((nn=squash_null_block(mem, (int)newlen)) > 0) {
		newlen -= nn;
	}
	if (nn < 0) {
		 return -1;
	}
	ret = uxfio_lseek(fd, current_pos, SEEK_SET);
        SWLIB_ASSERT(ret >= 0);
	ret = uxfio_ftruncate(fd, strlen(mem));
        SWLIB_ASSERT(ret >= 0);
	return 0;
}

int
swicat_req_get_pre_result(SWICAT_REQ * req)
{
	return req->pre_resultM;
}

int
swicat_req_get_ex_result(SWICAT_REQ * req)
{
	return req->ex_resultM;
}

int
swicat_req_print(GB * G, SWICAT_REQ * req)
{
	int i;
	int nn;
	char *s;
	if (req->pre_resultM != 0) {
		/* failed dependencies */
		i = 0;
		while((s=strar_get(req->failed_preM, i++)))
			sw_l_msg(G, SWC_VERBOSE_1, "prereqisite failed: %s\n", s);
	} else {
		/* fullfilled dependencies */
		SWICAT_PRE_SET * pre_set;
		char * pre;
		i = 0;
		while((pre_set=(SWICAT_PRE_SET *)vplob_val(req->pre_set_arrayM, i++))) {
			nn = 0;	
			while((pre=strar_get(pre_set->pre_fullfilledM, nn++))) {	
				sw_l_msg(G, SWC_VERBOSE_1, "prereqisite '%s' filled by: %s:\n",
					strob_str(pre_set->preM), pre);
			}
		}
	}

	if (req->ex_resultM != 0) {
		/* failed exrequisites */
		SWICAT_EX_SET * ex_set;
		char * ex;
		i = 0;
		while((ex_set=(SWICAT_EX_SET *)vplob_val(req->ex_set_arrayM, i++))) {
			nn = 0;	
			while((ex=strar_get(ex_set->ex_violatedM, nn++))) {	
				sw_l_msg(G, SWC_VERBOSE_1, "exreqisite '%s' failed: %s:\n",
					strob_str(ex_set->exM), ex);
			}
		}
	} else {
		/* succeded exrequisites */
		i = 0;
		while((s=strar_get(req->passed_exM, i++)))
			sw_l_msg(G, SWC_VERBOSE_3, "exrereqisite passed: %s\n", s);
	}

	return 0;
}

int
swicat_req_analyze(GB * G, SWICAT_REQ * req, char * query_text, SWICAT_SL ** swicat_sl_p)
{
	/*
	Form of result format: colon delimited line of ascii text

	Field 1:  {Q|R}N   Query or Response with Id Number
	Field 2:  {P|C|E}  Prerequisite Corequisite or Exrequisite
	Field 3:  {B|.}{P|.}{F|.}   Which object matched, Bundle, Product or Fileset
	Field 4:  Bundle Tag
	Field 5:  Entry 

	Example of result format:
	
	Q0:P:sddf*,r>3
	R0:P:...:
	Q1:P:ab*,r>=1.0
	R1:P:BP.:abiword:abiword                r=1.0.2         v=6
	*/

	char * line;
	STROB * text;
	STROB * rec;
	int err;
	char * spec;
	SWICAT_SL * swicat_sl;

	E_DEBUG2("query text=[%s]", query_text);

	text = strob_open(10);
	if (swicat_sl_p) {
		*swicat_sl_p = NULL;
		swicat_sl = swicat_sl_create();
	} else {
		swicat_sl = NULL;
	}
	err = 0;
	line = strob_strtok(text, query_text, "\r\n");
	while(line) {
		if (*line != 'Q') {
			/* internal error */
			return -1;
		}
		/* evaluate_query() returns the first line of the
		   next query */
		spec = line;
		spec = strchr(spec, ':'); if (spec == NULL) return -1;	
		spec = strchr(++spec, ':'); if (spec == NULL) return -1;	
		spec++;
		/* spec now points to the software_spec */
		line = evaluate_query(G, req, spec, text, line, &err, swicat_sl);
	}
	strob_close(text);
	if (err != 0) {
		if (swicat_sl)
			swicat_sl_delete(swicat_sl);
		if (swicat_sl_p)
			*swicat_sl_p = NULL;
		return 1;
	}
	if (swicat_sl_p)
		*swicat_sl_p = swicat_sl;
	return 0;
}

SWICAT_REQ *
swicat_req_create(void)
{
	SWICAT_REQ * req;
	req = (SWICAT_REQ*)malloc(sizeof(SWICAT_REQ));
	req->failed_preM = strar_open();
	req->failed_coM = strar_open();
	req->passed_exM = strar_open();
	req->pre_resultM = 0;
	req->ex_resultM = 0;
	req->co_resultM = 0;
	req->ex_set_arrayM = vplob_open();
	req->pre_set_arrayM = vplob_open();
	req->slM = NULL;
	return req;
}

void
swicat_req_delete(SWICAT_REQ * req)
{
	strar_close(req->failed_preM);
	strar_close(req->failed_coM);
	strar_close(req->passed_exM);
	vplob_close(req->ex_set_arrayM);
	vplob_close(req->pre_set_arrayM);
	free(req);
}


#ifdef VMAIN
#include "swmain.h"
int
main(int argc, char **argv)
{
	char *s;
	char nuls[100];
	int fd = swlib_open_memfd();
	
	memset(nuls, '\0', sizeof(nuls));
	
	uxfio_write(fd, nuls, 1);
	s="abcdef";
	uxfio_write(fd, s, strlen(s));
	uxfio_write(fd, nuls, 4);
	s="ghijklmnop";
	uxfio_write(fd, s, strlen(s));

	uxfio_write(fd, nuls, 10);

	s="qrstuvwxy";
	uxfio_write(fd, s, strlen(s));
	uxfio_write(fd, nuls, 30);

	s="z";
	uxfio_write(fd, s, strlen(s));
	uxfio_write(fd, nuls, 18);

	swicat_squash_null_bytes(fd);
	fprintf(stdout, "%s\n", (char*)uxfio_get_fd_mem(fd, (int *)NULL));
	
}
#endif
