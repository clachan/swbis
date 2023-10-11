#ifndef swstruct_19980901jhl_h
#define swstruct_19980901jhl_h

/*  
     Copyright (C) 1998  James H. Lowe, Jr.
*/

/*
//  COPYING TERMS AND CONDITIONS:
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "swptrlist.h"

class swMetaData;
class swSelection;
class swDefinition;
class swStructIter;
class swExStruct;

class swStruct {

public:
    virtual ~swStruct() { }
    virtual swStruct * get_swsobjectnode (int index)=0;
    virtual void  set_swsobjectnode (int index, swStruct * sws)=0;
    virtual swDefinition * get_swdefinition(void)=0;
    virtual void  set_swdefinition(swDefinition * swdef)=0;
    virtual swPtrList<swStruct> * get_swsobjectarray(void)=0;
    virtual void set_swsobjectarray(swPtrList<swStruct> * swsarray)=0;

    virtual int get_type(void)=0;
    virtual int get_level(void)=0;
    virtual void set_level(int level)=0;

    virtual int add_attribute (char * keyword, char * value)=0;
    virtual char * get_attribute (swSelection * software_spec, char * keyword)=0;
    virtual swMetaData * get_attribute(int inode)=0;
    virtual int delete_attribute (swSelection * software_spec, char * keyword)=0;
     
    virtual char * determineControlDirectory(void) = 0;
    virtual char * determineFilesControlDirectory(void) = 0;
    virtual char * determineTag(void) = 0;

    virtual int add_swstruct(swStruct * node)=0;
    virtual int del_swstruct(swStruct * node)=0;
    virtual int insert_swstruct(swStruct * node1, swStruct * node2)=0;
    virtual int get_index_from_pointer(swStruct * node)=0;
    virtual swStruct * get_pointer_from_index(int index)=0;

    virtual int write_node(int uxfio_fd)=0;
    virtual int iwrite(int uxfio_fd)=0;
    virtual int compare_tag(char * tag_string)=0;
    
    //
    // SwExstruct transformation routines. 
    //
    virtual int doProcess(swExStruct * swexstruct)=0;
    virtual swExStruct * getSwExStructContext(swExStruct *swExDistribution)=0;
};
#endif
