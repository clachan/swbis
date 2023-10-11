/* swverify_lib.c -- routines for iswverify.

 Copyright (C) 2007,2008,2009,2010,2014 James H. Lowe, Jr. 
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
#include "swverify_lib.h"
#include "ls_list.h"

#define SWREMOVE_ARG_MAX_THRESHOLD 900

static INFOLINE AA;  /* dummy object used for sizing */


/*
#define INL_NAME_OFFSET			0
#define INL_LINKNAME_OFFSET		((int)((char*)(&(AA.linknameM)) - ((char*)(&AA))))
#define INL_SIZE_OFFSET 		((int)((char*)(&(AA.sizeM)) - ((char*)(&AA))))
#define INL_PERMISSIONS_OFFSET 		((int)((char*)(&(AA.permissionsM)) - ((char*)(&AA))))
#define INL_OWNERS_OFFSET 		((int)((char*)(&(AA.ownersM)) - ((char*)(&AA))))
#define INL_MDATE_OFFSET 		((int)((char*)(&(AA.mdateM)) - ((char*)(&AA))))
#define INL_MD5_OFFSET 			((int)((char*)(&(AA.md5M)) - ((char*)(&AA))))
#define INL_SHA1_OFFSET 		((int)((char*)(&(AA.sha1M)) - ((char*)(&AA))))
#define INL_SHA512_OFFSET 		((int)((char*)(&(AA.sha512M)) - ((char*)(&AA))))

#define RPMV_offset_size	0
#define RPMV_offset_mode	1	
#define RPMV_offset_digest	2	
#define RPMV_offset_device	3	
#define RPMV_offset_linkname	4	
#define RPMV_offset_user	5	
#define RPMV_offset_group	6	
#define RPMV_offset_mtime	7	


#define RPMV_size	'S'
#define RPMV_mode	'M'
#define RPMV_digest	'5'
#define RPMV_device	'D'
#define RPMV_linkname	'L'
#define RPMV_user	'U'
#define RPMV_group	'G'
#define RPMV_mtime	'T'
#define RPMV_cap	'P'
*/

static
int
determine_rpmvoffset_from_inloffset(int inloffset)
{

	switch (inloffset) {

		case  INL_NAME_OFFSET:
			return RPMV_FORMAT_LEN;
			break;
		case  INL_LINKNAME_OFFSET:
			return RPMV_offset_linkname;
			break;
		case  INL_SIZE_OFFSET:
			return RPMV_offset_size;
			break;
		case  INL_PERMISSIONS_OFFSET:
			return RPMV_offset_mode;
			break;
		case  INL_OWNERS_OFFSET:
			return RPMV_offset_user;
			break;
		case  INL_MDATE_OFFSET:
			return RPMV_offset_mtime;
			break;
		case  INL_MD5_OFFSET:
			return RPMV_offset_digest;
			break;
		case  INL_SHA1_OFFSET:
			return RPMV_offset_digest;
			break;
		default:
			return -1;

	}
	return -1;
}



static
void
print_lines(char * name, char * pref, STRAR * ar)
{
	int fd;
	char * s;
	int i;
	E_DEBUG("START");
	i = 0;

	if (strcmp(name, "-") == 0) {
		fd = STDOUT_FILENO;
	} else {
		fd = open(name, O_RDWR|O_CREAT|O_TRUNC, 0644);	 
	}

	if (fd < 0) {
		fprintf(stderr, "%s: info dump failed: %s\n", swlib_utilname_get(), strerror(errno));
		return;
	}
	s = strar_get(ar, i++);
	while (s) {
		write(fd, s, strlen(s));
		write(fd, "\n", 1);
		s = strar_get(ar, i++);
	}
	close(fd);
	return;
}

static
char *
discern_name(char * line, int * len)
{
	char * s;
	char * delim =  "[linkname=";

	E_DEBUG("START");
	*len = 0;
	s = strstr(line, delim);
	if (s == NULL) {
		/* error */
		return NULL;
	}
	*len = (int)(s - line);		
	return line;
}

static
int
remove_duplicate_files(STRAR * meta)
{
	int im;
	char * m_s;
	char * name;
	char * prevname;
	int len;
	int prevlen;

	E_DEBUG("START");
	prevname = NULL;
	prevlen = 0;
	im = 0;
	m_s = strar_get(meta, im++);
	while (m_s) {
		/* fprintf(stderr, "%s\n", m_s); */
		name = discern_name(m_s, &len);
		if (!name)
			return -1;

		if (name && prevname) {
			if (strncmp(name, prevname, len) == 0) {
				/* remove the line with the lower
				   memory address since this should
				   indicate the first file is the INFO
				   file. */
				if (name < prevname) {
					/* fprintf(stderr, "REMOVING [%s] index %d\n", name, im-1); */
					strar_remove_index(meta, im-2);
				} else {
					/* fprintf(stderr, "REMOVING [%s] index %d\n", name, im-2); */
					strar_remove_index(meta, im-1);
				}
				im--;
			}
		}
		m_s = strar_get(meta, im++);
		prevname = name;
		prevlen = len;
	}
	return 0;
}

static
STRAR *
linize(char * text)
{
	char * s;
	STROB * tmp;
	STRAR * strar;
	tmp = strob_open(100);
	strar = strar_open();
	s = strob_strtok(tmp, text, "\n");
	while (s) {
		strar_add(strar, s);	
		s = strob_strtok(tmp, NULL, "\n");
	}
	strob_close(tmp);
	return strar;
}

static
void
looper_abort(GB * G, SWICOL * swicol, int ofd)
{
	swpl_send_abort(swicol, ofd, G->g_swi_event_fd,  SWBIS_TS_Abort);
}


static  /* same as in swproglib.c */
int
show_file_list(GB * G, SWI_FILELIST * fl)
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
	
		CSHID
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
		
						/*
						 * Here is the looper routine for getting the md5/sha1 of the
						 * prelink'ed files. It is fed  a filename one line at at time
						 * and either spits out the sha1 and md5 digest or a error message string
						 */

		"				shls_v_looper shls_v_looper_func_prelink_sha1 \"$sw_retval\"\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"
		CSHID
		"				shls_bashin2 \"" SWBIS_TS_Catalog_dir_remove "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"

		"				#shls_bashin2 \"" SWBIS_TS_Load_management_host_status "\"\n"
		"				#sw_retval=$?\n"
		"				#case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"				#sw_retval=$?\n"
		"				#swexec_status=$sw_retval\n"

		"				shls_bashin2 \"" SWBIS_TS_post_verify "\"\n"
		"				sw_retval=$?\n"
		"				case $sw_retval in 0) $sh_dash_s;; *) sleep 2; shls_false_;; esac\n"
		"				sw_retval=$?\n"
		"				swexec_status=$sw_retval\n"


		CSHID
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
inl_get_name_len(char * line)
{
	int ret;
	int status;
	char * s;
	/* E_DEBUG2("LINE: [%s]", line); */
	s = strstr(line,  LS_LIST_NAME_LENGTH);
	if (!s) {
		SWLIB_FATAL("");
	}
	s = s + strlen(LS_LIST_NAME_LENGTH);
	if (*s != '=') {
		SWLIB_FATAL("");
	}	
	s++;
	ret = swlib_atoi(s, &status);
	if (status) {
		SWLIB_FATAL("");
	}
	/* E_DEBUG2("return: [%d]", ret); */
	return ret;
}

static
char *
inl_get_name(char * line, int * plen)
{
	char * s;
	s = strchr(line, ' ');
	/* E_DEBUG2("LINE: [%s]", line); */
	if (!s) SWLIB_FATAL("");
	s++;
	if (plen) *plen = inl_get_name_len(line);
	/* E_DEBUG2("return: [%s]", s); */
	return s;
}


static
char *
inl_get_linkname(char * line, int * plen)
{
	int len;
	char * s;
	int status;
	int linknamelen;


	s = inl_get_name(line, &len);
	s += len;
	s = strstr(s,  LS_LIST_LINKNAME_LENGTH);
	/* E_DEBUG2("LINE: [%s]", line); */
	if (!s)  SWLIB_FATAL("");
	s = s + strlen(LS_LIST_LINKNAME_LENGTH);
	if (*s != '=')  SWLIB_FATAL("");
	s++;
	linknamelen = swlib_atoi(s, &status);
	if (status) SWLIB_FATAL("");

	s = strstr(s,  LS_LIST_LINKNAME_MARK);
	if (!s)  SWLIB_FATAL("");
	s = s + strlen(LS_LIST_LINKNAME_MARK);
	if (plen) *plen = linknamelen;
	/* E_DEBUG2("return: [%s]", s); */
	return s;	
}

