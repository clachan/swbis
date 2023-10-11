/* etar.c - A simpler interface to the tar writing routines

   Copyright (C) 2005 Jim Lowe
 
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
#include "ahs.h"
#include "strob.h"
#include "swlib.h"
#include "swutilname.h"
/* #include "taru.h" */
#include "etar.h"
#include "atomicio.h"
	
#define HEADER_ALLOC 2048
#define HDR(a)  ((struct tar_header *)((a)->tar_hdrM))
#define ETAR_UNSET_FILENAME "ETAR_UNSET_FILENAME"

ETAR *
etar_open (int flags)
{
	ETAR * etar;
	etar = (ETAR *)malloc(sizeof(ETAR));
	if (!etar) return etar;
	etar->etar_tarheaderflagsM = flags;
	etar->timeM = time(NULL);
	etar->tar_hdrM = (char*)malloc((size_t)(HEADER_ALLOC));
	memset(etar->tar_hdrM, '\0', (size_t)(HEADER_ALLOC));
	return etar;	
}

void
etar_close (ETAR * etar)
{
	free(etar->tar_hdrM);
	free(etar);
}

struct tar_header *
etar_get_hdr (ETAR * etar)
{
	return HDR(etar);
}

void
etar_init_hdr (ETAR * etar)
{
	memset(etar->tar_hdrM, '\0', (size_t)(HEADER_ALLOC));
	*(HDR(etar)->version) = '0';
	*(HDR(etar)->version + 1)= '0';
	strncpy(HDR(etar)->magic, TMAGIC, TMAGLEN);
	strncpy(HDR(etar)->magic + TMAGLEN, TVERSION, TVERSLEN);
	etar_set_pathname(etar, ETAR_UNSET_FILENAME);
	etar_set_mode_ul(etar, (unsigned int)(0550));
	etar_set_uid(etar, 0);
	etar_set_gid(etar, 0);
	etar_set_size(etar, 0);
	etar_set_time(etar, etar->timeM);
	etar_set_typeflag(etar, DIRTYPE);
	etar_set_linkname(etar, "");
	etar_set_uname(etar, "");
	etar_set_gname(etar, "");
	etar_set_devmajor(etar, (unsigned long)(0));
	etar_set_devminor(etar, (unsigned long)(0));
	etar_set_chksum(etar);
}

int
etar_emit_header(ETAR * etar, int fd)
{
	int ret;
	ret = atomicio((ssize_t (*)(int, void *, size_t))(write),
		fd, (void*)(HDR(etar)), TARRECORDSIZE);
	if (ret != TARRECORDSIZE) {
		return -1;
	}
	return TARRECORDSIZE;
}

int
etar_emit_data_from_fd(ETAR * etar, int ofd, int ifd)
{
	int ret;
	int ret1;
	int remains;
	ret = swlib_pipe_pump(ofd, ifd);
	if (ret < 0) {
		fprintf(stderr, "%s: etar_emit_data_from_fd(): loc=1: ret = %d\n",
			swlib_utilname_get(), ret);
		return -1;
	}
	remains = ret % TARRECORDSIZE;
	if (ret && remains > 0) {
		remains = TARRECORDSIZE - remains;
		ret1 = swlib_pad_amount(ofd, remains);
		if (ret1 < 0) {
			fprintf(stderr, "%s: etar_emit_data_from_fd(): loc=2: ret = %d\n",
				swlib_utilname_get(), ret);
			return -2;
		}
	} else {
		remains = 0;
	}
	return ret + remains;
}

int
etar_emit_data_from_buffer(ETAR * etar, int ofd, char * buf, int bufsize)
{
	int len;
	int ret;
	int ret1;
	int remains;
	if (bufsize < 0)
		len = strlen(buf);
	else
		len = bufsize;
	ret = atomicio(uxfio_write, ofd, (void*)buf, len);
	if (ret < 0) return ret;
	if (ret != len) return -2;
	remains = ret % TARRECORDSIZE;
	if (ret && remains > 0) {
		remains = TARRECORDSIZE - remains;
		ret1 = swlib_pad_amount(ofd, remains);
		if (ret1 < 0) return -2;
	} else {
		remains = 0;
	}
	return ret + remains;
}

