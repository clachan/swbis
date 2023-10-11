/* uinfile.h: Open a package.
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

#ifndef UINFILE_H_19990304
#define UINFILE_H_19990304

#include "swuser_config.h"
#include <signal.h>
#include "swpath.h"
#include "uxfio.h"
#include "vplob.h"
#include "shcmd.h"
#include "taru.h"
#include "taruib.h"

/*
#define UINFILE_AR_HEADER_SIZE  60
#define UINFILE_AR_SIZE_OFFSET  48
*/

#define UINFILE_COMPRESSED_NA 		-1   /* not applicable */
#define UINFILE_COMPRESSED_NOT 		0    /* Not compressed */
#define UINFILE_COMPRESSED_Z 		1    /* Old Unix Z */
#define UINFILE_COMPRESSED_GZ 		2    /* gzip */
#define UINFILE_COMPRESSED_BZ 		3    /* actually bz2 */
#define UINFILE_COMPRESSED_BZ2 		4
#define UINFILE_COMPRESSED_RPM		5    /* RPM is modeled as a compression type */
#define UINFILE_COMPRESSED_CPIO		6    /* CPIO format is modeled as a compression type */
					     /* for purposes of transparent conversion to tar format */
#define UINFILE_COMPRESSED_DEB		7    /* DEB format is modeled as a compression type */
#define UINFILE_COMPRESSED_LZMA		8    /* LZMA compressed */
#define UINFILE_COMPRESSED_GPG		9    /* GPG encrypted */
#define UINFILE_COMPRESSED_XZ		10   /* XZ compressed */
#define UINFILE_COMPRESSED_SLACK_WITH_NAME 11   /* XZ compressed */

#define UINFILE_IEEE_MAX_LEADING_DIR 	3  /* Maximum no of dirs before .../catalog/INDEX file */

#define UINFILE_NOSAVE 		0
#define UINFILE_SAVE 		1

#define UINFILE_MAGIC_gz 	"\x1f\x8b\x08\x00"
#define UINFILE_MAGIC_Z 	"\x1f\x9d\x90"
#define UINFILE_MAGIC_rpm 	"\xed\xab\xee\xdb"
#define UINFILE_MAGIC_bz2 	"\x42\x5a\x68"
#define UINFILE_MAGIC_lzma 	"\x5d\x00\x00\x80"
#define UINFILE_MAGIC_xz 	"\xfd\x37\x7a\x58"
#define UINFILE_MAGIC_ar 	"!<arch>\n"
#define UINFILE_MAGIC_deb 	UINFILE_MAGIC_ar "debian-binary"
#define UINFILE_MAGIC_binary_tarball	"./"
#define UINFILE_MAGIC_slack	"install"
#define UINFILE_MAGIC_slack_desc	UINFILE_MAGIC_slack "slack-desc"
#define UINFILE_MAGIC_gpg_armor "-----BEGIN PGP"
#define UINFILE_MAGIC_gpg_sym	"\x8c\x0d"
#define UINFILE_MAGIC_gpg_enc1	"\x85\x01"
#define UINFILE_MAGIC_gpg_enc2	"\x85\x02"

#define UINFILE_SWBIS_PATH0	"catalog/swbis-0.2/"
#define UINFILE_SW_MAGIC	"catalog/INDEX"
#define UINFILE_SW_MAGIC1	"catalog/INDEX"
#define UINFILE_SW_MAGIC2	"*/catalog/INDEX"
#define UINFILE_SW_MAGIC_SRC	"*/catalog/INDEX"

#define UINFILE_DEB_CONTROL_OFFSET	132

#define UINFILE_DETECT_2TARRPM       (1 << 0) /* convert a tar/cpio archive to a tar package v0.2 */
#define UINFILE_DETECT_2TAR          (1 << 1) /* convert a tar/cpio archive to an identical tar archive */
#define UINFILE_DETECT_2ARCHIVE      (1 << 2) /* convert a rpm package to its native archive */
#define UINFILE_DETECT_OTARALLOW     (1 << 3) /* if on, allow open to succeed on generic tarball */
#define UINFILE_DETECT_OTARFORCE     (1 << 4) /* if on, force open of a tarball as a generic tarball */
#define UINFILE_DETECT_2ARCHIVERAW   (1 << 5) /* if on, don't decompress the archive */
#define UINFILE_DETECT_FORCEUXFIOFD  (1 << 6) /* return uxfio descriptor instead of forking to return a unix descriptor*/
#define UINFILE_DETECT_FORCEUNIXFD   (1 << 7) /* always return unix fd, not a uxfio fd */
#define UINFILE_DETECT_RETURNFILE    (1 << 8) /* always return fd with the entire file (not missing the first tar header) */
#define UINFILE_DETECT_NATIVE        (1 << 9) /* Open a generic tar or cpio archive.*/
#define UINFILE_DETECT_FORCE_SEEK    (1 << 10) /* */
#define UINFILE_DETECT_IEEE          (1 << 11) 	/* Look for IEEE layout, error if not found. */
#define UINFILE_DETECT_ARBITRARY_DATA 		(1 << 12) /* Treat as arbitrary data. */
#define UINFILE_UXFIO_BUFTYPE_MEM		(1 << 13) 
#define UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM	(1 << 14) 
#define UINFILE_UXFIO_BUFTYPE_FILE		(1 << 15) 
#define UINFILE_DETECT_UNRPM			(1 << 16) /* Convert an RPM to IEEE format */
#define UINFILE_DETECT_UNCPIO			(1 << 17) /* Convert to tar format */
#define UINFILE_DETECT_DEB_DATA			(1 << 18) /* return fd to data.tar.gz */
#define UINFILE_DETECT_DEB_CONTROL		(1 << 19) /* return fd to control.tar.gz */
#define UINFILE_DETECT_DEB_CONTEXT	(1 << 20) /* return fd to control.tar.gz */
#define UINFILE_DETECT_SLACK		(1 << 21) /* return fd to control.tar.gz */
#define UINFILE_DETECT_RECOMPRESS	(1 << 22) /* Construct the re-compress SHCMD command vector */

