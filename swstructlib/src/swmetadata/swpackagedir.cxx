/* swpackagedir.cxx
 *
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
#include <tar.h>
#include "swpackagedir.h"

swPackageDir::swPackageDir(char * pkg_filename): swPackageFile(pkg_filename, pkg_filename)
{ 
	swfile_set_default_statbuf();
}

swPackageDir::~swPackageDir(void){ }

void swPackageDir::swfile_set_default_statbuf(void) {
	swPackageFile::swfile_set_default_statbuf();
	xFormat_set_mode(0755);		// File type must be set after this.
	xFormat_set_filetype_from_tartype(DIRTYPE);
	xFormat_set_filesize(0);
	xFormat_set_mtime (time(NULL));
}


