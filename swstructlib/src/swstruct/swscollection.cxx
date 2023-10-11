/* swscollection.cxx
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

#include "swuser_config.h"
#include "swscollection.h"
#include "swstruct.h"
#include "swsnode.h"
#include "swstruct_i.h"
#include "swstructiter.h"
extern "C" {
#include "swheaderline.h"
}

swsCollection::swsCollection (swDefinitionFile * swdeffile) {
  	swdeffileM = swdeffile; 
	collectionM = static_cast<swStruct*>(NULL);
	swFileMapFdM = -1;
}

swsCollection::~swsCollection (void) {
}

swDefinitionFile * swsCollection::get_swdeffile(void) {
	return swdeffileM;
}

swStruct * swsCollection::get_swstruct (void) {
	return collectionM;
}

int swsCollection::generateStructures(void) {
	return generate_from_swdefinitionfile(swdeffileM);
}

int swsCollection::generateStructuresFromParser(void) {
	return generateStructuresFromParser(-1);
}

int swsCollection::generateStructuresFromParser(int swfilemapfd) {
	int ret;
	ret = swsCollection::generate_from_parser(swdeffileM, swfilemapfd);
	return ret;
}

//
// Parse the output of swdeffile again.
// This time its guaranteed that there won't be any 
// extended definitions because they are all expanded.
// This can only be called after generateDefinitions.
int 
swsCollection::rewriteExpandedPsf(void) 
{
	int tmpfd;
	int swFileMapFd;
  	int memfd = swdeffileM->get_mem_fd(); 
	int curoffset;
	int offset;
	int len;

	curoffset = uxfio_lseek(memfd, (off_t)0, SEEK_CUR);
	offset = uxfio_lseek(memfd, (off_t)0, SEEK_END);

	tmpfd = uxfio_open("/dev/null", O_RDWR, 0);
	uxfio_fcntl(tmpfd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	uxfio_fcntl(tmpfd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

	if (swdeffileM->write_fd(tmpfd) < 0) {
		uxfio_close(tmpfd);
		return -2;
	}
	uxfio_lseek(tmpfd, (off_t)0, SEEK_SET);

	//
	// Now tmpfd contains a new parsing of the expanded PSF file.
	//

	//
	// Set up the parser's input and output files.
	//
	swdeffileM->getParserObject()->set_inputfd(tmpfd);
	swdeffileM->getParserObject()->set_outputfd(memfd);
	len=swdeffileM->run_parser(0, SWPARSE_FORM_MKUP_LEN, &swFileMapFd);

	//
	// Done with this temp file.
	//
	uxfio_close(tmpfd);

	//
	// Check for error.
	//
	if (len <= 0) return -2;

	
	//
	// Restore the global memory file to the way it was.
	//
	uxfio_lseek(memfd, (off_t)offset, SEEK_SET);
	return swFileMapFd;
}



//
// ------------------ Private ----------------------------
//


int 
swsCollection::generate_from_swdefinitionfile(swDefinitionFile * swdeffile) {
	int retval = 0;
	swStruct * stack[20]; // tree depth limit is 20
	collectionM = swsCollection::generate_from_swdefinitionfile_i(swdeffile, stack, &retval);
	return retval;
}

int 
swsCollection::generate_from_parser(swDefinitionFile * swdeffile, int swfilemapfd)
{
	int retval = 0;
	swStruct * stack[20]; // tree depth limit is 20
	collectionM = swsCollection::generate_from_deffile_recurse(swdeffile, stack, &retval, swfilemapfd);
	return retval;
}

swStruct * 
swsCollection::generate_from_deffile_recurse(swDefinitionFile * swdeffile, swStruct ** stack, int * retval, int swfilemapfd)
{
  int newlevel;
  int type;
  int rel_start_level;
  int offset;
  char *p;
  char *parser_line;
  swDefinition * swdef;
  swStruct *current_root = NULL;
  swStruct *parent;

  swdeffile->swFileMapPeekByIndex(swfilemapfd, &offset, NULL, NULL);

  p = (char*)(swdeffile->get_mem_addr()); 

  parser_line = p + offset; 
  type=swheaderline_get_type(parser_line);
  if (swMetaData::determine_type(type) != swstructdef::sdf_filereference_kw) {
         fprintf (stderr,"error in swsCollection::generate: missing filereference line in parser output. [%s]\n", parser_line);
	 (*retval) --;
         return NULL;
  } else {
         //fprintf (stderr,"JL generate_from_deffile_recurse got it. [%s]\n", parser_line);
  }

  rel_start_level=swheaderline_get_level(parser_line);
 
  offset=swdeffile->get_next_line_offset(swfilemapfd);
  while (offset >= 0) {
	parser_line = p + offset; 
	newlevel=swheaderline_get_level(parser_line);
	type=swheaderline_get_type(parser_line);
 
 	if(newlevel==rel_start_level) {
		parent=NULL;
		stack[0]=new swsNode(); 
		current_root = stack[0];
	} else {
		parent = *(stack + newlevel - 2);
	}

	// Should always be a object keyword here. 
	if (swMetaData::determine_type(type) == swstructdef::sdf_object_kw) {
		if (newlevel > 1) {
			current_root = swStruct_i::swstructure_factory(swheaderline_get_keyword(parser_line));
		}
		*(stack+newlevel-1) = current_root; 
		swdef = swdeffile->makeDefinition(parser_line, retval);
		current_root->set_swdefinition(swdef); 
		if (newlevel > 1) {
			parent->add_swstruct(current_root);
		}
	} else if (swMetaData::determine_type(type) == swstructdef::sdf_filereference_kw) {
		// Normal
		//fprintf (stderr,"error in swsCollection:generate filereference found.\n");
	} else if (swMetaData::determine_type(type) == swstructdef::sdf_extended_kw) {
		(*retval) --;
		fprintf (stderr,"extended definition not supported. %s\n", parser_line);
	} else {
		(*retval) --;
		fprintf (stderr,"error in swsCollection:generate\n");
		return NULL;
	}
  	offset=swdeffile->get_next_line_offset(swfilemapfd);
  }
  return stack[0];
}


swStruct * 
swsCollection::generate_from_swdefinitionfile_i(swDefinitionFile * swdeffile, swStruct ** stack, int * retval)
{
  int newlevel;
  int type;
  int index=0;
  swDefinition * swdef = swdeffile->swdeffile_get_pointer_from_index(index++);
  int rel_start_level = swdef->get_level();
  swStruct * current_root=NULL;
  swStruct * parent;
  
  while (swdef != static_cast<swDefinition*>(NULL) && *retval == 0) {
	newlevel=swdef->get_level();
	type=swdef->get_type();

	if(newlevel==rel_start_level) {
		parent=NULL;
		stack[0]=new swsNode(); 
		current_root = stack[0];
	} else {
		parent = *(stack + newlevel - 2);
	}

	// Should always be a object keyword here. 
	if (type == swstructdef::sdf_object_kw) {
		if (newlevel > 1) {
			current_root=swStruct_i::swstructure_factory(swdef->get_keyword());
		}
		*(stack+newlevel-1) = current_root; 
		current_root->set_swdefinition(swdef);
		if (newlevel > 1) {
			parent->add_swstruct(current_root);
		}
	} else if (type == swstructdef::sdf_filereference_kw) {
		*retval = -1;
		fprintf (stderr,"error in swsCollection:generate_from_swdefinitionfile.\n");
	} else if (type == swstructdef::sdf_extended_kw) {
		*retval = -2;
		fprintf (stderr,"extended definition not supported.\n");
	} else {
		*retval = -3;
		fprintf (stderr,"internal error in swsCollection:generate\n");
		return NULL;
	}
	swdef = swdeffile->swdeffile_get_pointer_from_index(index++);
   }
   return stack[0];
}

int swsCollection::write_debug(int uxfio_fd) {
    return (static_cast<swStruct_i*>(collectionM))->swStruct_i::write_debug(uxfio_fd);
}

int swsCollection::iwrite(int uxfio_fd) {
    return collectionM->iwrite(uxfio_fd);
}
