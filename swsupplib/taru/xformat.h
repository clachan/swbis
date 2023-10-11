/* xformat.h - read/write archives
 
   Copyright (C) 2000  Jim Lowe

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

#ifndef xformat_20001001_jhl_h
#define xformat_20001001_jhl_h

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "uxfio.h"
#include "strob.h"
#include "swvarfs.h"
#include "hllist.h"
#include "taru.h"
#include "defer.h"
#include "porinode.h"
#include "ahs.h"
#include "taruib.h"
#include "uinfile.h"


#define XFORMAT_OFF 0
#define XFORMAT_ON  1

typedef struct {
	int ifdM; 
	int ofdM; 
	enum archive_format format_codeM;
	enum archive_format output_format_codeM;

	int eoaM;
	uintmax_t bytes_writtenM;
	int make_false_inodesM;
	SWVARFS * swvarfsM;
	AHS * ahsM;
	HLLIST *link_recordM;
	DEFER * deferM; 
	PORINODE * use_false_inodesM;
	PORINODE * porinodeM;
	int trailer_bytesM;
	int swvarfs_is_externalM;
	int last_header_sizeM;
	TARU * taruM;
	struct stat lt2M;
	STROB * name2M;
	STROB * linkname2M;
} XFORMAT;

	int xformat_close(XFORMAT * xux);
	
	XFORMAT * xformat_open(int ifd, int ofd, int format);

	struct new_cpio_header * xformat_vfile_hdr(XFORMAT * xux);

	void xformat_init_vfile_header(XFORMAT * xux);

	AHS * xformat_ahs_object(XFORMAT * xux);

	char * xformat_get_header_buffer(XFORMAT * xux, char * buf);

	void xformat_set_tar_chksum(XFORMAT * xux);
	
	void xformat_set_format(XFORMAT * xux, int format);

	int  xformat_get_format(XFORMAT * xux);
	
	TARU * xformat_get_taru_object(XFORMAT * xux);
	
	void xformat_set_output_format(XFORMAT * xux, int format);

	int  xformat_get_output_format(XFORMAT * xux);

	void xformat_set_mode(XFORMAT * xux, mode_t mode);
	
	void xformat_set_perms(XFORMAT * xux, mode_t mode);

	void xformat_set_filetype_from_tartype (XFORMAT * xux, char s); 

	void xformat_set_uid(XFORMAT * xux, uid_t uid);

	int xformat_set_user_systempair(XFORMAT * xux, char * name);
	
	int xformat_set_group_systempair(XFORMAT * xux, char * name);

	void xformat_set_username (XFORMAT * xux, char * name);
	
	void xformat_set_groupname (XFORMAT * xux, char * name);
	
	void xformat_set_sys_db_u_policy(XFORMAT * xux, int c);
	
	void xformat_set_sys_db_g_policy(XFORMAT * xux, int c);

	void xformat_set_uid_by_name (XFORMAT * xux, char * username); 

	void xformat_set_gid_by_name (XFORMAT * xux, char * groupname); 

	void xformat_set_gid(XFORMAT * xux, gid_t gid ); 
  
	void xformat_set_filesize(XFORMAT * xux, intmax_t filesize ); 

	void xformat_set_nlink(XFORMAT * xux, int  nlink);
	
	void xformat_set_inode(XFORMAT * xux, ino_t ino);

	void xformat_set_mtime(XFORMAT * xux, time_t mtime );

	void xformat_set_devmajor(XFORMAT * xux, dev_t dev );
    
	void xformat_set_devminor(XFORMAT * xux, dev_t dev );

	void xformat_set_name (XFORMAT * xux, char *name ); 
    
	void xformat_set_linkname (XFORMAT * xux, char * linkname ); 

	int xformat_set_virtual_eof (XFORMAT * xux, size_t len );
	
	int xformat_set_to_statbuf(XFORMAT * xux, struct stat *st);

	int xformat_set_from_statbuf_path(XFORMAT * xux, char * path);

	int xformat_set_from_statbuf_fd(XFORMAT * xux, int fd);

	void xformat_set_from_statbuf(XFORMAT * xux, struct stat *st);

	void xformat_set_ofd(XFORMAT * xux, int fd);

	void xformat_set_ifd(XFORMAT * xux, int fd);

	void xformat_decrement_bytes_written(XFORMAT * xux, int amount);

	SWVARFS * xformat_get_swvarfs(XFORMAT * xux);

	int  xformat_get_ofd(XFORMAT * xux);

	int  xformat_get_ifd(XFORMAT * xux); 
	
	void xformat_set_pass_fd(XFORMAT * xux, int fd); 
	
	int xformat_get_pass_fd(XFORMAT * xux);
	
	int xformat_get_tarheader_flags(XFORMAT * xux);
	
	char * xformat_get_next_dirent(XFORMAT * xux, struct stat * st);
	
	int xformat_setdir(XFORMAT * xux, char * path);
	
	int xformat_clear_pass_buffer(XFORMAT * xux); 
	
	intmax_t xformat_write_file_data(XFORMAT * xux, int source_fd);
	
	intmax_t xformat_read_file_data(XFORMAT * xux, int dst_fd);

	int xformat_write_header(XFORMAT * xux);

	int xformat_write_header_wn(XFORMAT * xux, char * name);
	
	intmax_t xformat_write_by_name(XFORMAT * xux, char * name, struct stat *st);

	intmax_t xformat_write_by_fd(XFORMAT * xux, int srcfd, struct new_cpio_header * file_hdr);
	
	intmax_t xformat_write_file_by_fd(XFORMAT * xux, struct stat *t, char * name, int(*)(int), int source_fd);
	
	intmax_t xformat_write_file(XFORMAT * xux, struct stat *t, char * name, char * source);

	int xformat_write_trailer(XFORMAT * xux);
	
	void xformat_write_archive_stats(XFORMAT * xux, char * name, int fd);

	int xformat_u_open_file(XFORMAT * xux, char * name);

	int xformat_u_lstat(XFORMAT * xux, char * path, struct stat * st);

	int xformat_u_fstat(XFORMAT * xux, int fd, struct stat * st);

	int xformat_u_readlink(XFORMAT * xux, char * path, char * buf, size_t bufsize);

	int xformat_u_close_file(XFORMAT * xux, int fd);

	int xformat_open_archive(XFORMAT * xux, char * dirname, int flags, mode_t mode);
	
	int xformat_open_archive_dirfile(XFORMAT * xux, char * pathname, int flags, mode_t mode);

	int xformat_open_archive_regfile(XFORMAT * xux, char * pathname, int flags, mode_t mode);

	int xformat_close_archive(XFORMAT * xux);
	
	int xformat_open_archive_by_swvarfs(XFORMAT * xux, SWVARFS * sf);

	int xformat_open_archive_by_fd(XFORMAT * xux, int fd, int flags, mode_t mode);
	
	int xformat_open_archive_by_fd_and_name(XFORMAT * xux, int fd, int flags, mode_t mode, char * name);

	int xformat_read_header(XFORMAT * xux);

	int xformat_unread_header(XFORMAT * xux);

	int xformat_read (XFORMAT * xux, void * buf, size_t count); 

	intmax_t xformat_copy_pass_thru(XFORMAT * xux); 

	intmax_t xformat_copy_pass_by_dst(XFORMAT * xux, int dst_fd);

	intmax_t xformat_copy_pass_digs(XFORMAT * xux, int dst_fd, int src_fd, FILE_DIGS * digs);
	
	intmax_t xformat_copy_pass(XFORMAT * xux, int dst_fd, int src_fd);

	intmax_t xformat_copy_pass_md5(XFORMAT * xux, int dst_fd, int src_fd, char * md5buf);
	
	intmax_t xformat_copy_pass_file_data(XFORMAT * xux, int dst_fd, int src_fd);

	intmax_t xformat_copy_pass2 (XFORMAT * xux, int dst_fd, int src_fd, int adjunct_ofd);

	char xformat_get_tar_typeflag (XFORMAT * xux);
	
	int xformat_file_has_data(XFORMAT * xux);

	unsigned xformat_get_tar_chksum (XFORMAT * xux, void * tarhdr);

	char* xformat_get_username (XFORMAT * xux, char * buf);

	char* xformat_get_groupname (XFORMAT * xux, char * buf);
	
	char* xformat_get_tar_username (XFORMAT * xux);

	char* xformat_get_tar_groupname (XFORMAT * xux);

	intmax_t xformat_get_filesize (XFORMAT * xux);

	time_t xformat_get_mtime (XFORMAT * xux);
	
	char * xformat_get_name (XFORMAT * xux, STROB * buf);

	char * xformat_get_linkname (XFORMAT * xux, char * buf);

	mode_t xformat_get_perms (XFORMAT * xux);
	
	mode_t xformat_get_mode (XFORMAT * xux);

	int xformat_get_virtual_eof (XFORMAT * xux);

	char * xformat_get_source_filename (XFORMAT * xux, char * buf);

	int xformat_is_end_of_archive(XFORMAT * xux);
	
	int xformat_get_layout_type(XFORMAT * xux);

	void xformat_set_false_inodes(XFORMAT * xux, int n);

        void xformat_set_strip_leading_slash(XFORMAT * xux, int n);
        
	void xformat_set_numeric_uids(XFORMAT * xux, int on_off);

	void xformat_set_tarheader_flag(XFORMAT * xux, int flag, int n);

	void xformat_set_tarheader_flags(XFORMAT * xux, int flags);

	/*D void xformat_debug_dump(XFORMAT * xux, FILE * fp); */
	
	void xformat_reset_bytes_written(XFORMAT * xux);

	/*D char * xformat_dump_string_s(XFORMAT * uin, char * prefix); */

	int xformat_get_preview_fd(XFORMAT *);
	void xformat_set_preview_fd(XFORMAT *, int);
	void xformat_set_preview_level(XFORMAT *, int);

#endif
