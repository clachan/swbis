/* rpmpsf.c - Routines for rpm-to-PSF file translation.

   Copyright (C) 1998-2004,2014 Jim Lowe
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
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#ifdef HAVE_RPM_RPMLIB_H
#include "um_rpmlib.h"
#endif
#include "um_header.h"
#include "swfork.h"
#include "rpmpsf.h"
#include "swlib.h"
#include "topsf.h"
#include "uxfio.h"
#include "strob.h"
#include "swheaderline.h"
#include "swattributes.h"
#include "swintl.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swutilname.h"

#define INDEX_MALLOC_SIZE 8
#define PARSER_BEGIN 	0
#define PARSER_IN_ARRAY 1
#define PARSER_IN_EXPR  2

static char * rpmpsf_make_dfiles (TOPSF * topsf, char * dfiles, int len);
static char * de_quote_it (char * src);
static void strip_newline(char *s);

#ifndef RPMPSF_RPM_HAS_FILELANGS
#define RPMTAG_FILELANGS 1
#endif

/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */

struct header_lookup_cache {
	void *header;
	void *data;
	int count;
};

static struct header_lookup_cache HeaderLookupCacheArray[SWMAP_MAX_LENGTH + 1];
static int write_object_keyword(char *kw, int uxfio_ofd);
static int write_keyword(char *kw, int uxfio_ofd);
static int write_keyword(char *kw, int uxfio_ofd);
static int write__req(TOPSF  * topsf, int count, int NAME_TAG, int VERSION_TAG, int FLAGS_TAG, STROB * strb, STROB * swreq, STROB * rpmreq, char *);
static int get_filetype(TOPSF * topsf, char * path,  mode_t mode, char ** link_s);
static int classify_filelist(TOPSF  * topsf, char **typevector, int length);
static int rpmHeader_get_langNum(Header h, char ** chptr);


static
char *
get_sense(int flags)
{
    
    if ((flags & RPMSENSE_LESS) && (flags & RPMSENSE_EQUAL))
        return "<=";
    else if ((flags & RPMSENSE_GREATER) && (flags & RPMSENSE_EQUAL))
        return ">=";
    else if (flags & RPMSENSE_LESS)
        return "<";
    else if (flags & RPMSENSE_GREATER)
        return ">";
    else if (flags & RPMSENSE_EQUAL)
        return "==";
    else 
        return "==";

}

static
void
ascii_copy(STROB * dest, char * src)
{
	int newlen;
	char * pa;
	strob_strcpy(dest, "");
	if (*src == '<') {
		/*
		* This fixes attributes that begin with '<'
		* that are not intended to represent an
		* "included" file to swpackage.
		*/
		strob_strcat(dest, " ");
	}
	swlib_expand_escapes(&pa, &newlen, src, NULL);
	strob_strcat(dest, pa);
	/* strob_strcat(dest, "\n"); */
	free(pa);
}

static int
rpmpsf_headerGetRawEntry(Header h, int_32 tag, int_32 *type, const void **p, int_32 *c) {
	#ifdef RPMPSF_RPM_VERSION_403
		return headerGetRawEntry(h, tag, type, p, c);
	#elif defined  RPMPSF_RPM_VERSION_42
		return headerGetRawEntry(h, tag, type, p, c);
	#elif defined  RPMPSF_RPM_VERSION_402
		return headerGetRawEntry(h, tag, type, p, c);
	#else
		return headerGetRawEntry(h, tag, type, (void**)p, c);
	#endif
}

