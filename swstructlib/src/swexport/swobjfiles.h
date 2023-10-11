/* swobjfiles.h - The common (dfile/pfiles/fileset) exported file structure.
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

#ifndef swobjfiles_hxx
#define swobjfiles_hxx

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
#include "swobjfiles_i.h"
#include "swinfo.h"
#include "swptrlist.h"
#include "swpackagefile.h"
#include "swdefinitionfile.h"

class swINDEX;

class swObjFiles: public swObjFiles_i
{
	public:
	
	swObjFiles(void): swObjFiles_i(""){
		init_();
	}

	virtual ~swObjFiles(void) {
	}

	virtual swINDEX * getIndex(void){return NULL;}
	virtual void setIndex(swINDEX * sw){ }
	virtual char * getObjFileName(){ return  NULL;}

	//
	// Initialization of swExPfiles swExDfiles objects
	// control directories.
	//
	int initObjFiles(void) {
		char * myname;
		char * Ofiles;
		// swDefinition * referer = getReferer();
		swDefinition * dist = getDistributionFromGlobalIndex();

		//SWLIB_ASSERT(getInfo() != NULL);
		if (getInfo() == NULL) {
			swINFO * info =  new swINFO("INFO");
			SWLIB_ASSERT(info != NULL);
			setInfo(info);
		}

		//referer = getDistributionFromGlobalIndex();
		//SWLIB_ASSERT(referer != NULL);
		//setReferer(referer);
		
		SWLIB_ASSERT(dist != NULL);
		myname = getObjFileName();  // Either pfiles or dfiles.
		assert(myname);
		Ofiles = dist->find(myname);
		if (!Ofiles) {
			dist->add(myname, myname);
			Ofiles = dist->find(myname);
		}
		assert(Ofiles);	
		setControlDirectory(Ofiles);
		return 0;
	}

	int write_debug_stats(void){
		return 0;	
	}

	// virtual void doPass1payload(void) { initObjFiles(); }

	virtual void doInit1(void) { initObjFiles(); }

	char * determineControlDirectory(void) {
		swDefinition * swdef = getReferer();
		char * name = getObjectName();	// either "dfiles" or "pfiles"
		char * control_dir = static_cast<char*>(NULL);
	
		if (::strcmp(name, "pfiles") == 0) {
			//
			// Specialization hack for pfiles.
			//
			swdef = getDistributionFromGlobalIndex();
		}

		if (!swdef) {
			return static_cast<char*>(NULL);
		}

		control_dir = swdef->find(name);
		if (control_dir) {
			return control_dir;
		}
		if (!control_dir)
			control_dir = name;
		return control_dir;
	}

	static swObjFiles * swFilesFactory(char * xfiles_keyword);
	virtual swExStruct * getXfiles(void) { return NULL;  }
	
	virtual void performInitializationPass1(void) { }
	
	virtual void performInfoPass2(void) {
			swExStruct_i::adjustFileDefs(getInfo()); 
	}
	
	virtual void addControlDirectoryAttribute(void) { }
	
	private:

	void init_(void) { }


};
#endif
