/* test_write.c - write out all the function in shell_lib.c

   Copyright (C) 2006  James H. Lowe, Jr.
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS
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

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "strob.h"
#include "minilzo.h"
#include "lzoconf.h"
#include "shlib.h"

int 
main (int argc, char ** argv ) {
	STROB * buf = strob_open(10);
	char * function;
	struct shell_lib_function * farray;
	struct shell_lib_function * f;

	farray = shlib_get_function_array();
	f = farray;
	while(f->nameM) {
		if (f != farray) {
			fprintf(stdout, "\n");
		}
		function = shlib_get_function_text(f, buf);
		if (function == NULL) {
			exit(17);
		}
		fprintf(stdout, "%s", function);
		f++;
	}
	exit(0);
}
