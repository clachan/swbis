/* swcommon0.c -- swbis utility common routines.

 Copyright (C) 2003,2004 James H. Lowe, Jr.  <jhlowe@acm.org> 
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


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "strob.h"
#include "uxfio.h"
#include "taru.h"
#include "swlib.h"
#include "swheader.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swheaderline.h"
#include "swgp.h"
#include "swssh.h"
#include "swfork.h"
#include "swcommon.h"
#include "swevents.h"
#include "atomicio.h"

static
void close_stdio(void) {
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

void
swc0_create_parser_buffer() {
	yylval.strb = strob_open(128);
	if (!yylval.strb) {
		fprintf (stderr,"out of memory.\n");
		exit(1);
	}
}

int 
swc0_process_w_option(STROB * tmp, CPLOB * w_arglist, 
		char * i_optarg, int * w_argc_p)
{
	char * np;
	char * t;
	STROB * wtmp = strob_open(10);
	char * w_option_arg;
	char * w_option_value;
	
	w_option_arg = strdup(i_optarg);
	t = strob_strtok(wtmp, w_option_arg, ",");
				while (t) {
		w_option_value = strchr(t,'=');
		if (!w_option_value) {
			np = NULL;
			w_option_value = NULL;
		} else {
			np = w_option_value;
			*w_option_value = '\0';
			w_option_value++;
		}
		//
		// Now add the option and value to the arg list.
		//
		strob_strcpy(tmp, "--");
		strob_strcat(tmp, t);
		if (w_option_value) {
			strob_strcat(tmp, "=");
			strob_strcat(tmp, w_option_value);
		}
		cplob_add_nta(w_arglist, strdup(strob_str(tmp)));
		(*w_argc_p) ++;

		if (np) *np = '=';
		t = strob_strtok(wtmp, NULL, ",");
	}
	strob_close(wtmp);
	return 0;
}

int
swc0_set_arf_format(char * user_optarg, int * format_arf, int * do_oldgnutar, 
	int * do_bsdpax3, int * do_oldgnuposix, int * do_gnutar, int * do_paxexthdr)
{
	if (user_optarg == (char*)NULL) return 0;

	if (!strcmp(user_optarg,"ustar")) {
  		*format_arf = arf_ustar;
		*do_gnutar = 0;
		*do_oldgnuposix = 0;
	} else if (!strcasecmp(user_optarg,"posix")  || !strcasecmp(user_optarg, "pax")) {
  		*format_arf = arf_ustar;
		*do_paxexthdr = 1;
		*do_gnutar = 0;
		*do_oldgnuposix = 0;
	} else if (!strcmp(user_optarg,"newc")) {
		*format_arf = arf_newascii;
	} else if (
		!strcmp(user_optarg,"gnutar") ||
		!strcmp(user_optarg,"gnu") ||
		0
	) {
  		*format_arf = arf_ustar;
		*do_gnutar = 1;
		*do_oldgnutar = 0;
		*do_bsdpax3 = 0;
	} else if (
		!strcmp(user_optarg,"oldgnu") ||
		0
	) {
  		*format_arf = arf_ustar;
		*do_gnutar = 0;
		*do_oldgnutar = 1;
		*do_bsdpax3 = 0;
	} else if (!strcmp(user_optarg,"ustar0")) {
  		*format_arf = arf_ustar;
		*do_gnutar = 0;
		*do_oldgnuposix = 1;
		*do_bsdpax3 = 0;
	} else if (!strcmp(user_optarg,"bsdpax3")) {
  		*format_arf = arf_ustar;
		*do_gnutar = 0;
		*do_bsdpax3 = 1;
	} else if (!strcmp(user_optarg,"crc")) {
  		*format_arf = arf_crcascii;
	} else if (!strcmp(user_optarg,"odc")) {
  		*format_arf = arf_oldascii;
	} else {
		fprintf (stderr,"unrecognized format: %s\n", user_optarg);
		close_stdio();
		exit(1);
	}
	return 0;
}
