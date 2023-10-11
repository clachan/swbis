/* topsf.c  --  Open (almost) any format and translate to a PSF and archive.

   Copyright (C) 1999,2007,2014  Jim Lowe 
   All rights reserved.

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
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "swfork.h"
#include "topsf.h"
#include "cpiohdr.h"
#include "swvarfs.h"
#include "xformat.h"
#include "swutilname.h"
#include "rpmpsf.h"
#include "um_header.h"
#include "swgp.h"
#include "debpsf.h"
#include "strar.h"

static int print_links(TOPSF * topsf);
static void topsf_i_package_close(TOPSF * topsf);
static void topsf_i_bail(TOPSF * topsf);
static TOPSF * topsf_i_package_open(TOPSF * topsf, int oflags);
static int topsf_rpm_find_linkname(TOPSF * topsf, char * baselink, int fd, struct new_cpio_header *file_hdr, int format);
static int topsf_rpm_do_audit_hard_links(TOPSF * topsf, int fd, int oflags, char * decomp);

static
int
is_blank_line(char * s)
{
	while(*s && *s != ':' && *s != '\n') {
		if (
			isspace(*s)
		) {
			;
		} else {
			return 0;
		}
		s++;
	}
	return 1;
}

static
int
is_name(char * s)
{
	while(*s && *s != ':' && *s != '\n') {
		if (
			isalpha(*s) ||
			isdigit(*s) ||
			*s == '_' || *s == '-'
		) {
			;
		} else {
			return 0;
		}
		s++;
	}
	if (*s != ':') return 0;
	return 1;
}

void
topsf_add_excludes(TOPSF * topsf, STROB * buf)
{
	char * s;
	int i;
	i = 0;
	if (topsf->exclude_listM == NULL) return;
	while ((s=strar_get(topsf->exclude_listM, i++)) != NULL) {
		strob_sprintf(buf, STROB_DO_APPEND, SW_A_exclude " %s\n", s);
	}
}

int
parse_slack_desc(STROB * desc, STROB * title, char * src)
{
	int n;
	int fd;
	STROB * tmp;
	char * line;
	char * s;
	char * t;
	
	tmp = strob_open(32);
	fd = swlib_open_memfd();
	uxfio_write(fd, src, strlen(src));
	uxfio_lseek(fd, 0, SEEK_SET);

	strob_strcpy(title, "");
	strob_strcpy(desc, "");

	while ((n=swgp_read_line(fd, (STROB*)tmp, DO_NOT_APPEND)) > 0) {
		line = strob_str(tmp);
		s = line;

		if (*s == '#') {
			;
		} else if (is_blank_line(s)) {
			;
		} else if (is_name(s)) {
			t = strchr(s, ':');
			if (!t) return -1;
			t++;
			if (*t == ' ') t++;
			if (strob_strlen(title) == 0) {
				strob_strcpy(title, t);		
			} else if (is_blank_line(t)) {
				;
			} else {
				strob_strcat(desc, t);		
			}
		} else {
			;
		}
	}

	strob_strcpy(tmp, strob_str(desc));
	swlib_expand_escapes(NULL, NULL, strob_str(tmp), desc);

	strob_close(tmp);
	uxfio_close(fd);
	return 0;
}

int
topsf_parse_slack_pkg_name(char * pkgfilename, STRAR * st)
{
	char * n;
	char * s;
	STROB * tmp;
	char * name = NULL;
	char * version = NULL;
	char * arch = NULL;
	char * build_num = NULL;
	int ret;

	/* example: gzip-1.3.12-i486-1.tgz */

	/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
	ret = 0;
	tmp = strob_open(32);
	strob_strcpy(tmp, pkgfilename);

	n = strob_str(tmp);
	s = strrchr(n, '.');
	if (!s) return -1;
	*s = '\0';   /* squash the ``.tgz'' */

	s = strrchr(n, ':');
	if (s) {
		s++;
		n = s;
	}

	s = strrchr(n, '/');
	if (s) {
		s++;
		n = s;
	}

	s = strrchr(n, '-');
	if (!s) return -1;
	*s = '\0';
	s++;
	build_num = s;

	s = strrchr(n, '-');
	if (!s) return -1;
	*s = '\0';
	s++;
	arch = s;

	s = strrchr(n, '-');
	if (!s) return -1;
	*s = '\0';
	s++;
	version = s;

	name = n;

	if (strpbrk(name, "/:")) {
		/* bad, call this an error */
		ret = -2;
	}
	strar_add(st, name);
	strar_add(st, version);
	strar_add(st, arch);
	strar_add(st, build_num);

	strob_close(tmp);
	return ret;
}


static
int
parse_src_pkg_name(char * pkgfilename, STRAR * st)
{
	char * n;
	char * s;
	STROB * tmp;
	char * name = NULL;
	char * version = NULL;

	/* example: foo-devel-1.3.12.tar.gz */

	tmp = strob_open(32);
	strob_strcpy(tmp, pkgfilename);

	n = strob_str(tmp);
	s = NULL;
	if (!s) s = strstr(n, ".tgz");
	if (!s) s = strstr(n, ".tar");
	if (!s) s = strstr(n, ".tbz");

	if (!s) {
		;
		/* return -1; */
	} else {
		*s = '\0';   /* squash the ``.tgz'' */
	}

	s = strrchr(n, '-');
	if (!s) return -1;
	*s = '\0';
	s++;
	version = s;
	name = n;
	strar_add(st, name);
	strar_add(st, version);
	strob_close(tmp);
	return 0;
}

