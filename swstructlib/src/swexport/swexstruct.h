/* swexstruct.h: The Exported Package Structure Base Class.
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

#ifndef swexstruct_1_h
#define swexstruct_1_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "swmetadata.h"
#include "swpathname.h"
#include "swptrlist.h"
#include "swindex.h"

class swPackageFile;
class swDefinition;
class swINFO;

#define SWEXPORT_GEN_FILE_PATHS  	(1 << 0)
#define SWEXPORT_GEN_SERIAL_ARCHIVE 	(1 << 1)
#define SWEXPORT_GEN_DIRECTORY 		(1 << 2) 	/* Not supported */
#define SWEXPORT_GEN_ADJUNCT_ARCHIVE	(1 << 3) 	/* Skip SYMLNKS */

class swExStruct
{
  public:
	enum taskCodes {
		addControlDirectoryE,    // Add control_directory attribute to fileset and product definitions.

		removeSourceAttributeE,  // Mark as "removed" the source attribute.

		init1E,		// Init. Pass 1: Fill in the dfiles and pfiles objects.

		init2E,         // Init. Pass 2: remove 'source' attr. and add up the fileset size.

		addInfoDefinitionE, 	// Add control_file definition in each INFO for the INFO file.

		addControlFileForAttrFileE, 	// Add control_file definition in attribute stored as a file.

		addFileStatsE, 		// Add file stats.
		
		setupAttributeFilesE,	// Setup attribute files and register with INFO file.
		
		createControlScriptFilesE,	// Setup attribute files and register with INFO file.

		setInfoSizesE,		// Set the size in the INFO file for the INFO file.
		
		emitLeadingPathE,	// Emit the Leading package path archive member.
		
		emitExportedCatalogE,	// Emit the catalog section.

		emitStorageStructureE,	// Emit the file storage section.

		generateFileSetSizeE,	// Calculate the fileset size and set size attribute in fileset object.
		
		tuneSignatureFileE,	// Add signature and precursor region.
		
		resetDfilesInfoSizesE,	//  Fix the size attribute for the INFO control_file definition.
		
		setAllfilesetsE,	//  Set the all_filesets attribute in the products.
		
		tuneFilesFileE,		//  Add the catalog/dfiles/files file.

		last_taskE  
	};
        virtual ~swExStruct() { } 
	virtual int getStderrFd(void) = 0;
	virtual int getDebugState(void) = 0;
	virtual void setDebugState(int i) = 0;
	virtual int write_file(void) = 0;
	virtual int write_serial(void) = 0;
	virtual int write_directory(void) = 0;
	virtual int write_out_struct (void) = 0;
	virtual void writePreviewLine (STROB * buffer, char * filename) = 0;
	virtual int read_in_struct (void) = 0;

	virtual int initObjFiles(swDefinition * referer) = 0;

	virtual swPtrList<swPackageFile> * getFileListObject(void) = 0;
	virtual swPtrList<swExStruct> * getStorageObjectList(void) = 0;
	virtual swPtrList<swExStruct> * getThisStorageList(swExStruct * swExDist) = 0;
	virtual swPtrList<swExStruct> * getFilesets(void) = 0;
	virtual swPtrList<swExStruct> * getProducts(void) = 0;
	virtual char * getFormedControlPath(void) = 0;
	virtual char * getFormedControlPath(char * path, char * catalog_path) = 0;

	virtual swExStruct * findBySwDef(swDefinition * swdef) = 0;
	
	virtual swExStruct * getLastExStruct(void) = 0;
	virtual void setLastExStruct(swExStruct * last) = 0;

	virtual swDefinition * getPrevious(void) = 0;
	virtual void setPrevious(swDefinition * previous) = 0;

	virtual void doInitializeParentsPaths(char * formed_path) = 0;
	virtual int doesContain(char * keyword) = 0;
	virtual int doesHaveControlFiles(char * keyword) = 0;

