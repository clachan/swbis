/* swobjfiles_i.h - The common (dfile/pfiles/fileset) exported file structure.
 */

/*
 * Copyright (C) 2003,2004,2005,2006  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swobjfiles_i_hxx
#define swobjfiles_i_hxx

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#include "swuser_assert_config.h"
#include "swmetadata.h"
#include "swdefinition.h"
#include "swexstruct_i.h"
#include "swspsf.h"
#include "swexpsf.h"
#include "swinfo.h"
#include "swptrlist.h"
#include "swpackagefile.h"
#include "swcontrolscript.h"
#include "swattributefile.h"
#include "swdefinitionfile.h"
#include "swdefcontrolfile.h"
#include "swcatalogfile.h"

extern "C" {
#include "swlib.h"
#include "swutilname.h"
#include "uxfio.h"
#include "cplob.h"
#include "strob.h"
#include "swvarfs.h"
}

#define INFO_SIZE_BLANK	"\040\040\040\040\040\040\040\040"  // Eight (8) Spaces

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

class swObjFiles;
class swObjFiles_i:  public swExStruct_i
{
    private:
	static swDefinitionFile * psfM;
	static int swheader_offsetM;
	static swINDEX * global_indexM;
	swINFO * infoM;
	swINDEX * indexM;
	swPtrList<swExStruct> * containsM;	// Lower level contained object.
	swPtrList<swPackageFile> * controlFilesM;	// Control scripts.
	swPtrList<swPackageFile> * attributeFilesM;	// Attribute files.
	STROB * sbufM;					// Used for debugging.	
    	static SWVARFS * swvarfsM;
	static STROB * catalog_ownerM;
	static STROB * catalog_groupM;
	static STROB * leading_dir_ownerM;
	static STROB * leading_dir_groupM;
	static STROB * leading_dir_c_modeM;
	static STROB * catalog_dir_c_modeM;
	static STROB * catalog_file_c_modeM;
 	static mode_t  current_dir_modeM;
    public:
	
	swObjFiles_i(char * control_dir): swExStruct_i(control_dir) {
		initP();
	}
	
	virtual ~swObjFiles_i(void) {
		swPackageFile *pf;
		int i;
		delete infoM;

		//
		// FIXME : delete the containsM
		//
		if (controlFilesM) {
			i=0;
			while ((pf=controlFilesM->get_pointer_from_index(i++)))
					delete pf;
		}
		if (attributeFilesM) {
			i=0;
			while 
			  ((pf=attributeFilesM->get_pointer_from_index(i++)))
				delete pf;
		}
		::strob_close(sbufM);
	}

	//
	// Subordinates (Contained objects) list.
	//
	virtual swExStruct * containedByIndex(int index) {
		return containsM->get_pointer_from_index(index);
	}

	virtual int containedListAdd(swExStruct * o) {
		return containsM->list_add(o);
	}

	//char * swptrlist_dump_string_s(char * prebuf, STROB * ptrbuf);
	
	virtual swPtrList<swPackageFile> * getControlScriptList(void) {
		return controlFilesM; 
	}

	virtual swPtrList<swPackageFile> * getAttributeFileList(void) {
		return attributeFilesM; 
	}

	virtual swPtrList<swPackageFile> * getDistributionFileList(void) {
		return NULL;
	}

	virtual swPtrList<swPackageFile> * getFileListObject(void) {
		return getControlScriptList();
	}
	
	int get_swheader_offset(void) {
		return swheader_offsetM;
	}
	
	int * get_swheader_offset_p(void) {
		return &swheader_offsetM;
	}
	
	void set_swheader_offset(int a) {
		swheader_offsetM = a;
	}
	
	virtual void setIndex(swINDEX * a) { indexM = a; }
	
	virtual void setGlobalIndex(swINDEX * a) { global_indexM = a; }
	
	virtual swINDEX * getEmittedGlobalIndex(void) {
		return getGlobalIndex();
	}
	
	char * getCatalogOwner(void) {
		return strob_str(catalog_ownerM); 
	}

	char * getCatalogGroup(void) {
		return strob_str(catalog_groupM);
	}

	void setCatalogOwner(char * name) {
		strob_strcpy(catalog_ownerM, name);
	}

	void setCatalogGroup(char * name) {
		strob_strcpy(catalog_groupM, name);
	}

	char * getLeadingDirOwner(void) {
		return strob_str(leading_dir_ownerM);
	}

	char * getLeadingDirGroup(void) {
		return strob_str(leading_dir_groupM);
	}

	char * getLeadingDirModeString(void) {
		return strob_str(leading_dir_c_modeM);
	}

	mode_t getCurrentDirMode(void) {
		return current_dir_modeM;
	}

	char * getCatalogDirModeString(void) {
		return strob_str(catalog_dir_c_modeM);
	}

	char * getCatalogFileModeString(void) {
		return strob_str(catalog_file_c_modeM);
	}

	void setLeadingDirOwner(char * name) {
		strob_strcpy(leading_dir_ownerM, name); }
	void setLeadingDirGroup(char * name) {
		strob_strcpy(leading_dir_groupM, name); }
	void setLeadingDirMode(char * name) {
		strob_strcpy(leading_dir_c_modeM, name); }
	void setCatalogDirModeString(char * name) {
		strob_strcpy(catalog_dir_c_modeM, name); }
	void setCatalogFileModeString(char * name) {
		strob_strcpy(catalog_file_c_modeM, name); }
	void setCurrentDirMode(mode_t m) { current_dir_modeM = m; }
	
	swINDEX * getGlobalIndex(void) { return global_indexM; }
	
	virtual void setInfo(swINFO * a) { infoM = a; }
	
	virtual swINDEX * getIndex(void) { return indexM; }

	virtual swINFO * getInfo(void) { return infoM; }
	
	virtual swDefinitionFile * getPSF(void) { return psfM; }
	
	SWVARFS * getSwvarfs(void) { return swvarfsM; }
	
	void setSwvarfs(SWVARFS * a) { swvarfsM = a; }
	
	virtual void setPSF(swDefinitionFile * psf) { psfM = psf; }
	
	virtual int doesHaveControlFiles(char * key) { return 0; }

	virtual int doesContain(char * key) {
		return 0;
	}

	void 
	formCatalogDirModeString(void) { 
		char buf[40];
		mode_t cwdmode;
		mode_t finalperms;
		mode_t x;

		cwdmode =  getCurrentDirMode();
	
		x = cwdmode & (S_IRWXU | S_IRWXG | S_IRWXO |
					S_ISUID | S_ISGID | S_ISVTX);
		//
		// This determines the permission mode of the control
		// directories.
		finalperms = 0755;
		if (x != 0) {
			finalperms = x;
			if (cwdmode & S_ISUID) finalperms |= S_ISUID;
			if (cwdmode & S_ISGID) finalperms |= S_ISGID;
			if (cwdmode & S_IROTH) finalperms |= (S_IROTH|S_IXOTH);
			if (cwdmode & S_IWOTH) finalperms |= (S_IWOTH|S_IXOTH);
			if (cwdmode & S_IRGRP) finalperms |= (S_IRGRP|S_IXGRP);
			if (cwdmode & S_IWGRP) finalperms |= (S_IWGRP|S_IXGRP);
		} 
		snprintf(buf, sizeof(buf), "%o", (unsigned int)finalperms);
		buf[sizeof(buf) - 1] = '\0';
		setCatalogDirModeString(buf);
	}

	void 
	formCatalogFileModeString(void) { 
		char buf[40];
		mode_t cwdmode;
		mode_t finalperms;
		mode_t x;

		cwdmode =  getCurrentDirMode();
	
		x = cwdmode & (S_IRWXU | S_IRWXG | S_IRWXO | 
					S_ISUID | S_ISGID | S_ISVTX);
		//
		// this determines the permissions of internally generated 
		// control_files such as INFO
		//
		finalperms = 0644;

		if (x != 0) {
			finalperms = x;
			finalperms &= ~(S_IXOTH | S_IXGRP | S_IXUSR);
			if (cwdmode & S_ISUID) finalperms &= ~S_ISUID;
			if (cwdmode & S_ISGID) finalperms &= ~S_ISGID;
			if (cwdmode & S_IROTH) finalperms |=  S_IROTH;
			if (cwdmode & S_IWOTH) finalperms |=  S_IWOTH;
			if (cwdmode & S_IRGRP) finalperms |=  S_IRGRP;
			if (cwdmode & S_IWGRP) finalperms |=  S_IWGRP;
		} 
		snprintf(buf, sizeof(buf), "%o", (unsigned int)finalperms);
		buf[sizeof(buf) - 1] = '\0';
		setCatalogFileModeString(buf);
	}

	int write_filename(void){
		int i=0;
		char *name;
		swPackageFile * pf;
		swPtrList<swPackageFile> * files = getFileListObject();
		if (files) {
			while ((pf=files->get_pointer_from_index(i++))) {
				name = pf->swp_get_pkgpathname();
				fprintf(stderr,"%s\n", name);
			}
		}
		return 0;
	}

	virtual 
	int 
	swobjfile_file_out_of_scope(swMetaData * swmd, int current_level) {
		return 1;
	}

	swDefinition * 
	process_file_and_control_file_definitions(swMetaData * swmmd,
						SWHEADER * swheader, 
						swDefinitionFile * psf,
						swDefinition * last_def,
						int * error_code) {
		int type;
		int level;
		swPtrList<swDefinition> * psf_global_list;
		swINFO * info;
		swDefinition * swmd;
		swDefinition * previous_swmd;
		swDefinition * first_file;
		swDefinition * after_last_file;
			
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
			"swObjfiles_i::process_file_and_control_file_definitions: ENTERING\n");
		info = new swINFO("");
		SWLIB_ASSERT(info != NULL);
		setInfo(info);
	
		if (!swmmd) {
			//
			// Special case : fileset at end of PSF with no files.
			//
			return NULL;
		}
		
		SWLIB_ASSERT(swmmd->get_type() == swstructdef::sdf_object_kw);
		swmd = static_cast<swDefinition *>(swmmd);
		previous_swmd = swmd;
		SWLIB_ASSERT(swmd != NULL);
		type = swmd->get_type();
		level = swmd->get_level();
	
		//
		// Allocate the new contained object which are either
		// file or control_file objects.
		//
		SWLIB_ASSERT(type == swstructdef::sdf_object_kw);
		first_file = static_cast<swDefinition*>(swmd);


		//
		// Assign the global list to this object.
		//
		delete info->get_swdefinition_list();
		psf_global_list = getPSF()->get_swdefinition_list();
		info->set_swdefinition_list(psf_global_list);
				
		//
		// Set the head of the linked list.
		//
		info->swdeffile_linki_set_head(first_file);
		info->swdeffile_linki_set_tail(NULL);

		//
		// Now read all the files and control_files and return the
		// first out of scope object.
		//

		after_last_file =
			swObjFiles_i::process_files_step1(swheader, psf,
					info, level, error_code, first_file);
		if (after_last_file) {
			SWLIB_ASSERT(after_last_file->get_type() ==
						swstructdef::sdf_object_kw);
			//
			// Now link the first and last together.
			//
			last_def->set_next(after_last_file);

			//
			// terminate the INFO file, where after_last_file
			// is a new fileset.
			//
			if (first_file->get_level() > 
					after_last_file->get_level()) {

				after_last_file->get_prev()->set_next(NULL);

			} else if (first_file->get_level() ==
					after_last_file->get_level()) {
				
				after_last_file->get_prev()->set_next(NULL);
			
			} else {
				//
				// should never happen.
				//
				SWLIB_ASSERT(0);
			}

			//
			// Connect the Next file to the previous definition.
			//
			after_last_file->set_prev(last_def);

			first_file->set_prev(NULL);
       			swheader_set_current_offset_p_value(swheader,
							last_def->get_ino());
		} else {
			last_def->set_next(NULL);
			first_file->set_prev(NULL);
		}
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
			"swObjfiles_i::process_file_and_control_file_definitions: LEAVING\n");
		return after_last_file;
	}


	swDefinition *
	process_files_step1(SWHEADER * swheader, swDefinitionFile * psf, 
						swDefinitionFile * info,
						int current_level,
						int * error_code,
						swDefinition * first_file)
	{
		swDefinition * ret = NULL;
		int did_find_duplicate;
		char * next_line;
		char * path;
		char * tmpname;
		swMetaData * previous_swmd = NULL;
		swMetaData * swmd = NULL;
		swMetaData * swat = NULL;
		swDefinition * swmd_def = NULL;
		CPLOB * filedeflist = cplob_open(10);
		CPLOB * pathlist = cplob_open(10);
		STROB * basenames = strob_open(100);
			
		//
		// The first file definition was already read.
		// So process it here.
		//
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
			"swObjfiles_i::process_files_step1: ENTERING\n");
		SWLIB_ASSERT(first_file->get_type() == swstructdef::sdf_object_kw);
		swat = first_file->findAttribute(SW_A_path);
		if (swat) {
			path = swat->get_value(NULL);
			SWLIB_ASSERT(path != NULL);
			cplob_add_nta(filedeflist, (char*)first_file);
			cplob_add_nta(pathlist, (char*)path);
		} else {
			;
		}
		//
		// Now step through all the file definitions.
		//

		next_line = swheader_get_next_object(swheader,
							(int)UCHAR_MAX,
							(int)UCHAR_MAX);
		while (next_line){
			E_DEBUG2("next_line=[%s]", next_line);
			swmd = psf->swdeffile_linki_find_by_parserline(NULL,
								next_line);
			SWLIB_ASSERT(swmd != NULL);
			if (swobjfile_file_out_of_scope(swmd, current_level)) {
				//
				// end of the scope of the list of
				// control_files or files.  The out of scope
				// test for fileset:files is different from
				// the test for product:control_files,fileset,
				// hence the need for the virtual
				// function swobjfile_file_out_of_scope().
				//
				SWLIB_ASSERT(swmd->get_type() ==
						 swstructdef::sdf_object_kw);
				ret = static_cast<swDefinition*>(swmd);
				break;
			}	
			SWLIB_ASSERT(swmd->get_type() == swstructdef::sdf_object_kw);
			swat = (static_cast<swDefinition*>(swmd))->findAttribute(SW_A_path);
			if (swat) {
				path = swat->get_value(NULL);
				E_DEBUG2("path=[%s]", path);
			} else {
				path = NULL;
			}

			if (path == NULL) {
				//
				// This may happen when a file object is specified as a
				// file definition using the 'source' attribute alone.
				//
				swat = (static_cast<swDefinition*>(swmd))->findAttribute(SW_A_source);
				if (swat) {
					path = swat->get_value(NULL);
					E_DEBUG2("path=[%s]", path);
				} else {
					path = NULL;
				}
			}

			//
			// Now check for duplicate file definitions as required
			// by the POSIX spec.
			//

			// fprintf(stderr, "eraseme swpackage: checking for duplicate here [%s]\n", path);
			// swmd->write_fd(2);  // Heres howto write the definition to STDERR_FILENO

			if (
				get_opta_array() == NULL ||
				::swextopt_is_option_true(SW_E_swbis_check_duplicates, get_opta_array())
			) {
				E_DEBUG2("finding duplicates of pathname [%s]", path);
				did_find_duplicate = check_for_storage_section_duplicates(filedeflist, pathlist, path, basenames);
			} else {
				E_DEBUG2("Not even looking for duplicates for pathname [%s]", path);
				did_find_duplicate = 0;
			}
			swmd_def = static_cast<swDefinition*>(swmd);
			//
			// The static cast above is safe because the type assertion
			// succeeded above.
			//

			if (did_find_duplicate) {
				//
				// The current swdefinition is a duplicate.
				// Presumeably the first occurance was marked SWDEF_STATUS_DUP0
				// by check_for_storage_section_duplicates()
				//
				// Here, (swDefinition*)(swmd) represents the current last
				// duplicate found so far.
				//
				E_DEBUG("Duplicate found");
				E_DEBUG2("setting storage status SWDEF_STATUS_DUP for %s", path);
				swmd_def->set_storage_status(SWDEF_STATUS_DUP);
			} else {
				;
				E_DEBUG("Duplicate *NOT* found");
			}	

			swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
				"swObjfiles_i::process_files_step1: next_line=[%s] path=%s\n", next_line, path ? path : "");
	
			if (path) {
				E_DEBUG("");
				cplob_add_nta(filedeflist, (char*)swmd_def);
				cplob_add_nta(pathlist, (char*)path);
				tmpname = ::strrchr(path, '/');
				if (
					tmpname && 
					strlen(tmpname) > 1 &&
					*(tmpname + 1) != '/' &&
					*(tmpname + 1) != '.'
				) {
					strob_strcat(basenames, tmpname + 1);
				} else {
					strob_strcat(basenames, path);
				}	
				strob_strcat(basenames, "\n");
			}

			previous_swmd = swmd;
			next_line = swheader_get_next_object(swheader,
							(int)UCHAR_MAX,
							(int)UCHAR_MAX);
		}
		if (previous_swmd) {
			//
			// previous_swmd is the last file or control_file
			// object. Terminate the linked list at the last
			// file or control_file.
			//
			E_DEBUG("");
			static_cast<swDefinition*>(previous_swmd)
							->set_next(NULL);
			info->swdeffile_linki_set_tail(
				static_cast<swDefinition*>(previous_swmd)
							);
		} else {
			//
			// OK , has only one (1) file in the fileset.
			//
			;
		}
		E_DEBUG("");
		cplob_shallow_close(filedeflist);
		cplob_shallow_close(pathlist);
		strob_close(basenames);
		swlib_doif_writef(swlib_get_verbose_level(), SWPACKAGE_VERBOSE_V3, NULL, swfdio_get(STDERR_FILENO),
			"swObjfiles_i::process_files_step1: LEAVING\n");
		return ret;
	}

	//
	// Determine if object contains the requested key object.
	//
	virtual swExStruct *
	findBySwDef(swDefinition * swdef) {
		int index = getInfo()->swdeffile_get_index_from_pointer(swdef);
		if (index < 0) return static_cast<swExStruct*>(NULL);
		return this;
	}

	virtual swPackageFile *
	swPackageFileFactory(char * path, char * source) {
		if (swControlScript::is_controlscript(path)) {
			return new swControlScript(path, source);
		} else if ( ::strcmp(path, "INDEX")==0 ||
					::strcmp(path, "INFO")==0) {
			return new swDefinitionFile(path, source);
		} else {
			return new swAttributeFile(path, source);
		}
	}
   
	virtual swDefinition * getPrevious(void) {
		return NULL;
	}

	virtual void setPrevious(swDefinition * previous) {
		return;
	}

	void setLeadingPackagePath(char * leading_path) {
		int  list_index = 0;
		swExStruct 	* object;
		swExStruct	* xfiles;  // Pfiles or Dfiles object.
		char * control_dir;	

		control_dir = determineControlDirectory();
		if (control_dir == NULL) {
			fprintf(stderr, 
		"PSF error: Object %s has undetermined control directory.\n",
							getObjectName());
			exit(1);
		}
		setControlDirectory(control_dir);
		swExStruct_i::appendToParentPath(leading_path);

		xfiles = getXfiles();
		if (xfiles) {
			xfiles->setLeadingPackagePath(getFormedControlPath());
		}

		while((object = containedByIndex(list_index++))) {
			object->setLeadingPackagePath(getFormedControlPath());
		}
	}

	void addControlFileToINFOforINFO(void) {
		swDefinition * inext;
		swMetaData * att;
		swINFO * info = getInfo();
		swDefinition * xcfile;	
		STROB * tmp;

		if (info == NULL) {
			return;
		} else {
			;
		}

		inext = info->swdeffile_linki_get_head();

		while(inext) {
				// The first attribute in the definition.
			att = inext->get_next_node();
			while (att) {
				if (::strcmp(att->get_keyword(), SW_A_path) == 0) {
					if (::strcmp(att->get_value(NULL),
								"INFO") == 0) {
						//
						// This INFO file already
						// has a control_file def
						// for the INFO file.
						//
						return;	
					}
				}
				att = att->get_next_node();
			}
			inext = inext->get_next();
		}
	
		//
		// Make a control_file definition.
		//
		xcfile = swDefinition::make_newDefinition(getReferer(), SW_A_control_file);
		xcfile->set_level(3);
		xcfile->set_no_stat(1);
		xcfile->add(SW_A_source, "INFO");
		att = xcfile->findAttribute(SW_A_source);
		if (!att) {
			setErrorCode(10132, NULL);
		} else {
			att->vremove();
		}
		xcfile->add(SW_A_path, "INFO");
		xcfile->add(SW_A_tag, "INFO");
		xcfile->add(SW_A_size, INFO_SIZE_BLANK);
		tmp = strob_open(10);
		strob_sprintf(tmp, 0, "%lu", getCreateTime());
		xcfile->add(SW_A_mtime, strob_str(tmp));
		xcfile->add(SW_A_mode, getCatalogFileModeString());
		strob_close(tmp);
		info->swdeffile_linki_preppend(xcfile);	
		return;
	}
	
	void setInfoSizes(void) {
		char * tagvalue;
		char * value;
		char asize[30];
		int size;
		swDefinition * inext;
		swINFO * info;

		info = getInfo();
		if (info == NULL) {
			return;
		}

		size = info->swdeffile_linki_write_fd(UXFIO_NULL_FD);

		SWLIB_ASSERT(size > 0);
		if (size <= 0) {
			setErrorCode(10301, NULL);
			return;
		}

						// Eight chars wide.
		snprintf(asize, sizeof(asize) -1, "%8d", size);
		asize[sizeof(asize) - 1] = '\0';

		//
		// Now find the INFO file control_file definition.
		//

		inext = info->swdeffile_linki_get_head();
		while(inext) {
			tagvalue = inext->find(SW_A_tag);
			if (::strcmp(tagvalue, "INFO") == 0) {
				value = inext->find(SW_A_size);
				if (value == NULL) {
					setErrorCode(10302, NULL);
					return;
				}
				::strncpy(value, asize,
						strlen(INFO_SIZE_BLANK));		
				break;
			}			
			inext = inext->get_next();
		}
		return;
	}

	void addFileStatsToFileDefinitions(void)
	{
		int ret;
		swDefinition * inext;
		swINFO * info = getInfo();
		char * type;
		char * complete_value;
		SWVARFS * swvarfs = getSwvarfs();
		swPackageFile * archiver = getArchiver();
		int cksumflags = getCksumFlags();
		char * noesc_source = (char*)NULL;
		char * dupsource = (char*)NULL;
		int did_dup;

		if (info == NULL) {
			return;
		}	
	
		SWLIB_ASSERT(archiver != NULL);

		inext = info->swdeffile_linki_get_head();

		while(inext) {
			complete_value = inext->find("_complete");
			if (complete_value == NULL ||
					strcmp(complete_value, "true") != 0) {
				//
				// Add the file stats.
				//
				noesc_source = inext->find(SW_A_source);
				if (noesc_source == NULL) {
					fprintf(stderr, "swpackage: source attribute is missing\n");
					SWLIB_ASSERT(noesc_source != NULL);
				}	

				if (strstr(noesc_source, "\\x")) {
					dupsource = strdup(noesc_source);
					swlib_process_hex_escapes(dupsource);
					swlib_unexpand_escapes(NULL, dupsource);
					did_dup = 1;
				} else {
					dupsource = noesc_source;
					did_dup = 0;
				}

				type = inext->find(SW_A_type);
				if (type && (*type == SW_ITYPE_b || *type == SW_ITYPE_c)) {
					check_device_files_for_no_stat(inext);
				}

				ret = archiver->apply_file_stats(inext, swvarfs, dupsource, cksumflags);
				if (did_dup) free(dupsource);

				if (ret == -9991 && get_allow_missing_files()) {
					//
					// This code path handles files that have no storage
					// section file.  This happens when translating
					// some RPMS.
					//
					fprintf(stderr,
			"swpackage: Warning: path [%s] will not appear in storage section\n", inext->find(SW_A_source));
					inext->set_storage_status(SWDEF_STATUS_IGN);
					;
				} else if (ret) {
					//
					// This is the code path for a missing source file.
					//
					fprintf(stderr, "swpackage: perhaps the source file is missing: %s\n", inext->find(SW_A_source));
					setErrorCode(10503, NULL);
					break;
				}
			}
			inext = inext->get_next();
		}
		return;
	}


	static swExStruct * swExFactory(char * object_keyword);
	
	virtual 
	swDefinition * processForExport_step1(SWHEADER * swheader,
					swDefinitionFile * psf, int level,
					char * current_line, int * error_code)
	{ 
		return NULL;
	}

	swDefinition * getDistributionFromGlobalIndex(void) {
		int itry = 0;
		swDefinition * first = 
				getGlobalIndex()->swdeffile_linki_get_head();
		//
		// The first one might be Host defintiion.
		//
		while (itry < 2 && first && 
			::strcmp(first->get_keyword(), "distribution") != 0) {
			first=first->get_next();
			itry++;
		}
		if (itry >= 2 || first == NULL) {
			fprintf(stderr,
     "getDistributionFromGlobalIndex: Fatal internal implementation error.\n");
			exit(1);
		}
		return first;
	}
	
	virtual void addControlDirectoryAttribute(void) {
		char * p, *tag;
		swDefinition * swdef = getReferer();

		//
		// Take this opportunity to make sure the tag
		// value and control_directory is legal according to POSIX
		//
		tag = swdef->find(SW_A_tag);
		if (!tag) {
			setErrorCode(10347, NULL);
			return;
		}

		if (swlib_check_legal_tag_value(tag)) {
			strob_sprintf(sbufM, 0, "illegal characters is tag value: %s", tag);
			setErrorCode(10348, strob_str(sbufM));
		}

		//
		// FIXME check the string lengths against the POSIX limits
		//
		p = swdef->find(SW_A_control_directory);
		if (p) return;
		swdef->add(SW_A_control_directory, tag);
	}
	
	void setupAttributeFiles(void) {
		createAttributeFiles();
		registerAttributeFiles();
	}

	//
	// Walk through the INFO file and make a controlScriptM
	// package file for each control script.
	//
	// This routine should be run before the attribute files
	// are added as control_files.
	//
	void createControlScriptFiles(void) {
		int ret;
		char * path;
		char * tag;
		char * pkgname;
		char * source;
		char * keyword;
		STROB * tmp;
		swDefinition * swdef;
		swDefinitionFile * info;
		swCatalogFile * swattfile;
		swPackageFile * archiver;
		swPtrList<swPackageFile> * archiveMemberList;

		archiveMemberList = getControlScriptList();
		if (!archiveMemberList) return;	
		
		archiver = getArchiver();
		tmp = strob_open(10);
		swdef = getReferer();

		SWLIB_ASSERT(archiveMemberList != NULL);
		
		//
		// Step through the Info file looking for
		// control_files.
		//
	
		info = getInfo();
		if (info == NULL) {
			//strob_close(tmp);
			//return;  // No filesets.
			fprintf(stderr, "DEBUG: objectname = [%s]\n",
							getObjectName());
			SWLIB_ASSERT(info != NULL);
		}

		swdef = info->swdeffile_linki_get_head();
		while(swdef) {
			//
			// If the swdef is a control_file.
			//
			keyword = swdef->get_keyword();
			if (::strcmp(keyword, SW_A_control_file) == 0) {
				//
				// Make a controlScript object.
				// according to naming rules.
				//
				swdef->set_path_attribute
						(swdef->find(SW_A_source));
				swdef->set_tag_attribute();	
				path = swdef->find(SW_A_path);
				tag = swdef->find(SW_A_tag);

				if (path == static_cast<char*>(NULL)) {
					//
					// Section 5.2.12  Lines 730-737
					//
					pkgname = tag;
				} else {
					pkgname = path;	
				}

				if (::strcmp(pkgname, "INFO") != 0) 
				{
					if (swsdflt_get_attr_group(SW_A_product,
								pkgname)
							!= CONTROLFILE_EXT &&
						strcmp(pkgname, "install")	&&
							strcmp(pkgname, "remove"))
					{
						if (getVerboseLevel() >= 2)
							fprintf(stderr, 
		"%s: warning: processing unrecognized control script : %s\n",
								swlib_utilname_get(), pkgname);
					}

					swattfile = new swControlScript(pkgname);
			
					source = swdef->find(SW_A_source);
					SWLIB_ASSERT(source != NULL);
	
					ret = processCatalogFile(swattfile,
								strdup(source),
								swdef, 1);
					if (ret) {
						fprintf(stderr,
						"error processing %s code=%d\n",
						source, ret);
					}
                			archiveMemberList->list_add(swattfile);
				}
			}
			swdef = swdef->get_next();
		}
		strob_close(tmp);
		return;
	}

	void createAttributeFiles(void) {
		char * value;
		char * value2 = NULL;
		int ret;
		char * keyword;
		STROB * tmp;
		swMetaData * swat;
		swDefinition * swdef;
		swCatalogFile * swattfile;
		swPackageFile * archiver;
		swPtrList<swPackageFile> * archiveMemberList;

		archiveMemberList = getAttributeFileList();
		if (!archiveMemberList) return;	
		
		archiver = getArchiver();
		tmp = strob_open(10);
		swdef = getReferer();

		//SWLIB_ASSERT(archiveMemberList != NULL);
		//
		// Now look for attribute values that begin with '<'
		// Loop through the attributes in the definition.
		//
		// Step through the Referer definitition.
		//
		swat = swdef->get_next_node(); 	// Get the first attribute.
		while(swat) {
			value = swat->get_value((int*)(NULL));
			SWLIB_ASSERT(value != NULL);
			if (*value == '<') {
				//
				// Create a attribute file.
				//
				keyword = swat->get_keyword();
				
				//
				// Check that this attribure has not
				// already been processed.
				//
				if (lookup_attribute_file(archiveMemberList,
								keyword)) {
					//
					// Already done.
					//
					swat = swat->get_next_node();
					continue;
				}

				value++;
				while (isspace(*value)) value++;
				//
				// If  *value is a ':' then it is the special internal
				// hack to transfer an a string into a attribute file
				//			
				if (*value == ':') {
					//
					// Dup'ing this fixed a core dump
					// Because the memory ''value'' points to 
					// is subject to reallocation on a size increase.
					//
					value2 = strdup(value);
					value = value2;
				} else {
					//
					// FIXME
					// May have to dup this too to avoid 
					// core dumps.
					//
					value2 = static_cast<char*>(NULL);
				}
	
				swattfile = swObjFiles_i::make_attributefile(keyword, value);

				SWLIB_ASSERT(swattfile != NULL);
				
				swattfile->setAttributeReferer(swat);

				//
				// The attribute value in the INDEX file
				// will be the basename.
				//
			
				//
				// Add these attributes to cause the
				// owner/group to be as we
				// want.
				//
				swdef->add(SWBIS_CATALOG_OWNER_ATT,
							getCatalogOwner(), 1);
				swdef->add(SWBIS_CATALOG_GROUP_ATT,
							getCatalogGroup(), 1);
				
				ret = processCatalogFile(swattfile, strdup(value),
						swdef, 0/* logical off*/);
				if (ret) {
					fprintf(stderr,
					"error processing %s code=%d\n",
					value, ret);
				}

				if (value2) free(value2);
				value2 = static_cast<char*>(NULL);
                		archiveMemberList->list_add(swattfile);
			}
			swat = swat->get_next_node();
		}
		strob_close(tmp);
		return;
	}

	void registerAttributeFiles(void) {
		int nullfd;
		int fd;
		int index;
		int size;
		mode_t filemode;
		swMetaData * att;
		swMetaData * attReferer;
		swPackageFile * swfile;
		swDefinition * xcfile;
		STROB * tmp;
		swINFO * info;
		swPtrList<swPackageFile> * archiveMemberList;

		archiveMemberList = getAttributeFileList();
		tmp = strob_open(10);

		//
		// Now iterate through the Attribute Files
		// and make the entry in the INFO file.
		//

		info = getInfo();
		
		SWLIB_ASSERT(info != NULL);

		index = 0;
		swfile = archiveMemberList->get_pointer_from_index(index++);
		while(swfile) {
			swlib_basename(tmp,
					swfile->swfile_get_package_filename());
			if (lookup_tag(info, strob_str(tmp))) {
				swfile = 
				archiveMemberList->get_pointer_from_index(
								index++);
				continue;
			}
			
					// Hack, reopen the released descriptor.
			fd = swfile->swfile_re_open_public_image_fd();
			if (fd < 0) {
				setErrorCode(10136, NULL);
				strob_close(tmp);
				return;
			}
			nullfd = -1;
			size = swlib_pipe_pump(nullfd,  fd);
			if (size < 0) {
				fprintf(stderr, "nullfd = %d   fd = %d\n",
								nullfd, fd);
				setErrorCode(10138, NULL);
				strob_close(tmp);
				return;
			}

			swfile->swfile_re_close_public_image_fd();

			//
			// Now add the path, size and tag attributes
			// to the INFO file.
			//
			//  The path is the source attribute.
			//  The tag is the keyword.
			//

			//
			// Make a control_file definition.
			//	swattfile->
			//	swfile_set_source_filename(strob_str(tmp));
			//
			xcfile = swDefinition::make_newDefinition(getReferer(), SW_A_control_file);

			strob_strcpy(tmp, "");
			strob_strcat(tmp, swfile->swfile_get_source_filename());

			xcfile->add(SW_A_source, strob_str(tmp));

			att = xcfile->findAttribute(SW_A_source);
			if (!att) {
				setErrorCode(10332, NULL);
			} else {
				att->vremove();
			}

			swlib_basename(tmp,
				swfile->swfile_get_package_filename());

			xcfile->add(SW_A_path, strob_str(tmp));

			//
			// Now set the attributeReferer
			// value to this same string value.
			//
			attReferer = swfile->getAttributeReferer();
			if (attReferer) {
				char * value = attReferer->get_value(NULL);
				if (strlen(value) - 1 >= strob_strlen(tmp)) {
					if (*value != '<') {
						//
						// Sanity check;
						//
						setErrorCode(10333, NULL);
					} else {
						strncpy(value + 1,
							strob_str(tmp),
							strlen(value) - 2);
					}
				} else {
					;
				}
			}
			xcfile->add(SW_A_tag, strob_str(tmp));

			// Add the size attribute
			strob_sprintf(tmp, 0, "%8d", size);
			xcfile->add(SW_A_size, strob_str(tmp));

			// Add the mtime attribute 
			strob_sprintf(tmp, 0, "%lu", (unsigned long)(swfile->xFormat_get_mtime()));
			xcfile->add(SW_A_mtime, strob_str(tmp));

			// Add the mode attribute 
			filemode = swfile->xFormat_get_mode();
			filemode &= ~(S_ISGID | S_ISUID);
			filemode &= (0777);
			strob_sprintf(tmp, 0, "%o", (unsigned int)(filemode));
			xcfile->add(SW_A_mode, strob_str(tmp));

			info->swdeffile_linki_append(xcfile);	
	
			swfile = archiveMemberList->get_pointer_from_index(
								index++);
		}
		strob_close(tmp);
		return;
	}
		
	virtual void emitLeadingCatalogDirectory(STROB * tmp) {	
		//
		// Emit the next directory.
		//
		int ret;
		swlib_add_trailing_slash(tmp);
		ret = swExStruct_i::swexstruct_write_dir(strob_str(tmp), 
			getCreateTime(),
			getCatalogDirModeString(),
			getCatalogOwner(), getCatalogGroup());
		if (ret < 0) setErrorCode(20005, NULL);
	}
	
	//
	// Emit the leading directory of the fileset or product. 
	//
	virtual void emitLeadingStorageDirectory(STROB * tmp) {	
		char * s;
		//
		// IF the control_directorys are "" this test avoid writing the
		// <path>/ directory twice.
		//
		
		s = strstr(strob_str(tmp), getDistributionLeadingPath());
		if (!s) {
			s = strob_str(tmp);
		} else {
			s += strlen(getDistributionLeadingPath());
		}
	
		//
		// s now points to  an slash or null.
		//
	
		if ( (*s == '/' && *(s+1) == '\0') || (*(s+0) == '\0')) {
			//
			//  <path>\0      or    <path>/\0
			//
			// Don't write it,
			;
		} else {
			//
			// Write the directory.
			//
		
			int ret;
			swlib_add_trailing_slash(tmp);
			ret = swExStruct_i::swexstruct_write_dir(
				strob_str(tmp),
				getCreateTime(),
				getCatalogDirModeString(),
				getCatalogOwner(),
				getCatalogGroup()
				);
			if (ret < 0) setErrorCode(20006, NULL);
		}
		return;
	}

	virtual void emitExportedCatalog(void) { 
		swDefinitionFile * info;	
		STROB * tmp = strob_open(100);
		swPtrList<swPackageFile> * archiveMemberList;
		swPackageFile * swfile;
		swPackageFile * nextswfile;
		int index;
		mode_t filemode;
		int is_duplicate;
		STRAR * pathlist = strar_open();
		int ret;
	
		//
		// The fileset is a special case, hence the late dispatch of
		// emitLeadingDirectory().
		//
		strob_strcpy(tmp, getFormedControlPath(static_cast<char*>(NULL), 
					(char*)(SW_A_catalog)));

		emitLeadingCatalogDirectory(tmp);
		
		//
		// Emit the INFO File.
		//
		swlib_unix_dircat(tmp, "INFO");
		info = getInfo();
		
		SWLIB_ASSERT(info != NULL);
		info->swfile_set_package_filename(strob_str(tmp));

		filemode = info->xFormat_get_mode();
		// filemode &= ~(S_ISGID | S_ISUID | S_IXOTH | S_IXGRP | S_IXUSR);
		info->xFormat_set_mode(filemode);
		swexstruct_write_definitionfile(info,
				getCreateTime(),
				getCatalogFileModeString(),
				getCatalogOwner(), getCatalogGroup());

		//
		// Now emit the Attribute files.
		//

		archiveMemberList = getAttributeFileList();
		index = 0;
		swfile = archiveMemberList->get_pointer_from_index(index++);
		while(swfile) {
			nextswfile = archiveMemberList->
					get_pointer_from_index(index);
			emitAttributeFile(swfile, nextswfile);
			swfile = archiveMemberList->
					get_pointer_from_index(index++);
		}
	
		//
		// Now emit the control_scripts.
		//
		
		archiveMemberList = getControlScriptList();
		index = 0;
		swfile = archiveMemberList->get_pointer_from_index(
								index++);
		while(swfile) {
			swfile->swfile_re_open_public_image_fd();
			strob_strcpy(tmp,
				getFormedControlPath(
					static_cast<char*>(NULL),
					(char*)(SW_A_catalog))
				);
			swlib_unix_dircat(tmp,
				swfile->swfile_get_package_filename());
			swfile->swfile_set_package_filename(strob_str(tmp));

			//fprintf(stderr, "eraseme [%s]\n", strob_str(tmp));
			is_duplicate = check_control_file_duplicates(
						pathlist,
						strob_str(tmp));
			if (is_duplicate == 0) {
				strar_add(pathlist, strob_str(tmp));
			}

			// swfile->xFormat_set_perms(0755);
			swfile->xFormat_set_user_systempair(getCatalogOwner());
			swfile->xFormat_set_group_systempair(getCatalogGroup());
			ret = 0;
			if (is_duplicate == 0)
				ret = swfile->xFormat_write_file();
			if (ret < 0) setErrorCode(20008, NULL);
			swfile->swfile_re_close_public_image_fd();
			swfile = archiveMemberList->
				get_pointer_from_index(index++);
		}

		strar_close(pathlist);
		strob_close(tmp);
	}
	
	virtual void emitStorageStructure(void) { }

	void
	writePreviewLine(STROB * buffer, char * filename)
	{
		if (getVerboseLevel() >= SWC_VERBOSE_4) {
			swlib_writef(get_preview_fd(), buffer, "%s\n", filename);
		} else if (getVerboseLevel() >= SWC_VERBOSE_3) {
			swlib_writef(get_preview_fd(), buffer, "%s\n", filename);
		} else if (getVerboseLevel() >= SWC_VERBOSE_1) {
			swlib_writef(get_preview_fd(), buffer, "%s\n", filename);
		} else {
			;
		}
	}

   private:  

	void check_device_files_for_no_stat(swDefinition * inext) {
		if (
			inext->find(SW_A_mode) &&
			(inext->find(SW_A_owner) || inext->find(SW_A_uid)) &&
			(inext->find(SW_A_group) || inext->find(SW_A_gid)) &&
			inext->find(SW_A_major) &&
			inext->find(SW_A_minor) &&
			1
		) {
			inext->set_no_stat(1);
		}
	}

	int
	check_control_file_duplicates(STRAR * pathlist, char * incoming_path)
	{
		int index = 0;
		char * s;
		while((s=strar_get(pathlist, index))) {
			if (strcmp(s, incoming_path) == 0) {
				return 1;
			}
			index++;
		}
		return 0;
	}

	int
	check_for_storage_section_duplicates(CPLOB * filedeflist,
				CPLOB * pathlist,
				char * incoming_path,
				STROB * basenames)
	{
		int ret;
		int retval = 0;
		int index = 0;
		int got_first_duplicate = 0;
		char ** current_path_p;
		char ** path_array = cplob_get_list(pathlist);
		swDefinition * duplicate_def;
		char * incoming_base_name;
		char * tmpname;

		if (!incoming_path) return 0;
		E_DEBUG2("Entering: Looking for %s", incoming_path);

		//
		//  The scheme is to mark duplicates.
		//  Mark the first occurrance with SWDEF_STATUS_DUP0
		//  (This is the location of the file in the storage section).
		//  Mark the duplicate with SWDEF_STATUS_DUP
		//  SWDEF_STATUS_DUP0 status is never replaced with another status.
		//  If a third duplicate is found, transisition the  SWDEF_STATUS_DUP
		//  (i.e. first duplicate) to SWDEF_STATUS_IGN and mark this
		//  duplicate with SWDEF_STATUS_DUP.
		// 
		//  The result should be that, for duplicates, there is exactly
		//  one (1) SWDEF_STATUS_DUP0 status and one (1) SWDEF_STATUS_DUP
		//  status.
		//
		//  (It would be nice to replace the swdef's by swinging head and tail
		//   pointers but the data is traversed in flat file form in some
		//   views, therefore there is no way to delete the duplicate and fill
		//   in the hole.)
		//

		// 
		// Remember, the current swdef is not on the list being searched.
		//


		//
		// check the basenames list.  This provides a much needed speed optimization.
		//
		incoming_base_name = NULL;
		if ( 1 && 
			(tmpname=strrchr(incoming_path, '/')) != static_cast<char*>(NULL) &&
			strlen(tmpname) > 1 && // i.e. not just a trailing slash "/"
			*(tmpname + 1) != '/' &&
			*(tmpname + 1) != '.'
		) {
			incoming_base_name = tmpname + 1;
		}

		//
		// Only attempt this optimization if we get a clean basename.
		//
		E_DEBUG("");
		if (1 && 
			incoming_base_name && *incoming_base_name != '/'
		) {
			if (strstr(strob_str(basenames), incoming_base_name) == static_cast<char*>(NULL)) {
				//
				// We can safely assume that the incoming name has no prior duplicate.
				//
				return 0;
			}
		}

		E_DEBUG("");
		current_path_p = path_array;
		while(*current_path_p) {
			E_DEBUG3("checking for duplicate: current_path :: incoming_path  [%s] :: [%s]", *current_path_p, incoming_path);
			ret = swlib_dir_compare(*current_path_p, incoming_path, SWC_FC_NOAB);
			if (ret == 0) {
				E_DEBUG2("********** Got duplicate ************* %s", *current_path_p);
				//
				// Got duplicate.
				//
				// fprintf(stderr, "eraseme swpackage: got duplicate here [%s]\n", incoming_path);
				
				retval = 1;	
				duplicate_def = (swDefinition*)(cplob_val(filedeflist, index));

				//
				// duplicate_def is prior to the swdefinition represented by the search
				// key 'incoming_path'
				// 

				// DEBUG duplicate_def->write_fd(2);

				E_DEBUG("Duplicate");
				fprintf(stderr, "swpackage: warning: duplicate file definition: %s\n", incoming_path);
				if (duplicate_def->get_storage_status() == SWDEF_STATUS_DUP) {
					E_DEBUG("GOT ONE: 1");
					// duplicate_def->set_storage_status(SWDEF_STATUS_IGN);
					// fprintf(stderr, "eraseme: case SWDEF_STATUS_IGN: [%s]\n", incoming_path);
				} else if (duplicate_def->get_storage_status() == SWDEF_STATUS_DUP0) {
					//
					// Normal path for 1 or more duplicates. 
					//
					// fprintf(stderr, "eraseme: case SWDEF_STATUS_DUP0: [%s]\n", incoming_path);
					E_DEBUG("GOT ONE: 2");
					;
				} else if (duplicate_def->get_storage_status() == SWDEF_STATUS_NOM
					&& got_first_duplicate == 0) {
					//
					// This is the first occurance.
					//
					E_DEBUG("GOT ONE: 3");
					duplicate_def->set_storage_status(SWDEF_STATUS_DUP0);
					// fprintf(stderr, "eraseme: case SWDEF_STATUS_DUP0: [%s]\n", incoming_path);
					got_first_duplicate = 1;
				} else {
					;
				}
				
			} else {
				E_DEBUG3("**** [%s] is not a duplicate of [%s]", *current_path_p, incoming_path);
				;
			}
			index ++;
			current_path_p++;
		}
		return retval;
	}

	void
	emitAttributeFile(swPackageFile * swfile, swPackageFile * next)
	{
		char * name;	
		name = swfile->swfile_get_package_filename();
		if (strstr(name, "sig_header")) {
			tune_signature_header_attribute_file(swfile, next);
			emit_attribute_file(swfile);
		} else {
			emit_attribute_file(swfile);
		}
	}
	
	void
	tune_signature_header_attribute_file(swPackageFile * swfile, 
						swPackageFile * next)
	{
		int old_fd;
		int old_preview_fd;
		int new_fd;
		int datafd;
		int data_len = 0;
		int buffer_len = 0;
		char * name;
		char *s;
        	char * base = (char*)NULL;
		const char * signame = SW_A_signature;
		const char * sigpath = "/" SW_A_signature;
		
		old_fd = get_ofd();	
		old_preview_fd = get_preview_fd();	
		set_preview_fd(getNullFd());	
		new_fd = swlib_open_memfd();
		if (new_fd < 0) {
			set_preview_fd(old_preview_fd);	
			setErrorCode(10338, NULL);
			return;
		}	
		
		//
		// Sanity check.
		// Make sure the next file is the signature
		// member.
		//
		name = next->swfile_get_package_filename();
		if ((s=strstr(name, signame)) == NULL ||
			strlen(s) != strlen(signame)
		) {
			set_preview_fd(old_preview_fd);	
			setErrorCode(10339, NULL);
			return;
		}
	
		//
		// Now emit the signature into new_fd
		//
		set_ofd(new_fd);
		emit_attribute_file(next);
		set_ofd(old_fd);
		set_preview_fd(old_preview_fd);	

		//
		// Now get the first 512 bytes in new_fd and use
		// it to set it as the new value of swfile.
		//

		uxfio_get_dynamic_buffer(new_fd, &base,
					&buffer_len, &data_len);

		//
		// Another sanity check
		// 'base' pounts to a ustar header.
		// FIXME: Assumes the prefix is not used.
		//        (ie the path < 99 octets)
		//
		if ((s=strstr(base, sigpath)) == NULL ||
			strlen(s) != strlen(sigpath)
		) {
			setErrorCode(10340, NULL);
			return;
		}

		//
		// Now set the value to the fisrt 512 bytes of 
		// 'base'
		//
		datafd = swfile->swfile_re_open_public_image_fd();
		if (datafd >= 0) {
			if (uxfio_write(datafd, base, 512) != 512) {
				setErrorCode(10341, NULL);
				return;
			}
			swfile->swfile_re_close_public_image_fd();
		} else {
			//
			// This happens using the --files -p options
			// FIXME ??
			;
		}
		uxfio_close(new_fd);
		return;
	}
	
	void
	emit_attribute_file(swPackageFile * swfile)
	{
		STROB * tmp = strob_open(100);
		mode_t filemode;
	
		swfile->swfile_re_open_public_image_fd();
		strob_strcpy(tmp,
			getFormedControlPath(
				static_cast<char*>(NULL),
				(char*)(SW_A_catalog))
		);

		swlib_unix_dircat(tmp, 
			swlib_basename(static_cast<STROB*>(NULL),
				swfile->swfile_get_package_filename()));
		
		swfile->swfile_set_package_filename(strob_str(tmp));
		
		if (swfile->getArchiveInclude()) {
			int ret;
			filemode = swfile->xFormat_get_mode();
			// filemode &= ~(S_ISGID | S_ISUID | S_IXOTH | S_IXGRP | S_IXUSR);
			swfile->xFormat_set_mode(filemode);
			swfile->xFormat_set_user_systempair(getCatalogOwner());
			swfile->xFormat_set_group_systempair(getCatalogGroup());
			ret = swfile->xFormat_write_file();
			if (ret < 0) setErrorCode(20009, NULL);
		}
		swfile->swfile_re_close_public_image_fd();
		strob_close(tmp);
		return;
	}

	void initP(void) {
		infoM = new swINFO("INFO");
		indexM = NULL;
		containsM = new swPtrList<swExStruct>();
		attributeFilesM = new swPtrList<swPackageFile>();
		controlFilesM = new swPtrList<swPackageFile>();
		sbufM = ::strob_open(10);
		if (catalog_ownerM == static_cast<STROB *>(NULL))
					catalog_ownerM = strob_open(10);
		if (catalog_groupM == static_cast<STROB *>(NULL))
					catalog_groupM = strob_open(10);

		if (leading_dir_ownerM == static_cast<STROB *>(NULL))
					leading_dir_ownerM = strob_open(10);
		if (leading_dir_groupM == static_cast<STROB *>(NULL))
					leading_dir_groupM = strob_open(10);
		if (leading_dir_c_modeM == static_cast<STROB *>(NULL))
					leading_dir_c_modeM = strob_open(10);
		if (catalog_dir_c_modeM == static_cast<STROB *>(NULL))
					catalog_dir_c_modeM = strob_open(10);
		if (catalog_file_c_modeM == static_cast<STROB *>(NULL))
					catalog_file_c_modeM = strob_open(10);
	}
	
	int processCatalogFile(swCatalogFile * swattfile, char * psource,	
				swDefinition * swdef, int logical_find) {
		int fd;
		int datafd;
		STROB * tmp;
		//struct stat st;
		swPackageFile * archiver;
		char * old_stat;
		char * source;

		old_stat = swvarfs_get_stat_syscall(getSwvarfs());
		swvarfs_set_stat_syscall(getSwvarfs(), "stat");
		archiver = getArchiver();
		tmp = strob_open(10);

		if (*psource != ':') {
			//
			// Normal usage.
			//
			source = psource;
			swattfile->swfile_set_source_filename(source);
			//
			// Store the contents in the memory based file datafd.
			//
			datafd = swattfile->swfile_open_public_image_fd();
			if (datafd < 0) {
				setErrorCode(10148, NULL);
				strob_close(tmp);
				return -2;
			}

			//
			// Open the actual file.
			//
			fd = archiver->xFormat_u_open_file(source);
			if (fd < 0) {
				fprintf(stderr, "error : open error : %s\n",
					source);
				setErrorCode(10445, NULL);
				strob_close(tmp);
				return -3;
			}
	
			//
			// Pump the data into the swattfile's object fd
			// (and be retained in memory).
			//
			if (swlib_pipe_pump(datafd, fd) < 0) {
				setErrorCode(10448, NULL);
				strob_close(tmp);
				return -4;
			}

			archiver->xFormat_u_close_file(fd);

		} else {
			//
			// The leading ':' is a partial hack and is used to
			// trip this special case.
			//
			// This is how md5sum,adjunct_md5sum,signature and
			// files are set.
			// The value is not in a file, but rather in the
			// value part of the attribute.
			// 
			source = swattfile->get_keyword(); 
			swattfile->swfile_set_source_filename(source);

			datafd = swattfile->swfile_open_public_image_fd();
			if (datafd < 0) {
				setErrorCode(10151, NULL);
				strob_close(tmp);
				return -2;
			}

			//
			// This routine adds a '\n' to the data stream
			// making the total length writen one more than
			// strlen() of the source.
			//
			// FIXME: swdef_write_value() doesn't need to be
			// used here???
			//
			// Skip over the colon on psource
			swdef_write_value((char*)psource + 1, datafd,
					0 /*special meaning*/,
					1 /*quote off*/);

			//
			// Apply the create_time to this file.
			//
			swattfile->xFormat_set_mode(0);

			//
			// the next line determines the permissions of internally
			// generated attribute files such as md5sum, sha1sum,
			// and sig_header, and signature
			//
			swattfile->xFormat_set_perms(0644);

			swattfile->xFormat_set_mtime(getCreateTime());
			swattfile->xFormat_set_filetype_from_tartype(REGTYPE);
		}
		
		//
		// Apply the stats that are already in the swdef.
		// (i.e. override the &st buffer that was just set into
		// the swatfile).
		//

		if (swdef) {
			if (swattfile->xFormat_apply_swdef_stats(swdef,
						source, logical_find, (STROB*)(NULL))) {
				setErrorCode(10449, NULL);
			}
		}
				// Hack, Release the fd, but keep the data. 
		swattfile->swfile_re_close_public_image_fd();
		
		swvarfs_set_stat_syscall(getSwvarfs(), old_stat);
		strob_close(tmp);
		free(psource);
		return 0;
	}

	int lookup_attribute_file(swPtrList<swPackageFile> * archiveMemberList,
							  char * keyword) {
		int index = 0;
		swPackageFile * swfile;
		
		swfile = archiveMemberList->get_pointer_from_index(index++);
		while(swfile) {
			if (::strcmp(keyword,
					swfile->swfile_get_filename()) == 0) {
				return 1;
			}
			swfile = archiveMemberList->get_pointer_from_index(
								index++);
		}
		return 0;
	}

	static swCatalogFile * make_attributefile(char * keyword, 
							char * value) {
		swCatalogFile * catfile;

		if (*value == ':')
			catfile = new swAttributeFile(keyword, value);
		else
			catfile = new swCatalogFile(keyword, value);

		return catfile;
	}

	//
	// Recurse through the INFO file and make sure this
	// tag has not already been addded.
	//
	int lookup_tag(swINFO * info, char * tag) {
		char * value;
		swDefinition * swdef;
		swdef = info->swdeffile_linki_get_head();
		while(swdef) {
			if ((value=swdef->find(SW_A_tag)) != NULL)  {
				if (::strcmp(value, tag) == 0) return 1;
			}
			swdef = swdef->get_next();
		}
		return 0;	
	}

};
#endif
