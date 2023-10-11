/* swexhost.h
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

#ifndef swexhost2_i_h
#define swexhost2_i_h

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
#include "swptrlist.h"

//
// The swExHost object 
//

class swExHost: public swExCat
{
	STROB * targetPathM;
	swDefinition * previousM;
  	
  public:
	
	swExHost(void): swExCat(){
		init();
	}
  	
	virtual ~swExHost(void){}

	char * getObjectName(void) { return "host"; }

	char * getFormedControlPath(void){ return getParentPath(); }

	swDefinition * getPrevious(void){ return previousM; }

	void         setPrevious(swDefinition * s) { previousM = s; }

	swPtrList<swExStruct> * getFilesets(void) { return NULL; }

	swPtrList<swExStruct> * getStorageObjectList(void){ return NULL; }

	int doesContain(char * key) {
		return !strcmp(key, "distribution");
	}

	static swExStruct * make_exdist(void) {
		return new swExHost();
	}

	//static char * swexhost_dump_string_s(swExHost * pf, char * prefix);
	//char * dump_string_s(char * prefix) { return swexhost_dump_string_s(this, prefix); }
	
	void setLeadingPackagePath(char * path) {
		//int  list_index = 0;
		swExStruct 	* object = getD();
		setControlDirectory("");
		//while((object = containedByIndex(list_index++))) {
		//	object->setLeadingPackagePath(path);   // A host object only contains one distribution.
		//}
		SWLIB_ASSERT(object!=NULL);
		object->setLeadingPackagePath(path);   // A host object only contains one distribution.
	}
	
	void setGlobalIndex(swINDEX * a) {
		swExStruct 	* object = getD();
		SWLIB_ASSERT(object!=NULL);
		object->setGlobalIndex(a);
	}

	//
	// Add the control_directory attribute to each definition.
	//
	
	//void traversingVehicle(void (swExStruct::*payload)(void)) {
	//	swExStruct 	* object = getD();
	//	object->traversingVehicle(payload);
	//}

	void performInfoPass2(void) {
		swExStruct 	* object = getD();
		SWLIB_ASSERT(object!=NULL);
		 object->performInfoPass2();
	}
	
	void taskDispatcher(enum taskCodes code) {
		swExStruct 	* object = getD();
		SWLIB_ASSERT(object!=NULL);
		object->taskDispatcher(code);
	}
	
	void performInitializationPass1(void) {
		swExStruct 	* object = getD();
		SWLIB_ASSERT(object!=NULL);
		object->performInitializationPass1();
	}

	int registerWithGlobalIndex(void) {
		swExStruct 	* object = getD();
		
		SWLIB_ASSERT(object!=NULL);
		object->getGlobalIndex()->swdeffile_linki_append(getReferer());
		object->registerWithGlobalIndex();
		return 0;
	}

	int  createIndexFile(void) {
		//int  list_index = 0;
		swExStruct * object;

		//while((object = containedByIndex(list_index++))) {
		//	object->createIndexFile();
		//}
		
		object = getD();
		object->createIndexFile();
		
		return 0;
	}
	
	swINFO * getInfo(void) { 
		swExStruct * object;
		object = getD();
		return object->getInfo();	
	}
	
	swINDEX * getIndex(void) { 
		swExStruct * object;
		object = getD();
		return object->getIndex();	
	}
	
	//virtual swExStruct * getXfiles(void) { return NULL;  }

	virtual swExStruct * getXfiles(void) { 
		swExStruct * object;
		object = getD();
		return object == NULL ? object : object->getXfiles();	
	}
	
//	virtual int setupAttributeFiles(void) {
//		int ret;
//		swExStruct * object;
//		object = getD();
//		ret = object->setupAttributeFiles();
//		return ret;
//	}
//	
	virtual void addControlDirectoryAttribute(void) { }

  private:

	//
	// Private helper function to get the Distribution, A host object
	// may only contain one distribution Object.
	//
	swExStruct * getD(void) {
		swExStruct * object;
		object = containedByIndex(0);
		if (object && ::strcmp(object->getObjectName(), "distribution") != 0) {
			//
			// Fatal error.
			//
			fprintf(stderr, "Internal error, [Host Keyword]  distribution object not found.\n");
			exit(1);
		}
		return (object);
	}

	void init(void) {
		targetPathM = strob_open(10); 
		setLastExStruct(this);
		setControlDirectory(SW_A_catalog);
		previousM = static_cast<swDefinition*>(NULL);
		//swExCat::setXfiles(swObjFiles::swFilesFactory("dfiles"));
	}
};

#endif
