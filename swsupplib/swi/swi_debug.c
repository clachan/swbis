/*  swi_debug.c -- format and print the SWI data structure.
*/

/*
   Copyright (C) 2004 Jim Lowe
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "ahs.h"
#include "swvarfs.h"
#include "strar.h"
#include "swlib.h"
#include "swi_debug.h"
#include "swi.h"

static STROB * buf0 = NULL;
static STROB * buf1 = NULL;
static STROB * buf1a = NULL;
static STROB * buf2 = NULL;
static STROB * buf4 = NULL;
static STROB * buf6 = NULL;
static STROB * buf8 = NULL;
static STROB * buf9 = NULL;
static STROB * bufbase = NULL;
static STROB * buf10 = NULL;

char *
swi_script_list_dump_string_s(SWI_SCRIPTS * xx, char * prefix)
{
	int i = 0;
	STROB * buf;
	char * tmp_s;
	
	if (buf4 == (STROB*)NULL) buf4 = strob_open(100);
	buf = buf4;

	strob_sprintf(buf, 0, "%s%p (SWI_SCRIPTS*)\n", prefix,  (void*)xx);

	i = 0;
	while(i < SWI_MAX_OBJ && xx->swi_coM[i]) {
		strob_sprintf(buf, 1, "%s%p->swi_coM[%d] = %p\n",  prefix, (void*)xx, i, xx->swi_coM[i]);
		tmp_s = swi_xfile_script_dump_string_s(xx->swi_coM[i], prefix);
		strob_sprintf(buf, 1, "%s",  tmp_s);
		i++;
	}
	return strob_str(buf);
}

char *
swi_control_script_info_definition_dump(SWI_CONTROL_SCRIPT * xx, char * prefix)
{
	SWI_XFILE * xfile;
	STROB * buf;
	STROB * tmp = strob_open(10);
	SWHEADER * h;
	char * next_attr;
	char * image;
	int memfd;
	int len;
	SWHEADER_STATE state;

	memfd = swlib_open_memfd();
	if (buf10 == (STROB*)NULL) buf10 = strob_open(100);
	buf = buf10;
	strob_strcpy(buf, "");

	xfile = xx->swi_xfileM;
	h = xfile->info_headerM;

	swheader_store_state(h, &state);
	swheader_reset(h);
	swheader_set_current_offset_p(h, &(xfile->INFO_header_indexM));
	swheader_set_current_offset_p_value(h, xx->INFO_offsetM);

	while((next_attr=swheader_get_next_attribute(h)))
		swheaderline_write_debug(next_attr, memfd);

	image = swi_com_get_fd_mem(memfd, &len);
	strob_sprintf(buf, 1, "%s%p->dataM T>>>>>> BEGIN DEFINITION ATTRIBUTES\n", prefix, (void*)xx);
	strob_strcat(buf, image);
	strob_sprintf(buf, 1, "%s%p->dataM T>>>>>> END DEFINITION ATTRIBUTES\n", prefix, (void*)xx);
	uxfio_close(memfd);
	swheader_restore_state(h, &state);
	return strob_str(buf);
}

char *
swi_xfile_member_dump_string_s(SWI_FILE_MEMBER * xx, char * prefix)
{
	STROB * buf;
	STROB * tmp = strob_open(10);
	char * datap;	
	char * newline;	
	if (buf1 == (STROB*)NULL) buf1 = strob_open(100);
	buf = buf1;

	strob_sprintf(buf, 0, "%s%p (SWI_FILE_MEMBER*)\n", prefix,  (void*)xx);

	strob_sprintf(buf, 1, "%s%p->baseM  = [%p]\n", prefix, (void*)xx,  (void*)(&(xx->baseM)));
	strob_sprintf(tmp, 0, "%s%p->baseM:", prefix, (void*)(&(xx->baseM)));
	strob_sprintf(buf, 1, "%s", swi_base_dump_string_s(&(xx->baseM), strob_str(tmp)));

	strob_sprintf(buf, 1, "%s%p->pathnameM         = [%s]\n", prefix, (void*)xx,  xx->pathnameM);
	strob_sprintf(buf, 1, "%s%p->lenM              = [%d]\n", prefix, (void*)xx,  xx->lenM);
	strob_sprintf(buf, 1, "%s%p->dataM T>>>>>> BEGIN DATA\n", prefix, (void*)xx);
	datap = xx->dataM;
	newline = datap;

	while(*datap) {
		if (*datap == '\n') {
			*datap = '\0';
			strob_sprintf(buf, 1, "%s%p->dataM D>%s\n", prefix, (void*)xx, newline);
			newline = datap + 1;
		}	
		datap++;
	}
	strob_sprintf(buf, 1, "%s%p->dataM T>>>>>> END DATA\n", prefix, (void*)xx);
	strob_close(tmp);
	return strob_str(buf);
}

char *
swi_bundle_dump_string_s(SWI_BUNDLE * xx, char * prefix)
{
	STROB * buf;
	
	if (buf1a == (STROB*)NULL) buf1a = strob_open(100);
	buf = buf1a;
	/*
	char type_idM;	
	int is_activeM;		
	char * tagM;	
	time_t create_timeM;
	time_t mod_timeM;
	*/
	strob_sprintf(buf, 0, "%s%p (SWI_BUNDLE*)\n", prefix,  (void*)xx);
	return strob_str(buf);
}

