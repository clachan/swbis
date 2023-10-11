/* swacfl.c  -  Archive-to-Source Name translation lists.
   
   Copyright (C) 1999  Jim Lowe 
   All rights reserved.

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
#include "swacfl.h"
#include "strob.h"
#include "swlib.h"

static void free_entry (swacfl_entry * en);

SWACFL *
swacfl_open ()
{
	SWACFL * swacfl=(SWACFL *)malloc(sizeof(SWACFL));
	if (!swacfl) return NULL;
	swacfl->entry_array_=cplob_open(8);
        cplob_additem(swacfl->entry_array_, 0, NULL);
	return swacfl;
}

void
swacfl_close(SWACFL * swacfl)
{
	int i=0;
	swacfl_entry * en;
	while ((en=(swacfl_entry*)cplob_val(swacfl->entry_array_,i++))) {
		free_entry (en);
	}
	cplob_close(swacfl->entry_array_);	
}

/*
void swacfl_print (SWACFL * swacfl, FILE * fp)
{
	int i=0;
	swacfl_entry * en;
	char * p;
	while ((en=(swacfl_entry*)cplob_val(swacfl->entry_array_,i++))) {
		p=strob_str(en->archiveNameM);
		fprintf(fp,"%d:",en->source_code_);
		if (en->from_name_) 
			fprintf(fp,"  %s",en->from_name_);
		if (p) 
			fprintf(fp,"  %s\n", strob_str(en->archiveNameM));
	}
}
*/

static void
free_entry (swacfl_entry * en) {
	strob_close(en->archiveNameM);	
	if (en->from_name_) swbis_free(en->from_name_);
}

swacfl_entry *
swacfl_find_entry(SWACFL * swacfl, char * archive_name)
{
	int i=0;
	swacfl_entry * en;
	char * p;

	archive_name = swlib_return_no_leading(archive_name);
	while ((en=(swacfl_entry*)cplob_val(swacfl->entry_array_,i++))) {
		p = strob_str(en->archiveNameM);
		p = swlib_return_no_leading(p);
		/* fprintf(stderr, "D: have [%s],  looking for [%s]\n", p, archive_name); */
		/* ***
		fprintf(fp,"%d:",en->source_code_);
		if (en->from_name_) 
			fprintf(fp,"  %s",en->from_name_);
		if (p) 
			fprintf(fp,"  %s\n", strob_str(en->archiveNameM));
		*** */
		if (swlib_compare_8859(archive_name, p) == 0) return en;
	}
	return (swacfl_entry *)NULL;
}

void
swacfl_add(SWACFL * swacfl, char * archive_name, char * source_name, int type)
{
	E_DEBUG3(" archive_name=[%s] source_name=[%s]", archive_name, source_name);
	swacfl_entry *en=swacfl_make_entry(archive_name, source_name, type);
	swacfl_add_entry(swacfl, en);
}

void
swacfl_add_entry(SWACFL * swacfl, swacfl_entry * en)
{
        CPLOB * lob=swacfl->entry_array_;
        cplob_additem(lob, cplob_get_nused(lob) - 1, (char*)en);
        cplob_additem(lob, cplob_get_nused(lob), NULL);
}

swacfl_entry *
swacfl_make_entry(char * archive_name, char * source_name, int type)
{
	swacfl_entry * en = (swacfl_entry *)malloc(sizeof(swacfl_entry));
	
	en->archiveNameM = strob_open(10);
	strob_strcpy(en->archiveNameM, archive_name);
	en->from_name_=swlib_strdup(source_name);
	en->source_code_=type;
	return en;
}

void
swacfl_set_source_name(swacfl_entry * en, char * name)
{
	if (en->from_name_) swbis_free(en->from_name_);	
	en->from_name_=swlib_strdup(name);
}

o__inline__ 
void
swacfl_set_type(swacfl_entry * en, int type)
{
	en->source_code_=type;
}


o__inline__ 
char *
swacfl_get_archive_name(swacfl_entry * en)
{
	return strob_str(en->archiveNameM);
}

o__inline__ 
char *
swacfl_get_source_name(swacfl_entry * en)
{
	return en->from_name_;
}

o__inline__ 
int
swacfl_get_type(swacfl_entry * en)
{
	return en->source_code_;
}