static
char *
inl_get_filesize(char * line, int * plen) {
	int len;
	char * ret;
	char * e;
	char * s;

	/* E_DEBUG2("LINE: [%s]", line); */
	s = inl_get_linkname(line, &len);
	if (!s) SWLIB_FATAL("");
	s = s + len;
	s++; s++;
	if (*s != ' ') SWLIB_FATAL("");
	s++;
	if (*s != '[') SWLIB_FATAL("");
	s++;
	ret = s;
	e = s;
	while (*e && *e != ']') e++;
	if (*e != ']') SWLIB_FATAL("");
	if (plen) *plen = (int)(e - s);
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
char *
inl_get_permissions(char * line, int * plen)
{
	int len;
	char * ret;
	char * e;
	char * s;

	/* E_DEBUG2("LINE: [%s]", line); */
	s = inl_get_filesize(line, &len);
	if (!s) SWLIB_FATAL("");
	s = s + len;
	if (*s != ']') SWLIB_FATAL("");
	if (*(++s) != ' ') SWLIB_FATAL("");
	if (*(++s) != '[') SWLIB_FATAL("");
	s++;

	/* Now at mode string */
	ret = s;
	e = s;
	while (*e && *e != ']') e++;
	if (*e != ']') SWLIB_FATAL("");
	if (plen) *plen = (int)(e - s);
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
int
inl_is_regfile(char * line)
{
	char * s;
	int len;
	/* E_DEBUG2("LINE: [%s]", line); */
	s = inl_get_permissions(line, &len);
	if (*s == '-')
		return 1;
	else
		return 0;
}

static
char *
inl_get_ownernames(char * line, int * plen)
{
	int len;
	int status;
	char * ret;
	char * s;

	/* E_DEBUG2("LINE: [%s]", line); */
	s = inl_get_permissions(line, &len);
	if (!s) SWLIB_FATAL("");
	s = s + len;
	if (*s != ']') SWLIB_FATAL("");
	if (*(++s) != ' ') SWLIB_FATAL("");
	if (*(++s) != '[') SWLIB_FATAL("");
	s++;

	/* Now at "len=NNN" */
	if (strstr(s, "len=") != s) SWLIB_FATAL("");
	s += strlen("len=");

	len = swlib_atoi(s, &status);
	while (*s && *s != ' ') s++;
	s++;
	if (*s != '[') SWLIB_FATAL("");
	s++;
	if (plen) *plen = len;
	ret = s;
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
char *
inl_get_mdate(char * line, int * plen) {
	int len;
	char * ret;
	char * e;
	char * s;

	/* E_DEBUG2("LINE: [%s]", line); */
	s = inl_get_ownernames(line, &len);
	if (!s) SWLIB_FATAL("");
	s = s + len;
	s++; s++;
	if (*s != ' ') SWLIB_FATAL("");
	s++;
	if (*s != '[') SWLIB_FATAL("");
	s++;
	ret = s;
	e = s;
	while (*e && *e != ']') e++;
	if (*e != ']') SWLIB_FATAL("");
	if (plen) *plen = (int)(e - s);
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
char *
inl_get_md5(char * line, int * plen) {
	int len;
	char * ret;
	char * e;
	char * s;

	s = inl_get_mdate(line, &len);
	if (!s) SWLIB_FATAL("");
	s = s + len;
	s++;
	if (*s != ' ') SWLIB_FATAL("");
	s++;
	if (*s != '[') SWLIB_FATAL("");
	s++;
	if (strstr(s, "MD5=") == s) 
		s += strlen ("MD5=");
	ret = s;
	e = s;
	while (*e && *e != ']') e++;
	if (*e != ']') SWLIB_FATAL("");
	if (plen) *plen = (int)(e - s);
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
char *
inl_get_sha1(char * line, int * plen) {
	int len;
	char * ret;
	char * e;
	char * s;

	s = inl_get_md5(line, &len);
	if (!s) SWLIB_FATAL("");
	while (*s && *s != ']') s++;
	s++;
	if (*s != ' ') SWLIB_FATAL("");
	s++;
	if (*s != '[') SWLIB_FATAL("");
	s++;

	if (strstr(s, "SHA1=") == s) 
		s += strlen ("SHA1=");
	ret = s;
	e = s;
	while (*e && *e != ']') e++;
	if (*e != ']') SWLIB_FATAL("");
	if (plen) *plen = (int)(e - s);
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
char *
inl_get_sha512(char * line, int * plen) {
	int len;
	char * ret;
	char * e;
	char * s;

	s = inl_get_sha1(line, &len);
	if (!s) SWLIB_FATAL("");
	while (*s && *s != ']') s++;
	s++;
	if (*s != ' ') SWLIB_FATAL("");
	s++;
	if (*s != '[') SWLIB_FATAL("");
	s++;

	if (strstr(s, "SHA512=") == s) 
		s += strlen("SHA512=");
	ret = s;
	e = s;
	while (*e && *e != ']') e++;
	if (*e != ']') SWLIB_FATAL("");
	if (plen) *plen = (int)(e - s);
	/* E_DEBUG2("return: [%s]", ret); */
	return ret;
}

static
int
compare_names(char * mname, char * sname)
{
	char * mname_start;
	char * sname_start;
	char * s;
	int ml;
	int sl;
	E_DEBUG("");
	ml = inl_get_name_len(mname);
	E_DEBUG("");
	sl = inl_get_name_len(sname);
	E_DEBUG("");
	if (ml != sl) return 1;
	E_DEBUG("");

	s = strchr(mname, ' ');  /* the space after 'name_length=NNN' */
	if (!s) SWLIB_FATAL("");	
	mname_start = s + 1;
	
	s = strchr(sname, ' ');  /* the space after 'name_length=NNN' */
	if (!s) SWLIB_FATAL("");	
	sname_start = s + 1;

	return strncmp(mname_start, sname_start, ml);
}

static
char * 
return_normal_name (STROB * buf, INFOLINE * info_inl)
{
	char * name = info_inl->nameM.addrM;
	int len = info_inl->nameM.lenM;

	strob_strcpy(buf, "");
	if (strstr(name, "/") == name && strlen(name) > 1) { len--; name++; }
	if (strstr(name, "./") == name && strlen(name) > 2) { len-=2; name+=2; }
	strob_strncpy(buf, name, len);
	return strob_str(buf);
}


static
void
init_rpmv_field(STROB * field)
{
	strob_memset(field, '\0', 24 /* really only 11 bytes will be used */ );
	strob_memset(field, '.', RPMV_RESULT_STRING_LEN);
	strob_chr_index(field, RPMV_RESULT_STRING_LEN     , ' '/*space*/);
	strob_chr_index(field, RPMV_RESULT_STRING_LEN + 1 , ' '/*space*/);
	strob_chr_index(field, RPMV_RESULT_STRING_LEN + 2 , ' '/*space*/);
}

static
int
set_rpmv_field_condition(INFOLINE * status_inl, int rpmv_offset, int ch)
{
	int failch;
	STROB * field = status_inl->rpmvM;
	char  status_chars[] = RPMV_STATUS_STRING;

	if (rpmv_offset < 0) {
		/* FIXME: Maybe give warning message */
		return -1;
	}

	if (ch == RPMV_fail_NOT) {
		strob_chr_index(field, rpmv_offset, RPMV_fail_NOT);
	} else {
		strob_chr_index(field, rpmv_offset, status_chars[rpmv_offset]);
	}
	return 0;
}

static
int
determine_inl_field_condition(INFOLINE * status_inl, INFOLINE * info_inl, INFOLINE * syst_inl, int offset)
{
	int rpmv_offset;
	int ret;
	INFOITEM * ti = (INFOITEM*)(((char*)info_inl)+offset);
	INFOITEM * ts = (INFOITEM*)(((char*)syst_inl)+offset);

	ret = INL_STATUS_UNSET;

	rpmv_offset = determine_rpmvoffset_from_inloffset(offset);
	if ( 
		/* if lengths are equal and field is identical */
		ti->lenM > 0 &&
		ti->lenM == ts->lenM &&
		strncmp(ti->addrM, ts->addrM, ti->lenM) == 0
	) {
		E_DEBUG("present and equal");
		ret = INL_STATUS_1_1_EQ;
		set_rpmv_field_condition(status_inl, rpmv_offset, RPMV_fail_NOT);
		if (offset == INL_OWNERS_OFFSET ) {
			set_rpmv_field_condition(status_inl, RPMV_offset_user, RPMV_fail_NOT);
			set_rpmv_field_condition(status_inl, RPMV_offset_group, RPMV_fail_NOT);
		}
	} else if ( 
		/* present and unequal */
		ti->lenM > 0 &&
		ts->lenM > 0 &&
		strncmp(ti->addrM, ts->addrM, ti->lenM)
	) {
		E_DEBUG("present and unequal");
		if (offset == INL_OWNERS_OFFSET ) {
			/* FIXME consider owner and group separately */
			set_rpmv_field_condition(status_inl, RPMV_offset_user, RPMV_fail_fail);
			set_rpmv_field_condition(status_inl, RPMV_offset_group, RPMV_fail_fail);
		} else {
			set_rpmv_field_condition(status_inl, rpmv_offset, RPMV_fail_fail);
		}
		ret = INL_STATUS_1_1_NEQ;
	} else if ( 
		ti->lenM == 0 &&
		ts->lenM == 0
	) {
		E_DEBUG("lenM is 0 for both");
		set_rpmv_field_condition(status_inl, rpmv_offset, RPMV_fail_NOT/*FIXME is this right? */);
		ret = INL_STATUS_0_0_NA;
	} else if ( 
		/* present in INFO only */
		ti->lenM > 0
	) {
		E_DEBUG(" in INFO only");
		set_rpmv_field_condition(status_inl, rpmv_offset, RPMV_fail_fail);
		ret = INL_STATUS_1_0_NA;	
	} else if ( 
		/* present in SYSTEM only */
		ts->lenM > 0
	) {
		E_DEBUG(" in SYSTEM only");
		set_rpmv_field_condition(status_inl, rpmv_offset, RPMV_fail_unknown);
		ret = INL_STATUS_0_1_NA;
	} else {
		/* error, should never get here */
		SWLIB_FATAL("");
	}

	/* return -2, if present in INFO and empty in system */
	/* return -1, if present in INFO and system and unequal */
	/* return 0, if identical */
	/* return 1, if present in system, but missing from INFO */
	/* return 2, missing from system and INFO */

	return ret;
}


static
void
set_inl_field_status_conditionally(INFOLINE * status_inl, int offset, int status)
{
	INFOITEM * t = (INFOITEM*)(((char*)status_inl)+offset);
	int current_status;
	current_status = t->statusM;
	if (current_status == INL_STATUS_1_1_EQ && status != INL_STATUS_1_1_EQ)
		t->statusM = status;
	return;
}

static
int
get_inl_field_status(INFOLINE * status_inl, int offset)
{
	int current_status;
	INFOITEM * t = (INFOITEM*)(((char*)status_inl)+offset);
	current_status = t->statusM;
	E_DEBUG2("READING STATUS at [%p]", (void*)&(t->statusM));
	E_DEBUG3("OFFSET(ATTR)=[%d] current_status=[%d]", offset, current_status);
	return current_status;
}

static
int
set_inl_condition(INFOLINE * status_inl, int offset, int incoming_status)
{
	int current_status;
	INFOITEM * t = (INFOITEM*)(((char*)status_inl)+offset);
	E_DEBUG("");
	current_status = t->statusM;
	if ( incoming_status < current_status) {
		/* By design, the worse the status is the lower the number,
		hence it is safe to overwrite a previous status with a higher number */
		t->statusM = incoming_status;
	}
	return t->statusM;
}


/* From the rpm manpage 
       c %config configuration file.
       d %doc documentation file.
       g %ghost file (i.e. the file contents are not included in the package payload).
       l %license license file.
       r %readme readme file.

       from the package header, followed by the file name.  Each of the 8 characters denotes the result of a
       comparison  of  attribute(s) of the file to the value of those attribute(s) recorded in the database.
       A single "." (period) means the test passed, while a single "?" (question mark)  indicates  the  test
       could  not be performed (e.g. file permissions prevent reading). Otherwise, the (mnemonically emBold-
       ened) character denotes failure of the corresponding --verify test:

       S file Size differs
       M Mode differs (includes permissions and file type)
       5 digest (formerly MD5 sum) differs
       D Device major/minor number mismatch
       L readLink(2) path mismatch
       U User ownership differs
       G Group ownership differs
       T mTime differs
       P caPabilities differ
*/


int
inl_does_status_have_rpmv_error(INFOLINE * status_inl)
{
	return strncmp(strob_str(status_inl->rpmvM), "........ ", 8); 
}
				
static
void
modify_system_inl(GB * G, INFOLINE * system_inl, STROB * prelink_md5, STROB * prelink_sha1)
{

	char * md5;
	int md5_len;
	char * sha1;
	int sha1_len;

	md5 = system_inl->md5M.addrM;
	md5_len = system_inl->md5M.lenM;

	sha1 = system_inl->sha1M.addrM;
	sha1_len = system_inl->sha1M.lenM;

	*(strob_str(prelink_md5) + MD5_ASCII_LEN) = '\0';
	*(strob_str(prelink_sha1) + SHA1_ASCII_LEN) = '\0';

	if (md5_len != MD5_ASCII_LEN || sha1_len != SHA1_ASCII_LEN) {
		sw_e_msg(G, "internal error in modify_system_inl\n");
		return;
	} 
	E_DEBUG2("Before: [%s]", md5);
	E_DEBUG2("Before: [%s]", sha1);
	strncpy(md5, strob_str(prelink_md5), MD5_ASCII_LEN);
	strncpy(sha1, strob_str(prelink_sha1), SHA1_ASCII_LEN);
	E_DEBUG2("After: [%s]", md5);
	E_DEBUG2("After: [%s]", sha1);

	system_inl->sha512M.addrM = NULL;
	system_inl->sha512M.lenM = 0;
}

static
int
obtain_prelinked_binary_digests(STROB * buf, GB * G, SWI * swi, int ofd, int ifd, INFOLINE * info_inl, STROB * prelink_md5, STROB * prelink_sha1)
{
	/* Send a "filename\n" string, read one (1) line for the md5
	   Send "filename\n" string again, read one (1) line for the sha1
	*/ 

	int retval;
	int ret;
	char * name = info_inl->nameM.addrM;
	int len = info_inl->nameM.lenM;
	

	E_DEBUG("");
	retval = 0;


	/* 
 	 * remove leading slash or dot slash, send relative path only to the prelink program
 	 *  since we chdir() to the target path.
	 */

				/* FIXME: remove all leading slashes */
	if (strstr(name, "/") == name && strlen(name) > 1) { len--; name++; }
	if (strstr(name, "./") == name && strlen(name) > 2) { len-=2; name+=2; }

	if (len <= 0) {
		/* 
 		 * Normal for "/" or "./"
 		 * Hence, do nothing
 		 */
		return 1;
	}

	/* construct the line to send
 	 * NOTE: name is not NUL terminated at the end of the field 
 	 */
	strob_strncpy(buf, name, len);
	
	E_DEBUG2("Obtaining digests for [%s]", strob_str(buf));
	ret = swgp_write_as_echo_line(ofd, strob_str(buf));
	if (ret != 0) {
		/* error */
		E_DEBUG("error");
		return -1;
	}

	/* read the md5, ascii form 32 bytes long plus the filename */
	strob_strcpy(prelink_md5, "");
	ret = swgp_read_line(ifd, prelink_md5, 0 /* DO not append */);
	/*
 	 * This ret should be 32 plus a space, plus the length of the filename
	 */
	if (ret < MD5_ASCII_LEN + len || ret > MD5_ASCII_LEN + 1 + len + 5 /*sanity margin of error*/ ) {
		/* error */
		E_DEBUG("error");
		retval-=2;

	}
	E_DEBUG2("MD5 result [%s]", strob_str(prelink_md5));

	/* read the sha1, ascii form 40 bytes long plus the filename */
	strob_strcpy(prelink_sha1, "");
	ret = swgp_read_line(ifd, prelink_sha1, 0 /* Do not append */);
	if (ret < SHA1_ASCII_LEN + len || ret > SHA1_ASCII_LEN + 1 + len + 5 /*sanity margin of error*/ ) {
		/* error */
		E_DEBUG("error");
		retval--;
	}
	E_DEBUG2("SHA1 result [%s]", strob_str(prelink_sha1));
	E_DEBUG2("returning [%d]", retval);
	return retval;
}

static
int
check_prelink_list (STROB * buf, GB * G, INFOLINE * info_inl)
{
	char * list = G->g_prelink_fd_memM;
	char * name = info_inl->nameM.addrM;
	int len = info_inl->nameM.lenM;
	char * find;

	E_DEBUG("");
	if (list == NULL) /* sanity check */
		return 0;

	if (strstr(name, "/") == name && strlen(name) > 1) { len--; name++; }
	if (strstr(name, "./") == name && strlen(name) > 2) { len-=2; name+=2; }
	strob_strncpy(buf, name, len);
	E_DEBUG2("searching for [%s]", strob_str(buf));


	/* search for name in list */
	if ((find=strstr(list, strob_str(buf))) != NULL) {
		/* Now make sure the find is anchor to the beginning of the line
 		 * to eliminate false hits
 		 */
		if (
			(int)(find - list) < 3  || /* first line, really <2 is correct, 3 is a sanity margin */ 
			(*(find-1) == '/' && isspace(*(find-2))) /* preceeding slash and newline */
		) {
			E_DEBUG("found in list");
			return 1;
		} else {
			return 0; /* false hit */
		}
	} else {
		E_DEBUG2("NOT found in list [%s]", strob_str(buf));
		return 0;
	}
}

static
void
init_infoitem(INFOLINE * inl, int offset)
{
	INFOITEM * it;
	it = (INFOITEM*)((char*)inl + offset);
	it->addrM = NULL;
	it->lenM = 0;
	it->statusM = INL_STATUS_UNSET;
}

static
int
inl_compare_infoline(STROB * tmpbuf, GB * G, SWI * swi, int ofd, int ifd,
		INFOLINE * status_inl, INFOLINE * info_inl, INFOLINE * system_inl, int do_prelink)
{
	int field_status;
	int md5_field_status;
	int sha1_field_status;
	int sha512_field_status;
	int size_field_status;
	int retval = 0;
	int ret;
	STROB * prelink_md5;
	STROB * prelink_sha1;

	E_DEBUG("");
	prelink_md5 = NULL;
	prelink_sha1 = NULL;
	init_rpmv_field(status_inl->rpmvM);

	/* find out which field mis-matched */

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_LINKNAME_OFFSET);
	ret = set_inl_condition(status_inl, INL_LINKNAME_OFFSET, field_status);
	if (ret == INL_STATUS_1_1_NEQ) retval++;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_SIZE_OFFSET);
	size_field_status = field_status;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_OWNERS_OFFSET);
	ret = set_inl_condition(status_inl, INL_OWNERS_OFFSET, field_status);
	if (ret == INL_STATUS_1_1_NEQ) retval++;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_PERMISSIONS_OFFSET);
	ret = set_inl_condition(status_inl, INL_PERMISSIONS_OFFSET, field_status);
	if (ret == INL_STATUS_1_1_NEQ) retval++;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_MDATE_OFFSET);
	ret = set_inl_condition(status_inl, INL_MDATE_OFFSET, field_status);
	if (ret == INL_STATUS_1_1_NEQ) retval++;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_MD5_OFFSET);
	md5_field_status = field_status;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_SHA1_OFFSET);
	sha1_field_status = field_status;

	field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_SHA512_OFFSET);
	sha512_field_status = field_status;


	if (    do_prelink &&
		G->g_no_prelinkM == 0 &&
		G->g_prelink_fdM > 0 &&
		(
		md5_field_status == INL_STATUS_1_1_NEQ ||
		sha1_field_status == INL_STATUS_1_1_NEQ ||
		sha512_field_status == INL_STATUS_1_1_NEQ
		)
	) {
	
		E_DEBUG2("Got prelink candidate for [%s]", return_normal_name(tmpbuf, info_inl));
		/*
 		 * Look for the name in the list of prelink'ed files
 		 */ 

		ret = check_prelink_list(tmpbuf, G, info_inl);	
		E_DEBUG2("check_prelink_list returned [%d]", ret);

		if (ret > 0) {
			/* reconcile the digest mismatch */
			prelink_md5 = strob_open(60);
			prelink_sha1 = strob_open(60);
			ret = obtain_prelinked_binary_digests(tmpbuf, G, swi, ofd, ifd, info_inl, prelink_md5, prelink_sha1);
			if (ret == 0) {
				/* got the digests OK */
				E_DEBUG("");
				/* 
 				 * Modify system_inl  then re-check
				 */
				modify_system_inl(G, system_inl, prelink_md5, prelink_sha1);	

				/*
 				 * FIXME  Zero out the sha512 fields, it is not used at this time
				 */

				init_infoitem(info_inl,  INL_SHA512_OFFSET);
				init_infoitem(system_inl,  INL_SHA512_OFFSET);

				/*
 				 * Now re-test the md5 and sha1 fields
 				 */

				E_DEBUG2("md5 field status before mod: [%d]", get_inl_field_status(status_inl, INL_MD5_OFFSET));
				md5_field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_MD5_OFFSET);
				set_inl_condition(status_inl, INL_MD5_OFFSET, md5_field_status);
				E_DEBUG2("md5 field status after mod: [%d]", get_inl_field_status(status_inl, INL_MD5_OFFSET));

				E_DEBUG2("sha1 field status before mod: [%d]", get_inl_field_status(status_inl, INL_SHA1_OFFSET));
				sha1_field_status = determine_inl_field_condition(status_inl, info_inl, system_inl, INL_SHA1_OFFSET);
				set_inl_condition(status_inl, INL_SHA1_OFFSET, sha1_field_status);
				E_DEBUG2("sha1 field status after mod: [%d]", get_inl_field_status(status_inl, INL_SHA1_OFFSET));

				if (	md5_field_status == INL_STATUS_1_1_EQ &&
					sha1_field_status == INL_STATUS_1_1_EQ) 
				{
					/*
 					 * Set the size field to equal
 					 */
					set_inl_condition(status_inl, INL_SIZE_OFFSET, INL_STATUS_1_1_EQ);
					set_rpmv_field_condition(status_inl, RPMV_offset_size, RPMV_fail_NOT);
					init_infoitem(info_inl,  INL_SIZE_OFFSET);
					init_infoitem(system_inl,  INL_SIZE_OFFSET);
				} else {
					set_inl_condition(status_inl, INL_SIZE_OFFSET, size_field_status);
				}

				/*
				 * Now re-test all the fields
				 */

				ret = inl_compare_infoline(NULL /*tmpbuf*/, G, swi, ofd, ifd,
						status_inl, info_inl, system_inl, 0 /*no do_prelink*/);

				return ret;
			} else {
				ret = set_inl_condition(status_inl, INL_MD5_OFFSET, md5_field_status);
				if (ret == INL_STATUS_1_1_NEQ) retval++;
				ret = set_inl_condition(status_inl, INL_SHA1_OFFSET, sha1_field_status);
				if (ret == INL_STATUS_1_1_NEQ) retval++;
				ret = set_inl_condition(status_inl, INL_SHA512_OFFSET, sha512_field_status);
				if (ret == INL_STATUS_1_1_NEQ) retval++;
				ret = set_inl_condition(status_inl, INL_SIZE_OFFSET, size_field_status);
				if (ret == INL_STATUS_1_1_NEQ) retval++;
				if (ret < 0) retval = -100;
			} 
		}
	} else {
		E_DEBUG("file not prelinked");
		ret = set_inl_condition(status_inl, INL_MD5_OFFSET, md5_field_status);
		if (ret == INL_STATUS_1_1_NEQ) retval++;
		ret = set_inl_condition(status_inl, INL_SHA1_OFFSET, sha1_field_status);
		if (ret == INL_STATUS_1_1_NEQ) retval++;
		ret = set_inl_condition(status_inl, INL_SHA512_OFFSET, sha512_field_status);
		if (ret == INL_STATUS_1_1_NEQ) retval++;
		ret = set_inl_condition(status_inl, INL_SIZE_OFFSET, size_field_status);
		if (ret == INL_STATUS_1_1_NEQ) retval++;
	}

	if (prelink_md5) strob_close(prelink_md5);
	if (prelink_sha1) strob_close(prelink_sha1);

	E_DEBUG2("leaving with retval=[%d]", retval);

	/* fprintf(stderr, "inl_compare_infoline returning [%d]\n", retval); */
	return retval;
}


