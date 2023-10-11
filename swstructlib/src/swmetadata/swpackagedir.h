/* swpackagedir.h
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


#ifndef swpackagedir_19980601jhl_h
#define swpackagedir_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <tar.h>
#include "swpackagefile.h"

class swPackageDir : public swPackageFile
{
	public:
	swPackageDir (char * pkg_filename);
	virtual ~swPackageDir(void);
     
	virtual int    	swfile_open_public_image_fd	(void) { return 0; } 
	virtual int    	swfile_close_public_image_fd	(void) { return 0; }
	virtual void	swfile_set_default_statbuf(void);


	virtual intmax_t xFormat_write_file(void) {
		intmax_t ret;
		char * name;

		name = swfile_get_package_filename();	

		xFormat_set_filetype_from_tartype(DIRTYPE);

		ret = swPackageFile::xFormat_write_file(static_cast<struct stat*>(NULL), name, -1);
		return ret;
	}
};
#endif
