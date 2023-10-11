/* swi.c -- POSIX package decoding. 

  Copyright (C) 2004,2005,2006,2007 Jim Lowe
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swheader.h"
#include "swheaderline.h"
#include "ugetopt_help.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "swi.h"
#include "swinstall.h"
#include "atomicio.h"
#include "ls_list.h"
#include "swevents.h"
/* #include "swevents_array.h" */
#include "debug_config.h"


static
int
show_nopen(void)
{
	int ret;
	ret = /*--*/open("/dev/null",O_RDWR);
	if (ret<0)
		fprintf(stderr, "fcntl error: %s\n", strerror(errno));
	close(ret);
	return ret;
}

static
int
find_swdef_in_INFO(SWI_XFILE * xfile, char * tag, char * name)
{
	int ret;
	char * p;
	ret = -1;
	p = swheader_get_object_by_tag(xfile->info_headerM, SW_A_control_file, tag);
	E_DEBUG3("tag==[%s] name=[%s]", tag, name);
	E_DEBUG2("p=[%p]", (void*)p);
	if (p) {
		ret = p - swheader_get_image_head(xfile->info_headerM);
		E_DEBUG2("ret=%d", ret);
		/* Sanity check */
		if (ret < 0 ) {
			fprintf(stderr, "%s: error: invalid value calculated in find_swdef_in_INFO: %d\n", swlib_utilname_get(), ret);
			ret = -1;
		}
		if (ret >987654321) {
			/* Sanity check because an INFO file is probably never this big.
			 * Probably delete this in the future.
			 */
			fprintf(stderr, "%s: error: possible invalid value calculated in find_swdef_in_INFO: %d\n", swlib_utilname_get(), ret);
			ret = -1;
		}
	}
	return ret;
}

static 
SWPATH_EX *
swi_get_last_swpath_ex(SWI * swi)
{
	SWPATH_EX * ret;
	CPLOB * list = swi->swi_pkgM->exlistM;
	int index;

	/*
	* The list is always Null Terminated and the null is
	* part of the list therefore the array index of the last
	* useful element is 2 less than the reported nused.
	*/	
	index = cplob_get_nused(list) - 2;
	ret = (SWPATH_EX *)cplob_val(list, index);
	return ret;
}


/**
*  Find a product or fileset by the 'tag'
*/
static
SWI_BASE *
find_object_by_swsel(void * parent, char * swsel, char * number, int * p_index)
{
	int i;
	SWI_PACKAGE * package;
	SWI_BASE * object;
	char * verid_delim;

	if (p_index) *p_index = -1;
	package = (SWI_PACKAGE *)(parent);
	for(i=0; i<SWI_MAX_OBJ; i++) {
		object = (SWI_BASE*)(&(package->swi_coM[i]->p_baseM));
		if (object) {
			/*
			 * FIXME, crudely handle version ids by ignoring them
			 */
			verid_delim = strchr(swsel, ',');
			if (!verid_delim) {
				verid_delim = swsel + strlen(swsel);
			}
			
			if (strncmp(swsel, object->b_tagM, (size_t)(verid_delim - swsel)) == 0) {
				if (number && object->numberM) {
					if (strcmp(number, object->numberM) != 0) {
						continue;
					} 
				}
				if (p_index) *p_index = i;
				return object;
			}
		} else {
			return NULL;
		}
	}
	return NULL;
}

static
int
need_new_pfiles(SWI * swi)
{
	SWPATH * swpath = swi->swpathM;
	char * pfiles_dir;
	char * pfiles_attribute;
	char * prod_dir;
	
	pfiles_dir = swpath_get_pfiles(swpath);
	prod_dir = swpath_get_product_control_dir(swpath);
	pfiles_attribute = swi->swi_pkgM->pfiles_attributeM; 

	if (
		(strlen(pfiles_dir) && 
		strcmp(pfiles_attribute,  pfiles_dir)) ||
		(strlen(prod_dir) == 0)
	) {
		/*
		* Sanity check.
		*    Error
		*/
		return -1;
	}

	if (strlen(pfiles_dir) && 
		strcmp(pfiles_attribute,  pfiles_dir) == 0 &&
		strlen(swpath_get_basename(swpath))
	) {
		return 1;
	}
	return 0;
}

int
swi_audit_all_compatible_products(SWI * swi, SWUTS * target_uts, int *pn_products, int * p_swi_index)
{
	SWI_PRODUCT * product;
	SWHEADER * global_index;
	SWUTS * product_uts;
	int product_number;
	int ret;
	int n_matches;

	if (p_swi_index) *p_swi_index = -1;
	if (pn_products) *pn_products = 0;
	n_matches = 0;

	/* Get the global INDEX access header object */
	global_index = swi_get_global_index_header(swi);
	swheader_store_state(global_index, NULL);

	/* Loop over the products */
	product_number = 0;
	while ((product=swi_package_get_product(swi->swi_pkgM, product_number++))) {

		/* Now set the offset of the relevant part of the global INDEX file
		   which is the start of the product definition */
		if (pn_products) ++(*pn_products);
		swheader_reset(global_index);
		swheader_set_current_offset(global_index, product->p_baseM.header_indexM);

		product_uts = swuts_create();
		swi_get_uts_attributes_from_current(swi, product_uts, global_index);
		ret = swuts_compare(target_uts, product_uts, 0 /*verbose*/);
		if (ret == 0) {
			n_matches++;
			product->is_compatibleM = SW_TRUE;
			if (p_swi_index) *p_swi_index = product_number-1;
		}
		swuts_delete(product_uts);
	}
	swheader_restore_state(global_index, NULL);
	return n_matches;
}

void
swi_set_utility_id(SWI * swi, int swc_u_id)
{
	swi->swc_idM = swc_u_id;
}

int
swi_is_definition_file(SWI * swi, char * name, int *swdeffile_type_p)
{
	char * s;
	if ((s=strstr(name, "/" SW_A_INDEX)) && *(s+6) == '\0') {
		if (swi_is_global_index(swi->swpathM, name)) {
			if (swi->swi_pkgM->did_parse_def_fileM) {
				swi_com_fatal_error("loc=index", __LINE__);
			}
			swi->swi_pkgM->did_parse_def_fileM = 1;
			*swdeffile_type_p = SWPARSE_SWDEF_FILETYPE_INDEX;
			return 1;
		} else {
			;
			/*
			 * ignore INDEX files that are not the 
			 * global index file.
			 */
		}
	}
	else if ((s=strstr(name, "/" SW_A_INFO)) && *(s+5) == '\0') {
		if (swi->swi_pkgM->current_xfileM->did_parse_def_fileM) {
			swi_com_fatal_error("loc=info", __LINE__);
		}
		swi->swi_pkgM->current_xfileM->did_parse_def_fileM = 1;
		*swdeffile_type_p = SWPARSE_SWDEF_FILETYPE_INFO;
		return 1;
	}
	*swdeffile_type_p = -1;
	return 0;
}

void
swi_xfile_add_control_script(SWI * swi, SWI_XFILE * xfile, char * name, 
					int fd, char * tag)
{
	SWI_CONTROL_SCRIPT * sc;
	SWI_FILE_MEMBER * s;
	int offset;

	sc = swi_control_script_create();
	s = swi_xfile_construct_file_member(xfile, name, fd,
					swi->xformatM->swvarfsM);
	sc->baseM.b_tagM = strdup(tag);
	swi_com_check_clean_relative_path(sc->baseM.b_tagM);
	sc->afileM = s;
	sc->swi_xfileM = (void*)xfile;
	offset = find_swdef_in_INFO(xfile, tag, name);
	sc->INFO_offsetM = offset;
	if (offset < 0) {
		fprintf(stderr, "warning: find_swdef_in_INFO returned %d\n", offset);
		/* should be an error in the future */
	}
	swi_add_script(xfile->swi_scM, sc);
	return;
}