static
char *
inl_status_msg(int status)
{
	if (status == INL_STATUS_1_1_EQ) return "PASSED";
	if (status == INL_STATUS_1_1_NEQ) return "FAILED ***";
	if (status == INL_STATUS_0_1_NA) return  "MISSING FROM INFO File";
	if (status == INL_STATUS_0_0_NA) return  "Not Checked";
	if (status == INL_STATUS_1_0_NA) return  "MISSING FROM SYSTEM";
	if (status == INL_STATUS_MISSING_FILE) return  "ONE OR MORE FILES MISSING";
	if (status == INL_STATUS_UNSET) return  "Not determined";
	return "Internal Error";
}

static
void	
write_rpmv_output_line(GB * G, INFOLINE * status_inl,  INFOLINE * m_s_inl, char * target_path)
{
	char * name;
	int len;

	if (
		(
		G->g_output_formM == SWVERIFY_OUTF_RPM || 
		G->g_output_formM == SWVERIFY_OUTF_ALL
		) &&
		(
		inl_does_status_have_rpmv_error(status_inl) ||
		status_inl->nameM.statusM == INL_STATUS_MISSING_FILE ||
		G->g_verboseG > SWC_VERBOSE_1
		)
	) {
		name = m_s_inl->nameM.addrM;
		len = m_s_inl->nameM.lenM;
		if (strcmp(target_path, "/") == 0) {
			/* squash the leading ./ to a / in path */
			if (strncmp(m_s_inl->nameM.addrM, "./", 2) == 0) {
				name += 1;
				len -= 1;
			}
		} 

		if (status_inl->nameM.statusM == INL_STATUS_MISSING_FILE) {
			strob_strcpy(status_inl->rpmvM, "missing   ");
			strob_strncat(status_inl->rpmvM, name, len);
			fprintf(stdout, "%s\n", strob_str(status_inl->rpmvM));
		} else {
			strob_strncat(status_inl->rpmvM, name, len);
			fprintf(stdout, "%s\n", strob_str(status_inl->rpmvM));
		}
	}
}

