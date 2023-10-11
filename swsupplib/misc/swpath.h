/* swpath.h: parse a posix package pathname.
 *
 *  Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 *  This file may be copied under the terms of the GNU GPL.
 */

#ifndef swpath_h_2003a
#define swpath_h_2003a

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swsdflt.h"
#include "strob.h"
#include "strar.h"

#define SWPATH_PARSE_CA		22
#define SWPATH_CTYPE_DIR	-1  /* Leading dir */
#define SWPATH_CTYPE_CAT	1   /* catalog section */
#define SWPATH_CTYPE_STORE	0   /* storage section */

typedef struct 
{
    int swpath_is_catalog_;	/* 1 if in '/catalog/', 0 or -1 if not */ 
    int errorM;
    int control_path_nominal_depth_;	/* 2 for normal layout */
					/* 0 for degenerate layout */
					/* 1 is error, i.e. product or fileset
					 directory is empty,  Not supported. */
    int max_rel_n_;
    STROB * product_control_dir_;   /* These are set anew for each pathname. */
    STROB * fileset_control_dir_;
    STROB * pfiles_;
    STROB * dfiles_;
    STROB * open_pathname_;
    STROB * swpackage_pathname_;
    STROB * pathname_;
    STROB * buffer_;
    STROB * basename_;
    STROB * all_filesets_;  /* The all_filesets attribute from the INDEX file */
    
    char * pfiles_default;
    char * dfiles_default;

    STROB * prepath_;	/* Stores the prepath dir, one time and saves it. */
    STROB * p_pfiles_;	/* Stores the dfiles dir, one time and saves it.*/
    STROB * p_dfiles_;	/* Stores the pfiles dir, one time and saves it.*/

 		/* points to the respective value in the strob object */ 
    char * swpath_p_prepath_;
    char * swpath_p_dfiles_;
    char * swpath_p_pfiles_;
    char * swpath_e_dfiles_;    /* Explicit external specicification. */
    char * swpath_e_pfiles_;    /* Explicit external specicification. */
    char * swpath_p_fileset_dir_;
    char * swpath_p_product_dir_;

    STRAR * pathlist_;
    int is_minimal_layoutM;
    int dfiles_guardM;	/* Guards against multiple dfiles directories */
    int catalog_guardM;	/* Guards against multiple catalogs */
} SWPATH;

typedef struct 
{
	 int is_catalog;		/* 1 is catalog section, 0 is not. */
	 int is_minimal_layoutM;
	 int ctl_depth;
	 char * pkgpathname;		/* pathname as is */
	 char * prepath;		/* path preceeding catalog/ */
	 char * dfiles;
	 char * pfiles;
	 char * product_control_dir;
	 char * fileset_control_dir;
	 char * pathname;		/* un-adorned (installed) path */
	 char * basename;
} SWPATH_EX;	/* SWPATH portable export object. */

/* public Methods */

     SWPATH * swpath_open (char * pathname);
     void swpath_reset(SWPATH * swpath);
     int swpath_close (SWPATH * swpath);

     int swpath_num_of_components(SWPATH * swpath, char * str);
     int swpath_resolve_prepath(SWPATH * swpath, char * name);
     void swpath_set_dfiles(SWPATH * swpath, char * name);
     void swpath_set_product_control_dir (SWPATH * swpath, char * s);
     void swpath_set_fileset_control_dir (SWPATH * swpath, char * s);
     void swpath_set_pfiles (SWPATH * swpath, char * s);
     void swpath_set_prepath (SWPATH * swpath, char * s);
     void swpath_set_filename (SWPATH * swpath, char * s);
     void swpath_set_pathname (SWPATH * swpath, char * s);
     void swpath_set_pkgpathname (SWPATH * swpath, char * s);

     char * swpath_make_name(SWPATH * swpath, char *dirpath, char *name);
     char * swpath_get_product_control_dir (SWPATH * swpath);
     char * swpath_get_fileset_control_dir (SWPATH * swpath);
     char * swpath_get_pfiles (SWPATH * swpath);
     char * swpath_get_dfiles (SWPATH * swpath);
     char * swpath_get_basename (SWPATH * swpath);
     char * swpath_get_pathname (SWPATH * swpath);
     char * swpath_get_prepath (SWPATH * swpath);
     char * swpath_get_pkgpathname (SWPATH * swpath);
   
     char * swpath_form_catalog_path (SWPATH * swpath,  STROB * buf );
     char * swpath_form_storage_path (SWPATH * swpath,  STROB * buf );
     int swpath_parse_path (SWPATH * swpath, char * name);
     int swpath_is_catalog (SWPATH * swpath, char * name);
     void swpath_set_is_catalog (SWPATH * swpath, int n);
     int swpath_get_is_catalog (SWPATH * swpath);

     void swpath_debug_dump(SWPATH * sp, FILE * fp);
     /* char * swpath_dump_string_s(SWPATH * swpath, char * prefix); */
     /* char * swpath_dump_string_ex_s(SWPATH_EX * swpath, char * prefix); */

     SWPATH_EX * swpath_create_export(SWPATH * sp);
     SWPATH_EX * swpath_shallow_create_export(void);
     void 	swpath_shallow_delete_export(SWPATH_EX * sx);
     void 	swpath_delete_export(SWPATH_EX * sx);
     void swpath_shallow_fill_export(SWPATH_EX * sx, SWPATH * sp);
     void swpath_set_is_minimal_layout(SWPATH * swpath, int n);
     int  swpath_get_is_minimal_layout(SWPATH * swpath);
     char * swpath_form_path (SWPATH * swpath,  STROB* buf );
     char * swpath_ex_print(SWPATH_EX * swpath, STROB * buf, char * prefix);

#endif
