/* debpsf.c - Routines for deb-to-PSF file translation.

   Copyright (C) 2007 Jim Lowe
 
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
#include "swfork.h"
#include "swlib.h"
#include "uxfio.h"
#include "strob.h"
#include "swheaderline.h"
#include "swintl.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swutilname.h"
#include "swgp.h"
#include "debpsf.h"
#include "topsf.h"


static
void
normalize_leading_directory(STROB * b, char * name)
{
	strob_strcpy(b, ".");
	swlib_unix_dircat(b, name);
}

static
void
squash_illegal_tag_chars(char * s)
{
	while(*s != (int)'\0') {
		if (*s == '.') *s = '_';
		if (*s == ',') *s = '_';
		if (*s == ':') *s = '_';
		s++;
	}
}

static
int
parse_version(DEB_ATTRIBUTES * da)
{
	char * s0;
	char * s;
	char * e;
	char * r;
	if (da->Version == NULL) {
		fprintf(stderr, "Version attribute missing\n");
		return -1;
	}
	s0 = strdup(strob_str(da->Version));
	s = s0;
	e = strchr(s, ':');
	if (e) {
		/* epoch */
		*e = '\0';
		da->Version_epoch = strob_open(32);	
		strob_strcpy(da->Version_epoch, s);
		s = e + 1;
	}
	r = strrchr(s, '-');
	if (r) {
		da->Version_release = strob_open(32);	
		*r = '\0';	
		r++;
		strob_strcpy(da->Version_release, r);
	}
	da->Version_revision = strob_open(32);	
	strob_strcpy(da->Version_revision, s);
	free(s0);
	return 0;
}

static
int
x_strncasecmp(char * line, char * s) {
	return strncasecmp(line, s, strlen(s));
}
				
static
int
process_line(STROB ** attr, char * line, int do_append)
{
	char * s;
	char * p;
	char * w;

	if (do_append == 0) {
		s = strchr(line, ':');
		if (!s) {
			return -1;	
		}
		*attr = strob_open(32);
		s++;
		while(isspace(*s)) s++;
	
		p = line;
		w = NULL;
		while(*p != '\0') {
			if (*p && isspace(*p)) {
				w = p;
			} else if (*p) {
				w = NULL;
			}
			p++;
		}	
		strob_strncpy(*attr, s, w ? (int)(w - s) : (int)strlen(s)); 
		return 0;
	} else {
		p = line;
		s = line;
		while(isspace(*s)) s++;
		p = s;
		w = NULL;
		while(*p != '\0') {
			if (*p && isspace(*p)) {
				w = p;
			} else if (*p) {
				w = NULL;
			}
			p++;
		}	

		if (*s == '\0') return -1;
		if (*s == '.' && s+1 == w) {
			strob_sprintf(*attr, 1, "\n");	
		} else {
			strob_strcat(*attr, "\n");
			strob_strncat(*attr, s, w ? (int)(w - s) : (int)strlen(s)); 
		}
		return 0;
	}
	return 0;
}

