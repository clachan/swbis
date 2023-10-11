/* swselection.h
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


#ifndef sw_selection_19981001_h
#define sw_selection_19981001_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <typeinfo>

extern "C" {
#include "strob.h"
}

class swSelection
{
    STROB * product_control_dir_;
    STROB * fileset_control_dir_;
    STROB * pfiles_;
    STROB * dfiles_;
    STROB * swpackage_pathname_;
    STROB * pathname_;
    STROB * prepath_;
    STROB * buffer_;
    STROB * basename_;
    static char * p_prepath_;
    int is_catalog_;

  public:

     swSelection(char * pathname);
     swSelection(void);
     virtual ~swSelection(void);

};
#endif
