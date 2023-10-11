/* swicat_s.c -- catalog query parsing routines

 Copyright (C) 2007,2009 Jim Lowe
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


/*
This file contains routines to parse the blob of text output by the
following command:

swlist -x verbose=3 --deps --prereq swbis

which, depending on the contents of your catalog results in something
like this (This output is made by the awk code in shell_lib.sh):

Q0:P:swbis
R0:P:BP.:swbis:swbis                    r=0.585.20070324a       i=0   

*/

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
#include "swverid.h"
#include "swi.h"
#include "swevents.h"
#include "globalblob.h"
#include "vplob.h"
#include "strar.h"
#include "swicat.h"
#include "swicat_s.h"

				
static
int
sr_construct_swverid(SWICAT_SR * sr, SWICAT_SE * se)
{
	STROB * buf;
	SWVERID * swverid;


	buf = strob_open(30);

	if (sr->bundleM)
		strob_sprintf(buf, STROB_DO_APPEND,
			"%s", sr->bundleM);

	if (strob_strlen(buf))
		strob_sprintf(buf, STROB_DO_APPEND, ".");

	if (se->tagM)
		strob_sprintf(buf, STROB_DO_APPEND,
			"%s", se->tagM); /* FIXME, presumably this is always the product tag */

	if (se->vendor_tagM)
		strob_sprintf(buf, STROB_DO_APPEND,
			"," SWVERID_VERIDS_VENDOR_TAG SWVERID_RELOP_EQ2 "%s", se->vendor_tagM);
	if (se->revisionM)
		strob_sprintf(buf, STROB_DO_APPEND,
			"," SWVERID_VERIDS_REVISION SWVERID_RELOP_EQ "%s", se->revisionM);
	if (se->locationM)
		strob_sprintf(buf, STROB_DO_APPEND,
			"," SWVERID_VERIDS_LOCATION  SWVERID_RELOP_EQ2 "%s", se->locationM);
	if (se->qualifierM)
		strob_sprintf(buf, STROB_DO_APPEND,
			"," SWVERID_VERIDS_QUALIFIER SWVERID_RELOP_EQ2 "%s", se->qualifierM);

	E_DEBUG2("fully qualified swspec=[%s]", strob_str(buf));

	swverid = swverid_open(SW_A_product, strob_str(buf));

	if (!swverid)
		fprintf(stderr, "%s: construction of version id failed for %s\n",
			swlib_utilname_get(), strob_str(buf));

	sr->swspecM = swverid;
	strob_close(buf);
	return swverid == NULL ? 1 : 0;
}

SWICAT_SE *
swicat_se_create(void)
{
	SWICAT_SE * se;
	se = (SWICAT_SE *)malloc(sizeof(SWICAT_SE));
	se->entryM = NULL;
	se->tagM = NULL;
	se->revisionM = NULL;
	se->vendor_tagM = NULL;
	se->locationM = NULL;
	se->sequenceM = NULL;
	se->qualifierM = NULL;
	return se;
}

void
swicat_se_delete(SWICAT_SE * se)
{
	if (se->entryM != NULL) free(se->entryM);
	if (se->tagM != NULL) free(se->tagM);
	if (se->revisionM != NULL) free(se->revisionM);
	if (se->vendor_tagM != NULL) free(se->vendor_tagM);
	if (se->locationM != NULL) free(se->locationM);
	if (se->sequenceM != NULL) free(se->sequenceM);
	if (se->qualifierM != NULL) free(se->qualifierM);
	free(se);
}

int
swicat_se_parse(SWICAT_SE * se, char * entry)
{
	int fn;
	char * s;
	STROB * tmp;
	tmp = strob_open(80);
        /* Example:
                swbis                    r=0.585.20070324a       v=GNU   i=0
	*/
	E_DEBUG2("Entering: [%s]", entry);
	se->entryM = swlib_strdup(entry);
	fn = 0;
	s = strob_strtok(tmp, entry, " \t\n\r");
	while(s) {
		switch(fn) {
		case 0:
			E_DEBUG2("assigning tag: [%s]", s);
			se->tagM = swlib_strdup(s);	
			break;
		default:
			if (strncmp(s, "r=", 2) == 0) {
				s+=2;
				E_DEBUG2("assigning revision: [%s]", s);
				se->revisionM = swlib_strdup(s);
			} else if (strncmp(s, "v=", 2) == 0) {
				s+=2;
				E_DEBUG2("assigning vendor_tag: [%s]", s);
				se->vendor_tagM = swlib_strdup(s);
			} else if (strncmp(s, "i=", 2) == 0) {
				s+=2;
				E_DEBUG2("assigning sequence: [%s]", s);
				se->sequenceM = swlib_strdup(s);
			} else if (strncmp(s, "l=", 2) == 0) {
				s+=2;
				E_DEBUG2("assigning location: [%s]", s);
				se->locationM = swlib_strdup(s);
			} else if (strncmp(s, "q=", 2) == 0) {
				s+=2;
				E_DEBUG2("assigning qualifier: [%s]", s);
				se->qualifierM = swlib_strdup(s);
			} else {
				/* error */
				return -1;
			} 
		break;
		}
		fn++;
		s = strob_strtok(tmp, NULL, " \t\r\n");
	}
	strob_close(tmp);
	return 0;
}

