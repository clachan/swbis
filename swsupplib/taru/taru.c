/*  taru.c  Low-level tarball utility functions 

   Copyright (C) 1998, 1999, 2014  Jim Lowe

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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "cpiohdr.h"
#include "strob.h"
#include "tarhdr.h"
#include "ahs.h"
#include "swlib.h"
#include "swutilname.h"
#include "uxfio.h"
#include "taru.h"
#include "ls_list.h"
#include "taruib.h"

static
void
i_taru_needy_message(char * filename, char * field_name)
{
	fprintf(stderr,
		"%s: %s: %s value too long for selected format\n",
			swlib_utilname_get(), filename, field_name);
	return;
}

TARU * taru_create(void)
{
	TARU * taru = (TARU*)malloc(sizeof(TARU));
	taru->idM = 'A';
	taru->headerM = strob_open(1024);
	taru->header_lengthM = 0;
	taru->taru_tarheaderflagsM = 0;
	taru->do_record_headerM = 0;	
	taru->u_name_bufferM = strob_open(8);
	taru->u_ent_bufferM = strob_open(SNAME_LEN+SNAME_LEN /* 64 */);
	taru->nullfdM = swbis_devnull_open("/dev/null", O_RDWR, 0);
	/* taru->do_md5M = 0; */
	taru->md5bufM = strob_open(64);
	strob_set_memlength(taru->md5bufM, 64);
	taru->linkrecord_disableM = 0;
	taru->preview_fdM = -1;
	taru->preview_levelM = TARU_PV_0;
	taru->preview_bufferM = strob_open(64);
	taru->read_bufferM = strob_open(TARRECORDSIZE*4);
	taru->read_buffer_posM = 0;
	taru->exthdr_dataM = strob_open(16);
	taru->etarM = etar_open(0 /* empty tar header flags */);
	taru->localbufM = strob_open(12);
	return taru;
}

void
taru_delete(TARU * taru)
{
	strob_close(taru->headerM);
	strob_close(taru->u_name_bufferM);
	strob_close(taru->u_ent_bufferM);
	strob_close(taru->md5bufM);
	strob_close(taru->preview_bufferM);
	swbis_devnull_close(taru->nullfdM);
	etar_close(taru->etarM);
	free(taru);
}

int
taru_read_pax_data_blocks(TARU * taru, int fd, char * fsource_buffer, int data_len,  ssize_t (*vfread) (int, void *, size_t)) {

	int n_blocks_to_read;
	int n;
	unsigned char * buf;
	int ret;
	int retval = 0;
	STROB * strb = taru->read_bufferM;

	n_blocks_to_read = data_len / (int)TARRECORDSIZE;

	if (data_len % TARRECORDSIZE)
		n_blocks_to_read++;

	/*
	 * preallocate the space for the ustar header at the end of the pax header
	 * data blocks
	 */
	strob_set_length(strb,
			/* current position in read buffer */ taru->read_buffer_posM +
			/* pax data blocks */   n_blocks_to_read*TARRECORDSIZE + 
			/*ustar header*/ TARRECORDSIZE +
			10 /* NUL byte and some padding */
			);
	buf = (unsigned char*)strob_str(strb) + taru->read_buffer_posM;	

	n = 0;
	while (n < n_blocks_to_read) {
		if (fsource_buffer) {
			memcpy (buf + n *TARRECORDSIZE, (void*)(fsource_buffer + (n+1)*TARRECORDSIZE), (size_t)TARRECORDSIZE);
			ret = TARRECORDSIZE;
		} else {
			ret = vfread(fd, buf + n * TARRECORDSIZE, TARRECORDSIZE);
		}
		if (ret != TARRECORDSIZE) {
			return -1;
		}

		if (taru->do_record_headerM) {
			strob_set_memlength(taru->headerM, taru->header_lengthM + 512);
			memcpy((void*)(strob_str(taru->headerM) + taru->header_lengthM), (void*)(buf + n*TARRECORDSIZE), 512);
			taru->header_lengthM += 512;
		}
		retval += ret;
		taru->read_buffer_posM += ret;
		n++;
	}
	*(buf+retval) = '\0'; /* Null terminate the data blocks */
	return retval;
}

void
taru_set_header_recording(TARU * taru, int n)
{
	taru->do_record_headerM = n;	
}

