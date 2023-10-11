/* swnode.h: 
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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


#ifndef swsNode_19980901jhl_h
#define swsNode_19980901jhl_h

#include "swstruct_i.h"
#include "swdefinitionfile.h"
#include "swexstruct.h"
#include "swptrlist.h"


class swsNode: public swStruct_i {

   swPtrList<swStruct> * swsobjectarray_;
   static swStruct * distributionM;

public:
   swsNode(void);
   virtual ~swsNode(void);

   swPtrList<swStruct> * get_swsobjectarray(void);
   void set_swsobjectarray(swPtrList<swStruct> * swsarray);

   int insert_swstruct (swStruct * before, swStruct * node);
   int add_swstruct (swStruct * node);
   int del_swstruct (swStruct * node);
   int get_index_from_pointer (swStruct * p);
   swStruct * get_pointer_from_index (int index);
   static void swsnodeSetDistribution(swStruct * sws);
   static swStruct * swsnodeGetDistribution(void);
     
   virtual char * determineControlDirectory(void);

protected:
    int doProcess(swExStruct * swex)
    {
	swDefinition * swdef;
	swStruct_i::doProcess(swex);
	swex->setReferer(swdef);
    	return 0;
    }

    int doProcess_node_bh(swExStruct * distribution, swExStruct * swex)
    {
	swPtrList<swExStruct> * list = swex->getThisStorageList(distribution);
	int ret = swsNode::doProcess(swex);
	list->list_add(swex);
	return ret;
    }
};
#endif