static
int	
write_summary_report(GB * G, INFOLINE * status_inl, STROB * rpt)
{
	int ret;
	int retval;
	strob_strcpy(rpt, "");

	retval = 0;	

	ret = get_inl_field_status(status_inl, INL_NAME_OFFSET);
	E_DEBUG2("FILE LIST ret=[%d]", ret);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"Fileset file list",
	inl_status_msg(ret));

	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_NAME;
	}
	
	ret = get_inl_field_status(status_inl, INL_LINKNAME_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
		"Fileset linknames",
		inl_status_msg(ret));
	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_LINKNAME;
	}
	
	ret = get_inl_field_status(status_inl, INL_SIZE_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"File sizes",
	inl_status_msg(ret));
	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_SIZE;
	}

	ret = get_inl_field_status(status_inl, INL_OWNERS_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"POSIX.1 file ownerships",
	inl_status_msg(ret));


	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_OWNERS;
	}

	ret = get_inl_field_status(status_inl, INL_MDATE_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"File Modification times",
	inl_status_msg(ret));
	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_MDATE;
	}

	ret = get_inl_field_status(status_inl, INL_PERMISSIONS_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"POSIX.1 File permissions",
	inl_status_msg(ret));
	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_PERMISSIONS;
	}
	
	ret = get_inl_field_status(status_inl, INL_MD5_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"File Contents MD5 digests",
	inl_status_msg(ret));

	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_MD5;
	}
	
	ret = get_inl_field_status(status_inl, INL_SHA1_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"File Contents SHA1 digests",
	inl_status_msg(ret));
	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG2("BF %d", ret);
		retval |= INL_BF_SHA1;
	}	

	ret = get_inl_field_status(status_inl, INL_SHA512_OFFSET);
	strob_sprintf(rpt, 1, "%s: INFO check: %30s: %s\n", swlib_utilname_get(),
	"File Contents SHA512 digests",
	inl_status_msg(ret));

	if (ret <= INL_STATUS_1_0_NA && ret != INL_STATUS_1_1_EQ) {
		E_DEBUG("BF");
		retval |= INL_BF_SHA512;
	}
	return retval;
}

