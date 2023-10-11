/* swlist_lib.h - swlist routines
  
 Copyright (C) 2006 James H. Lowe, Jr.

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef swlist_lib_200609_h
#define swlist_lib_200609_h

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vplob.h"
#include "usgetopt.h"
#include "swcommon0.h"
#include "swextopt.h"
#include "swutillib.h"
#include "swverid.h"
#include "globalblob.h"


#ifndef DEFAULT_PAX_W 
#define DEFAULT_PAX_W "pax"
#endif
#ifndef DEFAULT_PAX_R 
#define DEFAULT_PAX_R "pax"
#endif

struct blist_level { /* See static array in swlist.h */
	int lv_is_setM;   /* level is set of this object */
	int lv_valueM;    /* level value */
	char * lv_nameM;  /* level name */
};

typedef struct {
	STRAR * attr_levelsM;           /* List of levels for each this->attributesM[i] */
	STRAR * attributesM;      /* List of '-a' option args */
	STROB * attr_flagsM;      /* flags for the list of attributes, one-for-one */
	int do_get_create_dateM;     /* special option arg for '-a' option is set */
	int do_get_mod_dateM;        /* special option arg for '-a' option is set */
	int do_get_software_specM;   /* special option arg for '-a' option is set */
	char * catalogM;             /* '-c' option arg */
	struct blist_level * levelsM;   /* '-l'  level settings */
	int index_format_is_setM;   /* '-v' option is set */
	int level_is_setM;          /* '-l' option has been set, other than host level */
	int has_soc_attributesM;    /* -a  attr specified other than uts host attributes */
	int has_uts_attributesM;    /* -a  attr specified that are uts host attributes */
	int has_annex_attributesM;    /* -a  attr specified that are uts host attributes */
	SWHEADER * isf_headerM;
} BLIST;

void blist_init(BLIST * blist);

int swlist_write_source_copy_script2(GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv,
		int delaytime, int nhops, char * pax_write_command_key, char * hostname, char * blocksize);

int swlist_write_source_copy_script(GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv,
		int delaytime, int nhops, char * pax_write_command_key, char * hostname, char * blocksize);


int swlist_list_distribution_archive(GB * G, struct extendedOptions * opta,
	VPLOB * swspecs, char * target_path, int target_nhops, int efd);

SWI * swlist_list_create_swi(GB * G, struct extendedOptions * opta, VPLOB * swspecs, char * target_path);

int
swlist_looper_sr_payload(GB * G, BLIST * BL, char * target_path, char * cl_target_target, SWICOL * swicol,
		SWICAT_SR * sr, int ofd, int ifd, int * p_rp_status,
		SWUTS * uts, char * pax_write_command_key, int wopt_file_sys, char * wopt_pgm_mode);
void swlist_blist_attr_add(BLIST * BL, char * level , char * attr);
char * swlist_blist_get_next_attr(BLIST * BL, int start, int * px);

void swlist_blist_mark_as_processed(BLIST * BL, int index);
void swlist_blist_clear_all_as_processed(BLIST * BL);
int swlist_blist_level_is_set(BLIST * BL, int level);
int swlist_has_annex_attribute(BLIST * BL);
void swlist_write_attr2(BLIST * BL, char * host, char * name, char * value, int attr_index);
int swlist_is_annex_attribute(char * name);

#endif
