// swpsfi.cxx
//  Copyright (C) 1998  James H. Lowe, Jr.  <jhl@richmond.infi.net>
// 
//  COPYING TERMS AND CONDITIONS:
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

#include "swuser_config.h"
#include "swpsfi.h"
       
swPSFi::swPSFi (void): swDefinitionFile("PSFi") { }
swPSFi::swPSFi (char * path): swDefinitionFile(path) { }
swPSFi::swPSFi (char * path, int fd): swDefinitionFile(path) { }
swPSFi::~swPSFi (void) { }
int swPSFi::get_type(void) { return swstructdef::return_entry_index(NULL, "PSFi");}