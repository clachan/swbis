#ifndef swscollection_19980901jhl_h
#define swscollection_19980901jhl_h

/*  
   Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
*/
/*
//
//  COPYING TERMS AND CONDITIONS
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3, or (at your option)
//  any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
*/


#include "swmetadata.h"
#include "swdefinitionfile.h"

class swStruct;

class swsCollection{
	int swFileMapFdM;
	swDefinitionFile * swdeffileM;
	swStruct * collectionM;

   public:
	swsCollection(swDefinitionFile * swdeffile);
	virtual ~swsCollection(void);
	int iwrite(int fd);
	int write_debug(int fd);
	swStruct * get_swstruct (void);
	swDefinitionFile * get_swdeffile(void);
	int generateStructures(void);
	int generateStructuresFromParser(void);
	int generateStructuresFromParser(int);
	int rewriteExpandedPsf(void);

	int swFileMapReset(void) {
		int ret;
		int offset;
		int current_offset;
		int len;
		swdeffileM->swFileMapReset();
		swdeffileM->swFileMapPeek(&offset, &current_offset, &len);
		ret = uxfio_lseek(swdeffileM->get_mem_fd(), current_offset, SEEK_SET);
		return ret;
	}

   private:
	int generate_from_swdefinitionfile(swDefinitionFile * swdeffile);
	int generate_from_parser (swDefinitionFile * swdeffile, int);
	static swStruct * generate_from_deffile_recurse(swDefinitionFile * swdeffile, swStruct ** stack, int * retval, int);
	static swStruct * generate_from_swdefinitionfile_i(swDefinitionFile * swdeffile, swStruct ** stack, int * retval);

};
#endif