static
int
plain_source_tarball_write_psf_buf(TOPSF * topsf, STROB * psf)
{
	unsigned long int prefix_mode;
	unsigned long ul_dir_mtime;
	time_t dir_mtime;
	int ret;
	STROB * tmp;
	STROB * xtmp;
	STROB * desc;
	STROB * title;

	STRAR * name_fields;
	XFORMAT * xformat;
	AHS * ahs;
	char * name;

	dir_mtime = (time_t)0;
	name_fields = strar_open();
	xtmp = strob_open(32);
	tmp = strob_open(32);
	desc = strob_open(32);
	title = strob_open(32);

	if (topsf->format_desc_->pathname_prefixM == NULL) {
		return -1;
	}

	ret = parse_src_pkg_name(topsf->format_desc_->pathname_prefixM, name_fields);

	if (ret < 0) {
		fprintf(stderr, "%s: error parsing name, should be name-version{.tar|.tgz|*}: %s\n",
			swlib_utilname_get(), topsf->format_desc_->pathname_prefixM);
		return -2;
	}

	if (topsf->format_desc_->slackheaderM) {
		/* this is a tar header, use it to set the <distribution>.mode
		   attribute which set the mode of the leading path prefix. */
		struct tar_header *tar_hdr;
		tar_hdr = (struct tar_header *)(topsf->format_desc_->slackheaderM);
		taru_otoul(tar_hdr->mode, &prefix_mode);
		taru_otoul(tar_hdr->mtime, &ul_dir_mtime);
		dir_mtime = (time_t)ul_dir_mtime;
	} else {
		prefix_mode = 0;
	}

	/*
	fprintf(stderr, "SOURCE TARBALL: [%s][%s]\n",
			strar_get(name_fields, 0),
			strar_get(name_fields, 1));
	*/

	strob_sprintf(psf, STROB_NO_APPEND, "");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_distribution "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_layout_version " 1.0\n");
	strob_sprintf(psf, STROB_DO_APPEND, "dfiles dfiles\n");
	strob_sprintf(psf, STROB_DO_APPEND, "pfiles pfiles\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " %s\n", topsf->format_desc_->pathname_prefixM);
	if (topsf->checkdigestnameM) {
		strob_sprintf(psf, STROB_DO_APPEND, "checkdigest <catalog/checkdigest\n");
	}
	if (prefix_mode)
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_mode " %04o\n",
			prefix_mode & (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX));
	if (dir_mtime)
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_mtime " %lu\n", (unsigned long)dir_mtime);
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_owner " 0\n"); /* swbis imposed policy */
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_group " 0\n"); /* swbis imposed policy */


	strob_sprintf(psf, STROB_DO_APPEND, "\n");

	xformat = xformat_open(-1, -1, arf_ustar);
	ret = uxfio_lseek(topsf->fd_, 0, SEEK_SET);
	if (ret < 0) {
		E_DEBUG("error");
		return -2;
	}
	
	ret = xformat_open_archive_by_fd(xformat, topsf->fd_,
			UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_DEB_CONTROL, (mode_t)(0));
	if (ret < 0) {
		E_DEBUG("error");
		return -3;
	}

	ret = xformat_read_header(xformat);
	if (xformat_is_end_of_archive(xformat)) {
		ret = -5; fprintf(stderr, "%s: empty data archive [%d]\n", swlib_utilname_get(), ret);
		E_DEBUG("error");
		return ret;
	}

	if (xformat_get_tar_typeflag(xformat) != DIRTYPE) {
		ret = -6; fprintf(stderr, "%s: first file not a directory  [%d]\n", swlib_utilname_get(), ret);
		return ret;
	}

	ahs = xformat_ahs_object(xformat);
	name = ahsStaticGetTarFilename(ahs->file_hdrM);

	if (swlib_dir_compare(name, topsf->format_desc_->pathname_prefixM, 0)) {
		ret = -7; fprintf(stderr, "%s: first file not %s %s\n",
			swlib_utilname_get(),
			topsf->format_desc_->pathname_prefixM, name);
		return ret;
	}

	/* Make a temporary copy of the explicit file definition */
	strob_sprintf(xtmp, STROB_NO_APPEND,  SW_A_file " -t d -o 0 -g 0 -m %o %s\n",
		(unsigned int)(ahs_get_perms(ahs)), topsf->format_desc_->pathname_prefixM
		);

	E_DEBUG2("title: [%s]", strob_str(title));
	E_DEBUG2("description: [%s]", strob_str(desc));

	E_DEBUG("");
	swlib_squash_illegal_tag_chars(strar_get(name_fields, 0));
	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_product "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " %s\n", strar_get(name_fields, 0));
	
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_revision " %s\n", strar_get(name_fields, 1));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " \"\"\n");

	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_fileset "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " bin\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " \"\"\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_directory " . #/\n");

	/* Use an explicit definition for the "./" archive member */
	strob_sprintf(psf, STROB_DO_APPEND, "file_permissions -o 0 -g 0\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_file " *\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " PSF\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " catalog\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " catalog/*\n");
	
	topsf_add_excludes(topsf, psf);

	strar_close(name_fields);
	strob_close(xtmp);
	strob_close(tmp);
	strob_close(desc);
	strob_close(title);
	return 0;
}

static
int
slack_write_psf_buf(TOPSF * topsf, STROB * psf)
{
	int ret;
	STROB * tmp;
	STROB * xtmp;
	STROB * desc;
	STROB * title;
	STROB * slack_desc;
	STROB * doinst;
	STROB * vola;

	STRAR * name_fields;
	XFORMAT * xformat;
	AHS * ahs;
	struct stat st;
	char * sb;
	char * name;

	slack_desc = NULL;
	doinst = NULL;
	name_fields = strar_open();
	xtmp = strob_open(32);
	tmp = strob_open(32);
	desc = strob_open(32);
	title = strob_open(32);
	vola = strob_open(32);
	
	if (topsf->pkgfilenameM == NULL) {
		fprintf(stderr, "%s: must specify a slackware package as a regular file arg\n", swlib_utilname_get());
		E_DEBUG("error");
		return -1;
	}

	ret = topsf_parse_slack_pkg_name(topsf->pkgfilenameM, name_fields);
	if (ret < 0) {
		E_DEBUG("error");
		return -2;
	}

	/*
	fprintf(stderr, "SLACK: [%s][%s][%s][%s]\n",
			strar_get(name_fields, 0),
			strar_get(name_fields, 1),
			strar_get(name_fields, 2),
			strar_get(name_fields, 3));
	*/	

	strob_sprintf(psf, STROB_NO_APPEND, "");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_distribution "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_layout_version " 1.0\n");
	strob_sprintf(psf, STROB_DO_APPEND, "dfiles dfiles\n");
	strob_sprintf(psf, STROB_DO_APPEND, "pfiles pfiles\n");
	if (topsf->ownerM)
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_owner " %s\n", topsf->ownerM);
	if (topsf->groupM)
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_group " %s\n", topsf->groupM);

	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, "vendor\n");
	strob_sprintf(psf, STROB_DO_APPEND, "the_term_vendor_is_misleading True\n");
	swlib_squash_illegal_tag_chars(strar_get(name_fields, 3));
	strob_sprintf(psf, STROB_DO_APPEND, "tag \"%s\"\n", strar_get(name_fields, 3));
	strob_sprintf(psf, STROB_DO_APPEND, "title The Slackware Linux Project\n");
	strob_sprintf(psf, STROB_DO_APPEND, "url \"http://www.slackware.com\"\n");

	xformat = xformat_open(-1, -1, arf_ustar);
	ret = uxfio_lseek(topsf->fd_, 0, SEEK_SET);
	if (ret < 0) {
		E_DEBUG("error");
		return -2;
	}
	
	ret = xformat_open_archive_by_fd(xformat, topsf->fd_,
			UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_DEB_CONTROL, (mode_t)(0));
	if (ret < 0) {
		E_DEBUG("error");
		return -3;
	}

	ret = xformat_read_header(xformat);
	if (xformat_is_end_of_archive(xformat)) {
		ret = -5; fprintf(stderr, "%s: empty data archive [%d]\n", swlib_utilname_get(), ret);
		E_DEBUG("error");
		return ret;
	}

	if (xformat_get_tar_typeflag(xformat) != DIRTYPE) {
		ret = -6; fprintf(stderr, "%s: first file not a directory  [%d]\n", swlib_utilname_get(), ret);
		return ret;
	}

	ahs = xformat_ahs_object(xformat);
	name = ahsStaticGetTarFilename(ahs->file_hdrM);

	/* we expect name to be "./" */

	if (strcmp(name, "./")) {
		ret = -7; fprintf(stderr, "%s: first file not ./\n", swlib_utilname_get());
		return ret;
	}

	/* Make a temporary copy of the explicit file definition */
	strob_sprintf(xtmp, STROB_NO_APPEND,  SW_A_file " -t d -o %s -g %s -m %o /\n",
		ahsStaticGetTarUsername(ahs->file_hdrM),
		ahsStaticGetTarGroupname(ahs->file_hdrM),
		(unsigned int)(ahs_get_perms(ahs))
		);

	/* Now audit the ./install/ directory contents */

	name = xformat_get_next_dirent(xformat, &st);
	while (name) {
		if (
			strstr(name, "./install/") == name ||
			strstr(name, "install/") == name
		) {
			if (strstr(name, "./install/") == name)
				name += 2;
			E_DEBUG2("NAME=%s", name);
			if (strcmp(name, "install/doinst.sh") == 0) {
				topsf_h_write_to_buf(xformat, name, &doinst);
			} else if (strcmp(name, "install/") == 0) {
				;
			} else if (strcmp(name, "install/slack-desc") == 0) {
				topsf_h_write_to_buf(xformat, name, &slack_desc);
			} else {
				topsf_h_write_to_buf(xformat, name, NULL);
				fprintf(stderr, "%s: unrecognized skackware control file: %s\n",
					swlib_utilname_get(), name);
			}
		} else if (
			(sb=strstr(name, ".new")) != NULL &&  /* volatile file */
 			(*(sb+4) == '\0') 
		) {
			strob_sprintf(vola, 1, "file\n");
			strob_sprintf(vola, 1, "source %s\n", name);
			strob_sprintf(vola, 1, "path /%s\n", name);
			strob_sprintf(vola, 1, "is_volatile true\n");
		}
		name = xformat_get_next_dirent(xformat, &st);
	}

	if (slack_desc) {
		/* E_DEBUG2("%s", strob_str(slack_desc)); */
		parse_slack_desc(desc, title, strob_str(slack_desc));
		if (strrchr(strob_str(desc), '\n')) *strrchr(strob_str(desc), '\n') = '\0';
		if (strrchr(strob_str(desc), '\r')) *strrchr(strob_str(desc), '\r') = '\0';
		if (strrchr(strob_str(title), '\n')) *strrchr(strob_str(title), '\n') = '\0';
		if (strrchr(strob_str(title), '\r')) *strrchr(strob_str(title), '\r') = '\0';
	}

	E_DEBUG2("title: [%s]", strob_str(title));
	E_DEBUG2("description: [%s]", strob_str(desc));

	E_DEBUG("");
	swlib_squash_illegal_tag_chars(strar_get(name_fields, 0));
	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_product "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " %s\n", strar_get(name_fields, 0));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_vendor_tag " %s\n", strar_get(name_fields, 3));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_title " \"%s\"\n", strob_str(title));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_description " \"%s\"\n", strob_str(desc));
	
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_revision " %s\n", strar_get(name_fields, 1));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " \"\"\n");
	if (doinst)
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_postinstall " install/doinst.sh\n");

	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_fileset "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " bin\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " \"\"\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_directory " . /\n");

	/* Use an explicit definition for the "./" archive member */
	strob_sprintf(psf, STROB_DO_APPEND,  "%s", strob_str(xtmp));

	/* Recursive spec after the leading ./ */

	strob_sprintf(psf, STROB_DO_APPEND, SW_A_file " *\n");

	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " PSF\n");
	topsf_add_excludes(topsf, psf);
	strob_sprintf(psf, STROB_DO_APPEND, "#" SW_A_exclude " control\n");
	strob_sprintf(psf, STROB_DO_APPEND, "#" SW_A_exclude " control/*\n");

	strob_sprintf(psf, STROB_DO_APPEND, "%s\n", strob_str(vola));

	strar_close(name_fields);
	strob_close(xtmp);
	strob_close(tmp);
	strob_close(desc);
	strob_close(title);
	return 0;
}

