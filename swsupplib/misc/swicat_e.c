/* swicat_e.c -- catalog entry parsing routines

 Copyright (C) 2007 Jim Lowe
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

/*
This file contains routines to parse a tarball blob that looks something
like this:

var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/vendor_tag
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/export/
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/export/catalog.tar
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/export/catalog.tar.sig
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/session_options
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/control.sh
var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/INSTALLED

this can be made with the command:

swprogs/swlist -c - bz\*  @ /  | tar tf -
*/

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
#include "swverid.h"
#include "swi.h"
#include "swevents.h"
#include "globalblob.h"
#include "vplob.h"
#include "strar.h"
#include "swicat.h"
#include "swicat_s.h"
#include "swicat_e.h"
#include "fnmatch_u.h"
#include "swgpg.h"

static
int
show_nopen(void)
{
	int ret;
	ret = /*--*/open("/dev/null",O_RDWR);
	if (ret<0)
		fprintf(stderr, "fcntl error: %s\n", strerror(errno));
	close(ret);
	return ret;
}

static
int
fix_offset(SWICAT_E * e)
{
	int ret;

	E_DEBUG("");
	ret = uxfio_fcntl(e->entry_fdM, UXFIO_F_GET_VEOF, 0);
	E_DEBUG2("ret=%d", ret);
	if (ret < 0) {
		/* virtual file not set */
		ret = uxfio_lseek(e->entry_fdM, (off_t)(0), SEEK_CUR);
		E_DEBUG2("ret=%d", ret);
		if (ret < 0) {
			e->restore_offsetM = -1;
			return -1;
		}
		E_DEBUG2("setting restore_offsetM to %d", ret);
		e->restore_offsetM = ret;
		ret = uxfio_lseek(e->entry_fdM, (off_t)(e->start_offsetM), SEEK_SET);
		E_DEBUG2("ret=%d", ret);
		if (ret < 0) {
			SWLIB_ERROR("setting initial position");
			return -1;
		}
		ret = uxfio_fcntl(e->entry_fdM, UXFIO_F_SET_VEOF, (int)(e->end_offsetM));
		E_DEBUG2("ret=%d", ret);
		if (ret < 0) {
			SWLIB_ERROR("setting veof");
			return -1;
		}
	} else {
		if (ret != (int)(e->end_offsetM)) {
			/* internal error */
			SWLIB_ERROR("INTERNAL ERROR");
			return -1;
		}
		ret = uxfio_lseek(e->entry_fdM, (off_t)(0), UXFIO_SEEK_VSET);
		E_DEBUG2("ret=%d", ret);
		if (ret < 0) {
			SWLIB_ERROR("error setting UXFIO_SEEK_VSET with offset 0");
			return -1;
		}
	}
	E_DEBUG("");
	return 0;
}

static
int
unfix_offset(SWICAT_E * e)
{
	int ret;
	uxfio_fcntl(e->entry_fdM, UXFIO_F_SET_VEOF, -1);
	ret = uxfio_lseek(e->entry_fdM, (off_t)(e->restore_offsetM), SEEK_SET);
	if (ret < 0) {
		SWLIB_ERROR("setting veof");
		return -1;
	}
	return 0;
}

static
void
set_entry_fd(SWICAT_E * e)
{
	if (e->xformatM)
		xformat_set_ifd(e->xformatM, e->entry_fdM);
	if (e->xformatM->swvarfsM)
		e->xformatM->swvarfsM->fdM = e->entry_fdM;
}

