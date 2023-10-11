/* swsnode.cxx
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
#include "swsnode.h"
#include "swptrlist.h"

swStruct * swsNode::distributionM;

swStruct * swsNode::swsnodeGetDistribution(void){
	return distributionM;
}

void swsNode::swsnodeSetDistribution(swStruct * sws){
	distributionM = sws;
}

swsNode::swsNode(void) {
   swsobjectarray_=new swPtrList<swStruct>();   
}

swsNode::~swsNode(void){ 
}

swPtrList<swStruct> * swsNode::get_swsobjectarray(void) {
   return swsobjectarray_;
}

void swsNode::set_swsobjectarray(swPtrList<swStruct> * swsarray) {
   swsobjectarray_=swsarray;
}


int swsNode::insert_swstruct (swStruct * before, swStruct * node) {
   return swsobjectarray_->list_insert (before, node);
}

int swsNode::add_swstruct (swStruct * node) {
   swsobjectarray_->list_add (node);
   node->set_level(get_level()+1);
   return get_level()+1;
}

int swsNode::del_swstruct (swStruct * node) {
   return swsobjectarray_->list_del (node);
}

int swsNode::get_index_from_pointer(swStruct * node) {
   return swsobjectarray_->get_index_from_pointer(node);
}

swStruct * swsNode::get_pointer_from_index(int index) {
   return swsobjectarray_->get_pointer_from_index(index);
}

char * swsNode::determineControlDirectory(void) {
	swDefinition * swdef = get_swdefinition();
	char * control_dir = static_cast<char*>(NULL);

	if (!swdef) return static_cast<char*>(NULL);
	control_dir = swdef->find("control_directory");
	if (control_dir) {
		return control_dir;
	}
	control_dir = swdef->find(SW_A_tag);
	if (!control_dir)
		control_dir = swStruct_i::determineControlDirectory();
	return control_dir;
}

