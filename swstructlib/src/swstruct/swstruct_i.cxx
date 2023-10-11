/* swstruct_i.cxx
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
#include "swstruct_i.h"
#include "swsfile.h"
#include "swscontrolfile.h"
#include "swsbundle.h"
#include "swsfileset.h"
#include "swssubproduct.h"
#include "swscategory.h"
#include "swsvendor.h"
#include "swsmedia.h"
#include "swsdistribution.h"
#include "swscategory.h"
#include "swsproduct.h"
#include "swshost.h"
#include "swsvendor.h"
#include "swstructiter.h"
#include "switer.h"
#include "swexstruct.h"

static char empty_string[] = "";

swStruct_i::swStruct_i(void) {
   swdefM=NULL; 
}

swStruct_i::swStruct_i(char * object_name, int level){
	swdefM=new swDefinition(object_name, level);
}

swStruct_i::~swStruct_i (void){ }

int swStruct_i::get_type(void){
  return swdefM->get_type();
}

int swStruct_i::get_level(void){
  return swdefM->get_level();
}

void swStruct_i::set_level(int level){
  return swdefM->set_level(level);
}

swStruct * swStruct_i::get_swsobjectnode(int index) {
  return NULL;
}

void swStruct_i::set_swsobjectnode(int index, swStruct *sws) {
  return;
}

void swStruct_i::set_swdefinition(swDefinition * swdef) {
  swdefM=swdef;
}

swDefinition * swStruct_i::get_swdefinition(void){
	return swdefM; 
}

int swStruct_i::write_node(int uxfio_fd) {
   return swdefM->write_fd(uxfio_fd);
}

int swStruct_i::add_attribute (char * keyword, char * value){ 
	swdefM->add(keyword, value);
}

swMetaData * swStruct_i::get_attribute(int inode){
  return swdefM->find_by_ino(inode);
}

char * swStruct_i::get_attribute (swSelection * software_spec, char * keyword){
  return NULL;
}

int swStruct_i::delete_attribute (swSelection * software_spec, char * keyword){
  return  -1;
}

void swStruct_i::set_swsobjectarray(swPtrList<swStruct> * swsarray) {
  return;
}

swPtrList<swStruct> * swStruct_i::get_swsobjectarray(void) {
  return NULL;
}

int swStruct_i::compare_tag (char * tag) {
  return -1;
}

swStruct * swStruct_i::swstructure_factory(char * object_keyword) {
             if ( !::strcmp (SW_A_file, object_keyword )) { return swsFile::make_swstructure(); }
        else if ( !::strcmp (SW_A_category, object_keyword )) { return swsCategory::make_swstructure(); }
        else if ( !::strcmp (SW_A_control_file, object_keyword )) { return swsControlFile::make_swstructure(); }
        else if ( !::strcmp (SW_A_fileset, object_keyword )) { return swsFileset::make_swstructure(); }
        else if ( !::strcmp (SW_A_product, object_keyword )) { return swsProduct::make_swstructure(); }
        else if ( !::strcmp (SW_A_subproduct, object_keyword )) { return swssubProduct::make_swstructure(); }
        else if ( !::strcmp (SW_A_distribution, object_keyword )) { return swsDistribution::make_swstructure(); }
        else if ( !::strcmp (SW_A_media, object_keyword )) { return swsMedia::make_swstructure(); }
        else if ( !::strcmp (SW_A_bundle, object_keyword )) { return swsBundle::make_swstructure(); }
        else if ( !::strcmp (SW_A_host, object_keyword )) { return swsHost::make_swstructure(); }
        else if ( !::strcmp (SW_A_vendor, object_keyword )) { return swsVendor::make_swstructure(); }
        //else if ( !::strcmp ("installed_software",object_keyword )) { return swsInstalled_Software::make_swstructure(); }
        else { return NULL; }
}

int swStruct_i::doProcess(swExStruct * swex){
	swDefinition * swdef = get_swdefinition();
   	swDefinitionFile * global_index = swex->getGlobalIndex();
   	global_index->swdeffile_list_add(swdef);
    	return 0;
}

swExStruct * swStruct_i::getSwExStructContext(swExStruct * swexdist){
	return swexdist->getLastExStruct();
}

char * swStruct_i::determineTag(void)
{
	swDefinition * swdef = get_swdefinition();
	char * tag = static_cast<char*>(NULL);

	if (!swdef) 
		return static_cast<char*>(NULL);
	tag = swdef->find(SW_A_tag);
	if (tag) {
		return tag;
	}
	return static_cast<char*>(NULL);
}

char * swStruct_i::determineFilesControlDirectory(void)
{
	return swStruct_i::determineControlDirectory();
}

char * swStruct_i::determineControlDirectory(void)
{
	return empty_string;
}

int swStruct_i::iwrite(int uxfio_fd) 
{
    int len=0;
    swStruct * sw=this;
    swIter switer(sw);
    swStructIter * swit=switer.peek();
    
    len+=sw->write_node(uxfio_fd);

    while (swit) {
	sw=swit->get_next_object();
	if (!sw){
		swit=switer.pop(NULL);
		continue;
        }
	swit=new swStructIter(sw);
	switer.push(swit);
	len+=sw->write_node(uxfio_fd);
    }
    return len;
}
