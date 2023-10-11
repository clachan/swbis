/*  swdefinition.h
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.
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

#ifndef swdefinition_19980601jhl_h
#define swdefinition_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swattribute.h"
#include "swpathname.h"
#include "swstructdef.h"

extern "C" {
#include "swparse.h"
#include "strob.h"
#include "uxfio.h"
#include "swutilname.h"
}

#ifdef SWDEFINITIONNEEDDEBUG
#define SWDEFINITION_DEBUG(format) SWBISERROR("SWDEFINITION DEBUG: ", format)
#define SWDEFINITION_DEBUG2(format, arg) SWBISERROR2("SWDEFINITION DEBUG: ", format, arg)
#define SWDEFINITION_DEBUG3(format, arg, arg1) SWBISERROR3("SWDEFINITION DEBUG: ", format, arg, arg1)
#else
#define SWDEFINITION_DEBUG(arg)
#define SWDEFINITION_DEBUG2(arg, arg1)
#define SWDEFINITION_DEBUG3(arg, arg1, arg2)
#endif


#define SWDEF_STATUS_IGN	0	/* Exclude from storage section */
#define SWDEF_STATUS_NOM	1	/* Default, do include in storage section */
#define SWDEF_STATUS_DUP	2	/* has duplicates, take exceptional action */
#define SWDEF_STATUS_DUP0	3	/* definition of first duplicate */

class swDefinition: public swAttribute {
	swDefinition * nextM;
	swDefinition * prevM;
	int no_statM;
	int storage_statusM;	// 1 is nominal, 0 means it won't appear in the storage section.
	STROB * sbufM;		// Used for debugging.
public:
    swDefinition (char * name, int level);
    swDefinition (void);
    
    virtual ~swDefinition (void);
    static swDefinition * swdefinition_factory(char * parserline);

    char * findPhysical(char * keyword);

    inline swDefinition * get_prev(void) { return prevM; }
    inline swDefinition * get_next(void) { return nextM; }
    void set_next(swDefinition * next) { nextM = next; }
    void set_prev(swDefinition * prev) { prevM = prev; }

    void set_storage_status(int c) { storage_statusM = c; }
    int get_storage_status(void) { return storage_statusM; }

    virtual void set_level(int level);

    virtual void set_type(int i);
    virtual int get_type(void);
    virtual swAttribute * add(char * keyword, char * value);
    virtual swAttribute * add(char * keyword, char * value, int status);
    
    virtual void list_add(swMetaData * swmd);
    virtual void list_insert(swMetaData * swmd, swMetaData * location);
    virtual int list_add_if_new(swMetaData * swmd);
    virtual void list_replace(swMetaData * swmd);
    virtual void list_replace_if_not_explicitly_set(swMetaData * swmd);
    
    swMetaData * get_attribute_by_index(int n);
    char * 	 get_parserline_by_index(int n);
    
    int 	write_fd_debug (int fd, char * prefix);
    int 	write_fd (int fd);

    int get_no_stat(void) { return no_statM; }
    void set_no_stat(int c) { no_statM = c; }

    char * deleteAttribute (char * keyword);
    char * find (char * keyword);
    virtual char * getPathAttribute(void);
    virtual char * getTagAttribute(void);
 
    virtual void set_path_attribute(char * source);
    virtual void set_tag_attribute(void);

    void merge(swDefinition * source, int do_all, int do_not_replace);
    swMetaData *  findAttribute(char * keyword);
    swMetaData *  find_by_ino(int offset_key);
    swMetaData *  find_by_p_offset(int offset_key);
    swMetaData *  find_by_parserline(char * line);
    static swDefinition * make_newDefinition (swDefinition * parent_swdef, char * object_keyword);
    void setup_contained_by(void);
    virtual void apply_file_stat_specialization(char filestats[][file_permsLength], int fileArrayWasSet[]);
    virtual int apply_cksum_policy_specialization(int cksumflags);
    void vremove(char * keyword);
    void squash_duplicates(void);
private:
    void insert_before_location(swMetaData * swmd, swMetaData * location);
    int i_squash_duplicates(void);

};

#endif