static
int
compare_diff(GB * G, SWI * swi, int ofd, int ifd, STROB * rpt_diff, STROB * rpt_summary, STRAR * meta, STRAR * sys, int do_show, char * target_path)
{
	int retval;
	char * m_s;
	char * s_s;
	char * name;
	int name_len;
	int compare_ret;
	int did_print_header;
	int summary_ret;
	INFOLINE * m_s_inl;
	INFOLINE * s_s_inl;
	INFOLINE * status_inl;
	STROB * tmpbuf; /* used by reconcile_prelink */
	int is;
	int im;

	E_DEBUG("START");
	retval = 0;
	did_print_header = 0;
	tmpbuf = strob_open(100);

	strob_strcpy(rpt_diff, "");
	strob_strcpy(rpt_summary, "");

	/* Make a psuedo diff-like output, this is easy because
	   we can assume that the files are sorted, and, only files will
	   be missing from the system list (STRAR*)sys or else they will
	   be changed 1-to-1 with (STRAR*)meta */

	status_inl = swverify_inl_create(); /* Special global status object */
	m_s_inl = swverify_inl_create();
	s_s_inl = swverify_inl_create();

	im = 0;
	is = 0;
	m_s = strar_get(meta, im);
	s_s = strar_get(sys, is);
	status_inl->nameM.statusM = INL_STATUS_1_1_EQ;
	while (m_s) {
		/* status_inl->nameM.statusM = INL_STATUS_1_1_EQ; */
		set_inl_field_status_conditionally(status_inl, INL_NAME_OFFSET, INL_STATUS_1_1_EQ);
		E_DEBUG2("NAME m_s=%s", m_s);
		if (G->g_ignore_slack_installM) {
			name = inl_get_name(m_s, &name_len);		
			SWLIB_ASSERT(name != NULL);
			SWLIB_ASSERT(name_len > 0);
			if (
			 	(strstr(name, "./install") == name && name_len == 9) ||
			 	strstr(name, "./install/") == name
			) {
				im++;
				E_DEBUG("goto slack_end");
				goto slack_end;
			}
		}
		/* parse the data to prove its what we expect */
		E_DEBUG("");
		if (m_s) swverify_inl_parse(m_s_inl, m_s); 
		if (s_s) swverify_inl_parse(s_s_inl, s_s); 

		if (m_s && s_s && strcmp(m_s, s_s) == 0) {
			/* good, lines match */
			im++;
			is++;
			E_DEBUG("000000000000000000000000000 match");
			if (do_show) {
				if (
					G->g_output_formM == SWVERIFY_OUTF_DIFF ||
					G->g_output_formM == SWVERIFY_OUTF_ALL
				) {
					strob_sprintf(rpt_diff, 1/*do append*/,
						"= %s\n",
		 				s_s);
				}
			}
			compare_ret = inl_compare_infoline(tmpbuf, G, swi, ofd, ifd,
						status_inl, m_s_inl, s_s_inl, 0 /*no prelink*/);
			if (compare_ret != 0) {
				/* Sanity check, this should never happen */
				retval++;
			}
			write_rpmv_output_line(G, status_inl, m_s_inl, target_path);
		} else if (m_s && s_s && strcmp(m_s, s_s) != 0) {
			/* either changed or deleted file */
			retval++;
			E_DEBUG("000000000000000000000 mis match");
			if (compare_names(m_s, s_s) == 0) {
				/* changed file */
				compare_ret = inl_compare_infoline(tmpbuf, G, swi, ofd, ifd,
							status_inl, m_s_inl, s_s_inl, G->g_no_prelinkM?0:1);
				if (compare_ret == 0 && G->g_no_prelinkM == 0) {
					/*
 					 * Here, the prelink reconciliation probably averted the error
 					 * hence decrement retval to indicate all is OK
 					 */
					retval--;	
				} else {
					if (
						G->g_output_formM == SWVERIFY_OUTF_DIFF ||
						G->g_output_formM == SWVERIFY_OUTF_ALL
					) {
						strob_sprintf(rpt_diff, 1,
							"%dc%d\n"
							"< %s\n"
							"---\n"
							"> %s\n",
			 				im, im, m_s, s_s);
					}
				}
				im++;
				is++;
				write_rpmv_output_line(G, status_inl, m_s_inl, target_path);
			} else {
				/* missing file */
				E_DEBUG("000000000000000 missing file");
				/* status_inl->nameM.statusM = INL_STATUS_MISSING_FILE; */
				set_inl_field_status_conditionally(status_inl, INL_NAME_OFFSET, INL_STATUS_MISSING_FILE);
				E_DEBUG2("MISSING FILE: &statusM=[%p]", (void*)&(status_inl->nameM.statusM));
				if (
					G->g_output_formM == SWVERIFY_OUTF_DIFF ||
					G->g_output_formM == SWVERIFY_OUTF_ALL
				) {
					strob_sprintf(rpt_diff, 1 /*do append */,
						"%dd%d\n"
						"< %s\n",
	 					im, is, m_s);
				}
				write_rpmv_output_line(G, status_inl, m_s_inl, target_path);
				im++;
			}
			E_DEBUG("diff");
		} else if (m_s && !s_s) {
			/* files missing at end */
			E_DEBUG("000000000000000 missing file at end");
			retval++;
			/* status_inl->nameM.statusM = INL_STATUS_MISSING_FILE; */
			set_inl_field_status_conditionally(status_inl, INL_NAME_OFFSET, INL_STATUS_MISSING_FILE);
				if (
					G->g_output_formM == SWVERIFY_OUTF_DIFF ||
					G->g_output_formM == SWVERIFY_OUTF_ALL
				) {
					strob_sprintf(rpt_diff, 1 /*do append*/,
						"%dd%d\n"
						"< %s\n",
	 					im, is, m_s);
				}
				write_rpmv_output_line(G, status_inl, m_s_inl, target_path);
			im++;
			E_DEBUG("missing2");
		} else {
			E_DEBUG("never here");
			retval++;
			im++;
		}
slack_end:
		m_s = strar_get(meta, im);
		s_s = strar_get(sys, is);
		E_DEBUG("");
	}
	E_DEBUG2("returning %d", retval);

	summary_ret = write_summary_report(G, status_inl, rpt_summary);
	E_DEBUG2("write_summary_report returned [%d]", summary_ret);

	if (summary_ret != 0) {
		/*
 		 * This check prevent a false success for the case of unsigned or
 		 * package without filelist, md5's etc.
 		 */
		retval++;
	}

	swverify_inl_delete(status_inl);
	swverify_inl_delete(m_s_inl);
	swverify_inl_delete(s_s_inl);
	strob_close(tmpbuf);
/* fprintf(stderr, "compare_diff: returning %d\n", retval); */
	return retval;
}

