/* swi.h -  Posix package decoding.
   Copyright (C) 2005  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef swi_h_20030523
#define swi_h_20030523

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "uxfio.h"
#include "strob.h"
#include "swvarfs.h"
#include "hllist.h"
#include "defer.h"
#include "porinode.h"
#include "ahs.h"
#include "taru.h"
#include "taruib.h"
#include "uinfile.h"
#include "swheader.h"
#include "swheaderline.h"
#include "strar.h"
#include "swicol.h"
#include "swi_base.h"
#include "swi_common.h"
#include "swi_afile.h"
#include "swi_xfile.h"
#include "swverid.h"


#define SWI_GET_PRODUCT(W, N) (swi_package_get_product(W->swi_pkgM, N))
#define SWI_INDEX_HEADER(W) (W->swi_pkgM->baseM.global_headerM)

#define SWI_FL_TRUE			'T'
#define SWI_FL_FALSE			'F'
#define SWI_FL_OFFSET_TARTYPE		0
#define SWI_FL_OFFSET_IS_VOLATILE	1
#define SWI_FL_OFFSET_USER_FLAG		2
#define SWI_FL_OFFSET_KEEP_FLAG		3
#define SWI_FL_OFFSET_PATH		10
#define SWI_FL_HEADER_LEN		10

typedef union {
	STRAR * arM;
} SWI_FILELIST;

SWI_FILELIST * swi_fl_create(void);
void swi_fl_delete(SWI_FILELIST *);
char * swi_fl_add_store(SWI_FILELIST * fl, int len);
void  swi_fl_set_path(SWI_FILELIST*, char * hdr, char * prefix, char * path);
void  swi_fl_set_is_volatile(SWI_FILELIST*, char * hdr, int index, int is_volatile);
void  swi_fl_set_type(SWI_FILELIST*, char * hdr, int );
void  swi_fl_set_user_flag(SWI_FILELIST*, char * hdr, int );
int swi_fl_get_user_flag(SWI_FILELIST*, int );
char * swi_fl_get_path(SWI_FILELIST*, int index);
int swi_fl_is_volatile(SWI_FILELIST*, int index);
int swi_fl_get_type(SWI_FILELIST*, int index);
int swi_fl_compare(const void * f1, const void * f2);
int swi_fl_compare_reverse(const void * f1, const void * f2);
void swi_fl_qsort_reverse(SWI_FILELIST * fl);
void swi_fl_qsort_forward(SWI_FILELIST * fl);

typedef struct {		/* the Bundle structure			*/
	char type_idM;		/* 'B'					*/
	int is_activeM;		
	char * tagM;		/* the bundle tag			*/
	time_t create_timeM;
	time_t mod_timeM;
} SWI_BUNDLE;

typedef struct {		/* the Product structure		*/
	SWI_BASE p_baseM;
	SWI_XFILE * xfileM;	/* Product pfiles			*/
	SWI_XFILE * swi_coM[SWI_MAX_OBJ];  /* list of filesets  	*/
	char * package_pathM;	/* The package path for this object	*/
	char * control_dirM;	/* the product control dir		*/
	STRAR * filesetsM;	/* list of installed filesets		*/
	int is_selectedM;	/* is selected				*/
	int is_compatibleM;	/* is compatible			*/
	int INDEX_ordinalM;     /* Order in INDEX file.  0 is first     */      
} SWI_PRODUCT;

typedef struct {			/* The global package structure	*/
	SWI_BASE baseM;

	SWI_PRODUCT * swi_coM[SWI_MAX_OBJ];	/* List of products	*/
	SWI_SCRIPTS * swi_scM;		/* control scripts 		*/
	CPLOB 	  * exlistM;		/* list of SWPATH_EX objects 	*/ 
	int swdef_fdM;			/* INDEX file uxfio fd		*/
	SWI_PRODUCT * current_productM;	/* current context		*/
	SWI_XFILE * dfilesM;		/* dfiles control object	*/
	SWI_XFILE * current_xfileM;	/* current context Xfile	*/
	int did_parse_def_fileM;	/* set to 0 or 1, re: INDEX file*/
	SWPATH_EX * prev_swpath_exM;    /* State variable, previous     */
	char * dfiles_attributeM;	/* dfiles attribute		*/
	char * pfiles_attributeM;	/* pfiles attribute		*/
	int catalog_start_offsetM;
	int catalog_end_offsetM;
	int catalog_lengthM;
	char * target_pathM;		/* target path */
	char * target_hostM;		/* target host */
	char * catalog_entryM;		/* location of catalog entry 	*/
					/* relative to  target_path	*/
	char * installed_software_catalogM; /* Extended option value	*/
	int is_minimal_layoutM;		/* does have empty control dirs */
	char * locationM;		/* location spec */
	char * qualifierM;		/* qualifier spec */
	char * installer_sigM;          /* gpg signature by swinstall */
	char * installed_catalog_ownerM; /* owner of installed_software_catalog */
	char * installed_catalog_groupM; /* group of installed_software_catalog */
	mode_t installed_catalog_modeM;	 /* mode of installed_software_catalog */
} SWI_PACKAGE;

typedef struct {
	int did_part1M;
	char * dist_tagM;
	char * dist_revisionM;
	STRAR * bundle_tagsM;
	STRAR * product_tagsM;
	STRAR * product_revisionsM;
	STRAR * vendor_tagsM;		/* product vendor tags */
	char * catalog_bundle_dir1M;	/* First tag in installed_software catalog */
} SWI_DISTDATA;

