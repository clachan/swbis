/* swattributefile.cxx
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.
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
#include "swattributefile.h"


swAttributeFile::swAttributeFile(char * pkg_name): swCatalogFile(pkg_name, "")
{
}

swAttributeFile::swAttributeFile(char * pkg_name, char * src_name): swCatalogFile(pkg_name, src_name)
{
}

swAttributeFile::~swAttributeFile (void) { 
}

