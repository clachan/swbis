/* swheader_d.c  --  Debugging functions.
 */

/*
   Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
   All rights reserved.
  
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <tar.h>

#include "taru.h"
#include "strob.h"
#include "cplob.h"
#include "uxfio.h"
#include "swlib.h"
#include "uinfile.h"
#include "swheader.h"
#include "swheaderline.h"

void
swheader__dump(SWHEADER * swheader)
{
	int i = 0;

 	fprintf(stderr, "image_head_ = %p\n", swheader->image_head_);
 	fprintf(stderr, "image_object_ = %p\n", swheader->image_object_);

	for (i=0; i <=SWHEADER_IMAGE_STACK_LEN; i++) {
 		fprintf(stderr, "image_object_stack_[%d] = %p\n", i, swheader->image_object_stack_[i]);
	}

	fprintf(stderr,"current_offset_p_=%p\n",(void*)(swheader->current_offset_p_));
	fprintf(stderr,"*current_offset_p_=%d\n",*(swheader->current_offset_p_));
	fprintf(stderr,"current_offset=%d\n",(swheader->current_offset_));
}


void
swheader__print_all_objects(SWHEADER * swheader)
{
	int my_offset=0;
	char * next_line;

	/* step to first object */


	swheader_set_current_offset_p(swheader, &my_offset);
	swheader_set_current_offset_p_value(swheader, 0);
	
/*
	next_line=swheader_get_current_line(swheader);
	while (next_line && swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ)
		next_line=swheader_goto_next_line(swheader, my_offset);
*/

	next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
	while (next_line){
		swheaderline_write_debug(next_line, STDOUT_FILENO);	
		next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
	}

}