char *
swi_xfile_script_dump_string_s(SWI_CONTROL_SCRIPT * xx, char * prefix)
{
	STROB * buf;
	char * tmps;
	
	if (buf2 == (STROB*)NULL) buf2 = strob_open(100);
	buf = buf2;

	strob_sprintf(buf, 0, "%s%p (SWI_CONTROL_SCRIPT*)\n", prefix,  (void*)xx);
	strob_sprintf(buf, 1, "%s%p->baseM.is_activeM  = [%d]\n", prefix, (void*)xx,  xx->baseM.is_activeM);
	strob_sprintf(buf, 1, "%s%p->b_tagM        = [%s]\n", prefix, (void*)xx,  xx->baseM.b_tagM);
	strob_sprintf(buf, 1, "%s%p->resultM     = [%d]\n", prefix, (void*)xx,  xx->resultM);
	strob_sprintf(buf, 1, "%s%p->afileM      = [%p]\n", prefix, (void*)xx,  xx->afileM);
	strob_sprintf(buf, 1, "%s%p->swi_xfileM      = [%p]\n", prefix, (void*)xx,  xx->swi_xfileM);
	strob_sprintf(buf, 1, "%s%p->INFO_offsetM      = [%d]\n", prefix, (void*)xx,  xx->INFO_offsetM);

	tmps = swi_control_script_info_definition_dump(xx, prefix);
	strob_sprintf(buf, 1, "%s", tmps);

	tmps = swi_xfile_member_dump_string_s(xx->afileM, prefix);
	strob_sprintf(buf, 1, "%s", tmps);

	return strob_str(buf);
}

char * 
swi_base_dump_string_s(SWI_BASE * xx, char * prefix)
{
	STROB * tmp = strob_open(1);
	STROB * buf;
	STROB * bufverid;
	int current_offset;

	if (bufbase == (STROB*)(NULL)) bufbase = strob_open(100);
	buf = bufbase;
	bufverid = strob_open(32);


	strob_sprintf(buf, 0, "%s%p (SWI_BASE*)\n", prefix,  (void*)xx);
	strob_sprintf(buf, 1, "%s%p->id_startM       = [%d]\n", prefix, (void*)xx,     (int)(xx->id_startM));
	strob_sprintf(buf, 1, "%s%p->type_idM        = [%d]\n", prefix, (void*)xx,     (int)(xx->type_idM));
	strob_sprintf(buf, 1, "%s%p->is_activeM      = [%d]\n", prefix, (void*)xx,     xx->is_activeM);
	strob_sprintf(buf, 1, "%s%p->b_tagM          = [%s]\n", prefix, (void*)xx,     xx->b_tagM);
	strob_sprintf(buf, 1, "%s%p->create_timeM    = [%lu]\n", prefix, (void*)xx,    (unsigned long)(xx->create_timeM));
	strob_sprintf(buf, 1, "%s%p->mod_timeM       = [%lu]\n", prefix, (void*)xx,    (unsigned long)(xx->mod_timeM));
	strob_sprintf(buf, 1, "%s%p->header_indexM   = [%d]\n", prefix, (void*)xx,     xx->header_indexM);
	strob_sprintf(buf, 1, "%s%p->global_headerM  = [%p]\n", prefix, (void*)xx,     (void*)(xx->global_headerM));
 	strob_sprintf(buf, 1, "%s%p->swveridM        = [%p]\n", prefix, (void*)xx,     (void*)(xx->swveridM));
 	strob_sprintf(buf, 1, "%s%p->%p->swverid_print() = [%s]\n", prefix, (void*)xx, (void*)(xx->swveridM), swverid_print(xx->swveridM, tmp));

	strob_sprintf(tmp, 0, "%s->%p:", prefix, (void*)(xx->swveridM));
 	strob_sprintf(buf, 1, "%s%p->swveridM        = [%s]\n",
				prefix, (void*)xx, swverid_show_object_debug(xx->swveridM, bufverid, strob_str(tmp)));
	strob_sprintf(buf, 1, "%s%p->verboseM        = [%d]\n", prefix, (void*)xx,     xx->verboseM);
	strob_sprintf(buf, 1, "%s%p->id_endM         = [%d]\n", prefix, (void*)xx,     (int)(xx->id_endM));


	/* Print the object from the INDEX file */

	strob_sprintf(buf, 1, "%s%p->(implied data) T>>>>>> BEGIN INDEX DATA\n", prefix, (void*)xx);
	if (xx->global_headerM) {
		char * next_attr;
		char * obj;
		int level;
		int memfd;
		int len;
		char * image;
		current_offset = swheader_get_current_offset(xx->global_headerM);
		swheader_set_current_offset(xx->global_headerM, xx->header_indexM);

		obj = swheader_get_current_line(xx->global_headerM);
		level = swheaderline_get_level(obj);

		memfd = swlib_open_memfd();

		swheaderline_write_debug(obj, memfd);
		while((next_attr=swheader_get_next_attribute(xx->global_headerM)))
			swheaderline_write_debug(next_attr, memfd);

		image = swi_com_get_fd_mem(memfd, &len);
		strob_strcpy(tmp, image);
		uxfio_close(memfd);

		strob_sprintf(buf, 1, "%s", strob_str(tmp));

		swheader_set_current_offset(xx->global_headerM, current_offset);
	} else {
		strob_sprintf(buf, 1, "%s%p->(implied data)\n", prefix, (void*)xx);
	}
	strob_sprintf(buf, 1, "%s%p->(implied data) T>>>>>> END INDEX DATA\n", prefix, (void*)xx);


	strob_close(tmp);
	return strob_str(buf);
}