static
int
init_catalog_entry(SWICAT_E * e, SWVARFS * swvarfs, char * fpath)
{
        char * path;
        struct stat st;
	int cfd;

	E_DEBUG("ENTERING");
	cfd = e->entry_fdM;

	if (fpath == NULL)
	        path = swvarfs_get_next_dirent(swvarfs, &st);
	else
	        path = fpath;

	if (path == NULL || strlen(path) == 0) {
		return -1;
	}
	E_DEBUG2("new entry: %s", path);

	e->entry_prefixM = strdup(path);
	e->start_offsetM = uxfio_lseek(swvarfs->fdM, (off_t)0, SEEK_CUR);
	e->end_offsetM = -1;
	E_DEBUG("");
	while ((path=swvarfs_get_next_dirent(swvarfs, &st)) != NULL &&
		swvarfs->eoaM == 0 &&
		strlen(path) &&
		strstr(path, e->entry_prefixM) == path
	) {
		E_DEBUG2("    entry: %s", path);
		;
       	}
	E_DEBUG("");
	e->end_offsetM = uxfio_lseek(swvarfs->fdM, (off_t)0, SEEK_CUR);
	E_DEBUG("LEAVING");
	return 0;
}
	
static
int
initialize_file_conflicts_fd(SWI * swi, SWHEADER * swheader)
{
	char * line;
	char * next_line;
	char * tag;
	char * number;
	char * value;
	char * keyword;
	int retval;
	int ix;
	int ex_memfd;
	int re_memfd;
	SWHEADER_STATE state1;

	E_DEBUG("");
	ex_memfd = -1;
	re_memfd = -1;
	retval = 0;
	swheader_store_state(swheader, &state1);
	swheader_reset(swheader);
	
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
			;
		} else if (strcmp(keyword, SW_A_fileset) == 0) {
			while((line=swheader_get_next_attribute(swheader))) {
				keyword = swheaderline_get_keyword(line);
				value = swheaderline_get_value(line, (int *)NULL);
				if (strcmp(keyword, SW_A_excluded_from_install) == 0) {
					if (ex_memfd < 0) ex_memfd = swlib_open_memfd();
					uxfio_write(ex_memfd, value, strlen(value));
					uxfio_write(ex_memfd, "\n", 1);
				} else if (strcmp(keyword, SW_A_replaced_by_install) == 0) {
					if (re_memfd < 0) re_memfd = swlib_open_memfd();
					uxfio_write(re_memfd, value, strlen(value));
					uxfio_write(re_memfd, "\n", 1);
				}
			}
			if (ex_memfd > 0) uxfio_write(ex_memfd, "\0", 1);
			if (re_memfd > 0) uxfio_write(re_memfd, "\0", 1);
			break;
		}
	}
	swheader_restore_state(swheader, &state1);
	swi->excluded_file_conflicts_fdM = ex_memfd;
	swi->replaced_file_conflicts_fdM = re_memfd;
	return 0;
}

SWICAT_E *
swicat_e_create(void)
{
	SWICAT_E * swicat_e;
	swicat_e = (SWICAT_E *)malloc(sizeof(SWICAT_E));
	SWLIB_ASSERT(swicat_e != NULL);
	
	swicat_e->entry_fdM = -1;
	swicat_e->start_offsetM = -1;
	swicat_e->end_offsetM = -1;
	swicat_e->entry_prefixM = NULL;
	swicat_e->xformatM = NULL;
	swicat_e->taruM = NULL;
	swicat_e->swiM = NULL;
	swicat_e->tmp_fdM = swlib_open_memfd();
	swicat_e->restore_offsetM = -1;
	swicat_e->xformat_close_on_e_deleteM = 0;
	swicat_e->taru_close_on_e_deleteM = 0;
	swicat_e->swi_close_on_e_deleteM = 0;
	return swicat_e;
}

void
swicat_e_delete(SWICAT_E * swicat_e)
{
	if (swicat_e->entry_prefixM)
		free(swicat_e->entry_prefixM);

	if (swicat_e->xformat_close_on_e_deleteM)
		if (swicat_e->xformatM)
			xformat_close(swicat_e->xformatM);
	
	if (swicat_e->swi_close_on_e_deleteM)
		if (swicat_e->swiM)
			swi_delete(swicat_e->swiM);

	if (swicat_e->taru_close_on_e_deleteM)
		if (swicat_e->taruM)
			taru_delete(swicat_e->taruM);

	if (swicat_e->entry_fdM >= 0)
		uxfio_close(swicat_e->entry_fdM);
	if (swicat_e->tmp_fdM >= 0)
		uxfio_close(swicat_e->tmp_fdM);
	free(swicat_e);
}

