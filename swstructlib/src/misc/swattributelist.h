// swattributelist.h
//

// Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swattributelist_19980601jhl_h
#define swattributelist_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swmetadata.h"

class swAttributeList {
	swMetaData * next_;
	swMetaData * prev_;
    public:
	swAttributeList(void) { next_ = NULL; }
	swAttributeList(swMetaData * p) { next_ = p; }
	virtual ~swAttributeList(void){};


	//void add_at_end_of_list(swMetaData * newnode) {
	//	swMetaData * p = get_last_node();
	//	if (p) dynamic_cast<swAttributeList*>(p)->list_add (newnode);
	//}

	swMetaData * get_next_node (void) {
		return next_;
	}
    
	swMetaData * set_next_node (swMetaData * p) {
		next_ = p;
		 return next_;
	}

	void list_add (swMetaData * newnode) {
		newnode->set_next_node (next_);
		next_ = newnode;
	}

	static void  delete_list(swMetaData *header) {
		swMetaData *p, *pp;
		p = pp = header->get_next_node();
		delete header; 
		while ( p )
		{
			pp = p;
			p = p->get_next_node();
			delete p;
		}
	}

	swMetaData* get_last_node (void) {
	   swMetaData *p, *pp;
	   p = pp = get_next_node();
	   while ( p )
	     {
	       pp = p;
	       p = p->get_next_node();
	     }
	   return pp;
	}

	swMetaData * get_last_node_in_object (swMetaData * swi ) {
	   int target_level;
	   swMetaData * p = swi, *pp;
	     if (!p) return static_cast<swMetaData*>(NULL);
     
	     target_level = p->get_level();
     
	     pp=p;
	     while (p && p->get_level() == target_level) {
       		 pp=p;
       		 p=pp->get_next_node ();
     		}
	    return pp;
	}
};
#endif