	virtual char * getParentPath(void) = 0;
	virtual void setParentPath(char *s) = 0;
	virtual void setMediaType(int s) = 0;
	virtual char * getControlDirectory(void) = 0;
	virtual void setReferer(swDefinition * swdef) = 0;
	virtual swDefinition * getReferer(void) = 0;
	virtual void setIndex(swINDEX * index) = 0;
	virtual void setInfo(swINFO * info) = 0;
	virtual swINDEX* getIndex(void) = 0;

	virtual swINDEX* getEmittedGlobalIndex(void) = 0;
	virtual swINDEX* getGlobalIndex(void) = 0;

	virtual swINFO* getInfo(void) = 0;
	virtual swPtrList<swPackageFile> * getControlScriptList(void) = 0;
	virtual swPtrList<swPackageFile> * getAttributeFileList(void) = 0;
	virtual swPtrList<swPackageFile> * getDistributionFileList(void) = 0;
	virtual int write_filename(void)=0;
	virtual swDefinition * processForExport_step1 \
			(SWHEADER * swheader, swDefinitionFile * psf, \
				int level, char * currentline, int * error_code) = 0;
	virtual void set_swheader_offset(int) = 0;
	virtual int get_swheader_offset(void) = 0;
	virtual int * get_swheader_offset_p(void) = 0;
	virtual swDefinitionFile* getPSF(void) = 0;
	virtual void setPSF(swDefinitionFile* PSF) = 0;
	virtual void do_terminate_link(swDefinition * def) = 0;
	virtual int write_fd(int ofd) = 0;
	virtual swExStruct * get_first_product(int * found) = 0;

	virtual char * determineControlDirectory(void) = 0;
	virtual char * getObjectName(void) = 0;
	virtual int swobjfile_file_out_of_scope(swMetaData * swmd, int current_level) = 0;

	virtual int registerWithGlobalIndex(void) = 0;

	virtual void setLeadingPackagePath(char * leading_path) = 0;
	
	virtual int setControlDirectory(char * path) = 0;

	virtual int setControlDirectory(void) = 0;

	virtual int createIndexFile(void) = 0;

	virtual swExStruct * containedByIndex(int index) = 0;

	virtual int containedListAdd(swExStruct * o) = 0;
	
	virtual int printDebugStructure(int fd) = 0;
	
	virtual swExStruct * getXfiles(void) = 0;

	virtual void performInitializationPass1(void) = 0;

	virtual void doPass1payload(void) = 0;
	
	virtual void performInfoPass2(void) = 0;
	
	virtual void setGlobalIndex(swINDEX * index) = 0;
	
	virtual SWVARFS * getSwvarfs(void) = 0;
	
	virtual void  setSwvarfs(SWVARFS * a) = 0;
	
	virtual void addControlDirectoryAttribute(void) = 0;
	
	virtual void taskDispatcher(enum taskCodes code) = 0;
	
	virtual void taskResponder(enum taskCodes code) = 0;
	
	virtual void addControlFileToINFOforINFO(void) = 0;
	
	virtual void addFileStatsToFileDefinitions(void) = 0;

	virtual void generateFileSetSize(void) = 0;

	virtual void emitLeadingPath(void) = 0;
	
	virtual void emitStorageStructure(void) = 0;

	virtual void emitExportedCatalog(void) = 0;
	
	virtual void setupAttributeFiles(void) = 0;
	
	virtual void createControlScriptFiles(void) = 0;
	
	virtual void createAttributeFiles(void) = 0;
	
	virtual void registerAttributeFiles(void) = 0;
	
	virtual void setInfoSizes(void) = 0;
	
	virtual void resetDfilesInfoSizes(void) = 0;
	
	virtual void taskPreResponse(enum taskCodes code) = 0;

	virtual void emitLeadingStorageDirectory(STROB * tmp) = 0;

	virtual void emitLeadingCatalogDirectory(STROB * tmp) = 0;
	
	virtual void tuneSignatureFile(void) = 0;
	
	virtual void setAllfilesets(void) = 0;
 
};

#endif