char *
taru_get_recorded_header(TARU * taru, int * len)
{
	if (len) *len = taru->header_lengthM;
	return strob_str(taru->headerM);
}

void 
taru_clear_header_buffer(TARU * taru)
{
	taru->header_lengthM = 0;
}

intmax_t
taru_hdr_get_filesize(struct new_cpio_header * file_hdr)
{
	static int check_off_t = 0;

	if (file_hdr->c_filesize >= LLONG_MAX) {
		SWLIB_FATAL("c_filesize too big: >=LLONG_MAX");
	}
	if (check_off_t == 0) {
		struct stat st;
		if (sizeof(st.st_size) < 5) {
			check_off_t = 1;
		} else {
			check_off_t = 2;
		}
	}
	if (check_off_t == 1 && file_hdr->c_filesize >= LONG_MAX) {
		SWLIB_FATAL("c_filesize too big: >=LONG_MAX");
	}
	return (intmax_t)(file_hdr->c_filesize);
}

void 
taru_append_to_header_buffer(TARU * taru, char * buf, int len) 
{
	char * s;
	strob_set_length(taru->headerM, taru->header_lengthM + len + 1);
	s = strob_str(taru->headerM);
	memcpy(s + taru->header_lengthM, buf, len);
	taru->header_lengthM += len;
}

void
taru_digs_print(FILE_DIGS * digs, STROB * buffer)
{

	if (digs->do_md5 == DIGS_ENABLE_ON)
		strob_sprintf(buffer, STROB_DO_APPEND, " [%s]", digs->md5);
	else
		strob_sprintf(buffer, STROB_DO_APPEND, " []");

		
	if (digs->do_sha1 == DIGS_ENABLE_ON)
		strob_sprintf(buffer, STROB_DO_APPEND, " [%s]", digs->sha1);
	else
		strob_sprintf(buffer, STROB_DO_APPEND, " []");
}

void
taru_digs_delete(FILE_DIGS * digs)
{
	free(digs);
}

FILE_DIGS *
taru_digs_create(void)
{
	FILE_DIGS * digs = malloc(sizeof(FILE_DIGS));
	if (!digs) return digs;
	taru_digs_init(digs, DIGS_ENABLE_OFF, 0);
	return digs;
}

void
taru_digs_init(FILE_DIGS * digs, int enable, int poison)
{
	digs->do_poisonM = poison;
	digs->md5[0] = '\0';
	digs->sha1[0] = '\0';
	digs->sha512[0] = '\0';
	digs->size[0] = '\0';
	if (enable >= 0) {
		digs->do_md5 = enable;
		digs->do_sha1 = enable;
		digs->do_sha512 = enable;
		digs->do_size = enable;
	}
}

/*
int
taru_ustar_field_size_check(struct new_cpio_header * file_hdr)
{
	int ret;
	ret = taru_fill_exthdr_needs_mask(file_hdr, 0);
	return ret; 
}
*/

