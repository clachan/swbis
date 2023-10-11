/* copyinnew.c - readin new ascii headers
   Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.

   Portions of this code are derived from code (found in GNU cpio)
   copyrighted by the Free Software Foundation.  Retention of their 
   copyright ownership is required by the GNU GPL and does *NOT* signify 
   their support or endorsement of this work.
   						jhl
					   
   *** 
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

static int tarui_read_new_ascii_from_buf(TARU * taru, struct new_cpio_header * file_hdr,  char * ascii_header);
static int tarui_read_in_new_ascii(TARU * taru, struct new_cpio_header * file_hdr,
				int in_des, enum archive_format archive_format_in, char * buf);


/* read in a head beginning just after the magic.
   Does not read the linkname in the file data portion.  
   Return the number of bytes read, -1 on error.
 */


o__inline__
int
taru_read_in_new_ascii (TARU * taru, struct new_cpio_header * file_hdr,
			int in_des, enum archive_format archive_format_in)
{
	return tarui_read_in_new_ascii(taru, file_hdr,in_des,archive_format_in, NULL);
}

static int
tarui_read_in_new_ascii (TARU * taru, struct new_cpio_header * file_hdr,
				int in_des, enum archive_format archive_format_in, char * buf)
{
  char ascii_header[112];
  int bytesread; 

  if (buf) {
	memcpy (ascii_header, buf, 104);
  } else {
  	if (taru_tape_buffered_read (in_des, ascii_header,104L) != 104) {
		fprintf (stderr,"error in tarui_read_in_new_ascii at 000a\n");
		return -1;
	}
  }
  ascii_header[104] = '\0';
  bytesread=tarui_read_new_ascii_from_buf(taru, file_hdr, ascii_header);
  if (bytesread < 0) {
	fprintf (stderr,"error in tarui_read_in_new_ascii at 000b\n");
  	return -bytesread;
  }

  /* Read file name from input.  */
  ahsStaticSetTarFilenameLength(file_hdr, file_hdr->c_namesize + 1);

  if (buf) {
       if (file_hdr->c_namesize > 406 /* 512 - 104 */) {
		fprintf(stderr, "name too long.\n");
		return -1;
       }
       memcpy(ahsStaticGetTarFilename(file_hdr), buf+104, file_hdr->c_namesize); 
  } else {
       taru_tape_buffered_read (in_des, ahsStaticGetTarFilename(file_hdr), (long) file_hdr->c_namesize);
  	/* fprintf(stderr, "name is %s\n", ahsStaticGetTarFilename(file_hdr)); */
  }
  bytesread += file_hdr->c_namesize;

  /* In SVR4 ASCII format, the amount of space allocated for the header
     is rounded up to the next long-word, so we might need to drop
     1-3 bytes.  */
  return bytesread+=taru_tape_skip_padding (in_des, bytesread + 6, archive_format_in);
}

static int
tarui_read_new_ascii_from_buf(TARU * taru, struct new_cpio_header * file_hdr,  char * ascii_header)
{
  unsigned long xx;
  sscanf (ascii_header,
	  "%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx",
	  &file_hdr->c_ino, &file_hdr->c_mode, &file_hdr->c_uid,
	  &file_hdr->c_gid, &file_hdr->c_nlink, &file_hdr->c_mtime,
	  &xx /*file_hdr->c_filesize*/, 
			(unsigned long*)(&file_hdr->c_dev_maj),
			(unsigned long*)(&file_hdr->c_dev_min),
			(unsigned long*)(&file_hdr->c_rdev_maj),
			(unsigned long*)(&file_hdr->c_rdev_min),
		&file_hdr->c_namesize, &file_hdr->c_chksum);

  /* do some checks in case the magic matched accidentally */
  file_hdr->c_filesize  = xx;
  
  if (file_hdr->c_namesize > 6000 ) return -1;
  return 104;
}
