/* Extended cpio header from POSIX.1.
   Copyright (C) 1992 Free Software Foundation, Inc.

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

#ifndef _CPIOHDR_H

#define _CPIOHDR_H 1

#include "swuser_config.h"
/* -------------------------------------- */

/* A cpio archive consists of a sequence of files.
   Each file has a 76 byte header,
   a variable length, NUL terminated filename,
   and variable length file data.
   A header for a filename "TRAILER!!!" indicates the end of the archive.  */

/* All the fields in the header are ISO 646 (approximately ASCII) strings
   of octal numbers, left padded, not NUL terminated.

   Field Name	Length in Bytes	Notes
   c_magic	6		must be "070707"
   c_dev	6
   c_ino	6
   c_mode	6		see below for value
   c_uid	6
   c_gid	6
   c_nlink	6
   c_rdev	6		only valid for chr and blk special files
   c_mtime	11
   c_namesize	6		count includes terminating NUL in pathname
   c_filesize	11		must be 0 for FIFOs and directories  */

/* Values for c_mode, OR'd together: */

#define C_IRUSR		000400
#define C_IWUSR		000200
#define C_IXUSR		000100
#define C_IRGRP		000040
#define C_IWGRP		000020
#define C_IXGRP		000010
#define C_IROTH		000004
#define C_IWOTH		000002
#define C_IXOTH		000001

#define C_ISUID		004000
#define C_ISGID		002000
#define C_ISVTX		001000

#define C_ISBLK		060000
#define C_ISCHR		020000
#define C_ISDIR		040000
#define C_ISFIFO	010000
#define C_ISSOCK	0140000
#define C_ISLNK		0120000
#define C_ISCTG		0110000
#define C_ISREG		0100000


struct old_cpio_header
{
  unsigned short c_magic;
  short c_dev;
  unsigned short c_ino;
  unsigned short c_mode;
  unsigned short c_uid;
  unsigned short c_gid;
  unsigned short c_nlink;
  short c_rdev;
  unsigned short c_mtimes[2];
  unsigned short c_namesize;
  unsigned short c_filesizes[2];
  unsigned long c_mtime;	/* Long-aligned copy of `c_mtimes'. */
  unsigned long c_filesize;	/* Long-aligned copy of `c_filesizes'. */
  char *c_name;
};

/* "New" portable format and CRC format:

   Each file has a 110 byte header,
   a variable length, NUL terminated filename,
   and variable length file data.
   A header for a filename "TRAILER!!!" indicates the end of the archive.  */

/* All the fields in the header are ISO 646 (approximately ASCII) strings
   of hexadecimal numbers, left padded, not NUL terminated.

   Field Name	Length in Bytes	Notes
   c_magic	6		"070701" for "new" portable format
				"070702" for CRC format
   c_ino	8
   c_mode	8
   c_uid	8
   c_gid	8
   c_nlink	8
   c_mtime	8
   c_filesize	8		must be 0 for FIFOs and directories
   c_maj	8
   c_min	8
   c_rmaj	8		only valid for chr and blk special files
   c_rmin	8		only valid for chr and blk special files
   c_namesize	8		count includes terminating NUL in pathname
   c_chksum	8		0 for "new" portable format; for CRC format
*/

#define EXATT_NAMESPACE_LEN 100
#define EXATT_PREFIX_LEN 100

typedef struct {
	char magicM[3];
	int is_in_useM;
	char vendorM[EXATT_PREFIX_LEN];  /* e.g. SCHILY, RHT, must by capatilized */
	char namespaceM[EXATT_NAMESPACE_LEN];  /* context, e.g.: acl, selinux */
	void * attrnameM;   		/* Attribute Name    STROB object */
	void * attrvalueM;  		/* Storage of the Value  STROB object */
	int lenM;       		/* Length of the Value */
} EXATT;

