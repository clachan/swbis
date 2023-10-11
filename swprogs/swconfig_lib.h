/* swconfig_lib.h - swconfig routines
  
 Copyright (C) 2009 James H. Lowe, Jr.

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

#ifndef swconfig_lib_200908_h
#define swconfig_lib_200908_h

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

int swconfig_write_source_copy_script2(GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv,
		int delaytime, int nhops, char * pax_write_command_key, char * hostname, char * blocksize);

int swconfig_looper_sr_payload(GB * G, char * target_path, SWICOL * swicol, SWICAT_SR * sr,
			int ofd, int ifd, int *, SWUTS * uts, int * did_skip);

int swconfig_construct_script2(GB * G, CISF_PRODUCT * cisf, STROB * buf, SWI * swi, char * script_tag);

#endif
