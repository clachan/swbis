/* swstruct_i.h
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


#ifndef swstruct_i_19981001jhl_h
#define swstruct_i_19981001jhl_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swdefinition.h"
#include "swstruct.h"


class swIter;
class swStructIter;
class swExStruct;
class swStruct_i: public swStruct
{
  
  swDefinition * swdefM;
  public:
     swStruct_i (void);
     swStruct_i (char * object_name, int level);
     virtual ~swStruct_i (void);
 
     virtual swStruct * get_swsobjectnode(int index);
     virtual void  set_swsobjectnode(int index, swStruct * sws);
     virtual swDefinition * get_swdefinition(void);
     virtual void  set_swdefinition(swDefinition * swdef);
    
     virtual void set_swsobjectarray(swPtrList<swStruct> * swsarray);
     virtual swPtrList<swStruct> * get_swsobjectarray(void);

     virtual int get_type(void);
     virtual int get_level(void);
     virtual void set_level(int level);

     virtual int add_attribute (char * keyword, char * value);
     virtual char * get_attribute (swSelection * software_spec, char * keyword);
     //virtual void get_attribute (swSelection * software_spec, char * keyword, string &strbuf);
     virtual swMetaData *  get_attribute(int inode);
     virtual int delete_attribute (swSelection * software_spec, char * keyword);

     int insert_swstruct (swStruct * before, swStruct * node) {
     	return -1;
     }
     int add_swstruct (swStruct * node) {return -1;}
     int del_swstruct (swStruct * node) {return -1;}
     int get_index_from_pointer (swStruct * p) {return -1;}
     
     swStruct * get_pointer_from_index (int i){ return NULL;}

     int write_node(int uxfio_fd);
     int iwrite(int uxfio_fd);
     int write_debug(int uxfio_fd);
     void  set_iterator(swStructIter * clswit);
     swStructIter *  get_iterator(void);

     static swStruct * swstructure_factory(char * object_keyword);
     static char * swtruct_goto_next_line (void * cplusplus_object, char * headerline);
     int compare_tag (char * tag);
    
     virtual int doProcess(swExStruct * swex);
     virtual swExStruct * getSwExStructContext(swExStruct *);
     virtual char * determineControlDirectory(void);
     virtual char * determineFilesControlDirectory(void);
     virtual char * determineTag(void);

  private:
     int write_recurse1(int fd, int * size);

};
#endif