static
int
slack_write_psf(TOPSF * topsf, int fd_out)
{
	int ret;
	STROB * psf;

	psf = strob_open(32);
	ret = slack_write_psf_buf(topsf, psf);
	if (ret < 0) {
		return -1;
	}
	ret = uxfio_write(fd_out, strob_str(psf), strob_strlen(psf));
	if (ret != (int)strob_strlen(psf)) {
		ret = -1;
	}
	strob_close(psf);
	return ret;
}

static
int
plain_source_tarball_psf(TOPSF * topsf, int fd_out)
{
	int ret;
	STROB * psf;
	
	psf = strob_open(32);
	/* ret = slack_write_psf_buf(topsf, psf); */
	ret = plain_source_tarball_write_psf_buf(topsf, psf);
	if (ret < 0) {
		return -1;
	}
	ret = uxfio_write(fd_out, strob_str(psf), strob_strlen(psf));
	if (ret != (int)strob_strlen(psf)) {
		ret = -1;
	}
	strob_close(psf);
	return ret;
}

static
int
find_link(TOPSF * topsf, char * keyname)
{
	int i;
	char * name;
	char * linkname;
	CPLOB * node_names=topsf->hl_node_names_, *linkto_names=topsf->hl_linkto_names_;
	
	if (strstr(keyname, "./") == keyname) keyname+=2;
	i = 0;
	while ((name = cplob_val(node_names, i))) {
		linkname = cplob_val(linkto_names, i);
		/*
		if (strstr(name, "./") == name) name+=2;
		if (strstr(linkname, "./") == linkname) linkname+=2;
		*/
		if (swlib_compare_8859(keyname, name) == 0 || swlib_compare_8859(keyname, linkname) == 0) return 1;
		i++;
	}
	return 0;
}	

void topsf_set_taru(TOPSF * topsf, TARU * taru) { topsf->taruM = taru; }

TOPSF * topsf_open(char * filename, int oflags, char * package_name)
{
	TOPSF * topsf=(TOPSF*)malloc(sizeof(TOPSF));
	
	/* oflags |= UINFILE_DETECT_NATIVE; */
	oflags |= UINFILE_DETECT_FORCEUNIXFD;

	E_DEBUG("");
	topsf->swacfl_=swacfl_open();
	E_DEBUG("");

	
	if (strcmp(filename, "-") == 0 && package_name != NULL) {
		/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
		topsf->fd_ = uinfile_open_with_name(filename, (mode_t)(0), &(topsf->format_desc_), oflags, package_name); 
	} else {
		topsf->fd_ = uinfile_open(filename, (mode_t)(0), &(topsf->format_desc_), oflags); 
	}
	E_DEBUG("");
	
	if (topsf->fd_ < 0) {
		fprintf(stderr, "%s: error opening package.\n", swlib_utilname_get());
		swacfl_close(topsf->swacfl_);
		swbis_free(topsf);
		E_DEBUG("");
		return NULL;
	}
	E_DEBUG("");
	topsf->cwd_prefix_=(char*)(NULL);
	topsf->prefix_=(char*)(NULL);
	topsf->hl_node_names_ = cplob_open(1);
	topsf->hl_linkto_names_ = cplob_open(1);
	/* topsf->archive_names_ = cplob_open(1); */
	topsf->archive_namesM = strar_open();

	topsf->file_status_arrayM = strob_open(1);
	topsf->header_namesM = strar_open();
	strob_set_fill_char(topsf->file_status_arrayM, (int)'0'/* ascii zero, i.e. decimal 48 */);
	cplob_additem(topsf->hl_node_names_, 0, NULL);
	cplob_additem(topsf->hl_linkto_names_, 0, NULL);
	/* cplob_additem(topsf->archive_names_, 0, NULL); */
	topsf->user_addr=NULL;
	topsf->use_recursive_ = 0;
	topsf->reverse_links_ = 0;
	topsf->debug_link_ = 0;
	topsf->single_fileset_ = 0;
	topsf->info_only_ = 0;
	topsf->smart_path_ = 0;
	topsf->form_ = 0;
	topsf->control_directoryM = strob_open(10);
	topsf->taruM = taru_create();
	time(&(topsf->mtimeM));
	topsf->debpsfM = NULL;
	topsf->rpmpsfM = NULL;
	if (strcmp(filename, "-") == 0)
		topsf->pkgfilenameM = NULL;
	else
		topsf->pkgfilenameM = strdup(filename);
	topsf->checkdigestnameM = NULL;
	topsf->ownerM = NULL;
	topsf->groupM = NULL;
	topsf->exclude_listM = NULL;
	topsf->rpmtag_default_prefixM = NULL;
	topsf->verboseM = 0;
	topsf->usebuf1M = strob_open(40);
	return topsf_i_package_open(topsf, oflags);
}

static void 
topsf_i_bail(TOPSF * topsf) 
{
	swacfl_close(topsf->swacfl_);
	uinfile_close(topsf->format_desc_);
	uxfio_close(topsf->fd_);	
	swbis_free(topsf);
}

void 
topsf_close(TOPSF * topsf)
{
	switch (uinfile_get_type(topsf->format_desc_)) {
		case PLAIN_TARBALL_SRC_FILEFORMAT:
			break;
		case SLACK_FILEFORMAT:
			break;
		case DEB_FILEFORMAT:
			debpsf_close((DEBPSF*)(topsf->debpsfM));
			break;
	}
	E_DEBUG("");
	swacfl_close(topsf->swacfl_);
	E_DEBUG("");
	topsf_i_package_close(topsf);
	E_DEBUG("");
	uinfile_close(topsf->format_desc_);
	E_DEBUG("");
	uxfio_close(topsf->fd_);	
	E_DEBUG("");
	if(topsf->prefix_) swbis_free(topsf->prefix_);
	E_DEBUG("");
	swbis_free(topsf);
}

