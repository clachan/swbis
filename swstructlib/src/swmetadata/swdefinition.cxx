/* swdefinition.cxx
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <string.h>
#include "swdefinition.h"
#include "swdeffile.h"
#include "swdefcontrolfile.h"
#include "swdeffileset.h"
#include "swdefproduct.h"
#include "swdefsubproduct.h"
#include "swdefdistribution.h"
#include "swdefmedia.h"
#include "swdefbundle.h"
#include "swdefinstalled_software.h"
#include "swdefcategory.h"
#include "swdefhost.h"
#include "swdefvendor.h"
extern "C" {
#include "swparse.h"
#include "swheaderline.h"
}

swDefinition * swDefinition::swdefinition_factory (char * parserline) {

    int i;
    swDefinition * swdef = NULL;
    char * object_keyword = swheaderline_get_keyword(parserline); 
	     
	if ( !::strcmp (SW_A_file, object_keyword )) {
		E_DEBUG2("making file \n\n[%s]\n\n", parserline);
		swdef = swDefFile::make_definition(); 
	} else if ( !::strcmp (SW_A_control_file, object_keyword )) {
		swdef = swDefControlFile::make_definition();
	} else if ( !::strcmp (SW_A_category, object_keyword )) {
		swdef = swDefCategory::make_definition();
	} else if ( !::strcmp (SW_A_fileset, object_keyword )) {
		swdef = swDefFileset::make_definition();
	} else if ( !::strcmp (SW_A_product, object_keyword )) {
		swdef = swDefProduct::make_definition();
	} else if ( !::strcmp (SW_A_subproduct, object_keyword )) {
		swdef = swDefsubProduct::make_definition();
	} else if ( !::strcmp (SW_A_distribution, object_keyword )) {
		swdef = swDefDistribution::make_definition();
	} else if ( !::strcmp (SW_A_media, object_keyword )) {
		swdef = swDefMedia::make_definition();
	} else if ( !::strcmp (SW_A_bundle, object_keyword )) {
		swdef = swDefBundle::make_definition();
	} else if ( !::strcmp (SW_A_host, object_keyword )) {
		swdef = swDefHost::make_definition();
	} else if ( !::strcmp (SW_A_installed_software, object_keyword )) {
		swdef = swDefInstalled_Software::make_definition();
	} else if ( !::strcmp (SW_A_vendor, object_keyword )) {
		swdef = swDefVendor::make_definition();
	} else {
		return static_cast<swDefinition *>(NULL);
	}

	assert(swdef);
	swdef->set_p_offset((i=parserline - static_cast<char*>(swdef->get_mem_addr()),i));
	swdef->set_level(::swheaderline_get_level(parserline));
	swdef->set_ino(i);
	return swdef;
}

swDefinition::swDefinition(char * name, int level): swAttribute() {
	::swparse_write_attribute_obj(get_mem_fd(), name, level, SWPARSE_FORM_MKUP_LEN);
	set_type(swstructdef::sdf_object_kw);
  	set_next_node(NULL);
  	nextM = static_cast<swDefinition*>(NULL);
  	prevM = static_cast<swDefinition*>(NULL);
	no_statM = 0;
	storage_statusM = 1;
	//sbufM = NULL;
}

swDefinition::swDefinition (): swAttribute() {
	set_type(swstructdef::sdf_object_kw);
	nextM = static_cast<swDefinition*>(NULL);
  	prevM = static_cast<swDefinition*>(NULL);
	no_statM = 0;
	storage_statusM = 1;
	//sbufM = NULL;
}

int swDefinition::get_type(void) {
	return swstructdef::sdf_object_kw;
}

void swDefinition::set_type (int i) {
   return; 
   set_type(swstructdef::sdf_unknown);
   //obj_type_=swstructdef::sdf_unknown;
}

void swDefinition::setup_contained_by(void)
{
   swMetaData *p = get_next_node();
   while (p) {
	// fprintf(stderr, "Setting parserline=[%s]: addr=%p\n", p->get_parserline(), this);
   	p->set_contained_by(this);
	p=p->get_next_node();
   }       
}

void swDefinition::set_level(int level){
   swMetaData *p = get_next_node();
   (static_cast<swMetaData*>(this))->swMetaData::set_level(level);
   while (p) {
        p->set_level(level+1);
	p=p->get_next_node();
   }       
}

swDefinition::~swDefinition(void) {
	//if (sbufM) strob_close(sbufM);
}

swAttribute * swDefinition::add(char * keyword, char * value)
{
	swAttribute * sat=new swAttribute(keyword, value);
	sat->set_level(get_level()+1);
        list_add(sat); 
	return sat;
}

swAttribute * swDefinition::add(char * keyword, char * value, int status)
{
	swAttribute * sat = add(keyword, value);
	sat->set_status(status);
	return sat;
}

void swDefinition::list_insert(swMetaData * swmd, swMetaData * location)
{
	insert_before_location(swmd, location);
}

void swDefinition::list_add(swMetaData * swmd)
{
	insert_before_location(swmd, static_cast<swMetaData *>(NULL));
}

int swDefinition::list_add_if_new(swMetaData * swmd)
{
	if (find(swmd->get_keyword()) == static_cast<char*>(NULL)) {
		list_add(swmd);
		return 1;
	}
	return 0;
}

void swDefinition::list_replace_if_not_explicitly_set(swMetaData * swmd)
{
	swMetaData *p;
	char * keyword;

	keyword = swmd->get_keyword();
	p = findAttribute(keyword);
	if (!p) {
		list_add(swmd);
	} else {
		if (p->get_is_explicit() == 0) {
			list_replace(swmd);
		} else {
			/* do nothing */
			;
		}
	}
}

