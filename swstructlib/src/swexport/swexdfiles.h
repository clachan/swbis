/* swexdfiles.h - The dfiles exported file structure.
 */

/*
 * Copyright (C) 2003  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swexdfiles_hxx
#define swexdfiles_hxx

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swmetadata.h"
#include "swexstruct.h"
#include "swexstruct_i.h"
#include "swobjfiles.h"
#include "swinfo.h"
#include "swptrlist.h"
#include "swpackagefile.h"
#include "swdefinitionfile.h"

class swExDfiles: public swObjFiles
{
	public:

	swExDfiles(void): swObjFiles(){
		init();
		return;
	}

	static swExDfiles * makeXfiles(void) { return new swExDfiles(); }

	~swExDfiles(void) { }
	
	char * getObjFileName(){ static char d[]="dfiles"; return d;}

	swINDEX * getIndex(void){return NULL;}

	char * getObjectName(void) { return "dfiles"; }

	void tuneFilesFile(void) {
		char * files;
		int memfd;
		memfd = getFilesFd();
		if (memfd < 0) return;	
		uxfio_get_dynamic_buffer(memfd, &files, (int*)NULL, (int*)NULL);
		tuneAttributeFile("files", files);
		fixFilesFilesize((int)strlen(files));
		setFilesFd(-1);
	}
	
	void tuneSignatureFile(void) {
		char * sig;
		sig=getSigFileBuffer();
		tuneAttributeFile(SW_A_signature, sig);
	}

	void tuneAttributeFile(char * name, char * buffer) {
		swPtrList<swPackageFile> * archiveMemberList;
		swPackageFile * swfile;
		int index = 0;
		
		archiveMemberList = getAttributeFileList();

		if (buffer == static_cast<char*>(NULL)) {
			//
			// Delete the attribute file.
			//
			swfile = archiveMemberList->get_pointer_from_index(index);
			while(swfile) {
				if (::strcmp(name, swfile->swfile_get_filename()) == 0) {
					//
					// delete it (in a virtual way) so it can appear
					// in a (option -p) filelist. 
					//
					// Excludes it from the archive
				  	// but still writes it in the filelist.
					
					//OLD LINE archiveMemberList->list_del(index);
					swfile->setArchiveInclude(0);   
					return;
				}
				index++;
				swfile = archiveMemberList->get_pointer_from_index(index);
			}
			//
			// error.
			//
			setErrorCode(10436, NULL);
		} else {
			//
			// Add the correct file contents.
			//
			int fd;
			int ret;
			while((swfile=archiveMemberList->get_pointer_from_index(index++)) != NULL) {
				if ( ::strcmp(name, swfile->swfile_get_filename()) == 0) {
					fd = swfile->swfile_re_open_public_image_fd();
					if (fd < 0) {
						setErrorCode(10450, NULL);
						return;
					}

					ret = uxfio_write(fd, buffer, strlen(buffer));
					if (ret != (int)strlen(buffer)) {
						setErrorCode(10451, NULL);
					}

					if (::strcmp(name, SW_A_signature) == 0) {	
						int padlen;
						char * pad;
						int sigend;

						sigend = ret; // from above
	
						padlen = uxfio_lseek(fd, (off_t)0, SEEK_END);
						if (padlen < 0) {
							setErrorCode(10453, NULL);
							return;
						}
						if (padlen == 0) {
							setErrorCode(10455, NULL);
							return;
						}
		
						if (padlen != 512 && padlen != 1024) {
							setErrorCode(10455, NULL);
						}

						if ((int)strlen(buffer) > (padlen - 2)) {
							/* Sanity check */
							setErrorCode(10449, NULL);
							return;
						}

						if (uxfio_lseek(fd, (off_t)sigend, SEEK_SET) != sigend) {
							setErrorCode(10454, NULL);
							return;
						} 

						pad = (char *)malloc(padlen);
						SWLIB_ASSERT(pad != NULL);
						memset(pad, '\n', padlen);
						ret = uxfio_write(fd, pad, (size_t)(padlen - sigend));
						if (ret != padlen - sigend) {
							setErrorCode(10452, NULL);
						}
						free(pad);
					}
					swfile->swfile_re_close_public_image_fd();
					return;
				}
			}
			//
			// error.
			//
			setErrorCode(10437, NULL);
		}
	}

	private:
	void init(void) { }

	void fixFilesFilesize(int newsize) {
		char asize[24];
		char asizeformat[24];
		char * value;
		char * tagvalue;
		swDefinition * inext;
		swINFO * info;
		const int fieldWidth = 8;

		info = getInfo();
		if (info == NULL) {
			setErrorCode(10460, NULL);
			return;
		}

		if (newsize > 99999999) {
			setErrorCode(10461, NULL);
			return;
		}

		snprintf(asizeformat, sizeof(asizeformat) - 1, "%%%dd", fieldWidth);
		snprintf(asize, sizeof(asize) -1, asizeformat, newsize);  // Eight chars wide.
		asize[sizeof(asize) - 1] = '\0';

		inext = info->swdeffile_linki_get_head();
		while(inext) {
			tagvalue = inext->find(SW_A_tag);
			if (::strcmp(tagvalue, "files") == 0) {
				value = inext->find("size");
				if (value == NULL) {
					setErrorCode(10463, NULL);
					return;
				}
				::strncpy(value, asize, fieldWidth);
				break;
			}
			inext = inext->get_next();
		}
		return;
	}
};
#endif