void
swi_xfile_add_file_member(SWI * swi, SWI_XFILE * xfile, char * name, int fd)
{
	SWI_FILE_MEMBER * s;
	s = swi_xfile_construct_file_member(xfile, name, fd,
					swi->xformatM->swvarfsM);
	cplob_add_nta(xfile->archive_filesM, (char*)s);
	return;
}

int
swi_get_distribution_attributes(SWI * swi, SWHEADER * swheader)
{
	char * value;
	char * obj;
	
	E_DEBUG("");
	swi_com_header_manifold_reset(swheader);
	E_DEBUG("");
	obj = swheader_get_object_by_tag(swheader, SW_A_distribution, "*" );
	E_DEBUG("");
	if (!obj) {
		return -1;
	}
	E_DEBUG("");
	value = swheader_get_single_attribute_value(swheader, SW_A_dfiles);
	if (value)
		swi->swi_pkgM->dfiles_attributeM = strdup(value);
	else
		swi->swi_pkgM->dfiles_attributeM = strdup(SW_A_dfiles);

	E_DEBUG("");
	value = swheader_get_single_attribute_value(swheader, SW_A_pfiles);
	if (value)
		swi->swi_pkgM->pfiles_attributeM = strdup(value);
	else
		swi->swi_pkgM->pfiles_attributeM = strdup(SW_A_pfiles);

	E_DEBUG("");
	return 0;
}

void 
swi_package_delete(SWI_PACKAGE * s)
{
	SWPATH_EX * expath;
	int index;
	int i;

	index = 0;
	expath = (SWPATH_EX*)cplob_val(s->exlistM, index++);
	while (expath) {
		swpath_delete_export(expath);
		expath = (SWPATH_EX*)cplob_val(s->exlistM, index++);
	}
	free((void*)(cplob_release(s->exlistM)));	

	/* if (s->current_xfileM) swi_xfile_delete(s->current_xfileM); */
	if (s->catalog_entryM != (char*)NULL) free(s->catalog_entryM);
	if (s->target_pathM != (char*)NULL) free(s->target_pathM);
	if (s->target_hostM != (char*)NULL) free(s->target_hostM);
	if (s->installed_software_catalogM != (char*)NULL) {
		 free(s->installed_software_catalogM);
	}
	if (s->dfilesM) swi_xfile_delete(s->dfilesM);
	if (s->baseM.global_headerM) swheader_close(s->baseM.global_headerM);
	if (s->swdef_fdM > 0) uxfio_close(s->swdef_fdM);
	if (s->installer_sigM) free(s->installer_sigM);
	s->swdef_fdM = -1;
	swi_scripts_delete(s->swi_scM);
	for(i=0; i<SWI_MAX_OBJ; i++) {
		if (s->swi_coM[i])
			swi_product_delete(s->swi_coM[i]);
	}
	free(s);
	return; 
}

SWI_PACKAGE *
swi_package_create(void)
{
	SWI_PACKAGE * s = (SWI_PACKAGE *)malloc(sizeof(SWI_PACKAGE));
	E_DEBUG("");
	swi_com_assert_pointer((void*)s, __FILE__, __LINE__);
	swiInitListOfObjects((void**)(s->swi_coM));
	swi_vbase_init(s, SWI_I_TYPE_PACKAGE, NULL, NULL);
	s->swi_scM = swi_scripts_create();
	s->exlistM = cplob_open(1);
	s->baseM.global_headerM = NULL;
	s->swdef_fdM = -1;
	s->current_productM = NULL;
	s->dfilesM = NULL;
	s->current_xfileM = NULL;
	s->did_parse_def_fileM = 0;
	s->prev_swpath_exM = NULL; 
	s->catalog_lengthM = 0;
	s->catalog_entryM = (char*)NULL;
	s->target_pathM = (char*)NULL;
	s->target_hostM = (char*)NULL;
	s->installed_software_catalogM = (char*)NULL;
	s->locationM = (char*)NULL;
	s->qualifierM = (char*)NULL;
	s->is_minimal_layoutM = 0;
	s->installer_sigM = NULL;
	s->installed_catalog_ownerM = NULL;
	s->installed_catalog_groupM = NULL;
	s->installed_catalog_modeM = (mode_t)(0);
	return s;
}

