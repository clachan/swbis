#ifdef DOTO_ID
static char tar2tar_cxx[] =
"tar2tar.cxx";
#endif

/*
 Copyright (C) 1998  James H. Lowe, Jr.  <jhl@richmond.infi.net>

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
#include "portablearchive.h"
#include "swmain.h"

int
main (int argc, char ** argv)
{
  int c=0, ret;
  char name[256]; 
  portableArchive * package; 
  
  package = new portableArchive(STDIN_FILENO, STDOUT_FILENO);

  while ((ret=package->xFormat_read_header()) == 512) {
         package->xFormat_write_header();
	 package->xFormat_copy_pass(); 
  }

  if (ret<0) {
     fprintf (stderr,"%s: error in package.\n", argv[0]);
     exit (1);
  } else if (ret==0) {
     while ((ret=package->xFormat_read_header()) == 0) {c++;}
  } else {
     fprintf (stderr,"%s: Error: short read.\n", argv[0]);
     exit (1);
  }
  
  
  if (c < 1) {
     fprintf (stderr,"%s: invalid null block.\n", argv[0]);
     exit (1);
  }
  
  package->xFormat_write_trailer();
  delete package;
  return 0;
}