static
int
compare_fileset(GB * G, SWI * swi, int ofd, int ifd, STROB * rpt_diff, STROB * rpt_summary, int archive_fd, int * p_compare_status)
{
	int nullfd;
	int ret;
	XFORMAT * xformat;
	STROB * tmp;
	FILE_DIGS * digs;
	struct new_cpio_header * file_hdr;
	int ls_attributes;
	int available_attributes;
	int meta_fd;
	int sys_fd;
	char * meta_text;
	char * sys_text;
	STRAR * sys_str;
	STRAR * meta_str;
	int check_volatile;
	int check_mtime;
	int check_permissions;
	int check_contents;
	int compare_status_is_set;
	int check_owners;

	E_DEBUG("START");
	compare_status_is_set = 0;
	*p_compare_status = -1;
	meta_fd = swlib_open_memfd();
	sys_fd = swlib_open_memfd();
	nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	tmp = strob_open(100);

	available_attributes = 0;
	/* list the catalog metadata */

	E_DEBUG("");
	check_volatile = swextopt_is_option_true(SW_E_check_volatile, G->optaM);
	check_mtime = swextopt_is_option_true(SW_E_swbis_check_mtime, G->optaM);
	check_permissions = swextopt_is_option_true(SW_E_check_permissions, G->optaM);
	check_contents = swextopt_is_option_true(SW_E_check_contents, G->optaM);
	check_owners = swextopt_is_option_true(SW_E_swbis_check_owners, G->optaM);

	if (check_contents == 0)
		check_mtime = 0; /* per spec */
	
	ls_attributes = LS_LIST_VERBOSE_COMP2;
	ls_attributes &= ~(LS_LIST_VERBOSE_WITH_MD5);
	ls_attributes &= ~(LS_LIST_VERBOSE_WITH_SHA1);
	ls_attributes &= ~(LS_LIST_VERBOSE_WITH_SHA512);

	if (check_mtime == 0) {
		ls_attributes &= ~(LS_LIST_VERBOSE_WITH_REG_DATES|LS_LIST_VERBOSE_WITH_ALL_DATES);
	}

	E_DEBUG("");
	if (!check_owners)
		ls_attributes |= (LS_LIST_VERBOSE_WITHOUT_OWNERS);
	
	E_DEBUG("");
	if (!check_permissions)
		ls_attributes |= (LS_LIST_VERBOSE_WITHOUT_PERMISSIONS);
	
	E_DEBUG("");
	swpl_ls_fileset_from_iscat(swi, meta_fd, ls_attributes,
			&available_attributes, !check_volatile, check_contents);

	/* Now use the value of &available_attributes to set the
	   LS_LIST_VERBOSE_* controls.  This is required, for example,
           when the package INFO file lists only the uid/gid and not
           the uname and gname.  */	

	/* The catalog meta-data is audited and a inclusion policy is
	   formed for uid/uname and digests that are in the meta-data.
	   This is represented in (int)available_attributes */

	available_attributes |= LS_LIST_VERBOSE_ALTER_FORM|LS_LIST_VERBOSE_NORMAL |
					/* LS_LIST_VERBOSE_WITH_SIZE | */
					LS_LIST_VERBOSE_STRIPSLASH |
					LS_LIST_VERBOSE_WITH_REG_DATES;
	available_attributes &= ~LS_LIST_VERBOSE_OFF;
	if (check_mtime == 0) {
		available_attributes &= ~(LS_LIST_VERBOSE_WITH_REG_DATES|LS_LIST_VERBOSE_WITH_ALL_DATES);
	}

	if (!check_owners)
		available_attributes |= (LS_LIST_VERBOSE_WITHOUT_OWNERS);

	if (!check_permissions)
		available_attributes |= (LS_LIST_VERBOSE_WITHOUT_PERMISSIONS);

	E_DEBUG("");
	xformat = xformat_open(-1, -1, arf_ustar);
	xformat_set_ifd(xformat, archive_fd);

	E_DEBUG("");
	file_hdr = ahs_vfile_hdr(xformat_ahs_object(xformat));
	taru_init_header_digs(file_hdr);
	digs = file_hdr->digsM;

	E_DEBUG("");
	if (check_contents == 0) {
		digs->do_md5 = DIGS_ENABLE_OFF;
		digs->do_sha1 = DIGS_ENABLE_OFF;
		digs->do_sha512 = DIGS_ENABLE_OFF;
		available_attributes &= ~(LS_LIST_VERBOSE_WITH_MD5|LS_LIST_VERBOSE_WITH_SHA1|LS_LIST_VERBOSE_WITH_SHA512);
		available_attributes &= ~(LS_LIST_VERBOSE_WITH_SIZE|LS_LIST_VERBOSE_WITH_ALL_DATES);
	} else {
		available_attributes |= (LS_LIST_VERBOSE_WITH_SIZE);
		digs->do_md5 = DIGS_ENABLE_OFF;
		digs->do_sha1 = DIGS_ENABLE_OFF;
		digs->do_sha512 = DIGS_ENABLE_OFF;
		if (available_attributes & LS_LIST_VERBOSE_WITH_MD5) {
			digs->do_md5 = DIGS_ENABLE_ON;
		}
		if (available_attributes & LS_LIST_VERBOSE_WITH_SHA1) {
			digs->do_sha1 = DIGS_ENABLE_ON;
		}
		if (available_attributes & LS_LIST_VERBOSE_WITH_SHA512) {
			digs->do_sha512 = DIGS_ENABLE_ON;
		}

		if (
			(available_attributes & LS_LIST_VERBOSE_WITH_MD5) == 0 &&
			(available_attributes & LS_LIST_VERBOSE_WITH_SHA1) == 0 &&
			(available_attributes & LS_LIST_VERBOSE_WITH_SHA512) == 0
		) {
			/* The the package contains no digests, verification
			   must therefore fail */
			compare_status_is_set = 1;
			*p_compare_status = 1;
			fprintf(stderr, "%s: warning: the package does not contain file digests\n", swlib_utilname_get());
		}

	}

	E_DEBUG("");
	uxfio_lseek(archive_fd, 0, SEEK_SET);
	while ((ret = xformat_read_header(xformat)) > 0) {
		if (xformat_is_end_of_archive(xformat)){
			break;
		}
		E_DEBUG("");

		if (xformat_get_tar_typeflag(xformat) == REGTYPE) {
			E_DEBUG("");
			ret = xformat_copy_pass_digs(xformat, nullfd, archive_fd, digs);
		}
		E_DEBUG("");
		available_attributes |= LS_LIST_VERBOSE_PREPEND_DOTSLASH; 
		available_attributes |= LS_LIST_VERBOSE_LINKNAME_PLAIN;
		taru_print_tar_ls_list(tmp, file_hdr, available_attributes);
		uxfio_write(sys_fd, strob_str(tmp), strob_strlen(tmp));
	}
	
	E_DEBUG("");
	meta_text = uxfio_get_fd_mem(meta_fd, (int *)NULL);
	sys_text = uxfio_get_fd_mem(sys_fd, (int *)NULL);

	E_DEBUG("");
	/* split into lines */
	meta_str = linize(meta_text);
	sys_str = linize(sys_text);
	

	/* sort the lines */
	strar_qsort(meta_str, strar_qsort_strcmp);
	strar_qsort(sys_str, strar_qsort_strcmp);

	if (G->g_catalog_info_nameM)
		print_lines(G->g_catalog_info_nameM, "", meta_str);
	if (G->g_system_info_nameM)
		print_lines(G->g_system_info_nameM, "", sys_str);

	remove_duplicate_files(meta_str);

	/* Now compare these line by line */
	ret = compare_diff(G, swi, ofd, ifd, rpt_diff, rpt_summary, meta_str, sys_str, G->g_do_show_linesM, swi->swi_pkgM->target_pathM);

	if (compare_status_is_set == 0) {
		compare_status_is_set = 1;
		*p_compare_status = ret;
	}

	E_DEBUG("");
	strar_close(meta_str);
	strar_close(sys_str);
	uxfio_close(sys_fd);
	uxfio_close(meta_fd);
	swbis_devnull_close(nullfd);
	strob_close(tmp);
	xformat_close(xformat);
	E_DEBUG("");
	return 0;
}


