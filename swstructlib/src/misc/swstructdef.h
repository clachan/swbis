/* swstructdef.h
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

#ifndef swstructdef_19980513jhl_h
#define swstructdef_19980513jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

extern "C" {
#include "swsdflt.h"
}

class swstructdef
{
   static struct swsdflt_defaults * current_;
     
   public:

   swstructdef (void) { 
   }
   ~swstructdef (void){ 
   }

   enum obj_type { sdf_unknown, sdf_object_kw, sdf_attribute_kw, sdf_extended_kw, sdf_filereference_kw };

   enum swdef_sanction { sdf_POSIX_7_2, sdf_swbis, sdf_this_implementation, sdf_dpkg, sdf_rpm};
   enum swdef_group {sdf_not_found, sdf_id, sdf_attr, sdf_objs, CONTROLFILE_EXT, FILE_EXT, SWDEFFILE};
   enum swdef_value_type {sdf_single_value, sdf_list};
   enum swdef_has_default_value {sdf_no, sdf_yes, sdf_IMDEF /*ImplementationDefined*/ };
   enum swdef_length {sdf_undefined};

	static struct swsdflt_defaults * defaults() {
		return swsdflt_defaults_array();
	}

	static int get_attr_group (int index) {
		current_=swstructdef::defaults() + index;
		return (int)(current_->group);
	}

	static int is_sw_keyword(char * object, char * keyword ) {
		if ( swstructdef::return_entry(object, keyword) != NULL) {
			return 1;
		} else {
			return 0;
		}
	}

	static int return_entry_group(int entry_index) {
		if (entry_index == 0) return -1;
		return ::swsdflt_return_entry_group(entry_index);
	}

	static struct swsdflt_defaults * return_entry(char * object, char * keyword ) {
		return current_=::swsdflt_return_entry(object, keyword);
	}

	static int return_entry_index (char * object, char * keyword) {
		current_=::swsdflt_return_entry(object, keyword);
		if (!current_) return 0;	
		return current_- ::swsdflt_defaults_array();
	}

	static char * return_entry_keyword (char * buf, int entry_index) {
		 return ::swsdflt_return_entry_keyword(buf, entry_index);
	}

	/*
	static char * get_object_keyword(void) {
		return swsdflt_get_object_keyword(current_);
	}

	static int get_sanction(void) {
		return swsdflt_get_sanction(current_);
	}

	static int get_group(void) {
		return swsdflt_get_group(current_);
	}
	*/

	static char * get_keyword(void) {
		return swsdflt_get_keyword(current_);
	}

	static int get_value_type(void){
		return swsdflt_get_value_type(current_);
	}

	/*
	* static char * get_permitted_value(void) {
	* 	return swsdflt_get_permitted_value(current_);
	* }
	*/

	static int get_has_default_value(void) {
		return swsdflt_get_has_default_value(current_);
	}

	static char * get_default_value(void) {
		return swsdflt_get_default_value_by_ent(current_);
	}
};
#endif
