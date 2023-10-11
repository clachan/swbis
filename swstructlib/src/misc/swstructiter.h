/* swstructiter.h :  An iterator index for swstruct descendents.
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


#ifndef swstructiter_h_19981018
#define swstructiter_h_19981018

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swmetadata.h"
#include "swdefinition.h"
#include "swstruct.h"

extern "C" {
#include "debug_config.h"
#ifdef SWITERNEEDDEBUG
#define SWSTRUCTITER_E_DEBUG(format) SWBISERROR("SWSTRUCTITER DEBUG: ", format)
#define SWSTRUCTITER_E_DEBUG2(format, arg) SWBISERROR2("SWSTRUCTITER DEBUG: ", format, arg)
#define SWSTRUCTITER_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWSTRUCTITER DEBUG: ", format, arg, arg1)
#else
#define SWSTRUCTITER_E_DEBUG(arg)
#define SWSTRUCTITER_E_DEBUG2(arg, arg1)
#define SWSTRUCTITER_E_DEBUG3(arg, arg1, arg2)
#endif /* SWITERNEEDDEBUG */
}

class swIter;
class swStructIter {
           int object_index_;
	   int index_save_;
	   swStruct * sw_;
      public:
	
	swStructIter(swStruct * s) { 
		sw_=s;
		init_();
	}

	~swStructIter() { }

	void reset(void) { init_(); }

	int get__object_index(void){ return  object_index_;}
	void undo_mark(void) {index_save_=object_index_;}
	void undo_back(void) {object_index_=index_save_;}
	swStruct * get_next_object(void) { return sw_->get_pointer_from_index(object_index_++); }
           
	swStruct * peek_current_object(void) { return sw_->get_pointer_from_index(object_index_); }
          
	swStruct * get_swstruct(void) { return sw_; }

	swMetaData * find_by_p_offset(swIter * switer, int physical_offset) {
		swMetaData * p;
		p = static_cast<swMetaData *>(find_next_by_ino_if(switer, physical_offset, 0, 0, 0));
		SWSTRUCTITER_E_DEBUG3("physical_offset=%d  retval=%p", physical_offset, static_cast<void*>(p));
		return p;
	}

	swStruct * find_swstruct_by_p_offset(swIter * switer, int physical_offset)
	{
		swStruct * p;
		p = static_cast<swStruct*>(find_next_by_ino_if(switer, physical_offset, 0, 1, 0));
		SWSTRUCTITER_E_DEBUG3("physical_offset=%d  retval=%p", physical_offset, static_cast<void*>(p));
		return p;
	}

	swStruct * find_swstruct_by_ino(swIter * switer, int inode)
	{
		swStruct * p;
		p = static_cast<swStruct*>(find_next_by_ino_if(switer, inode, 0, 1, 1));
		SWSTRUCTITER_E_DEBUG3("inode=%d  retval=%p", inode, static_cast<void*>(p));
		return p;
	}

	swMetaData * find_by_ino(swIter * switer, int inode)
	{
		swMetaData * p;
		p = static_cast<swMetaData *>(find_next_by_ino_if(switer, inode, 0, 0, 1));
		SWSTRUCTITER_E_DEBUG3("inode=%d  retval=%p", inode, static_cast<void*>(p));
		return p;
	}

	swMetaData * find_next_by_ino(swIter * switer, int inode)
	{
		swMetaData * p;
		p =  static_cast<swMetaData *>(find_next_by_ino_if(switer, inode, 1, 0, 1));
		SWSTRUCTITER_E_DEBUG3("inode=%d  retval=%p", inode, static_cast<void*>(p));
		return p;
	}
	void * find_next_by_ino_if(swIter * switer, int inode, int get_next, int get_sws, int get_by_ino);

// --------- Private Functions --------------------

private:	  	 

	void init_(void){
		index_save_= 0;
		object_index_= 0; 
	}
};

#endif