static
int
parse_control_file(DEB_ATTRIBUTES * da, char * filebuf)
{
	int n;
	int fd;
	STROB * tmp;
	STROB * last_attribute;
	char * line;
	
	tmp = strob_open(32);
	fd = swlib_open_memfd();
	uxfio_write(fd, filebuf, strlen(filebuf));
	uxfio_lseek(fd, 0, SEEK_SET);

	last_attribute = NULL;
	while ((n=swgp_read_line(fd, (STROB*)tmp, DO_NOT_APPEND)) > 0) {
		line = strob_str(tmp);
		if (isalpha(*line)) {
			/* New attribute */
			if (x_strncasecmp(line, DEBPSF_ATTR_Package ":") == 0) {
				process_line(&((*da).Package), line, 0 /*append*/);
				last_attribute = da->Package;
				;
			} else if (x_strncasecmp(line, DEBPSF_ATTR_Version ":") == 0) {
				process_line(&(da->Version), line, 0 /*append*/);
				last_attribute = da->Version;
				;
			} else if (x_strncasecmp(line, DEBPSF_ATTR_Architecture ":") == 0) {
				process_line(&(da->Architecture), line, 0 /*append*/);
				last_attribute = da->Architecture;
				;
			} else if (x_strncasecmp(line, DEBPSF_ATTR_Maintainer ":") == 0) {
				process_line(&(da->Maintainer), line, 0 /*append*/);
				last_attribute = da->Maintainer;
				;	
			} else if (x_strncasecmp(line, DEBPSF_ATTR_Description ":") == 0) {
				process_line(&(da->Description), line, 0 /*append*/);
				last_attribute = da->Description;
				;
			} else {
				fprintf(stderr, "%s: debpsf: attribute not supported at this time: %s",
					swlib_utilname_get(), line);
			}

		} else if (isspace(*line)) {
			/* Multi line attributes */
			if (last_attribute == NULL) {
				return -1;
			}
			process_line(&(last_attribute), line, 1 /*append*/);
		} else {
			;
			/* error */
		}
	}
	strob_close(tmp);
	uxfio_close(fd);
	return 0;
}

static
int
write_to_buf(XFORMAT * xformat, char * name, STROB ** buf)
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

DEBPSF *
debpsf_create(void)
{
	DEBPSF * debpsf;
	debpsf = malloc(sizeof(DEBPSF));
	debpsf->headM = NULL;
	debpsf->topsfM = NULL;
	debpsf->header_fdM = -1;
	debpsf->control_fdM = -1;
	debpsf->data_fdM = -1;
	return debpsf;
}

int
debpsf_open(DEBPSF * dp, void * v_topsf)
{
	int ret;
	int fd;
	TOPSF * topsf = (TOPSF*)v_topsf;

	dp->topsfM = topsf;

	if (uinfile_get_type(topsf->format_desc_) != DEB_FILEFORMAT) {
		return -1;
	}
	fd = swlib_open_memfd();
	if (fd < 0) return -2;

	/* store the control.tar.gz archive member in fd */	
	ret = swlib_pipe_pump(fd, topsf->fd_);
	if (ret < 0) return -3;

	dp->control_fdM = fd;	
	uxfio_lseek(fd, 0, SEEK_SET);

	/* Now go after the data.tar.gz archive member */
	ret = uinfile_opendup(topsf->format_desc_->deb_file_fd_, 0,
			&(topsf->format_desc_), UINFILE_DETECT_DEB_DATA);
	if (ret < 0) return -4;

	/* the data is now in data_fdM */
	dp->data_fdM = swlib_open_memfd();
	dp->source_data_fdM = ret;

	return 0;
}

void
debpsf_deb_attributes_init(DEB_ATTRIBUTES * da)
{
	da->Package = NULL;
	da->Version = NULL;
	da->Version_epoch = NULL;
	da->Version_revision = NULL; /* upstream version */
	da->Version_release = NULL;  /* Debian revision */
	da->Architecture = NULL;
	da->Maintainer = NULL;
	da->Description = NULL;
}

int
debpsf_close(DEBPSF * dp)
{
	swlib_pipe_pump(dp->data_fdM, dp->source_data_fdM);
	uxfio_close(dp->control_fdM);
	uxfio_close(dp->data_fdM);
	dp->control_fdM = -1;
	dp->data_fdM = -1;
	return 0;
}

int
debpsf_write_psf(void * v_topsf, int ofd)
{
	int ret;
	STROB * psf;
	psf = strob_open(32);
	ret = debpsf_write_psf_buf(v_topsf, psf);
	if (ret < 0) return ret;
	ret = uxfio_write(ofd, psf->str_, strlen((char*)(psf->str_)));
	strob_close(psf);
	return ret;
}