static 
TOPSF *
topsf_i_package_open(TOPSF * topsf, int oflags) {
	int rc, isSource;
	STROB * tagval = strob_open(12);
	char * uncompressor;
	int tagret = -1;
	int ret;

	E_DEBUG("FUNCTION ENTER");
	switch (uinfile_get_type(topsf->format_desc_)) {
		case RPMRHS_FILEFORMAT:
			topsf->rpmfd_=rpmfd_open((FD_t)NULL, topsf->fd_);	
			rc = rpmReadPackageHeader(topsf_rpmFD(topsf), &(topsf->h_), &isSource, NULL, NULL);
			if (rc || !topsf->h_) {
				fprintf(stderr, "lxpsf: error reading package in rpmReadPackageHeader().\n");
				topsf_i_bail(topsf);
				E_DEBUG("ABNORMAL FUNCTION EXIT");
				strob_close(tagval);
				return (TOPSF*)(NULL);
			}

			#ifdef RPMPSF_RPM_HAS_PAYLOADCOMPRESSOR
				tagret = rpmpsf_get_rpmtagvalue(topsf->h_, RPMTAG_PAYLOADCOMPRESSOR, 0, (int*)(NULL), tagval);
			#endif
			if (tagret != 0) {
				strob_strcpy(tagval, "gzip");
			}
			uncompressor = strdup(strob_str(tagval));

			tagret = rpmpsf_get_rpmtagvalue(topsf->h_, RPMTAG_DEFAULTPREFIX, 0, (int*)(NULL), tagval);
			if (tagret == 0) {
				fprintf(stderr, "%s: Warning: RPM file uses deprecated RPMTAG_DEFAULTPREFIX(1056): %s\n",
							swlib_utilname_get(), strob_str(tagval));
				topsf->rpmtag_default_prefixM = strdup(strob_str(tagval));
			}

			if ((oflags & TOPSF_OPEN_NO_AUDIT) == 0) {
				if (topsf_rpm_do_audit_hard_links(topsf, topsf->fd_, oflags, uncompressor)) {
					E_DEBUG("ABNORMAL FUNCTION EXIT");
					strob_close(tagval);
					return NULL;
				}
			}
			break;
		case PLAIN_TARBALL_SRC_FILEFORMAT:
			break;
		case SLACK_FILEFORMAT:
			break;
		case DEB_FILEFORMAT:
			{
				DEBPSF * d;
				d = debpsf_create();
				ret = debpsf_open(d, (void*)topsf);
				if (ret < 0) {
					fprintf(stderr, "error decoding deb format, retval=%d\n", ret);
				}
				topsf->debpsfM = (void*)d;
			}
			break;
		default:
				fprintf(stderr, "lxpsf: unrecognized package format.\n");
				topsf_i_bail(topsf);
				E_DEBUG("ABNORMAL FUNCTION EXIT");
				strob_close(tagval);
				return (TOPSF*)(NULL);
			break;
	}
	E_DEBUG("FUNCTION EXIT");
	strob_close(tagval);
	return topsf;
}

static void
topsf_i_package_close(TOPSF * topsf) {
	E_DEBUG("FUNCTION ENTER");
	switch (uinfile_get_type(topsf->format_desc_)) {
		case RPMRHS_FILEFORMAT:
			headerFree(topsf->h_);
		break;
		case DEB_FILEFORMAT:
			break;
		case PLAIN_TARBALL_SRC_FILEFORMAT:
			break;
		case SLACK_FILEFORMAT:
			break;
		default:
			fprintf(stderr, "unrecognized format in topsf_close().\n");
		break;
	}
	E_DEBUG("FUNCTION EXIT");
}

o__inline__
int 
topsf_get_fd(TOPSF * topsf)
{
	return topsf->fd_;
}

void 
topsf_set_mtime(TOPSF * topsf, time_t tm)
{
	topsf->mtimeM = tm;
}

o__inline__
void 
topsf_set_fd(TOPSF * topsf, int fd)
{
	topsf->fd_=fd;
}

o__inline__
UINFORMAT * 
topsf_get_format_desc(TOPSF * topsf)
{
	return topsf->format_desc_;
}

o__inline__
Header 
topsf_get_rpmheader(TOPSF * topsf)
{
	return topsf->h_;
}

o__inline__
SWACFL *
topsf_get_archive_filelist(TOPSF * topsf)
{
	return topsf->swacfl_;
}

void 
topsf_add_fl_entry(TOPSF * topsf, char * to_name, char * from_name, int source_code)
{
	E_DEBUG("FUNCTION ENTER");
	/*
	* from_name is the  name in the converted archive.
	* to_name is the name the original rpm archive.
	*/
	/* fprintf(stderr, "to_name=[%s], from_name=[%s]\n", to_name, from_name);  */
	E_DEBUG3("BEGINNING: to_name=[%s] from_name=[%s]", to_name, from_name);
	if (topsf->cwd_prefix_) { 
		if (!strstr(from_name, topsf->cwd_prefix_)) {
			E_DEBUG("exception");
		} else {
			/* int i = strlen(topsf->cwd_prefix_); */
			from_name+=strlen(topsf->cwd_prefix_);
			/* if (*from_name == '/') from_name++; */
			from_name--;   /* backup one char to get off the '/' */
			/* Now back up over the name-version-release */

			/* FIXME
			*  the name-version-release is not present in all psf-forms
			*  supported by the lxpsf program
			*/
			/*
			while (*from_name != '/' && i) {
				from_name--; i--; 
				fprintf(stderr, "FROM NAME+ [%s]\n", from_name);
			}
			from_name++; */ /* move forward off the '/'*/
		}
	}
	E_DEBUG3("ADDDING ENTRY: to_name=[%s] from_name=[%s]", to_name, from_name);
	swacfl_add(topsf->swacfl_, to_name, from_name, source_code);
	E_DEBUG("FUNCTION EXIT");
}

o__inline__
char *
topsf_get_cwd_prefix(TOPSF * topsf)
{
	return topsf->cwd_prefix_;
}

o__inline__
char *
topsf_get_psf_prefix(TOPSF * topsf)
{
	return topsf->prefix_;
}

o__inline__
void
topsf_set_cwd_prefix(TOPSF * topsf, char * p)
{
	topsf->cwd_prefix_=swlib_strdup(p);
}

o__inline__
void
topsf_set_psf_prefix(TOPSF * topsf, char * p)
{
	topsf->prefix_=swlib_strdup(p);
}

