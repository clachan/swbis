/* glbindex.c  --  Routines for searching the global index file.
 */

/*
   Copyright (C) 2005 James H. Lowe, Jr.  <jhlowe@acm.org>
   All Rights Reserved.
  
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <tar.h>

#include "taru.h"
#include "strob.h"
#include "cplob.h"
#include "uxfio.h"
#include "swlib.h"
#include "uinfile.h"
#include "swheader.h"
#include "swheaderline.h"
#include "uinfile.h"
#include "cpiohdr.h"
#include "swverid.h"
#include "swparse.h"
#include "swheader.h"
#include "strar.h"
#include "swpath.h"

static
char *
determine_control_directory(SWHEADER * swheader)
{
	char * value;
	value = swheader_get_single_attribute_value(swheader, SW_A_control_directory);
	if (!value)
		value = swheader_get_single_attribute_value(swheader, SW_A_tag);
	return value;
}

static
char *
search_object_for_directory(char * i_next_line,
		SWHEADER * swheader,
		char * key_dir,
		char * object_name,
		int level)
{
	char * control_directory;
	char * next_line = i_next_line;
	char * keyword;

	while (next_line) {
		keyword = swheaderline_get_keyword(next_line);
		if (strcmp(keyword, object_name) == 0) {
			control_directory = determine_control_directory(swheader);
			if (control_directory) {
				if (strcmp(key_dir, control_directory) == 0) {
					/*
					 * Got a match
					 */
					return next_line;	
				}
			} else {
				/*
				 * this is an error.
				 */
				break;	
			}
		}
		next_line = swheader_get_next_object(swheader, level /*(int)UCHAR_MAX*/, (int)UCHAR_MAX);
	}
	return NULL;
}

static
char *
search_for_fileset(char * i_next_line, SWHEADER * swheader, char * key_dir)
{
	char * ret;
	int level;
	int type;

	level = swheaderline_get_level(i_next_line);
	type = swheaderline_get_type(i_next_line);

	if (type == SWPARSE_MD_TYPE_OBJ) {
		level++;
	}

	ret = search_object_for_directory(i_next_line,
		swheader,
		key_dir,
		SW_A_fileset,
		level);
	return ret;
}

static
char *
search_for_product(char * i_next_line, SWHEADER * swheader, char * key_dir)
{
	char * ret;
	int level;
	level = swheaderline_get_level(i_next_line);

	ret =  search_object_for_directory(i_next_line,
		swheader,
		key_dir,
		SW_A_product,
		(int)UCHAR_MAX);
	return ret;
}

