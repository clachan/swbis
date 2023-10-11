// swattributemem.h

// Copyright (C) 1998  James H. Lowe, Jr.  <jhl@richmond.infi.net>
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


#ifndef swattributemem_19980601jhl_h
#define swattributemem_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

extern "C" {
#include "swparse.h"
#include "strob.h"
#include "uxfio.h"
#include "uxfio_i.h"
}

class swAttributeMem {

	int data_fd_;
    public:
	void * uxfio_addrM;

	swAttributeMem(void) {
		data_fd_=uxfio_open("/dev/null", O_RDONLY, 0 );
		if (uxfio_fcntl(data_fd_, UXFIO_F_SET_BUFACTIVE, UXFIO_ON)) {
			fprintf (stderr,"error in swattributemem 1 .\n");
		}
		if (uxfio_fcntl(data_fd_, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM)) {
			fprintf (stderr,"internal error in swattributemem 2.\n");
		}
		if (data_fd_ >= 0)
			uxfio_addrM = uxfio_get_object_address(data_fd_);
		else
			uxfio_addrM = NULL;
	}
	
	virtual ~swAttributeMem(void) {
		::uxfio_close(data_fd_);
	}
	
	inline int get_mem_offset(void) {
		return (int)(((UXFIO*)(uxfio_addrM))->current_offsetM);
		//return ::uxfio_lseek(data_fd_, 0, SEEK_CUR);
	}

	void * get_mem_addr(void) {
		return (void*)(((UXFIO*)uxfio_addrM)->bufM);
		/*	
		char *p; 
		
		if (((UXFIO*)uxfio_addrM)->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM) {
			p = ((UXFIO*)uxfio_addrM)->bufM;
		} else {
			p = NULL;
		}
		
		//OLD Slow correct way     ::uxfio_get_dynamic_buffer(data_fd_, &p, NULL, NULL);
		return (void*)(p);
		*/
	}

	inline int get_mem_fd(void) {
		return data_fd_;
	}

	int  set_buftype (int arg1, int arg2) {
		return ::uxfio_fcntl(data_fd_, arg1, arg2);
	}
};
#endif
