/*  swi_distdata.c - Get bundle/product and distribution attributes. */
/*
   Copyright (C) 2004 Jim Lowe
   All rights reserved.
  
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "swlib.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swi.h"
#include "swcommon.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swevents.h"
#include "swinstall.h"
#include "swheader.h"
#include "swheaderline.h"
#include "strar.h"

static
int
loop_and_record(SWHEADER * INDEX, char * object_keyword, STRAR * list, 
			STRAR * vendor_list, STRAR * revision_list)
{
	char * obj;	
	char * value = NULL;
	char * attrline = NULL;
	int count = 0;
	int retval;

	obj = swheader_get_object_by_tag(INDEX, object_keyword, "*");
	while(obj) {
		attrline = swheader_get_attribute(INDEX, SW_A_tag, NULL);
		if (!attrline) {
			fprintf(stderr, "no tag found for %s\n", object_keyword);
			retval = 20;
			return retval;
		}
		value = swheaderline_get_value(attrline, NULL);
		strar_add(list, value);

		attrline = swheader_get_attribute(INDEX, SW_A_vendor_tag, NULL);
		if (attrline) {
			value = swheaderline_get_value(attrline, NULL);
			if (vendor_list) strar_add(vendor_list, value);
		}
	
		attrline = swheader_get_attribute(INDEX, SW_A_revision, NULL);
		if (!attrline) {
			/* fprintf(stderr, "no revision found for %s\n", object_keyword); */
			retval = 23;
			return retval;
		}
		value = swheaderline_get_value(attrline, NULL);
		if (revision_list)
			strar_add(revision_list, value);
	
		swheader_get_next_object(INDEX, (int)UCHAR_MAX, (int)UCHAR_MAX);
		obj = swheader_get_object_by_tag(INDEX, object_keyword, "*");
		count++;
	}
	return count;
}

static
int
determine_catalog_directories(SWI_DISTDATA * part1,
			int enforce_swinstall_policy)
{
	char * bundle_tag;
	char * product_tag;

	bundle_tag = strar_get(part1->bundle_tagsM, 0);
	product_tag = strar_get(part1->product_tagsM, 0);

	if (enforce_swinstall_policy) {
		/*
		* allow only one product and zero or one bundle.
		* FIXME: not done yet.
		*/
	}

	if (bundle_tag == NULL)
		bundle_tag = product_tag;

	if (product_tag == NULL)
		return 1;

	part1->catalog_bundle_dir1M = strdup(bundle_tag);
	return 0;
}

SWI_DISTDATA *
swi_distdata_create(void)
{
	SWI_DISTDATA * part1 = (SWI_DISTDATA*)malloc(sizeof(SWI_DISTDATA));
	swi_distdata_initialize(part1);
	return part1;
}

void
swi_distdata_initialize(SWI_DISTDATA * part1)
{
	part1->did_part1M = 0;
	part1->dist_tagM = (char*)(NULL);
	part1->dist_revisionM = (char*)(NULL);
	part1->catalog_bundle_dir1M = (char*)(NULL);
	part1->bundle_tagsM = strar_open();
	part1->product_tagsM = strar_open();
	part1->product_revisionsM = strar_open();
	part1->vendor_tagsM = strar_open();
}

int 
swi_distdata_resolve(SWI * swi, SWI_DISTDATA * part1,
			int enforce_swinstall_policy)
{
	char * obj;	
	int offset;
	SWHEADER * INDEX;
	char * value = NULL;
	char * attrline = NULL;
	int retval;
	int count = 0;

	swi_distdata_initialize(part1);

	INDEX = SWI_get_index_header(swi);
	swheader_reset(INDEX);

	/*
	 * ========================================
	 * loop through the bundles in the INDEX file.
	 * ========================================
	 */
	swheader_reset(INDEX);
	count = loop_and_record(INDEX, SW_A_bundle, part1->bundle_tagsM,
				(STRAR*)NULL, (STRAR*)NULL);

	/*
	 * ========================================
	 * loop through the products in the INDEX file.
	 * ========================================
	 */
	swheader_reset(INDEX);
	count = loop_and_record(INDEX, SW_A_product, part1->product_tagsM,
			part1->vendor_tagsM, part1->product_revisionsM);

	if (count == 0) {
		retval = 30;
		return retval;
	}

	/*
	 * ========================================
	 * Now get some metadata from the distribution
	 * object of the the global INDEX file.
	 * ========================================
	 */

	swheader_reset(INDEX);
	obj = swheader_get_object_by_tag(INDEX, SW_A_distribution, "*");
	if (!obj) {
		retval = 40;
		return retval;
	}
	offset = swheader_get_current_offset(INDEX);

	attrline = swheader_get_attribute(INDEX, SW_A_tag, NULL);
	if (!attrline) { 
		/*
		 * dist.tag is optional
		 * If none, use the first product tag.
		 */
		value = strar_get(part1->product_tagsM, 0);
	} else {
		value = swheaderline_get_value(attrline, NULL);
	}
	part1->dist_tagM = strdup(value);
	swheader_reset(INDEX);

	/*
	 * Now analize the results and determine the 
	 * installed_software catalog directories.
	 */

	if (determine_catalog_directories(part1, enforce_swinstall_policy))
		return 44;

	retval = 0;
	return retval;
}

void
swi_distdata_delete(SWI_DISTDATA * part1)
{
	free(part1->dist_tagM);
	strar_close(part1->bundle_tagsM);
	strar_close(part1->product_tagsM);
	strar_close(part1->product_revisionsM);
	strar_close(part1->vendor_tagsM);
}

