/* swyyparse.c
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "swutilname.h"
#include "swlex_supp.h"
#include "swlib.h"

extern int swlex_definition_file;
extern int swlex_debug;
extern int swlex_inputfd;
extern int swlex_errorcode;
extern int swparse_outputfd;
extern int swparse_atlevel;
extern int swparse_form_flag;
extern int swlex_linenumber;

int yyparse(void);

static
int
icmp(char * path, char * name)
{
	int ret;
	char * s;
	if ((s=swlib_basename(NULL, path))) {
		ret = strcmp(s, name);
	} else {
		ret = -1;
	}
	return ret;
}

int
sw_yyparse (int uxfio_ifd, int  uxfio_ofd, char * metadata_filename, int atlevel, int mark_up_flag ) {
	char * s;

	if (icmp(metadata_filename, "INFO") == 0) {
		swlex_definition_file=SW_INFO;
  	} else if (icmp(metadata_filename, "INDEX") == 0) {
		swlex_definition_file=SW_INDEX;
  	} else if (icmp(metadata_filename, "INSTALLED") == 0) {
		swlex_definition_file=SW_INSTALLED;
	} else if (icmp(metadata_filename, "OPTION") == 0) {
		swlex_definition_file=SW_OPTION;
	} else if (icmp(metadata_filename, "PSF") == 0) {
		swlex_definition_file=SW_PSF;
	} else if (icmp(metadata_filename, "psf") == 0) {
		swlex_definition_file=SW_PSF;
	} else if (icmp(metadata_filename, "PSFi") == 0) {
		swlex_definition_file=SW_PSF_INCL;
	} else { 
		fprintf (stderr,"%s: bad file type selection: %s\n", swlib_utilname_get(), metadata_filename); return -1;
	}
	swlex_linenumber = 0;
	swparse_outputfd = uxfio_ofd;
	swlex_inputfd = uxfio_ifd;
	swparse_atlevel = atlevel; 
	swparse_form_flag = mark_up_flag;
	SW_YYPARSE();
	return swlex_errorcode;
}

int yywrap(void) 
{ 
	/* yyrestart(NULL);  */
       return 1; 
}

