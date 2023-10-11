/* swfilemap.h - Provide a stack of virtual files within a file.
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


#ifndef swfilemap_1999_h
#define swfilemap_1999_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <typeinfo>
extern "C" {
#include "swuser_config.h"
#include "strob.h"
}

class swFileMap
{
  private:
	struct vFile {
		int   start_offsetM;		// Offset in a manifold file to the virtual file.
		int   current_offsetM;		// Current offset in manifold file.
		int   lenM;			// Length of virtual file within manifold file.
  	};
  	STROB * stackM;				// Stack of vFile objects.
	int   stackIndexM;			// Stack index.
  	vFile userM;				// The vfile object.
  public:
     
	swFileMap (void) {
		stackIndexM = 0;
		stackM = strob_open(sizeof(vFile) * 10);
	}
	
	virtual ~swFileMap (void){
		strob_close(stackM);	
	}

	int swFileMapPush(int offset, int len){
		int i =  sizeof(vFile) * stackIndexM;
		userM.current_offsetM = userM.start_offsetM = offset;
		userM.lenM=len;
		strob_memmove_to(stackM, i, static_cast<void*>(&userM), sizeof(vFile));
		return stackIndexM++;
	}
	
	int swFileMapPop(int *start_offset, int * current_offset, int *len){
		if (stackIndexM == 0)
			return -1;
		return swFileMap_get(start_offset, current_offset, len, --stackIndexM);
	}
	
	int swFileMapPeekByIndex(int index, int *start_offset, int *current_offset, int *len){
		if (index < 0) 
			index = stackIndexM;
		return swFileMap_get(start_offset, current_offset, len, index);
	}

	int swFileMapPeek(int *start_offset, int *current_offset, int *len){
		if (stackIndexM == 0)
			return -1;
		return swFileMap_get(start_offset, current_offset, len, stackIndexM - 1);
	}
	
	int swFileMapSetCurrentOffset(int offset){
		vFile * vf;
		if (stackIndexM == 0)
			return -1;
		vf = swFileMap_get_vFile(stackIndexM - 1);
		vf->current_offsetM = offset;
		return 0;
	}

	int swFileMapReset(void){
		int start_offset, current_offset, len;
		int ret = swFileMapPeek(&start_offset, &current_offset, &len);
		if (ret < 0) return ret;
		return swFileMapSetCurrentOffset(start_offset);
	}
	
	private:
	
	int swFileMap_get(int *start_offset, int *current_offset, int *len, int index){
		vFile * vf = swFileMap_get_vFile(index);
		if (start_offset) *start_offset = vf->start_offsetM;
		if (current_offset) *current_offset = vf->current_offsetM;
		if (len) *len = vf->lenM;
		return index;
	}
	
	vFile * swFileMap_get_vFile(int index){
		return static_cast<vFile*>(static_cast<void*>(strob_str(stackM) + (index * sizeof(vFile))));
	}
};
#endif
