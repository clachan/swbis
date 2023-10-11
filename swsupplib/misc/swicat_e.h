/* swicat_e.h -- catalog entry parsing

 Copyright (C) 2007 Jim Lowe
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef swicat_e_h_200704
#define swicat_e_h_200704

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
#include "swgpg.h"
#include "swi.h"


#define SWICAT_DEACTIVE_ENTRY 0
#define SWICAT_ACTIVE_ENTRY 1
#define SWICAT_RETVAL_NULLARCHIVE 33

typedef struct {
	int entry_fdM;  /* fd of archive, which may contain multiple entries */
	int start_offsetM;   /* start offset of entry */
	int end_offsetM;    /* end offset of entry */
	char * entry_prefixM; /* leading directory archive name */
	XFORMAT * xformatM;
	TARU * taruM;
	SWI * swiM;
	int tmp_fdM;
	int restore_offsetM;
	int xformat_close_on_e_deleteM;
	int taru_close_on_e_deleteM;
	int swi_close_on_e_deleteM;
} SWICAT_E;

SWICAT_E * swicat_e_create(void);
void swicat_e_delete(SWICAT_E * swicat_e);
SWI_FILELIST * swicat_e_make_file_list(SWICAT_E * e, SWUTS * swuts, SWI * swi);
int swicat_e_open_entry_tarball(SWICAT_E * swicat_e, int ifd);
int swicat_e_find_attribute_file(SWICAT_E * swicat_e, char * attribute, STROB * buf, AHS *);
int swicat_e_verify_gpg_signature(SWICAT_E * e, SWGPG_VALIDATE * swgpg);
VPLOB * swicat_e_open_catalog_tarball(int ifd);
SWI * swicat_e_open_swi(SWICAT_E * e);
int swicat_e_reset_fd(SWICAT_E * e);
char * swicat_e_form_catalog_path(SWICAT_E * e, STROB * buf, char * isc_path, int active_flag);
SWHEADER * swicat_e_isf_parse(SWICAT_E * e, int *pstatus, AHS * ahs);

#endif
