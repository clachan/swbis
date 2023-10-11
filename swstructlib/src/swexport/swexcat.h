/* swexcat.h: The Exported Product and Distribution Catalog Class.
 */

/*
 * Copyright (C) 2003 James H. Lowe, Jr. 
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

#ifndef swexcat_1_h
#define swexcat_1_h

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG 

extern "C" {
#include "swuser_config.h"
#include "swextopt.h"
#include "swuser_assert_config.h"
#include "debug_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#include "swmetadata.h"
#include "swexstruct.h"
#include "swpathname.h"
#include "swobjfiles.h"
#include "swexstruct_i.h"
#include "swexpsf.h"

class swExHost;
class swExDistribution;

/*
#ifdef FILENEEDDEBUG
#define E_DEBUG(format) SWBISERROR("DEBUG", format)
#define E_DEBUG2(format, arg) SWBISERROR2("DEBUG", format, arg)
#define E_DEBUG3(format, arg, arg1) \
 			SWBISERROR3("DEBUG", format, arg, arg1)
#else
#define E_DEBUG(format)
#define E_DEBUG2(format, arg)
#define E_DEBUG3(format, arg, arg1)
#endif
*/

class swExCat: public swObjFiles_i
{
	swObjFiles * xFilesM;	// either pfiles or dfiles. 
	STROB * sbufM;		// debug buffer.
  public:
	swExCat(char * parent_path): swObjFiles_i(parent_path) { init(); }
	swExCat(void): swObjFiles_i("") { init(); }
	virtual ~swExCat(void) { strob_close(sbufM); }
	
	//
	// Accessor Functions.
	//

	virtual swPtrList<swExStruct> * getProducts(void){ return NULL; }
	virtual swPtrList<swExStruct> * getFilesets(void){ return NULL; }

	virtual swINFO * getInfo(void) { return NULL; }

	void setXfiles(swObjFiles * x) { xFilesM = x; }

	int write_filename(void){
		int  list_index = 0;
		swExStruct 	* object;
		swPtrList<swExStruct> * objects = getStorageObjectList();
		char * path = getFormedControlPath("", NULL);
		fprintf(stdout,"%s\n", path);
		while((object = objects->get_pointer_from_index(list_index++))) {
			object->write_filename();
		}
		return 0;
	}
	
	virtual swPtrList<swPackageFile> * getAttributeFileList(void){ return getXfiles()->getAttributeFileList(); }
	virtual swPtrList<swPackageFile> * getControlScriptList(void){ return getXfiles()->getControlScriptList(); }
	
	swExStruct * get_first_product(int * found) {
		int  list_index = 0;
		swExStruct * cret;
		swExStruct * ret = NULL;
		swExStruct 	* object;

		while((object = containedByIndex(list_index++))) {
			if (::strcmp(object->getObjectName(), SW_A_product) == 0) {
				*found = 1;
				return object;
			}
			cret = object->get_first_product(found);
			if (*found) return cret;
		}
		return ret;
	}

	void
	write_first_product_directory(void) {
		int found = 0;
		swExStruct * prod1 = get_first_product(&found);

		if (!found || !prod1 || strcmp(prod1->getObjectName(), SW_A_product)) {
			fprintf(stderr, "internal error in write_first_product_directory\n");
			setErrorCode(10941, NULL);
			return;
		}
		
		{
			STROB * tmp = strob_open(10);
			strob_strcpy(tmp, "swbis_internal_stream_dir_-Please_report_if_seen.");
			prod1->emitLeadingStorageDirectory(tmp);
			strob_close(tmp);	
		}	
		return;
	}

	int write_fd(int fd) {
		int  list_index = 0;
		int cret;
		int ret = 0;
		swExStruct 	* object;
		ret = getReferer()->write_fd(fd);
		if (ret < 0) return ret;

                if (getInfo() != NULL)
                	ret += getInfo()->swdeffile_linki_write_fd(fd);

		while((object = containedByIndex(list_index++))) {
			cret = object->write_fd(fd);
			if (cret < 0) return cret;
			ret += cret;
		}

		return ret;
	}


	int registerWithGlobalIndex(void) {
		swExStruct 	* object;
		int  list_index = 0;
		getGlobalIndex()->swdeffile_linki_append(getReferer());

		while((object = containedByIndex(list_index++))) {
			object->registerWithGlobalIndex();
		}
		return 0;
	}
	
	virtual void setReferer(swDefinition * swdef){ 
		swExStruct_i::setReferer(swdef);
		if (getXfiles())
			getXfiles()->setReferer(swdef);
	}

	/** processForExport_step1 -
	 *  @swheader: The iterator/search object.
	 *  @psf: The PSF file.
	 *  @current_level: The current level of the calling object.
	 *  @current_line: The current line of the calling object.
	 *
	 *  Does the work of allocating the swExStruct tree. The files are split off
	 *  into INFO files and the global INDEX is constructed.
	 */