int
swicat_e_reset_fd(SWICAT_E * e)
{
	fix_offset(e);
	return fix_offset(e);
}


/**
 * swicat_e_open_entry_tarball - read a catalog entry
 * @swicat_e:  object, previously created.
 * @ifd: file descriptor containing the archive
 *
 * This routine assmues the entire archive has only one entry.
 */

int
swicat_e_open_entry_tarball(SWICAT_E * swicat_e, int ifd)
{
	int retval;
	int ret;
	intmax_t len;
	int mem_fd;
	int nullarchive;
	char * b;
	TARU * taru;
	char * zerobuf;
	char * readbuf;
	XFORMAT * xformat;

	E_DEBUG("");
	retval = 0;
	mem_fd = swlib_open_memfd();
	SWLIB_ASSERT(mem_fd >= 0);

	taru = taru_create();
	SWLIB_ASSERT(taru != NULL);

	E_DEBUG("");
	ret = taru_process_copy_out(taru, ifd, mem_fd, NULL/*defer*/, NULL, arf_ustar, -1, -1, &len, NULL);
	if (ret < 0) {
		taru_delete(taru);
		uxfio_close(mem_fd);	
		SWLIB_ERROR2("taru_process_copy_out returned %d", ret);
		return -1;
	}
	
	E_DEBUG("");
	ret = uxfio_lseek(mem_fd, (off_t)0, SEEK_SET);
	if (ret != 0) {
		taru_delete(taru); uxfio_close(mem_fd);	return -2;
	}

	E_DEBUG("");
	/* Check for empty i.e. end-of-archive */
	nullarchive = 0;
	zerobuf = (char*)malloc(TARRECORDSIZE);
	readbuf = (char*)malloc(TARRECORDSIZE);
	memset(zerobuf, '\0', TARRECORDSIZE);
	uxfio_read(mem_fd, readbuf, TARRECORDSIZE);
	if (memcmp(zerobuf, readbuf, TARRECORDSIZE) == 0) {
		uxfio_read(mem_fd, readbuf, TARRECORDSIZE);
		if (memcmp(zerobuf, readbuf, TARRECORDSIZE) == 0) {
			/* empty archive, handle this as a special
			   error case */
			nullarchive = 1;	
		}
	}
	E_DEBUG("");
	free(zerobuf);
	free(readbuf);

	E_DEBUG("");
	ret = uxfio_lseek(mem_fd, (off_t)0, SEEK_SET);
	if (ret != 0) {
		taru_delete(taru); uxfio_close(mem_fd);	return -2;
	}

	xformat = xformat_open(-1, -1, arf_ustar);
	SWLIB_ASSERT(xformat != NULL);

	E_DEBUG("");
	if (nullarchive == 0) {
		ret = xformat_open_archive_by_fd(xformat, mem_fd, UINFILE_DETECT_NATIVE, 0);
		if (ret != 0) {
			E_DEBUG("error, returning -3");
			uxfio_close(mem_fd);
			taru_delete(taru);
			xformat_close(xformat);	
			return -3;
		}
	}

	E_DEBUG("");
	swicat_e->xformatM = xformat;
	swicat_e->taruM = taru;
	swicat_e->taru_close_on_e_deleteM = 1;
	swicat_e->start_offsetM = 0;
	swicat_e->end_offsetM = (int)len;
	swicat_e->entry_fdM = mem_fd;
	swicat_e->entry_prefixM = NULL;

	E_DEBUG("");
	/* sanity check */
	ret = uxfio_lseek(mem_fd, (off_t)0, SEEK_END);
	if (ret < 0) {
		SWLIB_ERROR("");
		return -4;
	}	
	if (ret != (int)len) {
		SWLIB_ERROR3("sanity check failed ret=%d, len=%d", ret, (int)len);
		return -5;
	}
	
	E_DEBUG("");
	uxfio_lseek(mem_fd, (off_t)0, SEEK_SET);

	E_DEBUG("");
	if (nullarchive == 0) {
		ret = init_catalog_entry(swicat_e, swicat_e->xformatM->swvarfsM, (char*)NULL);
		if (ret != 0) {
			SWLIB_ERROR("init_catalog_entry failed");
			return -6;
		}
		retval = 0;
	} else {
		retval = SWICAT_RETVAL_NULLARCHIVE;
	}	
	E_DEBUG("");
	uxfio_lseek(mem_fd, (off_t)0, SEEK_SET);

	E_DEBUG2("retval = %d", retval);
	return retval;
}