static
int
topsf_rpm_do_audit_hard_links(TOPSF * topsf, int fd, int oflags, char * decomp)
{
	char buf[6];
	int buffertype;
	int uxfio_fd, pid;
	int opipe[2];	
	SHCMD *cmd[100];	
	UINFORMAT * uinformat;
	
	E_DEBUG("FUNCTION ENTER");

	buffertype = uinfile_decode_buftype(oflags, UXFIO_BUFTYPE_DYNAMIC_MEM);

	cmd[0]=shcmd_open();
	shcmd_add_arg(cmd[0], decomp);
	shcmd_add_arg(cmd[0], "-d");
	shcmd_set_exec_function(cmd[0], "execvp");
	cmd[1]=NULL;

	uinformat = topsf_get_format_desc(topsf);
	pipe(opipe);
	
	pid = swfork((sigset_t*)(NULL));
	
	if (!pid) {
		close(opipe[0]);
		shcmd_set_srcfd(cmd[0], fd);
		shcmd_set_dstfd(cmd[0], opipe[1]);
		swlib_exec_filter(cmd, -1, NULL);
		uxfio_close(fd);
		_exit(0);
	}
	close(opipe[1]);
	E_DEBUG("just before uxfio_opendup");
	uxfio_fd=uxfio_opendup(opipe[0], buffertype);
	E_DEBUG("just after uxfio_opendup");
	/* E_DEBUG2("uxfio_fd\n%s", uxfio_dump_string(uxfio_fd)); */
	
	
	/* Peek at at cpio header magic, rpm (depending on the vintage)
	   may use either format */

	if (uxfio_sfread(uxfio_fd, buf, 6) != 6) {
		return -1;
	}

	if (!strncmp(buf, "070701", 6)) {
		uinfile_set_type(uinformat, CPIO_NEWC_FILEFORMAT);
	} else if (!strncmp(buf, "070702", 6)) {
		uinfile_set_type(uinformat, CPIO_CRC_FILEFORMAT);
	} else {
		E_DEBUG("error");
		return -1;
	}

	if (uxfio_lseek(uxfio_fd,(long int)(-6), SEEK_CUR) < 0) {
		E_DEBUG("uxfio_lseek error");
		return -1;
	}
	
	if (topsf_rpm_audit_hard_links(topsf, uxfio_fd)){
		SWBIS_E_FAIL("error in topsf_audit_hard_links, exiting...");
		exit(1);
	}

	if (uxfio_lseek(uxfio_fd, 0L, SEEK_SET) < 0){
		SWBIS_E_FAIL("error from uxfio_lseek");
		return -1;
	}
	E_DEBUG2("closing fd=[%d]", fd);
	/* E_DEBUG2("fd\n%s", uxfio_dump_string(fd)); */
	uxfio_close(fd);
	E_DEBUG2("setting topsf fd to [%d]", uxfio_fd);
	topsf_set_fd(topsf, uxfio_fd);
	/* E_DEBUG2("topsf fd\n%s", uxfio_dump_string(topsf_get_fd(topsf))); */
	uinfile_set_type(uinformat, RPMRHS_FILEFORMAT);
	if (topsf->debug_link_) print_links(topsf);
	return 0;	
}

static
void
add_link_pair(TOPSF *topsf, char * base_link, char * link) {

	if (find_link(topsf, link)) return;
	/* fprintf(stderr, "ADD:  Adding [%s] ==  [%s]\n", base_link, link); */
	if (topsf->debug_link_) fprintf(stderr, "ADD:  Adding [%s] ==  [%s]\n", base_link, link);
	cplob_additem(topsf->hl_node_names_, cplob_get_nused(topsf->hl_node_names_)-1, strdup(base_link));
	cplob_additem(topsf->hl_node_names_, cplob_get_nused(topsf->hl_node_names_), NULL);
	cplob_additem(topsf->hl_linkto_names_, cplob_get_nused(topsf->hl_linkto_names_)-1, strdup(link));
	cplob_additem(topsf->hl_linkto_names_, cplob_get_nused(topsf->hl_linkto_names_), NULL);
}

int
topsf_rpm_audit_hard_links(TOPSF * topsf, int fd)
{
	STROB * tmp = strob_open(10);
	struct new_cpio_header *file_hdr=ahsStaticCreateFilehdr();
	int format=uinfile_get_type(topsf->format_desc_);

	E_DEBUG("FUNCTION ENTER");
	while(1) {
		if (taru_read_header(topsf->taruM, file_hdr, fd, format, NULL, 0) < 0){
			fprintf(stderr,"header error %s\n", ahsStaticGetTarFilename(file_hdr));
			return -1;
		}
		/* fprintf(stderr, "FILENAME = [%s]\n", ahsStaticGetTarFilename(file_hdr)); */
		if (!strcmp(ahsStaticGetTarFilename(file_hdr), "TRAILER!!!")) {
			break;
		}
		
		/* fprintf(stderr, "ADDING FILENAME = [%s]\n", ahsStaticGetTarFilename(file_hdr));  */
		/*
		cplob_additem(topsf->archive_names_, 
				cplob_get_nused(topsf->archive_names_)-1, 
						swlib_strdup(ahsStaticGetTarFilename(file_hdr)));
		cplob_additem(topsf->archive_names_, cplob_get_nused(topsf->archive_names_), NULL);
		*/

		strar_add(topsf->archive_namesM, ahsStaticGetTarFilename(file_hdr));

		/* FIXME check for eoa */
		if ((file_hdr->c_mode & CP_IFMT) == CP_IFREG) {
			if (taru_write_archive_member_data(topsf->taruM, file_hdr,
						-1, fd, (int(*)(int))NULL, format, -1, NULL) < 0){
				fprintf(stderr,"data error %s\n", ahsStaticGetTarFilename(file_hdr));
				return -2;
			}
			taru_tape_skip_padding (fd, file_hdr->c_filesize, format);
		}		

                if (file_hdr->c_nlink > 1 && (((file_hdr->c_mode & CP_IFMT) == CP_IFREG) || /* found link */
                                            ((file_hdr->c_mode & CP_IFMT) == CP_IFBLK) ||
                                            ((file_hdr->c_mode & CP_IFMT) == CP_IFCHR) ||
                                            ((file_hdr->c_mode & CP_IFMT) == CP_IFSOCK))
                 ) {
				
			if (topsf->debug_link_) fprintf(stderr, "Found First : name = %s linkname=[%s]\n", 
				ahsStaticGetTarFilename(file_hdr), ahsStaticGetTarLinkname(file_hdr));
			add_link_pair(topsf, ahsStaticGetTarFilename(file_hdr), ahsStaticGetTarFilename(file_hdr));
			strob_strcpy(tmp, ahsStaticGetTarFilename(file_hdr));
			topsf_rpm_find_linkname(topsf, strob_str(tmp), fd, file_hdr, format);
		} else  if (file_hdr->c_nlink > 1) {
			;
			/* 
			fprintf(stderr,"warning: directory with nlink greater than 1. %s\n", 
					ahsStaticGetTarFilename(file_hdr)); 
			*/
		}
	}
	ahsStaticDeleteFilehdr(file_hdr);
	strob_close(tmp);
	return 0;
}

FD_t
topsf_rpmFD(TOPSF * topsf)
{
	return rpmfd_getfd(topsf->rpmfd_);
}


static int
print_links(TOPSF * topsf)
{
	int i = 0;
	char * name;
	char * linkname;
	CPLOB * node_names=topsf->hl_node_names_, *linkto_names=topsf->hl_linkto_names_;
	
	while ((name = cplob_val(node_names, i))) {
		linkname = cplob_val(linkto_names, i);
		fprintf(stderr, "PRINT: name = [%s], linkname = [%s]\n", name, linkname ? linkname : "<nil>");
		i++;
	}
	return 0;
}	


static
int
topsf_rpm_find_linkname(TOPSF * topsf, char * base_link, int fd, struct new_cpio_header *base_file_hdr, int format)
{
	char * lastmatch=NULL;
	struct new_cpio_header *file_hdr1=ahsStaticCreateFilehdr();
	int oldoffset = uxfio_lseek(fd, 0L, SEEK_CUR);

	E_DEBUG("FUNCTION ENTER");
	while(1) {
		if (taru_read_header(topsf->taruM, file_hdr1, fd, format, NULL, 0) < 0){
			fprintf(stderr,"header error %s\n", ahsStaticGetTarFilename(file_hdr1));
			exit(1);
		}

		if ((file_hdr1->c_mode & CP_IFMT) == CP_IFREG) {
			if (taru_write_archive_member_data(topsf->taruM, file_hdr1,
						-1, fd, (int(*)(int))NULL, format, -1, NULL) < 0){
				fprintf(stderr,"data error %s\n", ahsStaticGetTarFilename(file_hdr1));
				exit(1);
			}
			taru_tape_skip_padding (fd, file_hdr1->c_filesize, format);
		}		
		if (file_hdr1->c_ino == base_file_hdr->c_ino){  /* FIXME need to check the device */
			if (lastmatch) swbis_free(lastmatch);	
			if (topsf->debug_link_)
				fprintf(stderr, "*Found linked files %s %s\n",
					base_link, ahsStaticGetTarFilename(file_hdr1));
			add_link_pair(topsf, base_link, ahsStaticGetTarFilename(file_hdr1));
			lastmatch=swlib_strdup(ahsStaticGetTarFilename(file_hdr1));
		}
		
		if (!strcmp(ahsStaticGetTarFilename(file_hdr1),"TRAILER!!!"))
			break;
	}
	ahsStaticSetPaxLinkname(base_file_hdr, lastmatch);
	ahsStaticDeleteFilehdr(file_hdr1);
	E_DEBUG("FUNCTION EXIT");
	if (uxfio_lseek(fd, oldoffset, SEEK_SET) != oldoffset) {
		fprintf(stderr, "error seeking back in topsf_rpm_find_linkname\n");
		exit(2);
	}
	return 0;
}