void
swi_delete(SWI * s)
{
	E_DEBUG2("Entering swi_delete LOWEST FD=%d", show_nopen());
	swbis_devnull_close(s->nullfdM);
	swi_package_delete(s->swi_pkgM);
	swicol_delete(s->swicolM);
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	if (s->distdataM){
		 swi_distdata_delete(s->distdataM);
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	if (s->xformat_close_on_deleteM) {
		E_DEBUG("calling xformat_close");
		xformat_close(s->xformatM);
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	if (s->swvarfs_close_on_deleteM) {
		E_DEBUG("calling swvarfs_close");
		swvarfs_close(s->swvarfsM);
	}
	E_DEBUG2("LOWEST FD=%d", show_nopen());
	if (s->uinformat_close_on_deleteM) {
		E_DEBUG("calling uinfile_close");
		uinfile_close(s->uinformatM);
	}
	E_DEBUG2("LEAVING swi_delete: LOWEST FD=%d", show_nopen());

	uxfio_close(s->excluded_file_conflicts_fdM);
	uxfio_close(s->replaced_file_conflicts_fdM);
	uxfio_close(s->pending_file_conflicts_fdM);

	free(s);
}

SWI * swi_create(void)
{
	SWI * swi;

	E_DEBUG("");
	swi = (SWI*)malloc(sizeof(SWI));
	swi_com_assert_pointer((void*)swi, __FILE__, __LINE__);
	swi->xformatM = NULL;
	swi->swvarfsM = NULL;
	swi->uinformatM = NULL;
	swi->swpathM = NULL;
	swi->swi_pkgM = swi_package_create();
	swi->nullfdM = swbis_devnull_open("/dev/null", O_RDWR, 0);
	memset(swi->tarbufM, '\0', sizeof(swi->tarbufM));
	swi->distdataM = NULL;
	swi->opt_alt_catalog_rootM = 0;
	swi->swicolM = swicol_create();
	swi->exported_catalog_prefixM = NULL;
	swi->verboseM = 0;
	swi->debug_eventsM = 0;
	swi->target_version_idM = swverid_open("HOST", NULL);
	swi->does_have_payloadM = -1; /* unset */
	swi->xformat_close_on_deleteM = 0;
	swi->swvarfs_close_on_deleteM = 0;
	swi->uinformat_close_on_deleteM = 0;

	swi->excluded_file_conflicts_fdM = -1;
	swi->replaced_file_conflicts_fdM = -1;
	swi->pending_file_conflicts_fdM = -1;
	return swi;
}

void
swi_product_delete(SWI_PRODUCT * s)
{
	int i;
	if (s->p_baseM.b_tagM) free(s->p_baseM.b_tagM);
	if (s->package_pathM) free(s->package_pathM);
	if (s->control_dirM) free(s->control_dirM);

	if (s->filesetsM) strar_close(s->filesetsM);

	if (s->xfileM) swi_xfile_delete(s->xfileM);
	
	for(i=0; i<SWI_MAX_OBJ; i++) {
		if (s->swi_coM[i])
			swi_xfile_delete(s->swi_coM[i]);
	}
	free(s);
}

SWI_PRODUCT *
swi_product_create(SWHEADER * global_index_header, SWPATH_EX * current)
{
	SWI_PRODUCT * s = (SWI_PRODUCT *)malloc(sizeof(SWI_PRODUCT));
	E_DEBUG("");
	swi_com_assert_pointer((void*)s, __FILE__, __LINE__);

        #ifdef SWSWINEEDDEBUG
                {
                STROB * tmp = strob_open(100);
                fprintf(stderr, "%s", 
                        swpath_ex_print(current, tmp, "swi_product_create"));                            
                strob_close(tmp);
                }
        #endif

	swi_vbase_init(s, SWI_I_TYPE_PROD, global_index_header, current);

	s->package_pathM = (char*)NULL;
	s->control_dirM = (char*)NULL;
	s->filesetsM = (STRAR*)NULL;
	swiInitListOfObjects((void**)(s->swi_coM));
	s->xfileM = NULL;
	s->is_selectedM = 1;
	s->is_compatibleM = 0;
	return s;
}

void
swi_product_add_fileset(SWI_PRODUCT * thisis, SWI_XFILE * v)
{
	E_DEBUG("ENTERING");
	swiAddObjectToList((void **)(thisis->swi_coM), (void *)v);
	E_DEBUG("LEAVING");
}

void
swi_package_add_product(SWI_PACKAGE * thisis, SWI_PRODUCT * v)
{
	E_DEBUG("ENTERING");
	swiAddObjectToList((void **)(thisis->swi_coM), (void *)v);
	E_DEBUG("LEAVING");
}

int
swi_store_file(SWI * swi, char * name, SWI_XFILE * current_xfile) {
	int fd;
	char * tag;
	XFORMAT * xformat = swi->xformatM;
	
	E_DEBUG2("ENTERING name=[%s]", name);
	swi_com_assert_pointer((void*)current_xfile, __FILE__, __LINE__);
	
	fd = xformat_u_open_file(xformat, name);
	swi_com_assert_value(fd >= 0, __FILE__, __LINE__);

	if (swi_afile_is_ieee_control_script(name)) {
		tag = swlib_basename(NULL, name);
		swi_xfile_add_control_script(swi, current_xfile,
			name, fd, tag);
	} else {
		swi_xfile_add_file_member(swi, current_xfile, name, fd);
	}
	xformat_u_close_file(xformat, fd);
	E_DEBUG("LEAVING");
	return 0;
}

int
swi_expand_shared_file_control_sripts(SWI * swi, SWI_XFILE * xfile)
{
	SWHEADER * swheader = xfile->info_headerM;
	SWI_CONTROL_SCRIPT * script;
	SWI_CONTROL_SCRIPT * new_script;
	SWI_FILE_MEMBER * afile;
	char * next_line;
	char * path_attr;
	char * path_value;
	char * tag_attr;
	char * tag_value;

	E_DEBUG("ENTERING");
	swheader_reset(swheader);
	swheader_set_current_offset_p_value(swheader, 0);

	next_line = swheader_get_next_object(swheader, 
		(int)UCHAR_MAX, (int)UCHAR_MAX);
	while (next_line){
		swheader_goto_next_line((void *)swheader, 
			swheader_get_current_offset_p(swheader), 
				SWHEADER_PEEK_NEXT);
		path_attr = swheader_get_attribute(swheader,
			SW_A_path, NULL);
		SWLIB_ASSERT(path_attr != NULL);
		tag_attr = swheader_get_attribute(swheader,
			SW_A_tag, NULL);
		SWLIB_ASSERT(tag_attr != NULL);

		/*
		 * Now search the control scripts to see if this
		 * tag is present.
		 */
		swi_com_assert_value(tag_attr != NULL, __FILE__, __LINE__);
		tag_value = swheaderline_get_value(tag_attr, NULL);
		path_value = swheaderline_get_value(path_attr, NULL);

		if (swi_afile_is_ieee_control_script(tag_value)) {
			script = swi_xfile_get_control_script_by_tag(xfile,
				tag_value);
			if (script == NULL) {
				/*
				 * Not found, need to make it.
				 * This happens when control_files share the
				 * same script.
				 */

				/* fprintf(stderr, "tag=[%s] Not Found\n", tag_attr ? swheaderline_get_value(tag_attr, NULL) : "null"); */
				/* swi_xfile_add_control_script(swi, xfile, name, int fd, tag); */

				swi_com_assert_value(path_value != NULL, __FILE__, __LINE__);
				script = swi_xfile_get_control_script_by_path(xfile, path_value);
				/* BUG ME swi_com_assert_value(source_script != NULL, __FILE__, __LINE__); */

				/*
				 * Now we need to make a new (SWI_CONTROL_SCRIPT *) object.
				 */
				if (script) {
					afile = script->afileM;
				}  else {
					afile = swi_xfile_get_control_file_by_path(xfile, path_value);
				}		
		
				SWLIB_ASSERT(afile != NULL);
					
				new_script = swi_control_script_create();
				new_script->baseM.b_tagM = strdup(tag_value);
				swi_com_check_clean_relative_path(new_script->baseM.b_tagM);
				new_script->afileM = afile;
				(afile->refcountM)++;
				swi_add_script(xfile->swi_scM, new_script);
			}
		} else {
			;	
		}
		next_line = swheader_get_next_object(swheader, 
			(int)UCHAR_MAX, (int)UCHAR_MAX);
	}
	return 0;
}

int
swi_parse_file(
		SWI * swi, 
		char * name, 
		int swdeffile_type)
{
	XFORMAT * xformat = swi->xformatM;
	char * base;
	char * newbase;
	SWHEADER * swheader;
	SWPATH * swpath = swi->swpathM;
	int ret;
	int ufd;
	int curfd = swlib_open_memfd();
	int ofd = swlib_open_memfd();
	int len;

	E_DEBUG("ENTERING");
	ufd = xformat_u_open_file(xformat, name);
	if (ufd < 0) {
		swi_com_fatal_error( __FILE__, __LINE__);
	}	

	swlib_pipe_pump(curfd, ufd);
	xformat_u_close_file(xformat, ufd);

	uxfio_lseek(curfd, (off_t)0, SEEK_SET);
	
	ret = sw_yyparse(curfd, ofd, name, 0, SWPARSE_FORM_MKUP_LEN);
	E_DEBUG("");
	if (ret) {
		E_DEBUG("");
		fprintf(stderr,
		"%s: error parsing %s\n", swlib_utilname_get(), name);
		return ret;
		swi_com_fatal_error( __FILE__, __LINE__);
	}
	E_DEBUG("");
	uxfio_write(ofd, "\x00", 1);
	base = swi_com_get_fd_mem(ofd, &len);
	newbase = (char*)malloc(len);
	swi_com_assert_pointer((void*)(newbase), __FILE__, __LINE__);
	memcpy(newbase, base, len);
	swi_com_close_memfd(ofd);

	swheader = swheader_open((char *(*)(void *, int *, int))(NULL), NULL);
	swheader_set_image_head(swheader, newbase);	
	uxfio_lseek(curfd, (off_t)0, SEEK_SET);

	swi_com_assert_pointer((void*)(swi->swi_pkgM), __FILE__, __LINE__);
	switch(swdeffile_type) {
		case SWPARSE_SWDEF_FILETYPE_INFO:
			E_DEBUG("case SWPARSE_SWDEF_FILETYPE_INFO");
			E_DEBUG2("xfile: %p", swi->swi_pkgM->current_xfileM);
			swi_com_assert_pointer(
				(void*)(swi->swi_pkgM->current_xfileM),
					__FILE__, __LINE__);
			swi->swi_pkgM->current_xfileM->info_headerM = swheader;
			swi->swi_pkgM->current_xfileM->INFO_header_indexM = 0;
			swi->swi_pkgM->current_xfileM->swdef_fdM = curfd;
			swi->swi_pkgM->current_xfileM->did_parse_def_fileM = 1;
		
			swheader_set_current_offset_p(swheader, 
			&(swi->swi_pkgM->current_xfileM->INFO_header_indexM));
			swheader_set_current_offset_p_value(swheader, 0);
			swheader_reset(swheader);
			swheader_goto_next_line(swheader, 
				swheader_get_current_offset_p(swheader),
				SWHEADER_GET_NEXT);
			break;
		case SWPARSE_SWDEF_FILETYPE_INDEX:
			E_DEBUG("SWPARSE_SWDEF_FILETYPE_INDEX");
			swi->swi_pkgM->baseM.global_headerM = swheader;
			swi->swi_pkgM->baseM.header_indexM = 0;
			swi->swi_pkgM->swdef_fdM = curfd;
			swi->swi_pkgM->did_parse_def_fileM = 1;
			
			E_DEBUG("");
			swheader_set_current_offset_p(swheader, 
					&(swi->swi_pkgM->baseM.header_indexM));
			swheader_set_current_offset_p_value(swheader, 0);
			swheader_reset(swheader);
			E_DEBUG("");
			swheader_goto_next_line(swheader,
				swheader_get_current_offset_p(swheader),
				SWHEADER_GET_NEXT);
			
			E_DEBUG("");
			if (swi_get_distribution_attributes(swi, swheader) != 0)
				swi_com_fatal_error( __FILE__, __LINE__);
			E_DEBUG("");
			swpath_set_dfiles(swpath, 
					swi->swi_pkgM->dfiles_attributeM);
			E_DEBUG("");
			swpath_set_pfiles(swpath, 
					swi->swi_pkgM->pfiles_attributeM);
			E_DEBUG("");
			break;
		default:
			swi_com_fatal_error( __FILE__ ":invalid case", __LINE__);
			break;	
	}
	E_DEBUG("LEAVING");
	return ret;
}

int
swi_update_current_context(SWI * swi, SWPATH_EX * swpath_ex)
{
	return 0;
}

int
swi_add_swpath_ex(SWI * swi)
{
	CPLOB * list = swi->swi_pkgM->exlistM;
	SWPATH * swpath = swi->swpathM;
	SWPATH_EX * swpath_ex;

	E_DEBUG("ENTERING");
	swi->swi_pkgM->prev_swpath_exM = swi_get_last_swpath_ex(swi);

	swpath_ex = swpath_create_export(swpath);
	swi_com_assert_pointer((void*)swpath_ex, __FILE__, __LINE__);

	cplob_add_nta(list, (char*)(swpath_ex));
	E_DEBUG("LEAVING");
	return 0;	
}

int
swi_handle_control_transition(SWI * swi)
{
	SWI_PACKAGE * package = swi->swi_pkgM;
	SWPATH_EX * current;
	SWPATH_EX * previous;
	SWI_XFILE * fileset;
	SWI_XFILE * pfiles;
	int tret;
	int detect;
	int found_edge = 0;	

	E_DEBUG("ENTERING");
	previous = swi->swi_pkgM->prev_swpath_exM;
	current = swi_get_last_swpath_ex(swi);
	
	E_DEBUG("ENTERING");

        #ifdef SWSWINEEDDEBUG
	{
                STROB * tmp = strob_open(100);
		E_DEBUG2("Previous SWPATH_EX:\n %s\n",
			swpath_ex_print(previous, tmp, "previous"));
		E_DEBUG2("Current  SWPATH_EX:\n %s\n",
			swpath_ex_print(current, tmp, "current"));
                strob_close(tmp);
	}
	#endif

	/* SWPATH_EX
	 * int is_catalog;	
	 * int ctl_depth;
	 * char * pkgpathname;
	 * char * prepath;
	 * char * dfiles;
	 * char * pfiles;
	 * char * product_control_dir;
	 * char * fileset_control_dir;
	 * char * pathname;	
	 * char * basename;
	 */

	if (!previous) {
		/*
		* FIXME, Free this.
		*/
		previous = swpath_create_export((SWPATH*)NULL);
	}

	detect = swi_com_field_edge_detect(current->dfiles,  previous->dfiles);
	if (found_edge == 0 && detect) {
		/*
		 * New dfiles.
		 */
		E_DEBUG("New dfiles");
		package->dfilesM = swi_xfile_create(SWI_XFILE_TYPE_DFILES,
					SWI_INDEX_HEADER(swi), current);
		package->current_xfileM = package->dfilesM;
		package->dfilesM->package_pathM = strdup(current->pkgpathname);
		E_DEBUG2("Setting Current XFILE (dfiles) [%p]",
					(void*)(package->current_xfileM));
		found_edge = 1;
	}

	detect = swi_com_field_edge_detect(current->product_control_dir, 
					previous->product_control_dir);
	if (found_edge == 0 && detect) {
		/*
		 * New product.
		 */
		E_DEBUG("New Product");
		package->current_productM = swi_product_create(SWI_INDEX_HEADER(swi), current);
		swi_package_add_product(package, package->current_productM);
		package->current_xfileM = NULL;
		package->current_productM->package_pathM = 
					strdup(current->pkgpathname);
		if ((tret=need_new_pfiles(swi)) > 0) {
			/*
			 * This happens in packages without directories in 
			 * the archive.
			 * e.g.
			 *	...
			 *	catalog/dfiles/INFO
			 *	catalog/prod1/pfiles/INFO
			 *	...
			 */
			pfiles = swi_xfile_create(SWI_XFILE_TYPE_FILESET, SWI_INDEX_HEADER(swi), current);
			pfiles->package_pathM = strdup(current->pkgpathname);
			package->current_productM->xfileM = pfiles;
			package->current_xfileM = pfiles;
		} else if (tret < 0) {
			SWI_internal_error();
			return -1;
		} else {
			;
		}
		E_DEBUG2("Setting Current Product [%p]",
				(void*)(package->current_productM));
		E_DEBUG2("Setting Current XFILE (NULL) [%p]",
					(void*)(package->current_xfileM));
		found_edge = 1;
	}

	detect = swi_com_field_edge_detect_fileset(current, previous);
	if (found_edge == 0 && detect) {
		/*
		 * New fileset.
		 */
		E_DEBUG("New Fileset");
		fileset = swi_xfile_create(SWI_XFILE_TYPE_FILESET, SWI_INDEX_HEADER(swi), current);
		swi_product_add_fileset(package->current_productM, fileset);
		package->current_xfileM = fileset;
		package->current_xfileM->package_pathM = 
						strdup(current->pkgpathname);
		E_DEBUG2("        Current Product [%p]",
				(void*)(package->current_productM));
		E_DEBUG2("Setting Current XFILE (fileset) [%p]",
					(void*)(package->current_xfileM));
		found_edge = 1;
	}

	detect = swi_com_field_edge_detect(current->pfiles, previous->pfiles);
	if (found_edge == 0 && detect) {
		/*
		 * New pfiles.
		 */
		E_DEBUG("New pfiles");
		pfiles = swi_xfile_create(SWI_XFILE_TYPE_PFILES, SWI_INDEX_HEADER(swi), current);
		if (package->current_productM == NULL) {
			/*
			 * This happens for minimal package layout.
			 */
			package->current_productM = swi_product_create(SWI_INDEX_HEADER(swi), current);
			swi_package_add_product(package, 
						package->current_productM);
			package->current_xfileM = NULL;
		}
		pfiles->package_pathM = strdup(current->pkgpathname);
		package->current_productM->xfileM = pfiles;
		package->current_xfileM = pfiles;
		E_DEBUG2("   Current Product [%p]",
				(void*)(package->current_productM));
		E_DEBUG2("Setting Current XFILE (pfiles) [%p]",
					(void*)(package->current_xfileM));
		found_edge = 1;
	}

	E_DEBUG2("LEAVING return value = [%d]", found_edge);
	return found_edge;
}

int
swi_decode_catalog(SWI * swi)
{
	XFORMAT * xformat = swi->xformatM;
	SWPATH * swpath = swi->swpathM;

	STROB * tmp = strob_open(24);
	struct stat st;
	int nullfd;
	int copyret = 0;
	int ret;
	int handle_ret;
	int swpath_ret;
	int retval;
	int is_catalog;
	char * name;
	int swdef_ret = 0;
	int swdeffile_type;
	int ifd;
	int offset = 0;
	int oldoffset = 0;
	int catalog_start_offset = -1;
	int catalog_end_offset = 0;

	retval = 0;

	nullfd = swi->nullfdM;
	if (nullfd < 0) return -22;		

	ifd = xformat_get_ifd(xformat);
	name = xformat_get_next_dirent(xformat, &st);
	if (ifd >= 0) {
		offset = uxfio_lseek(ifd, (off_t)0, SEEK_CUR);	
	}

	while(name) {
		E_DEBUG2("ENTERING LOOP: name=[%s]", name);
		if (xformat_is_end_of_archive(xformat)){
			/* 
			 * error 
			 */
			E_DEBUG("EOA found before storage section: break");
			retval = -1;
			break;
		}

		/*
		 * Check for absolute path, and for shell metacharacters
		 * characters in the pathname.
		 */
		swi_com_check_clean_relative_path(name);
		E_DEBUG2("name=[%s]", name);

		swpath_ret = swpath_parse_path(swpath, name);
		if (swpath_ret < 0) {
			E_DEBUG("swpath_parse_path error");
			fprintf(stderr, "%s: error parsing pathname [%s]\n", swlib_utilname_get(), name);
			retval = -2;
			break;
		}

		is_catalog = swpath_get_is_catalog(swpath);
		if (is_catalog != SWPATH_CTYPE_STORE) {
			/*
			 * Either -1 leading dir, or 1 catalog.
			 */
			if (is_catalog == 1 && catalog_start_offset < 0) {
				E_DEBUG("");
				catalog_start_offset = offset;
				swi->swi_pkgM->catalog_start_offsetM = offset;
				/*
				fprintf(stderr,
				"eraseme START ---OFFSET=%d %d\n",
					offset,
				xformat->swvarfsM->current_header_offsetM); 
				*/
			}
			
			if (swi->exported_catalog_prefixM == NULL && is_catalog == 1) {
				/*
				 * fill in the swi->exported_catalog_prefixM  member.
				 * This is how the catalog directory is
				 * specified when being unpacked (by tar)
				 * and deleted.
				 */
				E_DEBUG("");
				strob_strcpy(tmp, swpath_get_prepath(swpath));
				swlib_unix_dircat(tmp, SW_A_catalog);
				swi->exported_catalog_prefixM = strdup(strob_str(tmp));
				swi_com_check_clean_relative_path(strob_str(tmp));
			}

			swi_add_swpath_ex(swi);
			
			handle_ret = swi_handle_control_transition(swi);
			if (handle_ret < 0) {	
				/*
				 * error
				 */	
				E_DEBUG("");
				swi_com_assert_pointer((void*)NULL, 
						__FILE__, __LINE__);
			}
 
			if (swi_is_definition_file(swi, name, 
							&swdeffile_type)) 
			{
				/*
				 * parse the IEEE 1387.2 software definition file
				 */
				E_DEBUG("");
				swdef_ret = swi_parse_file(swi, name, 
							swdeffile_type);
				E_DEBUG("");
				if (swdef_ret) {
					fprintf(stderr, "error parsing %s\n",
							name);
					retval = -3;
					break;
				}
				E_DEBUG("");
				copyret = 0;	
			} else if (xformat_file_has_data(xformat)) {
				/*
				 * Store the file data.
				 */
				E_DEBUG("");
				swi_store_file(swi, name,
					swi->swi_pkgM->current_xfileM);
			} else {
				/*
				 * No data
				 */
				E_DEBUG("");
				copyret = 0;	
				;
			}
			if (copyret < 0) {
				/*
				 * error
				 */
				E_DEBUG("");
				SWI_internal_error();
				retval = -4;
				break;
			}
			E_DEBUG("");
			catalog_end_offset = uxfio_lseek(ifd, (off_t)0, SEEK_CUR);	
			swi->swi_pkgM->catalog_end_offsetM = catalog_end_offset;
			E_DEBUG2("catalog end offset = [%d]", swi->swi_pkgM->catalog_end_offsetM);
		} else {
			/*
			 * Good.
			 * Finished, read past catalog section
			 * This is the first file of the storage section.
			 */
			E_DEBUG("STORAGE SECTION found: break");
			swi->does_have_payloadM = 1;
			break;
		}

		E_DEBUG("");
		oldoffset = offset;
		name = xformat_get_next_dirent(xformat, &st);
		offset = uxfio_lseek(ifd, (off_t)0, SEEK_CUR);
		E_DEBUG2("NAME=[%s]", name?name:"<NIL>");
	}

	E_DEBUG("");
	if (retval < 0) {
		E_DEBUG2("ERROR: retval = [%d]", retval);
		return retval;
	}
	
	E_DEBUG("");
	if (swi->does_have_payloadM < 0 /* unset */) {
		/* Must not have a payload such as the
		   catalog.tar file from the installed_software_catalog
		   or a package with no files */

		swi->does_have_payloadM = 0;

		/* also we must set the swi->swi_pkgM->catalog_end_offsetM value
		 * which is wrong at the present
		 */

		catalog_end_offset = uxfio_lseek(ifd, (off_t)0, SEEK_CUR);	
		swi->swi_pkgM->catalog_end_offsetM = catalog_end_offset /* - xformat->eoaM */;
		E_DEBUG2("NO PAYLOAD: NOW catalog end offset = [%d]", swi->swi_pkgM->catalog_end_offsetM);
		E_DEBUG2("EOA bytes = [%d]", xformat->eoaM);
	}

	E_DEBUG("");
	/* sanity check */
	if (catalog_start_offset < 0) {
		catalog_start_offset = 0;
		fprintf(stderr, "internal error: %s:%d\n",
			__FILE__, __LINE__);
	}

	E_DEBUG("");
	swi->swi_pkgM->catalog_lengthM = 
		catalog_end_offset - catalog_start_offset; 

	/* Now fix-up the control scripts that share
	   a common control file.  */
	E_DEBUG("");

	if (
		swi->swi_pkgM == NULL
		/*
		swi->swi_pkgM->swi_coM[0] == NULL ||
		swi->swi_pkgM->swi_coM[0]->xfileM == NULL
		*/
	) {
		return -8;
	}


	if (swi->swi_pkgM && swi->swi_pkgM->swi_coM[0]) {
		ret = swi_expand_shared_file_control_sripts(swi, swi->swi_pkgM->swi_coM[0]->xfileM);
	} else {
		/* This is an internal error and should never happen.
		*/
		fprintf(stderr, "%s: internal error swi->swi_pkgM && swi->swi_pkgM->swi_coM[0]\n", swlib_utilname_get());
		ret = 0;
	}

	E_DEBUG("");
	swi_com_assert_value(ret == 0, __FILE__, __LINE__);

	E_DEBUG("");

	swi_recurse_swi_tree(swi, (char *)(NULL), swi_vbase_set_verbose_level, (void*)(&(swi->verboseM)));
	E_DEBUG("");
	swi_recurse_swi_tree(swi, (char *)(NULL), swi_vbase_update, NULL);
	E_DEBUG("");
	ret = swi_recurse_swi_tree(swi, (char *)(NULL), swi_vbase_generate_swverid, NULL);
	if (ret) {
		if (retval == 0) retval = -1;
	}
	E_DEBUG("");
	swi->swi_pkgM->is_minimal_layoutM = swpath_get_is_minimal_layout(swpath);
	strob_close(tmp);
	E_DEBUG("");
	return retval;
}

SWI_PRODUCT *
swi_package_get_product(SWI_PACKAGE * swi_pkg, int index)
{
	SWI_PRODUCT * ret;
	ret = swi_pkg->swi_coM[index];
	return ret;
}


SWI_XFILE *
swi_product_get_fileset(SWI_PRODUCT * swi_prod, int index)
{
	SWI_XFILE * ret;
	ret = swi_prod->swi_coM[index];
	return ret;
}

int
swi_product_has_control_file(SWI_PRODUCT * swi_prod, char * control_file_name)
{
	int ret;
	ret = swi_xfile_has_posix_control_file(swi_prod->xfileM,
		control_file_name);
	return ret;
}

SWI_CONTROL_SCRIPT *
swi_product_get_control_script_by_tag(SWI_PRODUCT * prod, char * tag)
{
	SWI_CONTROL_SCRIPT * ret; 
	ret = swi_xfile_get_control_script_by_tag(prod->xfileM, tag);
	return ret;
}

SWI_CONTROL_SCRIPT *
swi_get_control_script_by_swsel(SWI * swi, char * swsel, char * script_tag)
{
	/*
	 * 'swsel' *      is software selection
	 */



	return NULL;
}

int
swi_recurse_swi_tree(SWI * swi, char * sw_selections, int (*payload)(void*, void*), void * uptr)
{
	int i;
	int j;
	SWI_PACKAGE * package;
	SWI_PRODUCT * product;
	SWI_XFILE * fileset;
	int ret;
	int retval = 0;

	package = swi->swi_pkgM;
	for(i=0; i<SWI_MAX_OBJ; i++) {
		product = package->swi_coM[i];
		if (product) {
			if (payload) {
				ret = (*payload)((void*)(product), uptr);
				if (ret) retval++;
			}
			for(j=0; j<SWI_MAX_OBJ; j++) {
				fileset = product->swi_coM[j];
				if (fileset) {
					if (payload) {
						ret = (*payload)((void*)(fileset), uptr);
						if (ret) retval++;
					}
				} else break;
			}
		} else break;
	}
	return retval;
}

SWI_PRODUCT *
swi_find_product_by_swsel(SWI_PACKAGE * parent, char * swsel, char * number, int * p_index)
{
	SWI_BASE * base;
	base = find_object_by_swsel(parent, swsel, number, p_index);
	if (!base) return (SWI_PRODUCT*)base;
	swi_base_assert(base);
	SWLIB_ASSERT(base->type_idM == SWI_I_TYPE_PROD); 	
	return (SWI_PRODUCT*)(base);
}

SWI_XFILE *
swi_find_fileset_by_swsel(SWI_PRODUCT *  parent, char * swsel, int * p_index)
{
	SWI_BASE * base;
	base = find_object_by_swsel(parent, swsel, NULL, p_index);
	if (!base) return (SWI_XFILE*)base;
	swi_base_assert(base);
	SWLIB_ASSERT(base->type_idM == SWI_I_TYPE_XFILE); 	
	SWLIB_ASSERT(
		((SWI_XFILE*)base)->typeM == SWI_XFILE_TYPE_FILESET ||
		((SWI_XFILE*)base)->typeM == SWI_XFILE_TYPE_PFILES 
		); 	
	return (SWI_XFILE*)(base);
}

void
swi_get_uts_attributes_from_current(SWI * swi, SWUTS * uts, SWHEADER * swheader)
{
	char * value;	

	value = swheader_get_single_attribute_value(swheader, SW_A_os_name);
	if (value) {
		swuts_add_attribute(uts, SW_A_os_name, value);
	} else {
		swuts_add_attribute(uts, SW_A_os_name, "");
	}	

	value = swheader_get_single_attribute_value(swheader, SW_A_os_version);
	if (value) {
		swuts_add_attribute(uts, SW_A_os_version, value);
	} else {
		swuts_add_attribute(uts, SW_A_os_version, "");
	}	

	value = swheader_get_single_attribute_value(swheader, SW_A_os_release);
	if (value) {
		swuts_add_attribute(uts, SW_A_os_release, value);
	} else {
		swuts_add_attribute(uts, SW_A_os_release, "");
	}	

	value = swheader_get_single_attribute_value(swheader, SW_A_machine_type);
	if (value) {
		swuts_add_attribute(uts, SW_A_machine_type, value);
	} else {
		swuts_add_attribute(uts, SW_A_machine_type, "");
	}	
	return;
}

int
swi_do_decode(SWI * swi, SWLOG * swutil,
	int target_fd1,
	int source_fd0,
	char * target_path,
	char * source_path,
	VPLOB * swspecs,
	char * target_host,
	struct extendedOptions * opta,
	int is_seekable,
	int do_debug_events,
	int verboseG,
	struct sw_logspec * g_logspec,
	int uinfile_open_flags
	)
{
	int ret;
	int ifd;
	int retval;
	XFORMAT * xformat;
	SWPATH * swpath;
	UINFORMAT * uinformat;
	SWVARFS * swvarfs;
	char * rpm_newname;
	int format;
	int flags;

	E_DEBUG("");
	SWLIB_ASSERT(source_fd0 >= 0);
	swlib_doif_writef(verboseG, SWC_VERBOSE_IDB,
					g_logspec, swutil->swu_efdM, 
					"run audit: start setup_xformat\n");

	format = arf_ustar;

	/* Create the xformat object */
        xformat = xformat_open( -1 /*STDIN_FILENO*/  , -1 /* target_fd1 */, format);

	/* FIXME  the open policy described by flags may should be
	   forced up a level so the top level utility can
	   control the policy */

	E_DEBUG("");
	if (uinfile_open_flags < 0) {
		flags = UINFILE_DETECT_FORCEUXFIOFD |
			UINFILE_DETECT_UNCPIO |
			UINFILE_DETECT_IEEE;
	} else {
		flags = uinfile_open_flags;
	}
	
	/* Now, open the package */

	E_DEBUG("");
	xformat = swutil_setup_xformat(swutil,
			xformat,
			source_fd0,
			source_path,
			opta,
			is_seekable,
			verboseG,
			g_logspec,
			flags);

	if (xformat == (XFORMAT*)NULL) {
		E_DEBUG("error");
		return -1;
	}
	E_DEBUG("");
	
	/* The package is now opened and partially decoded */

	swlib_doif_writef(verboseG, SWC_VERBOSE_IDB2, g_logspec, swutil->swu_efdM, 
				"run audit: finished setup_xformat\n");

	xformat_set_ofd(xformat, target_fd1);

	swlib_doif_writef(verboseG, SWC_VERBOSE_IDB2, g_logspec, swutil->swu_efdM, 
			"run audit: starting swlib_audit_distribution\n");

	swvarfs = xformat_get_swvarfs(xformat);
	uinformat = swvarfs_get_uinformat(swvarfs);
	swpath = uinfile_get_swpath(uinformat);
        swpath_reset(swpath);

	xformat_set_tarheader_flag(xformat, TARU_TAR_FRAGILE_FORMAT, 1);
	xformat_set_tarheader_flag(xformat, TARU_TAR_RETAIN_HEADER_IDS, 1);

	/*
	 * If translating an RPM, don't install the RPMFILE_CONFIG files
	 */
	if (uinfile_get_ztype(uinformat) == UINFILE_COMPRESSED_RPM) {
		/*
		 * we'er installing an RPM, yeah!!! ;)
		 */
		if (opta) {
			set_opta(opta, SW_E_swbis_install_volatile, "true");
			rpm_newname = get_opta(opta, SW_E_swbis_volatile_newname);
			if (
				swextopt_is_option_set(SW_E_swbis_volatile_newname, opta) == 0 &&
				(!rpm_newname || !strlen(rpm_newname))
			) {
				set_opta(opta, SW_E_swbis_volatile_newname, ".rpmnew");
			} else {
				;
				/*
				 * Use the one the user set.
				 */
			}
		}
	}

	swi->xformatM = xformat; 
	swi->swvarfsM = swvarfs; 
	swi->uinformatM = uinformat;
	swi->swpathM = swpath;   
	swi->verboseM = verboseG;
	if (target_host) {
		swi->swi_pkgM->target_hostM = strdup(target_host);
	} else {
		swi->swi_pkgM->target_hostM = strdup("localhost");
	}	
	ifd = xformat_get_ifd(xformat);	
	swi->optaM = (void*)opta;
	swi->debug_eventsM = do_debug_events;
	
	/*
	 * Decode the catalog section.
	 */
	ret = swi_decode_catalog(swi);
	E_DEBUG("");

	/*
	 * Set the auto disable of the memory cache.
	 * This will turn off memory cache as soon as the
	 * file advances beyond the previous farthest point
	 */
	E_DEBUG("");
	swvarfs_uxfio_fcntl(swvarfs, UXFIO_F_ARM_AUTO_DISABLE, 1);
	E_DEBUG("");

	/*
	 * Now seek back to the beginning of the package.
	 */
	E_DEBUG("");
	if (uxfio_lseek(ifd, 0, SEEK_SET) != 0) {
		fprintf(stderr, "uxfio_lseek error: %s : %d\n", __FILE__, __LINE__);
		exit(1);
	}

	E_DEBUG("");
        swpath_reset(swpath);
	if (ret) {
		/*
		* error, swi_decode_catalog failed
		*/
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, g_logspec, swutil->swu_efdM, 
				"swi_decode_catalog: return code %d\n", ret);
		retval = -1;
		swi = (SWI*)(NULL);
	} else {
		retval = 0;
	}
	E_DEBUG("");
	return retval;
}

int
swi_find_compatible_product(SWI * swi, SWUTS * target_uts, int disallow_incompat)
{
	SWI_PRODUCT * product;
	SWHEADER * global_index;
	SWUTS * product_uts;
	int check_found;
	int retval;
	int ret;
	int product_number;
	int n_matches;
	int n_products;

	n_matches = swi_audit_all_compatible_products(swi, target_uts, &n_products, (int*)NULL);

	if (disallow_incompat == 0 &&
	    n_matches == 0 &&
	    n_products == 1)
		return 0;  /* index of first product */

	if (n_matches == 1 && n_products == 1)
		return 0;  /* index of first product */

	check_found = 0;
	product_number = 0;
	while ((product=swi_package_get_product(swi->swi_pkgM, product_number))) {
		if (product->is_compatibleM == SW_TRUE) {
			check_found = 1;
			break;
		}
		product_number++;
	}

	if (product_number >= n_products)
		return -1;

	return product_number;
}

SWHEADER *
swi_get_global_index_header(SWI * swi)
{
	SWHEADER * ret;
	ret = swi->swi_pkgM->baseM.global_headerM;
	return ret;
}

SWHEADER *
swi_get_fileset_info_header(SWI * swi, int product_index, int fileset_index)
{
	SWHEADER * ret;
	ret = swi->swi_pkgM->swi_coM[product_index]->swi_coM[fileset_index]->info_headerM;
	return ret;
}

SWI_FILELIST *
swi_make_fileset_file_list(SWI * swi, int product_num, int fileset_num, char * location)
{
	int ret;
	char * object_line;
	char * object_keyword;
	char * directory;
	char * path;
	char * prefix;
	char * store;
	char * filetype;
	char * is_volatile;
	int tar_filetype;
	STROB * tmp;
	STROB * relocated_path;
	SWI_FILELIST * flist;
	SWI_PRODUCT * product;
	SWHEADER * infoheader;
	SWHEADER * global_index;
	SWHEADER_STATE state;

	E_DEBUG("");
	flist = swi_fl_create();
	tmp = strob_open(12);
	relocated_path = strob_open(12);

	if (swi == NULL) {
		SWLIB_ERROR("");
		return NULL;
	}

	E_DEBUG("");

	/* Get the product */
	product = swi_package_get_product(swi->swi_pkgM, product_num);
	if (product == NULL) {
		SWLIB_ERROR("");
		return NULL;
	}

	E_DEBUG("");

	/* Get the INFO file for the fileset */
	infoheader = swi_get_fileset_info_header(swi, product_num, fileset_num);
	if (infoheader == NULL) {
		SWLIB_ERROR("");
		return NULL;
	}

	global_index = swi_get_global_index_header(swi);
	swheader_set_current_offset(global_index, product->p_baseM.header_indexM);
	directory = swheader_get_single_attribute_value(global_index, SW_A_directory);
	directory = swlib_attribute_check_default(SW_A_product, SW_A_directory, directory);
	
	E_DEBUG("");
	swheader_store_state(infoheader, &state);
	swheader_reset(infoheader);

	E_DEBUG("");
	while ((object_line = swheader_get_next_object(infoheader, (int)(UCHAR_MAX), (int)UCHAR_MAX)) != NULL) {
		E_DEBUG("---------------------------------------------------------------");
		if (swheaderline_get_type(object_line) != SWPARSE_MD_TYPE_OBJ) {
			SWLIB_ERROR("");
			return NULL;
		}
		object_keyword = swheaderline_get_keyword(object_line);		
		if (strcmp(object_keyword, SW_A_control_file) == 0) {
			continue;
		}
		path = swheader_get_single_attribute_value(infoheader, SW_A_path);
		if (path == NULL || strlen(path) == 0) {
			SWLIB_ERROR("unexpected null path");
		}
		/* need to check if its a volatile file */
		E_DEBUG2("location=[%s]", location);
		E_DEBUG2("path=[%s]", path);
		E_DEBUG2("directory=[%s]", directory);

		/* apply relocation */
		if (strcmp(location, "/") != 0) {
			E_DEBUG2("APPLY LOCATION:  path=[%s]", path);
			swlib_apply_location(relocated_path, path, location, directory);
			path =  strob_str(relocated_path);
			E_DEBUG2("AFTER path=[%s]", path);
		}
		/* SWLIB_ERROR2("path=[%s]", path); */

		/*
		 * Impose the every pathname begins with ''./''
		 */
		if (*path == '.' && *(path+1) == '/') {
			prefix = "";
		} else {
			E_DEBUG2("path=[%s]", path);
			path = swlib_return_relative_path(path);
			E_DEBUG2("path=[%s]", path);
			prefix = "./";
		}
		E_DEBUG2("path=[%s]", path);
		store = swi_fl_add_store(flist, strlen(path)+strlen(prefix));
		swi_fl_set_path(flist, store, prefix, path);
		E_DEBUG2("path=[%s]", path);
		
		/* Set the file type */
		filetype = swheader_get_single_attribute_value(infoheader, SW_A_type);
		if (filetype == NULL || strlen(filetype) == 0) {
			filetype = "0"; /* REGTYPE */
			SWLIB_ERROR("unexpected error, type attribute not present");
		}
		tar_filetype = swheader_getTarTypeFromTypeAttribute(*filetype);
		swi_fl_set_type(flist, store, tar_filetype);

		/* Set the volatile flag */
		is_volatile = swheader_get_single_attribute_value(infoheader, SW_A_is_volatile);
		ret = swlib_is_option_true(is_volatile);
		swi_fl_set_is_volatile(flist, store, -1, ret);
	}
	E_DEBUG("");
out:
	swheader_restore_state(infoheader, &state);
	strob_close(tmp);
	strob_close(relocated_path);
	return flist;
}


SWI_FILELIST *
swi_make_file_list(SWI * swi, SWUTS * swuts, char * location, int disallow_incompatible)
{
	int n_compat;
	int check_found;
	int product_number;
	int n_products;
	STROB * relocated_path;
	SWI_FILELIST * flist;
	SWI_PRODUCT * product;

	E_DEBUG("");
	flist = swi_fl_create();
	relocated_path = strob_open(12);

	if (swi == NULL) {
		SWLIB_ERROR("");
		return NULL;
	}

	E_DEBUG("");
	product_number = swi_find_compatible_product(swi, swuts, disallow_incompatible);

	if (product_number < 0) {
		/* SWLIB_INTERNAL("no compatible product found in entry"); */
		SWLIB_WARN("no compatible product found in entry");
		return NULL;
		;
	}
	E_DEBUG("");

	/* Ok, if we've made it this far the product given by
	   product_number is the correct product for this installation
	   and we have the location from the catalog and we have the
	   decoded catalog.tar file in the (SWI) object.

	   we can now combine them and produce the installed file list */

	flist = swi_make_fileset_file_list(swi, product_number, SWI_FILESET_0, location);
	return flist;
}

SWI_FILELIST *
swi_fl_create(void) {
	return (SWI_FILELIST *)strar_open();	
}

void
swi_fl_delete(SWI_FILELIST * fl) {
	strar_close((STRAR*)fl);
}

char *
swi_fl_add_store(SWI_FILELIST * fl, int len) {
	STRAR * ar = (STRAR*)fl;
	char * s;
	s = strar_return_store(ar, len + SWI_FL_HEADER_LEN);
	return s;	
}

void
swi_fl_set_path(SWI_FILELIST * fl, char * s,  char * prefix, char * path) {
	E_DEBUG2("path=[%s]", path);
	E_DEBUG2("prefix=[%s]", prefix);
	if (prefix == NULL) prefix = "";
	if (strlen(prefix) > 0) {
		strncpy(s + (SWI_FL_HEADER_LEN), prefix, strlen(prefix));
		E_DEBUG2("s=[%s]", s);
	}
	strncpy(s + (SWI_FL_HEADER_LEN + strlen(prefix)), path, strlen(path));
	E_DEBUG2("s=[%s]", s);
		/* NUL is added by strar_return_store() it must
		   be the only terminating NUL for the record otherwise
		   the STRAR object is corrupted. */
}

void
swi_fl_set_user_flag(SWI_FILELIST * fl, char * hdr, int msg) {
	*(hdr + SWI_FL_OFFSET_USER_FLAG) = msg;
}

int
swi_fl_get_user_flag(SWI_FILELIST * fl, int index) {
	STRAR * ar = (STRAR*)fl;
	char * s;
	s = strar_get(ar, index);
	if (s == NULL) return 0;
	return (int)(*(s + SWI_FL_OFFSET_USER_FLAG));
}

void
swi_fl_set_is_volatile(SWI_FILELIST * fl, char * hdr, int index, int is_vol) {
	if (hdr == NULL) {
		STRAR * ar = (STRAR*)fl;
		char * s;
		s = strar_get(ar, index);
		if (s == NULL) return;
		hdr = s;
	}
	if (is_vol)
		*(hdr + SWI_FL_OFFSET_IS_VOLATILE) = 'T';
	else
		*(hdr + SWI_FL_OFFSET_IS_VOLATILE) = 'F';
}

void
swi_fl_set_type(SWI_FILELIST * fl, char * hdr, int tartype) {
	*(hdr + SWI_FL_OFFSET_TARTYPE) = (unsigned char)tartype;
}

int
swi_fl_get_type(SWI_FILELIST * fl, int index) {
	STRAR * ar = (STRAR*)fl;
	char * s;
	s = strar_get(ar, index);
	if (s == NULL) return 0;
	return (int)(*(s + SWI_FL_OFFSET_TARTYPE));
}


char *
swi_fl_get_path(SWI_FILELIST * fl, int index) {
	STRAR * ar = (STRAR*)fl;
	char * s;
	s = strar_get(ar, index);
	if (s == NULL) return NULL;
	return s + SWI_FL_OFFSET_PATH;
}

int
swi_fl_is_volatile(SWI_FILELIST * fl, int index) {
	STRAR * ar = (STRAR*)fl;
	char * s;
	s = strar_get(ar, index);
	if (s == NULL) return 0;
	if ((int)(*(s + SWI_FL_OFFSET_IS_VOLATILE)) == 'T') {
		return 1;
	} else {
		return 0;
	}
}

int
swi_fl_compare_reverse(const void * vf1, const void * vf2)
{
	return -swi_fl_compare(vf1, vf2);
}

int
swi_fl_compare(const void * vf1, const void * vf2)
{
	char * f1;
	char * f2;
	f1 = *((char**)(vf1)); 	
	f2 = *((char**)(vf2)); 	
	return strcmp(f1 + SWI_FL_HEADER_LEN, f2 + SWI_FL_HEADER_LEN);
}

void
swi_fl_qsort_forward(SWI_FILELIST * fl) {
	STRAR * ar = (STRAR*)fl;
	strar_qsort(ar, swi_fl_compare);
}

void
swi_fl_qsort_reverse(SWI_FILELIST * fl) {
	STRAR * ar = (STRAR*)fl;
	strar_qsort(ar, swi_fl_compare_reverse);
}
