/* swmemfile.h - Manage/Store the offset and and length of the
 *               file stored the uxfio memory file.
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


#ifndef swmemfile_1999_h
#define swmemfile_1999_h



extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <typeinfo>
#include <errno.h>

extern "C" {
#include "uxfio.h"
}

class swMemFile
{
  private:
	int   fdM;
  public:
     
	swMemFile (void) { 
		fdM = -1;
	}

	virtual ~swMemFile (void){
		swMemFile_u_close();
	}

	int	swMemFileGetfd(void){ 
		return fdM; 
	}
	
	int	swMemFile_u_creat(void) {
		swMemFile_u_close();
		initialize();
		return fdM;
	}

	int	swMemFile_u_open(void) {
		if (fdM < 0)
			swMemFile_u_creat();
		uxfio_lseek(fdM, 0, SEEK_SET);
		return fdM;
	}
	
	void	swMemFile_u_close(void) {
		if (fdM >= 0) uxfio_close(fdM);
		fdM = -1;
	}
	
	void initialize(void) {
		//int nullfd;

		fdM = uxfio_open("", O_RDWR, 0);
		
		if (fdM < 0) {
			fprintf(stderr, "swmemfile: uxfio_open failed, fatal\n");
			exit(23);
		}
		uxfio_fcntl(fdM, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
		uxfio_fcntl(fdM, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	}

};
#endif