/**
 * swicat_e_open_catalog_tarball - read a catalog tarball containing
 *    multiple entries.
 * @ifd: file descriptor containing the archive
 *
 * Returns a list of (SWICAT_E*) objects, NUL on error.
 */

VPLOB *
swicat_e_open_catalog_tarball(int cfd)
{
        SWVARFS * swvarfs;
	VPLOB * list;
	SWICAT_E * e;
	int ret;
        char * path;
        struct stat st;

	E_DEBUG("ENTERING");
	list = vplob_open();
	if (list == NULL) return NULL;	
	
	uxfio_lseek(cfd, (off_t)0, SEEK_SET);

        swvarfs = swvarfs_opendup(cfd, UINFILE_DETECT_NATIVE, (mode_t)0);
	E_DEBUG2("swvarfs=%p", (void*)swvarfs);
	if (swvarfs == NULL) return NULL;

        while (
		(path=swvarfs_get_next_dirent(swvarfs, &st)) != NULL &&
		strlen(path) &&
		/*
 		 * taru_read_in_tar_header2() sets this TRAILER!!! on detecting NUL blocks
 		 * in a tar archive.  Out of defererence to cpio, continue this.
 		 * Somehow it worked, but when taru_read_in_tar_header2() was made
 		 * to read both trailer nul blocks (c.March2014) a hang occured here
 		 * and hence discovery of this long standing bug.
 		 */
		strcmp(CPIO_INBAND_EOA_FILENAME, path)
	) {
		E_DEBUG2("new entry: %s", path);
		e = swicat_e_create();
		vplob_add(list, e);
		e->entry_fdM = cfd;
		ret = init_catalog_entry(e, swvarfs, path);
	}
	swvarfs_close(swvarfs);
	E_DEBUG("LEAVING");
	return list;
}

int
swicat_e_find_attribute_file(SWICAT_E * swicat_e, char * attribute, STROB * pbuf, AHS * ahs)
{
	int retval;
	int ret;
	int memfd;
	int ufd;
	char * name;
	struct stat st;
	XFORMAT * xformat;
	STROB * tmp;

	tmp = strob_open(32);
	strob_strcpy(pbuf, "");

	E_DEBUG("");
	ret = fix_offset(swicat_e);
	if (ret < 0) return -1;

	E_DEBUG("");
	memfd = swicat_e->entry_fdM;
	xformat = swicat_e->xformatM;
	set_entry_fd(swicat_e);

	E_DEBUG("");
	strob_strcpy(tmp, "*/");
	strob_strcat(tmp, attribute);

	retval = 1;
	E_DEBUG("");
	while ((name=xformat_get_next_dirent(xformat, &st)) != NULL) {
		E_DEBUG2("name=%s", name);
		if (fnmatch(strob_str(tmp), name, 0) == 0) { 
			ufd = xformat_u_open_file(xformat, name);
			if (ufd < 0) {
				retval = -1;
				break;
			}
			if (ahs) {
				ahs_copy(ahs, swvarfs_get_ahs(xformat->swvarfsM));
			}
			ret = swlib_ascii_text_fd_to_buf(pbuf, ufd);
			E_DEBUG2("swlib_ascii_text_fd_to_buf returned %d", ret);
			if (ret != 0) {
				retval = -2;
			} else {
				retval = 0;
			}
			ret = xformat_u_close_file(xformat, ufd);
		}
	}
	if (retval != 0) {
		/* FIXME */
		/* fill in default value */
		if (retval < 0) {
			SWLIB_ERROR("need default value");
		}

	}
	swlib_squash_all_trailing_vnewline(strob_str(pbuf));
	unfix_offset(swicat_e);
	strob_close(tmp);
	E_DEBUG3("name=%s  value=[%s]", attribute, strob_str(pbuf));
	E_DEBUG2("retval=%d", retval);
	return retval;
}