void swDefinition::list_replace(swMetaData * swmd)
{
	char * keyword = swmd->get_keyword();
	if (find(keyword) != static_cast<char*>(NULL)) {
		// Delete all attributes of this keyword.
		while(deleteAttribute(keyword) != static_cast<char *>(NULL)) { ; }
	}
	list_add(swmd);
}

int swDefinition::write_fd (int uxfio_fd) {
   //char objectkeyword[80]; 
   swMetaData * p;
   int eret, ret = 0; 
   
   if (swAttribute::debug_writeM) {
   	swDefinition::write_fd_debug(uxfio_fd, "");
   } else {
        char * keyword = get_keyword();
	//swstructdef::return_entry_keyword (objectkeyword, get_type());
	ret+=::swdef_write_attribute (keyword, NULL, 0 /*get_level()*/, 0, (int)(SWPARSE_MD_TYPE_OBJ), uxfio_fd);
   }
   p = get_next_node();
   while (p) {
        eret=p->write_fd (uxfio_fd);
        if (eret >= 0) {
		ret += eret; 
	} else { 
		return -1; 
	}
	p=p->get_next_node();
   }        
   return ret;
}

int swDefinition::write_fd_debug(int uxfio_fd, char * prefix) {
	swMetaData * p;
	int eret, ret = 0; 
	int type = get_type();

	if (type == swstructdef::sdf_object_kw) {
		// Write the object 
		ret = swAttribute::write_fd_debug(uxfio_fd, prefix);

		// Now write the attributes.
   		p = get_next_node();
		while (p) {
			eret=p->write_fd_debug(uxfio_fd, prefix);
			if (eret > 0) ret += eret; else return -1; 
			p=p->get_next_node();
		}        
	} else {
		fprintf(stderr,"%s: unexpected result in swDefinition::write_fd_debug type = %d %s.\n",
			swlib_utilname_get(), (int)get_type(), get_parserline());
		return -1;
	}
	return ret;
}

char * swDefinition::deleteAttribute(char * keyword)
{ 
	swMetaData *oldp = this;
	swMetaData *pp = this;
	swMetaData *next;
	swMetaData *p = pp->get_next_node();
	char * value = static_cast<char*>(NULL);

	while (p){
		if (
			::strcmp(keyword, p->get_keyword()) == 0
		 ) {
			value = p->get_value((int*)NULL);
			p->set_contained_by(NULL);
			next = p->get_next_node();
			oldp->set_next_node(next);
			oldp = next;
			p = next;
		} else {
			oldp = p;
			p = p->get_next_node();
		}
	}
	return value;
}

