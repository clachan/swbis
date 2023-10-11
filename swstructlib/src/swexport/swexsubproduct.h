/* swexsubproduct.h
 */

/*
 * Copyright (C) 2002  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swexsubproduct_i_h
#define swexsubproduct_i_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "swexstruct.h"
#include "swmetadata.h"
#include "swexstruct_i.h"
#include "swobjfiles_i.h"
#include "swobjfiles.h"
#include "swexpfiles.h"
#include "swexcat.h"

class swExSubproduct: public swExCat
{
  	
  public:

	swExSubproduct(char * control_dir): swExCat(control_dir){ init_(); }
  	swExSubproduct(void): swExCat(){ init_(); }
  	~swExSubproduct(void){}

	swPtrList<swExStruct> * getProductList() { return NULL; }
	
	char * getObjectName(void) { return "media"; }

	static swExStruct * make_exdist(void) {
		return new swExSubproduct("");
	}
	
	virtual int swobjfile_file_out_of_scope(swMetaData * swmd, int current_level) {
		return 1;
	}	
	
	int doesHaveControlFiles(char * key) {
		return 0;
	}

	int doesContain(char * key) {
		return 0;
	}

	void do_terminate_link(swDefinition * def) { 
		// do nothing; 
	}
	
	virtual int setControlDirectory(void){ return 0; } 
	
	char * getControlDirectory(void) {  return NULL; }
	
	//D char * dump_string_s(char * prefix) { return swExCat::dump_string_s(prefix); }
	
	private:

	void init_(void) {		
	}
};

#endif