int
taru_fill_exthdr_needs_mask(struct new_cpio_header * file_hdr,
		int tarheaderflags,
		int is_pax_header_allowed,
		int be_verbose)
{
  	/* file_hdr->extHeader_needs_maskM = 0; */
	int mask = 0;


	/*
	 * These tests represent tests of the limits of the ustar header fields
	 */

	if (taru_is_tar_filename_too_long(ahsStaticGetTarFilename(file_hdr),
						0 /*tarheaderflags ... this disables exceptions
						   for the gnu formats 
						   */,
						NULL /* don't care if gnu_long_links would be used
						    */, 
						0    /*is_dir*/))
	{
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "path");
	  	mask |=  TARU_EHUM_PATH;
	}


	if (taru_is_tar_linkname_too_long(ahsStaticGetTarLinkname(file_hdr),
				0 /*tarheaderflags*/, NULL /*&do_gnu_long_link*/)){
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "linkname");
	  	mask |=  TARU_EHUM_LINKPATH;
	}


	if (file_hdr->c_filesize > TAR_MAX_SIZE) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "size");
		mask |=  TARU_EHUM_SIZE;
	} 


	if (file_hdr->c_uid > TAR_MAX_UID) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "uid");
		mask |=  TARU_EHUM_UID;
	} 


	if (file_hdr->c_gid > TAR_MAX_GID) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "gid");
		mask |=  TARU_EHUM_GID;
	} 


	if (strlen(ahsStaticGetTarUsername(file_hdr)) >= THB_FL_uname ) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "owner name");
	  	mask |=  TARU_EHUM_UNAME;
	}

	if (strlen(ahsStaticGetTarGroupname(file_hdr)) >= THB_FL_gname ) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "group name");
	  	mask |=  TARU_EHUM_GNAME;
	}

	if (file_hdr->c_filesize > TAR_MAX_SIZE) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "size");
		mask |=  TARU_EHUM_SIZE;
	} 

	if (file_hdr->c_dev_maj > TAR_MAX_DEVICE) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "dev maj");
		mask |=  TARU_EHUM_DEV_MAJ;
	} 

	if (file_hdr->c_dev_min > TAR_MAX_DEVICE) {
		if (be_verbose && !is_pax_header_allowed)
			i_taru_needy_message(ahsStaticGetTarFilename(file_hdr), "dev min");
		mask |=  TARU_EHUM_DEV_MIN;
	} 

	/* FIXME check size field also */


	if ((tarheaderflags & TARU_TAR_PAXEXTHDR_MD) != 0) {
		/*
 		 * Mandatory PAX Header
 		 */
	  	mask |=  TARU_EHUM_PATH;
	  	mask |=  TARU_EHUM_LINKPATH;
		mask |=  TARU_EHUM_SIZE;
		mask |=  TARU_EHUM_UID;
		mask |=  TARU_EHUM_GID;
	  	mask |=  TARU_EHUM_UNAME;
	  	mask |=  TARU_EHUM_GNAME;
	  	mask |=  TARU_EHUM_MTIME;
		/* mask |=  TARU_EHUM_DEV_MIN; */
		/* mask |=  TARU_EHUM_DEV_MAJ; */
		mask |=  TARU_EHUM_SIZE;
	}
	return mask;
}

void
taru_exatt_init(EXATT * hnew) {
	hnew->is_in_useM = 0;
	hnew->lenM = -1;
	hnew->vendorM[0] = '\0';
	hnew->namespaceM[0] = '\0';
	strob_strcpy(hnew->attrnameM, "");
	strob_strcpy(hnew->attrvalueM, "");
}

EXATT *
taru_exatt_create(void) {
	EXATT * hnew;
	hnew = malloc(sizeof(EXATT));
	strcpy(hnew->magicM, "XH");
	hnew->attrnameM = strob_open(22);
	hnew->attrvalueM = strob_open(22);
	taru_exatt_init(hnew);
	return hnew;
}

void
taru_exatt_delete(EXATT * p) {
	strob_close((STROB*)(p->attrnameM));
	strob_close((STROB*)(p->attrvalueM));
	free(p);
}

void
taru_exattshow_debug(struct new_cpio_header * file_hdr)
{
	EXATT  *p;
	int index; 
	if (file_hdr->extattlistM == NULL) {
		return;
	}
	index = 0;
	
	fprintf(stderr, "extended header begin\n");
	while((p=(EXATT*) cplob_val(file_hdr->extattlistM, index))) {
		fprintf(stderr, "attribute=[%s] value=[%s]\n", strob_str(p->attrnameM), strob_str(p->attrvalueM)); 
		index++;
	}
	fprintf(stderr, "extended header end\n");
}

void
taru_exattlist_delete_all(struct new_cpio_header * file_hdr)
{
	EXATT  *p;
	int index; 
	if (file_hdr->extattlistM == NULL) {
		return;
	}
	index = 0;
	while((p=(EXATT*) cplob_val(file_hdr->extattlistM, index))) {
		taru_exatt_delete(p);
		index++;
	}
}

EXATT *
taru_exattlist_get_free(struct new_cpio_header * file_hdr)
{
	EXATT  *p;
	int index; 

	if (file_hdr->extattlistM == NULL) {
		file_hdr->extattlistM = (void*)cplob_open(3);
		((CPLOB*)(file_hdr->extattlistM))->refcountM = 1;
	}

	/* Loop over the records and find the first one that is not used */
	index = 0;
	while((p=(EXATT*) cplob_val(file_hdr->extattlistM, index))) {
		if (p->is_in_useM == 0) {
			taru_exatt_init(p);
			return p;
		}
		index++;
	}
	p  = taru_exatt_create();
	cplob_add_nta(file_hdr->extattlistM, (char*)p);
	return p;
}

