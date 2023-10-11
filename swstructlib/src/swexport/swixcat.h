/* swixcat.h
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

#ifndef swixcat_1_h
#define swixcat_1_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "swuser_assert_config.h"
#include "swmetadata.h"
#include "swexstruct.h"
#include "swexcat.h"

class swIxCat: public swExCat
{
  public:
	swIxCat(char * parent_path): swExCat(parent_path) { init(); }
	swIxCat(void): swExCat("") { init(); }
	virtual ~swIxCat(void) {  }
	
	virtual swExStruct * getXfiles(void) { return NULL; }
	
	void setupAttributeFiles(void) { }
	
	void setupControlScriptFiles(void) { }
	
	int printDebugStructure(int fd) { return 0; }
	
	virtual swINFO * getInfo(void) { return NULL; }
	
	void performInfoPass2(void) {  }
	
	virtual void addControlDirectoryAttribute(void) { }

	virtual swPtrList<swPackageFile> * getControlScriptList(void) {
			return NULL; }
	
	virtual swExStruct * get_first_product(int * found) { return NULL; }

  private:
	void init(void) { }

};
#endif
