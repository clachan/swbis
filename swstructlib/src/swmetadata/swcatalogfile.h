/* swcatalogfile.h
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swcatalogfile_h_0923
#define swcatalogfile_h_0923

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "swmemfile.h"
#include "swpackagefile.h"

class swCatalogFile: public swPackageFile
{
   char * bufM;
   int lenM;
   struct stat stM;
   swMetaData * attributeRefererM;
   swMemFile * swmemfileM;
   int archiveIncludeM;  // Default is 1 (yes include in archive), 0 (no)

   public:

	swCatalogFile(void): swPackageFile() { 
		init_(); 
	}
	
	swCatalogFile(char * pkg_name, char * src_name): swPackageFile(pkg_name, src_name) { 
		init_(); 
	}

	virtual ~swCatalogFile (void){ 
		delete swmemfileM;  
	}

	virtual int write_fd(int fd){ 
		int memfd = swmemfileM->swMemFileGetfd(); 
		if (memfd < 0) return -1;
		uxfio_lseek(memfd, 0L, SEEK_SET);
		return swlib_pipe_pump(fd, memfd);
	}

	virtual int swfile_open_public_image_fd(void) {
		int memfd;
		memfd = swmemfileM->swMemFile_u_open();
		// xFormat_set_from_statbuf(&stM);		
		return memfd;
	}

	virtual int swfile_close_public_image_fd(void) {
		swmemfileM->swMemFile_u_close();
		return 0;
	}
	
	virtual int swfile_re_close_public_image_fd(void) {
		int len;
		char * buf;
		int fd = swmemfileM->swMemFileGetfd(); 
		
		if (fd < 0) return -1;
		xFormat_set_to_statbuf(&stM);		
		if (uxfio_get_dynamic_buffer(fd, &buf, NULL, &len) < 0) return -1;
		bufM = buf;
		lenM = len;

		//
		// This tricks uxfio not to free the mem.
		//
		uxfio_ioctl(fd, UXFIO_IOCTL_SET_IMEMBUF, (void*)NULL);

		//
		// Now do the real close.
		//
		swfile_close_public_image_fd();
		return 0;
	}
	
	virtual void setArchiveInclude(int c) {
		archiveIncludeM = c;
	}

	virtual int getArchiveInclude(void) {
		return archiveIncludeM;
	}
	
	virtual int swfile_re_open_public_image_fd(void) {
		int fd;

		if (bufM == NULL) return -1;
	
		fd = swfile_open_public_image_fd();
		if (fd < 0) return fd;	
		if (uxfio_write(fd, bufM, lenM) != lenM) {
			fprintf(stderr, "internal error loc=3ua.\n");	
			exit(55);	
		}
		xFormat_set_from_statbuf(&stM);		
		free(bufM);
		bufM = NULL;
		uxfio_lseek(fd, (off_t)0, SEEK_SET);
		return fd;
	}


	virtual intmax_t xFormat_write_file(void) {
		intmax_t ret;
		int source_fd;
		char * name;
		off_t size;

		name = swfile_get_package_filename();	
		source_fd = swfile_open_public_image_fd();

		if ((ret=(int)uxfio_lseek(source_fd, 0L, SEEK_END)) < 0) {
			fprintf(stderr, "internal error in swAttributeFile::xFormat_write_file loc=1\n");
			exit(33);
		}

		size = uxfio_lseek(source_fd, 0L, SEEK_CUR);
		xFormat_set_filesize((intmax_t)size);
		xFormat_set_filetype_from_tartype(REGTYPE);
		xFormat_set_name(name);
		xFormat_set_nlink(1);
		if ((ret=(int)uxfio_lseek(source_fd, 0L, SEEK_SET)) < 0) {
			fprintf(stderr, "internal error in swAttributeFile::xFormat_write_file loc=2\n");
			exit(33);
		}

		ret = swPackageFile::xFormat_write_file(static_cast<struct stat*>(NULL), name, source_fd);
		return ret;
	}
	
	void setAttributeReferer(swMetaData * att) { attributeRefererM = att;}

	swMetaData * getAttributeReferer(void) { return attributeRefererM; }

   private:
	void init_(void){
		bufM = NULL;
		lenM = 0;
		attributeRefererM = NULL;
		archiveIncludeM = 1;
		swmemfileM = new swMemFile;
		swmemfileM->swMemFile_u_creat();
		memset((void*)(&stM), '\0', sizeof(struct stat));
	}

};
#endif