char * 
swi_xfile_dump_string_s(SWI_XFILE * xx, char * prefix)
{
	int i = 0;
	STROB * tmp = strob_open(1);
	char * tmp_s;
	SWI_FILE_MEMBER * afile;
	STROB * buf;
	int current_offset;

	if (buf0 == (STROB*)(NULL)) buf0 = strob_open(100);
	buf = buf0;

	strob_sprintf(buf, 0, "%s%p (SWI_XFILE*)\n", prefix,  (void*)xx);

	strob_sprintf(buf, 1, "%s%p->baseM  = [%p]\n", prefix, (void*)xx,  (void*)(&(xx->baseM)));
	strob_sprintf(tmp, 0, "%s%p->baseM:", prefix, (void*)(&(xx->baseM)));
	strob_sprintf(buf, 1, "%s", swi_base_dump_string_s(&(xx->baseM), strob_str(tmp)));

	strob_sprintf(buf, 1, "%s%p->is_selectedM    = [%d]\n", prefix, (void*)xx,     xx->is_selectedM);
	strob_sprintf(buf, 1, "%s%p->typeM           = [%d]\n", prefix, (void*)xx,     xx->typeM);
	strob_sprintf(buf, 1, "%s%p->package_pathM   = [%s]\n", prefix, (void*)xx,     xx->package_pathM);
	strob_sprintf(buf, 1, "%s%p->control_dirM    = [%s]\n", prefix, (void*)xx,     xx->control_dirM);
	strob_sprintf(buf, 1, "%s%p->swi_scM         = [%p]\n", prefix, (void*)xx,     xx->swi_scM);
	strob_sprintf(buf, 1, "%s%p->info_headerM         = [%p]\n", prefix, (void*)xx,     xx->info_headerM);
	strob_sprintf(buf, 1, "%s%p->INFO_header_indexM   = [%d]\n", prefix, (void*)xx,  xx->INFO_header_indexM);
	strob_sprintf(buf, 1, "%s%p->archive_filesM  = [%p]\n", prefix, (void*)xx,       xx->archive_filesM);
	strob_sprintf(buf, 1, "%s%p->swdef_fdM       = [%d]\n",  prefix, (void*)xx,      xx->swdef_fdM);
	strob_sprintf(buf, 1, "%s%p->did_parse_def_fileM = [%d]\n",  prefix, (void*)xx,  xx->did_parse_def_fileM);


	i = 0;
	while ((afile = (SWI_FILE_MEMBER *)cplob_val(xx->archive_filesM, i))) {
		strob_sprintf(tmp, 0, "[n =%-30d]", i);
		strob_sprintf(buf, 1, "%s%p->archive_filesM[%d] = %p\n",  prefix, (void*)xx, i, (void*)afile);
		tmp_s = swi_xfile_member_dump_string_s(afile, prefix);
		strob_sprintf(buf, 1, "%s",  tmp_s);
		i++;
	}
	
	if (xx->swi_scM) {
		strob_sprintf(buf, 1, "%s%p->swi_scM = %p\n",  prefix, (void*)xx, xx->swi_scM);
		tmp_s = swi_script_list_dump_string_s(xx->swi_scM,  prefix);
		strob_sprintf(buf, 1, "%s", tmp_s);
	}
	strob_close(tmp);
	return strob_str(buf);
}

