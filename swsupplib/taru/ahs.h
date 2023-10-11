/* ahs.h - Archive header accessor.

   Copyright (C) 1998, 1999 Jim Lowe
 
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

#ifndef ahs_19980415_h
#define ahs_19980415_h

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "strob.h"
#include "cpiohdr.h"

#ifdef HAVE_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#define AHS_USERNAME_NOBODY 	"nobody"
#define AHS_UID_NOBODY 		(07777777)
#define AHS_GROUPNAME_NOBODY 	"nobody"
#define AHS_GID_NOBODY 		(07777777)
#define AHS_ID_NOBODY		AHS_UID_NOBODY

typedef struct {
	struct new_cpio_header * file_hdrM;
} AHS;

struct new_cpio_header * ahsStaticCreateFilehdr(void);
void 			ahsStaticDeleteFilehdr(struct new_cpio_header * file_hdr);
void 	ahsStatic_strip_name_leading_slash(struct new_cpio_header * file_hdr);

void 	ahsStaticSetTarLinknameLength(struct new_cpio_header * file_hdr, int len);
void 	ahsStaticSetTarFilenameLength(struct new_cpio_header * file_hdr, int len);

char * 	ahsStaticGetTarLinkname(struct new_cpio_header * file_hdr);
char * 	ahsStaticGetTarFilename(struct new_cpio_header * file_hdr);
char * 	ahsStaticGetTarGroupname(struct new_cpio_header * file_hdr);
char * 	ahsStaticGetTarUsername(struct new_cpio_header * file_hdr);

void 	ahsStaticSetPaxLinkname(struct new_cpio_header * file_hdr, char * name);
void 	ahsStaticSetTarLinkname(struct new_cpio_header * file_hdr, char * name);
void 	ahsStaticSetTarFilename(struct new_cpio_header * file_hdr, char * name);
void 	ahsStaticSetTarUsername(struct new_cpio_header * file_hdr, char * name);
void 	ahsStaticSetTarGroupname(struct new_cpio_header * file_hdr, char * name);

void ahs_close(AHS * xhs);

struct new_cpio_header * ahs_vfile_hdr(AHS * ahs);

AHS * ahs_open(void);

void ahs_init_header(AHS * ahs);
	
char *  ahs_get_header_buffer(char * buf);

void ahs_set_tar_chksum(void);
	
void ahs_set_mode(AHS * xhs, mode_t mode);

void ahs_set_perms(AHS * xhs, mode_t perms);

mode_t ahs_get_perms(AHS * xhs);

void ahs_set_filetype_from_tartype(AHS * xhs, char  s);

void ahs_set_uid(AHS * xhs, uid_t uid);

void ahs_set_uid_by_name(AHS * xhs, char * username);
	
void ahs_set_gid_by_name(AHS * xhs, char * groupname);

void ahs_set_gid(AHS * xhs, gid_t gid);

void ahs_set_sys_db_u_policy(AHS * xhs, int c);

void ahs_set_sys_db_g_policy(AHS * xhs, int c);

int ahs_set_user_systempair(AHS * xhs, char * name);

int ahs_set_group_systempair(AHS * xhs, char * name);

void ahs_set_tar_username(AHS * xhs, char * name);

void ahs_set_tar_groupname(AHS * xhs, char * name);

char * ahs_get_tar_username(AHS * xhs);

char * ahs_get_tar_groupname(AHS * xhs);
  
void ahs_set_filesize(AHS * xhs, intmax_t filesize);

void ahs_set_nlink(AHS * xhs, int  nlink);

void ahs_set_inode(AHS * xhs, ino_t ino);

void ahs_set_mtime(AHS * xhs, time_t mtime);
	
void ahs_set_devmajor(AHS * xhs, dev_t dev);
    
void ahs_set_devminor(AHS * xhs, dev_t dev);

void ahs_set_name(AHS * xhs, char *name);
    
void ahs_set_linkname(AHS * xhs, char *linkname);

void ahs_set_from_statbuf(AHS * xhs, struct stat *st);

void ahs_set_to_statbuf(AHS * xhs, struct stat *st);
	
void ahs_set_from_new_cpio_header(AHS * xhs, void *vfh);
	
void * ahs_get_new_cpio_header(AHS * xhs);
	
char ahs_get_tar_typeflag(AHS * xhs);
	
unsigned ahs_get_tar_chksum(AHS * xhs, void * tarhdr);
	
char* ahs_get_system_username(AHS * xhs, char * buf);
	
char* ahs_get_system_groupname(AHS * xhs, char * buf);
	
intmax_t ahs_get_filesize(AHS * xhs);

time_t ahs_get_mtime(AHS * xhs);

char * ahs_get_name(AHS * xhs, STROB * buf);

char * ahs_get_linkname(AHS * xhs, char * buf);

mode_t ahs_get_mode(AHS * xhs);

char * ahs_get_source_filename(AHS * xhs, char * buf);

int ahs_copy(AHS * ahs_to, AHS * ahs_from);

/*D unsigned long ahs_debug_dump(AHS * xhs, FILE *fp); */
/*D char * ahs_dump_string_s(AHS * xhs, char * prefix); */

#endif
