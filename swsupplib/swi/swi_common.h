/* swi_common.h -  POSIX package decoding
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

#ifndef swi_common_h_200501
#define swi_common_h_200501

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shcmd.h"
#include "swlib.h"
#include "swutilname.h"
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
#include "sw.h"
#include "swicol.h"

/* Internal Structure Id's */
#define SWI_I_TYPE_PACKAGE	'K'	
#define SWI_I_TYPE_BUNDLE	'B'	
#define SWI_I_TYPE_PROD		'P'	
#define SWI_I_TYPE_XFILE	'C'   /* either fileset, dfiles or pfiles */
#define SWI_I_TYPE_AFILE	'E'
#define SWI_I_TYPE_SCRIPT	'D'

#define SWI_MAX_OBJ			10
#define SWI_SECTION_BOTH		2
#define SWI_SECTION_CATALOG		1
#define SWI_SECTION_STORAGE		0

#define SWI_RESULT_UNDEFINED	SW_NONE     /* POSIX result  'none'    */
#define SWI_RESULT_SUCCESS	SW_SUCCESS  /* POSIX result  'success' */
#define SWI_RESULT_WARNING	SW_WARNING  /* POSIX result  'warning' */
#define SWI_RESULT_FAILURE	SW_ERROR    /* POSIX result  'failure'   */

/* #define SWI_INDEX_ORDINAL(W)	(W->INDEX_ordinalM) */

#define SWI_get_index_header(A) A->swi_pkgM->baseM.global_headerM;
#define SWI_get_dfiles(A) A->swi_pkgM->dfiles_attributeM;
#define SWI_get_pfiles(A) A->swi_pkgM->pfiles_attributeM;

#define SWI_internal_error() swi_com_internal_error(__FILE__, __LINE__)
#define SWI_internal_fatal_error() swi_com_internal_fatal_error(__FILE__, __LINE__)

#define SWINSTALL_CATALOG_TAR 	"catalog.tar"

void swi_com_fatal_error(char * msg, int msg2);
void swi_com_assert_pointer(void * p, char * file, int lineno);
void swi_com_assert_value(int p, char * file, int lineno);
void swi_com_internal_error(char * file, int line);
void swi_com_internal_fatal_error(char * file, int line);
void swi_com_do_preview(TARU * taru, struct new_cpio_header * file_hdr, char * tar_header_p, int header_len, time_t now);
void swi_com_check_clean_relative_path(char * name);
void swiInitListOfObjects(void ** pp);
int swiAddObjectToList(void ** pp, void * p);
int swiGetNumberOfObjects(void ** pp);
char * swi_com_get_fd_mem(int fd, int * datalen);
int swi_is_global_index(SWPATH * swpath, char * name);
int swi_com_field_edge_detect(char * current, char * previous);
int swi_com_field_edge_detect_fileset(SWPATH_EX * current, SWPATH_EX * previous);
void swi_com_header_manifold_reset(SWHEADER * swheader);
int swi_com_close_memfd(int fd);
int swi_com_set_header_index(SWHEADER * header, SWPATH_EX *, int * ai);
/* char * swi_com_determine_control_directory(SWHEADER * header); */
char * swi_com_new_fd_mem(int fd, int * datalen);

#endif