int
swverify_write_source_copy_script2(GB * G,
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
	char * xx;
	STROB * locked_region;
	STROB * looper_routine;
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
	STROB * prelink_funcbuf;

	int vlv;
	char * debug_task_shell;
	
	basename = (char*)NULL;
	dirname = (char*)NULL;
	buffer = strob_open(100);
	buffer_new = strob_open(100);
	to_devnull = strob_open(100);
	is_archive_msg = strob_open(100);
	is_directory_msg = strob_open(100);
	set_vx = strob_open(100);
	tmp = strob_open(100);
	subsh = strob_open(100);
	subsh2 = strob_open(100);
	isc_msg = strob_open(100);
	shell_lib_buf = strob_open(100);
	locked_region = strob_open(100);
	looper_routine = strob_open(500);
	prelink_funcbuf = strob_open(200);

	vlv = G->g_verboseG;
	if (G->g_do_task_shell_debug == 0) {
		debug_task_shell="";
	} else {
		debug_task_shell="x";
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

	swverify_v_looper_prelink_md5_func(prelink_funcbuf);

	/* Now here is the script */
	/* ret = swlib_writef(ofd, buffer,  */
	strob_sprintf(buffer_new, STROB_DO_APPEND,
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
		"export swbis_ignore_scripts\n"
		"lock_did_lock=\"\"\n"
		"opt_force_lock=\"%s\"\n"
		"opt_allow_no_lock=\"%s\"\n"
		"export PATH\n"
		"PATH=`getconf PATH`:$PATH\n"
		"swbis_ignore_scripts=\"%s\"\n"
		"export swutilname\n"
		"swutilname=swverify\n"
		"%s\n"				/* shls_bashin2 from shell_lib.sh */
		"%s\n"				/* shls_false_ from shell_lib.sh */
		"%s\n"				/* shls_looper from  shell_lib.sh */
		"%s\n"				/* shls_looper_payload from shell_lib.sh */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* lf_ lock routine */
		"%s\n"			/* shls_make_dir_absolute */
		"%s\n"			/* shls_v_looper control function */
		"%s\n"			/* shls_v_looper_prelink function */
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
		"%s\n"    /* <<<--- shls_looper_payload */

		/* Here, in this first if-then statement we will classify and
		   type-test the target path */
									/* ism_test_begin */
		CSHID
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
				/* This is an error for swverify */
				/* throw an error */
		"		sw_retval=1\n"
		"		;;\n"
		"	\"dir\")\n"
		"		%s\n"
		"		%s\n"  
		"		swexec_status=0\n"

		CSHID
		"shls_bashin2 \"" SWBIS_TS_uts "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"sw_retval=$?\n"

		"shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		"shls_bashin2 \"" SWBIS_TS_Get_prelink_filelist "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		"shls_bashin2 \"" SWBIS_TS_Get_iscs_listing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"
		
		"shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"sw_retval=$?\n"
		"case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"sw_retval=$?\n"
		"swexec_status=$sw_retval\n"

		/* Loop over the selections */
		"		shls_looper \"$sw_retval\"\n"
		"		sw_retval=$?\n"
		"		swexec_status=$sw_retval\n"

		"		shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
		"		sw_retval=$?\n"
		"		case $sw_retval in 0) $sh_dash_s;; *) shls_false_;; esac\n"
		"		sw_retval=$?\n"
		"		swexec_status=$sw_retval\n"

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
/*_% */		shlib_get_function_text_by_name("shls_bashin2", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_false_", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_looper", shell_lib_buf, NULL),
/*_% */		shlib_get_function_text_by_name("shls_looper_payload", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_make_lockfile_name", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_make_lockfile_entry", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_test_lock", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_remove_lock", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls_make_dir_absolute", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls_v_looper", shell_lib_buf, NULL),
/*_% */         strob_str(prelink_funcbuf),
/*_% */		strob_str(set_vx),
/*_% */		blocksize,
/*_% */		dirname,
/*_% */		basename,
/*_% */		targetpath,
/*_% */		delaytime,
/*_% */		swc_get_default_sh_dash_s(G),
/*_% */		debug_task_shell,
/*_% */		strob_str(looper_routine),
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
	strob_close(buffer);
	strob_close(is_archive_msg);
	strob_close(is_directory_msg);
	strob_close(set_vx);
	strob_close(to_devnull);
	strob_close(subsh);
	strob_close(subsh2);
	strob_close(isc_msg);
	strob_close(shell_lib_buf);
	strob_close(prelink_funcbuf);
	/*
	 * 0 is OK
	 * !0 is error
	 */
	return !(ret > 0);
}

char *
swverify_v_looper_prelink_md5_func(STROB * buf)
{
	strob_strcpy(buf, "");
	strob_sprintf(buf, STROB_DO_APPEND,
	"\n"
	"shls_v_looper_func_prelink_sha1() {\n"
	"prelinked_file=\"$1\"\n"
	"PATH=/usr/sbin:/usr/bin:/sbin:\"$PATH\"\n"
	"# Prefix: xxab\n"
	"# Actually get both the md5 and sha1\n"
	"xxab_pgm=prelink\n"
	"xxab_name=`which $xxab_pgm 2>/dev/null`\n"
	"test -f \"$xxab_name\" -o -h \"$xxab_name\"\n"
	"case \"$?\" in\n"
	"0) echo 1>/dev/null ;;\n"
	"*) xxab_name=/ ;;\n"
	"esac\n"
	"PRELINK=\"$xxab_name\"\n"
	"case \"$?\" in\n"
	"0)\n"
	"# Get the md5 \n"
	"	dig1=`\"$PRELINK\" -y --md5 \"$prelinked_file\" 2>/dev/null`\n"
	"	status=\"$?\"\n"
	"	case \"$status\" in\n"
	"	0)\n"
	"	echo \"$dig1\"\n"
	"	;;\n"
	"	*)\n"
	"	PRELINK_ERROR=\"${PRELINK}_returned_status_${status}\": md5\n"
	"	echo \"$PRELINK_ERROR\"\n"
	"	;;\n"
	"	esac\n"
	"# Get the sha1 \n"
	"	dig1=`\"$PRELINK\" -y --sha \"$prelinked_file\" 2>/dev/null`\n"
	"	status=\"$?\"\n"
	"	case \"$status\" in\n"
	"	0)\n"
	"	echo \"$dig1\"\n"
	"	;;\n"
	"	*)\n"
	"	PRELINK_ERROR=\"${PRELINK}_returned_status_${status}\": sha1\n"
	"	echo \"$PRELINK_ERROR\"\n"
	"	;;\n"
	"	esac\n"
	";;\n"
	"*)\n"
	"	status=127\n"
	"	PRELINK_ERROR=\"${PRELINK}_not_found_status_${status}\"\n"
	"	echo \"$PRELINK_ERROR\" md5\n"
	"	echo \"$PRELINK_ERROR\" sha1\n"
	"	;;\n"
	"esac\n"
	"}\n"
	);
	return strob_str(buf);
}

int
swverify_looper_sr_payload(GB * G, char * target_path, SWICOL * swicol,
		SWICAT_SC * sc, SWICAT_SR * entry_sr, int ofd, int ifd, int * p_rp_status,
		SWUTS * uts, char * pax_write_command_key, int * p_compare_status)
{
	int retval;
	int rstatus;
	int ret;
	int do_skip_entry;
	char * ctmp;
	char * pax_read_command;
	char * epath;
	char * installed_software_catalog;
	char * catalog_entry_directory;
	int result;
	int sig_level;
	STROB * tmp;
	STROB * btmp;
	STROB * btmp2;
	STROB * swspec_string;
	STROB * rpt_diff;
	STROB * rpt_summary;
	STROB * diff_header;
	SWICAT_E * e;
	SWI * swi;
	SWI_FILELIST * file_list;
	SWGPG_VALIDATE * swgpg;
	int archive_fd;
	
	E_DEBUG("START");
	*p_compare_status = -1;
	archive_fd = -1;
	do_skip_entry = 0;
	retval = 0;
	rstatus = 0;
	tmp = strob_open(100);
	btmp = strob_open(100);
	btmp2 = strob_open(100);
	swspec_string = strob_open(100);
	rpt_diff = strob_open(100);
	rpt_summary = strob_open(100);
	diff_header = strob_open(100);
	swi = NULL;
	file_list = NULL;
	swgpg = NULL;
	e = NULL;

	sig_level = swlib_atoi(get_opta(G->optaM, SW_E_swbis_sig_level), &result);
	if (result) {
		sig_level = 0;
	}

	pax_read_command = swc_get_pax_read_command(G->g_pax_read_commands,
		"tar", G->g_verboseG >= SWC_VERBOSE_5,
		0 /*keep_old_files*/, DEFAULT_PAX_R);
	E_DEBUG("");
	installed_software_catalog = get_opta_isc(G->optaM, SW_E_installed_software_catalog);
	catalog_entry_directory = swicat_sr_form_catalog_path(entry_sr, installed_software_catalog, NULL);

	E_DEBUG2("catalog_entry_directory = [%s]", catalog_entry_directory);

	swicat_sr_form_swspec(entry_sr, swspec_string);

	*p_rp_status = 0;
	if (strlen(catalog_entry_directory) == 0) {
		/* return with no error
		   this happens for `the empty response` to a query */
		return 1;
	}

	/* fprintf(stderr, "ENTRY: [%s]\n", catalog_entry_directory); */

	strob_sprintf(btmp, 0, "%s\n", catalog_entry_directory);
	/* swlib_squash_trailing_vnewline(strob_str(btmp));	 */
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
	if (sig_level >= 0) {
		E_DEBUG("");
		ret = swicat_e_verify_gpg_signature(e, swgpg);
		E_DEBUG("");
	}
	
	if (ret != 0) {
		rstatus = 3;
		retval = 0;
		sw_e_msg(G, "error from swicat_e_verify_gpg_signature, ret=%d\n", ret);
	}

	E_DEBUG("");
	/* Now interpret the verification results according to extended option
	   requirements */

	E_DEBUG("");
	ret = 0;
	if (sig_level >= 0)
		ret = swpl_signature_policy_accept(G, swgpg, G->g_verboseG, strob_str(swspec_string));
	if (ret != 0) {
		/* Bad signature or not enough good signatures
		   Can't use the blob to make a removal file list
		   because were assuming its tainted. */
		rstatus = SWVERIFY_RP_STATUS_NO_INTEGRITY;
		retval = 0;
		do_skip_entry = 1;
	} else {
		/* Signatures OK, or don't care */
		;
	}

	E_DEBUG("");
	/*
	 * If we're here its OK to decode the catalog.tar file to create (SWI*)swi
	 * because its authenticated integrity or we don't care.
	 */

	E_DEBUG("");
	if (ret == 0) {
		/* Only decode if signatures pass */
		E_DEBUG("running swicat_e_open_swi(e)");
		swi = swicat_e_open_swi(e);
	} else {
		swi = NULL;	
	}

	E_DEBUG("");
	if (swi == NULL) {
		/* swi might be NULL due to file system permission,
		   handle NULL gracefully */
		E_DEBUG("swi==NULL");
		do_skip_entry = 1;
		if (rstatus != SWVERIFY_RP_STATUS_NO_INTEGRITY)
			sw_e_msg(G, "catalog read error, possible permission denied: %s/%s\n", target_path, epath);
	}

	E_DEBUG("");
	if (swi) {
		swi->swi_pkgM->catalog_entryM = strdup(epath); /* FIXME, this must set explicitly */
		swi->swi_pkgM->target_pathM = strdup(target_path); /* FIXME, this must set explicitly */

		ctmp = swverid_get_verid_value(entry_sr->swspecM, SWVERID_VERIDS_LOCATION, 1);
		if (ctmp) swi->swi_pkgM->locationM = strdup(ctmp);

		ctmp = swverid_get_verid_value(entry_sr->swspecM, SWVERID_VERIDS_QUALIFIER, 1);
		if (ctmp) swi->swi_pkgM->qualifierM = strdup(ctmp);
	}

	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	if (swi) swi->optaM = G->optaM;
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());

	error_out_and_report:

	E_DEBUG("");
	/* SWBIS_TS_report_status
	The purpose of this is solely to provide an opportunity for
	the remote script to report a problem and to re-verify script-data
	sychronization. */
	
	E_DEBUG("");
	ret = swpl_report_status(swicol, ofd, G->g_swi_event_fd);
	*p_rp_status = ret;
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
	if (ret) {
		rstatus = -6;
		retval = -5;
		goto error_out;
	}

	E_DEBUG("");
	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);

	E_DEBUG("");
	if (swi) 
		file_list = swicat_e_make_file_list(e, uts, swi);
	else
		file_list = NULL;

	E_DEBUG("");
	if (swi && file_list == NULL) {
		rstatus = 7;
		retval = 0;
		goto error_out;
	}
	
	E_DEBUG("");
	if (
		G->g_opt_previewM  ||
		G->g_verboseG > SWC_VERBOSE_3 ||
		G->devel_verboseM ||
		0
	) {
		if (file_list) show_file_list(G, file_list);
	}

	E_DEBUG("");
	if (G->devel_verboseM)
		SWLIB_INFO2("%s", strob_str(btmp));

	skip:

	E_DEBUG("");
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
			retval = -5;
			rstatus = -8;
			goto error_out;
		}
	} else {
		ret = swpl_unpack_catalog_tarfile(G, swi, ofd,
			catalog_entry_directory,
			pax_read_command,
			0 /*alt_catalog_root*/,
			G->g_swi_event_fd);
		if (ret != 0) {
			retval = -5;
			rstatus = -8;
			goto error_out;
		}
	}

	/* SWBIS_TS_retrieve_files_archive */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_retrieve_files_archive,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			retval = -6;
			rstatus = -9;
			goto error_out;
		}
	} else {
		if (1 && 0) {
			/* dead code, for testing */
			ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
       		                SWBIS_TS_retrieve_files_archive,
				SWICOL_TL_8,
				SW_SUCCESS);
			if (ret != 0) {
				rstatus = -8;
				retval = -6;
				goto error_out;
			}
		} else {
			E_DEBUG("");
			archive_fd = swlib_open_memfd();
			SWLIB_ASSERT(archive_fd > 0);

			E_DEBUG("");
			ret = swpl_retrieve_files(G, swi, swicol, e, file_list,
					ofd, ifd, archive_fd,
					pax_write_command_key, -1, -1, (FILE_DIGS*)NULL);
			E_DEBUG("");
			if (ret < 0) {
				rstatus = -10;
				retval = -11;
				goto error_out;
			} else if (ret > 0) {
				if (ret == SWP_RP_STATUS_NO_GNU_TAR) {
					sw_e_msg(G, "GNU tar or pax is required for this operation but appears missing.\n");
				}
				rstatus = 9;
			}
		}
	}

	/* Compare 
	   The authenticated metatdata is in (SWI*)swi and the tar archive of the
	   files is in (int)archive_fd */
	E_DEBUG("");

	if (rstatus) {
		*p_compare_status = 1;
		ret = 0;
	} else {
		if (archive_fd > 0) {
			/* compare the archive to the catalog */
			ret = compare_fileset(G, swi, ofd, ifd, rpt_diff, rpt_summary, archive_fd, p_compare_status);
			uxfio_close(archive_fd);
			if (
				G->g_output_formM == SWVERIFY_OUTF_DIFF ||
				G->g_output_formM == SWVERIFY_OUTF_ALL
			) {
				strob_sprintf(diff_header, 0,
					"1c1\n"
					"< INFO %s @ %s:%s using  installed_software_catalog=%s\n"
					"---\n"
					"> SYSTEM located at %s:%s\n",
			 		strob_str(swspec_string), G->g_target_terminal_host, target_path,
					get_opta_isc(G->optaM, SW_E_installed_software_catalog),
			 		G->g_target_terminal_host, target_path
					);

				if (*p_compare_status) {
					fprintf(stderr, "%s", strob_str(diff_header));
					fprintf(stderr, "%s", strob_str(rpt_diff));
				} else {
					if (G->g_do_show_linesM) {
						fprintf(stdout, "%s", strob_str(rpt_diff));
					}
				}
				if (G->g_no_summary_reportM == 0)
					fprintf(stderr, "%s", strob_str(rpt_summary));
			}

		}
	}
	archive_fd = -1;


	/*
 	 * this terminates the shls_v_looper() loop used to obtain the
 	 * prelink(8)'ed digest for verification
 	 */
	ret = swicol_send_loop_trailer(swicol, ofd);

	if (ret != 0) {
		rstatus = -11;
		retval = -12;
		goto error_out;
	}


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
			rstatus = -12;
			retval = -13;
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
			rstatus = -13;
			retval = -14;
			goto error_out;
		}
	}

	/* DISABLED; NOT USED   SWBIS_TS_Load_management_host_status */
	if (0) {
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
			rstatus = -14;
			retval = -15;
			goto error_out;
		}
        } else {
		ret = swpl_load_single_status_value (G, swi, ofd, G->g_swi_event_fd,
			SWBIS_TS_Load_management_host_status, 0 /**p_compare_status*/);
        } 
	}

	E_DEBUG("");
	/* SWBIS_TS_post_verify */
	if (
		G->g_opt_previewM ||
		do_skip_entry ||
		0 ||
		1  /* Temporary */
	) {
		ret = swpl_send_nothing_and_wait(swicol, ofd, G->g_swi_event_fd,
                        SWBIS_TS_post_verify,
			SWICOL_TL_8,
			SW_SUCCESS);
		if (ret != 0) {
			retval = -1;
		}
	} else {
		/* post verify */
	}

	E_DEBUG("");
	error_out:
	E_DEBUG("");

	strob_close(tmp);
	strob_close(btmp);
	strob_close(btmp2);
	strob_close(swspec_string);
	strob_close(rpt_diff);
	strob_close(rpt_summary);
	strob_close(diff_header);
	E_DEBUG("");
	if (file_list)
		swi_fl_delete(file_list);

	if (G->g_do_debug_events)
		swicol_show_events_to_fd(swicol, STDERR_FILENO, -1);
	if (rstatus)
		*p_rp_status = rstatus;
	if (swi) swi_delete(swi);
	if (swgpg) swgpg_delete(swgpg);
	if (e) swicat_e_delete(e);
	if (G->devel_verboseM)
		SWLIB_INFO2("NOPEN=%d", uxfio_uxfio_get_nopen());
	E_DEBUG("");
	return retval;
}


