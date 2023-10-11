#ifndef swsFile_19980901jhl_h
#define swsFile_19980901jhl_h

/*  

   Copyright (C) 1998  James H. Lowe, Jr.  <jhl@richmond.infi.net>

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

#include "swstruct_i.h"
#include "swexstruct.h"


class swsFile: public swStruct_i {
public:
    swsFile (void);
    virtual ~swsFile(void);
    static swsFile * make_swstructure(void);
    
	int doProcess(swExStruct * swexdist)
	{
		swDefinition * swdef = get_swdefinition();
		swExStruct * parent = getSwExStructContext(swexdist);
		// Dead code. return parent->addPackageFile(swdef);
		return 1;
	}
	virtual char * determineTag(void)
	{
		swDefinition * swdef = get_swdefinition();
		char * tag;
	
		if (!swdef) return static_cast<char*>(NULL);
		tag = swdef->find(SW_A_path);
		if (tag) {
			return tag;
		}
		return static_cast<char*>(NULL);
	}
};
#endif