/* Query */

SWICAT_SQ *
swicat_sq_create(void)
{
	SWICAT_SQ * sq;
	sq = (SWICAT_SQ *)malloc(sizeof(SWICAT_SQ));
	sq->numberM = 0;
	sq->levelM[0] = '\0';
	sq->levelM[1] = '\0';
	sq->lineM = NULL;
	sq->swspec_stringM = NULL;
	sq->swspecM = NULL;
	return sq;
}

void
swicat_sq_delete(SWICAT_SQ * sq)
{
	if (sq->lineM != NULL)
		free(sq->lineM);
	if (sq->swspec_stringM != NULL)
		free(sq->swspec_stringM);
	if (sq->swspecM != NULL)
		swverid_close(sq->swspecM);
	free(sq);
}

int
swicat_sq_parse(SWICAT_SQ * sq, char * line)
{
	STROB * tmp;
	char * s;
	int field_no;
	int status;

	field_no = 0;
	tmp = strob_open(80);
	sq->lineM = swlib_strdup(line);

	/* Example:
		Q1:P:swbis,r==0.585.20070324a
	*/
	
	E_DEBUG2("line=[%s]", line);

	s = strob_strtok(tmp, line, ":\r\n");
	while(s) {
		switch(field_no) {
			case 0:
				/* first field */
				sq->numberM = swlib_atoi(s+1, &status);
				if (status != 0) return -1;
				break;
			case 1:
				/* second field */
				sq->levelM[0] = *s;
				break;
			case 2:
				/* third field */
				sq->swspec_stringM = swlib_strdup(s);
				sq->swspecM = swverid_open(NULL, s);
				if (sq->swspecM == NULL)
					return -1;
				break;
		}
		s = strob_strtok(tmp, NULL, ":\r\n");
		field_no++;
	}
	return 0;
}

/* Response */

SWICAT_SR *
swicat_sr_create(void)
{
	SWICAT_SR * sr;
	sr = (SWICAT_SR *)malloc(sizeof(SWICAT_SR));
	sr->lineM = NULL;
	sr->bundleM = NULL;
	sr->entry_lineM = NULL;
	sr->swspecM = NULL;
	sr->catalog_entry_pathM = strob_open(48);
	sr->seM = NULL;
	sr->foundM = 0;
	return sr;
}

void
swicat_sr_delete(SWICAT_SR * sr)
{
	if (sr->lineM != NULL) free(sr->lineM);
	if (sr->bundleM != NULL) free(sr->bundleM);
	if (sr->swspecM != NULL) swverid_close(sr->swspecM);
	free(sr);
}

int
swicat_sr_parse(SWICAT_SR * sr, char * line)
{
	SWICAT_SE * se; 
	STROB * tmp;
	char * s;
	int field_no;
	int status;
	int ret;
	int retval;

	E_DEBUG2("Entering: [%s]", line);
	retval = 0;
	field_no = 0;
	tmp = strob_open(80);
	sr->lineM = swlib_strdup(line);

        /* Example:
                R1:P:BP.:swbis:swbis                    r=0.585.20070324a       v=GNU   i=0
        */
	E_DEBUG2("sr_parse line=[%s]", line);

	s = strob_strtok(tmp, line, ":\n\r");
	while(s) {
		E_DEBUG2("[%s]", s);
		switch(field_no) {
			case 0:
				/* first field */
				E_DEBUG("case 0");
				sr->numberM = swlib_atoi(s+1, &status);
				if (status != 0) return -1;
				break;
			case 1:
				/* second field */
				E_DEBUG("case 1");
				sr->levelM[0] = *s;
				break;
			case 2:
				/* third field */
				E_DEBUG("case 2");
				if (strlen(s) != 3) return -3;
				sr->matches_bundleM = 0;
				sr->matches_productM = 0;
				sr->matches_filesetM = 0;
				if (*s == 'B') 
					sr->matches_bundleM = 1;
				if (*(s+1) == 'P') 
					sr->matches_productM = 1;
				if (*(s+2) == 'F') 
					sr->matches_filesetM = 1;
				break;
			case 3:
				/* fourth field */
				E_DEBUG("case 4");
				sr->bundleM = swlib_strdup(s);
				break;
			case 4:
				/* fifth field */
				/* NOTE: the bundle tag is stripped from  the string */
				E_DEBUG("case 5");
				E_DEBUG2("parsing (_se_parse) [%s]", s);
				se = swicat_se_create();
				ret = swicat_se_parse(se, s);
				if (ret) return -4;
				ret = sr_construct_swverid(sr, se);	
				retval = ret;
				sr->seM = se;
				break;
		}
		s = strob_strtok(tmp, NULL, ":\r\n");
		field_no++;
		E_DEBUG2("field number: %d", field_no);
	}
	strob_close(tmp);
	return retval;
}