INFOLINE *
swverify_inl_create(void)
{
	INFOLINE * inl;
	inl = (INFOLINE*)malloc(sizeof(INFOLINE));

	inl->rpmvM = strob_open(16);
	init_rpmv_field(inl->rpmvM);
	init_infoitem(inl, INL_NAME_OFFSET);
	init_infoitem(inl, INL_LINKNAME_OFFSET);
	init_infoitem(inl, INL_SIZE_OFFSET);
	init_infoitem(inl, INL_PERMISSIONS_OFFSET);
	init_infoitem(inl, INL_OWNERS_OFFSET);
	init_infoitem(inl, INL_MDATE_OFFSET);
	init_infoitem(inl, INL_MD5_OFFSET);
	init_infoitem(inl, INL_SHA1_OFFSET);
	init_infoitem(inl, INL_SHA512_OFFSET);
	inl->lineM = NULL;
	
	return inl;
}

void
swverify_inl_delete(INFOLINE * inl) {
	free((void*)inl);
}

int
swverify_inl_parse(INFOLINE * inl, char * line) {
	char * s;	
	int len;

	inl->lineM = line;

	/* filename */
	s = inl_get_name(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->nameM.addrM = s;
	inl->nameM.lenM = len;
	/* E_DEBUG3("name: %*s", len, s); */
	
	/* linkname */
	s = inl_get_linkname(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->linknameM.addrM = s;
	inl->linknameM.lenM = len;
	/* E_DEBUG3("linkname: %*s", len, s); */

	/* filesize */
	s = inl_get_filesize(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->sizeM.addrM = s;
	inl->sizeM.lenM = len;

	/* mode string */
	s = inl_get_permissions(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->permissionsM.addrM = s;
	inl->permissionsM.lenM = len;

	/* owner names and ids */
	s = inl_get_ownernames(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->ownersM.addrM = s;
	inl->ownersM.lenM = len;

	/* modification date */
	s = inl_get_mdate(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->mdateM.addrM = s;
	inl->mdateM.lenM = len;

	/* md5 */
	s = inl_get_md5(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->md5M.addrM = s;
	inl->md5M.lenM = len;

	/* sha1 */
	s = inl_get_sha1(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->sha1M.addrM = s;
	inl->sha1M.lenM = len;

	/* sha512 */
	s = inl_get_sha512(line, &len);
	if (!s) SWLIB_FATAL("");
	inl->sha512M.addrM = s;
	inl->sha512M.lenM = len;

	return 0;
}
