/* swexstruct_i.h: The Exported Package Structure Base Class Implementation.
 */

/*
 * Copyright (C) 2003 Jim Lowe <jhlowe@acm.org>
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

#ifndef swexstruct_i_h
#define swexstruct_i_h

extern "C" {
#include "swuser_config.h"
#include "swextopt.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <assert.h>
#include "swmetadata.h"
#include "swdefinition.h"
#include "swexstruct.h"
#include "swexpsf.h"
#include "swinfo.h"
#include "swpackagedir.h"
#include "swspsf.h"

extern "C" {
#include "swsdflt.h"
#include "swlib.h"
#include "strob.h"
}


class swExStruct_i: public swExStruct
{
   private:
	int debugM;
	int mediaTypeM;			// Serial or directory.
	STROB * controlPathM;	// The control Path,including control directory.
	STROB * parentPathM;	// The parent path, everything above the control directory.
	STROB * controlDirectoryM;	// control directory pathname component.
	STROB * sbufM;			// Used for debugging buffer.
	static STRAR * errorMessagesM;
	static int outputfdM;		// output file descriptor.
	static int previewfdM;		//  preview or verbose output uses.
	static int errorCodeM;		// Global error code. non-zero is bad.
	swPathName 	*	swpathM;
	swDefinition 	* 	refererM; // global INDEX that refers to this.
  	static swExStruct * 	lastM;
        static int cksumflagsM;
  	static swPackageFile * archiverM;// The archive writing methods.
  	static int nullfdM;		// The /dev/null file descriptor.
  	static int generationFlagsM;	// Output form options.
	static swPackageDir * dirM;	// Object to write out directory.
	static STROB * leadingPathM;	// The distribution leading path.
	static time_t current_timeM;	// The current time (create_time).
	static time_t dir_mtimeM;	// The mtime of the leading path prefix 
	static char * sigFileM;		// ascii armor sigfile.
	static int filesFdM;		// catalog/dfiles/files file list.
	static char * filesM;		// The pointer to the list of files.
  	static int verboseM;		// the -x verbose=LEVEL value.
  	static int store_regtype_onlyM;	// 
 	static int allowAbsolutePathsM;	// 
	static int allowMissingSourceFileM; // Used when translating RPMS.
	static struct extendedOptions * optaM; // Options structure from main()
  public:
  	swExStruct_i(char * control_dir): swExStruct() { 
		init(); 
		if (control_dir == static_cast<char*>(NULL))
						control_dir = "";	
		setControlDirectory(control_dir);
	}

  	swExStruct_i(void): swExStruct() { init(); }

	int getStderrFd(void) { return STDERR_FILENO; }
	int getNullFd(void) { return nullfdM; }

	int get_ofd(void) { return outputfdM; }
	
	void set_ofd(int fd) {
		outputfdM = fd; 
		if (archiverM) archiverM->xFormat_set_ofd(fd);
	}
	
	int get_preview_fd(void) { return previewfdM; }

	void set_preview_fd(int fd) {
		previewfdM = fd;
		if (archiverM) {
			archiverM->xFormat_set_preview_fd(fd);
		}
	}
	
	void set_preview_level(int level) {
		if (archiverM) {
			archiverM->xFormat_set_preview_level(level);
		}
	}
	
	int get_allow_missing_files(void) { return allowMissingSourceFileM; }
	void set_allow_missing_files(int d) { allowMissingSourceFileM = d; }
	
	int getErrorCode(void) { return errorCodeM; }

	void setErrorCode(int c, char * msg) { 
		errorCodeM = c; 
		if (msg)
			store_error_msg(msg);
	}
	
	void assertNoErrorCondition(int status) {
		char * s;
		int ix;
		if (errorCodeM == 0) return;
		ix = 0;
		s = strar_get(errorMessagesM, ix++);
		while(s) {
			if (verboseM > 0)	
				fprintf(stderr, "swpackage: error: %s\n", s);
                	s = strar_get(errorMessagesM, ix++);
        	}
		if (ix < 2)
			fprintf(stderr, "swpackage: fatal error: code=%d\n", errorCodeM);
		if (status > 0) exit(status);
	}


	time_t getDirMtime(void) { return dir_mtimeM; }
	
	void setDirMtime(time_t tm) { dir_mtimeM = tm; }
	
	time_t getCreateTime(void) { return current_timeM; }
	
	void setCreateTime(time_t tm) { current_timeM = tm; }
	
	void setStoreRegTypeOnly(int c) { store_regtype_onlyM = c; }
	int getStoreRegTypeOnly(void) { return store_regtype_onlyM; }
	
	void setVerboseLevel(int c) { verboseM = c; }
	
	int getVerboseLevel(void) { return verboseM; }
	int& allowAbsolutePaths(void) { return allowAbsolutePathsM; }

	int& gFlags(void) { return generationFlagsM; }
	int getFlags(void) { return generationFlagsM; }
	void setFlags(int f) { generationFlagsM = f; }

	virtual ~swExStruct_i(void) {
		::strob_close(controlPathM);
		::strob_close(parentPathM);
		::strob_close(controlDirectoryM);
		::strob_close(sbufM);
		delete swpathM;
	}

 	int getDebugState(void){ return debugM; }

	void setDebugState(int i){ debugM=i; }

	swPackageDir * getPackageDirObject(void) { return dirM; }
	
	char * getDistributionLeadingPath(void) {
			return strob_str(leadingPathM);
	}
 	
	void set_opta_array(struct extendedOptions * opta) {
		optaM = opta;
	}

	struct extendedOptions * get_opta_array(void) {
		return optaM; 
	}

 	void setDistributionLeadingPath(char * path) {
		strob_strcpy(leadingPathM, path);
	}

	virtual swExStruct * containedByIndex(int index) {
		return NULL;
	}

	virtual int containedListAdd(swExStruct * o) {
		assert(0); return 0;
	}
	
	virtual swPtrList<swPackageFile> * getFileListObject(void) {
		return NULL;
	}

	virtual swPtrList<swExStruct> * getStorageObjectList(void) {
		return NULL;
	}

	virtual swPtrList<swExStruct> *
		getThisStorageList(swExStruct * swexdist){
			return NULL;
	}

	virtual swPtrList<swExStruct> * getFilesets(void){ return NULL; }

	virtual swPtrList<swExStruct> * getProducts(void){ return NULL; }

	virtual int write_directory(void){ return 0; }

	virtual int write_serial (void){ return 0; }

	virtual int write_file (void){ return 0; }

	virtual int write_out_struct (void){ return 0; }

	virtual int read_in_struct (void){ return 0; }

	virtual int initObjFiles(swDefinition * swdef){ return -1; }

	virtual void setMediaType(int type){ mediaTypeM = type; }

	virtual STROB * getControlPathStrob(void){ return controlPathM; }
	
	virtual char * getFormedControlPath(void) {
		return getFormedControlPath(static_cast<char*>(NULL),
				static_cast<char*>(NULL));
	}
			
	char * getSigFileBuffer(void) { return sigFileM; }
	void   setSigFileBuffer(char * s) { sigFileM = s; }
	
	int getFilesFd(void) { return filesFdM; }
	void   setFilesFd(int filesfd) { filesFdM = filesfd; }
	
	char * getFilesBuffer(void) { return filesM; }
	void   setFilesBuffer(char * s) { filesM = s; }

	virtual char * getFormedControlPath(char * path, char * catalog_path){ 

		::strob_strcpy(controlPathM, strob_str(parentPathM));
		if (catalog_path) {
			insert_catalog_path(controlPathM);
		}

		::swlib_unix_dircat(controlPathM, strob_str(controlDirectoryM));
		if (path) {
			::swlib_unix_dircat(controlPathM, path);
		}
		return ::strob_str(controlPathM);
	}


	virtual void setParentPath(char * s){ 
		::strob_strcpy(parentPathM, s);
	}
	
	virtual int setControlDirectory(char * s){
		::strob_strcpy(controlDirectoryM, s);
		return 0;
	}
	
	virtual int printDebugStructure(int fd) {
		int  list_index;
		STROB * tmp;
		swExStruct      * object;

		tmp = strob_open(20);
		swlib_writef(fd, tmp,
			"In printDebugStructure BEGIN : ObjectName = [%s]\n",
					getObjectName());

		swlib_writef(fd, tmp,
			"RefererName = [%s] FormedControlPath = [%s]\n",
				getReferer()->get_keyword(),
				getFormedControlPath());
		getReferer()->write_fd(fd);
 
		if (getInfo() != NULL) {
			swlib_writef(fd, tmp, "INFO file BEGIN : [%s]\n",
				getObjectName());
			getInfo()->swdeffile_linki_write_fd(fd);
			swlib_writef(fd, tmp,
				"INFO file END : [%s]\n", getObjectName());
		}

		list_index = 0;
		while((object = containedByIndex(list_index++))) {
			object->printDebugStructure(fd);	
		}
		swlib_writef(fd, tmp,
			"In printDebugStructure END : [%s]\n",
				getObjectName());
		strob_close(tmp);
		return 0;
	}
	
	virtual int setControlDirectory(void){ 
		char * up;
		char * tag=NULL;
		char * cdir=NULL;
		swMetaData * p;
		
		assert(refererM);
		p = refererM->get_next_node();
		while(p) {
			if(!(::strcmp((up=p->get_value((int*)NULL)), SW_A_tag))) {
				tag=up;
			}
			else if(!(::strcmp((up=p->get_value((int*)NULL)),
						"control_directory"))){
				cdir=up;
			}
			p = p->get_next_node();
		}
		if (cdir) {
			setControlDirectory(cdir);
		} else if (tag) {
			setControlDirectory(tag);
		} else {
			fprintf(stderr,
		"error : %s has no control_directory or tag attribute.\n",
							getObjectName());
			exit(1);
		}
		return	0;
	}

	virtual char * getParentPath(void) { return ::strob_str(parentPathM); }
	
	virtual char * getControlDirectory(void) {
			return ::strob_str(controlDirectoryM);
	}
	
	virtual void setReferer(swDefinition * swdef){ refererM=swdef; }

	virtual swDefinition * getReferer(void){ return refererM; }

	swExStruct * getLastExStruct(void) { return lastM; }

	void setLastExStruct(swExStruct * last) { lastM = last; }

	virtual void setIndex(swINDEX * sw){ }

	virtual swExStruct * findBySwDef(swDefinition * swdef) {
		return static_cast<swExStruct*>(NULL);
	}

	virtual swINDEX * getIndex(void) { return NULL; }

	virtual swINFO * getInfo(void) { return NULL; }

	char * getPackagePath(swDefinition * swdef){
		char * path = swdef->getPathAttribute();
		assert(path);	
		return path;
	}
	
	char * getSourcePath(swDefinition * swdef) {
		return swdef->find(SW_A_source);
	}

	void doInitializeParentsPaths(char * formed_path) { ; }
	
	int write_filename(void) { return -1; }
	
	virtual void do_terminate_link(swDefinition * def) {
		def->set_next(NULL);
	}
	
	virtual int write_fd(int fd) {
		return 0;
	}

	virtual swExStruct * get_first_product(int * found) {
		return NULL;
	}

	virtual char * determineControlDirectory(void) {
		swDefinition * swdef = getReferer();
		char * control_dir = static_cast<char*>(NULL);
	
		if (!swdef) {
			return static_cast<char*>(NULL);
		}
		control_dir = swdef->find("control_directory");
		if (control_dir == static_cast<char*>(NULL)) {
			control_dir = swdef->find(SW_A_tag);
		}
		return control_dir;
	}

	char * getObjectName(void) { return ""; }
	
	virtual int registerWithGlobalIndex(void) { return 0; }

	virtual int createIndexFile(void) { return 0; }
	
	virtual void appendToParentPath(char * path) {
		swlib_unix_dircat(parentPathM, path);
	}

	virtual void setLeadingPackagePath(char * path) {  }
	
	virtual void addControlDirectoryAttribute(void) {
		/* fprintf(stderr, "HERE : %s\n", getObjectName()); */
	}
	
	virtual swExStruct * getXfiles(void) { return NULL;  }
	
	virtual void addControlFileToINFOforINFO(void){ }

	virtual void generateFileSetSize(void) { }
					
	virtual void emitLeadingPath(void) { }
	
	virtual void emitStorageStructure(void) { }

	virtual void emitExportedCatalog(void) { }
	
	int getCksumFlags(void) { return cksumflagsM; }

	void setCksumFlags(int f) { cksumflagsM = f; }

	virtual void setupAttributeFiles(void) {  }
	
	virtual void createAttributeFiles(void) {  }
	
	virtual void createControlScriptFiles(void) {  }
	
	virtual void registerAttributeFiles(void) {  }
	
	virtual void resetDfilesInfoSizes(void) { }
	
	virtual void setInfoSizes(void) { }
	
	virtual void tuneSignatureFile(void) { }
	
	virtual void tuneFilesFile(void) { }
	
	virtual void taskPreResponse(enum taskCodes code) { }
	
	virtual void doPass1payload() { }
	
	virtual void setAllfilesets(void) { }
	
	virtual void doInit1() { }

	//
	// General code to touch all the objects and run the 
	// specified task.
	//
	virtual void taskDispatcher(enum taskCodes code) {
		int  list_index = 0;
		swExStruct 	* object;
		swExStruct	* xfiles;  // Pfiles or Dfiles object.
	
		taskPreResponse(code);	

		xfiles = getXfiles();
		if (xfiles) {
			xfiles->taskDispatcher(code);
		}
		while((object = containedByIndex(list_index++))) {
			object->taskDispatcher(code);
		}

		taskResponder(code);	
	}

	//
	// Generalized task catcher.
	//
	virtual void taskResponder(enum taskCodes code) {
			
		switch(code) {	
			case addControlDirectoryE:
				addControlDirectoryAttribute();
				break;
			case removeSourceAttributeE:
				//
				// Done in init2E, not used.
				//
				break;
			case init1E:
				//
				// Pass1, misc. initialization of pfiles 
				// and dfiles.
				//
				doInit1();
				break;
			case init2E:
				//
				// Pass2
				// Done in its own code.
				//
				break;
			case addFileStatsE:
				//
				// Add file stats.
				//
				addFileStatsToFileDefinitions();
				break;
			case addInfoDefinitionE:
				//
				// Add control_file definition to the
				// INFO file for the INFO file.
				//
				addControlFileToINFOforINFO();
				break;
			case generateFileSetSizeE:
				//
				// Add control_file definition to the
				// INFO file for the INFO file.
				//
				generateFileSetSize();
				break;
			case createControlScriptFilesE:
				//
				// Create the package files for the
				// control scripts.
				//
				createControlScriptFiles();
				break;
			case setupAttributeFilesE:
				//
				// Create AttributeFiles and register
				// in INFO file.
				//
				setupAttributeFiles();
				break;

			case emitLeadingPathE:
				//
				// Write the catalog section.
				//
				emitLeadingPath();
				break;
			case resetDfilesInfoSizesE:
				//
				// Fix the size in the INFO of the INFO file.
				//
				resetDfilesInfoSizes();
				break;
			case setInfoSizesE:
				//
				// Fix the size in the INFO of the INFO file.
				//
				setInfoSizes();
				break;
			case emitExportedCatalogE:
				//
				// Write the catalog section.
				//
				emitExportedCatalog();
				break;

			case emitStorageStructureE:
				//
				// Write the catalog section.
				//
				emitStorageStructure();
				break;
			case tuneFilesFileE:
				//
				// 
				//
				tuneFilesFile();
				break;
			case tuneSignatureFileE:
				//
				// 
				//
				tuneSignatureFile();
				break;
			case setAllfilesetsE:
				//
				// 
				//
				setAllfilesets();
				break;
			case last_taskE:
				break;
			default:
				break;
		}
	}

	//
	//  runs adjustFileDefs() below.
	//
	virtual void performInfoPass2(void) {
		int  list_index = 0;
		swExStruct 	* object;
		swExStruct	* xfiles;  // Pfiles or Dfiles object.
		
		xfiles = getXfiles();
		if (xfiles) {
			xfiles->performInfoPass2();
		}
		while((object = containedByIndex(list_index++))) {
			object->performInfoPass2();
		}
	}
	

	//
	// runs initObjFiles which initializes the
	// swExDfiles and swExPfiles objects.
	//
	virtual void performInitializationPass1(void) {
		int  list_index = 0;
		swExStruct 	* object;
		swExStruct	* xfiles;  // Pfiles or Dfiles object.
		
		xfiles = getXfiles();
		if (xfiles) {
			xfiles->performInitializationPass1();
		}
		doPass1payload();
		while((object = containedByIndex(list_index++))) {
			object->performInitializationPass1();
		}
	}


	//
	// Remove the source attribute and create path attribute if neeeded.
	//
	static int adjustFileDefs(swINFO * info) {
		int bytesize = 0;
		swDefinition * inext;
		swMetaData * att;
	
		if (info == NULL) return 0;
		inext = info->swdeffile_linki_get_head();

		while(inext) {
			att = inext->get_next_node();  // The first attribute.
			while (att) {
				if (::strcmp(att->get_keyword(),
							SW_A_source) == 0) {
					att->vremove();
				}
				att = att->get_next_node();
			}
			inext = inext->get_next();
		}
		return bytesize;
	}
	
	void
	set_cksum_creation(int s) {
		if (s) {
			// On
			cksumflagsM |= SWFILE_DO_CKSUM;
		} else {
			// Off
			cksumflagsM &= ~(SWFILE_DO_CKSUM);
		}
	}
	
	void
	set_digests1_creation(int s) {
		if (s) {
			 // On
			 cksumflagsM |= SWFILE_DO_FILE_DIGESTS;
		} else {
			// Off
			cksumflagsM &= ~(SWFILE_DO_FILE_DIGESTS);
		}
	}
	
	void
	set_digests2_creation(int s) {
		if (s) {
			 // On
			 cksumflagsM |= SWFILE_DO_FILE_DIGESTS_SHA2;
		} else {
			// Off
			cksumflagsM &= ~(SWFILE_DO_FILE_DIGESTS_SHA2);
		}
	}


	swPackageFile * getArchiver(void) { return archiverM; }

	void setArchiver(swPackageFile * a) { archiverM = a; }

	void initPackageDirObject(void) {
		//
		// Initialize a long name to mitigate further
		// memory allocation. FIXME ??
		//
		dirM = new swPackageDir(\
			"/usr/tmp/xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		);
	}

	static int 
	swexstruct_write_definitionfile(swDefinitionFile * swfile, time_t ptm,
				char * perms, char * owner, char * group) {
		int ret;
		unsigned int ulmode;
		mode_t mode;
		time_t tm = ptm;
		int (swDefinitionFile::*def_write)(int);
		
		def_write = &swDefinitionFile::swdeffile_linki_write_fd;

		swfile->swfile_set_default_statbuf();

		swfile->xFormat_set_mtime(tm);
		swfile->xFormat_set_linkname("");	
		if (!owner) owner = SWBIS_DEFAULT_CATALOG_USER;
		if (!group) group = SWBIS_DEFAULT_CATALOG_GROUP;

		swfile->xFormat_set_user_systempair(owner);
		swfile->xFormat_set_group_systempair(group);
		
		if (!perms) {
			perms = "0640";
		}
		sscanf(perms, "%o", (unsigned int*)&ulmode);
		mode = (mode_t)ulmode;
		
		swfile->xFormat_set_mode(0);
		swfile->xFormat_set_perms(mode);
		swfile->xFormat_set_filetype_from_tartype(REGTYPE);
		ret = swfile->xFormat_write_file(def_write);
		return ret;
	}

	static int 
	swexstruct_write_dir(char * name, time_t tm, char * perms,
						char * owner, char * group) {
		unsigned int ulmode;
		swPackageDir * dir = dirM;
		int ret;
		mode_t mode;

		dir->swfile_set_default_statbuf();
		dir->xFormat_set_filesize(0);
		dir->xFormat_set_mtime(tm);
		
		if (!owner) owner = SWBIS_DEFAULT_CATALOG_USER;
		if (!group) group = SWBIS_DEFAULT_CATALOG_GROUP;

		dir->swfile_set_package_filename(name);
		dir->xFormat_set_user_systempair(owner);
		dir->xFormat_set_group_systempair(group);
		
		if (!perms) {
			perms = "0750";
		}
		sscanf(perms, "%o", (unsigned int*)&ulmode);
		mode = (mode_t)ulmode;
		
		dir->xFormat_set_perms(mode);
		dir->xFormat_set_filetype_from_tartype(DIRTYPE);
		ret = dir->xFormat_write_file();
		return ret;
	}
	
	virtual void emitLeadingStorageDirectory(STROB * tmp)  { }

	virtual void emitLeadingCatalogDirectory(STROB * tmp)  { }

   private:

	void
	store_error_msg(char * msg)
	{
		strar_add(errorMessagesM, msg);
		return;
	}
	
	void insert_catalog_path(STROB * controlpath) {
		int len;
		char * p;
		char * al;
		char * leading_path;
		
		//
		// The "catalog" path component is always inserted after the
		// the distribution leading path.
		//

		//
		// make room to add "catalog"
		//
		strob_set_length(controlpath, strob_strlen(controlpath) + 9);

		p = strob_str(controlpath);

		leading_path = getDistributionLeadingPath();

		if (strlen(leading_path) && strlen(p)) {
			al = strstr(p, leading_path);
			if (al != p) {
				setErrorCode(10201, NULL);
			}
			len = strlen(leading_path);
			memmove(p + len + 8, p + len, strlen(p+len)+1);
			memcpy(p + len, "/catalog", 8);
		} else {
			//
			// Prepend "catalog"
			//
			memmove(p+8, p, strlen(p)+1);
			memcpy(p, SW_A_catalog "/", 8);
		}
	}

	void init(void) {
		debugM=0;
		mediaTypeM = 0; // 0=Directory, 1=Serial;
		controlPathM = ::strob_open(16);
		controlDirectoryM = ::strob_open(16);
		parentPathM = ::strob_open(16);
		sbufM = ::strob_open(16);
		refererM=NULL;
		swpathM = new swPathName("");
		cksumflagsM = 0;
		archiverM = NULL;
		sigFileM = NULL;
		verboseM = 1;
		allowAbsolutePathsM = 0;
		allowMissingSourceFileM = 0;
		if (current_timeM == 0) {
			current_timeM = time(NULL);
		}
		
		if (dir_mtimeM == 0) {
			dir_mtimeM = current_timeM;
		}

		if (nullfdM == 0) {
			nullfdM = open("/dev/null", O_RDWR, 0);
			if (nullfdM < 0) {
				setErrorCode(20201, NULL);
			}
		}
		if (dirM == NULL) {
			initPackageDirObject();
		}

		if (leadingPathM == NULL) {
			leadingPathM = ::strob_open(16);
		}
		if (errorMessagesM == NULL)
			errorMessagesM = strar_open();
	}
};
#endif