int
topsf_copypass_swacfl_write_missing_files(TOPSF * topsf, XFORMAT * xux, SWVARFS * swvarfs, int ofd)
{
	char * missing_array;
	char * ch;
	int c;
	int headerindex;
	int fnameindex;
	int hret;
	char * name;
	STROB * fname;
	STROB * strb;
	STROB * linkname;
	mode_t mode;
	Header h;
	int ret;
	AHS * ahs;
	time_t tm;
	int nullfd;

	tm = time(NULL);

	h = topsf->h_;
	fname = strob_open(32);
	strb = strob_open(32);
	linkname = strob_open(32);
	missing_array = strob_str(topsf->file_status_arrayM);
	ch = missing_array; /* Nul terminated */

	E_DEBUG("START");
	while(*ch) {
		c = (int)(*ch);
		headerindex = (int)(ch - missing_array);
		if (c == TOPSF_FILE_STATUS_IN_HEADER && topsf->rpm_construct_missing_filesM == 0) {
			/*
			 * Do not construct it
			 */
			rpmpsf_get_rpm_filenames_tagvalue(h, headerindex, (int *)NULL, fname);
			fprintf(stderr, "%s: RPM file entry missing from archive: %s\n",
							swlib_utilname_get(), strob_str(fname));
		} else if (c == TOPSF_FILE_STATUS_IN_HEADER && topsf->rpm_construct_missing_filesM) {
			/*
			 * Found missing file
			 * Write out the file as zero length using the stats from the rpm Header
			 */
			E_DEBUG2("found missing at index=%d", headerindex);
			rpmpsf_get_rpm_filenames_tagvalue(h, headerindex, (int *)NULL, fname);
			E_DEBUG2("missing filename=%s", strob_str(fname));

			ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEMODES, headerindex, (int *) NULL, strb);
			mode = (mode_t) swlib_atoi(strob_str(strb), NULL);
			if (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode)) {
				ahs = xformat_ahs_object(xux);
				ahs_init_header(ahs);
				if ( S_ISLNK(mode) ) {
					/*
					 * Need to find the linksource name from the Header
					 */
					fnameindex = rpmpsf_get_index_by_name(topsf, strob_str(fname));
					if (fnameindex >= 0) {
						hret = rpmpsf_get_rpmtagvalue(h, RPMTAG_FILELINKTOS,
									fnameindex,
									(NULL), linkname);
						if (hret) {
							/*
							 * Error
							 */
							ret = -1;
							goto SKIPLINK;
						}
						xformat_set_linkname(xux, strob_str(linkname));
						xformat_set_mode(xux, (mode_t)(C_ISLNK|0777));
						if (topsf->verboseM > 1)
							fprintf(stderr, "%s: setting perms 0777 for symlink in archive: %s\n",
							swlib_utilname_get(), strob_str(fname));
					} else {
						;
						/*
						 * Error
						 */
						ret = -1;
						goto SKIPLINK;
					} 
				} else {
					xformat_set_mode(xux, mode);
				}
				swlib_squash_leading_slash(strob_str(fname));
				xformat_set_name(xux, strob_str(fname));
	
				ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEUSERNAME, headerindex, (int *) NULL, strb);
				xformat_set_username(xux, strob_str(strb));
				xformat_set_uid(xux, (uid_t)99);
				xformat_set_user_systempair(xux, strob_str(strb));

				ret += rpmpsf_get_rpmtagvalue(h, RPMTAG_FILEGROUPNAME, headerindex, (int *) NULL, strb);
				xformat_set_groupname (xux, strob_str(strb));
				xformat_set_gid(xux, (gid_t)99);
				xformat_set_group_systempair(xux, strob_str(strb));

				xformat_set_mtime(xux, tm);
				xformat_set_filesize(xux, 0);
				nullfd = open("/dev/null", O_RDWR, 0);
				ret = xformat_write_by_fd(xux, nullfd, ahs_vfile_hdr(ahs));
				close(nullfd);
SKIPLINK:
				if (ret < 0) {
					fprintf(stderr, "%s: error constructing missing file: %s\n",
							swlib_utilname_get(), strob_str(fname));
				} else {
					if (topsf->verboseM > 1)
						fprintf(stderr, "%s: file constructed for RPM Header entry with missing file: %s\n",
							swlib_utilname_get(), strob_str(fname));
				}
				E_DEBUG2("writing missing file: xformat_write_by_fd returned [%d]", (int)ret);
			} else {
				fprintf(stderr, "%s: not creating missing file (filetype not supported): %s\n",
							swlib_utilname_get(), strob_str(fname));
			}
		} else if (c == TOPSF_FILE_STATUS_IN_ARCHIVE) {
			/* OK, good
			 * Nothing to do
			 */
			; 
		} else {
			/* Bad,Never happens, should be hard error */
			fprintf(stderr, "fatal error in topsf_copypass_swacfl_write_missing_files\n");
			exit(2);
		}
		ch++;
	}
	strob_close(linkname);
	strob_close(fname);
	strob_close(strb);
	return 0;
}

int
topsf_copypass_swacfl_header_list(TOPSF * topsf, XFORMAT * xxformat, SWVARFS * swvarfs, int ofd)
{
	int done;
	int en_index=0;
	int pheader[2];
	char * p;
	char * savep;
	pid_t pid;
	struct new_cpio_header *file_hdr=ahsStaticCreateFilehdr();
	swacfl_entry * en;
	SWACFL *swacfl=topsf->swacfl_;
	int (*do_write_file)(TOPSF * topsf, int o_fd);

	done = 0;
	while (!done && (en=(swacfl_entry*)cplob_val(swacfl->entry_array_, en_index++))) {
		savep = swlib_strdup(strob_str(en->archiveNameM));
		p = savep;
		if (*p == '/') p++;	
		switch(en->source_code_){
			case SWACFL_SRCCODE_ARCHIVE_STREAM:	
				done = 1;
				break;	
			
			case SWACFL_SRCCODE_RPMHEADER:
			case SWACFL_SRCCODE_PSF:

				E_DEBUG("in SWACFL_SRCCODE_PSF");
				file_hdr->c_magic=CPIO_NEWASCII_MAGIC;	
				file_hdr->c_ino=1;
				file_hdr->c_mode = (0644);
				file_hdr->c_mode |= CP_IFREG;
				file_hdr->c_uid=getuid();
				file_hdr->c_gid=getgid();
				file_hdr->c_nlink=1;
				ahsStaticSetPaxLinkname(file_hdr, NULL);
				file_hdr->c_mtime=topsf->mtimeM;
				file_hdr->c_filesize=0;
				file_hdr->c_namesize=strlen(en->from_name_) +1;
				file_hdr->c_chksum=0;
				ahsStaticSetTarFilename(file_hdr, en->from_name_);
				ahsStaticSetPaxLinkname(file_hdr, NULL);

				switch(en->source_code_){
					case SWACFL_SRCCODE_RPMHEADER:
						E_DEBUG("in SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_RPMHEADER");
						topsf->user_addr=p;
						file_hdr->c_filesize=rpmpsf_get_rpmtag_length(topsf, p) + 1;
						do_write_file=rpmpsf_write_out_tag;
						E_DEBUG("leaving SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_RPMHEADER");
						break;
					case SWACFL_SRCCODE_PSF:
						E_DEBUG("in SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_PSF");
						file_hdr->c_filesize=rpmpsf_get_psf_size(topsf);
						do_write_file=rpmpsf_write_beautify_psf;
						E_DEBUG("leaving SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_PSF");
						break;	
					default:
						SWBIS_E_FAIL("fatal");
						exit(1);
				}
				pipe(pheader);
				pid = swfork((sigset_t*)(NULL));
				if (pid < 0){
					SWBIS_E_FAIL("fatal, fork error");
					_exit(1);
				} else if (pid == 0) {
					close(pheader[0]);
					(*do_write_file)(topsf, pheader[1]);
					if (en->source_code_ == SWACFL_SRCCODE_RPMHEADER) {
						uxfio_unix_safe_write(pheader[1], "\n", 1);
					}
					_exit(0);
				}
				close(pheader[1]);
				
				xformat_write_by_fd(xxformat, pheader[0], file_hdr);
				E_DEBUG3("OK %s %s", p, en->from_name_);
				E_DEBUG("leaving SWACFL_SRCCODE_PSF");
				break;
			
			case SWACFL_SRCCODE_FILESYSTEM:
				SWBIS_E_FAIL("bad case error");
				break;
		}
	}
	ahsStaticDeleteFilehdr(file_hdr);
	E_DEBUG("FUNCTION EXIT");
	return 0;
}


