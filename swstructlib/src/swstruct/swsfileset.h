#ifndef swsfileset_19980601jhl_h
#define swsfileset_19980601jhl_h

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
#include "swexfileset.h"

class swsFileset: public swsNode {

public:
    swsFileset (void);
    virtual ~swsFileset (void);
    static swsFileset * make_swstructure(void);

    int doProcess(swExStruct * swexdist)
    {
	//swExStruct * fileset = swExFileset::make_swexstruct(this);
	//int ret = doProcess_node_bh(swexdist, fileset);
	int ret = 0;
	return ret;
    }
};
#endif