SWHEADER *
swicat_e_isf_parse(SWICAT_E * e, int *pstatus, AHS * ahs)
{
	STROB * tmp;
	SWHEADER * swheader;
	int ifd = swlib_open_memfd();
	int ofd = swlib_open_memfd();
	char * img;
	char * img2;
	int datalen;
	int ret;

	E_DEBUG("BEGIN");
	tmp = strob_open(12);
	if (pstatus) *pstatus = 0;
	ret = swicat_e_find_attribute_file(e, SW_A_INSTALLED, tmp, ahs);
	E_DEBUG2("swicat_e_find_attribute_file ret=[%d]", ret);
	if (ret != 0) {
		/* fatal error */
		E_DEBUG("ret != 0");
		SWLIB_ERROR("");
		if (pstatus) *pstatus = 1;
		E_DEBUG("returning NULL");
		return NULL;
	} 

	uxfio_write(ifd, strob_str(tmp), strob_strlen(tmp));
	uxfio_write(ifd, "\n\n\n", 2);     /* the _e_find_attribute_file()
					      routine strips the trailing newline,
					      we have to put it back */
	uxfio_lseek(ifd, (off_t)0, SEEK_SET);

	E_DEBUG2("running sw_yyparse on fd=%d", ofd);
	ret = sw_yyparse(ifd, ofd, SW_A_INSTALLED, 0, SWPARSE_FORM_MKUP_LEN);
	E_DEBUG2("sw_yyparse: ret=[%d]", ret);
	if (ret) {
		fprintf(stderr,
		"%s: error parsing %s\n", swlib_utilname_get(), SW_A_INSTALLED);
		if (pstatus) *pstatus = 2;
		return NULL;
	}
	uxfio_lseek(ofd, (off_t)0, SEEK_SET);
	img = swi_com_get_fd_mem(ofd, &datalen);
	img2 = malloc((size_t)datalen+1);
	memcpy(img2, img, datalen);
	img2[datalen] = '\0';	
	swheader = swheader_open((char *(*)(void *, int *, int))(NULL), NULL);
	swheader_set_image_head(swheader, img2);
	uxfio_close(ifd);
	uxfio_close(ofd);
	strob_close(tmp);
	return swheader;
}

SWI_FILELIST *
swicat_e_make_file_list(SWICAT_E * e, SWUTS * swuts, SWI * swi)
{
	int ret;
	STROB * tmp;
	SWI_FILELIST * flist;
	char * location;

	E_DEBUG("");
	flist = NULL;
	tmp = strob_open(12);

	if (swi == NULL) {
		SWLIB_ERROR("");
		goto out;
	}

	if (fix_offset(e) < 0) {
		SWLIB_ERROR("");
		goto out;
	}

	/*
 	 * find the location attribute, which is stored as a separate file
 	 * because it is a version attribute
 	 */	
	E_DEBUG("");
	ret = swicat_e_find_attribute_file(e, SW_A_location, tmp, NULL);
	if (ret < 0) {
		/* fatal error */
		SWLIB_ERROR("");
		goto out;
	} else if (ret > 0) {
		/* attribute file not found */
		/* SWLIB_WARN("location file unexpectedly not found"); */
		strob_strcpy(tmp, "/");
	}
	location = strdup(strob_str(tmp));

	flist = swi_make_file_list(swi, swuts, location, SW_FALSE);
out:
	unfix_offset(e);
	strob_close(tmp);
	return flist;
}

