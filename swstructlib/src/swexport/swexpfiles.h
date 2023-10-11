/* swexpfiles.h - The common pfiles exported file structure.
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

#ifndef swexpfiles_hxx
#define swexpfiles_hxx

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

class swExPfiles: public swObjFiles
{
	public:
	
	static swExPfiles * makeXfiles(void) { return new swExPfiles(); }
	
	swExPfiles(void): swObjFiles(){
		init();
		return;
	}

	virtual ~swExPfiles(void) { }

	swINDEX * getEmittedGlobalIndex(void){ return NULL; }

	swINDEX * getIndex(void){return NULL;}

	char * getObjFileName(){ static char d[]="pfiles"; return d;}

	char * getObjectName(void) {
		return "pfiles";
	}
	
	private:
	void init(void) {
		//setControlDirectory();
	}
	

};
#endif
