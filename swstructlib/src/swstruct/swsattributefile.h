/*  swsattributefile.h
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


#ifndef swsAttributeFile_19980901jhl_h
#define swsAttributeFile_19980901jhl_h

#include "swstruct_i.h"

class swCatalogFile;

class swsAttributeFile: public swStruct_i {
    swCatalogFile * attributefile_;
public:
    swsAttributeFile (void) {}
    virtual ~swsAttributeFile(void){}
};
#endif