int
swicat_e_verify_gpg_signature(SWICAT_E * e, SWGPG_VALIDATE * swgpg)
{
	int i;
	int retval;
	int ret;
	char * sig;
	STROB * gpg_status;
	STROB * tmp;
	STROB * sig_buf;
	char * name;
	struct stat st;
	int catalog_tar_fd;
	int sig_fd;

	retval = 0;

	swgpg_reset(swgpg);

	gpg_status = strob_open(12);
	sig_buf = strob_open(12);
	tmp = strob_open(12);

	E_DEBUG("BEGIN");
	
	ret = fix_offset(e);
	if (ret < 0) {
		SWLIB_ERROR("");
		return -1;
	}	
	while ((name=xformat_get_next_dirent(e->xformatM, &st)) != NULL) {
                if (xformat_is_end_of_archive(e->xformatM)){
                        break;
                }
		E_DEBUG2("2name=%s", name);
		if (
			fnmatch("*/export/catalog.tar.sig", name, 0) == 0 ||
			fnmatch("*/export/catalog.tar.sig*", name, 0) == 0 
		) {
			E_DEBUG("");
			sig_fd = xformat_u_open_file(e->xformatM, name);
			SWLIB_ASSERT(sig_fd >= 0);
			ret = swlib_ascii_text_fd_to_buf(tmp, sig_fd);
			SWLIB_ASSERT(ret == 0);
			strar_add(swgpg->list_of_sig_namesM, name);
			strar_add(swgpg->list_of_sigsM, strob_str(tmp));
			xformat_u_close_file(e->xformatM, sig_fd);
			E_DEBUG("");
		} 
	}

	ret = fix_offset(e);
	if (ret < 0) {
		SWLIB_ERROR("");
		return -1;
	}	

	catalog_tar_fd = -1;
	while ((name=xformat_get_next_dirent(e->xformatM, &st)) != NULL) {
		if (fnmatch("*/export/catalog.tar", name, 0) == 0) {
			catalog_tar_fd = xformat_u_open_file(e->xformatM, name);
			break;
		}
	}
	if (catalog_tar_fd < 0) {
		retval = -1;
		goto error_out;
	}
	uxfio_lseek(catalog_tar_fd, (off_t)0, SEEK_SET);

	/* Now loop over the list of sigs */

	i = 0;
	while((sig=strar_get(swgpg->list_of_sigsM, i)) != NULL) {
		name = strar_get(swgpg->list_of_sig_namesM, i);
		E_DEBUG2("signame=%s", name);
		E_DEBUG2("sig=%s", sig);
		ret = uxfio_lseek(catalog_tar_fd, (off_t)0, SEEK_SET);
		SWLIB_ASSERT(ret >= 0);
		ret = swgpg_run_gpg_verify(swgpg, catalog_tar_fd, sig, 2, gpg_status);
		/* strar_add(swgpg->list_of_status_blobsM, strob_str(gpg_status)); */
		swgpg_disentangle_status_lines(swgpg, strob_str(gpg_status));
		/* fprintf(stderr, "%s", strob_str(gpg_status)); */
		i++;
	}

	xformat_u_close_file(e->xformatM, catalog_tar_fd);

	/* Now classify each signature as to BAD, GOOD, NO_SIG, or NOKEY */

	swgpg_set_status_array(swgpg);

	/* swgpg_show_all_signatures(swgpg, STDERR_FILENO); */

error_out:
	unfix_offset(e);
	strob_close(gpg_status);
	strob_close(sig_buf);
	strob_close(tmp);
	return retval;
}

/**
 * swicat_e_open_swi - find the .../export/catalog.tar file and open 
 *    a (SWI*) swi object.
 *
 * Returns (SWI*) pointer, NUL on error.
 */

