#ifndef swsDistribution_19980901jhl_h
#define swsDistribution_19980901jhl_h

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
#include "swsnode.h"


class swsDistribution: public swsNode {

//    swStructArray * swsattributefiles_;
    
public:
    swsDistribution (void);
    virtual ~swsDistribution (void);
    static swsDistribution * make_swstructure (void);
   
    int doProcess(swExStruct * swexdist)
    {
	int ret = 0;
	swDefinition * swdef = get_swdefinition();
   	//swDefinitionFile * index = swexdist->getGlobalIndex();
	char * lv = swdef->find("layout_version");

	// Common Processing.
        ret = swsNode::doProcess(swexdist);

	// Add 'layout_version' if required.
        if (!lv) {
		swAttribute * swatt;
		swatt = new swAttribute("layout_version", "1.0");
		swatt->set_level(1);
		swdef->list_add(swatt);
	}
    	return ret;
    }
    char * determineFilesControlDirectory(void)
    {
	swDefinition * swdef = get_swdefinition();
	char * dfiles = swdef->find("dfiles");
	if (dfiles)
		return dfiles;
	else
		return "dfiles";
    }
};
#endif
