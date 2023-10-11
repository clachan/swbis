/* swi_base.c -- 

   Copyright (C) 2005 Jim Lowe
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS:
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swpath.h"
#include "swheader.h"
#include "swheaderline.h"
#include "ugetopt_help.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "atomicio.h"
#include "swi_common.h"
#include "swi_base.h"

void
swi_vbase_init(void * derived, int type, SWHEADER * index, SWPATH_EX * current)
{
	SWI_BASE * base = &(((SWI_BASE_Derived_*)derived)->baseM);
	int current_file_offset;
	char * tag;
	char * number;
	int ret;

	base->id_startM = SWI_BASE_ID_BEGIN;
	base->type_idM = type;
	base->is_activeM = 0;
	base->b_tagM = NULL;
	base->create_timeM = 0;
	base->mod_timeM = 0;
	base->global_headerM = index;
	base->verboseM = 1;
 	base->swveridM = swverid_open(NULL, NULL); /* Fixme */
	base->id_endM = SWI_BASE_ID_END;
	base->numberM = NULL;
	
	if (index && current) {
		/*
		 * This block of code is executed for filesets and products
		 */
		if (type == SWI_I_TYPE_PROD)
			swverid_set_namespace(base->swveridM, SW_A_product);
		else
			swverid_set_namespace(base->swveridM, SW_A_fileset);

		ret = swi_com_set_header_index(index, current, &(base->header_indexM));
		if (ret != 0) {
			fprintf(stderr, "swinstall:  Fatal: the section of the Global INDEX file belonging\n"
			                "swinstall:  Fatal: to a package file could not be determined.\n");
		}
		SWLIB_ASSERT(ret == 0);
		current_file_offset = swheader_get_current_offset(index);
		swheader_set_current_offset(index, base->header_indexM);
		tag = swheader_get_single_attribute_value(index, SW_A_tag);
		swheader_set_current_offset(index, current_file_offset);

		/*
		 * enforce a sanity check for a tag value
		 */
		if (!tag) {
			fprintf(stderr, "swinstall: Fatal: tag attribute not found\n");	
		}
		SWLIB_ASSERT(tag != NULL);
		base->b_tagM = strdup(tag);

		/*
		 * Store the SW_A_number attribute as well
		 */
		number = swheader_get_single_attribute_value(index, SW_A_number);
		if (number) {
			base->numberM = strdup(number);
		} else {
			base->numberM = NULL;
		}

	} else {
		/* initialize */
		base->header_indexM = 0; /* 0 is the unset value */
		base->b_tagM = strdup("");
	}
}

void
swi_base_assert(SWI_BASE * base)
{
	SWLIB_ASSERT(base->id_startM == SWI_BASE_ID_BEGIN);
	SWLIB_ASSERT(base->id_endM == SWI_BASE_ID_END);
}

void
swi_base_set_is_active(SWI_BASE * base, int n)
{
	base->is_activeM = n;
}

int
swi_vbase_update(void * vbase, void * user_defined_parameter)
{
	SWI_BASE * base = &(((SWI_BASE_Derived_*)vbase)->baseM);
	swi_base_assert(base);
	base->is_activeM = 1;
	if (base->create_timeM == 0) {
		time(&(base->create_timeM));
	}
	time(&(base->mod_timeM));
	return 0;
}

int
swi_vbase_generate_swverid(void * derived, void * user_defined_parameter)
{
	SWI_BASE * base = &(((SWI_BASE_Derived_*)derived)->baseM);
	int current_file_offset;
	char * obj;
	char * tag;
	char * next_attr;
	int ret;
	
	swi_base_assert(base);

	ret = 0;

	/*
	 * Store the current position
	 */
	current_file_offset = swheader_get_current_offset(base->global_headerM);

	/*
	 * Seek to the offset of this object
	 */
	swheader_set_current_offset(base->global_headerM, base->header_indexM);

	if ( 0 /* base->verboseM >= SWC_VERBOSE_SWIDB */) {
		/*
		 *  This code prints the object to stderr, for debugging purposes.
		 */
		fprintf(stderr, "<<< New Object \n");
		obj = swheader_get_current_line(base->global_headerM);
		swheaderline_write_debug(obj, STDERR_FILENO);
		while((next_attr=swheader_get_next_attribute(base->global_headerM)))
			swheaderline_write_debug(next_attr, STDERR_FILENO);
		fprintf(stderr, "<<<\n");

		/*
        	swlib_doif_writef(base->verboseM,  SWC_VERBOSE_SWIDB, (NULL), STDERR_FILENO, "");
		*/
	}


	obj = swheader_get_current_line(base->global_headerM);
	tag = swheader_get_single_attribute_value(base->global_headerM, SW_A_tag);

	/*
	 * 'obj' should be a object keyword line 
	 * Make this sanity assertion
	 */

	if (swheaderline_get_type(obj) != SWPARSE_MD_TYPE_OBJ) {
		/*
		 * Sanity check
		 */
		SWBIS_IMPL_ERROR_DIE(1);	
	}

	/*
	 * Generate the version id for this object
	 */
	ret = swheader_generate_swverid(base->global_headerM, base->swveridM, obj);
	if (ret < 0) {
		/*
		 * error generating version id
		 */
        	swlib_doif_writef(base->verboseM, 
			SWC_VERBOSE_1, (NULL), STDERR_FILENO,
			"error generating version id for %s [tag=%s]\n",
				swheaderline_get_keyword(obj), tag);
		if (base->verboseM >= SWC_VERBOSE_8) {
			obj = swheader_get_current_line(base->global_headerM);
			swheaderline_write_debug(obj, STDERR_FILENO);
			while((next_attr=swheader_get_next_attribute(base->global_headerM)))
				swheaderline_write_debug(next_attr, STDERR_FILENO);
		}
	}

	/*
	 * restore the original position
	 */
	swheader_set_current_offset(base->global_headerM, current_file_offset);
	return ret;
}

int
swi_vbase_set_verbose_level(void * derived, void * verbose_level)
{
	SWI_BASE * base = &(((SWI_BASE_Derived_*)derived)->baseM);
	base->verboseM = *((int*)verbose_level);
	return 0;
}