SWI *
swicat_e_open_swi(SWICAT_E * e)
{
	SWI * swi;
	int ret;
	int isf_status;
	int ifd;
	int newfd;
	int newfd2;
	int catalog_tar_fd;
	char * name;
	struct stat st;
	XFORMAT * xformat;
        SWVARFS * swvarfs;
	SWPATH * swpath;
	UINFORMAT * uinformat;
	SWHEADER * installed_header;

	E_DEBUG("BEGIN");
	E_DEBUG2("LOWEST ENTERING AA swicat_e_open_swi FD=%d", show_nopen());
	ret = fix_offset(e);
	if (ret < 0) {
		SWLIB_ERROR("");
		return NULL;
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	newfd = swlib_open_memfd();

	ifd = e->entry_fdM;
	ret = swlib_pipe_pump(newfd, ifd);

	/* 
	   e->entry_fdM is a file that contains multiple catalog entries
	   such as:

	var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/
	var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/...
	var/lib/swbis/catalog/bzip2-devel/bzip2-devel/1.0.2/0/INSTALLED
	var/lib/swbis/catalog/foo/foo/1.0.2/0/
	var/lib/swbis/catalog/foo/foo/1.0.2/0/..
	var/lib/swbis/catalog/foo/foo/1.0.2/0/INSTALLED

	   newfd contains only the entry that belongs to this SWICAT_E object

	*/

	E_DEBUG2("swlib_pipe_pump returned %d", ret);
	
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	uxfio_lseek(newfd, (off_t)(0), SEEK_SET);
	E_DEBUG2("CALLING swvarfs_opendup(%d", newfd);
       	swvarfs = swvarfs_opendup(newfd, UINFILE_DETECT_NATIVE, (mode_t)0);
	E_DEBUG2("LOWEST AFTER swvarfs_opendup FD=%d", show_nopen());
	if (swvarfs == NULL) {
		E_DEBUG("");
		return NULL;
	}
	swvarfs->did_dupM = 1; /* This causes swvarfs_close to close the fd */
	E_DEBUG("searching: START");

	/*
	 * Now find the ../export/catalog.tar file and return a file descriptor
	 */	

	E_DEBUG2("LOWEST BEFORE swvarfs_get_next_dirent FD=%d", show_nopen());
	catalog_tar_fd = -1;
        while ((name=swvarfs_get_next_dirent(swvarfs, &st)) != NULL && strlen(name)) {
		E_DEBUG2("searching: name=%s", name);
		if (fnmatch("*/export/catalog.tar", name, 0) == 0) {
			E_DEBUG("found catalog.tar");
			catalog_tar_fd = swvarfs_u_open(swvarfs, name);
			break;
		}
	}
	E_DEBUG2("LOWEST AFTER swvarfs_get_next_dirent FD=%d", show_nopen());
	E_DEBUG("searching: END");
	E_DEBUG2("catalog_tar_fd=%d", catalog_tar_fd);
	if (catalog_tar_fd < 0) {
		E_DEBUG("error");
		unfix_offset(e);
		return NULL;
	}

	/*
	 * Now transfer this file to its own dedicated file descriptor
	 */

	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	newfd2 = swlib_open_memfd();
	E_DEBUG("");
	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	ret = swlib_pipe_pump(newfd2, catalog_tar_fd);
	E_DEBUG("");
	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	uxfio_lseek(newfd2, (off_t)(0), SEEK_SET);
	E_DEBUG2("catalog_tar_fd is %d", catalog_tar_fd);
	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	swvarfs_u_close(swvarfs, catalog_tar_fd);
	E_DEBUG("");
	E_DEBUG("");
	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	swvarfs_close(swvarfs);
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG2("UXFIO fd: %s",(uxfio_show_all_open_fd(stderr), ""));	
	
	/* uxfio_close(newfd); */
	ifd = newfd2;
	E_DEBUG2("LOWEST FD=%d", show_nopen());

	/*
	 * Now create the SWI object which models the in-memory structures
	 * of the exported catalog contained in catalog.tar
	 */
	
	E_DEBUG("");
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	swi = swi_create();

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	/*
	 * open the package catalog
	 */

	xformat = xformat_open(-1, -1, arf_ustar);
	E_DEBUG2("entry_fdM = %d", e->entry_fdM);
	ret = xformat_open_archive_by_fd(xformat, ifd,
			UINFILE_DETECT_IEEE,
			(mode_t)(0));
	if (ret < 0) {
		E_DEBUG("error");
		unfix_offset(e);
		return NULL;
	}

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	/*
	 * fill in the (SWI*) structure
	 */

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	swi->xformatM = xformat;
	swi->xformat_close_on_deleteM = 1;
	if (e->xformatM)
		xformat_close(e->xformatM);
	e->xformatM = xformat;
	swi->swvarfsM = xformat->swvarfsM;
	xformat->swvarfs_is_externalM = 1;
	swi->swvarfs_close_on_deleteM = 1;
	swi->uinformatM = swvarfs_get_uinformat(swi->swvarfsM);
	uinformat = swvarfs_get_uinformat(swi->swvarfsM);
	swi->swvarfsM->uinformat_close_on_deleteM = 0;
	swi->uinformat_close_on_deleteM = 1;
	swpath = uinfile_get_swpath(uinformat);
	swpath_reset(swpath);
	xformat_set_tarheader_flag(xformat, TARU_TAR_FRAGILE_FORMAT, 1);
	xformat_set_tarheader_flag(xformat, TARU_TAR_RETAIN_HEADER_IDS, 1);
	swi->uinformatM = uinformat;
	swi->swpathM = swpath;
	swi->swi_pkgM->target_hostM = strdup("localhost");
	xformat_set_ifd(swi->xformatM, ifd);
	E_DEBUG("");

	E_DEBUG2("LOWEST FD=%d", show_nopen());
	/* FIXME, also check the GPG signature here */

	/*
         * Decode the package catalog
	 */
	
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	ret = swi_decode_catalog(swi);
	if (ret < 0) {
		E_DEBUG("error");
		unfix_offset(e);
		return NULL;
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	uxfio_close(ifd);  /* newfd2 */
	unfix_offset(e);
	set_entry_fd(e);
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	E_DEBUG2("swi=%p", (void*)swi);
	E_DEBUG2("LOWEST LEAVING AA swicat_e_open_swi FD=%d", show_nopen());

	installed_header = swicat_e_isf_parse(e, &isf_status, (AHS*)NULL);
	if (!installed_header)
		return NULL;

	initialize_file_conflicts_fd(swi, installed_header);

	return swi;
}

char *
swicat_e_form_catalog_path(SWICAT_E * e, STROB * buf, char * isc_path, int active_name)
{
	char * p;
	char * risc;
	p = e->entry_prefixM;
	if (p == NULL) return NULL;
	if (swlib_check_clean_path(p)) return NULL;

	if (isc_path) {
		/* risc = swlib_return_relative_path(isc_path); */
		/* the path has had the file:/// URL syntax processed, hence
		 * if it is absolute it really should be absolute
		 */
		strob_strcpy(buf, isc_path);
	} else {
		strob_strcpy(buf, "");
	}

	if (active_name == SWICAT_ACTIVE_ENTRY) {
		swlib_unix_dircat(buf, p);
		p = strob_str(buf);
		swlib_squash_trailing_slash(p);
		return p;
	} else if (active_name == SWICAT_DEACTIVE_ENTRY) {
		/* make an path name with a leading underscore in the
		  sequence number part, for example
			var/lib/swbis/catalog/emacs/emacs/21.3.1/_1
		  This tells all the utilities to act as if it does not exist.
		*/
		char * seq;

		swlib_unix_dircat(buf, p);
		p = strob_str(buf);
		swlib_squash_trailing_slash(p);
		seq = strrchr(p, '/');
		if (seq == NULL) return NULL;
		if (seq == p) return NULL;
		seq++;
		/* make sanity check */
		if (*seq == '\0' || isdigit(*seq) == 0) {
			/* sanity error */
			SWLIB_ERROR("badly formed catalog entry path");
			return NULL;
		}
		strob_set_length(buf, strlen(p) + 1);	
		memmove(seq+1, seq, strlen(seq)+1);
		*seq = '_';	
		return strob_str(buf);
	}
	return NULL;
}
