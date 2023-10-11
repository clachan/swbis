/* swpsf.cxx  -- Parse a PSF and handle Extended Definitions.
 */

/*
 * Copyright (C) 2000  James H. Lowe, Jr.  <jhlowe@acm.org>
 * All Rights Reserved.
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


extern "C" {
#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG
#include "swuser_config.h"
}

#include "swpsf.h"

swDefinition * swPSF::makeDefinition (char * parser_line, int * retval) {
	swDefinition * swdef;
	swMetaData * attributes;
	char * keyword = swheaderline_get_keyword(parser_line);

	E_DEBUG2("make definition for parser line [%s]", parser_line);
	
	//
	// Sanity Check.
	//
	if (swMetaData::determine_type(parser_line) != swstructdef::sdf_object_kw) {
		fprintf(stderr,"internal error in swPSF::makeDefinition, 0001.\n"); 
		return NULL;
	}
	
	//
	// Reset the extended definition object when a product or fileset is
	// encounterered because the previous extended defintiions in one object
	// don't influence the next fileset or product object.
	//
	if (
		!::strcmp(keyword, SW_A_product) || 
		!::strcmp(keyword, SW_A_fileset) ||
		!::strcmp(keyword, SW_A_bundle) ||
		0
	) {
		E_DEBUG("swextM->reset()");
		swextM->reset();		
	}

	swdef=swDefinition::swdefinition_factory(parser_line);
	
	if (swdef == static_cast<swDefinition*>(NULL)) {
		fprintf(stderr, "internal error in swPSF::makeDefinition 002\n");
		assert(NULL);
		return NULL;
	}
	//
	// Save the object, when the new objects are created from extended definitions
	// they will be created under this object.
	//
	
	//
	// Only filesets and products contain extended definitions.
	//
	if (
		!::strcmp(keyword, SW_A_product) || 
		!::strcmp(keyword, SW_A_fileset) ||
		!::strcmp(keyword, SW_A_bundle) ||
		0
	) {
		E_DEBUG("");
		swdefM = static_cast<swDefinition*>(swdef);
	}	

	//
	// Set the start index in the swdefinition_list of this fileset
	// so it acts as starting index when swextM judges files to have the
	// same <path> attribute value.
	//
	if (
		!::strcmp(keyword, SW_A_fileset) ||
		0
	) {
		E_DEBUG("");
		swextM->set_fileset_start_index(this);
	}

	attributes = swdef->generate_attribute_list(swdef->get_level()  + 1  , retval); 
	swdef->set_next_node(attributes);
	
	if (!::strcmp(keyword, SW_A_file) || !::strcmp(keyword, SW_A_control_file)) {
		if (swextM->filePermissionsActive()) {
			//
			// Set the default filePermissions.
			//
			E_DEBUG("setting default permissions");
			swextM->addDefaultFilePermissions(static_cast<swDefinition*>(swdef)); // FIXME, static down cast.
		}
	}
	return static_cast<swDefinition*>(swdef);
}

swMetaData * swPSF::generate_attribute_list(int at_level, int *retvalp) {
	int retval=0;
	int u_break=0;
	int nloops=0;
	int u_type;
	char  *p, *parser_line;
	int offset, oldoff;
	swMetaData *node, *prevnode, *head=static_cast<swMetaData*>(NULL);

	//
	// When a extended definition is encountered, process only if it the
	// first line in the while loop (below), otherwise terminate the loop.
	//

	E_DEBUG("");
	set_next_node(NULL);
	p = (char*)get_mem_addr();
	prevnode=this;
	//
	// get_next_line_offset() returns the current offset, then seeks
	// to the next line.
	//
	while ( !u_break && (offset=get_next_line_offset()) >= 0 ) {
		parser_line = p + offset;
		u_type=swMetaData::determine_type(parser_line);
		oldoff=uxfio_lseek(get_mem_fd(), 0, SEEK_CUR);
		E_DEBUG2("%s", parser_line);
		switch(u_type) {	
			case swstructdef::sdf_attribute_kw:
				E_DEBUG("case swstructdef::sdf_attribute_kw:");
				if (swheaderline_get_keyword(parser_line)) {
					//
					// seek back since swAttribute operates on the
					// current file position.
					//
					uxfio_lseek(get_mem_fd(), offset, SEEK_SET);
					node = new swAttribute(parser_line, at_level);
					node->set_p_offset(offset); 
					node->set_ino(offset); 
					node->set_is_explicit(); 

					//
					// seek back (i.e forward) to the old position.
					//
					uxfio_lseek(get_mem_fd(), oldoff, SEEK_SET);
					swFileMapSetCurrentOffset(oldoff);				
					if (!head) head=node; 
					E_DEBUG("attribute");
					if (!get_next_node()) {
						set_next_node(node);
						node->set_next_node(static_cast<swMetaData*>(NULL));
					} else {
						prevnode->set_next_node(node);
						node->set_next_node(static_cast<swMetaData*>(NULL));
					}
					prevnode=node; 
				} else {
					retval--;
					fprintf(stderr,"internal error in swPSF::generate_attribute_list: 005.\n"); 
				}
				break;
			case swstructdef::sdf_extended_kw:
				E_DEBUG("case swstructdef::sdf_extended_kw");
				if(!nloops) {
					E_DEBUG2("running processExtendedDefinition on %s", parser_line);
					if (swextM->processExtendedDefinition(parser_line, this, swdefM)) {
						u_break=1;
						retval--;
					}
					uxfio_lseek(get_mem_fd(), oldoff, SEEK_SET);
				} else {
					//
					// Terminate the attribute processing.
					// This line must be read again, therefore seek back
					// to it.
					//
					E_DEBUG("terminate attribute processing");
					uxfio_lseek(get_mem_fd(), offset, SEEK_SET);
				}
				swFileMapSetCurrentOffset(oldoff);				
				u_break=1;
				break;
			case swstructdef::sdf_object_kw:
			default:
				E_DEBUG("case swstructdef::sdf_object_kw");
				uxfio_lseek(get_mem_fd(), offset, SEEK_SET);
				u_break=1;
				break;
		}
		nloops++;
	}
	if (retvalp != static_cast<int*>(NULL))
		*retvalp = retval;
	return head;
}