	swDefinition * processForExport_step1(SWHEADER * swheader, swDefinitionFile * psf, int current_level, 
								char * current_line, int *error_code) 
	{
		int type;
		int level;
		char *next_line;
		char * keyword;
		swExStruct * swex = NULL;
		swMetaData * swmd = NULL;
		swMetaData * previous_swmd;
		swDefinition * current_swmd;
		swDefinition * last_def = NULL;
		swDefinition * first_def;
		swDefinition * uplink = NULL;
		swDefinition * inext;
		swDefinition * last_inext;
		
		*error_code = 0;
		swmd = psf->swdeffile_linki_find_by_parserline(NULL, current_line);
		SWLIB_ASSERT(swmd != NULL);
			
		SWLIB_ASSERT(swmd->get_type() == swstructdef::sdf_object_kw);
		current_swmd = static_cast<swDefinition*>(swmd);
		setReferer(static_cast<swDefinition*>(current_swmd));
		last_def = current_swmd;
		E_DEBUG("ENTERING");
		E_DEBUG3("this=[%s] parserline address=[%p]", this->getObjectName(), current_swmd->get_parserline());

		next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
		while (next_line){
			E_DEBUG2("next_line=[%s]", next_line);
			swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
				"Export_step1: next_line=[%s]\n", next_line);
			previous_swmd = swmd;
			swmd = psf->swdeffile_linki_find_by_parserline(NULL, next_line);
			SWLIB_ASSERT(swmd != NULL);
			type = swmd->get_type();
			SWLIB_ASSERT(type == swstructdef::sdf_object_kw);
			level = swmd->get_level();
			keyword = swmd->get_keyword();
			if (level <= current_level) {
				//
				// end of the object scope.
				//
				SWLIB_ASSERT(swmd->get_type() == swstructdef::sdf_object_kw);
				uplink =  static_cast<swDefinition*>(swmd);
				break;
			}

			uplink = NULL;
			if (doesHaveControlFiles(keyword)) {
				//
				// e.g. A product and fileset have control files.
				//

				E_DEBUG("");
				uplink = process_file_and_control_file_definitions(static_cast<swDefinition*>(swmd), 
							swheader, psf, last_def, error_code);
				E_DEBUG("");
				if (uplink) {
					E_DEBUG("");
					SWLIB_ASSERT(uplink->get_type() == swstructdef::sdf_object_kw);
					last_inext = last_def;
					inext = last_def->get_next();
					while(inext) {
						if (swlib_get_verbose_level() >= SWPACKAGE_VERBOSE_V3) {
							inext->write_fd_debug(
								swfdio_get(STDERR_FILENO),
								"Export_step1.1:");
						}
						if (inext == uplink) break;
						last_inext = inext;
						inext = inext->get_next();
					}
	        			swheader_set_current_offset_p_value(swheader, last_inext->get_ino());
				} else {
					;
					// A product with control_files but no fileset.
				}

			} else if (doesContain(keyword)) {
								
				//
				// Allocate the new contained object.
				//
				SWLIB_ASSERT(type == swstructdef::sdf_object_kw);
				first_def = static_cast<swDefinition*>(swmd);
				
				//fprintf(stderr, "BEFORE [%s] [%s]\n", keyword, next_line);
				swex = swExFactory(keyword);
				swex->setInfo(NULL);
				//fprintf(stderr, "AFTER [%s] [%s]\n", keyword, next_line);
				SWLIB_ASSERT(swex != NULL);
				swex->setReferer(static_cast<swDefinition*>(swmd));
				uplink = swex->processForExport_step1(swheader, psf, level, next_line, error_code);
				containedListAdd(swex);

				if (uplink) {
					SWLIB_ASSERT(uplink->get_type() == swstructdef::sdf_object_kw);

					//
					// Now set the inode to definition before 'uplink'
					// Start at 'last_def' and step down until 'uplink' is
					// found.
					//

					last_inext = last_def;
					inext = last_def->get_next();
					while(inext) {
						if (swlib_get_verbose_level() >= SWPACKAGE_VERBOSE_V3) {
							inext->write_fd_debug(
								swfdio_get(STDERR_FILENO),
								"Export_step1.2:");
						}
						if (inext == uplink) break;
						last_inext = inext;
						inext = inext->get_next();
					}
					if (last_inext)
		        			swheader_set_current_offset_p_value(swheader, last_inext->get_ino());

				} else {
					//
					// End of PSF ??, normal, or,  object not contained.
					//
					;
				}
				last_def = static_cast<swDefinition*>(swmd);
			}
			next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
		}
		E_DEBUG2("uplink = [%p]", uplink);
		E_DEBUG("LEAVING");
		return uplink;
	}
	
	/** contructSwExDist - Construct a swExport object from a PSF.
	 *  @psf: The PSF file object. 
	 *
	 *  Process the PSF file which has already had extended definitions
	 *  expanded.   Scan and split up the PSF into pieces storing them
	 *  in the correct place in the swExDistribution object.
	 *  Return the swExDistribution object, NULL on error.
	 *  After Execution the psf object has been modified.
	 */

	static swExCat * 
	constructSwExDist(swDefinitionFile * psf, int * error_code, struct extendedOptions * opta)
	{
		char * current_line;
		swExCat * swexdist;
		SWHEADER 	 * swheader;

		//
		// Hack:
		// Alloc ahead 10000 bytes so a call to realloc() in uxfio.c that
		// relocates the mem block does not trip up these routines.
		// Make it a fatal error if the memory is moved.
		// Memory is only added for INFO file objects 
		// so the limit is on the number of product and fileset
		// objects. (100's of these could be allocated in 10000 bytes)
		// and most packages have only between 1 and 10.
		//
		uxfio_fcntl(psf->get_mem_fd(), UXFIO_F_DO_MEM_REALLOC, 10000);
		uxfio_fcntl(psf->get_mem_fd(), UXFIO_F_SET_LOCK_MEM_FATAL, 1);

		//
		// Make a swexdist for the first object.
		// which is either a host or distribution object.
		//
		current_line = psf->swdeffile_get_pointer_from_index(0)->get_keyword();
		if (strcmp(current_line, "host") != 0 && strcmp(current_line, "distribution") != 0) {
			return NULL;
		}
		E_DEBUG("");
		swexdist = static_cast<swExCat*>(swExFactory(current_line));  // Warning : Downcast !!
		swexdist->setPSF(psf);
		swexdist->set_opta_array(opta);
		
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swexdist->getStderrFd(),
				"running constructSwExDist()\n");

		//
		// Need to initialize the link-lists in the <swDefinitionFile*> object.
		//

		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swexdist->getStderrFd(),
				"initializing: running psf->swdeffile_linki_init()\n");

		E_DEBUG("");
		psf->swdeffile_linki_init();
		
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swexdist->getStderrFd(),
				"initializing: running psf->swdeffile_linki_init() Done\n");
	
		//
		// Contruct the iterator and search object.
		//
		E_DEBUG("");
		swexdist->set_swheader_offset(0);
		swheader=swheader_open(swDefinitionFile::swdeffile_linki_nextline, static_cast<void*>(psf));
		swheader_set_current_offset_p(swheader, swexdist->get_swheader_offset_p());
		swheader_set_current_offset_p_value(swheader, swexdist->get_swheader_offset());
		swheader_reset(swheader);
		
		E_DEBUG("");
		current_line = swheader_goto_next_line(swheader, swexdist->get_swheader_offset_p(), SWHEADER_GET_NEXT);

		//
		// Current_line should be the "distribution" or "host" object keyword.
		//

		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swexdist->getStderrFd(),
				"running psf->processForExport_step1()\n");
		swexdist->processForExport_step1(swheader, psf, 0, current_line, error_code);
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swexdist->getStderrFd(),
				"running psf->processForExport_step1() Done\n");
	
		//
		// Set the mem_fd back to its normal state.
		// Allow the buffer to be realloc'ed to a new location.
		//
		uxfio_fcntl(psf->get_mem_fd(), UXFIO_F_SET_LOCK_MEM_FATAL, 0);

		E_DEBUG("");
		//
		// Set the global INDEX;
		//
		swexdist->createIndexFile();
		swexdist->setGlobalIndex(swexdist->getIndex());

		if (*error_code == 0) {
			//
			// do some initializations.
			//
			swexdist->registerWithGlobalIndex();
		}
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swexdist->getStderrFd(),
				"running constructSwExDist() Done\n");
		E_DEBUG("");
		return (swexdist);
	}
	

	// virtual char * dump_string_s(char * prefix); //		return swExCat::swexcat_dump_string_s(this, prefix); 

	virtual swExStruct * getXfiles(void) { return xFilesM; }
	
	swExStruct * getDistribution(void) {
		swExStruct * object;

		//
		// this should be either a host or distribution object.
		//
		if (::strcmp(getObjectName(), "distribution") == 0) {
			return this;	
		}

		object = containedByIndex(0);
		if (::strcmp(object->getObjectName(), "distribution") != 0) {
			//
			// Fatal error.
			//
			return NULL;
		}
		return (object);
	}
	
	virtual void emitStorageStructure(void) { }

	virtual void emitExportedCatalog(void) { }

  private:

	void init(void) {
		sbufM = strob_open(10);
	}

};
#endif
