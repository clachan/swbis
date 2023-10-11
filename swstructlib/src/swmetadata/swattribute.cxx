/* swattribute.cxx
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
#include "swattribute.h"
#include <assert.h>

int swAttribute::debug_writeM;

swAttribute::swAttribute (char * parserline, int level): swMetaData() {
     if (level == -1) {
        level=0;
     }
     set_level(level); // Here its OK because the usage is such
			// the parserline already is in memory.
     set_type(swheaderline_get_type(parserline));
     set_status((char)0);
}

swAttribute::swAttribute (char * keyword, char * value): swMetaData(keyword, value) { }

swAttribute::swAttribute (void): swMetaData() {
     set_p_offset(-1); //offset_ = -1;
     set_level_(-1);   // Calling set_level() causes a core dump because
			// the parser_line does not exist in memory.
     set_type(0);   
     set_status(0); 
}

swAttribute::~swAttribute (void) { }

int swAttribute::insert (char * parser_line, swMetaData * location) {
    return -1;
}

int swAttribute::insert (char * keyword, char * value) {
    return -1;
}

int swAttribute::write_fd (int uxfio_fd) {
   int length;
   char * val, *keyword;
   if (swAttribute::debug_writeM) {
	return write_fd_debug(uxfio_fd, "");
   }
   if (get_status() == 0) {
        keyword = get_keyword();
        val = get_value(&length);
	if (strcmp(val, TARU_C_DO_NUMERIC)) {
   		return ::swdef_write_attribute(keyword, val, 
					0 /*get_level()*/, length,
					get_type(), uxfio_fd);
	} else {
		/*
		* FIXME, the better way to do this is
		* set the status.
		*/
		return 0;
	}
   } else {
        return 0;
   }
}

int 
swAttribute::add (char * keyword, char * value) {
   swMetaData *node = new swAttribute(); 
   return swMetaData::add(keyword, value, node, get_level());   
}

int swAttribute::find( char * key ) {
  return -1;
}

swMetaData * swAttribute::generate_attribute_list(int at_level, int * retvalp) {
  int retval = 0;
  char  *p, *p1, *parser_line;
  int offset, oldoff;
  swMetaData *node, *prevnode, *head=NULL;

  set_next_node(NULL);
  p = (char*)get_mem_addr();
  prevnode=this;
  while ((offset=get_next_line_offset()) >= 0 ) {
    parser_line = p + offset;
    if (swMetaData::determine_type(parser_line) != swstructdef::sdf_attribute_kw) {
        uxfio_lseek (get_mem_fd(), offset, SEEK_SET);
        break;
    }
    // fprintf(stderr, "JLX [%s]\n", parser_line);
    if ((p1=swheaderline_get_keyword(parser_line))) {
	
        oldoff=uxfio_lseek (get_mem_fd(), 0, SEEK_CUR);
        uxfio_lseek (get_mem_fd(), offset, SEEK_SET);
	node = new swAttribute(parser_line, at_level);
        node->set_p_offset(offset); 
        node->set_ino(offset); 
	node->set_is_explicit();
	uxfio_lseek (get_mem_fd(), oldoff, SEEK_SET);
	
	if (!head) head=node; 
	if (!get_next_node()) {
           set_next_node(node);
           node->set_next_node(static_cast<swMetaData*>(NULL));
	} else {
           prevnode->set_next_node(node);
           node->set_next_node(static_cast<swMetaData*>(NULL));
	}
        prevnode=node; 
    }
  }
  if (retvalp != static_cast<int*>(NULL))
  	*retvalp = retval;
  return head;
}

swAttribute * swAttribute::make_newAttribute (int fd, char * keyword, char * value) {
	int current_offset = uxfio_lseek(fd, 0, SEEK_CUR);
	swAttribute * swatt = new swAttribute(keyword, value);
	if (uxfio_lseek(fd, current_offset, SEEK_SET) < 0) {
		fprintf(stderr,"uxfio error in make_newAttribute().\n");	
	}
	assert(swatt);
	return swatt;
}

int swAttribute::write_fd_debug (int uxfio_fd, char * prefix) {
	STROB * tmp = strob_open(10);
	char * line;
	char * line2;
	char * line3;
	char * val;
	int ret;
	int value_len;
		
	val = get_value(&value_len);

	ret = ::swlib_writef(uxfio_fd, tmp,
	"%s[%s] keyword=[%s] value=[%s] len=[%d] offset=[%d] inode=[%d] status=[%d]\n",  prefix,
		((line=get_parserline())) ? line : "ERROR: get_parserline returned null.", 
		((line2=get_keyword())) ? line2 : "", 
		((line3=val) ? line3 : ""),
		/* get_length() */ 0,
		get_p_offset(),
		get_ino(),
		get_status()
		);
	strob_close(tmp);
  	return ret;
}
