/* swicat_s.h -- catalog query parsing routines

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

#ifndef swicat_s_h_200704
#define swicat_s_h_200704

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

/*  Query/Response Format made by swpg_get_catalog_entries()
Q0:P:swbis,r<0.585.20070324a
R0:P:...:
R1:P:BP.:swbis:swbis                    r=0.585.20070324a       v=GNU   i=0   
Q2:P:swbis,r>0.585.20070324a
R2:P:...:
*/

#define SWICAT_SC_STATUS_UNSET		0
#define SWICAT_SC_STATUS_NOT_FOUND	1
#define SWICAT_SC_STATUS_FOUND		2
#define SWICAT_SC_STATUS_DELETE		3

typedef struct {  /* Query */
	/* Example:
		Q1:P:swbis,r==0.585.20070324a
	*/
	char * lineM;
	int numberM;
	char levelM[2];   /* 'P' or 'B" */
	char * swspec_stringM;
	SWVERID * swspecM;	
} SWICAT_SQ;

typedef struct {  /* Entry listing */
	/* Example:
		swbis                    r=0.585.20070324a       v=GNU   i=0   
	*/
	char * entryM;
	char * tagM;
	char * revisionM;
	char * vendor_tagM;
	char * locationM;
	char * sequenceM;
	char * qualifierM;
} SWICAT_SE;

typedef struct {  /* Response */
	/* Example:
		R1:P:BP.:swbis:swbis                    r=0.585.20070324a       v=GNU   i=0   
	*/
	int numberM;
	char levelM[2];   /* 'P' or 'B" */
	int matches_bundleM; /* true or false */
	int matches_productM; /* true or false */
	int matches_filesetM; /* true or false */
	char * bundleM;
	char * lineM;
	char * entry_lineM;
	SWICAT_SE * seM;
	SWVERID * swspecM;
	STROB * catalog_entry_pathM;
	int foundM;
} SWICAT_SR;


SWICAT_SE * swicat_se_create(void);
void swicat_se_delete(SWICAT_SE * se);

/* Query/Response pair */
typedef struct { 
	SWICAT_SQ * sqM;  /* The query line */
	VPLOB * srM;  /* List of SWICAT_SR responses */
	int swverid_uuidM;
	int statusM;
} SWICAT_SC;

typedef struct { 
	VPLOB * scM;  /* List of SWICAT_SC pairs */
} SWICAT_SL;


/* Query */

SWICAT_SQ * swicat_sq_create(void);
void swicat_sq_delete(SWICAT_SQ *);
int swicat_sq_parse(SWICAT_SQ *, char * line);

/* Response */

SWICAT_SR * swicat_sr_create(void);
void swicat_sr_delete(SWICAT_SR *);
int swicat_sr_parse(SWICAT_SR *, char * line);

/* Query/Response Combination */

SWICAT_SC * swicat_sc_create(void);
void swicat_sc_delete(SWICAT_SC *);
void swicat_sc_add_sr(SWICAT_SC *, SWICAT_SR *);
void swicat_sc_set_sq(SWICAT_SC *, SWICAT_SQ *);

SWICAT_SL * swicat_sl_create(void);
void swicat_sl_delete(SWICAT_SL *);
void swicat_sl_add_sc(SWICAT_SL *, SWICAT_SC *);


char * swicat_sr_form_catalog_path(SWICAT_SR * sr, char * installed_software_catalog, STROB * buf);
char * swicat_sr_form_swspec_from_catalog_path(char * path, STROB * swspec);
char * swicat_sr_form_swspec(SWICAT_SR * sr, STROB * buf);

#endif
