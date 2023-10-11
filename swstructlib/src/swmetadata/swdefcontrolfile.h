/* swdefcontrolfile.h
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


#ifndef swdefcontrolfile_19980601jhl_h
#define swdefcontrolfile_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "swdefinition.h"

class swDefControlFile: public swDefinition {

public:
    swDefControlFile (void);
    swDefControlFile (int level);
    ~swDefControlFile (void);
    static swDefControlFile * make_definition (void) {return new swDefControlFile();}
    char * getPathAttribute(void);
    char * getTagAttribute(void);
    virtual void set_tag_attribute(void);
    virtual void set_path_attribute(char * source);
    void apply_file_stat_specialization(char fileArray[][file_permsLength], int fileArrayWasSet[]);
//    int get_type(void);
};

#endif