void swDefinition::vremove(char * keyword)
{
	swMetaData *p;
	int m = get_find_mode_logical();

	set_find_mode_logical(1);
	p = findAttribute(keyword);
	while (p) {
		p->vremove();	
		p = findAttribute(keyword);
	}
	set_find_mode_logical(m);
}

char * 
swDefinition::findPhysical(char * keyword)
{ 
	char * s;
	int m = get_find_mode_logical();
	set_find_mode_logical(0);
	s = find(keyword);
	set_find_mode_logical(m);
	return s;
}

char * swDefinition::find(char * keyword)
{ 
	swMetaData *p=get_next_node();
	char * value = static_cast<char*>(NULL);
	while (p){
		if (!::strcmp(keyword, p->get_keyword())){
			value = p->get_value((int*)NULL);
		}
		p=p->get_next_node();
	}
	return value;
}

char * swDefinition::getPathAttribute(void)
{
	return find(SW_A_path);
}

char * swDefinition::getTagAttribute(void)
{
	return find(SW_A_tag);
}

swMetaData * swDefinition::findAttribute(char * keyword)
{ 
	swMetaData *p=get_next_node();
	swMetaData * fp = NULL;
	int m = get_find_mode_logical();
	while (p){
		if (!::strcmp(keyword, p->get_keyword()) && ((m == 0) || (m && p->get_status() == 0))){
			fp = p;
		}
		p=p->get_next_node();
	}
	return fp;
}

///
// FIXME Paramaterize to consolidate similar code.
//
swMetaData * swDefinition::find_by_ino(int inode)
{
	int c = 0;
	swMetaData *p=this;
	//int start = p->get_ino();
	if (p->get_ino() == inode) {
		//fprintf(stderr, "find_by_ino[%d]: found current.\n", inode);
		return p;
	}
	while ((p=p->get_next_node()) != NULL){
		c++;
		if (p->get_ino() == inode) {
			//fprintf(stderr, "find_by_ino[%d]: starting at [%d], found in [%d] tries.\n", inode, start, c);
			return p;
		}
	}
	//fprintf(stderr, "find_by_ino[%d]: starting from [%d], Not found.\n", inode, c);
	return NULL;
}

swMetaData * swDefinition::find_by_p_offset(int offset)
{
	swMetaData *p=this;
	if (p->get_p_offset() == offset)
		return p;
	while ((p=p->get_next_node()) != NULL){
		if (p->get_p_offset() == offset) 
			return p;
	}
	return NULL;
}

swMetaData * swDefinition::find_by_parserline(char * parserline)
{
	swMetaData *p=this;
	if (p->get_parserline() == parserline)
		return p;
	while ((p=p->get_next_node()) != NULL){
		if (p->get_parserline() == parserline) 
			return p;
	}
	return NULL;
}


swMetaData * swDefinition::get_attribute_by_index(int n)
{
	int i=0;
	swMetaData *p=this;
	if (!n) return this;
	while (((p=p->get_next_node()) != NULL) && i++ <  n) { ; }
	return p;
}

char * swDefinition::get_parserline_by_index(int n)
{
	return get_attribute_by_index(n)->get_parserline();
}

swDefinition * swDefinition::make_newDefinition (swDefinition * parent_swdef, char * object_keyword){
	int level = parent_swdef->get_level() + 1;
	int mem_fd = parent_swdef->get_mem_fd();
	int current_offset = uxfio_lseek(mem_fd, 0, SEEK_CUR);
	int offset;
	char * new_parserline;
	swDefinition * swdef;
	
	//
	// Write a the parser line into get_mem_fd()
	//
	offset = uxfio_lseek(mem_fd, 0, SEEK_END);
	swparse_write_attribute_obj(mem_fd, object_keyword, level, SWPARSE_FORM_MKUP_LEN);

	//
	// Set the file back the way it was.
	//
	uxfio_lseek(mem_fd, current_offset, SEEK_SET);

	//
	// Now make the swDefinition.
	// Remember.. Always get the get_mem_addr() after adding to it because
	// the base address may change.
	//
	new_parserline = static_cast<char *>(parent_swdef->get_mem_addr()) + offset;
	swdef = swDefinition::swdefinition_factory (new_parserline);
	assert(swdef);	
	return swdef;
}