typedef struct {	/* The Global package control structure */
	XFORMAT   * xformatM;	/* Package format object. 		*/
	SWVARFS   * swvarfsM;	/* Package I/O implementation layer     */
	UINFORMAT * uinformatM;	/* Package opening/decoding  object	*/
	SWPATH    * swpathM;	/* Posix pathname parsing object 	*/
	SWI_PACKAGE * swi_pkgM;	/* Package contents object 		*/
	int nullfdM;		/* fd open on /dev/null			*/
	int verboseM;		/* Controls debugging messages 		*/
	char tarbufM[1024];
	SWI_DISTDATA * distdataM;
	int opt_alt_catalog_rootM; /* -r option				*/
	char * exported_catalog_prefixM; /* i.e.  <path>/catalog/ from  */
				/* the distribution 			*/
	int excluded_file_conflicts_fdM; /* List of SW_A_excluded_from_install */
	int replaced_file_conflicts_fdM; /* List of SW_A_replaced_by_install  */
	int pending_file_conflicts_fdM; /* */

	SWICOL * swicolM;
	STRAR * swevent_listM;
	void * optaM;		/* extended options structure		*/
	int swc_idM;		/* SWC_?   which utility		*/ 
	int debug_eventsM;      /*                                      */
	SWVERID * target_version_idM;  /* Version Id of the target host        */
	int does_have_payloadM; /* -1, 0 or 1, is there a payload? */
	int xformat_close_on_deleteM;
	int swvarfs_close_on_deleteM;
	int uinformat_close_on_deleteM;
} SWI;

void swi_base_set_is_active(SWI_BASE * base, int n);
void swi_distdata_initialize(SWI_DISTDATA * part1);

void swi_product_add_fileset(SWI_PRODUCT * thisis, SWI_XFILE * v);
void swi_package_add_product(SWI_PACKAGE * thisis, SWI_PRODUCT * v);

SWI * swi_create(void);
SWI_PACKAGE * swi_package_create(void);
SWI_CONTROL_SCRIPT  * swi_control_script_create(void);
/* SWI_XFILE * swi_xfile_create(void); */
SWI_PRODUCT * swi_product_create(SWHEADER * header, SWPATH_EX *);
SWI_FILE_MEMBER * swi_file_member_create(void);

void do_swi_preview(TARU * taru, struct new_cpio_header * file_hdr,
		char * tar_header_p, int header_len, time_t now);
void show_swicol_events(SWICOL * swicol);
void swi_package_delete(SWI_PACKAGE * s);
void swi_delete(SWI * s);
void swi_file_member_delete(SWI_FILE_MEMBER * s);
void swi_product_delete(SWI_PRODUCT * s);
void swi_distdata_delete(SWI_DISTDATA * s);
SWI_DISTDATA * swi_distdata_create(void);

int swi_decode_catalog(SWI * swi);
char * swi_new_fd_mem(int fd, int * datalen);
char * swi_get_fd_mem(int fd, int * datalen);
int swi_is_definition_file(SWI * swi, char * name, int * swdeffile_type_p);
int swi_add_swpath_ex(SWI * swi);
int swi_parse_file( SWI * swi, char * name, int swdeffile_type);
int swi_update_current_context(SWI * swi, SWPATH_EX * swpath_ex);
int swi_detect_control_transition(SWI * swi);
int swi_store_file(SWI * swi, char * name, SWI_XFILE *);
void swi_xfile_add_file_member(SWI * swi, SWI_XFILE * xfile, char * name, int fd);
int swi_close_memfd(int fd);
int swi_get_distribution_attributes(SWI * swi, SWHEADER * swheader);
void swi_xfile_add_control_script(SWI * swi, SWI_XFILE * xfile, char * name, int fd, char * tag);
int swi_distdata_resolve(SWI * swi, SWI_DISTDATA * part1, int);
char * swi_dump_string_s(SWI * xx, char * prefix);
char * swi_base_dump_string_s(SWI_BASE * xx, char * prefix);
int swi_examine_signature_blocks(SWI_XFILE * dfiles, int * sig_block_start, int * sig_block_end);
SWI_PRODUCT * swi_package_get_product(SWI_PACKAGE * swi_pkg, int arr_index);
SWI_XFILE * swi_package_get_fileset(SWI_PRODUCT * swi_prod, int arr_index);
SWI_XFILE * swi_product_get_fileset(SWI_PRODUCT * swi_prod, int arr_index);
int swi_product_has_control_file(SWI_PRODUCT * swi_prod, char * control_file_name);
SWI_CONTROL_SCRIPT * swi_product_get_control_script_by_tag(SWI_PRODUCT * prod, char * tag);
int swi_recurse_swi_tree(SWI * swi, char * sw_selections, int (*payload)(void*, void * uptr), void * uptr);
SWI_PRODUCT * swi_find_product_by_swsel(SWI_PACKAGE * parent, char * swsel, char * number, int *);
SWI_XFILE * swi_find_fileset_by_swsel(SWI_PRODUCT * parent, char * swsel, int *);
void swi_get_uts_attributes_from_current(SWI * swi, SWUTS * uts, SWHEADER * swheader);
void swi_set_utility_id(SWI * swi, int swc_u_id);

int swi_do_decode(SWI * swi, SWLOG * swutil,
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
	int uinfile_open_flags);

int swi_audit_all_compatible_products(SWI * swi, SWUTS * target_uts, int *pn_products, int * p_swi_index);
int swi_find_compatible_product(SWI * swi, SWUTS * target_uts, int);
SWHEADER * swi_get_global_index_header(SWI * swi);
SWHEADER * swi_get_fileset_info_header(SWI * swi, int product_index, int fileset_index);
SWI_FILELIST * swi_make_fileset_file_list(SWI * swi, int product_num, int fileset_num, char * location);
SWI_FILELIST * swi_make_file_list(SWI * swi, SWUTS * swuts, char * location, int disallow_incompatible);
#endif
