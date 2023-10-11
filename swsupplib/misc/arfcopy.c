/* arfcopy.c - Archive Pass-thru with decode.
 */

/*
 * Copyright (C) 2003  James H. Lowe, Jr.
 * All rights reserved.
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
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "taru.h"
#include "uxfio.h"
#include "strob.h"
#include "swi.h"
#include "swpath.h"
#include "swgp.h"
#include "swlib.h"
#include "taruib.h"
#include "swutilname.h"

int
swlib_arfcopy(XFORMAT * package, SWPATH * swpath, int ofd,
		char * leadingpath, int do_preview, int * deadman)
{
	int format;
	int output_format;
	char * name;
	int ifd;
	int ret;
	int aret;
	int parseret;
	int pathret;
	int retval = -1;
	int depth;
	STROB * resolved_path;
	STROB * namebuf;
	STROB * newnamebuf;
	STROB * tmp;
	long int bytes = 0;
	int tarheaderflags;
	int do_gnu_long_link;
	int namelengthret;
	struct new_cpio_header * file_hdr;


	newnamebuf = strob_open(100);
	namebuf = strob_open(100);
	tmp = strob_open(10);
	resolved_path = strob_open(10);
	tarheaderflags = xformat_get_tarheader_flags(package);
	file_hdr = (struct new_cpio_header*)(xformat_vfile_hdr(package));
	
	output_format = xformat_get_output_format(package);
	format = xformat_get_format(package);
	ifd = xformat_get_ifd(package);
	if (ifd < 0) return -32;		

	xformat_set_ofd(package, ofd);
	while ((ret = xformat_read_header(package)) > 0 &&
			(!deadman || (deadman && *deadman == 0))) {
		if (xformat_is_end_of_archive(package)){
			break;
		}
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);

		strob_strcpy(newnamebuf, leadingpath);
		swlib_unix_dircat(newnamebuf, name);

		/*
		* Make sure its a normal looking posix pathname.
		*/
		parseret = swpath_parse_path(swpath, name);
		if (parseret < 0) {
			retval = -3;
			goto error;
		}

		pathret = swlib_vrealpath("", 
					name, 
					&depth, 
					resolved_path);


		if (depth <= 1 && strstr(name, "..")) {
			/*
			* Somebody tried to sneak in too many ".."
			*/
			retval = -4;
			goto error;
		}
		
		namelengthret = taru_is_tar_filename_too_long(
			strob_str(newnamebuf), 
			tarheaderflags, 
			&do_gnu_long_link, 
			1 /* is_dir */);

		if (namelengthret) {
			/*
			* Name too long for the output format.
			*/
			retval = -5;
			goto error;
		}

		/*
		* FIXME: Need to test symlinks for too many ".."
		*/

		/*
		* Set the New name in the archive.
		*/
		ahsStaticSetTarFilename(file_hdr, strob_str(newnamebuf));
	
		aret = xformat_write_header(package);
		if (aret <= 0) goto error;
		bytes += aret;
		
		aret = xformat_copy_pass(package, ofd, ifd);
		if (aret < 0) goto error;
		bytes += aret;
	}
	if (deadman && *deadman) return -1;
	/*
	* write out the trailer.
	*/
	aret = taru_write_archive_trailer(
				package->taruM, 
				output_format, 
				ofd, 
				512, 
				(int)bytes, 
				tarheaderflags);

	retval = 0;
	if (aret <= 0) {
		retval = -2;
	}
error:
	strob_close(resolved_path);
	strob_close(tmp);
	strob_close(namebuf);
	return retval;
}

int
swlib_audit_distribution(XFORMAT * xformat, 
		int do_re_encode, 
		int ofd, 
		uintmax_t *pstatbytes, 
		int * deadman,
		void (*alarm_handler)(int))
{
	int ret;
	int retval;
	SWI * swi;
	SWPATH * swpath;
	UINFORMAT * uinformat;
	SWVARFS * swvarfs;
	int ifd;
	SWI_DISTDATA  distdataO;

	swvarfs = xformat_get_swvarfs(xformat);
	uinformat = swvarfs_get_uinformat(swvarfs);
	swpath = uinfile_get_swpath(uinformat);
        swpath_reset(swpath);
	xformat_set_tarheader_flag(xformat, TARU_TAR_FRAGILE_FORMAT, 1 /*on*/ );

	if (uinfile_get_layout_type(uinformat) == UINFILE_FILELAYOUT_IEEE) {
		swi = swi_create();
		swi->xformatM = xformat;
		swi->xformatM = xformat; 
		swi->swvarfsM = swvarfs; 
		swi->uinformatM = uinformat;
		swi->swpathM = swpath;   
		ret = swi_decode_catalog(swi);
		/*
		* FIXME : check ret
		*/
		swi_distdata_resolve(swi, &distdataO, 0);
		swvarfs_uxfio_fcntl(swvarfs, UXFIO_F_ARM_AUTO_DISABLE, 1);
		ifd = xformat_get_ifd(xformat);	
	} else {
		swvarfs_uxfio_fcntl(swvarfs, UXFIO_F_ARM_AUTO_DISABLE, 1);
		ifd = xformat_get_ifd(xformat);	
	}
	if (uxfio_lseek(ifd, 0, SEEK_SET) != 0) {
		fprintf(stderr,
			"uxfio_lseek error: %s : %d\n", __FILE__, __LINE__);
		exit(2);
	}
        swpath_reset(swpath);

	if (do_re_encode) {
		swgp_signal_block(SIGALRM, (sigset_t *)NULL);
		retval = swlib_arfcopy(xformat, swpath, ofd, "", 0, deadman);
		swgp_signal_unblock(SIGALRM, (sigset_t *)NULL);
	} else {	
		xformat_set_pass_fd(xformat, ofd);
		retval = taruib_arfcopy((void*)xformat, (void*)swpath,
				ofd, "", 0, pstatbytes, deadman, alarm_handler);
	}

	return retval;
}
