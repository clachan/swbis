/* swinstalled.h
 */

/*
 * Copyright (C) 2005 James H. Lowe, Jr.  <jhlowe@acm.org>
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



#ifndef swinstalled_2005_h
#define swinstalled_2005_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swdefinitionfile.h"

class swINSTALLED: public swDefinitionFile 
{

public:
	swINSTALLED(void);
	swINSTALLED(char * path);
	~swINSTALLED(void);
	int get_type (void);
	char * getObjectName(void) { return SW_A_INSTALLED; }
private:

};

#endif
