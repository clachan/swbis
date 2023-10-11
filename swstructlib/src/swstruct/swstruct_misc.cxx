/* swstruct_misc.cxx
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



#include "swuser_config.h"
#include "swstruct_i.h"
#include "swsfile.h"
#include "swscontrolfile.h"
#include "swsbundle.h"
#include "swsfileset.h"
#include "swssubproduct.h"
#include "swscategory.h"
#include "swsvendor.h"
#include "swsmedia.h"
#include "swsdistribution.h"
#include "swscategory.h"
#include "swsproduct.h"
#include "swshost.h"
#include "swsvendor.h"
#include "swstructiter.h"
#include "switer.h"

//#include "stream_config.h"

int swStruct_i::write_debug(int uxfio_fd) 
{
    int len=0;
    swStruct * sw=this;
    swIter switer(sw);
    swStructIter * swit=switer.peek();
    
    fprintf(stdout, "made: %p\n", (void*)swit);
    fflush(stdout);
    len+=sw->write_node(uxfio_fd);

    while (swit) {
	fflush(stdout);
   	switer.show_debug(stdout); 
	fflush(stdout);
	sw=swit->get_next_object();
	if (!sw){
		swit=switer.pop(NULL);
		fprintf(stdout, "+++++++++++++++++ pop\n");
		fflush(stdout);
		continue;
        }
	swit=new swStructIter(sw);
	switer.push(swit);
	fprintf(stdout, "made:  %p\n", (void*)swit);
	fflush(stdout);
	len+=sw->write_node(uxfio_fd);
    }
    fprintf(stdout, "Finished.\n");
    switer.show_debug(stdout); 
    return len;
}



