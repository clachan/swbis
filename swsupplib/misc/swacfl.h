/* swacfl.h: A Archive-to-Source Name translation manager.
 */

/*
 *  Copyright (C) 1998  James H. Lowe, Jr.  <jhl@richmond.infi.net>
 *  This file may be copied under the terms of the GNU GPL.
 *
 */

#ifndef swacfl_h_199901h
#define swacfl_h_199901h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strob.h"
#include "cplob.h"


/*       Where to get the source file data.
 */
#define SWACFL_SRCCODE_ARCHIVE_STREAM		0
#define SWACFL_SRCCODE_FILESYSTEM		1
#define SWACFL_SRCCODE_RPMHEADER		2
#define SWACFL_SRCCODE_PSF			3


/*	There is one of these for every file in 
 *	the package.
 */

typedef struct {
	char * from_name_;	/* name of the source file. */
	int    source_code_;	/* where to get it. */
	STROB * archiveNameM;
} swacfl_entry;


typedef struct 
{
	CPLOB * entry_array_;  /* list of archive members */
} SWACFL;


/* public Functions */
SWACFL * 	swacfl_open (void);
void		swacfl_close (SWACFL * swacfl);
/* void		swacfl_print (SWACFL * swacfl, FILE * fp); */
void		swacfl_add(SWACFL * swacfl, char * archive_name, char * source_name, int type);
void		swacfl_add_entry(SWACFL * swacfl, swacfl_entry * en);
swacfl_entry *  swacfl_make_entry(char * archive_name, char * source_name, int type);
/* void		swacfl_set_archive_name(swacfl_entry * entry, char * name); */
void		swacfl_set_source_name(swacfl_entry * entry, char * name);
void		swacfl_set_type(swacfl_entry * entry, int type);
char *		swacfl_get_archive_name(swacfl_entry * entry);
char *		swacfl_get_source_name(swacfl_entry * entry);
int		swacfl_get_type(swacfl_entry * entry);
swacfl_entry *  swacfl_find_entry(SWACFL * swacfl, char * archive_name);


#endif

