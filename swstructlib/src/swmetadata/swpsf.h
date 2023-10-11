/* swpsf.h
 */

/*
 * Copyright (C) 1999  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swPSF_19980516jhl_h
#define swPSF_19960516jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swdefinition.h"
#include "swdefinitionfile.h"
#include "swextendeddef.h"

class swPSF: public swDefinitionFile 
{
	swDefinition * swdefM;		// The current object containing ext. definitions.
	swExtendedDef * swextM;		// The Extended definition object.
public:
	swPSF(char * path): swDefinitionFile (path) { 
		init_(); 
	}
	
	~swPSF(void) { }

	int
	open_serial_archive(char * name, int uinfile_flags) {
		return xFormat_open_archive(name, uinfile_flags);
	}

	swExtendedDef * get_swextdef(void) { return swextM; }

	int
	open_serial_archive(int fd, int uinfile_flags) {
		return xFormat_open_archive(fd, uinfile_flags);
	}

	int
	open_directory_archive(char * dir) {
		return xFormat_open_archive(dir);
	}

	int
	close_archive(void) {
		return xFormat_close_archive();
	}

	void
	set_cksum_creation(int s) {
		swextM->set_cksum_creation(s);
	}
	
	void
	set_digests1_creation(int s) {
		swextM->set_digests1_creation(s);
	}
	
	void
	set_digests2_creation(int s) {
		swextM->set_digests2_creation(s);
	}

	void
	set_follow_symlinks(int s) {
		swextM->set_follow_symlinks(s);
	}

	virtual int get_type(void) { return swstructdef::return_entry_index(NULL, "PSF"); }
      
	virtual swMetaData * generate_attribute_list (int at_level, int * retvalp);

	swDefinition * makeDefinition(char * parser_line, int * retvalp);

	 char * getObjectName(void) { return "PSF"; }

private:
	void init_(void) {
		swextM = new swExtendedDef(xFormat_get_swvarfs());
		swdefM = static_cast<swDefinition*>(NULL);
	}
};
#endif
