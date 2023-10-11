/* swexfileset.h
 */

/*
 * Copyright (C) 2003,2008,2009 James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swexfileset_hxx
#define swexfileset_hxx

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swuser_assert_config.h"
#include "swmetadata.h"
#include "swexstruct.h"
#include "swexstruct_i.h"
#include "swobjfiles_i.h"
#include "swpackagefile.h"
#include "swdefinitionfile.h"
#include "swinfo.h"
#include "swptrlist.h"
#include "swexproduct.h"
extern "C" {
#include "swuser_config.h"
#include "uxfio.h"
#include "strob.h"
#include "swvarfs.h"
#include "swheaderline.h"
#include "taru.h"
#include "strtoint.h"
}

// #define SWEXFILESETNEEDDEBUG 1
#undef SWEXFILESETNEEDDEBUG

#ifdef SWEXFILESETNEEDDEBUG
#define SWEXFILESET_DEBUG(format) SWBISERROR("SWEXFILESET DEBUG: ", format)
#define SWEXFILESET_DEBUG2(format, arg) SWBISERROR2("SWEXFILESET DEBUG: ", format, arg)
#define SWEXFILESET_DEBUG3(format, arg, arg1) SWBISERROR3("SWEXFILESET DEBUG: ", format, arg, arg1)
#else
#define SWEXFILESET_DEBUG(arg)
#define SWEXFILESET_DEBUG2(arg, arg1)
#define SWEXFILESET_DEBUG3(arg, arg1, arg2)
#endif 

class swExFileset: public swObjFiles_i
{
	swPtrList<swPackageFile> * distributionFilesM;
	STROB * sbufM;
	uintmax_t sizeM;
      public:
	
	swExFileset(char * control_dir): swObjFiles_i(control_dir) {
		distributionFilesM = new swPtrList<swPackageFile>();
		sizeM = 0;
		sbufM = strob_open(10);
	}
	
	swExFileset(void): swObjFiles_i("") {
		distributionFilesM = new swPtrList<swPackageFile>();
		sizeM = 0;
		sbufM = strob_open(10);
	}

	~swExFileset(void) {
		int i=0;
		swPackageFile * pf;
		if (distributionFilesM) {
			while ((pf=distributionFilesM->get_pointer_from_index(i++)))
				delete pf;
		}
		strob_close(sbufM);
	}
	
	//D char * dump_string_s(char * prefix);
	
	swINDEX * getIndex(void) { return static_cast<swINDEX*>(NULL); }

	char * getObjectName(void) { return SW_A_fileset; }

	int write_filename(){
		int i=0;
		char *name;
		swPackageFile * pf;
		if (distributionFilesM) {
			while ((pf=distributionFilesM->get_pointer_from_index(i++))) {
				name = pf->swp_get_pkgpathname();
				fprintf(stdout,"%s\n", name);
			}
		}
		return 0;
	}

	static swExStruct * make_exdist(void) {
		return new swExFileset("");
	}

	virtual swPtrList<swPackageFile> * getFileList(void){ 
		return distributionFilesM; 
	}

	virtual swPtrList<swPackageFile> * getFileListObject(void){ 
		return getFileList(); 
	}
	
	
	virtual swPackageFile * swPackageFileFactory(char * path, char * source) {
		return new swPackageFile(path, source);
	}
	
	int doesContain(char * key) {
		return (strcmp(key, SW_A_file) == 0 ||
			strcmp(key, SW_A_control_file) == 0
			);
	}

	int write_fd(int fd) {
		int ret = 0;
		ret = getReferer()->write_fd(fd);	
		ret += getInfo()->swdeffile_linki_write_fd(fd);	
		return ret;
	}

	virtual int swobjfile_file_out_of_scope(swMetaData * swmd,
			int current_level) {
		return (swmd->get_level() < current_level);
	}	

	int registerWithGlobalIndex(void) {
		swINDEX * index = getGlobalIndex();
		swDefinition *u_swdef = getReferer();
		index->swdeffile_linki_append(u_swdef);
		return 0;
	}

	/** processForExport_step1 -
	 *  @swheader: The iterator/search object.
	 *  @psf: The PSF file.
	 *  @current_level: The current level of the calling object.
	 *  @current_line: The current line of the calling object.
	 *
	 *  The "fileset" specific routine.  This override is required
	 *  is required because the fileset is slightly different because it
	 *  has the file(s) which are split off from the PSF file and put in
	 *  their own INFO file.
	 */
	
	swDefinition  * 
	processForExport_step1(SWHEADER * swheader, swDefinitionFile * psf,
			int current_level, char * current_line, int * error_code)
	{ 
		int type;
		int level;
		int does_have_files = 0;
		char *next_line;
		char * keyword;
		swMetaData * swmd = NULL;
		swDefinition * current_swmd;
		swMetaData * previous_swmd;
		swDefinition * after_last_file = NULL;
		swDefinition * last_def;

		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
			"swExFileset::Export_step1: ENTERING\n");

		if (strlen(current_line) == 0) return NULL;

		swmd = psf->swdeffile_linki_find_by_parserline(NULL, current_line);
		
		if (swmd == NULL) {
			fprintf(stderr, "NULL...... [%s]\n", current_line);
			SWLIB_ASSERT(swmd != NULL);
		}
		SWLIB_ASSERT(swmd->get_type() == swstructdef::sdf_object_kw);
		current_swmd = static_cast<swDefinition*>(swmd);
		setReferer(current_swmd);
		last_def = current_swmd;
		swmd = NULL;
		
		SWBIS_E_DEBUG("ENTERING");
		SWBIS_E_DEBUG3("this=[%s] parserline address=[%p]",
				this->getObjectName(), current_swmd->get_parserline());
	
		next_line = swheader_get_next_object(swheader,
				(int)UCHAR_MAX, (int)UCHAR_MAX);
		if (next_line == NULL) {
			fprintf(stderr, "swpackage: Warning: fileset has no files.\n");
			after_last_file = process_file_and_control_file_definitions(swmd,
							swheader, psf, last_def, error_code);
			*error_code = 0;
		}
		while (next_line){
			swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
				"swExFileset::Export_step1: next_line=[%s]\n", next_line);
			SWBIS_E_DEBUG2("next_line = [%s]", next_line);
			previous_swmd = swmd;
			swmd = psf->swdeffile_linki_find_by_parserline(NULL, next_line);
			SWLIB_ASSERT(swmd != NULL);
			type = swmd->get_type();
			level = swmd->get_level();
			keyword = swmd->get_keyword();
			SWLIB_ASSERT(type == swstructdef::sdf_object_kw);
			if (level <= current_level) {
				//
				// end of the object scope.
				//
				if (does_have_files == 0) {
					swINFO * info;
					fprintf(stderr,
				"swpackage: Warning: fileset has no files.\n");
					info = new swINFO("");
					info->swdeffile_linki_set_head(NULL);
					info->swdeffile_linki_set_tail(NULL);
					setInfo(info);
					// OK type == swstructdef::sdf_object_kw Asserted above.
					after_last_file = static_cast<swDefinition*>(swmd);
				}
				SWBIS_E_DEBUG("break");
				break;
			}
			if (doesContain(keyword)) {
				does_have_files = 1;
				SWBIS_E_DEBUG2("doesContain [%s]", keyword);
				after_last_file = process_file_and_control_file_definitions(swmd,
							swheader, psf, last_def, error_code);
				SWBIS_E_DEBUG2("after_last_file = [%p]", after_last_file);
			}
			next_line = swheader_get_next_object(swheader,
					(int)UCHAR_MAX, (int)UCHAR_MAX);
			last_def = static_cast<swDefinition*>(swmd);
		}
		SWBIS_E_DEBUG2("LEAVING --  after_last_file = [%p]", after_last_file);
		SWBIS_E_DEBUG("LEAVING");
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
			"swExFileset::Export_step1: LEAVING\n");
		return after_last_file;
	}
	
	virtual void performInfoPass2(void) {
		SWEXFILESET_DEBUG("entering");
		swExStruct_i::adjustFileDefs(getInfo()); 
		SWEXFILESET_DEBUG("leaving");
	}


	void generateFileSetSize(void) {
		swINFO * info = getInfo();
		uintmax_t bytesize = 0;
		swDefinition * inext;
		swMetaData * att;
		swDefinition *swdef = getReferer();
		char * asize;
		STROB * uimaxbuf;

		SWEXFILESET_DEBUG("entering");
		uimaxbuf = strob_open(24);
		if (info == NULL) return;
		inext = info->swdeffile_linki_get_head();

		while(inext) {
			att = inext->get_next_node();  // The first attribute.
			while (att) {
				if ( ::strcmp(att->get_keyword(), SW_A_size) == 0) {
					bytesize += ::strtoumax(att->get_value(NULL), NULL, 10);
				}
				att = att->get_next_node();
			}
			inext = inext->get_next();
		}
		sizeM = bytesize;
		asize = swlib_umaxtostr(sizeM, uimaxbuf);
		swdef->add(SW_A_size, asize);
		strob_close(uimaxbuf);
		SWEXFILESET_DEBUG("leaving");
	}
	
	//
	// Emit the leading directory of the fileset. 
	//
	virtual void emitLeadingCatalogDirectory(STROB * tmp) {	
		char * s;
		int ret;
		//
		// IF the control_directorys are "" this test avoid writing the
		// <path>/catalog/ 
		//
		
		s = strstr(strob_str(tmp), SW_A_catalog);
		SWLIB_ASSERT(s != NULL);
		s += strlen(SW_A_catalog);
		
		if ( (*s == '/' && *(s+1) == '\0') || (*(s+0) == '\0')) {
			//
			// Its "catalog\0"  or  "catalog/\0"
			//		or
			//  <path>\0      or    <path>/\0
			//
			// Don't write it,
			;
		} else {
			//
			// Write the directory.
			//
			swlib_add_trailing_slash(tmp);
			ret = swExStruct_i::swexstruct_write_dir(
				strob_str(tmp), 
				getCreateTime(),
				getCatalogDirModeString(),
				getCatalogOwner(),
				getCatalogGroup());
			if (ret < 0) setErrorCode(20004, NULL);
		}
		return;
	}

	//
	// Emit the Storage Structure.
	//
	virtual void emitStorageStructure(void){ 
		swDefinitionFile * info;	
		swDefinition * swdef;
		swDefinition * stats_swdef;
		char * filetype;
		char * path;
		char * value;
		int ofd;
		int preview_fd;
		int ret;
		intmax_t retval;
		int skip;
		int do_adjunct = 0;
		swPackageFile * archiver;
		STROB * tmp;

		SWEXFILESET_DEBUG("");
		ofd = get_ofd();
		preview_fd = get_preview_fd();
		info = getInfo();
		tmp = strob_open(100);
		archiver = getArchiver();

		//
		// Form the package path for the control path.
		//
		strob_strcpy(tmp,
			getFormedControlPath(static_cast<char*>(NULL),
				static_cast<char*>(NULL)));
	
		//
		// Emit the leading directory for the fileset.
		//
		emitLeadingStorageDirectory(tmp);

		swdef = info->swdeffile_linki_get_head();
		while(swdef) {
			if (info->determine_status_for_writing(swdef) == 0) {
				//
				// this results from multiple file definitions
				// in the PSF file.  determine_status_for_writing()
				// actually merges attributes as well as indicating
				// whether to write it.
				//
				goto skipdup;
			} else {
				// DEBUG: swdef->write_fd(2);
				;
			}

			if ((path=swdef->find(SW_A_path)) != NULL)  {
				if (
					::strcmp(path, SW_A_INFO) == 0 ||
					::strcmp(swdef->get_keyword(), SW_A_control_file) == 0 ||
					0
				) {
					//
					// Skip the INFO file and control_files.
					//
					swdef = swdef->get_next();
					continue;
				}
			} else {
				//
				// error.
				//
				setErrorCode(12302, NULL);
				fprintf(stderr, "path attribute not found.\n");
				return;
			}
		
			//
			// Form the path of the archive member.
			//
			strob_strcpy(tmp,
				 getFormedControlPath(path,
					static_cast<char*>(NULL)));
			swlib_squash_double_slash(strob_str(tmp));

			if (strcmp(strob_str(tmp), "./.") == 0) {
				//
				// HACK, FIXME maybe, handle a special case.
				// This happens when the user specifies --dir="."
				//
				strob_strcpy(tmp, ".");
			}

			if (allowAbsolutePaths() == 0) {
				swlib_squash_all_leading_slash(strob_str(tmp));
			}

			//
			// Write the archive member.
			//
			retval = 0;
			skip = 0;
			SWEXFILESET_DEBUG2("Processing [%s]", path);
			filetype = swdef->find(SW_A_type);
			if (!filetype) {
				setErrorCode(12303, NULL);
				return;
			}

			if (*filetype != SW_ITYPE_f && getStoreRegTypeOnly() ) {
				skip = 1;
			}

			if (do_adjunct) {
				//
				// Skip symlinks.
				//
				if (*filetype == SW_ITYPE_s) {
					//
					// skip it.
					//
					skip = 1;
					;;
				}
			}

			//
			// Check that if a special device file, the minor/major can fit
			// in a standard tar header.
			//

			if (*filetype == SW_ITYPE_c || *filetype == SW_ITYPE_b) {
				char * xp;
				int xret;
				int xresult;
				struct new_cpio_header * file_hdr;

				file_hdr = taru_make_header();
				xp = swdef->find(SW_A_major);
				file_hdr->c_rdev_maj = swlib_atoul(xp, &xresult, (char**)NULL);
				if (xresult) setErrorCode(12306, NULL);
				xp = swdef->find(SW_A_minor);
				file_hdr->c_rdev_min = swlib_atoul(xp, &xresult, (char**)NULL);
				if (xresult) setErrorCode(12307, NULL);
				assertNoErrorCondition(41);
				xret = taru_check_devno_for_tar(file_hdr);
				if (xret) {
					// can't represent in seven (7) octal digits
					skip = 1;
					fprintf(stderr,
       	                                	"%s: device number too large, omitting from storage section: %s\n",
						swlib_utilname_get(), path);
				} else {
					xret = taru_check_devno_for_system(file_hdr);
					if (xret) {
						fprintf(stderr,
       		                                	"%s: warning: device number may be too large for this system: %s\n",
							swlib_utilname_get(), path);
					}
				}
				taru_free_header(file_hdr);
			}

			stats_swdef = swdef;
			if (swdef->get_storage_status() == SWDEF_STATUS_IGN) {
				skip = 1;
					SWEXFILESET_DEBUG2("skipping path=[%s] status=SWDEF_STATUS_IGN", path);
					#ifdef SWEXFILESETNEEDDEBUG
						SWEXFILESET_DEBUG2(">>>>> skipping path=[%s]", path);
						stats_swdef->write_fd(2);
						SWEXFILESET_DEBUG2("<<<<< skipping path=[%s]", path);
					#endif
			} else if (swdef->get_storage_status() == SWDEF_STATUS_DUP) {
				skip = 1;
					SWEXFILESET_DEBUG2("skipping path=[%s] status=SWDEF_STATUS_DUP", path);
					#ifdef SWEXFILESETNEEDDEBUG
					SWEXFILESET_DEBUG2(">>>>> skipping path=[%s]", path);
					stats_swdef->write_fd(2);
					SWEXFILESET_DEBUG2("<<<<< skipping path=[%s]", path);
					#endif
					// fprintf(stderr, "eraseme skipping _DUP [%s]\n", path);
					// stats_swdef->write_fd(2);
			} else if (swdef->get_storage_status() == SWDEF_STATUS_DUP0) {
				skip = 0;
				SWEXFILESET_DEBUG2("geting last swdef for path=[%s] status=SWDEF_STATUS_DUP0", path);
				
				//fprintf(stderr, "eraseme including _DUP0 [%s]\n", path);
				//stats_swdef->write_fd(2);

				//
				// Find the swdef with the same path and with a status
				// of SWDEF_STATUS_DUP.   This is the definition to
				// use the stats of because this was the last one as
				// as determined in swobfiles_i.h
				//
				// The swdefinition we are looking for has to be further down
				// the same list, the search key is the path attribute and
				// the storage status must be SWDEF_STATUS_DUP.	
				// It must be found, or else an internal error has occurred.
				//
				// WRONG as of 2008-11-16 stats_swdef = find_last_swdef(swdef, path);
				// SWLIB_ASSERT(stats_swdef != NULL);
			}

			// fprintf(stderr, "eraseme DX writing %s\n", strob_str(tmp));
			// swdef->write_fd_debug(2, "");
			if (skip) {
				// archiver->xFormat_set_ofd(getNullFd());
				// retval = archiver->xFormat_write_file(swdef, strob_str(tmp));
				// archiver->xFormat_set_ofd(ofd);
				SWEXFILESET_DEBUG2("skipping [%s]", path);
				retval = 0;
			} else {
				SWEXFILESET_DEBUG2("writing [%s]", path);
				swlib_process_hex_escapes(strob_str(tmp));
				#ifdef SWEXFILESETNEEDDEBUG
					SWEXFILESET_DEBUG2(">>>>> writing path=[%s]", path);
					stats_swdef->write_fd(2);
					SWEXFILESET_DEBUG2("<<<<< writing path=[%s]", path);
				#endif
				/* Handle a root directory in the package as
				   a special case */
				if (strcmp("/", path) == 0) {
					if (
						strcmp(strob_str(tmp), ".") == 0 ||
						strob_strlen(tmp) < 3 /* arbitrary sanity limit */
					) {
						;
						//fprintf(stderr,
                                        	//"%s: Warning: root directory member processing exception\n",
	                                        //swlib_utilname_get());
					} else {
						swlib_unix_dircat(tmp, "./");
					}
				}
				SWEXFILESET_DEBUG2("<<<<< writing [%s]", tmp->str_);

				/* Final sanity check on the archive path */
				if (strstr(strob_str(tmp), "../")) {
					retval = -1;
				} else {
					retval = archiver->xFormat_write_file(stats_swdef, strob_str(tmp));
				}
			} 
			skip = 0;	
		
			//
			// If retval =  TARU_HE_LINKNAME_4 i.e. -4 then just leave it out of the
			// storage section.
			//

	
			if (retval < 0 && retval != TARU_HE_LINKNAME_4 /* link name too long */) {
				fprintf(stderr,
					"%s: error writing archive member, status=%s, name=%s\n",
					swlib_utilname_get(), swlib_imaxtostr(retval, NULL), strob_str(tmp));
				setErrorCode(12304, NULL);
				return;
			}
	skipdup:;
			swdef = swdef->get_next();
		}
		return;	
	}
};
#endif
