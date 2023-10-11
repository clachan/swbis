/* uxfio_i.h:  buffered u*ix I/O functions.
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

#ifndef uxfio_i_20020419_h
#define uxfio_i_20020419_h

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "atomicio.h"

#define UXFIO_I_NULL_FD -10001

#define Xint intmax_t

typedef struct {
	int uxfdM;		/* unix file descriptor */
	int buffertypeM;	/* memmory segment, disk file, dynamic (growable) memory */
	int uxfd_can_seekM;	/* 1 if seekable file */
	Xint posM;		/* pointer in buf , always >=start AND <end */
	Xint startM;		/* beginning offset in buf of valid data, always 0 */
	Xint endM;		/* pointer offset of first byte of invalid data */
	Xint lenM;		/* length of buffer, memory length, or, maximum file length */
	int errorM;		/* error */
	Xint bytesreadM;		/* bytes read on virtual fd */
	Xint current_offsetM;	/* curent offset in physical/logical file.*/
	Xint virtual_offsetM;	/* curent offset in physical/logical file.*/
	int buffer_activeM;	/* 1 if any buffertype is in use */
	char *bufM;		/* stream buffer for buffertype 0 */
	int buffdM;		/* file buffer for buffertype 1 */
	Xint offset_eof_savedM;	/* saved value of offset_eofM */
	Xint offset_eofM;	/* virtual end-of marker for the uxfio_read function */
	Xint offset_bofM;	/* virtual beginning of file. */
	int auto_disableM;	/* if a subsequent read() will read from uxfd entirely, set buffer_active to 0 */
	int auto_arm_delayM;	/* only disable buffer if auto_disable is set AND have previosly read from buffer */
	int write_insertM;	/* true if inserting into a file. see UXFIO_F_WRITE_INSERT */
	mode_t uxfio_modeM;
	Xint v_endM;		/* length of file */
	int use_countM;
	char * tmpfile_rootdirM;
	char buffilenameM[128];
	struct stat * statbufM;
	int did_dupe_fdM;
	int (*vir_fsyncM)(int  filedes);
	ssize_t (*vir_readM)(int  filedes, void * buf, size_t nbytes);
	ssize_t (*vir_tread_readM)(int  filedes, void * buf, size_t nbytes);
	ssize_t (*vir_writeM)(int  filedes, void * buf, size_t nbytes);
	int (*vir_ftruncateM)(int  filedes,  off_t nbytes);
	int (*vir_closeM)(int  filedes);
	off_t (*vir_lseekM)(int  filedes, off_t offset, int whence);
	int uxfio_fildesM;	/* Uxfio descriptor of this structure */
	int lock_buf_fatalM;     /* If realloc shifts the address of the file image, then exit */
	int output_block_sizeM;	
	Xint output_buffer_cM;		/* output buffer data length */
	char * output_bufferM;
} UXFIO;

UXFIO * uxfio_debug_get_object_address(int fd);

#endif
