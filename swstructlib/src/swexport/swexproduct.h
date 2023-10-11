/* swexproduct.h: The Exported product catalog 
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

#ifndef swexproduct_i_h
#define swexproduct_i_h

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
#include "swexpfiles.h"
#include "swexcat.h"

class swExProduct: public swExCat
{
  	STROB * all_filesetsM;	
  public:

	swExProduct(char * control_dir): swExCat(control_dir){ init_(); }
  	swExProduct(void): swExCat(){ init_(); }
  	~swExProduct(void){}

	swPtrList<swExStruct> * getProductList() { return NULL; }
	
	char * getObjectName(void) { return SW_A_product; }

	static swExStruct * make_exdist(void) {
		return new swExProduct("");
	}
	
	virtual int swobjfile_file_out_of_scope(swMetaData * swmd, int current_level) {
		if (swmd->get_level() < current_level || 
			strcmp(swmd->get_keyword(), SW_A_fileset) == 0 || 
			strcmp(swmd->get_keyword(), SW_A_vendor) == 0 ||
			strcmp(swmd->get_keyword(), SW_A_subproduct) == 0
		)
			return 1;
		else
			return 0;
	}	
	
	void setInfo(swINFO * a) { getXfiles()->setInfo(a); }	
	
	swINFO * getInfo(void) { return getXfiles()->getInfo(); }

	void setAllfilesets(void) {
		swDefinition * filesetdef;
		swDefinition * swdef = getReferer();
		swExStruct 	* object;
		int list_index = 0;
		char * tag;

		while((object = containedByIndex(list_index++))) {
			filesetdef = object->getReferer();
			if (filesetdef == NULL) { setErrorCode(10801, NULL); return; }
	
			if (::strcmp(object->getObjectName(), SW_A_fileset) != 0)
				continue;
			tag = filesetdef->find(SW_A_tag);
			if (tag == NULL) { setErrorCode(10802, NULL); return; }
			if (strlen(tag) == 0) { setErrorCode(10803, NULL); return; }
			
			if (strob_strlen(all_filesetsM)) {
				strob_strcat(all_filesetsM, " ");
			}
			strob_strcat(all_filesetsM, tag);
		}
	
		if (strob_strlen(all_filesetsM) == 0) { setErrorCode(10804, NULL); }
		swdef->add("all_filesets", strob_str(all_filesetsM));	
		return;	
	}

	virtual void taskPreResponse(enum taskCodes code) {
			
		switch(code) {	
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
			case tuneSignatureFileE:
			case tuneFilesFileE:
			case setInfoSizesE:
			case resetDfilesInfoSizesE:
			case setAllfilesetsE:
					break;
			case emitExportedCatalogE:
			case emitStorageStructureE:
				{	
				char * s;
				STROB * tmp;
				tmp = strob_open(100);
				char * catalog_string;

				if (code == emitExportedCatalogE) {
					catalog_string = SW_A_catalog;
				} else {
					catalog_string = NULL;
				}
				//
				// Form the package path for the control path.
				//
				strob_strcpy(tmp, getFormedControlPath(static_cast<char*>(NULL), static_cast<char*>(catalog_string)));

				if (code == emitExportedCatalogE) {
                			s = strstr(strob_str(tmp), SW_A_catalog);
                			SWLIB_ASSERT(s != NULL);
                			s += strlen(SW_A_catalog);
                			if ( (*s == '/' && *(s+1) == '\0') || *(s) == '\0') {
							//
							// Don't write it.
							strob_close(tmp);
							break;
					}
				}
				//
				// Emit the leading directory for the product.
				//
				emitLeadingStorageDirectory(tmp);

				strob_close(tmp);
				}		
			
				break;	
			case last_taskE:
					break;
		}
	}


	virtual void taskResponder(enum taskCodes code) {
		switch(code) {	
			case resetDfilesInfoSizesE:
				//
				// Don't refigure the size of the product and
				// fileset INFO files.
				//
				break;	
			case setupAttributeFilesE:
				break;	
			case createControlScriptFilesE:
				//
				// This prevents the product from 
				// doing this task, It is done in
				// the pfiles.
				//
				break;	
			case emitExportedCatalogE:
			case addControlDirectoryE:
			case removeSourceAttributeE:
			case tuneSignatureFileE:
			case tuneFilesFileE:
			case init1E:
			case init2E:
			case addFileStatsE:
			case addInfoDefinitionE:
			case generateFileSetSizeE:
			case emitLeadingPathE:
			case addControlFileForAttrFileE:
			case setInfoSizesE:
			case emitStorageStructureE:
			case setAllfilesetsE:
			case last_taskE:
			default:
				swObjFiles_i::taskResponder(code);
				break;
		}
	}

	int doesHaveControlFiles(char * key) { return (strcmp(key, "control_file") == 0); }

	int doesContain(char * key) { return (
					strcmp(key, SW_A_fileset) == 0 || 
					strcmp(key, SW_A_subproduct) == 0 ||
					strcmp(key, SW_A_vendor) == 0
					);
				}

	void do_terminate_link(swDefinition * def) {  } // do nothing;
	
	// virtual char * dump_string_s(char * prefix);
	
	virtual void emitStorageStructure(void){ }
	
	private:

	void init_(void) {		
		swObjFiles * xfiles = swObjFiles::swFilesFactory("pfiles");
		swExCat::setXfiles(xfiles);
		all_filesetsM = strob_open(10);
	}
};

#endif
