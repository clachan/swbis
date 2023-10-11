/* swdefdistribution.h
 */

/*
 * Copyright (C) 1998,2006  James H. Lowe, Jr.  <jhlowe@acm.org>
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


#ifndef swdefdistribution_19980601jhl_h
#define swdefdistribution_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swdefinition.h"

class swDefDistribution: public swDefinition {

public:
    swDefDistribution (void);
    swDefDistribution (int level);
    ~swDefDistribution (void);
    static swDefDistribution * make_definition (void) {return new swDefDistribution();}

//
// The purpose of overriding here is to give the distribution.create_time attrbute
// special treatment, namely putting a comment that contains the human readable time
//
int
write_fd (int uxfio_fd) {
   swMetaData * p;
   int eret;
   int ret = 0; 
   char * kw;

   squash_duplicates();
 
   if (swAttribute::debug_writeM) {
   	swDefinition::write_fd_debug(uxfio_fd, "");
   } else {
        char * keyword = get_keyword();
	ret+=::swdef_write_attribute (keyword, NULL, get_level(), 0, (int)(SWPARSE_MD_TYPE_OBJ), uxfio_fd);
   }
   p = get_next_node();
   while (p) {
	kw = p->get_keyword();
	if (::strcmp(kw, SW_A_create_time) == 0) {
		char * oldtzenv;
		char ctbuf[32];
		time_t tm;
		unsigned long ultm;
		STROB * tmp = strob_open(10);
		char * calstring;

		oldtzenv=getenv("TZ");
		putenv("TZ=UTC");
		calstring = p->get_value(NULL);
		sscanf(calstring, "%lu", &ultm);
		tm = (time_t)ultm;
		/* ctime_r(&tm, ctbuf); */
		strncpy(ctbuf, asctime(localtime(&tm)), sizeof(ctbuf) - 1);
	        ctbuf[sizeof(ctbuf)-1] = '\0';
		if (strchr(ctbuf, '\n')) *strchr(ctbuf, '\n') = '\0';
		if (strchr(ctbuf, '\r')) *strchr(ctbuf, '\r') = '\0';
		strob_sprintf(tmp, 0, SW_A_create_time " %s # %s UTC\n", calstring, ctbuf);
		if (oldtzenv) putenv(oldtzenv);
        	eret = uxfio_write(uxfio_fd, strob_str(tmp), strob_strlen(tmp));
		strob_close(tmp);
	} else {
        	eret=p->write_fd (uxfio_fd);
	}
        if (eret >= 0) {
		ret += eret; 
	} else { 
		return -1; 
	}
	p=p->get_next_node();
   }        
   return ret;
}

};

#endif