static
int 
rpmpsf_write_swdef_internal(TOPSF * topsf, int uxfio_ofd, int swdef_filetype /* 2=PSF 1=INDEX 0=INFO*/)
{
	Header h=topsf_get_rpmheader(topsf);
	char * from_name=topsf_get_cwd_prefix(topsf);
	char *name, buf[200];
	STROB * all_filesets;
	int count, type, filetype;
	int ret = 0;

	all_filesets = strob_open(100);

	headerGetEntry(h, RPMTAG_SOURCERPM, &type, (void **) &name, &count);
	if (!name && topsf->single_fileset_ == 0) {
		filetype = RPMPSF_FILE_PSF_SRC;
	} else {
		filetype = RPMPSF_FILE_PSF_BIN;
	}

	/* hack the source_prefix to get the last name component */
	if (from_name) { 
			int i=strlen(from_name);
			from_name+=strlen(from_name);
			from_name--;   /* backup one char to get off the '/' */
			/* Now back up over the name-version-release */
			while (i && *from_name != '/') {
				from_name--; i--; 
			}
			from_name++; /* move forward off the '/'*/
	} else {
		fprintf(stderr,"null value from char * from_name=topsf_get_cwd_prefix(topsf) ...fatal.\n");
		exit(1);
	}
	strncpy(buf, from_name, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';
	strcat(buf, "/");
	strcat(buf, "PSF");

	topsf_add_fl_entry(topsf, "PSF", buf, SWACFL_SRCCODE_PSF);

	if (swdef_filetype == 2 || swdef_filetype == 1) {
		/* ret += rpmpsf_write_host(topsf, uxfio_ofd, filetype); */
		ret += rpmpsf_write_distribution(topsf, uxfio_ofd, filetype, from_name);
		ret += rpmpsf_write_vendor(topsf, uxfio_ofd, filetype);
		ret += rpmpsf_write_category(topsf, uxfio_ofd, filetype);
		ret += rpmpsf_write_product(topsf, uxfio_ofd, all_filesets, filetype);
		ret += rpmpsf_write_product_control_files(topsf, uxfio_ofd);
	}

	if (swdef_filetype == 2 || swdef_filetype == 0) {
		ret += rpmpsf_write_filesets(topsf, uxfio_ofd, all_filesets, filetype, swdef_filetype);
	}

	if (ret) {
		SWBIS_ERROR_IMPL();
	}
	strob_close(all_filesets);
	return ret;
}

static
int
rpmpsf_write_machine_name(Header h, int uxfio_ofd)
{
	STROB * tmp;
	int ret;
	char * machine_type;
	char * os_name; 

	tmp = strob_open(12);

	ret = rpmpsf_get_rpmtagvalue(h, RPMTAG_ARCH, 0, (int *)NULL, tmp);
	if (ret != 0)
		return -1;

	if (fnmatch("[ix]*86", strob_str(tmp), 0) == 0) {
		/* its a x86 32-bit platform platform, set machine_type to
		   "i[3-6]86 */
		machine_type="[xi][3-6]86";
		os_name="Linux";
	} else if (fnmatch("x86_64", strob_str(tmp), 0) == 0) {
		/* 64-bit */
		machine_type="[xi]86_64";
		os_name="Linux";
	} else if (
		(
		fnmatch("BSD", strob_str(tmp), 0) == 0 ||
		fnmatch("bsd", strob_str(tmp), 0) == 0 ||
		0
		) &&
		(
		fnmatch("[ix]*86", strob_str(tmp), 0) == 0 ||
		0
		) &&
		1
	) {
		os_name = "*[bB][sS][dD]*";
		machine_type = "[xi][3-6]86";

	} else {
		/* Non-Intel platforms */
		machine_type = strob_str(tmp);
		os_name = strob_str(tmp);
	}

	rpmpsf_write_rpm_attribute(SW_A_machine_type, machine_type, 0, 0, NULL, NULL, uxfio_ofd);
	rpmpsf_write_rpm_attribute(SW_A_os_name, os_name, 0, 0, NULL, NULL, uxfio_ofd);

	strob_close(tmp);
	return 0;
}


int 
rpmpsf_write_psf(TOPSF * topsf, int uxfio_ofd)
{
	int ret = rpmpsf_write_swdef_internal(topsf, uxfio_ofd, 2 /*means psf*/);
	return ret;
}

int 
rpmpsf_write_info(TOPSF * topsf, int uxfio_ofd)
{
	int ret = rpmpsf_write_swdef_internal(topsf, uxfio_ofd, 0 /*means psf*/);
	return ret;
}

#define HEADER_DUMP_INLINE   1

int
headerDumpSw(TOPSF * topsf, FILE * f, int flags,
	     struct_MI_headerTagTableEntry tags)
{
	Header h=topsf_get_rpmheader(topsf);
	int i, k = 0, c, ct;
	struct indexEntry *p;
	struct_MI_headerTagTableEntry tage;
	char *dp;
	char *type, *tag;
	STROB *strb, *strbval;
	struct indexEntry * table;
	int langNum;
	char * chptr;
	int holdtagnumber;

	strb = strob_open(16);
	strbval = strob_open(16);

	/* First write out the length of the index (count of index entries) */
	fprintf(f, "Entry count: %d\n", h->indexUsed);

	/* Now write the index */
	p = h->index;

	for (i = 0; i < h->indexUsed; i++) {
		if (p->info.tag < 1000) {
			/* fprintf(stderr, "headerDumpSw: skipping tag %d\n", p->info.tag); */
			p++;
			continue;
		}

		switch (p->info.type) {
		case RPM_NULL_TYPE:
			type = "NULL_TYPE";
			break;
		case RPM_CHAR_TYPE:
			type = "CHAR_TYPE";
			break;
		case RPM_BIN_TYPE:
			type = "BIN_TYPE";
			break;
		case RPM_INT8_TYPE:
			type = "INT8_TYPE";
			break;
		case RPM_INT16_TYPE:
			type = "INT16_TYPE";
			break;
		case RPM_INT32_TYPE:
			type = "INT32_TYPE";
			break;
			/*case RPM_INT64_TYPE:              type = "INT64_TYPE";    break; */
		case RPM_STRING_TYPE:
			type = "STRING_TYPE";
			break;
		case RPM_STRING_ARRAY_TYPE:
			type = "STRING_ARRAY_TYPE";
			break;
		case RPM_I18NSTRING_TYPE:
			type = "I18N_STRING_TYPE";
			break;
		default:
			type = "(unknown)";
			break;
		}
		tage = tags;
		
		while (tage->name && tage->val != p->info.tag) tage++;

		if (!tage->name) {
			tag = "(unknown)";
		} else {
			tag = (char*)(tage->name);
		}
		fprintf(f, "%s \"%s ", tag, type);

		if (flags & HEADER_DUMP_INLINE) {

			/* Print the data in-line */
			dp = p->data;
			c = p->info.count;
			ct = 0;

			strob_strcpy(strbval, "");
			switch (p->info.type) {
			case RPM_INT32_TYPE:
			case RPM_INT16_TYPE:
			case RPM_INT8_TYPE:
			case RPM_CHAR_TYPE:
				k = 0;
				holdtagnumber = p->info.tag;
				while (!rpmpsf_get_rpmtagvalue(h, holdtagnumber, k++, (int *) NULL, strb)) {
					strob_strcat(strbval, " ");
					strob_cat(strbval, strb);
				}
				break;

			case RPM_BIN_TYPE:
				k = 0;
				holdtagnumber = p->info.tag;
				rpmpsf_get_rpmtagvalue(h, holdtagnumber, k, (int *) NULL, strb);
				strob_cat(strbval, strb);
				break;
		
				
				holdtagnumber = p->info.tag;
				while (!rpmpsf_get_rpmtagvalue(h, holdtagnumber, k++, (int *) NULL, strb)) {
					strob_strcat(strbval, "Data: ");
					strob_cat(strbval, strb);
					strob_strcat(strbval, "\n");
				}
				break;
			case RPM_STRING_TYPE:
			case RPM_STRING_ARRAY_TYPE:
			case RPM_I18NSTRING_TYPE:
				if (p->info.tag == RPMTAG_FILELANGS) {
					table = findEntry(h, HEADER_I18NTABLE, RPM_STRING_ARRAY_TYPE); 
					if (table) {		
						chptr = table->data; 
						for (langNum = 0; langNum < table->info.count; langNum++) {
							fprintf(f, "{%s} ", chptr);
							chptr += strlen(chptr) + 1;
						}
					}
				} else {
					k = 0;
					holdtagnumber = p->info.tag;
					while (!rpmpsf_get_rpmtagvalue(h, holdtagnumber, k++, (int *) NULL, strb)) {
						strob_strcat(strbval, " {");
						strob_cat(strbval, strb);
						strob_strcat(strbval, "}");
					}
				}
				break;
			default:
				fprintf(stderr, "headerDumpSw: Data type %d not supported\n",
					(int) p->info.type);
				exit(1);
			}
			fprintf(f, "%s", strob_str(strbval));
			fprintf(f, "\"\n");
		}
		p++;
	}
	strob_close(strbval);
	strob_close(strb);
	return 0;
}

int
rpmpsf_get_rpmtagvalue(Header h, int tagnumber, int index, int *pcount, STROB * strb)
{
	int count, type, ct = 0, j, c;
	int arrindex = tagnumber - 1000;
	char *entry, *dp;
	char buf1[100], *bufp;
	void *da;
	struct header_lookup_cache *hap = HeaderLookupCacheArray;


	if (tagnumber < 1000) {
		/*
		* Internal tag in rpm-4.0, ignore.
		*/
		strob_strcpy(strb, "\"\"");
		return 0;	
	}

	if (pcount) {
		if (!rpmpsf_headerGetRawEntry(h, tagnumber, &type, (const void**)&da, pcount)) {
			/* fprintf(stderr,"get_rpmtagvalue():headerGetRawEntry() failed for tag %d.\n", tagnumber); */
			*pcount = -1;
			return -1;
		}
		if ((
			    type == RPM_STRING_ARRAY_TYPE ||
			    type == RPM_I18NSTRING_TYPE
		    ) && da
		    ) {
			swbis_free(da);
		}
		return 0;
	}
	if (arrindex < 0 || arrindex > SWMAP_MAX_LENGTH) {
		fprintf(stderr,"get_rpmtagvalue() invalid tag array index %d.\n", arrindex);
		return -1;
	}
	if (hap[arrindex].header == h &&
	    hap[arrindex].data != 0 &&
	(swdef_pkg_maptable[arrindex].rpmtag_type == RPM_STRING_ARRAY_TYPE ||
	 swdef_pkg_maptable[arrindex].rpmtag_type == RPM_I18NSTRING_TYPE
	    )
	    ) {

		if (index >= hap[arrindex].count || index < 0) {
			/*
			fprintf(stderr,"get_rpmtagvalue(): rejecting %s lookup due to out of range index [%d].\n", 
					swdef_pkg_maptable[arrindex].rpmtag_name, index);
			*/
			return -1;
		}
		entry = ((char **) (hap[arrindex].data))[index];
		if (!entry || ((entry - (char *) (hap[arrindex].data)) < 0)) {
			strob_strcpy(strb, "\"\"");
		} else {
			ascii_copy(strb, entry);
			/* strob_strcpy(strb, entry); */
		}
		return 0;

	} else {

		if (swdef_pkg_maptable[arrindex].rpmtag_type == RPM_STRING_ARRAY_TYPE ||
		    swdef_pkg_maptable[arrindex].rpmtag_type == RPM_I18NSTRING_TYPE
		    ) {
			if (hap[arrindex].data != 0) {
				swbis_free(hap[arrindex].data);
				hap[arrindex].data = NULL;
			}
		}
		if (!rpmpsf_headerGetRawEntry(h, tagnumber, &type, (const void**)&da, &count)) {
			/*
			fprintf(stderr,"get_rpmtagvalue():headerGetRawEntry() failed for tag %d.\n", tagnumber);
			*/
			hap[arrindex].header = 0;
			return -1;
		}

		if (count <= index || index < 0) {
			/*
			fprintf(stderr,"get_rpmtagvalue():  %s lookup out of range index [%d].\n", 
					swdef_pkg_maptable[arrindex].names[SWATMAP_NAME_RPMTAG], index);
					SWATMAP_NAME_RPMTAG
			*/
			return -1;
		}
		if (type /*swdef_pkg_maptable[arrindex].rpmtag_type*/ == RPM_STRING_ARRAY_TYPE) {
			hap[arrindex].header = h;
			hap[arrindex].data = da;
			hap[arrindex].count = count;
			entry = ((char **) (da))[index];
			if (!entry || ((entry - (char *) (hap[arrindex].data)) < 0)) {
				strob_strcpy(strb, "\"\"");
			} else {
				/* strob_strcpy(strb, entry); */
				ascii_copy(strb, entry);
			}
			return 0;
		} else if (type /*swdef_pkg_maptable[arrindex].rpmtag_type*/ == RPM_I18NSTRING_TYPE) {
			if (count == 1) {
				ct = 0;
				/*entry = (char *)da; */
				entry = ((char **) (da))[0];
				while (ct++ < index)
					entry += strlen(entry) + 1;
				/* strob_strcpy(strb, entry); */
				ascii_copy(strb, entry);
				return 0;
			} else {
				hap[arrindex].header = h;
				hap[arrindex].data = da;
				hap[arrindex].count = count;
				entry = ((char **) (da))[index];
				if (!entry || ((entry - (char *) (hap[arrindex].data)) < 0)) {
					strob_strcpy(strb, "\"\"");
				} else {
					/* strob_strcpy(strb, entry); */
					ascii_copy(strb, entry);
				}
				return 0;
			}
		} else if (type /* swdef_pkg_maptable[arrindex].rpmtag_type*/ == RPM_STRING_TYPE) {
			hap[arrindex].header = 0;
			hap[arrindex].data = 0;
			hap[arrindex].count = 0;
			entry = (char *)da; 
			
			if (!entry ) {
				strob_strcpy(strb, "\"\"");
			} else {
				ascii_copy(strb, entry);
			}
			return 0;
		} else {

			if (count <= index || index < 0)
				return -1;
			dp = (char *) da;
			switch (type) {
			case RPM_INT32_TYPE:
				dp += (index * sizeof(int_32));
				snprintf(buf1, sizeof(buf1), "%d", (uint_32) * ((int_32 *) dp));
				strob_strcpy(strb, buf1);
				break;
			case RPM_INT16_TYPE:
				dp += (index * sizeof(int_16));
				snprintf(buf1, sizeof(buf1), "%d", (uint_16) * ((int_16 *) dp));
				strob_strcpy(strb, buf1);
				break;
			case RPM_INT8_TYPE:
				dp += (index * sizeof(int_8));
				snprintf(buf1, sizeof(buf1), "%d", (char) *((int_8 *) dp));
				strob_strcpy(strb, buf1);
				break;
			case RPM_BIN_TYPE:	/* one 'index' is 8 counts on 'c' */
				bufp = buf1;
				dp = da;
				ct = 0;
				c=count;
				j = sprintf(bufp, "\n");
				bufp += j;
				strob_strcpy(strb, "");
				while (c > 0) {
					j = sprintf(bufp, "   Data: ");
					bufp += j;
					while (c--) {
						j = sprintf(bufp, " %02X", (unsigned char) *(int_8 *) dp);
						ct++;
						bufp += j;
						dp += sizeof(int_8);
						if (! (ct % 8)) {
							break;
						}
					}
					sprintf(bufp, "\n");
					strob_strcat(strb, buf1);
					bufp = buf1;
				}
				break;

			case RPM_CHAR_TYPE:
				dp += (index * sizeof(unsigned char));
				sprintf(buf1, "%2x", (unsigned char) *((unsigned char *) dp));
				strob_strcpy(strb, buf1);
				break;
			case RPM_STRING_TYPE:
				ascii_copy(strb, dp);
				break;
			default:
				fprintf(stderr, "get_rpmtagvalue: Data type %d not supported\n", (int) type);
				return -6;
			}
		}
	}
	return 0;
}

int
rpmpsf_write_host(TOPSF * topsf, int uxfio_ofd, int filetype)
{
	Header h=topsf_get_rpmheader(topsf);
	int ret = 0;
	struct utsname unamestruct;
	uname(&unamestruct);
	write_object_keyword("host", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("hostname", unamestruct.nodename, -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("os_name", unamestruct.sysname, -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("os_release", unamestruct.release, -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("os_version", unamestruct.version, -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("machine_type", unamestruct.machine, -1, 0, h, NULL, uxfio_ofd);
	return ret;
}

int
rpmpsf_write_distribution(TOPSF * topsf, int uxfio_ofd, int filetype, char * from_name)
{
	char dfiles[100];
	Header h=topsf_get_rpmheader(topsf);
	int ret = 0;

	write_object_keyword("distribution", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("layout_version", "1.0", -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("dfiles", rpmpsf_make_dfiles(topsf, dfiles, 100), RPMTAG_DISTRIBUTION, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("pfiles", "pfiles", -1, 0, h, NULL, uxfio_ofd);

	ret += rpmpsf_write_rpm_attribute(SW_A_tag, NULL, RPMTAG_NAME, 0, h, NULL, uxfio_ofd);
	if (topsf->smart_path_) {
		/* Control path name prefix */
		if (filetype != RPMPSF_FILE_PSF_SRC) {
			if (strcmp(from_name, "."))
			ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, from_name, 0, 0, h, NULL, uxfio_ofd);
		} else {
			ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, strob_str(topsf->control_directoryM),
							0, 0, h, NULL, uxfio_ofd);
		}
	} else if (topsf->form_ == TOPSF_PSF_FORM2 || topsf->form_ == TOPSF_PSF_FORM3) {
		/* Control path name prefix */
		ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, strob_str(topsf->control_directoryM),
							0, 0, h, NULL, uxfio_ofd);
	} else {
		;
		/* no control_directory specified in the distribution object */
	}

	ret += rpmpsf_write_multilang_attribute("title",  RPMTAG_SUMMARY, h, uxfio_ofd);
	ret += rpmpsf_write_multilang_attribute("description", RPMTAG_DESCRIPTION, h, uxfio_ofd);

	ret += rpmpsf_write_rpm_attribute("copyright", NULL, RPMTAG_COPYRIGHT, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("architecture", NULL, RPMTAG_ARCH, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("url", NULL, RPMTAG_URL, 0, h, "unknown", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("create_time", NULL, RPMTAG_BUILDTIME, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("rpm_buildarchs", NULL, RPMTAG_BUILDARCHS, -1, h, "linux", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("rpm_default_prefix", NULL, RPMTAG_DEFAULTPREFIX, 0, h, "/", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("rpm_obsoletes", NULL, RPMTAG_OBSOLETES, -1, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("rpmversion", NULL, RPMTAG_RPMVERSION, -1, h, NULL, uxfio_ofd);
	return ret;
}

int
rpmpsf_write_multilang_attribute(char * kw, int rpmtag, Header h, int uxfio_ofd)
{
	int i, lang_num, ret=0;
	char * lang_string, *chptr,  *language_name; 
	char  keyw[120];

	lang_num=rpmHeader_get_langNum(h, &lang_string);
       
       if (lang_num < 1) {
		/* fprintf(stderr,"error: Lang_num less than 1, only printing first language.\n"); */
		lang_num=0; /* was 1 */
		return rpmpsf_write_rpm_attribute(kw, NULL, rpmtag, lang_num, h, NULL, uxfio_ofd);
	}
	chptr=lang_string;
	for (i=0; i<lang_num; i++) {
		if (chptr == NULL) {
			fprintf(stderr,"rpm lang code is null.\n");
			continue;
		}
		language_name=swintl_get_lang_name(chptr);
		if (language_name == NULL) {
			fprintf(stderr,"language name for code %s not found. Please fix swintl.c\n", chptr);
			continue;
		}
		strncpy(keyw, kw, 60); keyw[60]='\0';
		if (strlen(language_name)) strcat(keyw, ".");
		strcat(keyw, language_name);	
	
		ret += rpmpsf_write_rpm_attribute(keyw, NULL, rpmtag, i, h, NULL, uxfio_ofd);
		chptr += strlen(chptr) + 1;
	}
	return ret;
}

int
rpmpsf_write_category(TOPSF * topsf, int uxfio_ofd, int filetype)
{
	Header h=topsf_get_rpmheader(topsf);
	write_object_keyword("category", uxfio_ofd);
	rpmpsf_write_rpm_attribute(SW_A_tag, "rpm_group", RPMTAG_GROUP, -1, h, NULL, uxfio_ofd);
	return rpmpsf_write_rpm_attribute("title", (char *) (NULL), RPMTAG_GROUP, -1, h, NULL, uxfio_ofd);
}

int
rpmpsf_write_bundle(TOPSF * topsf, int uxfio_ofd, int filetype)
{
	Header h=topsf_get_rpmheader(topsf);
	write_object_keyword("bundle", uxfio_ofd);
	return rpmpsf_write_rpm_attribute(SW_A_tag, NULL, RPMTAG_GROUP, -1, h, NULL, uxfio_ofd);
}

int
rpmpsf_write_vendor(TOPSF * topsf, int uxfio_ofd, int filetype)
{
	Header h=topsf_get_rpmheader(topsf);
	char tagstring[65];
	STROB * tmp = strob_open(1);
	char *name, *p1;
	int count, type;
	int ret = 0;

	headerGetEntry(h, RPMTAG_RELEASE, &type, (void **) &name, &count);
	if (!name) {
		strob_close(tmp);
		return 0;
	}
	write_object_keyword("vendor", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("the_term_vendor_is_misleading", "true", -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute(SW_A_tag, name, -1, 0, h, NULL, uxfio_ofd);
	{
	switch(0) {
		case 0:
		headerGetEntry(h, RPMTAG_NAME, &type, (void **) &name, &count);
		if (!name) break;
		strob_sprintf(tmp, 0, "A RPM packaged release of %s-", name);
		headerGetEntry(h, RPMTAG_VERSION, &type, (void **) &name, &count);
		if (!name) break;
		strob_sprintf(tmp, 1, "%s\n", name);
		ret += rpmpsf_write_rpm_attribute("title", strob_str(tmp), -1, 0, h, NULL, uxfio_ofd);
		break;
		}
	}	
	ret += rpmpsf_write_rpm_attribute("qualifier", "release", -1, 0, h, NULL, uxfio_ofd);

	headerGetEntry(h, RPMTAG_VENDOR, &type, (void **) &name, &count);
	if (!name) {
		strob_close(tmp);
		return 0;
	}
	write_object_keyword("vendor", uxfio_ofd);
	strncpy(tagstring, name, sizeof(tagstring));
	tagstring[sizeof(tagstring) -1] = '\0';
	p1 = tagstring;
	swlib_tr(tagstring, (int)('_'), (int)(' '));
	ret += rpmpsf_write_rpm_attribute("the_term_vendor_is_misleading", "true", -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute(SW_A_tag, tagstring, -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("title", (char *) NULL, RPMTAG_VENDOR, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("qualifier", "seller", -1, 0, h, NULL, uxfio_ofd);

	headerGetEntry(h, RPMTAG_PACKAGER, &type, (void **) &name, &count);
	if (!name) {
		strob_close(tmp);
		return 0;
	}
	write_object_keyword("vendor", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("the_term_vendor_is_misleading", "true", -1, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute(SW_A_tag, (char *) NULL, RPMTAG_PACKAGER, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("title", (char *) NULL, RPMTAG_PACKAGER, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("qualifier", "packager", 0, 0, h, NULL, uxfio_ofd);
	strob_close(tmp);
	return ret;
}

int
rpmpsf_write_product(TOPSF * topsf, int uxfio_ofd, STROB * all_filesets, int filetype)
{
	Header h=topsf_get_rpmheader(topsf);
	char * cdir;
	char *name, *p1;
	int count, type;
	int ret = 0;
	STROB * tagstring = strob_open(48);
	STROB * rpmtagname = strob_open(1);
	STROB * tmp = strob_open(1);
	STROB * rev = strob_open(1);

	write_object_keyword("product", uxfio_ofd);
	/* Use the package name as the tag and control_directory */
	headerGetEntry(h, RPMTAG_NAME, &type, (void **) &name, &count);
	if (!name)
		return -1;
	strob_strcpy(rpmtagname, name);
	strob_strcpy(tagstring, name);

	cdir = strob_str(tagstring);
	if (topsf->form_ == TOPSF_PSF_FORM3 && filetype != RPMPSF_FILE_PSF_SRC) {
		ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, "", -1, 0, h, NULL, uxfio_ofd);
	} else {
		ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, cdir, -1, 0, h, NULL, uxfio_ofd);
	}
	/* Use the vendor as a base for the vendor_tag */
	headerGetEntry(h, RPMTAG_VENDOR, &type, (void **) &name, &count);
	if (!name) {
		strob_strcpy(tagstring, "unknown");
	} else {
		strob_strcpy(tagstring, name);
	}
	p1 = strob_str(tagstring);
	swlib_tr(strob_str(tagstring), (int)('_'), (int)(' '));

	if (rpmpsf_get_rpmtagvalue(h, RPMTAG_VERSION, 0, (int *)NULL, tmp)) {
		SWBIS_ERROR_IMPL();
		fprintf(stderr, "rpmpsf.c error getting RPMTAG_VERSION\n");	
		strob_strcpy(rev, "unknown");
	} else {
		strob_strcpy(rev, strob_str(tmp));
	}
	ret += rpmpsf_write_rpm_attribute(SW_A_tag, NULL, RPMTAG_NAME, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute(SW_A_revision, strob_str(rev), RPMTAG_VERSION, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute(SW_A_number, NULL, RPMTAG_RELEASE, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute(SW_A_vendor_tag, NULL, RPMTAG_RELEASE, 0, h, NULL, uxfio_ofd);

	ret += rpmpsf_write_rpm_attribute("source_package", NULL, RPMTAG_SOURCE, -1, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("all_patches", NULL, RPMTAG_PATCH, -1, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("directory", NULL, RPMTAG_INSTALLPREFIX, 0, h, "/", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("build_host", NULL, RPMTAG_BUILDHOST, 0, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("build_root", NULL, RPMTAG_BUILDROOT, 0, h, "/", uxfio_ofd);
	ret += rpmpsf_write_rpm_attribute("mod_time", NULL, RPMTAG_INSTALLTIME, -1, h, NULL, uxfio_ofd);
	ret += rpmpsf_write_machine_name(h, uxfio_ofd);

	if (filetype == RPMPSF_FILE_PSF_BIN) {
		/*
		 * Gaurd against illegal characters in the tag string here
		 */
		strob_strcpy(tagstring, strob_str(rpmtagname));
		swlib_tr(strob_str(tagstring), (int)('_'), (int)('.'));  /* '.' are not allowed in tags */
		swlib_tr(strob_str(tagstring), (int)('_'), (int)('/'));  /* '/' are not allowed in tags */
		if (swlib_check_clean_path(strob_str(tagstring)) != 0) {
			/*
			 * Fatal
			 */
			return -1;
		}
		ret += rpmpsf_write_rpm_attribute("all_filesets", strob_str(tagstring), -1, 0, h, NULL, uxfio_ofd);
	} else if (filetype == RPMPSF_FILE_PSF_SRC) {
		/*
		 * the filesets will be: source, build_control, patches
		 */
		strob_strcpy(tagstring, RPMPSF_FILESET_SOURCE);
		strob_strcat(tagstring, " ");
		strob_strcat(tagstring, RPMPSF_FILESET_BUILD);
		strob_strcat(tagstring, " ");
		strob_strcat(tagstring, RPMPSF_FILESET_PATCHES);
		ret += rpmpsf_write_rpm_attribute("all_filesets", strob_str(tagstring), -1, 0, h, NULL, uxfio_ofd);
	}
	strob_strcpy(all_filesets, strob_str(tagstring));
	strob_close(tagstring);
	strob_close(rpmtagname);
	strob_close(tmp);
	strob_close(rev);
	return ret;
}

int
rpmpsf_write_filesets(TOPSF * topsf, int uxfio_ofd, STROB * all_filesets, int filetype, int swdef_filetype)
{
	int ret = 0, i = 0;
	STROB * tmp = strob_open(1);
	char *buf, *p;

	while ((p = rpmpsf_list_iterate((i++ == 0) ? (&buf) : NULL, strob_str(all_filesets), " ")) && p) {
		if (filetype == RPMPSF_FILE_PSF_BIN) {
			strob_sprintf(tmp, 0, "%s-RUN", p);
		} else {
			strob_sprintf(tmp, 0, "%s", p);
		}
		ret += rpmpsf_write_fileset(topsf, uxfio_ofd, p, strob_str(tmp), filetype, swdef_filetype);
	}
	if (buf)
		swbis_free(buf);
	strob_close(tmp);
	return ret;
}

int
rpmpsf_write_fileset(TOPSF * topsf, int uxfio_ofd, char *fileset_tag, char * fileset_control_dir, int filetype, int swdef_filetype)
{
	Header h=topsf_get_rpmheader(topsf);
	char * cdir;
	int ret = 0;
	char titlestring[256];
	char * source_prefix=topsf_get_cwd_prefix(topsf);
	STROB *strb1;
	char * vendor_tag;

	E_DEBUG("START");
	if (!source_prefix) {
		source_prefix=rpmpsf_make_package_prefix(topsf, (char*)NULL);
	}
	topsf_set_psf_prefix(topsf, source_prefix);	
	
	strb1 = strob_open(16);

	if (swdef_filetype == 2 || swdef_filetype == 1 ) {
		write_object_keyword("fileset", uxfio_ofd);
		ret += rpmpsf_write_rpm_attribute(SW_A_tag, fileset_tag, -1, 0, h, NULL, uxfio_ofd);
		if (filetype == RPMPSF_FILE_PSF_BIN)  cdir = ""; else cdir = fileset_tag; /* This line has no effect */
		cdir = fileset_control_dir;
		/* strob_sprintf(strb1, 0, "%s-RUN", cdir); */
		if (topsf->form_ == TOPSF_PSF_FORM3 && filetype != RPMPSF_FILE_PSF_SRC) {
			ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, "", -1, 0, h, NULL, uxfio_ofd);
		} else {
			ret += rpmpsf_write_rpm_attribute(SW_A_control_directory, cdir, -1, 0, h, NULL, uxfio_ofd);
		}
	}

	swlib_strncpy(titlestring, fileset_tag, 200);
	*titlestring = toupper((int) (*titlestring));

	if (rpmpsf_get_rpmtagvalue(h, RPMTAG_RELEASE, 0, (int *)NULL, strb1)) {
		SWBIS_ERROR_IMPL();
		fprintf(stderr, "rpmpsf.c error getting RPMTAG_RELEASE\n");	
		exit(1);
	}
	vendor_tag = strdup(strob_str(strb1));

	if (!strcmp(fileset_tag, RPMPSF_FILESET_SOURCE)) {
		E_DEBUG("RPMPSF_FILESET_SOURCE");
		
		strcat(titlestring, " (pristine sources)");
		ret += rpmpsf_write_rpm_attribute("title", titlestring, -1, 0, h, NULL, uxfio_ofd);

		ret += rpmpsf_write_psf_files(topsf, uxfio_ofd, SWPM_SRCFILESET_SOURCE, source_prefix, strb1);

	} else if (!strcmp(fileset_tag, RPMPSF_FILESET_BUILD)) {
		E_DEBUG("RPMPSF_FILESET_BUILD");
		ret += rpmpsf_write_rpm_attribute("excluded_arch", NULL, RPMTAG_EXCLUDEARCH, -1, h, NULL, uxfio_ofd);
		ret += rpmpsf_write_rpm_attribute("exclude_os", NULL, RPMTAG_EXCLUDEOS, -1, h, NULL, uxfio_ofd);
		ret += rpmpsf_write_rpm_attribute("exclusive_arch", NULL, RPMTAG_EXCLUSIVEARCH, -1, h, NULL, uxfio_ofd);
		ret += rpmpsf_write_rpm_attribute("exclusive_os", NULL, RPMTAG_EXCLUSIVEOS, -1, h, NULL, uxfio_ofd);
		if (ret) {
			SWBIS_ERROR_IMPL();
		}
		
		strcat(titlestring, " (RPM spec file)");
		ret += rpmpsf_write_rpm_attribute("title", titlestring, -1, 0, h, NULL, uxfio_ofd);
		rpmpsf_write_changelogs(topsf, uxfio_ofd);

		ret += rpmpsf_write_psf_files(topsf, uxfio_ofd, SWPM_SRCFILESET_BUILD_CONTROL, source_prefix, strb1);
		if (ret) {
			SWBIS_ERROR_IMPL();
		}

	} else if (!strcmp(fileset_tag, RPMPSF_FILESET_PATCHES)) {
		E_DEBUG("RPMPSF_FILESET_PATCHES");
		strcat(titlestring, " (patches on the pristine sources)");
		ret += rpmpsf_write_rpm_attribute("title", titlestring, -1, 0, h, NULL, uxfio_ofd);

		ret += rpmpsf_write_psf_files(topsf, uxfio_ofd, SWPM_SRCFILESET_PATCH, source_prefix, strb1);

		/* binary package, just one fileset */
	} else {
		E_DEBUG("BINARY");
		if (swdef_filetype == 1 || swdef_filetype == 2 ) {
			strncpy(titlestring, "The Packaged Files (the package distribution files)", 100);
			ret += rpmpsf_write_rpm_attribute("title", titlestring, -1, 0, h, NULL, uxfio_ofd);

			/* currently RPMTAG_BUILDROOT is not in any rpm TOPSFs even though it is in
			   tagtable.c.  We definitly need it, but it will be avaliable when linked with
			   rpm because it can be gotten from the RPMVAR_BUILDROOT variable.
			 */

			rpmpsf_get_rpmtagvalue(h, RPMTAG_BUILDROOT, 0, (int *) NULL, strb1);


			ret += rpmpsf_write_rpm_attribute("excluded_arch", NULL, RPMTAG_EXCLUDEARCH, -1, h, NULL, uxfio_ofd);
			ret += rpmpsf_write_rpm_attribute("exclude_os", NULL, RPMTAG_EXCLUDEOS, -1, h, NULL, uxfio_ofd);
			ret += rpmpsf_write_rpm_attribute("exclusive_arch", NULL, RPMTAG_EXCLUSIVEARCH, -1, h, NULL, uxfio_ofd);
			ret += rpmpsf_write_rpm_attribute("exclusive_os", NULL, RPMTAG_EXCLUSIVEOS, -1, h, NULL, uxfio_ofd);
			if (ret) { SWBIS_ERROR_IMPL(); }

			ret += rpmpsf_write_requisites(topsf, RPMTAG_REQUIRENAME, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS,
							uxfio_ofd, vendor_tag);
			if (ret) { SWBIS_ERROR_IMPL(); }
			
			ret += rpmpsf_write_requisites(topsf, RPMTAG_CONFLICTNAME, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS,
							uxfio_ofd, vendor_tag);
			if (ret) { SWBIS_ERROR_IMPL(); }
		
			ret += rpmpsf_write_requisites(topsf, RPMTAG_PROVIDES, -1, -1, uxfio_ofd, vendor_tag);
			if (ret) { SWBIS_ERROR_IMPL(); }
		
			rpmpsf_write_changelogs(topsf, uxfio_ofd);
		}

		if (swdef_filetype == 2 || swdef_filetype == 0) {
			E_DEBUG("");
			if (topsf->use_recursive_ == 0) {
				E_DEBUG("entering rpmpsf_write_psf_files topsf->use_recursive_ == 0");
				ret += rpmpsf_write_psf_files(topsf, uxfio_ofd, -1, source_prefix, strb1);
			} else {
				int nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
				E_DEBUG("topsf->use_recursive_ is true");
				strob_strcpy(strb1, "directory .\nfile *\n");
				uxfio_write(uxfio_ofd, (void*)strob_str(strb1), strob_strlen(strb1));
				ret += rpmpsf_write_psf_files(topsf, nullfd, -1, source_prefix, strb1);
				swbis_devnull_close(nullfd);
			}
			if (ret) { SWBIS_ERROR_IMPL(); }
		}
	}
	strob_close(strb1);
	return ret;
}

int
rpmpsf_write_product_control_files(TOPSF * topsf, int uxfio_ofd)
{
	Header h=topsf_get_rpmheader(topsf);
	char *name;
	int count, type;

	headerGetEntry(h, RPMTAG_POSTIN, &type, (void **) &name, &count);
	if (name) {
		rpmpsf_write_product_control_file(topsf, uxfio_ofd, "postinstall",  "RPMTAG_POSTIN", RPMTAG_POSTINPROG, RPMTAG_POSTIN);
	}
	headerGetEntry(h, RPMTAG_POSTUN, &type, (void **) &name, &count);
	if (name) {
		rpmpsf_write_product_control_file(topsf, uxfio_ofd, "unpostinstall", "RPMTAG_POSTUN", RPMTAG_POSTUNPROG, RPMTAG_POSTUN);
	}
	headerGetEntry(h, RPMTAG_PREIN, &type, (void **) &name, &count);
	if (name) {
		rpmpsf_write_product_control_file(topsf, uxfio_ofd, "preinstall",  "RPMTAG_PREIN", RPMTAG_PREINPROG, RPMTAG_PREIN);
	}
	headerGetEntry(h, RPMTAG_PREUN, &type, (void **) &name, &count);
	if (name) {
		rpmpsf_write_product_control_file(topsf, uxfio_ofd, "unpreinstall",  "RPMTAG_PREUN", RPMTAG_PREUNPROG, RPMTAG_PREUN);
	}
	return 0;
}

int
rpmpsf_write_product_control_file(TOPSF * topsf, int uxfio_ofd, char *tag, char * rpmtag, int rpm_prog, int rpm_progtag)
{
	Header h=topsf_get_rpmheader(topsf);
	char * source_prefix = topsf_get_cwd_prefix(topsf);
	char * p;
	int len;

	if (!source_prefix) {
		source_prefix=rpmpsf_make_package_prefix(topsf, (char*)NULL);
	}
	topsf_set_psf_prefix(topsf, source_prefix);	
	len = strlen(topsf_get_psf_prefix(topsf)) + 25;
	p=(char*)malloc(len);
	
	swlib_strncpy(p, topsf_get_psf_prefix(topsf), len);
	strcat(p, "/");
	strcat(p, tag);

	write_object_keyword("control_file", uxfio_ofd);
	rpmpsf_write_rpm_attribute(SW_A_tag, tag, -1, 0, h, NULL, uxfio_ofd);
	rpmpsf_write_rpm_attribute("interpreter", NULL, rpm_prog, 0, h, "/bin/sh", uxfio_ofd);
	rpmpsf_write_rpm_attribute("source", p, -1, 0, h, NULL, uxfio_ofd);
	topsf_add_fl_entry(topsf, rpmtag, p, SWACFL_SRCCODE_RPMHEADER);
	swbis_free(p);
	return 0;
}

int
rpmpsf_write_rpm_attribute(char *kw, char *value, int rpmtag, int index, Header h, char *defstring, int uxfio_ofd)
{
	static STROB *strb;
	char *towrite = NULL;
	STROB * tmp;
	int count, i = 0;
	if (!strb)
		strb = strob_open(16);

	if (value) {
		if (!strlen(value) && !defstring) {
			strob_strcpy(strb, "");
			towrite = strob_str(strb);
		} else {
			towrite = value;
		}
	} else if (h) {
		if (index < 0) {
			if (rpmpsf_get_rpmtagvalue(h, rpmtag, 0, &count, strb)) {
				towrite = defstring;
			} else {
				while (i++ < count) {
					if (rpmpsf_get_rpmtagvalue(h, rpmtag, i - 1, (int *) NULL, strb)) {
						SWBIS_ERROR_IMPL();
						fprintf(stderr, "rpm2swpsf error finding value\n");
					}
					if (i < count) {
						strob_strcat(strb, " ");
					}
				}
				towrite = strob_str(strb);
			}
		} else {
			if (rpmpsf_get_rpmtagvalue(h, rpmtag, index, (int *) NULL, strb)) {
				if (defstring) {
					towrite = defstring;
				} else {
					strob_strcpy(strb, "");
					towrite = strob_str(strb);
				}
			} else {
				towrite = strob_str(strb);
			}
		}
	} else {
		SWBIS_ERROR_IMPL();
		strob_strcpy(strb, "\"\"");
		towrite = strob_str(strb);
	}

	if (kw && towrite) {
		if (strcmp(kw, SW_A_tag) == 0 || strcmp(kw, SW_A_vendor_tag) == 0) {
			/*
			 * translate '.' to '_'
			 */
			tmp = strob_open(32);
			strob_strcpy(tmp, towrite);
			swlib_tr(strob_str(tmp), (int)('_'), (int)('.'));
			write_keyword(kw, uxfio_ofd);
			swdef_write_value(strob_str(tmp), uxfio_ofd, 0, 0);
		} else {
			write_keyword(kw, uxfio_ofd);
			swdef_write_value(towrite, uxfio_ofd, 0, 0);
		}
	}
	return 0;
}

int
rpmpsf_write_requisites(TOPSF * topsf, int NAME_TAG, int VERSION_TAG, int FLAGS_TAG, int uxfio_ofd, char * vendor_tag)
{
	Header h=topsf_get_rpmheader(topsf);
	int count, type;
	char keyword[90], *val;
	STROB *strb;
	STROB *swreq;
	STROB *rpmreq;

	strb = strob_open(16);
	swreq = strob_open(16);
	rpmreq = strob_open(16);
	strob_strcpy(swreq, "");

	switch (NAME_TAG) {
	case RPMTAG_REQUIRENAME:
		if (!headerGetEntry(h, RPMTAG_REQUIRENAME, &type, (void **) &val, &count))
			return 0;

		strob_strcpy(swreq, "");
		strob_strcpy(rpmreq, "");
		if (write__req(topsf, count, NAME_TAG, VERSION_TAG, FLAGS_TAG, strb, swreq, rpmreq, vendor_tag))
			return -2000;

		swlib_strncpy(keyword, SW_A_prerequisites, sizeof(keyword));
		write_keyword(keyword, uxfio_ofd);
		swdef_write_value(strob_str(swreq), uxfio_ofd, 0, 0);

		swlib_strncpy(keyword, SW_A_rpm_requires, sizeof(keyword));
		write_keyword(keyword, uxfio_ofd);
		swdef_write_value(strob_str(rpmreq), uxfio_ofd, 0, 0);

		break;
	case RPMTAG_CONFLICTNAME:
		if (!headerGetEntry(h, RPMTAG_CONFLICTNAME, &type, (void **) &val, &count))
			return 0;

		if (write__req(topsf, count, NAME_TAG, VERSION_TAG, FLAGS_TAG, strb, swreq, rpmreq, vendor_tag))
			return -2000;

		swlib_strncpy(keyword, SW_A_exrequisites, sizeof(keyword));
		write_keyword(keyword, uxfio_ofd);
		swdef_write_value(strob_str(swreq), uxfio_ofd, 0, 0);

		swlib_strncpy(keyword, SW_A_rpm_conflicts, sizeof(keyword));
		write_keyword(keyword, uxfio_ofd);
		swdef_write_value(strob_str(rpmreq), uxfio_ofd, 0, 0);

		break;
	case RPMTAG_PROVIDES:
		rpmpsf_write_rpm_attribute("rpm_provides", NULL, RPMTAG_PROVIDES, -1, h, NULL, uxfio_ofd);
		break;
	default:
		strob_close(swreq);
		strob_close(strb);
		strob_close(rpmreq);
		return -1;
	}
	strob_close(swreq);
	strob_close(strb);
	strob_close(rpmreq);
	return 0;
}

static
int
check_is_clean_tag(int flag, char * name)
{
	if (
		strchr(name, ' ') ||
		strchr(name, '/') ||
		strchr(name, '.') ||
		strchr(name, '(') ||
		strchr(name, ')') ||
		0
	) {
		return 1;
	} else {
		if (flag & RPMSENSE_RPMLIB)
			fprintf(stderr, "rpmpsf: %s : RPMSENSE_RPMLIB is set\n", name);
		return 0;
	}
}

static int
write__req(TOPSF * topsf, int count, int NAME_TAG, int VERSION_TAG,
		int FLAGS_TAG, STROB * tmp, STROB * swreq, STROB * rpmreq, char * vendor_tag)
{
	Header h=topsf_get_rpmheader(topsf);
	int i;
	int is_rpm_spec;
	STROB * spec;
	STROB * flags;
	int ret;
	char * s;
	char * v;
	int flag_value;
	int res;
	int did1 = 0;
	int did2 = 0;

	flags = strob_open(32);
	strob_strcpy(swreq, "");
	strob_strcpy(rpmreq, "");
	strob_strcpy(tmp, "");

	i = 0;
	while (i < count && !rpmpsf_get_rpmtagvalue(h, NAME_TAG, i++, (int *) NULL, tmp)) {

		rpmpsf_get_rpmtagvalue(h, FLAGS_TAG, i - 1, (int *) NULL, flags);
		flag_value = swlib_atoi(strob_str(flags), &res);
		if (res != 0) {
			fprintf(stderr, "error converting %s\n", strob_str(flags));
			flag_value = 0;
		}
		
		is_rpm_spec = check_is_clean_tag(flag_value, strob_str(tmp));		

		if (is_rpm_spec) {
			spec = rpmreq;
			if (did1) strob_strcat(spec, "\n");
			did1++;
		} else {
			spec = swreq;
			if (did2) strob_strcat(spec, "\n");
			did2++;
		}

		strob_cat(spec, tmp);

		rpmpsf_get_rpmtagvalue(h, VERSION_TAG, i - 1, (int *) NULL, tmp);
		if ((s=strstr(strob_str(tmp), vendor_tag)) != NULL) {
			if (*(s + strlen(vendor_tag)) == '\0') {
				/* The vendor tag is part of the version
				   chop it off */
				*s = '\0';	
				/* Now chop off a trailing . or - */
				if (strob_strlen(tmp) > 0) {
					s = strob_str(tmp) + strob_strlen(tmp) - 1;
					if (*s == '-' || *s == '.') {
						*s = '\0';
					}
				}
				v = vendor_tag;
			} else {
				v = (char*)NULL;
			}
		} else if ((s=strchr(strob_str(tmp), '-')) != NULL) {
			/* revision-vendor_tag */
			if (strlen(s) > 1) {
				*s = '\0';
				s++;
				v = s;
			} else {
				v = (char*)NULL;
			}
		} else {
			v = (char*)NULL;
		}

		if (strob_strlen(tmp)) {
			strob_sprintf(spec, 1, ",pr%s%s", get_sense(flag_value), strob_str(tmp));
		}
		
		if (v != NULL && strlen(v)) {
			strob_sprintf(spec, 1, ",v=%s", v);
		}

		if (strob_strlen(flags)) {
			/* Implementation extension version id for RPM flags */
			strob_sprintf(spec, 1, ",g=%s", strob_str(flags));
		}

	}
	strob_close(flags);
	return 0;
}

int
rpmpsf_write_changelogs(TOPSF * topsf, int uxfio_ofd)
{
	Header h=topsf_get_rpmheader(topsf);
	int numchanges, i;
	char *ds;
	time_t timeval;
	char * pa;
	int newlen;
	STROB *strb;
	STROB *strbval;

	rpmpsf_get_rpmtagvalue(h, RPMTAG_CHANGELOGNAME, 0, &numchanges, NULL);
	if (!numchanges)
		return 0;

	strb = strob_open(16);
	strbval = strob_open(16);

	for (i = 0; i < numchanges; i++) {
		strob_strcpy(strbval, "");
		if (rpmpsf_get_rpmtagvalue(h, RPMTAG_CHANGELOGTIME, i, (int *) NULL, strb)) {
			return -1;
		}
		strob_cat(strbval, strb);
		timeval = (time_t) atol(strob_str(strb));
		ds = ctime(&timeval);
		if (ds) {
			strob_strcat(strbval, " ");
			strob_strcat(strbval, ds);
		} else {
			strob_strcat(strbval, "\n");
		}
		if (rpmpsf_get_rpmtagvalue(h, RPMTAG_CHANGELOGNAME, i, (int *) NULL, strb)) {
			return -1;
		}
		
		ascii_copy(strbval, strob_str(strb));

		if (rpmpsf_get_rpmtagvalue(h, RPMTAG_CHANGELOGTEXT, i, (int *) NULL, strb)) {
			return -1;
		}
		strob_cat(strbval, strb);
		/* strob_strcat(strbval, "\n"); */
		rpmpsf_write_rpm_attribute("change_log", strob_str(strbval), -1, -1, h, NULL, uxfio_ofd);
	}
	strob_close(strb);
	strob_close(strbval);
	return 0;
}

int
rpmpsf_get_index_by_name(TOPSF * topsf, char * filename)
{
	Header h=topsf_get_rpmheader(topsf);
	int numfiles, i;
	char *typevector = NULL;
	STROB * strb;

	strb = strob_open(12);
	E_DEBUG("");
	if (rpmpsf_get_rpm_filenames_tagvalue(h, 0, &numfiles, NULL)) {
		return -1;	
	}
	if (numfiles <= 0)
		return -1;

	for (i = 0; i < numfiles; i++) {
		E_DEBUG3("numfiles=%d  i=%d", numfiles, i);
		rpmpsf_get_rpm_filenames_tagvalue(h, i, (NULL), strb);
		if (strcmp(strob_str(strb), filename) == 0) {
			strob_close(strb);
			return i;
		}
	}
	return -1;
}

int
rpmpsf_write_psf_files(TOPSF * topsf, int uxfio_ofd, int typeflag, char *source_prefix, STROB * strb)
{
	Header h=topsf_get_rpmheader(topsf);
	int numfiles, i;
	char *typevector = NULL;

	numfiles = -1;
	E_DEBUG("");
	if (rpmpsf_get_rpm_filenames_tagvalue(h, 0, &numfiles, NULL)) {
		E_DEBUG2("numfiles=%d", numfiles);
		/* 
 		 * This is a normal path for a RPM with no files
 		 * therefore return zero
 		 */
		return 0;	
	}
	if (numfiles <= 0)
		return 0;

	if (typeflag >= 0) {
		E_DEBUG("classify");
		if (classify_filelist(topsf, &typevector, numfiles)) return -1;
	}
	for (i = 0; i < numfiles; i++) {
		E_DEBUG3("numfiles=%d  i=%d", numfiles, i);
		if (typeflag < 0 || typeflag == typevector[i]) {
			rpmpsf_write_file_psf(topsf, i, uxfio_ofd, source_prefix, strb);
		}
	}
	if (typeflag >= 0 && typevector) {
		swbis_free(typevector);
	}
	return 0;
}

int
rpmpsf_write_file_psf(TOPSF * topsf, int arrayindex, int uxfio_ofd, char *source_prefix, STROB * strb)
{
	Header h=topsf_get_rpmheader(topsf);
	char prefix[124], modeval[16], *pre, *sourcename, *link_s;
	char * path;
	char * bname;
	int count, type, ret = 0, lmode;
	int is_config_file;
	int fileflag;
	mode_t mode;

	E_DEBUG("");
	E_DEBUG2("source_prefix=[%s]", source_prefix);
	E_DEBUG2("strb=[%s]", strob_str(strb));

	is_config_file = 0;

	/*
	headerGetEntry(h, RPMTAG_DEFAULTPREFIX, &type, (void **) &pre, &count);
	if (pre) {
		swlib_strncpy(prefix, pre, sizeof(prefix));
	} else {
		swlib_strncpy(prefix, "", 10);
	}
	*/

	swlib_strncpy(prefix, "", 10);

	write_object_keyword("file", uxfio_ofd);

	fileflag = 0;
	if (
		rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEFLAGS, arrayindex, (int *)NULL, strb)
	) {
		fprintf(stderr,"rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEFLAGS ... failed\n");
	} else {
		fileflag = swlib_atoi(strob_str(strb), NULL);
	}
	
	ret += rpmpsf_get_rpm_filenames_tagvalue(h, arrayindex, (int *) NULL, strb);
	path = swlib_strdup(strob_str(strb));
	rpmpsf_write_rpm_attribute("path", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);

	/*
	 * Store the filename and set a value in the status array
	 */

	strar_add(topsf->header_namesM, strob_str(strb));
	E_DEBUG3("Adding to header_namesM[%d] = [%s]", arrayindex, strob_str(strb));

	strob_chr_index(topsf->file_status_arrayM,  arrayindex, TOPSF_FILE_STATUS_IN_HEADER);

	E_DEBUG2("(VOLATILE TEST)Searching archive_namesM for [%s]", path);
	if (topsf->rpmtag_default_prefixM) {
		/* Remove prefix from path in order to compare to archive names
		 */
		bname = path;
		if (strstr(bname, topsf->rpmtag_default_prefixM) == path) {
			bname = bname + strlen(topsf->rpmtag_default_prefixM);
			while (*bname == '/') bname++;
		}
	} else {
		bname = path;
	}

	if (
		fileflag & RPMFILE_CONFIG ||
		topsf_search_list(topsf, topsf->archive_namesM, bname) < 0
	) {
		rpmpsf_write_rpm_attribute("is_volatile", "true", -1, -1, h, NULL, uxfio_ofd);
	} else {
		rpmpsf_write_rpm_attribute("is_volatile", "false", -1, -1, h, NULL, uxfio_ofd);
	}

	E_DEBUG2("source_prefix=[%s]", source_prefix);
	sourcename=(char*)malloc(strlen(prefix) + strlen(source_prefix) + strlen(strob_str(strb)) + 20);	
	
	swlib_strncpy(sourcename, source_prefix, strlen(source_prefix) + 1);
	E_DEBUG2("sourcename=[%s]", sourcename);
	strncat(sourcename, prefix, strlen(prefix) + 1);
	E_DEBUG2("sourcename=[%s]", sourcename);

	if (strlen(sourcename) && sourcename[strlen(sourcename) - 1] != '/' &&
	    *(strob_str(strb)) != '/')
		strcat(sourcename, "/");
	E_DEBUG2("sourcename=[%s]", sourcename);

	strcat(sourcename, strob_str(strb));
	E_DEBUG2("sourcename=[%s]", sourcename);
	
	E_DEBUG("");
	if (topsf->info_only_ == 0)
		rpmpsf_write_rpm_attribute("source", sourcename, -1, -1, h, NULL, uxfio_ofd);
	E_DEBUG3("attribute source: [%s] [%s]", strob_str(strb), sourcename);
	topsf_add_fl_entry(topsf, strob_str(strb), sourcename, SWACFL_SRCCODE_ARCHIVE_STREAM);

	ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEUSERNAME, arrayindex, (int *) NULL, strb);
	rpmpsf_write_rpm_attribute("owner", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);

	E_DEBUG("");
	ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEGROUPNAME, arrayindex, (int *) NULL, strb);
	rpmpsf_write_rpm_attribute("group", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);

	ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEMTIMES, arrayindex, (int *) NULL, strb);
	rpmpsf_write_rpm_attribute("mtime", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);
	E_DEBUG("");

	ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEMODES, arrayindex, (int *) NULL, strb);
	mode = (mode_t) swlib_atoi(strob_str(strb), NULL);
	E_DEBUG2("rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEMODES: mode=[%04o]", mode);


	modeval[0] = (char) get_filetype(topsf, path, mode, &link_s);
	modeval[1] = '\0';
	E_DEBUG("");
	if (link_s == NULL) {
		ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILELINKTOS, arrayindex, (int *) NULL, strb);
		if (strlen(strob_str(strb)))
			rpmpsf_write_rpm_attribute("link_source", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);
	} else {
		rpmpsf_write_rpm_attribute("link_source", link_s, -1, -1, h, NULL, uxfio_ofd);
	}
	swbis_free(path);

	E_DEBUG("");
	rpmpsf_write_rpm_attribute("type", modeval, -1, -1, h, NULL, uxfio_ofd);

	if (*modeval == 's' /* symbolic link */) {
		/* Make the permissions of sylinks be always 0777 */
		mode = 0777;
		if (topsf->verboseM > 1)
			fprintf(stderr, "%s: setting perms 0777 for symlink in metadata: %s\n",
							swlib_utilname_get(), path);
	}
	memset (modeval, '\0', sizeof(modeval));

	/*
	lmode = 0;
	lmode |= (S_ISUID | S_ISGID | S_IRWXU | S_IRWXG | S_IRWXO);
	*/

	mode &= ~CP_IFMT;
	E_DEBUG2("AFTER &= ~CP_IFMT  mode=[%04o]", mode);

	/*
	snprintf(modeval, 6, "%04o", mode & lmode);
	*/	
	snprintf(modeval, 6, "%04o", mode);
	E_DEBUG2("MODEVAL=[%s]", modeval);


	rpmpsf_write_rpm_attribute("mode", modeval, -1, -1, h, NULL, uxfio_ofd);

	E_DEBUG("");
	if (topsf->info_only_) {
		ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEMD5S, arrayindex, (int *) NULL, strb);
		rpmpsf_write_rpm_attribute("md5sum", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);
		ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILESIZES, arrayindex, (int *) NULL, strb);
		rpmpsf_write_rpm_attribute("size", strob_str(strb), -1, -1, h, NULL, uxfio_ofd);
	}
	E_DEBUG("");

	swbis_free(sourcename);	
	return 0;
}

static
char *
find_last_link(TOPSF * topsf, char * key,  int i)
{
	char * linkname = NULL;
	char * name;
	CPLOB * node_names=topsf->hl_node_names_, *linkto_names=topsf->hl_linkto_names_;
	
	while ((name = cplob_val(node_names, i)) && strcmp(key, name) == 0) {
		linkname = cplob_val(linkto_names, i);
		i++;
	}
	return linkname;
}	

static 
int
get_filetype(TOPSF * topsf, char * path,  mode_t mode, char ** linksource)
{
	CPLOB * node_names=topsf->hl_node_names_, *linkto_names=topsf->hl_linkto_names_;
	char * clinkname;
	char * linkname;
	char * name;
	char * tmp;
        int i=0;

	*linksource = (char*)(NULL);
	path++;
	while ((name = cplob_val(linkto_names, i))) {
		linkname = cplob_val(node_names, i);
		if (topsf->debug_link_) fprintf(stderr, "GET_TYPE: path=[%s] name=[%s] linkname=[%s]\n", path, name, linkname); 
		if (strstr(name, "./") == name) name+=2;

		clinkname = swlib_return_no_leading(linkname);
		name = swlib_return_no_leading(name);
		path = swlib_return_no_leading(path);

		if (topsf->reverse_links_ == 0 || 1) {
			/*
			* Cpio
			*/
			if (strcmp(path, name) == 0) {
				/* fprintf(stderr, "path=[%s] name=[%s] link=[%s]\n", path, name, clinkname); */
				if (strcmp(path, clinkname) == 0) {
					*linksource = NULL;
					return (int) 'f'; 
				} else {
					*linksource = linkname;
					return (int) 'h'; 
				}
			}
		} else {
			/*
			* Tar, Dead code.
			*/
			if (strcmp(path, name) == 0) {
				/* fprintf(stderr, "path=[%s] name=[%s] link=[%s]\n", path, name, clinkname); */
				if (strcmp(path, clinkname) == 0) {
					*linksource = linkname;
					return (int) 'h'; 
				} else {
					*linksource = NULL;
					return (int) 'f'; 
				}
			}
		}
		i++;
	}
	
	if (S_ISREG(mode))
		return (int) 'f';
	else if (S_ISDIR(mode))
		return (int) 'd';
	else if (S_ISCHR(mode))
		return (int) 'c';
	else if (S_ISBLK(mode))
		return (int) 'b';
	else if (S_ISFIFO(mode))
		return (int) 'p';
	else if (S_ISLNK(mode))
		return (int) 's';  /* sym link */
	else
		return -1;
}

static
int 
write_object_keyword(char *kw, int uxfio_ofd)
{
	char buf0[100];
	snprintf(buf0, sizeof(buf0), "%s\n", kw);
	buf0[sizeof(buf0) - 1] = '\0';
	return uxfio_write(uxfio_ofd, buf0, strlen(buf0));
}

static
int 
write_keyword(char *kw, int uxfio_ofd)
{
	char buf0[100];
	snprintf(buf0, sizeof(buf0), "%s ", kw);
	buf0[sizeof(buf0) - 1] = '\0';
	return uxfio_write(uxfio_ofd, buf0, strlen(buf0));
}

char *
rpmpsf_list_iterate(char **buf, char *str, char *delim)
{
	char *p, *p1;

	if (buf) {
		(*buf) = (char *) malloc((size_t) (strlen(str) + 1));
		if (!(*buf))
			return (char *) (NULL);
		p = *buf;
		strncpy(p, str, strlen(str) + 1);
		p1 = strtok(p, delim);
	} else {
		p1 = strtok(NULL, delim);
	}

	if (p1) {
		return p1;
	} else {
		if (buf) {
			if (*buf)
				swbis_free(*buf);
			*buf = NULL;
		}
		return NULL;
	}
}

static
int
classify_filelist(TOPSF * topsf, char **typevector, int length)
{
	Header h=topsf_get_rpmheader(topsf);
	int i;
	STROB *strb;
	strb = strob_open(16);

/* 
   In a source package there are three filesets: source, patches, and build_control.
   Here the source package file is classified into one oth these filesets
   according to each files name.
 */

	(*typevector) = (char *) malloc((size_t) length * sizeof(char));
	if (!(*typevector))
		return -1;

	for (i = 0; i < length; i++)
		(*typevector)[i] = (char) 0;

	for (i = 0; i < length; i++) {
		if(rpmpsf_get_rpm_filenames_tagvalue(h, i, (int *) NULL, strb)) {
			return -1;
		}
		if (
			fnmatch("*patch", strob_str(strb), 0) == 0 ||
			fnmatch("*dif", strob_str(strb), 0) == 0 ||
			fnmatch("*diff", strob_str(strb), 0) == 0 ||
			fnmatch("*.diff*", strob_str(strb), 0) == 0 ||
			fnmatch("*.dif*", strob_str(strb), 0) == 0 ||
			fnmatch("*.patch*", strob_str(strb), 0) == 0 ||
			0
		) {
			(*typevector)[i] = (char) (SWPM_SRCFILESET_PATCH);
		} else if (!fnmatch("*spec", strob_str(strb), 0)) {
			(*typevector)[i] = (char) (SWPM_SRCFILESET_BUILD_CONTROL);
		} else if (!fnmatch("*tar.gz", strob_str(strb), 0)) {
			(*typevector)[i] = (char) (SWPM_SRCFILESET_SOURCE);
		} else {
			(*typevector)[i] = (char) (SWPM_SRCFILESET_SOURCE);
		}
	}
	strob_close(strb);
	return 0;
}

static
void
strip_newline(char *s)
{
	char *p;
	if ((p=strchr(s,'\n'))){
		*p='\0';
	}	
}

static 
char *
de_quote_it (char * src)
{
	char *c, *c0;
	c=strchr(src,'\"');
        if (!c) return src;
	c++;
        if (!strlen(c)){
                return --c;
        }
	c0=c;
	c=strchr(c,'\"');
	if (c) {
		*c='\0';
	}
	return c0;
}

static
char *
rpmpsf_make_dfiles (TOPSF * topsf, char * dfiles, int len)
{
	char *s,*s0;
	Header h=topsf_get_rpmheader(topsf);
	STROB * strb=strob_open(16);	
	if (rpmpsf_get_rpmtagvalue(h, RPMTAG_DISTRIBUTION, 0, (int*)(NULL), strb)) {
		swlib_strncpy(dfiles, RPMPSF_DEF_DFILES, 100);
		return dfiles;	
	}
      	
	swlib_strncpy(dfiles, strob_str(strb), len);
	strob_close(strb);
	dfiles[99]='\0';
		/*
		 * Now replace ' ' with '_'
		 */
	strip_newline(dfiles);
	s=s0=de_quote_it(dfiles);
	while(*s){
		if (!isalnum(*s)) *s=(int)'_';
		s++;
	}	
	return s0;
}

char *
rpmpsf_make_package_prefix(TOPSF * topsf, char * cwdir)
{
	Header h=topsf_get_rpmheader(topsf);
	char mktempbuf[70];
	char * tmpstring, *s;
	char *name;
	char *version;
	char *release;
	int type, count;

	if (cwdir) {
		tmpstring=cwdir;
	} else {
		swlib_strncpy(mktempbuf, "/usr/tmp/rpmpsfXXXXXX", 70);
		tmpstring = mktemp(mktempbuf);
	}	
	headerGetEntry(h, RPMTAG_NAME, &type, (void **) &s, &count); name=swlib_strdup(s);
	headerGetEntry(h, RPMTAG_VERSION, &type, (void **) &s, &count); version=swlib_strdup(s);
	headerGetEntry(h, RPMTAG_RELEASE, &type, (void **) &s, &count); release=swlib_strdup(s);
		
	s=(char*)malloc(strlen(tmpstring) + strlen(name) + strlen(version) + strlen(release) + 10);
	strcpy(s, tmpstring);
	
	if (strlen(tmpstring))
		strcat(s, "/");
	
	strcat(s, name);
	strcat(s, "-");
	strcat(s, version);
	strcat(s, "-");
	strcat(s, release);
	swbis_free(name); swbis_free(release); swbis_free(version);
	return s;
}

static
int 
rpmHeader_get_langNum(Header h, char ** da)
{
	struct indexEntry * table= findEntry(h, HEADER_I18NTABLE, RPM_STRING_ARRAY_TYPE);
	if (table) {
		if (da != NULL)
			*da = table->data; 
		return table->info.count;
	} else { 
		return -1;
	}
}

int
rpmpsf_get_psf_size(TOPSF * topsf)
{
	int pid, mpipe[2], amount;
	pipe(mpipe);
	pid = swfork((sigset_t*)(NULL));
	if (pid == 0) {
		close(mpipe[0]);
		rpmpsf_write_beautify_psf(topsf, mpipe[1]);
		_exit(0);
	}
	close(mpipe[1]);
        amount = swlib_read_amount(mpipe[0], -1);
	return amount;	
}

int
rpm_tagstring_to_tagno (char * tagstring, struct_MI_headerTagTableEntry tags)
{
	struct_MI_headerTagTableEntry tage;

	tage = tags;
	while (tage->name && strcmp(tagstring, tage->name))
		tage++;

	if (!tage->name)
		return -1;
	else
		return tage->val;
}

int
rpmpsf_get_rpmtag_length(TOPSF *topsf, char * tag)
{
	int type, count;
	void *da;
	Header h=topsf_get_rpmheader(topsf);
	int tagnumber;

	tagnumber = rpm_tagstring_to_tagno(tag, (struct_MI_headerTagTableEntry)rpmTagTable);
	if (!rpmpsf_headerGetRawEntry(h, tagnumber, &type, (const void**)&da, &count)) {
		fprintf(stderr,"get_rpmtag_length():headerGetRawEntry() failed for tag %d.\n", tagnumber);
		return -1;
	}	
	if (type == RPM_STRING_TYPE) {
		count=strlen((char*)(da));
	}
	return count;
}

static
int
rpmpsf_write_beautify_psf_internal(TOPSF *topsf, int fd_out, int type)
{
	int pid, mpipe[2];
	pipe(mpipe);
	pid = swfork((sigset_t*)(NULL));
	if (pid == 0) {
		close(mpipe[0]);
		/* rpmpsf_write_psf(topsf, mpipe[1]); */
		rpmpsf_write_swdef_internal(topsf, mpipe[1], type);
		_exit(0);
	}
	close(mpipe[1]);
	if (type == 2) {
		return sw_yyparse(mpipe[0], fd_out, "psf", 0, SWPARSE_FORM_INDENT);
	} else if (type == 1) {
		return sw_yyparse(mpipe[0], fd_out, "INDEX", 0, SWPARSE_FORM_INDENT);
	} else if (type == 0) {
		return sw_yyparse(mpipe[0], fd_out, "INFO", 0, SWPARSE_FORM_INDENT);
	} else {
		return -1;
	}
}

int
rpmpsf_write_beautify_psf(TOPSF *topsf, int fd_out)
{
	return rpmpsf_write_beautify_psf_internal(topsf, fd_out, 2);
}

int
rpmpsf_write_beautify_info(TOPSF *topsf, int fd_out)
{
	return rpmpsf_write_beautify_psf_internal(topsf, fd_out, 0);
}

int
rpmpsf_write_out_tag(TOPSF *topsf, int ofd)
{
	Header h=topsf_get_rpmheader(topsf);
	char * tagstring = topsf->user_addr;
	int type, count, i=0, nread=0;
	void *da, *p;
	int tagnumber=rpm_tagstring_to_tagno (tagstring, (struct_MI_headerTagTableEntry)rpmTagTable);
	if (!rpmpsf_headerGetRawEntry(h, tagnumber, &type, (const void**)&da, &count)) {
		fprintf(stderr,"write_out_tag():headerGetRawEntry() failed for tag %d.\n", tagnumber);
		return -1;
	}	
	p=(char*)da;	
	if ( type == RPM_STRING_TYPE || type == RPM_BIN_TYPE ) {
		if ( type == RPM_STRING_TYPE) {
			count=strlen(p);
		}
		while(count > 512) {
			nread=uxfio_write(ofd, (void*)((char*)p + i), 512);
			if (nread < 0) return -i;
			i+=nread;
			count-=nread;
		}
		nread=uxfio_write(ofd, (void*)((char*)p+i), count);
		i+=nread;
		return i;
	}
	return -1;
}

int
rpmpsf_get_rpm_filenames_tagvalue(Header h, int index, int *pcount, STROB * strb)
{
#ifdef USE_RPMTAG_BASENAMES
	char *p1;
	char basename[124];
	int ret;

	ret = rpmpsf_get_rpmtagvalue(h, RPMTAG_BASENAMES, index, pcount, strb);
	if (ret) {
		if (pcount == NULL) {
			fprintf(stderr,"swbis: error from f_get_rpmtagvalue, tag=%d index=%d ret=%d.\n", RPMTAG_BASENAMES, index, ret); 
		} else {
			;
			/* silence the warning when pcount is given */
		}
		return ret;
	}
	if (strb) {
		strncpy(basename, strob_str(strb), sizeof(basename) - 1);
		basename[sizeof(basename) - 1] = '\0';
		if (rpmpsf_get_rpmtagvalue(h, RPMTAG_DIRINDEXES, index, pcount, strb)) {
			fprintf(stderr,"rpmpsf_get_rpm_filenames_tagvalue():rpmpsf_get_rpmtagvalue() failed for RPMTAG_DIRINDEXES, index=%d.\n", 
					index);
			return -1;
		}
		index = swlib_atoi(strob_str(strb), NULL);
		strob_setlen(strb, 0);
		if (rpmpsf_get_rpmtagvalue(h, RPMTAG_DIRNAMES, index, pcount, strb)) {
			strob_strcpy(strb, "");
			return -1;
		}
		strob_strcat(strb, "/");
		strob_strcat(strb, basename);
		while ((p1 = strstr(strob_str(strb), "//")) != (char *) (NULL))
			memmove(p1, p1 + 1, strlen(p1));
		E_DEBUG2("strb=[%s]", strob_str(strb));
	}
	return 0;
#else
	10MAR2014 This code should never happen, Eventually remove this test
	from config.h.in and configure

	int ret;
	ret = rpmpsf_get_rpmtagvalue(h, RPMTAG_FILENAMES, index, pcount, strb);
	return ret;
#endif
}