int
etar_set_size_from_buffer(ETAR * etar, char * buf, int bufsize)
{
	int len;
	if (bufsize < 0)
		len = strlen(buf);
	else
		len = bufsize;
	etar_set_size(etar, (unsigned int)(len));
	return len;	
}

int
etar_set_size_from_fd(ETAR * etar, int fd, int * newfd)
{
	int tmp_fd;
	int size;
	int vfd;

	if (newfd) *newfd = -1;
	if (uxfio_espipe(fd)) {
		/*
		 * can't seek
		 */
		tmp_fd = swlib_open_memfd();
		swlib_pipe_pump(tmp_fd, fd);
		vfd = tmp_fd;
		if (newfd == NULL)
			fprintf(stderr, "usage error in etar_set_size_from_fd\n");
		if (newfd) *newfd = tmp_fd;
	} else {
		vfd = fd;
	}

	/*
	 * Now can seek
	 */
	size = (size_t)uxfio_lseek(vfd, 0, SEEK_END);
	if (size < 0) {
		return -1;
	}
	etar_set_size(etar, (unsigned int)(size));
	uxfio_lseek(vfd, 0, SEEK_SET);
	return size;
}

int
etar_set_pathname(ETAR * etar, char * pathname)
{
	return taru_set_new_name(HDR(etar), -1, pathname, etar->etar_tarheaderflagsM);
}

int
etar_set_linkname(ETAR * etar, char * name)
{
	memset(HDR(etar)->linkname, '\0', 100);
	strncpy(HDR(etar)->linkname, name, 100);
	return strlen(name) <= 100 ? 0 : 1;
}

int
etar_set_uname(ETAR * etar, char * name)
{
	strncpy(HDR(etar)->uname, name, THB_FL_uname);
	if (strlen(name) > THB_FL_uname)
		return -1;
	else
		return 0;
}

int
etar_set_gname(ETAR * etar, char * name)
{
	strncpy(HDR(etar)->gname, name, THB_FL_gname);
	if (strlen(name) > THB_FL_gname)
		return -1;
	else
		return 0;
}

void
etar_set_chksum(ETAR * etar)
{
	taru_set_tar_header_sum(HDR(etar), etar->etar_tarheaderflagsM);
}

void
etar_set_mode_ul(ETAR * etar, unsigned int mode_i)
{
	MODE_TO_CHARS(mode_i, HDR(etar)->mode, 0 /*termch*/);
}

void
etar_set_uid(ETAR * etar, unsigned int val)
{
        UID_TO_CHARS(val, HDR(etar)->uid, 0);
}        

void
etar_set_gid(ETAR * etar, unsigned int val)
{
        GID_TO_CHARS(val, HDR(etar)->gid, 0);
}        

void
etar_set_size(ETAR * etar, unsigned int val)
{
        OFF_TO_CHARS(val,  HDR(etar)->size, 0);
}

void
etar_set_time(ETAR * etar, time_t val)
{
        TIME_TO_CHARS((unsigned long int)(val), HDR(etar)->mtime, 0);
}

void
etar_set_typeflag(ETAR * etar, int tar_type)
{
	HDR(etar)->typeflag = tar_type;
}

void
etar_set_devmajor(ETAR * etar, unsigned long devno)
{
	MAJOR_TO_CHARS(devno, HDR(etar)->devmajor, 0);
}

void
etar_set_devminor(ETAR * etar, unsigned long devno)
{
	MINOR_TO_CHARS(devno, HDR(etar)->devminor, 0);
}

int
etar_write_trailer_blocks(ETAR * etar, int ofd, int nblocks)
{
	int ret;
	int retval = 0;
	int count = nblocks;
	static char * nullblock = NULL;

	if (!nullblock) {
		nullblock = malloc(512);
		if (!nullblock) {
			exit(44);
			return -1;
		}
	}
	memset(nullblock, '\0', 512);

	while (count-- > 0) {
		ret = uxfio_unix_atomic_write(ofd, (void*)(nullblock), (size_t)(512));
		if (ret < 0) {
			fprintf(stderr, "%s: etar_write_trailer_blocks(): %s\n",
				swlib_utilname_get(), strerror(errno));
			return -1;
		}
		retval += 512;
	}

	if (retval != (512 * nblocks)) {
		SWLIB_FATAL("");
		return -1;
	}
	return retval;
}