void
taru_exattlist_init(struct new_cpio_header * file_hdr)
{
	EXATT  *p;
	int index; 

	if (file_hdr->extattlistM == NULL) {
		file_hdr->extattlistM = (void*)cplob_open(22);
		((CPLOB*)(file_hdr->extattlistM))->refcountM = 1;
	}

	/* Loop over the records and set all to not used */
	index = 0;
	while((p=(EXATT*) cplob_val(file_hdr->extattlistM, index))) {
		p->is_in_useM == 0;
		index++;
	}
}

int
taru_exatt_parse_fqname(EXATT * exatt, char * attr, int attrlen)
{
	int retval;
	STROB * tmp;
	char * s;
	int has_prefix = 0;
	int has_namespace = 0;
	int has_name = 0;

	retval = 0;
	tmp = strob_open(32);
	strob_strncpy(tmp, attr, attrlen);
			
	swlib_strncpy(exatt->vendorM, "", 2);
	swlib_strncpy(exatt->namespaceM, "", 2);
	strob_strcpy((STROB*)exatt->attrnameM, "");

	s = strob_strtok(tmp, strob_str(tmp), ".");
	while(s) {
		if (isupper(*s) && has_prefix == 0) {
			/*
 			 * This would be SCHILY, RMT, et.al.
 			 */
			swlib_strncpy(exatt->vendorM, s, EXATT_PREFIX_LEN);
			has_prefix = 1;
		} else if (!isupper(*s) && has_namespace == 0) {
			/*
 			 * This would be selinux, acl, et.al,
 			 * or an unqualified POSIX ustar header attribute
 			 */
			swlib_strncpy(exatt->namespaceM, s, EXATT_NAMESPACE_LEN);
			has_namespace = 1;
		} else if (!isupper(*s) && has_name == 0) {
			strob_strcpy(exatt->attrnameM, s);
			has_name = 1;
		} else {
			/*
 			 * this case would be a 4 field dot delimited field
 			 */
			/* for now consider this an error */
			retval = -1;
			break;
		}
		s = strob_strtok(tmp, NULL, ".");
	}

	if (has_namespace && ! has_name && ! has_prefix) {
		/*
 		 * This is the case for a an unqualified lower case
 		 * POSIX attribute
 		 */
		strob_strcpy((STROB*)exatt->attrnameM, exatt->namespaceM);
		swlib_strncpy(exatt->namespaceM, "", 2);
	}
	return retval;
}

void
taru_init_header_digs(struct new_cpio_header * file_hdr)
{
	if (file_hdr->digsM == NULL)
		file_hdr->digsM = taru_digs_create();
	else
		taru_digs_init(file_hdr->digsM, DIGS_ENABLE_OFF, 0);
}

void
taru_free_header_digs(struct new_cpio_header * file_hdr)
{
	if (file_hdr->digsM) {
		taru_digs_delete(file_hdr->digsM);
		file_hdr->digsM = NULL;
	}
}

void
taru_init_header(struct new_cpio_header * file_hdr)
{
	EXATT  *p;
	int index;
	file_hdr->c_mode = 0400;
       	file_hdr->c_uid = AHS_ID_NOBODY;
       	file_hdr->c_gid = AHS_ID_NOBODY;
       	file_hdr->c_nlink = 0;
       	file_hdr->c_mtime = time((time_t*)(NULL));
       	file_hdr->c_ctime = file_hdr->c_mtime;
       	file_hdr->c_atime = file_hdr->c_mtime;
       	file_hdr->c_atime_nsec = 0;
       	file_hdr->c_ctime_nsec = 0;
       	file_hdr->c_filesize = 0;
       	file_hdr->c_dev_maj = 0;
       	file_hdr->c_dev_min = 0;
       	file_hdr->c_rdev_maj = 0;
       	file_hdr->c_rdev_min = 0;
       	file_hdr->c_cu = TARU_C_BY_USYS;
       	file_hdr->c_cg = TARU_C_BY_GSYS;
       	file_hdr->c_is_tar_lnktype = -1;
	ahsStaticSetTarGroupname(file_hdr, "");
	ahsStaticSetTarUsername(file_hdr, "");
	ahsStaticSetPaxLinkname(file_hdr, "");
	ahsStaticSetTarFilename(file_hdr, "");
	file_hdr->usage_maskM = 0;
	file_hdr->extHeader_usage_maskM = 0;
	file_hdr->extHeader_needs_maskM = 0;
	taru_init_header_digs(file_hdr);
  	/* file_hdr->extattlistM = NULL; */
	if (file_hdr->extattlistM) {
		index = 0;
		while((p=(EXATT*) cplob_val(file_hdr->extattlistM, index))) {
			p->is_in_useM == 0;
			index++;
		}
	}
}

