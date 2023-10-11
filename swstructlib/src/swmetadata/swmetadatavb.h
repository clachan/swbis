/* swmetadata.h - Fat interface Abstract Base Class.
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


#ifndef swmetadata_19980601jhl_h
#define swmetadata_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <typeinfo>
#include "swmetadata.h"

class swPackage;
//class swMetaData;

class swMetaDataVB {

public:
    
    // swAttributeList 
    virtual swMetaData * get_last_node (void) = 0;
    virtual swMetaData * get_next_node (void) = 0;
    virtual swMetaData * set_next_node (swMetaData * new_next) = 0;
    virtual swMetaData * get_last_node_in_object (swMetaData * swthis ) = 0; 
    virtual void  	list_add 	(swMetaData * newnode) = 0;
    virtual void  	list_insert	(swMetaData * newnode, swMetaData * location) = 0;

    // swDefinition 
    virtual int list_add_if_new (swMetaData * newnode) = 0;
    virtual void  list_replace (swMetaData * newnode) = 0;
    virtual void list_replace_if_not_explicitly_set(swMetaData * swmd) = 0;
    
    // swAttributeMem
    virtual void *  get_mem_addr (void) = 0;
    virtual int     get_mem_offset (void) = 0;
    virtual int     get_mem_fd (void) = 0;
    
    virtual void   set_ino(int offset) = 0;
    virtual int    get_ino(void) = 0;
    virtual void   set_p_offset (int offset) = 0;
    // virtual int    get_p_offset (void) = 0;
    virtual void   set_length (int len) = 0; 
    virtual void   set_length_res (int len) = 0; 
    virtual int    get_length (void) = 0; 
    //virtual int    get_length_res (void) = 0; 
    virtual void   set_type  (int type) = 0; 
    virtual int    get_type  (void) = 0; 
    virtual void   set_level (int i) = 0;
    virtual int    get_level (void) = 0;
    // virtual void   set_status(char c) = 0;
    // virtual char   get_status(void) = 0;
    virtual char * get_keyword (void) = 0;
    virtual char * get_value (int * length) = 0;
    virtual swMetaData * get_contained_by(void) = 0;
    virtual void set_contained_by(swMetaData * parent) = 0;

    virtual int    write_fd (int fd) = 0;
    virtual int    write_fd_debug(int fd, char * prefix) = 0;
    virtual int    add (char * keyword, char * value, swMetaData * newnode, int level) = 0;
    virtual char * set_value (char * value) = 0;
    virtual char * set_value (swMetaData * source) = 0;
    virtual char * set_keyword (char * keyword) = 0;
    virtual char * get_parserline(void)=0;

    virtual int    insert (char * line, swMetaData * location) = 0;
    virtual int    insert (char * keyword, char * value) = 0;
    virtual void   vremove (void) = 0;

    // Provides Virtualness for overriding the swpsf{} class.
    virtual swMetaData * generate_attribute_list(int at_level, int * retvalp) = 0;
};

#endif