/* this order is the same as the arf_format enumeration in GNU cpio */
#define UNKNOWN_FILEFORMAT		0	/* arf_unknown		*/
#define BINARY_FILEFORMAT 		1	/* arf_binary		*/
#define CPIO_POSIX_FILEFORMAT		2	/* arf_oldascii		*/
#define CPIO_NEWC_FILEFORMAT		3	/* arf_newascii		*/
#define CPIO_CRC_FILEFORMAT		4	/* arf_crcascii		*/
#define TAR_FILEFORMAT 			5	/* arf_tar		*/
#define USTAR_FILEFORMAT		6	/* arf_ustar		*/
#define HP_OLDASCII_FILEFORMAT		7	/* arf_hpoldascii	*/
#define HP_BINARY_FILEFORMAT		8	/* arf_hpbinary		*/
#define RPMRHS_FILEFORMAT       	9       /* RedHat Format */
#define UINFILE_FILESYSTEM		10	/* the file is a directory in the file system. */
#define DEB_FILEFORMAT			11	/* debian package format */
#define SLACK_FILEFORMAT		12	/* slackware package format */
#define PLAIN_TARBALL_SRC_FILEFORMAT	13	/* common free software source tarball */

#define UINFILE_FILELAYOUT_NA			100	/* not applicable */
#define UINFILE_FILELAYOUT_IEEE			101	/* IEEE 1387.2 layout */
#define UINFILE_FILELAYOUT_UNKNOWN		102	/* Generic Tar or cpio archive */

typedef struct {
   int fdM;
   int underlying_fdM;
   int typeM;		/* Package Format */
   char type_revisionM[12]; /* Package Format Revision */
   int ztypeM;		/* Compression Format */
   int layout_typeM;	/* Layout type */
   int did_dupeM;
   struct new_cpio_header * file_hdrM;
   int pidlistM[20];
   int verboseM;
   int has_leading_slashM;
   SWPATH * swpathM;
   TARU * taruM;
   sigset_t blockmask_;
   sigset_t defaultmask_;
   int current_pos_;
   int deb_file_fd_; /* FIXME redundant with underlying_fdM, fd of ar() archive file */
   int deb_peeked_bytesM[12];
   int n_deb_peeked_bytesM;
   unsigned char * slackheaderM; /* leading ./ archive member */
   char * pathname_prefixM;
   char * slack_nameM;
   VPLOB * recompress_commandsM;
} UINFORMAT;

int     uinfile_get_layout_type(UINFORMAT * uinformat);
int 	uinfile_open          (char * filename, mode_t mode, UINFORMAT ** uinformat, int oflags);
int	uinfile_open_with_name(char * filename, mode_t mode, UINFORMAT ** uinformat, int oflags, char * name);
int 	uinfile_opendup 	(int fd, mode_t mode, UINFORMAT ** uinformat, int oflags);
int	uinfile_close		(UINFORMAT * uinformat);
int 	uinfile_get_ztype 	(UINFORMAT * uinformat);
int 	uinfile_get_type 	(UINFORMAT * uinformat);
SWPATH * uinfile_get_swpath	(UINFORMAT * uinformat);
int 	uinfile_get_has_leading_slash (UINFORMAT * uinformat);
void 	uinfile_set_type 	(UINFORMAT * uinformat, int type);
void 	uinfile_to_oct 		(register long value, register int digits, register char *  where);
int	uinfile_decode_buftype	(int oflags, int v);
char *	uinfile_debug		(UINFORMAT * uin, char * prefix);
int 	uinfile_wait_on_pid	(UINFORMAT * uinformat, int pid, int flag, int * status);
int 	uinfile_wait_on_all_pid (UINFORMAT * uinformat, int flag);
int	uinfile_opendup_with_name(int xdupfd, mode_t mode, UINFORMAT ** uinformat, int oflags, char * name);
SHCMD ** uinfile_get_recompress_vector(UINFORMAT * uinformat);
#endif