struct new_cpio_header *
taru_make_header(void)
{
	struct new_cpio_header *hnew;
	hnew = malloc(sizeof(struct new_cpio_header));
       	hnew->c_name = NULL;
       	hnew->c_tar_linkname = NULL;
       	hnew->c_username = NULL;
       	hnew->c_groupname = NULL;
	hnew->digsM = NULL;
  	hnew->extattlistM = NULL;
	taru_init_header(hnew);
	return hnew;
}

void
taru_set_tarheader_flag(TARU * taru, int flag, int n) {
	if (n) {
		taru->taru_tarheaderflagsM |= flag;
	} else {
		taru->taru_tarheaderflagsM &= ~flag;
	}
}

void
taru_set_preview_level(TARU * taru, int preview_level)
{
	taru->preview_levelM = preview_level;
}

int
taru_get_preview_level(TARU * taru)
{
	return taru->preview_levelM;
}

void
taru_set_preview_fd(TARU * taru, int fd)
{
	taru->preview_fdM = fd;
}

int
taru_get_preview_fd(TARU * taru)
{
	return taru->preview_fdM;
}

void 
taru_free_header(struct new_cpio_header *h)
{
	CPLOB * p;
	taru_free_header_digs(h);
	ahsStaticSetTarGroupname(h, NULL); /* these have effect of calling */
	ahsStaticSetTarUsername(h, NULL);  /* strob_close() */
	ahsStaticSetPaxLinkname(h, NULL);
	ahsStaticSetTarFilename(h, NULL);

	if (h->extattlistM) {
		p = (CPLOB*)(h->extattlistM);
		p->refcountM--;
		if (p->refcountM <= 0) {
			/* really delete */
			taru_exattlist_delete_all(h);
			cplob_shallow_close(p);
		}
	}
	swbis_free(h);
}

int
taru_print_tar_ls_list(STROB * buf, struct new_cpio_header * file_hdr, int vflag)
{
	int type;

	strob_strcpy(buf, "");
	type = taru_get_tar_filetype(file_hdr->c_mode);

	if (
		(file_hdr->c_is_tar_lnktype == 1) ||
		(
			type == REGTYPE &&
			strlen(ahsStaticGetTarLinkname(file_hdr))
		)
	) {
		type = LNKTYPE;
	}
	if (type >= 0)
		ls_list_to_string(
			/* name */ ahsStaticGetTarFilename(file_hdr),
			/* ln_name */ ahsStaticGetTarLinkname(file_hdr),
			file_hdr,
			(time_t)(0),
			buf,
			ahsStaticGetTarUsername(file_hdr),
			ahsStaticGetTarGroupname(file_hdr),
			type,
			vflag);
	else
		fprintf(stderr, "%s: unrecognized file type in mode [%d] for file: %s\n",
			swlib_utilname_get(), (int)(file_hdr->c_mode), ahsStaticGetTarFilename(file_hdr));

	/* Test line:  taru_digs_print(file_hdr->digsM, buf); */
	strob_strcat(buf, "\n");
	return 0;
}

void
taru_write_preview_line(TARU * taru, struct new_cpio_header *file_hdr)
{
	int fd;
	int flag;
	int level;
	char * filename;
	STROB * buffer;

	flag = 0;
	if (
		taru->preview_fdM < 0 ||
		taru->preview_levelM == TARU_PV_0
	) {
		return;
	}
	
	fd = taru->preview_fdM;
	level = taru->preview_levelM;
	buffer = taru->preview_bufferM;
	filename = ahsStaticGetTarFilename(file_hdr);

	if (isatty(fd)) {
		flag = ls_list_get_encoding_flag();
		ls_list_set_encoding_by_lang();
	}


	if (level >= TARU_PV_3) {
		taru_print_tar_ls_list(buffer, file_hdr, LS_LIST_VERBOSE_L2);
	} else if (level >= TARU_PV_2) {
		taru_print_tar_ls_list(buffer, file_hdr, LS_LIST_VERBOSE_L1);
	} else if (level >= TARU_PV_1) {
		/* strob_sprintf(buffer, 0, "%s\n", filename); */
		strob_strcpy(buffer, "");
		ls_list_safe_print_to_strob(filename, buffer, 0);
		strob_strcat(buffer, "\n");
	} else {
		strob_strcpy(buffer, "");
	}
	if (strob_strlen(buffer)) {
		uxfio_write(fd, strob_str(buffer), strob_strlen(buffer));
	}
	if (isatty(fd)) {
		ls_list_set_encoding_flag(flag);
	}
}

