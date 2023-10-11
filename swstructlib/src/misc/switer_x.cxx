// switer_x.cxx

//  Copyright (C) 1999  Jim Lowe  <jhlowe@acm.org>

/*  
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

#include "swstructiter.h"
#include "switer.h"
#include <typeinfo>

extern "C" {
#include "swheaderline.h"
}

void swIter::show_debug(FILE * fp)
{
	int i=0;
	void * c=(void*)cplob_val(iteratorsM, i);
	//ostra << "ptos, stack_index :" 
	//	  << ptos_ <<", "
	//	  << stack_index_ <<"\n";
	fprintf(fp, "ptos, stack_index : %d, %d\n", ptos_, stack_index_);

	while (c) {
		fprintf(fp, "%d: ", i++);   //ostra << i++  << ": ";
		switer_x_write_debug(static_cast<swStructIter*>(c), fp);
		c=(void*)cplob_val(iteratorsM, i);
	}
}

void swIter::switer_x_write_debug(swStructIter* c, FILE * fp)
{
	//ostr  <<  (void*)(c)  << " "
	//	<<swheaderline_get_keyword(c->get_swstruct()->get_swdefinition()->get_parserline())
	//	<< ": level=" 
	//	<<c->get_swstruct()->get_swdefinition()->get_level() 
	//	<< ": ino=" 
	//	<<c->get_swstruct()->get_swdefinition()->get_ino() 
	//	<< ": swstruct=" 
	//	<<c->get_swstruct()
	//	<< ": object_index_=" 
	//	<<c->get__object_index()
	//	<< "\n";


	fprintf(fp,  "%p  %s:level=%d:: ino=%d:: swstruct=%p:: object_index_=%d\n",
			(void*)(c),
			swheaderline_get_keyword(c->get_swstruct()->get_swdefinition()->get_parserline()),
			c->get_swstruct()->get_swdefinition()->get_level(),
			c->get_swstruct()->get_swdefinition()->get_ino(),
			c->get_swstruct(),
			c->get__object_index());


}