int
topsf_copypass_swacfl_archive_list(TOPSF * topsf, XFORMAT * xxformat, SWVARFS * swvarfs, int ofd)
{
	int u_fd;
	int pheader[2];
	char * p;
	char * savep = NULL;
	pid_t pid;
	struct new_cpio_header *file_hdr=ahsStaticCreateFilehdr();
	swacfl_entry * en;
	SWACFL *swacfl=topsf->swacfl_;
	struct stat st;
	int (*do_write_file)(TOPSF * topsf, int o_fd);
	int archive_name_index = 0;
	char * archive_name;
	STROB * tmp;

	tmp = strob_open(100);

	while ((archive_name = strar_get(topsf->archive_namesM, archive_name_index++))) {
		E_DEBUG2("archive_name=[%s]", archive_name ? archive_name : "<NULL>");
		if (topsf->rpmtag_default_prefixM) {
			strob_strcpy(tmp, topsf->rpmtag_default_prefixM);
			swlib_add_trailing_slash(tmp);
			strob_strcat(tmp, archive_name);
			E_DEBUG2("will FIND ENTY prefix: tmp=[%s]", strob_str(tmp));
			en = swacfl_find_entry(swacfl, strob_str(tmp));
		} else {
			E_DEBUG2("will FIND ENTY no prefix: archive_name=[%s]", archive_name);
			en = swacfl_find_entry(swacfl, archive_name);
		}
		if (!en) {
			fprintf(stderr, "lxpsf: Internal fatal error, swacfl entry not found for [%s]\n", archive_name);
			exit(1);
		}
		if (savep) free(savep);

		if (topsf->rpmtag_default_prefixM) {
			savep = swlib_strdup(archive_name);
		} else {
			savep = swlib_strdup(strob_str(en->archiveNameM));
		}
		E_DEBUG2("WILL OPEN: savep = [%s]", savep);
		E_DEBUG2("en->from_name_ = [%s]", en->from_name_);

		p = savep;
		swlib_process_hex_escapes(p);
		swlib_unexpand_escapes(NULL, p);
		if (*p == '/') p++;
		switch(en->source_code_){
			case SWACFL_SRCCODE_ARCHIVE_STREAM:	
				E_DEBUG("in SWACFL_SRCCODE_ARCHIVE_STREAM");
				E_DEBUG("SWACFL_SRCCODE_ARCHIVE_STREAM #1a");
				u_fd=swvarfs_u_open(swvarfs, p);
				E_DEBUG("SWACFL_SRCCODE_ARCHIVE_STREAM #1b");
				if (u_fd < 0) {	
					E_DEBUG("SWACFL_SRCCODE_ARCHIVE_STREAM #1c");
					fprintf(stderr,"swvarfs_u_open error: file %s not found\n", p);
					break;
				}
				E_DEBUG("SWACFL_SRCCODE_ARCHIVE_STREAM #1");
				swvarfs_u_fstat(swvarfs, u_fd, &st);	
				E_DEBUG("SWACFL_SRCCODE_ARCHIVE_STREAM #2");


				if (topsf->rpmtag_default_prefixM) {
					taru_statbuf2filehdr(file_hdr, &st, (char*)NULL, en->from_name_,
						swvarfs_u_get_linkname(swvarfs, u_fd));
				} else {
					taru_statbuf2filehdr(file_hdr, &st, (char*)NULL, p /*en->from_name_*/,
						swvarfs_u_get_linkname(swvarfs, u_fd));
				}

				E_DEBUG("SWACFL_SRCCODE_ARCHIVE_STREAM #3");
				file_hdr->c_uid=getuid();
				file_hdr->c_gid=getgid();
				xformat_write_by_fd(xxformat, u_fd, file_hdr);
				E_DEBUG3("OK %s %s", p, en->from_name_);
				ahsStaticSetTarFilename(file_hdr, NULL);
				ahsStaticSetPaxLinkname(file_hdr, NULL);
				swvarfs_u_close(swvarfs, u_fd);
				if (topsf_check_header_list(topsf, topsf->header_namesM, archive_name)) {
					/*
					 * This is an error
					 * Every file in the archive should be found in the Header,
					 * The end result is the file_status_array will indicate the files
					 * that are in the Header but not in the archive.
					 */
					fprintf(stderr, "%s: topsf_check_header_list() indicates not found: [%s]\n", swlib_utilname_get(), archive_name);
				}
				E_DEBUG("leaving SWACFL_SRCCODE_ARCHIVE_STREAM");
				break;

			case SWACFL_SRCCODE_RPMHEADER:
			case SWACFL_SRCCODE_PSF:

				E_DEBUG("in SWACFL_SRCCODE_PSF");
				file_hdr->c_magic=CPIO_NEWASCII_MAGIC;	
				file_hdr->c_ino=1;
				file_hdr->c_mode = (0644);
				file_hdr->c_mode |= CP_IFREG;
				file_hdr->c_uid=getuid();
				file_hdr->c_gid=getgid();
				file_hdr->c_nlink=1;
				ahsStaticSetPaxLinkname(file_hdr, NULL);
				file_hdr->c_mtime=topsf->mtimeM;
				file_hdr->c_filesize=0;
				file_hdr->c_namesize=strlen(en->from_name_) +1;
				file_hdr->c_chksum=0;
				ahsStaticSetTarFilename(file_hdr, en->from_name_);
				ahsStaticSetPaxLinkname(file_hdr, NULL);

				switch(en->source_code_){
					case SWACFL_SRCCODE_RPMHEADER:
						E_DEBUG("in SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_RPMHEADER");
						topsf->user_addr=p;
						file_hdr->c_filesize = rpmpsf_get_rpmtag_length(topsf, p);
						do_write_file = rpmpsf_write_out_tag;
						E_DEBUG("leaving SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_RPMHEADER");
						break;
					case SWACFL_SRCCODE_PSF:
						E_DEBUG("in SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_PSF");
						file_hdr->c_filesize=rpmpsf_get_psf_size(topsf);
						do_write_file=rpmpsf_write_beautify_psf;
						E_DEBUG("leaving SWACFL_SRCCODE_PSF: SWACFL_SRCCODE_PSF");
						break;	
					default:
						SWBIS_E_FAIL("fatal");
						exit(1);
				}
				pipe(pheader);
				pid = swfork((sigset_t*)(NULL));
				if (pid < 0){
					SWBIS_E_FAIL("fatal, fork error");
					_exit(1);
				} else if (pid == 0) {
					close(pheader[0]);
					(*do_write_file)(topsf, pheader[1]);
					_exit(0);
				}
				close(pheader[1]);
				
				xformat_write_by_fd(xxformat, pheader[0], file_hdr);
				E_DEBUG3("OK %s %s", p, en->from_name_);
				E_DEBUG("leaving SWACFL_SRCCODE_PSF");
				break;
			
			case SWACFL_SRCCODE_FILESYSTEM:
				SWBIS_E_FAIL("bad case error");
				break;
		}
	}

	E_DEBUG2("status_array [%s]", strob_str(topsf->file_status_arrayM));

	if (savep) free(savep);
	ahsStaticDeleteFilehdr(file_hdr);
	E_DEBUG("FUNCTION EXIT");
	return 0;
}