char * 
swi_product_dump_string_s(SWI_PRODUCT * xx, char * prefix)
{
	/*
	* char * package_pathM;	
	* char * control_dirM;
	* char * tagM;		
	* SWI_SCRIPTS * swi_scM;	
	* SWI_XFILE * xfileM;
	* SWI_XFILE * swi_coM[SWI_MAX_OBJ]; 
	*/
	int i = 0;
	STROB * buf;
	STROB * tmp = strob_open(1);
	SWI_XFILE * xfile;
	char * tmp_s;
	
	if (buf6 == (STROB*)NULL) buf6 = strob_open(100);
	buf = buf6;

	strob_sprintf(buf, 0, "%s%p (SWI_PRODUCT*)\n", prefix,  (void*)xx);

	strob_sprintf(buf, 1, "%s%p->p_baseM  = [%p]\n", prefix, (void*)xx,  (void*)(&(xx->p_baseM)));
	strob_sprintf(tmp, 0, "%s%p->baseM:", prefix, (void*)(&(xx->p_baseM)));
	strob_sprintf(buf, 1, "%s", swi_base_dump_string_s(&(xx->p_baseM), strob_str(tmp)));

	strob_sprintf(buf, 1, "%s%p->package_pathM   = [%s]\n", prefix, (void*)xx,  xx->package_pathM);
	strob_sprintf(buf, 1, "%s%p->control_dirM    = [%s]\n", prefix, (void*)xx,  xx->control_dirM);
	strob_sprintf(buf, 1, "%s%p->xfileM         = [%p]\n", prefix, (void*)xx,  (void*)(xx->xfileM));
	
	strob_sprintf(buf, 1, "%s%p->xfileM         = [%p] (BEGIN PFILES)\n",
						prefix, (void*)xx,  (void*)(xx->xfileM));
	tmp_s = swi_xfile_dump_string_s(xx->xfileM, prefix);
	strob_sprintf(buf, 1, "%s", tmp_s);
	strob_sprintf(buf, 1, "%s%p->xfileM         = [%p] (END PFILES)\n",
						prefix, (void*)xx,  (void*)(xx->xfileM));
	
	strob_sprintf(buf, 1, "%s%p->filesetsM  = [%p]\n",
					prefix, (void*)xx,  (void*)(xx->filesetsM));
	if (xx->filesetsM) {
		strob_sprintf(buf, 1, "%s", strar_dump_string_s(xx->filesetsM, prefix));
	}

	/*
	* dump the fileset object addresses.
	*/
	i = 0;	
	while(i < SWI_MAX_OBJ) {
		strob_sprintf(buf, 1, "%s%p->swi_coM[%d]      = [%p]\n",
				prefix, (void*)xx,  i, (void*)(xx->swi_coM[i]));
		i++;
	}

	/*
	* dump the filesets
	*/
	i = 0;
	while(i < SWI_MAX_OBJ && (xfile = xx->swi_coM[i])) {
		strob_sprintf(buf, 1, "%s%p->swi_coM[%d] = %p (BEGIN FILESET[%d])\n",
					prefix, (void*)xx, i, (void*)(xx->swi_coM[i]), i);
		tmp_s = swi_xfile_dump_string_s(xfile, prefix);
		strob_sprintf(buf, 1, "%s", tmp_s);
		strob_sprintf(buf, 1, "%s%p->swi_coM[%d] = %p (END FILESET[%d])\n",
					prefix, (void*)xx, i, (void*)(xx->swi_coM[i]), i);
		i++;
	}
	strob_close(tmp);
	return strob_str(buf);
}

