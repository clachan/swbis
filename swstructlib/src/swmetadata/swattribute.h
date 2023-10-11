/* swattribute.h
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


#ifndef swattribute_19980601jhl_h
#define swattribute_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swmetadata.h"

extern "C" {
#include "swparse.h"
#include "strob.h"
#include "uxfio.h"
#include "swlib.h"
#include "swheader.h"
#include "swheaderline.h"
}


class swAttribute :  public swMetaData {

  public:
	static int debug_writeM;
	swAttribute (void);
	swAttribute (char * keyword, char * value);
	swAttribute (char * parser_line, int at_level);
	virtual ~swAttribute (void);

	int insert (char * parser_line, swMetaData * location);
	int insert (char * keyword, char * value);
     
	int add (char * keyword, char * value);
     
	virtual int write_fd (int fd);
	virtual int write_fd_debug (int fd, char * prefix);
	int find (char * key);
	virtual swMetaData * generate_attribute_list(int at_level, int * return_val);
	static swAttribute * make_newAttribute (int fd, char * keyword, char * value);

private:
     static int determine_type (char type);

};
#endif