/* Query/Response Combination */

SWICAT_SC *
swicat_sc_create(void)
{
	SWICAT_SC * sc;
	sc = (SWICAT_SC *)malloc(sizeof(SWICAT_SC));
	sc->srM = vplob_open();
	sc->sqM = NULL;
	sc->statusM = 0;
	sc->swverid_uuidM = 0;
	return sc;
}

void
swicat_sc_delete(SWICAT_SC * swicat_sc)
{
	vplob_delete_store(swicat_sc->srM, (void(*)(void*))(swicat_sr_delete));
	free(swicat_sc);
}

void
swicat_sc_add_sr(SWICAT_SC * swicat_sc, SWICAT_SR * swicat_sr)
{
	vplob_add(swicat_sc->srM, swicat_sr);
}

void
swicat_sc_set_sq(SWICAT_SC * swicat_sc, SWICAT_SQ * swicat_sq)
{
	swicat_sc->sqM = swicat_sq;
}

SWICAT_SL *
swicat_sl_create(void)
{
	SWICAT_SL * sl;
	sl = (SWICAT_SL *)malloc(sizeof(SWICAT_SL));
	sl->scM = vplob_open();
	return sl;
}

void
swicat_sl_delete(SWICAT_SL * swicat_sl)
{
	vplob_delete_store(swicat_sl->scM, (void(*)(void*))(swicat_sc_delete));
	free(swicat_sl);
}

void
swicat_sl_add_sc(SWICAT_SL * swicat_sl, SWICAT_SC * swicat_sc)
{
	vplob_add(swicat_sl->scM, swicat_sc);
}

char *
swicat_sr_form_swspec(SWICAT_SR * sr, STROB * buf)
{
	char * path;
	STROB * tmp = strob_open(48);
	strob_strcpy(buf, "");
	path = swicat_sr_form_catalog_path(sr, "", tmp);
	if (path == NULL)
		return NULL;
	path = swicat_sr_form_swspec_from_catalog_path(path, buf);
	strob_close(tmp);
	return path;
}


char *
swicat_sr_form_catalog_path(SWICAT_SR * sr, char * installed_software_catalog, STROB * buf)
{
	char * s;
	STROB * sb;

	if (buf == NULL) {	
		sb = sr->catalog_entry_pathM;
	} else {
		sb = buf;
	}
	strob_strcpy(sb, "");

	if (sr->matches_bundleM == 0 && sr->matches_productM == 0) {
		return strob_str(sb);
	}

	strob_strcpy(sb, installed_software_catalog);
	s = sr->bundleM;

	swlib_unix_dircat(sb, s);
	s = sr->seM->tagM;
	swlib_unix_dircat(sb, s);
	s = sr->seM->revisionM;
	swlib_unix_dircat(sb, s);
	s = sr->seM->sequenceM;
	swlib_unix_dircat(sb, s);
	if (swlib_check_clean_path(strob_str(sb))) {
		fprintf(stderr, "error: path tainted: %s\n", strob_str(sb));
	}
	return strob_str(sb);
}

char *
swicat_sr_form_swspec_from_catalog_path(char * path, STROB * swspec)
{
	/* path has the form:
		<isc_path>/foo/foo/1.1/0

	Using this path, make a string with form
		bundle.product,r=revision,i=instance_id
	and pass it to swverid_open()
	*/

	int n;
	char * s;
	STROB * tmp;
	STRAR * dir_components;

	strob_strcpy(swspec, "");	
	if (path == NULL || strlen(path) == 0) {
		return NULL;
	}

	tmp = strob_open(100);
	dir_components = strar_open();

	s = strob_strtok(tmp, path, "/"); 
	while(s) {
		strar_add(dir_components, s);	
		s = strob_strtok(tmp, NULL, "/"); 
	}

	n = strar_num_elements(dir_components);
	if (n < 4) {
		return NULL;
	}
	
	n--;
	strob_sprintf(swspec, 0, "%s.%s,r==%s,i=%s",
		strar_get(dir_components, n-3),
		strar_get(dir_components, n-2),
		strar_get(dir_components, n-1),
		strar_get(dir_components, n-0));

	return strob_str(swspec);
}