void
swDefinition::merge(swDefinition * source, int do_all, int do_replace)
{
	//
	// Merge <this> with attributes of <source>.
	// Keep all attributes in <this> that are not
	// in <source>.
	//
	
	int status;
	char * keyword;
	swMetaData * p;
	//
	// Loop through the attributes of <source>
	//
	SWDEFINITION_DEBUG("Entering");
	p = source->get_next_node();
	while (p) {
		keyword = p->get_keyword();
		if (p->get_is_explicit() || do_all) {
			if (*keyword == '_') {
				//
				// keywords that begin with '_' are internal.
				// Setting the status to non-zero causes them not to
				// be printed.
				//
				status = 1;
			} else {
				status = 0;
			}
			if (find(keyword)) {
				//
				// Replace it.
				//
				// fprintf(stderr, "JL replacing %s with value %s\n",
				// 	keyword, p->get_value((int*)(NULL)));
				if (do_replace) {
					vremove(keyword);
					add(keyword, p->get_value((int*)(NULL)), status);
					if (strcmp(keyword, SW_A_source) == 0) {
						vremove(keyword);
					}
				}
			} else {
				//
				// Add it.
				//
				//fprintf(stderr, "JL adding %s with value %s\n",
				//	keyword, p->get_value((int*)(NULL)));
				SWDEFINITION_DEBUG3("adding : attribute=[%s]   new value is [%s]", keyword, p->get_value((int*)(NULL)));
				add(keyword, p->get_value((int*)(NULL)), status);
			}
		} else {
			;
			SWDEFINITION_DEBUG2("is_explicit is unset, doing nothing for keyword [%s]", keyword);
		}
		p=p->get_next_node();
	}
	SWDEFINITION_DEBUG("Leaving");
}

void swDefinition::apply_file_stat_specialization(char fileArray[][file_permsLength], int fileArrayWasSet[]) { }

int swDefinition::apply_cksum_policy_specialization(int cksumflags) { return cksumflags; }

void swDefinition::set_tag_attribute(void) { }

void 
swDefinition::set_path_attribute(char * source)
{
	char * path = find(SW_A_path);
	if (path) return;

	//
	// Section 5.2.11 Lines 677-678
	//
	add(SW_A_path, source);
}

void 
swDefinition::squash_duplicates(void)
{
	while(i_squash_duplicates()) { ; }
}

/* Private */

int 
swDefinition::i_squash_duplicates(void)
{
	int retval;
	int value_type;
	int ino1;
	int ino2;
	char * keyword1;
	char * keyword2;
	char * object_keyword;
	swMetaData * p1;
	swMetaData * p2;
	swstructdef defs;

	retval = 0;
	object_keyword = get_keyword();

	p1 = this;
	while ((p1 = p1->get_next_node()) != NULL) {
		ino1 = p1->get_ino();
		keyword1 = p1->get_keyword();
		//
		// Check if attribute is single_value type
		//
		if (p1->get_status()) {
			//
			// skip attributes that are
			// already (virtually) removed 
			//
			continue;
		}
		defs.return_entry_index(object_keyword, keyword1);
		value_type =  defs.get_value_type();
		if (value_type < 0) {
			//
			// If the attribute is unknown assume it is
			// a single value type.
			//
			value_type = sdf_single_value;
		}
		if (value_type != sdf_single_value) {
			continue;
		}
		p2 = this;
		while ((p2 = p2->get_next_node()) != NULL) {
			keyword2 = p2->get_keyword();
			ino2 = p2->get_ino();
			if (p2->get_status()) {
				continue;
			}
			if (::strcmp(keyword1, keyword2) == 0 && ino1 != ino2) {
				p1->vremove();
				retval = 1;
				break;
			}
		}
	}
	return retval;
}


void swDefinition::insert_before_location(swMetaData * swmd, swMetaData * location)
{
	// If location is NULL then insert at end.
	//
	swMetaData * pp = this;
	swMetaData * p = get_next_node();

	SWDEFINITION_DEBUG("");
	while (p != location){
		pp=p;
		p=p->get_next_node();
	}
	swmd->set_next_node(pp->get_next_node());
	pp->set_next_node(swmd);
	//fprintf(stderr, "insert_before: %p %s\n", this, swmd->get_keyword());
	swmd->set_contained_by(this);
	SWDEFINITION_DEBUG("");
}
