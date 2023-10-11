/* swcontrolfile.h
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



#ifndef swcontrolfile_19980720_h
#define swcontrolfile_19980720_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swmetadata.h"
#include "swpathname.h"
#include "swcatalogfile.h"


class swControlFile: public swCatalogFile
{

    public:

    swControlFile (char * pkg_path);
    swControlFile (char * pkg_path, char * src_path);
    virtual ~swControlFile (void);
    //virtual int read_from_package (swPackage * package);

};
#endif