static
int
determine_object(STROB * object_keyword, STRAR * control_dir_list, SWPATH_EX * swpath_ex)
{
	/*
	 * The object, based on the context indicated by swpath_ex
	 * will be one of: 'distribution' 'product' or 'fileset'
	 */	

	/*
	0x80b9a00->pkgpathname     = [swbis-0.0/catalog/dfiles/INFO]
	0x80b9a00->is_catalog      = [1]
	0x80b9a00->ctl_depth       = [0]
	0x80b9a00->prepath         = [swbis-0.0]
	0x80b9a00->dfiles          = [dfiles]
	0x80b9a00->pfiles          = []
	0x80b9a00->product_control_dir = []
	0x80b9a00->fileset_control_dir = []
	0x80b9a00->pathname            = [INFO]
	0x80b9a00->basename            = [INFO]
	*/

	if (
		(
			swpath_ex->is_minimal_layoutM == 0 &&
			strlen(swpath_ex->product_control_dir) &&
			!strlen(swpath_ex->fileset_control_dir) &&
			1
		) || 
		(
			swpath_ex->is_minimal_layoutM == 1 &&
			strlen(swpath_ex->pfiles) &&
			(
				!strlen(swpath_ex->fileset_control_dir)
			) &&
			1
		)
	) {
		/*
		 * Its a product
		 */
		strar_add(control_dir_list, SW_A_product);
		strar_add(control_dir_list, swpath_ex->product_control_dir);
		strob_strcpy(object_keyword, SW_A_product);
	}
	else if (
		(
			swpath_ex->is_minimal_layoutM == 0 &&
			strlen(swpath_ex->product_control_dir) &&
			strlen(swpath_ex->fileset_control_dir) &&
			1
		) ||
		(
			/*
			 * This will match  "<path>/catalog/INFO"
			 */
			swpath_ex->is_minimal_layoutM == 1 &&
			!strlen(swpath_ex->product_control_dir) &&
			!strlen(swpath_ex->fileset_control_dir) &&
			strcmp(swpath_ex->basename, SW_A_INFO) == 0 &&
			1
		)
	) {
		/*
		 * Its a fileset
		 */
		strar_add(control_dir_list, SW_A_product);
		strar_add(control_dir_list, swpath_ex->product_control_dir);
		strar_add(control_dir_list, SW_A_fileset);
		strar_add(control_dir_list, swpath_ex->fileset_control_dir);
		strob_strcpy(object_keyword, SW_A_fileset);
	} else {
		{
			/*
			 * This code path shold not happen
			 */
			STROB * tmp = strob_open(10);
			fprintf(stderr, "swinstall: Uh-Oh WARNING IN " __FILE__ " %d\n", __LINE__); 
			fprintf(stderr, "%s", swpath_ex_print(swpath_ex, tmp, "glbindex.c "));
			strob_close(tmp);
		}
		strob_strcpy(object_keyword, SW_A_distribution);
	}

	return 0;
}


/** 
 * Locate the portion of the global index that aaplies to
 * the broken down POSIX pathname in (SWHEADER *)(global_index)
 */
int
glbindex_find_by_swpath_ex(SWHEADER * global_index, SWPATH_EX * swpath_ex)
{
	int ret = 0;
	char * next_line;
	SWHEADER * swheader;
	STRAR * control_dir_list;
	STROB * object_keyword;

	swheader = global_index;
	swheader_store_state(swheader, NULL);

	swheader_set_current_offset(swheader, 0 /* the beginning */);

	control_dir_list = strar_open();
	object_keyword = strob_open(20);

	determine_object(object_keyword, control_dir_list, swpath_ex);
	
	/*
	 * Now 'control_dir_list' contains data pairs:
	 *    strar[0]     <object_name0>
	 *    strar[1]     <control_directory0> 
	 *    strar[2]     <object_name1>
	 *    strar[3]     <control_directory1>
         *     ....
         *    strar[N] NULL
	 */
	
	/*
	 * Now loop thru the INDEX header and locate the object that applies to
	 * the pathname by comparing the 'control_directory' attributes to the
	 * odd number indexes in 'control_dir_list'
	 */

/*
	while (next_line){
		swheaderline_write_debug(next_line, STDERR_FILENO); 
		keyword = swheaderline_get_keyword(next_line);
		fprintf(stderr, "test: [%s]\n", keyword);
*/
		
	next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
	SWLIB_ASSERT(next_line != NULL);
	/*
	 * next_line should be the distribution object
	 */

	if (strar_num_elements(control_dir_list) > 3) {
		/*
		 * Looking for a fileset object
		 * First search for the product with the control directory
		 * Then search the filesets within the product.
		 */
		next_line = search_for_product(next_line, swheader, strar_get(control_dir_list, 1));
		SWLIB_ASSERT(next_line != NULL);
		next_line = search_for_fileset(next_line, swheader, strar_get(control_dir_list, 3));
		SWLIB_ASSERT(next_line != NULL);
	} else {
		/*
		 * Looking for a product object
		 */
		next_line = search_for_product(next_line, swheader, strar_get(control_dir_list, 1));
		SWLIB_ASSERT(next_line != NULL);
	}

	ret = swheader_get_current_offset(swheader);

	/*
	 * ret is now the offset of the object that is refered by the
	 * control_directories as determined from the package path names.
	 */

	strar_close(control_dir_list);
	strob_close(object_keyword);
	swheader_restore_state(swheader, NULL);
	return ret; 
}