typedef struct {
	int do_poisonM;
	char md5[33];  /* ascii md5 */
	short int do_md5;
	char sha1[41];  /* ascii sha1 */
	short int do_sha1;
	char sha512[129];  /* ascii sha512 */
	short int do_sha512;
	char size[32];  /* ascii sha512 */
	short int do_size;
} FILE_DIGS;

#define DIGS_ENABLE_OFF 0
#define DIGS_ENABLE_ON 1
#define DIGS_ENABLE_POISON 2
#define DIGS_ENABLE_CLEAR -1
#define DIGS_DO_POISON 1

#define TARU_C_ID_UNSET ULONG_MAX

struct new_cpio_header
{
  unsigned short c_magic;
  unsigned long c_ino;
  unsigned long c_mode;
  unsigned long c_uid;
  unsigned long c_gid;
  unsigned long c_nlink;
  unsigned long c_mtime;
  uintmax_t c_filesize;
  unsigned long c_dev_maj;
  unsigned long c_dev_min;
  unsigned long c_rdev_maj;
  unsigned long c_rdev_min;
  unsigned long c_namesize;
  unsigned long c_chksum;
  char *c_name;			/* NOTE: In most contexts this is a (STROB*) object. */
  char *c_tar_linkname;		/* NOTE: In most contexts this is a (STROB*) object. */
  char *c_username;		/* NOTE: In most contexts this is a (STROB*) object. */
  char *c_groupname;		/* NOTE: In most contexts this is a (STROB*) object. */
  unsigned char c_cu;		/* Control code for uid/user lookup policy.  Deprecated? or should be */
  unsigned char c_cg;		/* Control code for gid/group lookup policy.  Deprecated? or should be */
  char c_is_tar_lnktype;
  unsigned char usage_maskM;    /* bit field that tells what has been set from various sources */
  int extHeader_usage_maskM;    /* bit field that tells what has been set from the Extended Headers */
  int extHeader_needs_maskM;    /* bit field that tells what has to be put in an Extended Headers */
  FILE_DIGS * digsM; 		/* file digest object */
  void * extattlistM;		/* actually a CPLOB object, list of EXATT objects */
  unsigned long c_atime;
  unsigned long c_atime_nsec;   /* nano second part */
  unsigned long c_ctime;
  unsigned long c_ctime_nsec;   /* nano second part */
};

#define TARU_UM_MODE		(1 << 0)  /* These are the bits of usage_maskM */
#define TARU_UM_UID		(1 << 1)
#define TARU_UM_GID		(1 << 2)
#define TARU_UM_OWNER		(1 << 3)
#define TARU_UM_GROUP		(1 << 4)
#define TARU_UM_MTIME		(1 << 5)
#define TARU_UM_IS_VOLATILE	(1 << 6)


#define TARU_EHUM_ATIME		(1 << 0) /* These are the bits of extHeader_usage_maskM */
#define TARU_EHUM_CHARSET	(1 << 1) /* and extHeader_needs_maskM */
#define TARU_EHUM_COMMENT	(1 << 2) /* They tell what has been set in the extended headers */
#define TARU_EHUM_CTIME		(1 << 3)
#define TARU_EHUM_GID		(1 << 4)
#define TARU_EHUM_GNAME		(1 << 5)
#define TARU_EHUM_LINKPATH	(1 << 6)
#define TARU_EHUM_MTIME		(1 << 7)
#define TARU_EHUM_PATH		(1 << 8)
#define TARU_EHUM_REALTIMEANY	(1 << 9)
#define TARU_EHUM_SECURITYANY	(1 << 10)
#define TARU_EHUM_SIZE		(1 << 11)
#define TARU_EHUM_UID		(1 << 12)
#define TARU_EHUM_UNAME		(1 << 13)
#define TARU_EHUM_ANY		(1 << 14)
#define TARU_EHUM_DEV_MAJ	(1 << 15)
#define TARU_EHUM_DEV_MIN	(1 << 16)

#endif /* cpiohdr.h */
