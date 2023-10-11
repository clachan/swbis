/*
 Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>

*/
/*
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <tar.h>
#include "swparser_global.h"
#include "uxformat.h"
#include "swpackagefile.h"

int
main (int argc, char ** argv)
{
  int  ret;
  STROB * namebuf = strob_open(100);
  char * name; 
  swPackageFile * package; 
  portableArchive * xformat_impl; 
  swPathName sp;


  xformat_impl =  new portableArchive(STDIN_FILENO, STDOUT_FILENO);
  if (!xformat_impl) {
     fprintf (stderr,"error opening package\n");
     exit (1);
  }
  
  package = new swPackageFile(xformat_impl);
  package->xFormat_open_archive(STDIN_FILENO);

  while ((ret=package->swfile_read_next_archive_header()) >= 0) {
     package->swfile_get_name(namebuf);
     name = strob_str(namebuf);
     sp.swp_parse_path(name);
     if (sp.swp_get_is_catalog() == SWPATH_CTYPE_STORE) {
         package->xFormat_set_name (sp.swp_get_pathname()); 
         if ( ::strlen(package->swfile_get_name(namebuf)) ) {
     	   name = strob_str(namebuf);
	   package->xFormat_write_header();
	   if (package->xFormat_get_tar_typeflag() == REGTYPE && ret > 0) {
               package->xFormat_copy_pass();
           }
         }
     } else {
         if (package->xFormat_get_tar_typeflag() == REGTYPE && ret > 0) {
             package->xFormat_copy_pass(-1);
         }
     }
  }
  if (ret < 0) {
    ;//fprintf (stderr, "%s: error in tar header.\n", argv[0]);
  } else {
    return 0;
  }
}
