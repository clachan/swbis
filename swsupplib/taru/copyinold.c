/* copyinold.c - readin old ascii cpio headers
   Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.

   Portions of this code are derived from code (found in GNU cpio)
   copyrighted by the Free Software Foundation.  Retention of their 
   copyright ownership is required by the GNU GPL and does *NOT* signify 
   their support or endorsement of this work.
   						jhl
					   

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
#include <sys/types.h>
#include <sys/stat.h>
#include "filetypes.h"
#include "system.h"
#include "cpiohdr.h"
#include "fnmatch_u.h"
#include "uxfio.h"
#include "ahs.h"
#include "uinfile.h"
#include "taru.h"
#include "swlib.h"
#include "strob.h"
#include "hllist.h"

#ifndef HAVE_LCHOWN
#define lchown chown
#endif

/* Fill in FILE_HDR by reading an old-format ASCII format cpio header from
   file descriptor IN_DES, except for the magic number, which is
   already filled in.  */

int
taru_read_in_old_ascii2 (TARU * taru, struct new_cpio_header * file_hdr, int in_des, char * buf)
{
  char ascii_header[78];
  unsigned long dev;
  unsigned long rdev;
  int bytesread = 0;
  int ret;
  unsigned long xxsize;

  if ( buf ) {
      memcpy(ascii_header, buf, 70);
  } else {
     if (taru_tape_buffered_read (in_des, ascii_header, 70L) != 70) return -1; 
  }
  bytesread = 70;
  ascii_header[70] = '\0';
  sscanf (ascii_header,
	  "%6lo%6lo%6lo%6lo%6lo%6lo%6lo%11lo%6lo%11lo",
	  &dev, &file_hdr->c_ino,
	  &file_hdr->c_mode, &file_hdr->c_uid, &file_hdr->c_gid,
	  &file_hdr->c_nlink, &rdev, &file_hdr->c_mtime,
	  &file_hdr->c_namesize, &xxsize);
  file_hdr->c_filesize = xxsize;
  file_hdr->c_dev_maj = major(dev);
  file_hdr->c_dev_min = minor(dev);
  file_hdr->c_rdev_maj = major(rdev);
  file_hdr->c_rdev_min = minor(rdev);

  /* Read file name from input.  */
  
  ahsStaticSetTarFilenameLength(file_hdr, file_hdr->c_namesize + 1);
  
  if (!buf) {
      if ((ret=taru_tape_buffered_read (in_des, (void*)(ahsStaticGetTarFilename(file_hdr)), (size_t) file_hdr->c_namesize))
          != (int)(file_hdr->c_namesize) ) {
	  	return -bytesread;
  	} 
  } else {
       /* Simulate a read from tape */
       if (file_hdr->c_namesize > 442 /* 512 - 70 */) {
		fprintf(stderr, "taru_read_in_oldascii2 name too long for this implementation.\n");
		exit(33);
		return -1;
       }
       memcpy(ahsStaticGetTarFilename(file_hdr), buf+70, file_hdr->c_namesize); 
  }
  bytesread += file_hdr->c_namesize;
  
  /* HP/UX cpio creates archives that look just like ordinary archives,
     but for devices it sets major = 0, minor = 1, and puts the
     actual major/minor number in the filesize field.  See if this
     is an HP/UX cpio archive, and if so fix it.  We have to do this
     here because process_copy_in() assumes filesize is always 0
     for devices.  */
  switch (file_hdr->c_mode & CP_IFMT)
    {
      case CP_IFCHR:
      case CP_IFBLK:
#ifdef CP_IFSOCK
      case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
      case CP_IFIFO:
#endif
	if (file_hdr->c_filesize != 0
	    && file_hdr->c_rdev_maj == 0
	    && file_hdr->c_rdev_min == 1)
	  {
	    file_hdr->c_rdev_maj = major (file_hdr->c_filesize);
	    file_hdr->c_rdev_min = minor (file_hdr->c_filesize);
	    file_hdr->c_filesize = 0;
	  }
	break;
      default:
	break;
    }
  return bytesread;
}

