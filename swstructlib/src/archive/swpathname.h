/*  swpathname.h
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


#ifndef swpathname_19980720_h
#define swpathname_19980720_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swstructdef.h"

extern "C" {
#include "swpath.h"
}

class swPathName
{
  private:
     SWPATH * swpathM;

  public:

	SWPATH * swpath_impl_p(void){
		return swpathM;
	}
	
	char * swp_buffer(void){
		return ::swpath_get_pkgpathname(swpath_impl_p());
	}

	void swpath_ctl_set_swpath(SWPATH * swpath){
		swpathM=swpath;
	}
     
	SWPATH * swpath_ctl_get_swpath(void){
		return swpath_impl_p();
	}   

	swPathName(char * path) {
		init_(path);
	}
	
	swPathName(void) {
		init_("");
	}
	
	virtual ~swPathName (void){ 
		swpath_close(swpathM);
		swpathM=NULL;
	}


	int swp_num_of_components(char * str) {
		return ::swpath_num_of_components(swpath_impl_p(), str);
	}
	
	int swp_resolve_prepath(char * name) {
		return ::swpath_resolve_prepath(swpath_impl_p(), name);
	}
	
	void  swp_set_product_control_dir (char * s) {
	     	::swpath_set_product_control_dir(swpath_impl_p(), s); 
	}
	
	void swp_set_fileset_control_dir (char * s) {
		::swpath_set_fileset_control_dir(swpath_impl_p(), s);
	}
	
	void swp_set_pfiles (char * s) {
		::swpath_set_pfiles(swpath_impl_p(), s);
	}
	
	void swp_set_dfiles (char * s) {
		::swpath_set_dfiles(swpath_impl_p(), s);
	}
	
	void swp_set_pathname (char * s) {
		::swpath_set_pathname(swpath_impl_p(),s);      
	}
	
	void swp_set_pkgpathname(char * s) {
		::swpath_set_pkgpathname(swpath_impl_p(), s);
	}
	void swp_set_filename (void) {
		swp_set_filename(NULL);
	}
	
	void swp_set_filename (char * s) {
	    ::swpath_set_filename(swpath_impl_p(), s);
	}
	
	void swp_set_prepath(char * s) {
		::swpath_set_prepath(swpath_impl_p(),s);
	}
	
	char * swp_get_product_control_dir (void) {
		return ::swpath_get_product_control_dir(swpath_impl_p());	
	}
	char * swp_get_fileset_control_dir (void) {
		return ::swpath_get_fileset_control_dir(swpath_impl_p());
	}
	
	char * swp_get_pfiles (void) {
		return ::swpath_get_pfiles(swpath_impl_p());
	}
	
	char * swp_get_dfiles (void) {
		return ::swpath_get_dfiles(swpath_impl_p());
	}
	
	char * swp_get_prepath(void){
		return ::swpath_get_prepath(swpath_impl_p());
	}
	
	char * swp_get_pathname (void) {
		return ::swpath_get_pathname(swpath_impl_p());
	}
	
	char * swp_get_pkgpathname (void) {
		return ::swpath_get_pkgpathname(swpath_impl_p());
	}
	
	char * swp_get_basename (void) {
		return ::swpath_get_basename(swpath_impl_p());
	}
	
	virtual char *  swp_form_path (STROB * buf) { // Virtual
		return ::swpath_form_path(swpath_impl_p(), buf);
	}
	
	char *  swp_form_catalog_path (STROB * buf) {
		return ::swpath_form_catalog_path(swpath_impl_p(), buf);
	}
	
	char *  swp_form_storage_path(STROB * buf){
		return ::swpath_form_storage_path(swpath_impl_p(), buf);
	}

	void swp_set_is_catalog(int i) {
		return ::swpath_set_is_catalog(swpath_impl_p(), i);
	}
	
	int swp_get_is_catalog(void) {
		return ::swpath_get_is_catalog(swpath_impl_p());
	}

	int swp_is_catalog(char * name){
	   swp_parse_path (name);
	   return swp_get_is_catalog();
	}

	int swp_parse_path(char * name){
		return ::swpath_parse_path(swpath_impl_p(), name);
	}

	//int swp_lookup_name(char * name){
	//	return ::swpath_lookup_name(swpath_impl_p(), name);
	//}

	char * swp_make_name(char * directory_components, char * file_name){
		return ::swpath_make_name(swpath_impl_p(), directory_components, file_name);
	}
	
	//D void swp_debug_dump(FILE * fp);
	//D char * swp_dump_string_s(char * prefix);

private: // -------------- Private ----------------------

	SWPATH *
	init_(char * path) {
		return swpathM=::swpath_open(path);
	}
};
#endif
