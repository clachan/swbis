/* swstructiter.cxx
 *
 */

/*
 * Copyright (C) 1999  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#include "swuser_config.h"
#include "switer.h"
#include "swstructiter.h"

void * 
swStructIter::find_next_by_ino_if(swIter * switer, int inode, int get_next, int get_sws, int get_by_ino)
{
	    int term_on=0;
	    swStructIter *swit=this;
	    swStruct *sw=swit->get_swstruct();
	    swMetaData *swm, *next;
	    while (swit) {
		if (!sw){
			swit=switer->pop(swit);
			if (swit){
				sw=swit->get_next_object();
			}
			continue;
       		 }
		if (term_on) {
			switer->popif(sw);
			if (get_sws) {
				return static_cast<void*>(sw);
			}
			return static_cast<void*>(sw->get_swdefinition()->get_attribute_by_index(0));
		}
		swit=switer->popif(sw);
		
		if (get_by_ino)
			swm=sw->get_swdefinition()->find_by_ino(inode);
		else
			swm=sw->get_swdefinition()->find_by_p_offset(inode);


		if (swm && !get_next){
			if (get_sws){
				return static_cast<void*>(sw);
			}
			return static_cast<void*>(swm);
		}
		if (swm && (next=swm->get_next_node())) {
			if (get_sws) return static_cast<void*>(sw);
			return static_cast<void*>(next);
    		} 
		if (swm) { // next is NULL, must be last attribute in definition.
			term_on=1;
		} 
		sw=swit->get_next_object();
	    }
	return static_cast<void*>(NULL);
}