int
topsf_copypass_swacfl_list(TOPSF * topsf, int ofd)
{
	int ret;
	SWVARFS * swvarfs;
	XFORMAT * xxformat;
	xxformat = xformat_open(-1, ofd, (int)(arf_newascii));
	swvarfs=swvarfs_opendup(topsf->fd_, 
		UINFILE_DETECT_FORCEUXFIOFD | /*UINFILE_DETECT_NATIVE|*/ UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, (mode_t)(NULL));
	if (!swvarfs) {
		return -1;
	}
	E_DEBUG("running topsf_copypass_swacfl_header_list");
	ret = topsf_copypass_swacfl_header_list(topsf, xxformat, swvarfs, ofd);
	if (ret != 0) {
		fprintf(stderr, "topsf_copypass_swacfl_list: error writing header files\n");
		return ret;
	}
	E_DEBUG("running topsf_copypass_swacfl_archive_list");
	ret = topsf_copypass_swacfl_archive_list(topsf, xxformat, swvarfs, ofd);

	/*
	 * Now write the files that are in the header but not in the cpio archive
	 */

	if (topsf->rpmtag_default_prefixM != NULL) {
		/* with these rare packages that use this deprecated tag, do
		 * not construct missing file
		 */
		;
	} else {
		topsf_copypass_swacfl_write_missing_files(topsf, xxformat, swvarfs, ofd);
	}

	swvarfs_close(swvarfs);
 	xformat_write_trailer(xxformat); 
	xformat_close(xxformat);
	return ret;
}

int
topsf_get_fd_fd(TOPSF * topsf) {
	int ret;
	E_DEBUG("FUNCTION ENTER");
	switch (uinfile_get_type(topsf->format_desc_)) {
		case RPMRHS_FILEFORMAT:
			ret= rpmfd_get_fd_fd(topsf->rpmfd_);
			E_DEBUG("FUNCTION EXIT");
			return ret;
		default:
			E_DEBUG("FUNCTION EXIT");
			return topsf->fd_;
			break;
	}
}

char *
topsf_make_package_prefix(TOPSF * topsf, char * cwdir)
{
	char * ret;
	
	E_DEBUG("FUNCTION ENTER");
	switch (uinfile_get_type(topsf->format_desc_)) {
		case RPMRHS_FILEFORMAT:
			ret = rpmpsf_make_package_prefix(topsf, cwdir);
			E_DEBUG("FUNCTION EXIT");
			return ret;
			break;
		case PLAIN_TARBALL_SRC_FILEFORMAT:
			break;
		case SLACK_FILEFORMAT:
			break;
		case DEB_FILEFORMAT:
			ret = strdup("");
			return ret;
			break;
		default:
			fprintf(stderr, "unrecognized package type.\n");
			break;
	}
	E_DEBUG("FUNCTION EXIT");
	return (char*)(NULL);
}

int
topsf_write_psf(TOPSF * topsf, int fd_out, int do_indent)
{
	int ret;

	E_DEBUG("FUNCTION ENTER");
	switch (uinfile_get_type(topsf->format_desc_)) {
		case RPMRHS_FILEFORMAT:
			E_DEBUG("");
			if(do_indent) {
				ret = rpmpsf_write_beautify_psf(topsf, fd_out);
				E_DEBUG("FUNCTION EXIT");
				return ret;
			} else {
				ret = rpmpsf_write_psf(topsf, fd_out);
				E_DEBUG("FUNCTION EXIT");
				return ret;
			}
			break;
		case DEB_FILEFORMAT:
			E_DEBUG("");
			ret = debpsf_write_psf((void *)topsf, fd_out);
			E_DEBUG("");
			return ret;
			break;
		case SLACK_FILEFORMAT:
			E_DEBUG("");
			ret = slack_write_psf(topsf, fd_out);
			return ret;
			break;
		case PLAIN_TARBALL_SRC_FILEFORMAT:
			ret = plain_source_tarball_psf(topsf, fd_out);
			return ret;
			break;
		default:
			E_DEBUG("");
			fprintf(stderr, "unrecognized package type.\n");
			break;
	}
	E_DEBUG("FUNCTION EXIT");
	return -1;
}

int
topsf_write_info(TOPSF * topsf, int fd_out, int do_indent)
{
	int ret;

	E_DEBUG("FUNCTION ENTER");
	switch (uinfile_get_type(topsf->format_desc_)) {
		case RPMRHS_FILEFORMAT:
			if(do_indent) {
				ret = rpmpsf_write_beautify_info(topsf, fd_out);
				E_DEBUG("FUNCTION EXIT");
				return ret;
			} else {
				ret = rpmpsf_write_info(topsf, fd_out);
				E_DEBUG("FUNCTION EXIT");
				return ret;
			}
			break;
		default:
			fprintf(stderr, "unrecognized package type.\n");
			break;
	}
	E_DEBUG("FUNCTION EXIT");
	return -1;
}

int
topsf_h_write_to_buf(XFORMAT * xformat, char * name, STROB ** buf)
{
	int ufd;
	int ret;
	if (buf) *buf = strob_open(32);

	ufd = xformat_u_open_file(xformat, name);
	if (ufd < 0) return -1;
	if (buf) {
		ret = swlib_ascii_text_fd_to_buf(*buf, ufd);
	} else {
		/* throw this file away */
		STROB * tmp = strob_open(32);
		ret = swlib_ascii_text_fd_to_buf(tmp, ufd);
		strob_close(tmp);
		ret = 0;
	}
	xformat_u_close_file(xformat, ufd);
	return ret;
}
	
int
topsf_search_list(TOPSF * topsf, STRAR * list, char * archive_name)
{
	char *s1;
	int ret;
	int i = 0;

	/* Find archive_name in list
	 * and check the topsf->file_status_arrayM
	 */

	while ((s1=strar_get(list, i)) != NULL) {
		E_DEBUG3("comparing [%s] [%s]", s1, archive_name);

		if (
			(!strcmp(archive_name, "/") && !strcmp(s1, "./")) ||
			(!strcmp(archive_name, "./") && !strcmp(s1, "/"))
		) {
			E_DEBUG3("found special case [%s] [%s]", s1, archive_name);
			return i;
		} else {
			ret = swlib_dir_compare(s1, archive_name, SWC_FC_NOAB);
			if (ret == 0) {
				/* Found */
				return i;
			}
		}
		i++;
	}
	return -1;
}

int
topsf_check_header_list(TOPSF * topsf, STRAR * list, char * archive_name)
{
	char *s1;
	int ret;
	int i = 0;
	

	/* Find archive_name in list
	 * and check the topsf->file_status_arrayM
	 */

	if (topsf->rpmtag_default_prefixM) {
		/*
		 * Must prepend the prefix to archive_name
		 */
		E_DEBUG2("archive name [%s]", archive_name);
		strob_strcpy(topsf->usebuf1M, topsf->rpmtag_default_prefixM);
		swlib_unix_dircat(topsf->usebuf1M, archive_name);
		E_DEBUG2("HAS prefix, searching for [%s]", strob_str(topsf->usebuf1M));
		ret = topsf_search_list(topsf, list, strob_str(topsf->usebuf1M));
	} else {
		E_DEBUG2("searching for [%s]", archive_name);
		ret = topsf_search_list(topsf, list, archive_name);
	}
	if (ret >= 0) {
		strob_chr_index(topsf->file_status_arrayM,  ret, TOPSF_FILE_STATUS_IN_ARCHIVE);
		return 0;
	} else {
		return 1;
	}
}
