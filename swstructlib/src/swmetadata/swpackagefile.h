/* swpackagefile.h
 */

/*
 * Copyright (C) 2003-2004  James H. Lowe, Jr.  <jhlowe@acm.org>
 * All Rights Reserved.
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

#ifndef swpackagefile_19980601jhl_h
#define swpackagefile_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <tar.h>
#include <time.h>
#include "swattribute.h"
#include "swdefinition.h"
#include "swstructdef.h"
#include "swattributemem.h"
#include "portablearchive.h"
#include "swpathname.h"

extern "C" {
#include "taru.h"
#include "swlib.h"
#include "swheaderline.h"
#include "md5.h"
#include "ahs.h"
#include "debug_config.h"
}

// #define SWPACKAGEFILENEEDDEBUG 1
#undef SWPACKAGEFILENEEDDEBUG 

#ifdef SWPACKAGEFILENEEDDEBUG
#define SWPACKAGEFILE_DEBUG(format) SWBISERROR("SWPACKAGEFILE DEBUG: ", format)
#define SWPACKAGEFILE_DEBUG2(format, arg) SWBISERROR2("SWPACKAGEFILE DEBUG: ", format, arg)
#define SWPACKAGEFILE_DEBUG3(format, arg, arg1) SWBISERROR3("SWPACKAGEFILE DEBUG: ", format, arg, arg1)
#else
#define SWPACKAGEFILE_DEBUG(arg)
#define SWPACKAGEFILE_DEBUG2(arg, arg1)
#define SWPACKAGEFILE_DEBUG3(arg, arg1, arg2)
#endif 

class swPackageFile :  public swAttribute
{
	class Initor {
		public:
			Initor(char * packagefilename, int flags){
				//
				// Open an existing archive.
				//
				SWPACKAGEFILE_DEBUG("In Initor(char * packagefilename)");
				if (!use_count_){
					SWPACKAGEFILE_DEBUG("use_count=0");
					delete uxformatM;
					did_create_ = 1;
					uxformatM=new portableArchive(packagefilename, flags);
				}
				use_count_++;				
			}
			Initor(int ifd, int ofd){
				SWPACKAGEFILE_DEBUG("In Initor(int ifd, int ofd)");
				if (!use_count_){
					SWPACKAGEFILE_DEBUG("use_count=0");
					delete uxformatM;
					did_create_ = 1;
					uxformatM=new portableArchive(ifd, ofd);
				}
				use_count_++;				
			}
			Initor(uxFormat * pkg){
				SWPACKAGEFILE_DEBUG("In Initor(uxFormat * pkg)");
				if (!use_count_){
					SWPACKAGEFILE_DEBUG("use_count=0");
					delete uxformatM;
					uxformatM= pkg;
				}
				use_count_++;				
			}
			Initor(void){
				SWPACKAGEFILE_DEBUG("In Initor(void)");
				if (!use_count_){
					SWPACKAGEFILE_DEBUG("use_count=0");
					delete uxformatM;
					uxformatM=new portableArchive();
					did_create_ = 1;
				}
				use_count_++;				
			}

			~Initor(void) {
				SWPACKAGEFILE_DEBUG("In ~Initor(void)");
				//fprintf(stderr, "use_count is %d\n", use_count_);
				if (use_count_) use_count_--;
				if (!use_count_) {
					SWPACKAGEFILE_DEBUG("In ~Initor(void) use_count=0");
					SWPACKAGEFILE_DEBUG("here");
					swPackageFile::finalize(did_create_);
					SWPACKAGEFILE_DEBUG("here0");
					SWPACKAGEFILE_DEBUG("here1");
				}
			}
		private:
			static int use_count_;
			static int did_create_;
	};
	Initor * initor_;
	friend class Initor;
  	static STROB * tmpM;	
  	static uxFormat * uxformatM;	// The archive package, or the source package or source file system.
	
	int is_ieee_layoutM;		// Non zero if package is IEEE 1387.2 layout.
	STROB *  	user_path_strob_buf_;	
  	swPathName      * swpathM; 
  	int		fdM;		// source file descriptor.

  public:
	static mode_t default_tar_mode_dir;
	static mode_t default_tar_mode_regfile;
	
	swPackageFile (void): swAttribute("", "") {
		initor_ = new Initor();
		init_(); 
	}

	swPackageFile (uxFormat * pkg): swAttribute("", "") {
		SWPACKAGEFILE_DEBUG("swPackageFile");
		initor_ = new Initor(pkg);
		init_(); 
	}

	swPackageFile (int ifd, int ofd): swAttribute("", "") {
		SWPACKAGEFILE_DEBUG("swPackageFile");
		initor_ = new Initor(ifd, ofd);
		init_(); 
	}
	
	swPackageFile (char * pkg_filename, int flags): swAttribute(pkg_filename, pkg_filename) {
		//
		// Open package file with flags.
		//
		SWPACKAGEFILE_DEBUG("swPackageFile");
		initor_ = new Initor();
		init_(); 
	}

	swPackageFile (char * pkg_filename, char * src_filename): swAttribute(pkg_filename, src_filename) {
		SWPACKAGEFILE_DEBUG("swPackageFile");
		initor_ = new Initor();
		init_(); 
	}

	virtual ~swPackageFile (void){ 
		un_init_();
	}

    	int apply_file_stats(swDefinition * swdef, SWVARFS * swvarfs,
					char *source, int cksumflags);

	int initialize_file_stats_array(swDefinition * swdef,
				char array[][file_permsLength],
					int array_is_set[]); 
	
	int check_no_stat_optimization(swDefinition * swdef,
				char array[][file_permsLength],
					int array_is_set[]); 

	virtual int add (char * keyword, char * value, swMetaData * newnode, int level) {
		int c;
		int off=uxfio_lseek(get_mem_fd(), 0, SEEK_END);
	
		newnode->set_level(level);
		newnode->set_p_offset(off);
		newnode->set_ino(off);
		c = SWPARSE_MD_TYPE_ATT;
		::swparse_write_attribute_att(get_mem_fd(), keyword, value, level, SWPARSE_FORM_MKUP_LEN);
		newnode->set_type (c);
		if (newnode != this) list_add(newnode);
			return 0;
	}

	
	// This Function overrides and should operate on the length not a '\n'.
	int get_next_line_offset (void) {
		int offset, amount;
		const int RBLK=512; 
		char  buf[RBLK+2], *p; 
 	 
		offset=uxfio_lseek (get_mem_fd(),0,SEEK_CUR);
		buf[RBLK]='\0'; 
   	 
		// Now read to end of line. 
		while ((amount=uxfio_sfread ( get_mem_fd(), buf, RBLK)) > 0) {
			p=strchr (buf, (int)('\n'));
			if (p) {
				// seek back to start of next line 
				uxfio_lseek (get_mem_fd(), (int)(p-buf) - amount + 1, SEEK_CUR);
				return offset;
			}
		}
		return -1;
	}

	virtual int swfile_write_pkg(void) {
		return swfile_write_pkg(static_cast<char*>(NULL));
	}
	
	virtual intmax_t swfile_write_pkg(char * name) {
		int 	source_fd = swfile_open_public_image_fd(name);
		int 	len;
		char * 	val;
	
		if (source_fd < 0) return source_fd;
		len = swfile_get_size(name);
		val = swfile_get_package_filename();	
		swfile_set_default_statbuf();
		if (len < 0) return -1;
		xFormat_set_filesize(len);
		xFormat_set_name(val);
		return xFormat_write_file(source_fd);
	}
	
	virtual void swfile_set_default_statbuf(void) {
		struct stat t;
		struct stat * statbuf = &t;
		time_t tim = time(NULL);
	
		statbuf->st_mode = (mode_t)(0644);
		::taru_set_filetype_from_tartype(REGTYPE, &(statbuf->st_mode), (char*)NULL);	
		statbuf->st_ino = (ino_t)(0);
		statbuf->st_dev = 0;
		statbuf->st_rdev = 0;
		statbuf->st_nlink = (nlink_t)(1);
		statbuf->st_uid = (uid_t)(0);
		statbuf->st_gid = (gid_t)(0);
		statbuf->st_size = (off_t)(0); 
		statbuf->st_atime = (time_t)(tim);
		statbuf->st_ctime = (time_t)(tim);
		statbuf->st_mtime = (time_t)(tim);
		statbuf->st_blksize = (long)(512);
		statbuf->st_blocks = (long)((int) (statbuf->st_size) / 512 + 1);
		xFormat_set_from_statbuf(statbuf);
	}
	
	char * swfile_get_name (STROB * buf) {
		if (!buf) {
			buf = user_path_strob_buf_; 
		}
		return xFormat_get_name(buf);
	}

	virtual int swfile_open_public_image_metadata(void) {
		return 0;
	}
	
	virtual int swfile_open_public_image_fd(void) {
		return swfile_open_public_image_fd(static_cast<char*>(NULL));
	}
	
	virtual int swfile_creat_public_image_fd(void) {
		return -1;
	}
	
	virtual char * swfile_get_filename(void) {
		return get_keyword();	
	}

	virtual int swfile_open_public_image_fd(char * name) {
		if (fdM < 0) {
			if (!name) {
				name = xFormat_get_name(static_cast<STROB*>(NULL));
			} else {
				;
			}
			fdM = xFormat_u_open_file(name);
		} else {
			fprintf(stderr,"usage error in swPackageFile::swfile_open_public_image_fd\n");
		}
		return fdM;
	}
	
	virtual int swfile_close_public_image_fd(void) {
		int ret = xFormat_u_close_file(fdM);
		fdM = -1;
		return ret;
	}
	virtual int swfile_re_close_public_image_fd(void) { return -1; }

	virtual int swfile_re_open_public_image_fd(void) { return -1; }
	
	virtual int swfile_get_size(void) {
		return swfile_get_size(static_cast<char*>(NULL));
	}

	virtual int swfile_get_size(char * name){
		int fd=swfile_open_public_image_fd(name);
		int offset=uxfio_lseek(fd, 0L, SEEK_CUR);
		int filesize=-1;
		if (offset < 0) return -1;
		filesize = ::swlib_pump_amount(-1, fd, -1);
		if (uxfio_lseek(fd, offset, SEEK_SET)) {
			return -1;
		}
		return filesize;
	}
	
	virtual int swfile_get_posix_cksum(unsigned long * crc) {
		return swfile_get_posix_cksum(static_cast<char*>(NULL), crc);
	}

	virtual int swfile_get_posix_cksum(char * name, unsigned long * crc){
		int fd=swfile_open_public_image_fd(name);
		if (fd < 0) 
			return -1;
		*crc=::swlib_cksum(fd);
		return 0;	
	}
	
	virtual char * swfile_get_ascii_md5(char * digest){
		return swfile_get_ascii_md5(static_cast<char*>(NULL), digest);
	}

	virtual char * swfile_get_ascii_md5(char * name, char * digest){
		unsigned char bindigest[17];
		if (!swfile_get_md5(name, static_cast<void*>(bindigest)))
			return static_cast<char*>(NULL);
	
		sprintf(digest, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
				"%02x%02x%02x%02x%02x",
			bindigest[0],  bindigest[1],  bindigest[2],  bindigest[3],
			bindigest[4],  bindigest[5],  bindigest[6],  bindigest[7],
			bindigest[8],  bindigest[9],  bindigest[10], bindigest[11],
			bindigest[12], bindigest[13], bindigest[14], bindigest[15]);
		
		return digest;
	}
	
	virtual void * swfile_get_md5(char * name, void * digest){
		int fd=swfile_open_public_image_fd(name);
		if (fd < 0)
			return static_cast<char*>(NULL);
		if (::swlib_md5(fd, (unsigned char*)digest, 0) == 0) {
			return digest;
		} else {
			return static_cast<char*>(NULL);
		}
	}

	intmax_t write_swdirectory  (char * name) {
		char * pathname = swp_make_name(name, NULL);
		xFormat_set_mtime(time(NULL));
		xFormat_set_mode(default_tar_mode_dir);
		xFormat_set_filetype_from_tartype(DIRTYPE);
		return xFormat_write_file(NULL, pathname, (int)(-1));
	}

	intmax_t swfile_write(char * name, int source_fd) {
		if (! name ) {
			char * pathname = swp_make_name (NULL, NULL);
			return  xFormat_write_file(NULL, pathname, source_fd);
		} else {
			return  xFormat_write_file(NULL, name, source_fd);
		}
	}

	int swfile_read_archive_header (char * buf) {
		int ret=xFormat_read_header();
		if (ret <= 0) return ret; 
		swfile_get_name(NULL);
		if (is_ieee_layoutM && !xFormat_is_end_of_archive()) {
			if (swp_parse_path(xFormat_get_name(NULL)) < 0) {
				fprintf(stderr, "error parsing path [%s].\n", strob_str(user_path_strob_buf_));
				return -1;
			}
		}
		return ret;
	}
	
	/*  Broken. */
	int swfile_read_next_archive_header (void) {
		int ret=-1;
		int pos = uxfio_lseek(xFormat_get_ifd(), 0, SEEK_CUR);
		static int oldpos;
		if (pos - oldpos > 0) { 
			ret=swfile_read_archive_header ((char*)(NULL));
		} 
		else if ((pos - oldpos) == 0){
			ret=xFormat_copy_pass(-1);
			if (ret < 0) {
				oldpos = pos;
				return -1;
			}
			ret=swfile_read_archive_header((char*)(NULL));
		}
		oldpos = pos;
		return ret;
	}

	int swfile_read_next_regfile_header (void) {
		int ret;
		while ( ((ret=swfile_read_next_archive_header())==512) && xFormat_get_tar_typeflag() != '0') { ; }
		if (ret != 512) return -1;    
		return ret;
	}

	int swfile_read_next_catalog_regfile_header (void) {
		int ret;
		while ((ret=swfile_read_next_regfile_header())==512) {
			swfile_get_name(NULL);
			if (!swp_is_catalog(strob_str(user_path_strob_buf_))) return -1;
		}
		return ret-512;
	}

        virtual void setArchiveInclude(int c) { }

        virtual int getArchiveInclude(void) { return 1; }
	
     // ======================================
     // accessor routines. 
     // ======================================

	virtual void setAttributeReferer(swMetaData * att) { }

	virtual swMetaData * getAttributeReferer(void) { return NULL; }

	void	swfile_set_package_filename(char * name) {	
		xFormat_set_name(name);
		set_keyword(name);
	}
	void	swfile_set_source_filename(char * name) {	
		set_value(name);
	}
	char * swfile_get_source_filename(void) {
		return get_value(NULL);
	}
	char * swfile_get_package_filename(void) {
		return get_keyword();
	}

	void swfile_set_swpackage(uxFormat * archive) {
    		if (uxformatM) delete uxformatM;
		uxformatM=archive;
	}
    
     // ===================================
     // Archive file routines.
     // ===================================

	int xFormat_u_lstat(char * path, struct stat * st) {
		return uxformatM->xFormat_u_lstat(path, st);
	}

	int xFormat_u_fstat(int fd, struct stat * st) {
		return uxformatM->xFormat_u_fstat(fd, st);
	}

	int xFormat_u_readlink(char * path, char * buf, size_t bufsize) {
		return uxformatM->xFormat_u_readlink(path, buf, bufsize);
	}

	int xFormat_u_close_file(int fd)  {
	    return uxformatM->xFormat_u_close_file(fd);
	}
	
	int xFormat_u_open_file(char * name) {
	    return uxformatM->xFormat_u_open_file(name);
	}

	int xFormat_open_archive(char * dirname) {
	    return uxformatM->xFormat_open_archive(dirname);
	}
	
	int xFormat_open_archive(char * packagename, int flags, mode_t mode) {
	    return uxformatM->xFormat_open_archive(packagename, flags, mode);
	}

	int xFormat_open_archive(char * packagename, int flags) {
	    return uxformatM->xFormat_open_archive(packagename, flags, SWVARFS_S_IFREG);
	}

	int xFormat_open_archive(int fd, int flags, mode_t mode) {
	    return uxformatM->xFormat_open_archive(fd, flags, mode);
	}
	
	int xFormat_open_archive(int fd, int flags) {
	    int ret;
	    ret =  uxformatM->xFormat_open_archive(fd, flags);
	    if (ret >= 0)
	    	is_ieee_layoutM = (xFormat_get_layout_type() == UINFILE_FILELAYOUT_IEEE);
	    return ret;
	}


	XFORMAT * xFormat_get_xformat(void) {
		return uxformatM->xFormat_get_xformat();
	}

	void xFormat_set_xformat(XFORMAT * xf) {
		uxformatM->xFormat_set_xformat(xf);
	}

	void xFormat_init_vfile_hdr(void) {
	    return uxformatM->xFormat_init_vfile_hdr();
	}

	struct new_cpio_header * xFormat_vfile_hdr(void) {
		return uxformatM->xFormat_vfile_hdr();
	}

	int xFormat_open_archive(int fd) {
	    return uxformatM->xFormat_open_archive(fd);
	}
	
	int xFormat_close_archive(void) {
	    return uxformatM->xFormat_close_archive();
	}
	
	int xFormat_open_archive(SWVARFS * sf) {
	    return uxformatM->xFormat_open_archive(sf);
	}

	char * xFormat_get_header_buffer(char * buf){
	   return uxformatM->xFormat_get_header_buffer(buf);
	}

	//uxFormat * get_xformat(void){
	//  return uxformatM;
	//}

	void xFormat_set_mode (mode_t mode) {
	   uxformatM->xFormat_set_mode (mode);
	}

	void xFormat_set_perms (mode_t mode) {
	   uxformatM->xFormat_set_perms(mode);
	}

	int xFormat_set_user_systempair(char * name) {
		return uxformatM->xFormat_set_user_systempair(name);
	}	       
	       
	int xFormat_set_group_systempair(char * name) {
		return uxformatM->xFormat_set_group_systempair(name);
	}	       

	void xFormat_set_username (char * name) {
	   uxformatM->xFormat_set_username(name);
	}
	
	void xFormat_set_groupname (char * name) {
	   uxformatM->xFormat_set_groupname(name);
	}
	
	void xFormat_set_format (int format) {
	   uxformatM->xFormat_set_format(format);
	}
	
	void xFormat_set_output_format (int format) {
	   uxformatM->xFormat_set_output_format(format);
	}
	
	void xFormat_set_filetype_from_tartype (char s) {
	   uxformatM->xFormat_set_filetype_from_tartype(s);
	}
	
	void xFormat_set_tar_chksum(void){ 
	   uxformatM->xFormat_set_tar_chksum();
	}	

	unsigned xFormat_get_tar_chksum(void * tarhdr){
	   return uxformatM->xFormat_get_tar_chksum(tarhdr);
	}

	void xFormat_set_uid(uid_t uid) {
	   uxformatM->xFormat_set_uid(uid);
	}
	
	void xFormat_set_gid(gid_t gid) {
	   uxformatM->xFormat_set_gid(gid); 
	}
	
	void xFormat_set_gid(char * name) {
	   uxformatM->xFormat_set_gid(name);
	}
	
	void xFormat_set_uid(char * name ) {
	   uxformatM->xFormat_set_uid(name);
	}
	
	void xFormat_set_nlink(int  nlink) {
	    uxformatM->xFormat_set_nlink(nlink);
	}
	
	void xFormat_set_inode(ino_t  inode) {
	    uxformatM->xFormat_set_inode(inode);
	}
	
	void xFormat_set_mtime(time_t mtime) {
	    uxformatM->xFormat_set_mtime(mtime);
	}
	
	void xFormat_set_filesize(intmax_t filesize) {
	      uxformatM->xFormat_set_filesize(filesize);
	}
	
	void xFormat_set_devmajor(dev_t dev) {
	    uxformatM->xFormat_set_devmajor(dev);
	}
	
	void xFormat_set_devminor(dev_t dev) {
	    uxformatM->xFormat_set_devminor(dev);
	}
	
	void xFormat_set_name(char *name) {
	   uxformatM->xFormat_set_name(name);
	}
	
	void xFormat_set_linkname(char *linkname) {
	   uxformatM->xFormat_set_linkname(linkname);
	}
	
	int xFormat_set_virtual_eof(size_t size) {
	   return uxformatM->xFormat_set_virtual_eof (size);
	}
	
	void xFormat_set_ifd (int fd) {
	   uxformatM->xFormat_set_ifd(fd);
	}
	
	void xFormat_set_ofd (int fd) {
	   uxformatM->xFormat_set_ofd(fd);
	}
	
	void xFormat_set_preview_fd (int fd) {
	   uxformatM->xFormat_set_preview_fd(fd);
	}
	
	void xFormat_set_preview_level (int level) {
	   uxformatM->xFormat_set_preview_level(level);
	}
	
	int xFormat_get_preview_fd (void) {
	   return uxformatM->xFormat_get_preview_fd();
	}
	
	void xFormat_set_pass_fd(int fd) {
	   uxformatM->xFormat_set_pass_fd(fd);
	}
	
	int xFormat_get_pass_fd(void) {
	   return uxformatM->xFormat_get_pass_fd();
	}
	
	int xFormat_clear_pass_buffer(void) {
	   return uxformatM->xFormat_clear_pass_buffer();
	}
	
	int xFormat_set_to_statbuf(struct stat * st) {
	   return uxformatM->xFormat_set_to_statbuf(st);
	}
	
	int xFormat_set_from_statbuf(int fd) {
	   return uxformatM->xFormat_set_from_statbuf(fd);
	}
	
	int xFormat_set_from_statbuf(char * pathname) {
	   return uxformatM->xFormat_set_from_statbuf(pathname);
	}
	
	void xFormat_set_from_statbuf(struct stat * st) {
	   uxformatM->xFormat_set_from_statbuf(st);
	}
	
	void *  xFormat_get_swvarfs(void) {
	   return uxformatM->xFormat_get_swvarfs();
	}
	
	int xFormat_get_ifd(void) {
	   return uxformatM->xFormat_get_ifd();
	}
	
	int xFormat_get_ofd(void) {
	   return uxformatM->xFormat_get_ofd();
	}
	
	virtual intmax_t xFormat_write_file(void) {
		return xFormat_write_file((struct stat *)NULL, swfile_get_package_filename());
	}
	
	virtual intmax_t xFormat_write_file(swDefinition * swdef, char * package_path) {
		intmax_t ret;
		char * source;
		char * path;
		char * pstart;
		int no_stat;
		STROB * tmp_control_path;

		path = swdef->find(SW_A_path);
		if (!path) {
			fprintf(stderr, "path attribute is missing\n");
			return -2;
		}

		source = swdef->find(SW_A_source);
		no_stat = swdef->get_no_stat();

		if (!source) {
			fprintf(stderr, "source attribute is missing\n");
			return -3;
		}

		//
		// The no_stat flag is set if this defintion has already been generated (stat'ed)
		// at an earlier point.
		//
		if (no_stat) {
			int source_fd;
			//
			// Use the stats in the swdefinition and transfer them to the archive header.
			//
			
			if (*(swdef->find("type")) == 'f') {
				//
				// This can be optimized. xFormat_u_open_file does unneccessary
				// fstat's.
				//
				if ((ret=swlib_unexpand_escapes(tmpM, source))) {
					fprintf(stderr, "%s: error expanding escapes for source name ( %s ) ret=%s\n",
						swlib_utilname_get(), source, swlib_imaxtostr(ret, NULL));
				}
				source_fd = xFormat_u_open_file(strob_str(tmpM));  // This sets the global stat/header buf.
				if (source_fd < 0)  {
					return -40;
				}
			} else {
				source_fd = -1;
			}

			//
			// Now override the filesystem stats. again so the stats specified in the PSF
			// appear in the archive header.
			//
			
			//
			// Form the control path in the (STROB*)tmpM object.
			//
			strob_strcpy(tmpM, path);
			swlib_squash_leading_dot_slash(strob_str(tmpM));
			swlib_squash_leading_slash(strob_str(tmpM));
			pstart = strstr(package_path, strob_str(tmpM));
			if (pstart) {
				//
				// Hard link handling trick. Make the tar archive
				// distribution have consistent links by adding the
				// the control directory part of the package path to
				// the link_source.  This will make tar happy when
				// unpacking the distribution with tar.
				//
				strob_strcpy(tmpM, package_path);
				*(strob_str(tmpM) + ((int)(pstart - package_path))) = '\0';
				tmp_control_path = tmpM;
			} else {
				tmp_control_path = (STROB*)NULL;
			}

			ret = xFormat_apply_swdef_stats(swdef, source,
				1 /*logical ON, means vremoved attr. will be ignored*/,
				tmp_control_path);
			if (ret)
				return -40;	
			
			swfile_set_package_filename(package_path);
			source = swdef->find(SW_A_source); // This line fixed a core dump.
			swfile_set_source_filename(source);
			if ((ret=swlib_unexpand_escapes2(package_path))) {
				fprintf(stderr, "%s: error expanding escapes for package_path ( %s ) ret=%s\n",
					swlib_utilname_get(), package_path, swlib_imaxtostr(ret, NULL));
			}
			SWPACKAGEFILE_DEBUG("Running xFormat_write_file");
			ret = swPackageFile::xFormat_write_file(static_cast<struct stat *>(NULL), package_path, source_fd);
			if (source_fd >= 0) xFormat_u_close_file(source_fd);
			SWPACKAGEFILE_DEBUG2("Running xFormat_write_file ret = %d", ret);
		} else {
			//
			// Stat the source.
			//
			if ((ret=swlib_unexpand_escapes(tmpM, source))) {
				fprintf(stderr, "%s: error expanding escapes for source name ( %s ) ret=%s\n",
					swlib_utilname_get(), source, swlib_imaxtostr(ret, NULL));
			}
			if ((ret=swlib_unexpand_escapes2(package_path))) {
				fprintf(stderr, "%s: error expanding escapes for package_path ( %s ) ret=%s\n",
					swlib_utilname_get(), package_path, swlib_imaxtostr(ret, NULL));
			}
			SWPACKAGEFILE_DEBUG("Running xFormat_write_file");
			ret = swPackageFile::xFormat_write_file(static_cast<struct stat*>(NULL), package_path, strob_str(tmpM)); 
			SWPACKAGEFILE_DEBUG2("Running xFormat_write_file ret = %d", ret);
		}
		return ret;
	}
	
	virtual intmax_t xFormat_write_file(char * name) {
		return uxformatM->xFormat_write_file(name);
	}
	
	virtual intmax_t xFormat_write_file(int fd) {
		return uxformatM->xFormat_write_file  (fd);
	}
	
	virtual intmax_t xFormat_write_file(int (*fout)(int)) {
		return uxformatM->xFormat_write_file(fout);
	}
	
	virtual intmax_t xFormat_write_file(struct stat *st, char * name) {
		return uxformatM->xFormat_write_file(st, name);
	}
	
	virtual intmax_t xFormat_write_file(struct stat *st, char * name, char * source) {
		return  uxformatM->xFormat_write_file(st, name, source); 
	}
	
	virtual intmax_t xFormat_write_file(char * name, char * source) {
		intmax_t ret;
		//struct stat st;
		//uxformatM->xFormat_set_to_statbuf(&st);
		ret = xFormat_write_file(static_cast<struct stat*>(NULL), name, source); 
		return ret;	
	}
	
	virtual intmax_t xFormat_write_file(struct stat *st, char * name, int source_fd) {
		return  uxformatM->xFormat_write_file(st, name, source_fd); 
	}
	
	virtual intmax_t xFormat_write_file(struct stat *st, char * name, int (*fout)(int)) {
	     return  uxformatM->xFormat_write_file(st, name, fout); 
	}
	
	virtual intmax_t xFormat_write_file_data(int source_fd){
	    return uxformatM->xFormat_write_file_data(source_fd);
	}
	
	int xFormat_write_trailer(void) {
	    return uxformatM->xFormat_write_trailer();
	}
	
	int xFormat_get_virtual_eof(void) {
	   return uxformatM->xFormat_get_virtual_eof();
	}
	
	intmax_t xFormat_read_file_data(int dst_fd) {
	    return  uxformatM->xFormat_read_file_data(dst_fd);
	}
	
	int xFormat_read(void * buf, size_t count) {
	    return  uxformatM->xFormat_read(buf, count);
	}
	
	int xFormat_read_header(void) {
	     return  uxformatM->xFormat_read_header(); 
	}
	
	int xFormat_unread_header(void) {
	     return  uxformatM->xFormat_unread_header(); 
	}
	
	int xFormat_write_header(void) {
	     return uxformatM->xFormat_write_header(); 
	}
	
	int xFormat_write_header(char * name) {
	     return uxformatM->xFormat_write_header(name); 
	}
	
	intmax_t xFormat_copy_pass(int discharge_fd) {
	     return  uxformatM->xFormat_copy_pass(discharge_fd); 
	}
	
	intmax_t xFormat_copy_pass(void) {
	     return  uxformatM->xFormat_copy_pass();
	}
	
	intmax_t xFormat_copy_pass(int discharge_fd, int source_fd) {
	     return  uxformatM->xFormat_copy_pass(discharge_fd, source_fd); 
	}
	
	char * xFormat_get_next_dirent(struct stat * st) {
	     return  uxformatM->xFormat_get_next_dirent(st);
	}
	
	int xFormat_setdir(char * path) {
	     return  uxformatM->xFormat_setdir(path);
	}
	
	char xFormat_get_tar_typeflag(void) {
	     return  uxformatM->xFormat_get_tar_typeflag(); 
	}
	
	int xFormat_file_has_data(void) {
	     return  uxformatM->xFormat_file_has_data();
	}
	
	char * xFormat_get_username(char * buf) {
	     return  uxformatM->xFormat_get_username(buf); 
	}
	
	char * xFormat_get_groupname(char * buf) {
	     return  uxformatM->xFormat_get_groupname (buf); 
	}

	void xFormat_set_sys_db_u_policy(int c) {
		return uxformatM->xFormat_set_sys_db_u_policy(c);
	}
	
	void xFormat_set_sys_db_g_policy(int c) {
		return uxformatM->xFormat_set_sys_db_g_policy(c);
	}
	
	size_t  xFormat_get_filesize(void) {
	     return  uxformatM->xFormat_get_filesize (); 
	}
	
	time_t xFormat_get_mtime(void) {
	     return  uxformatM->xFormat_get_mtime (); 
	}
	
	char * xFormat_get_name(STROB * buf) {
	     return  uxformatM->xFormat_get_name(buf); 
	}
	
	char * xFormat_get_linkname(char * buf) {
	     return  uxformatM->xFormat_get_linkname(buf); 
	}
	
	mode_t xFormat_get_mode(void) {
	    return uxformatM->xFormat_get_mode ();
	}
	
	mode_t xFormat_get_perms (void) {
	    return uxformatM->xFormat_get_perms();
	}
	
	char * xFormat_get_source_filename(char * buf) {
	     return  uxformatM->xFormat_get_source_filename(buf);
	}
	
	int xFormat_is_end_of_archive(void) {
	     return  uxformatM->xFormat_is_end_of_archive();
	}
	
	int xFormat_get_layout_type(void) {
		return uxformatM->xFormat_get_layout_type();
	}
	
	void xFormat_set_false_inodes(int n) {
		uxformatM->xFormat_set_false_inodes(n);
	}
	
	void xFormat_set_numeric_uids(int n) {
		uxformatM->xFormat_set_numeric_uids(n);
	}
	
	void xFormat_set_tarheader_flag(int flag, int n) {
		uxformatM->xFormat_set_tarheader_flag(flag, n);
	}
	
	int xFormat_get_tarheader_flags(void) {
		return uxformatM->xFormat_get_tarheader_flags();
	}
	
	//D void xFormat_debug_dump(FILE * fp) {
	//D 	uxformatM->xFormat_debug_dump(fp);
	//D 	if (is_ieee_layoutM) {
	//D 		swp_debug_dump(fp);
	//D 	}
	//D 	fprintf(fp,"\n");
	//D }
	
	void
	xFormat_reset_bytes_written(void) {
		uxformatM->xFormat_reset_bytes_written();
	}
        
	AHS * xFormat_hdr(void) {
		return uxformatM->uxFormat_hdr();
	}
	
        // ===================================
	// swPath methods.
        // ===================================
	
	char * swp_swpath_buffer(void){
		return swpathM->swp_buffer();
	}

	int swp_num_of_components(char * str) {
		return swpathM->swp_num_of_components(str);
	}
	
	int swp_resolve_prepath(char * name) {
		return swpathM->swp_resolve_prepath(name);
	}
	
	void  swp_set_product_control_dir (char * s) {
	     	swpathM->swp_set_product_control_dir(s); 
	}
	
	void swp_set_fileset_control_dir (char * s) {
		swpathM->swp_set_fileset_control_dir(s);
	}
	
	void swp_set_pfiles (char * s) {
		swpathM->swp_set_pfiles(s);
	}
	
	void swp_set_dfiles (char * s) {
		swpathM->swp_set_dfiles(s);
	}
	
	void swp_set_pathname (char * s) {
		swpathM->swp_set_pathname(s);
	}
	
	void swp_set_pkgpathname(char * s) {
		swpathM->swp_set_pkgpathname(s);
	}
	
	void swp_set_filename (char * s) {
	    swpathM->swp_set_filename(s);
	}
	
	void swp_set_prepath(char * s){
		swpathM->swp_set_prepath(s);
	}
	
	char * swp_get_product_control_dir (void) {
		return swpathM->swp_get_product_control_dir();	
	}
	char * swp_get_fileset_control_dir (void) {
		return swpathM->swp_get_fileset_control_dir();
	}
	
	char * swp_get_pfiles (void) {
		return swpathM->swp_get_pfiles();
	}
	
	char * swp_get_dfiles (void) {
		return swpathM->swp_get_dfiles();
	}
	
	char * swp_get_prepath(void){
		return swpathM->swp_get_prepath();
	}
	
	char * swp_get_pathname (void) {
		return swpathM->swp_get_pathname();
	}
	
	char * swp_get_pkgpathname (void) {
		return swpathM->swp_get_pkgpathname();
	}
	
	char * swp_get_basename (void) {
		return swpathM->swp_get_basename();
	}
	
	virtual char *  swp_form_path (STROB * buf) {
		return swpathM->swp_form_path(buf);
	}
	
	char *  swp_form_catalog_path (STROB * buf) {
		return swpathM->swp_form_catalog_path(buf);
	}
	
	char *  swp_form_storage_path(STROB * buf){
		return swpathM->swp_form_storage_path(buf);
	}
	
	void swp_set_is_catalog(int i) {
		return swpathM->swp_set_is_catalog(i);
	}
	
	int swp_get_is_catalog(void) {
		return swpathM->swp_get_is_catalog();
	}
	
	int swp_is_catalog(char * name){
		return swpathM->swp_is_catalog(name);
	}
	
	int swp_parse_path(char * name){
		return swpathM->swp_parse_path(name);
	}
	
	char * swp_make_name(char * directory_components, char * file_name){
		return swpathM->swp_make_name(directory_components, file_name);
	}
	
	//D void swp_debug_dump(FILE * fp){
	//D 	swpathM->swp_debug_dump(fp);
	//D }


	// char * swpackagefile_dump_string_s(char * prefix);
	// friend  char * swpackagefile_dump_string_s(swPackageFile  * pf, char * prefix);

	int xFormat_apply_swdef_stats(swDefinition * swdef, char * source,
			int logical_find, STROB * tmp_control_path) {
		int ret;
		int retval = 0;
		char * type;
		char * value;
		char * value2;
		unsigned long devnum;
		unsigned long timeval;
		unsigned long int uid;
		unsigned long int gid;
		unsigned long int dev;

		if (swdef->get_storage_status() == SWDEF_STATUS_DUP) {
			//
			// This software definition is a duplicate and is *not* the
			// first one (the first one is the one that survives and has
			// implicit file stats set for it).
			//
			return 0;
		}

		//
		// Make the find() routine skip attributes that have been vremove()'ed.
		// if logical_find == 1 then skip the vremove'ed attributes.
		//
		set_find_mode_logical(logical_find);

		xFormat_set_sys_db_g_policy(TARU_C_BY_GSYS); // Set default
		xFormat_set_sys_db_u_policy(TARU_C_BY_USYS); // Set default

		//
		// apply the file stats in the definition.
		//
			
		if ((value=swdef->find(SW_A_mtime))) {
			ret = taru_datoul(value, &timeval);
			if (ret == 2) retval = 3;
			xFormat_set_mtime((time_t)timeval);
		}
		else {
			//fprintf(stderr, "mtime missing for file %s\n", source);
			;
		}

		//
		// Set the user and uid
		//
		value2 = swdef->find(SW_A_uid);
		set_find_mode_logical(0);
		if ((value=swdef->find("owner"))) {
			//
			// Got a username
			//
			SWPACKAGEFILE_DEBUG2("Got a user name [%s]", value);
			if (!value2) {
				//
				// But no uid
				//
				SWPACKAGEFILE_DEBUG("");
				xFormat_set_sys_db_u_policy(TARU_C_BY_UNAME);
				xFormat_set_user_systempair(value);
				xFormat_set_sys_db_u_policy(TARU_C_BY_UNONE);
			} else {
				SWPACKAGEFILE_DEBUG("");
				if (strcmp(value, TARU_C_DO_NUMERIC) == 0) {
					SWPACKAGEFILE_DEBUG("Doing numeric");
					xFormat_set_username("");
				} else {
					SWPACKAGEFILE_DEBUG("Setting user name");
					xFormat_set_username(value);
				}
				ret = taru_datoul(value2, &uid);
				if (ret == 2) retval = 3;
				xFormat_set_uid((uid_t)uid);
				xFormat_set_sys_db_u_policy(TARU_C_BY_UNONE);
			}
		}
		else {
			//
			// No username
			//
			if (value2) {
				//
				// but has got uid
				//
				SWPACKAGEFILE_DEBUG("No user name, but has uid");
				xFormat_set_sys_db_u_policy(TARU_C_BY_UID);
				ret = taru_datoul(value2, &uid);
				if (ret == 2) retval = 3;
				xFormat_set_uid((uid_t)uid);
			} else {
				//
				// No uname or uid.
				//
				SWPACKAGEFILE_DEBUG("No user name and no uid");
				xFormat_set_sys_db_u_policy(TARU_C_BY_USYS);
				xFormat_set_group_systempair(AHS_USERNAME_NOBODY);
			}	
		}
		set_find_mode_logical(logical_find);
		//
		// Set the group and gid
		//
		value2 = swdef->find(SW_A_gid);
		set_find_mode_logical(0);
		if ((value=swdef->find(SW_A_group))) {
			//
			// Got a group name
			//
			SWPACKAGEFILE_DEBUG("Got a group name");
			if (!value2) {
				//
				// But no gid
				//
				SWPACKAGEFILE_DEBUG("");
				xFormat_set_sys_db_g_policy(TARU_C_BY_GNAME);
				xFormat_set_group_systempair(value);
				xFormat_set_sys_db_g_policy(TARU_C_BY_GNONE);
			} else {
				SWPACKAGEFILE_DEBUG("");
				if (strcmp(value, TARU_C_DO_NUMERIC) == 0) {
					SWPACKAGEFILE_DEBUG("Doing numeric");
					xFormat_set_groupname("");
				} else {
					SWPACKAGEFILE_DEBUG("Setting group name");
					xFormat_set_groupname(value);
				}
				ret = taru_datoul(value2, &gid);
				if (ret == 2) retval = 3;
				xFormat_set_gid((gid_t)gid);
				xFormat_set_sys_db_g_policy(TARU_C_BY_GNONE);
			}
		}
		else {
			//
			// No group
			//
			if (value2) {
				//
				// but has gid
				//
				SWPACKAGEFILE_DEBUG("No group name, but has gid");
				xFormat_set_sys_db_g_policy(TARU_C_BY_GID);
				ret = taru_datoul(value2, &gid);
				if (ret == 2) retval = 3;
				xFormat_set_gid((gid_t)gid);
			} else {
				//
				// No group or gid
				//
				SWPACKAGEFILE_DEBUG("No group name and no gid");
				xFormat_set_sys_db_g_policy(TARU_C_BY_GSYS);
				xFormat_set_group_systempair(AHS_USERNAME_NOBODY);
			}	
		}
		set_find_mode_logical(logical_find);

		//
		// Ignore the distribution.mode value since it is not wanted
		// to affect file permissions in the catalog.
		//
		if (
			((value=swdef->find(SW_A_mode)) != NULL) &&
			(strcmp(swdef->get_keyword(), SW_A_distribution) != 0)
		) {
			taru_otoul(value, &devnum);
			xFormat_set_mode(0);
			xFormat_set_perms((mode_t)devnum);
		}
		else {
			;
			//fprintf(stderr, "mode missing for file %s\n", source);
		}
		
		if ((value=swdef->find(SW_A_minor))) {
			ret = taru_datoul(value, &devnum);
			if (ret == 2) retval = 3;
			xFormat_set_devminor((dev_t)devnum);
		}
		else {
			//fprintf(stderr, "mode missing for file %s\n", source);
		}
		
		if ((value=swdef->find(SW_A_major))) {
			ret = taru_datoul(value, &devnum);
			if (ret == 2) retval = 3;
			xFormat_set_devmajor((dev_t)devnum);
		}
		else {
			//fprintf(stderr, "mode missing for file %s\n", source);
		}

		if ((value=swdef->findPhysical("_ino"))) {
			ret = taru_datoul(value, &devnum);
			if (ret == 2) retval = 3;
			xFormat_set_inode((ino_t)devnum);
		}
		
		if ((value=swdef->findPhysical("_nlink"))) {
			ret = taru_datoul(value, &devnum);
			if (ret) retval = 3;
			xFormat_set_nlink(devnum);
		}


		if ((type=swdef->find("type"))) {
			char t = swheader_getTarTypeFromTypeAttribute(*type);	
			xFormat_set_filetype_from_tartype(t);

			xFormat_set_linkname("");
			if (*type == 's' || *type == 'h' ) {
				if ((value=swdef->find("link_source"))) {
					if (*type == 'h' && tmp_control_path) {
						//
						// Check that the length of tmp_control_path
						// plus value is less than 100.  If it
						// is then do prepend the tmp_control_path
						// making the hard link linkname consistent
						// with the package path names.  This will
						// preserve hard link associations if the POSIX
						// layout is unpacked with tar. (This only a nicety,
						// since according to the standard the storage
						// section metadata need not match the INFO file
						// attributes.)
						//
						if (strob_strlen(tmp_control_path) +
							strlen(value) + 1 >= TARLINKNAMESIZE) {
							;
							// value is already set.	
						} else {
							//
							// Prepend the control_directories.
							//
							swlib_unix_dircat(tmp_control_path, value);	
							value = strob_str(tmp_control_path);
						}
					}
					xFormat_set_linkname(value);
				} else {
					retval = 1;
					fprintf(stderr, "link_source missing for file %s\n", source);
				}
			} else {
				xFormat_set_linkname("");
			}
	
			if (*type == 'b' || *type == 'c' ) {
				if ((value=swdef->find(SW_A_minor))) {
					sscanf(value, "%lu", &dev);
					xFormat_set_devminor((dev_t)dev);
				} else {
					retval = 1;
					fprintf(stderr, "minor device number missing for file %s\n", source);
				}
	
				if ((value=swdef->find(SW_A_major))) {
					sscanf(value, "%lu", &dev);
					xFormat_set_devmajor((dev_t)dev);
				} else {
					retval = 2;
					fprintf(stderr, "major device number missing for file %s\n", source);
				}
			}
		} 
		else {
			//fprintf(stderr, "type missing for file %s\n", source);
		}

		set_find_mode_logical(0);	
		return retval;
	}
	

  private:

	void dup_name_(char **obj, char * name) {
		delete[] *obj;
		*obj= ::new char[strlen(name) + 1];
		::strncpy(*obj, name, strlen(name) + 1);
	}
	
	static void finalize(int mm) {
		if (mm && uxformatM) {
			SWPACKAGEFILE_DEBUG("deleting uxformatM");
			delete uxformatM;
			uxformatM = NULL;
		}
	}

	void un_init_(void) {
		delete initor_;
		delete swpathM;
		if (tmpM) strob_close(tmpM);	
		tmpM = NULL;	
		if (user_path_strob_buf_) strob_close(user_path_strob_buf_);
		user_path_strob_buf_ = NULL;
	}

	void init_(void) {
		struct new_cpio_header * default_header;
		swpathM = new swPathName();
		default_header = ahsStaticCreateFilehdr();
		is_ieee_layoutM = 0;
		user_path_strob_buf_ = strob_open(2);		
		if (tmpM == NULL) tmpM = strob_open(100);
		fdM = -1;
	}
};
#endif