char * 
swi_package_dump_string_s(SWI_PACKAGE * xx, char * prefix)
{
	STROB * buf;
	STROB * tmp = strob_open(1);
	SWI_PRODUCT * prod;
	int i;
	STROB * tmpprefix = strob_open(10);
	char * tmps;
	
	if (buf8 == (STROB*)NULL) buf8 = strob_open(100);
	buf = buf8;

	strob_sprintf(buf, 0, "%s%p (SWI_PACKAGE*)\n", prefix,  (void*)xx);

	strob_sprintf(buf, 1, "%s%p->baseM  = [%p]\n", prefix, (void*)xx,  (void*)(&(xx->baseM)));
	strob_sprintf(tmp, 0, "%s%p->baseM:", prefix, (void*)(&(xx->baseM)));
	strob_sprintf(buf, 1, "%s", swi_base_dump_string_s(&(xx->baseM), strob_str(tmp)));

	strob_sprintf(buf, 1, "%s%p->swi_scM         = [%p]\n", prefix, (void*)xx,  xx->swi_scM);
	strob_sprintf(buf, 1, "%s%p->exlistM             = [%p]\n", prefix, (void*)xx,  xx->exlistM);
	strob_sprintf(buf, 1, "%s%p->dfilesM             = [%p]\n", prefix, (void*)xx,  xx->dfilesM);
	strob_sprintf(buf, 1, "%s%p->current_xfileM      = [%p]\n", prefix, (void*)xx,  xx->current_xfileM);
	strob_sprintf(buf, 1, "%s%p->prev_swpath_exM     = [%p]\n", prefix, (void*)xx,  xx->prev_swpath_exM);
	strob_sprintf(buf, 1, "%s%p->dfiles_attributeM   = [%s]\n", prefix, (void*)xx,  xx->dfiles_attributeM);
	strob_sprintf(buf, 1, "%s%p->pfiles_attributeM   = [%s]\n", prefix, (void*)xx,  xx->pfiles_attributeM);

	strob_sprintf(buf, 1, "%s%p->dfilesM             = [%p] (BEGIN DFILES)\n", prefix, (void*)xx,  xx->dfilesM);
	tmps = swi_xfile_dump_string_s(xx->dfilesM, prefix);
	strob_sprintf(buf, 1, "%s", tmps);
	strob_sprintf(buf, 1, "%s%p->dfilesM             = [%p] (END DFILES)\n", prefix, (void*)xx,  xx->dfilesM);

	strob_sprintf(buf, 1, "%s%p->catalog_start_offsetM = [%d]\n", prefix, (void*)xx,  xx->catalog_start_offsetM);
	strob_sprintf(buf, 1, "%s%p->catalog_end_offsetM = [%d]\n", prefix, (void*)xx,  xx->catalog_end_offsetM);
	strob_sprintf(buf, 1, "%s%p->catalog_lengthM = [%d]\n", prefix, (void*)xx,  xx->catalog_lengthM);
	strob_sprintf(buf, 1, "%s%p->target_pathM = [%s]\n", prefix, (void*)xx,  xx->target_pathM);
	strob_sprintf(buf, 1, "%s%p->target_hostM = [%s]\n", prefix, (void*)xx,  xx->target_hostM);
	strob_sprintf(buf, 1, "%s%p->catalog_entryM = [%s]\n", prefix, (void*)xx,  xx->catalog_entryM);
	strob_sprintf(buf, 1, "%s%p->installed_software_catalogM = [%s]\n",
					prefix, (void*)xx, xx->installed_software_catalogM);

	strob_sprintf(tmpprefix, 0, "%s", prefix);
	tmps = swi_script_list_dump_string_s(xx->swi_scM,  strob_str(tmpprefix));
	strob_sprintf(buf, 1, "%s", tmps);

	/*
	* dump the products
	*/
	i = 0;
	while(i < SWI_MAX_OBJ && (prod = xx->swi_coM[i])) {
		strob_sprintf(buf, 1, "%s%p->swi_coM[%d] = %p (BEGIN PRODUCT[%d])\n",
				 prefix, (void*)xx, i, (void*)(xx->swi_coM[i]), i);
		tmps = swi_product_dump_string_s(prod, prefix);
		strob_sprintf(buf, 1, "%s", tmps);
		strob_sprintf(buf, 1, "%s%p->swi_coM[%d] = %p (END PRODUCT[%d])\n",
				 prefix, (void*)xx, i, (void*)(xx->swi_coM[i]), i);
		i++;
	}
	strob_close(tmp);
	strob_close(tmpprefix);
	return strob_str(buf);
}

char * 
swi_dump_string_s(SWI * xx, char * prefix)
{
	STROB * buf;
	char * tmp_s;
	if (buf9 == (STROB*)NULL) buf9 = strob_open(100);
	buf = buf9;
	strob_sprintf(buf, 0, "%s%p (SWI*)\n", prefix, (void*)xx);
	tmp_s = swi_package_dump_string_s(xx->swi_pkgM, prefix);
	strob_sprintf(buf, 0, "%s", tmp_s);
	return strob_str(buf);
}
