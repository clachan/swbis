/* topsf.h - Open an alien format and translate to a PSF with the archive.

   Copyright (C) 1999,2007  Jim Lowe 

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
         

#ifndef TOPSF_19990131_H
#define TOPSF_19990131_H

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "rpmfd.h"
#include "cplob.h"
#include "strob.h"
#include "taru.h"
#include "swacfl.h"
#include "uinfile.h"
#include "uxfio.h"
#include "swlib.h"
#include "um_header.h"

#define TOPSF_OPEN_NO_AUDIT	(1 << 17) 
#define TOPSF_PSF_FORM2 	2
#define TOPSF_PSF_FORM3 	3

#define TOPSF_FILE_STATUS_UNSET		48 /* ascii 0 */
#define TOPSF_FILE_STATUS_IN_HEADER	49 /* ascii 1 */
#define TOPSF_FILE_STATUS_IN_ARCHIVE	50 /* ascii 2 */

typedef struct {
	char * prefix_;
	char * cwd_prefix_;
	SWACFL * swacfl_;
	CPLOB  * hl_node_names_;
	CPLOB  * hl_linkto_names_;
	Header h_;
	UINFORMAT * format_desc_;
	char * user_addr;
	RPMFD * rpmfd_;	/* The RPM descriptor switch. */
	int fd_;	/* depricated */
	int use_recursive_; /* Use "file *" to specify the fileset of a binary rpm */
	int reverse_links_; /* Interpret hard links in reverse when converting to tar */
	STRAR * archive_namesM;  /* List of files in the rpm cpio archive */
	STROB * file_status_arrayM; /* char Array: Status of appearance in cpio archive compared */
				    /* compared to Header;  Index of array is analog to (RPM)Header index */
				    /* 0=unset, 1 appear in one, 2 appears in both */
	STRAR * header_namesM;    /* List of files from the Header */
	int debug_link_;
	int single_fileset_;
	int info_only_;
	int smart_path_;
	int form_;
	TARU * taruM;	
	STROB * control_directoryM;	
	time_t mtimeM;
	void * debpsfM;
	void * rpmpsfM; /* does not exist */
	char * pkgfilenameM;
	char * checkdigestnameM;
	char * ownerM;
	char * groupM;
	STRAR * exclude_listM;
	int rpm_construct_missing_filesM;  /* User option */
	char * rpmtag_default_prefixM;  /* RPMTAG_DEFAULTPREFIX, use is very rare, */
				        /* and deprecated but support it anyway */
	STROB * usebuf1M;               /* pre-allocated buffer */
					/* Set by the -v option of lxpsf program */
	int verboseM;
} TOPSF;

TOPSF * topsf_open(char * filename, int oflags, char * package_name);
void topsf_set_taru(TOPSF * topsf, TARU * taru);
void topsf_set_mtime(TOPSF * topsf, time_t tm);
void 	topsf_close(TOPSF * topsf);
int 	topsf_get_fd(TOPSF * topsf);
void 	topsf_set_fd(TOPSF * topsf, int fd);
UINFORMAT * topsf_get_format_desc(TOPSF * topsf);
Header 	topsf_get_rpmheader(TOPSF * topsf);
SWACFL * topsf_get_archive_filelist(TOPSF * topsf);
void	topsf_add_fl_entry(TOPSF * topsf, char * to_name, char * from_name, int source_code);
char * 	topsf_get_psf_prefix(TOPSF * topsf);
void 	topsf_set_psf_prefix(TOPSF * topsf, char * p);
char * 	topsf_get_cwd_prefix(TOPSF * topsf);
void 	topsf_set_cwd_prefix(TOPSF * topsf, char * p);
int     topsf_rpm_audit_hard_links(TOPSF * topsf, int fd);
int	topsf_copypass_swacfl_list (TOPSF * topsf, int output_fd);
FD_t	topsf_rpmFD(TOPSF * topsf);
int 	topsf_get_fd_fd(TOPSF * topsf);
char *  topsf_make_package_prefix(TOPSF * topsf, char * cwdir);
int     topsf_write_psf(TOPSF * topsf, int fd_out, int do_indent);
int 	topsf_write_info(TOPSF * topsf, int fd_out, int do_indent);
int	topsf_h_write_to_buf(XFORMAT * xformat, char * name, STROB ** buf);
int topsf_parse_slack_pkg_name(char * pkgfilename, STRAR * st);
void topsf_add_excludes(TOPSF * topsf, STROB * buf);
int topsf_check_header_list(TOPSF * topsf, STRAR * list, char * archive_name);
int topsf_search_list(TOPSF * topsf, STRAR * list, char * archive_name);

#endif
