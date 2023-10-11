/* swexdistribution.h: The Exported distribution 
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

#ifndef swexdistribution_i_h
#define swexdistribution_i_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "swmetadata.h"
#include "swexstruct_i.h"
#include "swobjfiles.h"
#include "swexcat.h"
#include "swexdfiles.h"
#include "swpackagefile.h"
#include "swpackagedir.h"
#include "swptrlist.h"

//
// The swExDistribution object 
//

class swExDistribution: public swExCat
{
	STROB * targetPathM;
	swDefinition * previousM;
  	
  public:
	
	swExDistribution(void): swExCat(){
		init();
	}
  	
	virtual ~swExDistribution(void){}

	char * getObjectName(void) { return "distribution"; }

	char * getFormedControlPath(void){ return getParentPath(); }
	
	char * getFormedControlPath(char * path, char * catalog_path){

		STROB * cpath = getControlPathStrob();
		::strob_strcpy(cpath, getParentPath());
		
		if (catalog_path)
			::swlib_unix_dircat(cpath, catalog_path);
	
		if (path) {
			::swlib_unix_dircat(cpath, path);
		}

		return ::strob_str(cpath);
	}
	
	void setInfo(swINFO * a) { getXfiles()->setInfo(a); }	
	
	swINFO * getInfo(void) { return getXfiles()->getInfo(); }

	swDefinition * getPrevious(void){ return previousM; }
	
	void         setPrevious(swDefinition * s) { previousM = s; }

	swPtrList<swExStruct> * getFilesets(void) { return NULL; }

	swPtrList<swExStruct> * getStorageObjectList(void){ return getProducts(); }

	int doesContain(char * key) {
		return 
			(
				strcmp(key, SW_A_product) == 0 ||
				strcmp(key, SW_A_bundle) == 0 ||
				strcmp(key, SW_A_media) == 0 ||
				strcmp(key, SW_A_category) == 0 ||
				strcmp(key, SW_A_vendor) == 0
			)	;
	}

	static swExStruct * make_exdist(void) {
		return new swExDistribution();
	}

	int doBuild(void) {
		return 0;
	}
	
	void setLeadingPackagePath(char * path) {
		int  list_index = 0;
		swExStruct 	* object;
                swExStruct      * xfiles;  // Pfiles or Dfiles object.
		char * control_dir;	

		control_dir = determineControlDirectory();
		if (control_dir == NULL) {
			//  OK 
			control_dir = "";
		}

		//
		// FIXME, path is always "" as set from swpackage.cxx
		//
		setDistributionLeadingPath(control_dir);

		setControlDirectory(control_dir);
		swExStruct_i::appendToParentPath(path);
		swExStruct_i::appendToParentPath(control_dir);
                
		xfiles = getXfiles();
                if (xfiles) {
                        xfiles->setLeadingPackagePath(getFormedControlPath());
                }

		while((object = containedByIndex(list_index++))) {
			object->setLeadingPackagePath(getFormedControlPath());
		}
	}


	void doPass1payload(void) {
		//
		// Add the layout_version attribute.
		//

		swMetaData * first_attribute;
		swDefinition * swdef = getReferer();
	
		SWLIB_ASSERT(swdef != NULL);

		first_attribute = swdef->get_next_node();
		if (first_attribute && ::strcmp(first_attribute->get_keyword(), SW_A_layout_version) == 0)
			return;
		
		//
		// Delete any existing attribute and add a new attribute.
		//
		swdef->deleteAttribute("layout_version");
		first_attribute = new swAttribute("layout_version", SWBIS_PROGS_LAYOUT_VERSION);
		first_attribute->set_level(2);

		//
		// Insert it as the first attribute.
		//
		swdef->list_insert(first_attribute, swdef->get_next_node());	
		return;
	}
	
	void emitLeadingPath(void) {
		swDefinition * swdef = getReferer();
		int ret;
		char * path;
		STROB * tmp;

		path = swdef->find("control_directory");
		if (path == NULL) {
			path = swdef->find(SW_A_tag);
		}
		if (path == NULL || ::strlen(path) == 0 || strcmp(path, "/") == 0) return;

		tmp = strob_open(20);
		strob_strcpy(tmp, path);
		swlib_add_trailing_slash(tmp);
		ret = swExStruct_i::swexstruct_write_dir(strob_str(tmp), getDirMtime(), 
			getLeadingDirModeString(),
			getLeadingDirOwner() /* swdef->find(SWBIS_DISTRIBUTION_OWNER_ATT) */,
			getLeadingDirGroup() /* swdef->find(SWBIS_DISTRIBUTION_GROUP_ATT) */);
		if (ret < 0) setErrorCode(20007, NULL);
		strob_close(tmp);
	}

	// static char * swexdistribution_dump_string_s(swExDistribution * pf, char * prefix);
	// char * dump_string_s(char * prefix) { return swexdistribution_dump_string_s(this, prefix); }

	int createIndexFile(void) {
		swINDEX * index = new  swINDEX();
		setIndex(index);
		return 0;
	}

	virtual void addControlDirectoryAttribute(void) { }

	void resetDfilesInfoSizes(void) {
		setInfoSizes();
	}

	virtual void taskPreResponse(enum taskCodes code) {
			
		switch(code) {	
			case emitExportedCatalogE:
				//
				// Emit the <path>/catalog/ directory
				// and the  <path/catalog/INDEX file.
				//
				{
					STROB * tmp = strob_open(10);
					swDefinitionFile * index;	
					//
					// Emit the next directory.
					//
					strob_strcpy(tmp, getFormedControlPath(static_cast<char*>(NULL), 
							(char*)(SW_A_catalog)));
					
					swlib_add_trailing_slash(tmp);
					swExStruct_i::swexstruct_write_dir(strob_str(tmp), 
						getCreateTime(), getCatalogDirModeString(),
							getCatalogOwner(), getCatalogGroup());

					//
					// Emit the INDEX File.
					//
					swlib_unix_dircat(tmp, "INDEX");
					index = getGlobalIndex();
					SWLIB_ASSERT(index != NULL);
					index->swfile_set_package_filename(strob_str(tmp));

					swexstruct_write_definitionfile(index, getCreateTime(),
									getCatalogFileModeString(), 
									getCatalogOwner(),
									getCatalogGroup());
					strob_close(tmp);
				}

				break;
			case addControlDirectoryE:
			case removeSourceAttributeE:
			case init1E:
			case init2E:
			case addFileStatsE:
			case addInfoDefinitionE:
			case generateFileSetSizeE:
			case setupAttributeFilesE:
			case createControlScriptFilesE:
			case emitLeadingPathE:
			case addControlFileForAttrFileE:
			case setInfoSizesE:
			case resetDfilesInfoSizesE:
			case emitStorageStructureE:
			case tuneSignatureFileE:
			case tuneFilesFileE:
			case setAllfilesetsE:
			case last_taskE:
					break;
		}
	}
	
	virtual void taskResponder(enum taskCodes code) {
		switch(code) {	
			case setupAttributeFilesE:
				break;	
			case createControlScriptFilesE:
				//
				// This is done by the dfiles.
				//
				break;	
			case emitExportedCatalogE:
			case addControlDirectoryE:
			case removeSourceAttributeE:
			case init1E:
			case init2E:
			case addFileStatsE:
			case addInfoDefinitionE:
			case generateFileSetSizeE:
			case emitLeadingPathE:
			case addControlFileForAttrFileE:
			case setInfoSizesE:
			case resetDfilesInfoSizesE:
			case emitStorageStructureE:
			case tuneSignatureFileE:
			case tuneFilesFileE:
			case setAllfilesetsE:
			case last_taskE:
			default:
				swObjFiles_i::taskResponder(code);
				break;
		}
	}

  private:
	void init(void) {
		targetPathM = strob_open(10);  // Initial path is "".
		setLastExStruct(this);
		setControlDirectory("");
		previousM = static_cast<swDefinition*>(NULL);
		swExCat::setXfiles(swObjFiles::swFilesFactory("dfiles"));
	}

};

#endif
