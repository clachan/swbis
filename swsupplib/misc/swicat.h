/* swicat.h -- installed software catalog routines

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef swicat_h_20040322
#define swicat_h_20040322


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swverid.h"
#include "swi.h"
#include "swevents.h"
#include "globalblob.h"
#include "vplob.h"
#include "strar.h"
#include "swicat_s.h"

typedef struct {  /* exrequisites query details */
	STROB * exM;
	STRAR * ex_violatedM;
} SWICAT_EX_SET;

typedef struct {  /* prerequisite query details */
	STROB * preM;
	STRAR * pre_fullfilledM;
} SWICAT_PRE_SET;

typedef struct {
	int pre_resultM;
	int ex_resultM;
	int co_resultM;
	STRAR * failed_preM;  /* list of prerequistes that failed */
	STRAR * failed_coM;
	STRAR * passed_exM;  /* list of exrequisites that passed */
	VPLOB * ex_set_arrayM;
	VPLOB * pre_set_arrayM; /* list of prereqs that passed */
	SWICAT_SL * slM;
} SWICAT_REQ;

#define SWINSTALL_INCAT_VERSION		"0.8"
#define SWINSTALL_INCAT_NAME		"export" 

#define SWICAT_FORM_DIR1	"lform_dir1"	/* bundle product revision vendor_tag */
#define SWICAT_FORM_B1		"lform_b1"	/* bundle product revision vendor_tag */
#define SWICAT_FORM_P1		"lform_p1"	/* product revision vendor_tag */
#define SWICAT_FORM_DEP1	"lform_dep1"	/* contains information on what field matched */
#define SWICAT_FORM_TAR1	"lform_tar1"  	/* tar archive */ 

#define SWICAT_REQ_TYPE_P	"P"  	/* prerequisite */ 
#define SWICAT_REQ_TYPE_C	"C"  	/* corequisite */ 
#define SWICAT_REQ_TYPE_E	"E"  	/* exrequisite */ 

/* Array indices for prerequisistes, corequisites, exrequisites */
#define SWICAT_REQ_RESULT_LEN	3
enum req_results {SWICAT_QP, SWICAT_QC, SWICAT_QE};

int swicat_write_installed_software(SWI * swi, int ofd);
int swicat_isf_all_scripts(STROB * buf, SWI_SCRIPTS * xx, int do_if_active);
int swicat_isf_control_script(STROB * buf, SWI_CONTROL_SCRIPT * xx, int do_if_active);

int swicat_isf_fileset(SWI * swi, STROB * buf, SWI_XFILE * xx, int do_if_active);
int swicat_isf_product(SWI * swi, STROB * buf, SWI_PRODUCT * xx, int do_if_active);
int swicat_isf_installed_software(STROB * buf, SWI * swi);
int swicat_env(STROB * buf, SWI * swi, char * control_script_pkg_dir, char * tag);
void swicat_write_auto_comment(STROB * buf, char * filename);
int swicat_write_script_cases(SWI * swi, STROB * buf, char * sw_selection);
void swicat_construct_controlsh_taglist(SWI * swi, char * sw_selections, STROB * list);
int swicat_r_get_installed_catalog(GB * G, VPLOB * swspecs, char * target_path);
int swicat_write_isc_script(STROB * script_buf, GB * G, VPLOB * swspecs, VPLOB *,VPLOB *,VPLOB *, char * lform);
int swicat_req_analyze(GB * G, SWICAT_REQ *, char * query_result, SWICAT_SL **);
SWICAT_REQ * swicat_req_create(void);
void swicat_req_delete(SWICAT_REQ *);
int swicat_squash_null_bytes(int fd);
int swicat_req_print(GB * G, SWICAT_REQ * req);
int swicat_req_get_pre_result(SWICAT_REQ * req);
int swicat_req_get_ex_result(SWICAT_REQ * req);

#endif
