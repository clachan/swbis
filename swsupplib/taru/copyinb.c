/* copyinb.c - extract or list a cpio archive
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



static void swab_array(char *ptr, int count);

/* Return 16-bit integer I with the bytes swapped.  */
#define swab_short(i) ((((i) << 8) & 0xff00) | (((i) >> 8) & 0x00ff))


/* Fill in FILE_HDR by reading an old-format ASCII format cpio header from
   file descriptor IN_DES, except for the magic number, which is
   already filled in.  */

void
taru_read_in_binary(struct new_cpio_header * file_hdr, int in_des)
{
	struct old_cpio_header short_hdr;

	/* Copy the data into the short header, then later transfer
	   it into the argument long header.  */
	short_hdr.c_dev = ((struct old_cpio_header *) file_hdr)->c_dev;
	short_hdr.c_ino = ((struct old_cpio_header *) file_hdr)->c_ino;
	/* tape_buffered_read (((char *) &short_hdr) + 6, in_des, 20L); */
	taru_tape_buffered_read(in_des, ((char *) &short_hdr) + 6, 20L);

	/* If the magic number is byte swapped, fix the header.  */
	if (file_hdr->c_magic == swab_short((unsigned short) 070707)) {
		static int warned = 0;

		/* Alert the user that they might have to do byte swapping on
		   the file contents.  */
		if (warned == 0) {
			fprintf(stderr, "warning: archive header has reverse byte-order");
			warned = 1;
		}
		swab_array((char *) &short_hdr, 13);
	}
	file_hdr->c_dev_maj = major(short_hdr.c_dev);
	file_hdr->c_dev_min = minor(short_hdr.c_dev);
	file_hdr->c_ino = short_hdr.c_ino;
	file_hdr->c_mode = short_hdr.c_mode;
	file_hdr->c_uid = short_hdr.c_uid;
	file_hdr->c_gid = short_hdr.c_gid;
	file_hdr->c_nlink = short_hdr.c_nlink;
	file_hdr->c_rdev_maj = major(short_hdr.c_rdev);
	file_hdr->c_rdev_min = minor(short_hdr.c_rdev);
	file_hdr->c_mtime = (unsigned long) short_hdr.c_mtimes[0] << 16
	    | short_hdr.c_mtimes[1];

	file_hdr->c_namesize = short_hdr.c_namesize;
	file_hdr->c_filesize = (unsigned long) short_hdr.c_filesizes[0] << 16
	    | short_hdr.c_filesizes[1];

	/* Read file name from input.  */
	
  	ahsStaticSetTarFilenameLength(file_hdr, file_hdr->c_namesize);
	taru_tape_buffered_read(in_des, (void*)ahsStaticGetTarFilename(file_hdr), (long) file_hdr->c_namesize);

	/* In binary mode, the amount of space allocated in the header for
	   the filename is `c_namesize' rounded up to the next short-word,
	   so we might need to drop a byte.  */
	if (file_hdr->c_namesize % 2)
		swlib_read_amount(in_des, 1L);
	/* tape_toss_input (in_des, 1L); */

#ifndef __MSDOS__
	/* HP/UX cpio creates archives that look just like ordinary archives,
	   but for devices it sets major = 0, minor = 1, and puts the
	   actual major/minor number in the filesize field.  See if this
	   is an HP/UX cpio archive, and if so fix it.  We have to do this
	   here because process_copy_in() assumes filesize is always 0
	   for devices.  */
	switch (file_hdr->c_mode & CP_IFMT) {
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
		    && file_hdr->c_rdev_min == 1) {
			file_hdr->c_rdev_maj = major(file_hdr->c_filesize);
			file_hdr->c_rdev_min = minor(file_hdr->c_filesize);
			file_hdr->c_filesize = 0;
		}
		break;
	default:
		break;
	}
#endif				/* __MSDOS__ */
}



/* Exchange the bytes of each element of the array of COUNT shorts
   starting at PTR.  */

static void
swab_array(char *ptr, int count)
{
	char tmp;

	while (count-- > 0) {
		tmp = *ptr;
		*ptr = *(ptr + 1);
		++ptr;
		*ptr = tmp;
		++ptr;
	}
}