int
taru_set_tar_header_policy(TARU * taru, char * user_format, int * p_arf_format)
{
	int xx;
	int * format;
	if (!p_arf_format)
		format = &xx;
	else
		format = p_arf_format;
	if (!strcmp(user_format,"ustar")) {
		/*
		 * Default POSIX ustar format
		 * GNU tar-1.15.1 --format=ustar
		 */
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUTAR, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_LONG_LINKS, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_PAX, 0);
  		*format=arf_ustar;
	} else if (
		!strcmp(user_format,"gnu") ||
		!strcmp(user_format,"gnutar") ||
		0
	) {
		/*
		 * same as GNU tar 1.15.1 --format=gnu
		 */
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUPOSIX, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUTAR, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_PAX, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_LONG_LINKS, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_BLOCKSIZE_B1, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_GNUTAR, 1);
  		*format=arf_ustar;
	} else if (!strcmp(user_format,"ustar.star")) {
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUTAR, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUPOSIX, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_PAX, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_STAR, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_BLOCKSIZE_B1, 1);
  		*format=arf_ustar;
	} else if (
		!strcmp(user_format,"oldgnu") ||
		0
	) {
		/*
		 * GNU tar-1.13.25 -b1  // default compilation
		 */
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUTAR, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_LONG_LINKS, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_BLOCKSIZE_B1, 1);
  		*format=arf_ustar;
	} else if (!strcmp(user_format,"ustar0")) {
		/*
		 * GNU tar-1.13.25 --posix -b1
		 */
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUTAR, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_LONG_LINKS, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_PAX, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_BLOCKSIZE_B1, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUPOSIX, 1);
  		*format=arf_ustar;
	} else if (!strcmp(user_format,"bsdpax3")) {
		/*
		 * Emulate /bin/pax ustar format as found of some GNU and BSD systems.
		 */
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_BLOCKSIZE_B1, 1);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_PAX, 0);
  		*format=arf_ustar;
	} else if (!strcmp(user_format,"newc")) {
		*format=arf_newascii;
	} else if (!strcmp(user_format,"crc")) {
  		*format=arf_crcascii;
	} else if (!strcmp(user_format,"odc")) {
 		*format=arf_oldascii;
	} else if (!strcmp(user_format,"pax") || !strcmp(user_format,"posix")) {
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUPOSIX, 0);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_OLDGNUTAR, 0 /* Set off */);
		taru_set_tarheader_flag(taru, TARU_TAR_GNU_LONG_LINKS, 0 /* Set off */);
		taru_set_tarheader_flag(taru, TARU_TAR_BE_LIKE_PAX, 0 /* Set off */);
		taru_set_tarheader_flag(taru, TARU_TAR_PAXEXTHDR, 1 /*Set on*/);
  		*format=arf_ustar;
	} else {
		fprintf (stderr,"%s: unrecognized format: %s\n", swlib_utilname_get(), user_format);
		return -1;
	}
	return 0;
}

int
taru_check_devno_for_tar(struct new_cpio_header * file_hdr)
{
	if (file_hdr->c_rdev_maj > 2097151)  /* & 07777777 != file_hdr->c_rdev_maj) */
		return 1;
	if (file_hdr->c_rdev_min > 2097151)  /* & 07777777 != file_hdr->c_rdev_min) */
		return 2;
	return 0;
}

int
taru_check_devno_for_system(struct new_cpio_header * file_hdr)
{
	int d = makedev(file_hdr->c_rdev_maj, file_hdr->c_rdev_min);
	if ((int)(file_hdr->c_rdev_maj) != (int)minor(d))
		return 1;
	if ((int)(file_hdr->c_rdev_min) != (int)minor(d))
		return 2;
	return 0;
}
