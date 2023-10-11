/* uxformat.h - read/write tar and cpio archives.
 */

/*
 * Copyright (C) 1998  Jim Lowe <jhlowe@acm.org>
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#ifndef uxformat_19980415_impl_h
#define uxformat_19980415_impl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

extern "C" {
#include "xformat.h"
}

class uxFormat
   {
     private:
	XFORMAT * xxformatM;

     protected:
	uxFormat (int ifd, int ofd, int format)
	{
		xxformatM = xformat_open(ifd, ofd, format);
	}

     public:
	
	virtual ~uxFormat (void) {
		xformat_close(xxformatM);
	}
	
	uxFormat(void)
	{
		xxformatM = xformat_open(STDIN_FILENO, STDOUT_FILENO, arf_ustar);    
	}

	uxFormat (int ifd, int ofd)
	{
		xxformatM = xformat_open(ifd, ofd, arf_ustar);    
	}

	struct new_cpio_header * xFormat_vfile_hdr() {
		return xformat_vfile_hdr(xxformatM);
	}

	void xFormat_init_vfile_hdr(void) {
		xformat_init_vfile_header(xxformatM);
	}

	AHS * uxFormat_hdr(void) {
		return xformat_ahs_object(xxformatM);
	}
	
	void xFormat_set_xformat(XFORMAT * xformat) {
		xxformatM = xformat;
	}

	XFORMAT * xFormat_get_xformat(void) {
		return xxformatM;
	}

	char * xFormat_get_header_buffer (char * buf) {
		return xformat_get_header_buffer(xxformatM, buf);
	}

	void xFormat_set_tar_chksum(void) { 
		xformat_set_tar_chksum(xxformatM);
	}
	
	void xFormat_set_format(int format) {
		xformat_set_format(xxformatM, format);
	}
	
	void xFormat_set_output_format(int format) {
		xformat_set_output_format(xxformatM, format);
	}

	void xFormat_set_mode (mode_t mode) { 
		xformat_set_mode(xxformatM,  mode);
	}    
	
	int xFormat_set_user_systempair(char * name) { 
		return xformat_set_user_systempair(xxformatM, name);
	}

	int xFormat_set_group_systempair(char * name) { 
		return xformat_set_group_systempair(xxformatM, name);
	}
	
	void xFormat_set_perms (mode_t mode) { 
		xformat_set_perms(xxformatM,  mode);
	}    

	void xFormat_set_filetype_from_tartype (char  s) { 
	        xformat_set_filetype_from_tartype(xxformatM, s);
	}    

	void xFormat_set_uid (uid_t uid) {
	        xformat_set_uid(xxformatM, uid);
	}    

	void xFormat_set_uid (char * username) {
		xformat_set_uid_by_name(xxformatM, username);
	}    

	void xFormat_set_gid (char * groupname) { 
		xformat_set_gid_by_name(xxformatM, groupname);
	}    

	void xFormat_set_gid (gid_t gid) { 
	        xformat_set_gid(xxformatM, gid);
	}    
  
	void xFormat_set_filesize (intmax_t filesize) { 
	        xformat_set_filesize(xxformatM, filesize);
	}    

	void xFormat_set_nlink (int  nlink) { 
	        xformat_set_nlink(xxformatM, nlink);
	}    
	
	void xFormat_set_inode(ino_t i) { 
	        xformat_set_inode(xxformatM, i);
	}    

	void xFormat_set_mtime (time_t mtime) { 
	        xformat_set_mtime(xxformatM, mtime);
	}    

	void xFormat_set_devmajor (dev_t dev) { 
	        xformat_set_devmajor(xxformatM, dev);
	}    
    
	void xFormat_set_devminor (dev_t dev) { 
	        xformat_set_devminor(xxformatM, dev);
	}    

	void xFormat_set_name (char *name) { 
	        xformat_set_name(xxformatM, name);
	}
    
	void xFormat_set_linkname (char * linkname) { 
		xformat_set_linkname(xxformatM, linkname);
	}

	int xFormat_set_virtual_eof(size_t len) { 
		return xformat_set_virtual_eof(xxformatM, len);
	}
	
	int xFormat_set_to_statbuf(struct stat * st) { 
		return xformat_set_to_statbuf(xxformatM, st);
	}

	int xFormat_set_from_statbuf(char * path) { 
		return xformat_set_from_statbuf_path(xxformatM, path);
	}

	int xFormat_set_from_statbuf(int fd) { 
		return xformat_set_from_statbuf_fd(xxformatM, fd);
	}

	void xFormat_set_from_statbuf(struct stat *st) { 
		xformat_set_from_statbuf(xxformatM, st);
	}

	void xFormat_set_ofd(int fd) { 
		xformat_set_ofd(xxformatM, fd);
	}

	void xFormat_set_ifd(int fd) { 
		xformat_set_ifd(xxformatM, fd);
	}

	int  xFormat_get_ofd(void) { 
		return xformat_get_ofd(xxformatM);
	}
	
	void * xFormat_get_swvarfs(void) { 
		return (void*)xformat_get_swvarfs(xxformatM);
	}

	int  xFormat_get_ifd(void) { 
		return xformat_get_ifd(xxformatM);
	}
	
	void xFormat_set_pass_fd(int fd) { 
		return xformat_set_pass_fd(xxformatM, fd);
	}
	
	int xFormat_get_pass_fd(void) { 
		return  xformat_get_pass_fd(xxformatM);
	}
	
	char * xFormat_get_next_dirent(struct stat *st) { 
		return xformat_get_next_dirent(xxformatM, st);
	}
	
	int xFormat_setdir(char * path) {
		return xformat_setdir(xxformatM, path);
	}

	int xFormat_clear_pass_buffer(void) { 
		return xformat_clear_pass_buffer(xxformatM);
	}
	
	intmax_t xFormat_write_file(void) {
		return xformat_write_file(xxformatM, static_cast<struct stat *>(NULL), 
			static_cast<char *>(NULL),
			static_cast<char *>(NULL)
			);
	}
	
	intmax_t xFormat_write_file(char * name, char * source) {
		return xformat_write_file(xxformatM, static_cast<struct stat *>(NULL), name, source);
	}

	intmax_t xFormat_write_file(char * name) {
		return xformat_write_file(xxformatM, static_cast<struct stat *>(NULL), name, name);
	}

	intmax_t xFormat_write_file(int source_fd) {
		return xformat_write_file_by_fd(xxformatM, static_cast<struct stat *>(NULL), (char*)(NULL), (int(*)(int))(NULL), source_fd);
	}

	intmax_t xFormat_write_file(int (*fout)(int)) {
		return xformat_write_file_by_fd(xxformatM, static_cast<struct stat *>(NULL), (char*)(NULL), (int(*)(int))(fout), -1);
	}

	intmax_t xFormat_write_file(struct stat * st, char * name) {
	        return xformat_write_file(xxformatM, st, name, NULL);
	}

	intmax_t xFormat_write_file(struct stat *t, char * name , int source_fd) {
		return xformat_write_file_by_fd(xxformatM, t, name, (int(*)(int))(NULL), source_fd);
	}
	
	intmax_t xFormat_write_file(struct stat *t, char * name , int (*fout)(int)) {
		return xformat_write_file_by_fd(xxformatM, t, name, fout, -1);
	}

	intmax_t xFormat_write_file (struct stat *t, char * name, char * source) {
		return xformat_write_file(xxformatM, t, name, source);
	}
	
	int xFormat_write_header(void) {
		return xformat_write_header(xxformatM);
	}
	
	int xFormat_write_header(char * name) {
		return xformat_write_header_wn(xxformatM, name);
	}
	
	intmax_t xFormat_read_file_data(int dst_fd) {
		return xformat_read_file_data(xxformatM, dst_fd);
	}

	intmax_t xFormat_write_file_data(int source_fd) {
		return xformat_write_file_data(xxformatM, source_fd);
	}

	int xFormat_write_trailer (void) {
		return xformat_write_trailer(xxformatM);
	}

	int xFormat_u_open_file(char * name) {
		return xformat_u_open_file(xxformatM, name);
	}

	int xFormat_u_lstat(char * path, struct stat * st) {
		return xformat_u_lstat(xxformatM, path, st);
	}

	int xFormat_u_fstat(int fd, struct stat * st) {
		return xformat_u_fstat(xxformatM, fd, st);
	}

	int xFormat_u_readlink(char * path, char * buf, size_t bufsize) {
		return xformat_u_readlink(xxformatM, path, buf, bufsize);
	}

	int xFormat_u_close_file(int fd) {
		return xformat_u_close_file(xxformatM, fd);
	}

	int xFormat_open_archive(char * dirname) {
		//
		// Open directory.
		//
		return  xformat_open_archive_dirfile(xxformatM, dirname, (int)0, (mode_t)0);
	}

	int xFormat_open_archive(int fd) {
		//
		// Open serial access archive file.
		//
		return xformat_open_archive_by_fd(xxformatM, fd, UINFILE_DETECT_FORCEUXFIOFD, (mode_t)0);
	}
	
	int xFormat_open_archive(SWVARFS * swvarfs) {
		return xformat_open_archive_by_swvarfs(xxformatM, swvarfs);
	}

	int xFormat_open_archive(int fd, int flags) {
		//
		// Open serial access archive file.
		//
		return xformat_open_archive_by_fd(xxformatM, fd, flags, (mode_t)0);
	}
	
	int xFormat_open_archive(int fd, int flags, mode_t mode) {
		//
		// Open serial access archive file on descriptor fd.
		//
		return xformat_open_archive_by_fd(xxformatM, fd, flags, mode);
	}
	
	int xFormat_open_archive(char * pathname, int flags) {
		//
		// Open portable archive file.
		//
		return xformat_open_archive_regfile(xxformatM, pathname, flags, 0);
	}
	
	int xFormat_open_archive(char * pathname, int flags, mode_t mode) {
		//
		// Open package or directory file.
		//
		return xformat_open_archive(xxformatM, pathname, flags, mode);
	}
	
	int xFormat_close_archive(void) {
		return  xformat_close_archive(xxformatM);
	}

	int xFormat_read_header(void){
		return xformat_read_header(xxformatM);
	}
	
	int xFormat_unread_header(void){
		return xformat_unread_header(xxformatM);
	}

	int xFormat_read (void * buf, size_t count) {
		return xformat_read(xxformatM, buf, count);
	}

	intmax_t xFormat_copy_pass(void) {
		return xformat_copy_pass_thru(xxformatM);
	}

	intmax_t xFormat_copy_pass (int dst_fd) {
		return xformat_copy_pass_by_dst(xxformatM, dst_fd);
	}

	intmax_t xFormat_copy_pass (int dst_fd, int src_fd) {
		return  xformat_copy_pass(xxformatM, dst_fd, src_fd);
	}

	char xFormat_get_tar_typeflag (void) {
	  	return xformat_get_tar_typeflag(xxformatM);
	}
	
	int xFormat_file_has_data(void) {
	  	return xformat_file_has_data(xxformatM);
	}

	unsigned xFormat_get_tar_chksum (void * tarhdr) {
		return xformat_get_tar_chksum(xxformatM, tarhdr);
	}

	char* xFormat_get_username (char * buf) {
		return xformat_get_username(xxformatM, buf);
	}
	
	void xFormat_set_sys_db_u_policy(int c) {
		return xformat_set_sys_db_u_policy(xxformatM, c);
	}
	
	void xFormat_set_sys_db_g_policy(int c) {
		return xformat_set_sys_db_g_policy(xxformatM, c);
	}

	char* xFormat_get_groupname (char * buf) {
		return xformat_get_groupname(xxformatM, buf);
	}
	
	char* xFormat_get_tar_username(void) {
		return xformat_get_tar_username(xxformatM);
	}

	char* xFormat_get_tar_groupname(void) {
		return xformat_get_tar_groupname(xxformatM);
	}
	
	void xFormat_set_groupname (char * name) {
		return xformat_set_groupname(xxformatM, name);
	}
	
	void xFormat_set_username (char * name) {
		return xformat_set_username(xxformatM, name);
	}

	size_t xFormat_get_filesize (void) {
		return xformat_get_filesize(xxformatM);
	}

	time_t xFormat_get_mtime (void) {
		return xformat_get_mtime(xxformatM);
	}
	

	char * xFormat_get_name (STROB * buf) {
	   	return xformat_get_name(xxformatM, buf);
	}

	char * xFormat_get_linkname (char * buf) {
	   	return xformat_get_linkname(xxformatM, buf);
	}

	mode_t xFormat_get_mode (void){
	   	return xformat_get_mode(xxformatM);
	}
	
	mode_t xFormat_get_perms (void){
	   	return xformat_get_perms(xxformatM);
	}

	int xFormat_get_virtual_eof(void){
		return xformat_get_virtual_eof(xxformatM);
	}

	char * xFormat_get_source_filename (char * buf){
	   	return xformat_get_source_filename(xxformatM, buf);
	}

	int xFormat_is_end_of_archive(void) {
		return xformat_is_end_of_archive(xxformatM);
	}
	
	int xFormat_get_layout_type(void) {
		return xformat_get_layout_type(xxformatM);
	}

	void xFormat_set_false_inodes(int n) {
		xformat_set_false_inodes(xxformatM,  n);
	}
	
	void xFormat_set_strip_leading_slash(int n) {
		xformat_set_strip_leading_slash(xxformatM,  n);
	}
	
	void xFormat_set_numeric_uids(int n) {
		xformat_set_numeric_uids(xxformatM,  n);
	}
	
	void xFormat_set_tarheader_flag(int flag, int n) {
		xformat_set_tarheader_flag(xxformatM,  flag, n);
	}
	
	int xFormat_get_tarheader_flags(void) {
		return xformat_get_tarheader_flags(xxformatM);
	}
	
	//Dvoid xFormat_debug_dump(FILE * fp) {
	//D	xformat_debug_dump(xxformatM,  fp);
	//D}

	void xFormat_reset_bytes_written(void) {
		xformat_reset_bytes_written(xxformatM);
	}

	void
	xFormat_set_preview_level(int level)
	{ 
		xformat_set_preview_level(xxformatM, level);
	}

	void
	xFormat_set_preview_fd(int fd)
	{ 
		xformat_set_preview_fd(xxformatM, fd);
	}

	int
	xFormat_get_preview_fd(void)
	{ 
		return xformat_get_preview_fd(xxformatM);
	}

	// char * uxFormat_dump_string_s(char * prefix);
};
#endif