int
debpsf_write_psf_buf(void * v_topsf, STROB * psf)
{
	int ret;
	int control_fd;
	int data_fd;
	char * name;
	TOPSF * topsf;
	DEBPSF *dp;
	XFORMAT * control_xformat;
	XFORMAT * data_xformat;
	STROB * tmp;
	STROB * tmp2;
	STROB * namebuf;
	STROB * control_buf;
	STROB * md5sums_buf;
	STROB * postinstall_buf;
	STROB * preinstall_buf;
	STROB * preremove_buf;
	STROB * postremove_buf;
	STROB * conffiles_buf;
	STROB * shlib_buf;
	char * conffile_line;
	struct new_cpio_header * h;
	AHS * ahs;
	struct stat st;
	DEB_ATTRIBUTES * da;

	namebuf = strob_open(32);
	tmp = strob_open(12);
	tmp2 = strob_open(12);
	topsf = (TOPSF*)v_topsf;
	dp = topsf->debpsfM;
	dp->daM = malloc(sizeof(DEB_ATTRIBUTES));
	debpsf_deb_attributes_init(dp->daM);
	da = dp->daM;

	control_buf = NULL;
	md5sums_buf = NULL;
	postinstall_buf = NULL;
	preinstall_buf = NULL;
	preremove_buf = NULL;
	postremove_buf = NULL;
	conffiles_buf = NULL;
	shlib_buf = NULL;

	/* Read the control.tar file */
	control_fd = dp->control_fdM; 

	/* Loop over the files in control.tar.gz */
	E_DEBUG("");
	uxfio_lseek(control_fd, 0, SEEK_SET);

	control_xformat = xformat_open(-1, -1, arf_ustar);
	ret = xformat_open_archive_by_fd(control_xformat, control_fd,
			UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_DEB_CONTROL, (mode_t)(0));
	if (ret < 0) {
		return -2;
	}
	E_DEBUG("");

	while ((name=xformat_get_next_dirent(control_xformat, &st)) != NULL) {
		E_DEBUG("");

		if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_CONTROL) == 0) {
			E_DEBUG(DEBPSF_FILE_CONTROL);
			write_to_buf(control_xformat, name, &control_buf);
			parse_control_file(da, strob_str(control_buf));
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_MD5SUM) == 0) {
			E_DEBUG(DEBPSF_FILE_MD5SUM);
			write_to_buf(control_xformat, name, &md5sums_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_POSTINSTALL) == 0) {
			E_DEBUG(DEBPSF_FILE_POSTINSTALL);
			write_to_buf(control_xformat, name, &postinstall_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_PREINSTALL) == 0) {
			E_DEBUG(DEBPSF_FILE_PREINSTALL);
			write_to_buf(control_xformat, name, &preinstall_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_PREREMOVE) == 0) {
			E_DEBUG(DEBPSF_FILE_PREREMOVE);
			write_to_buf(control_xformat, name, &preremove_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_POSTREMOVE) == 0) {
			E_DEBUG(DEBPSF_FILE_POSTREMOVE);
			write_to_buf(control_xformat, name, &postremove_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_CONFFILES) == 0) {
			E_DEBUG(DEBPSF_FILE_CONFFILES);
			write_to_buf(control_xformat, name, &conffiles_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX DEBPSF_FILE_SHLIBS) == 0) {
			E_DEBUG(DEBPSF_FILE_SHLIBS);
			write_to_buf(control_xformat, name, &shlib_buf);
			;
		} else if (strcmp(name, DEBPSF_CONTROL_PREFIX) == 0) {
			E_DEBUG("");
			;
		} else {
			E_DEBUG("");
			/* read the file, but don't save it */
			write_to_buf(control_xformat, name, NULL);
			fprintf(stderr, "%s: unrecognized file in control.tar.gz: %s\n", swlib_utilname_get(), name);
		}
		E_DEBUG("");
	}
	E_DEBUG("");
	xformat_close(control_xformat);
	parse_version(da);

	E_DEBUG("");
	if (1 && 0 /*disabled*/) { /* debug and development code */
		fprintf(stderr, "dp->daM->Package: %s", dp->daM->Package ? strob_str(dp->daM->Package): "(null)");
		fprintf(stderr, "%s", control_buf ? strob_str(control_buf): "(null)" );
		fprintf(stderr, "da->Description [%s]\n", da->Description ? strob_str(da->Description) : "(null)");
		fprintf(stderr, "da->Version: Version: [%s]\n", da->Version ? strob_str(da->Version) : "(null)");
		fprintf(stderr, "da->Version_revision: Upstream revision: [%s]\n", da->Version_revision ? strob_str(da->Version_revision) : "(null)");
		fprintf(stderr, "da->Version_release: Debian revision: [%s]\n", da->Version_release? strob_str(da->Version_release):"(null)");
	}

	/* OK, the control file is now parsed, and other control files have been
	   audited and saved for replay.  Now its time to start writing the PSF */
	   
	strob_sprintf(psf, STROB_NO_APPEND, "");
	strob_sprintf(psf, STROB_DO_APPEND, "distribution\n");
	strob_sprintf(psf, STROB_DO_APPEND, "layout_version 1.0\n");
	strob_sprintf(psf, STROB_DO_APPEND, "dfiles dfiles\n");
	strob_sprintf(psf, STROB_DO_APPEND, "pfiles pfiles\n");
	
			/* FIXME fix illegal chars in Package */
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " \"%s-%s%s%s\"\n",
			strob_str(dp->daM->Package),
			strob_str(da->Version_revision),
			(da->Version_release && strlen(strob_str(da->Version_release))) ? "-" : "",
			da->Version_release ? strob_str(da->Version_release):""
			);

	if (md5sums_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, "md5sums <" DEBPSF_PSF_CONTROL_DIR "/md5sums\n");
	}

	if (conffiles_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, "conffiles <" DEBPSF_PSF_CONTROL_DIR "/conffiles\n");
	}

	if (shlib_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, "shlibs <" DEBPSF_PSF_CONTROL_DIR "/shlibs\n");
	}

	/* End of distribution object */

	if (dp->daM->Version_release) {
		strob_sprintf(psf, STROB_DO_APPEND, "\n");
		strob_sprintf(psf, STROB_DO_APPEND, "vendor\n");
		strob_sprintf(psf, STROB_DO_APPEND, "the_term_vendor_is_misleading True\n");
		squash_illegal_tag_chars(strob_str(dp->daM->Version_release));
		strob_sprintf(psf, STROB_DO_APPEND, "tag \"%s\"\n", strob_str(dp->daM->Version_release));
		strob_sprintf(psf, STROB_DO_APPEND, "title Debian\n");
		strob_sprintf(psf, STROB_DO_APPEND, "url \"http://www.debian.org\"\n");
	}

	/* Start of product object */

	squash_illegal_tag_chars(strob_str(dp->daM->Package));
	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_product "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " %s\n", strob_str(dp->daM->Package));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_revision " %s\n", strob_str(dp->daM->Version_revision));
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " \"\"\n");
	if (dp->daM->Version_release) {
		strob_sprintf(psf, STROB_DO_APPEND, "vendor_tag \"%s\"\n", strob_str(dp->daM->Version_release));
	}

	/* write the control_file extended definitions */
	if (control_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_file " " DEBPSF_PSF_CONTROL_DIR "/" DEBPSF_FILE_CONTROL " control\n");
	}
	if (preinstall_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_preinstall " " DEBPSF_PSF_CONTROL_DIR "/" DEBPSF_FILE_PREINSTALL "\n");
	}
	if (postinstall_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_postinstall " " DEBPSF_PSF_CONTROL_DIR "/" DEBPSF_FILE_POSTINSTALL "\n");
	}
	if (preremove_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_preremove " " DEBPSF_PSF_CONTROL_DIR "/" DEBPSF_FILE_PREREMOVE "\n");
	}
	if (postremove_buf) {
		strob_sprintf(psf, STROB_DO_APPEND, SW_A_postremove " " DEBPSF_PSF_CONTROL_DIR "/" DEBPSF_FILE_POSTREMOVE "\n");
	}

	/* Start of fileset object */
	strob_sprintf(psf, STROB_DO_APPEND, "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_fileset "\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_tag " bin\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_control_directory " \"\"\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_directory " .\n");

	E_DEBUG("");

	/* Now read the first 512 bytes of data, this should be the leading
	   directory "./".  Get the permissions so this can be set in the PSF
	   explicitly */

	/* Open the data archive */

	data_xformat = xformat_open(-1, -1, arf_ustar);
	ret = xformat_open_archive_by_fd(data_xformat, dp->source_data_fdM,
			UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_DEB_CONTEXT, (mode_t)(0));
	if (ret < 0) {
		ret = -2; fprintf(stderr, "%s: debpsf error [%d]\n", swlib_utilname_get(), ret);
		return ret;
	}
	
	ret = xformat_read_header(data_xformat);
	if (xformat_is_end_of_archive(data_xformat)) {
		ret = -5; fprintf(stderr, "%s: empty data archive [%d]\n", swlib_utilname_get(), ret);
		return ret;
	}

	if (xformat_get_tar_typeflag(data_xformat) != DIRTYPE) {
		ret = -6; fprintf(stderr, "%s: first file not a directory  [%d]\n", swlib_utilname_get(), ret);
		return ret;
	}

	ahs = xformat_ahs_object(data_xformat);
	name = ahsStaticGetTarFilename(ahs->file_hdrM);

	/* name had better be "./" */
	/* FIXME enforce name == "./" ?? */

	/* Use the stats in 'ahs' to write an explicit definition for the
	   "./" archive member */	
	strob_sprintf(psf, STROB_DO_APPEND, "# Note Using file -t d -o XX -g XX ./\n");
	strob_sprintf(psf, STROB_DO_APPEND, "# instead of file -t d -o XX -g XX /\n");
	strob_sprintf(psf, STROB_DO_APPEND, "# will be a broken package, this is a bug in swinstall\n");
	strob_sprintf(psf, STROB_DO_APPEND, " " SW_A_file " -t d -o %s -g %s -m %o /\n",
		ahsStaticGetTarUsername(ahs->file_hdrM),
		ahsStaticGetTarGroupname(ahs->file_hdrM),
		(unsigned int)(ahs_get_perms(ahs))
		);
	
	/* Recursive spec after the leading ./ */
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_file " *\n");

	/* Now set the is_volatile attributes for the conf files */
	if (conffiles_buf) {
		conffile_line = strob_strtok(tmp2, strob_str(conffiles_buf), "\n\r");
		while(conffile_line) {
			/* fprintf(stderr, "CONFFILE=[%s]\n", conffile_line); */
			swlib_is_sh_tainted_string_fatal(conffile_line);
			normalize_leading_directory(tmp, conffile_line);
			strob_sprintf(psf, STROB_DO_APPEND,  SW_A_file "\n");
			strob_sprintf(psf, STROB_DO_APPEND,  SW_A_source " %s\n", strob_str(tmp));
			strob_sprintf(psf, STROB_DO_APPEND,  SW_A_is_volatile " true\n");
			strob_sprintf(psf, STROB_DO_APPEND, "\n");
			conffile_line = strob_strtok(tmp2, NULL, "\n\r");
		}
	}
	
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " control\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " control/*\n");
	strob_sprintf(psf, STROB_DO_APPEND, SW_A_exclude " PSF\n");

	topsf_add_excludes(topsf, psf);

	xformat_close(data_xformat);
	strob_close(tmp);
	strob_close(tmp2);
	strob_close(namebuf);
	return ret;
}
