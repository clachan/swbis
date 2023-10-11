 /* mtar.c - tar/cpio routines */
   
/*
   Copyright (C) 2003 Jim Lowe
   All Rights Reserved.

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
#include <string.h>
#include <unistd.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <tar.h>
#include "system.h"
#include "swlib.h"
#include "to_oct.h"
#include "filetypes.h"
#include "cpiohdr.h"
#include "tarhdr.h"
#include "ahs.h"
#include "taru.h"
#include "uxfio.h"
#include "swutilname.h"

#define TARRECORDSIZE 512
#define FALSE 0
#define TRUE 1

#define TARNEEDDEBUG 1
#undef TARNEEDDEBUG
#define TARNEEDDEBUG 1

#include "debug_config.h"
#ifdef TARNEEDDEBUG
#define TAR_E_DEBUG(format) SWBISERROR("TAR DEBUG: ", format)
#define TAR_E_DEBUG2(format, arg) SWBISERROR2("TAR DEBUG: ", format, arg)
#define TAR_E_DEBUG3(format, arg, arg1) SWBISERROR3("TAR DEBUG: ", format, arg, arg1)
#else
#define TAR_E_DEBUG(arg)
#define TAR_E_DEBUG2(arg, arg1)
#define TAR_E_DEBUG3(arg, arg1, arg2)
#endif /* TARNEEDDEBUG */

char *
taru_dup_tar_linkname(void * pt)
{
	char * tar_hdr = (char *)pt;
	char * hold_tar_linkname = (char *)malloc(101);
	strncpy(hold_tar_linkname, tar_hdr + 157, TARLINKNAMESIZE);
	hold_tar_linkname[TARLINKNAMESIZE] = '\0';
	return hold_tar_linkname;
}

static char *
stash_tar_filename(char *prefix, char *filename)
{
	char * hold_tar_filename;
	static STROB * htf;
	if (!htf) htf = strob_open(512);

	hold_tar_filename = strob_str(htf);

	if (prefix == NULL || *prefix == '\0') {
		strncpy(hold_tar_filename, filename, TARNAMESIZE);
		hold_tar_filename[TARNAMESIZE] = '\0';
	} else {
		strncpy(hold_tar_filename, prefix, TARPREFIXSIZE);
		hold_tar_filename[TARPREFIXSIZE] = '\0';
		strcat(hold_tar_filename, "/");
		strncat(hold_tar_filename, filename, TARNAMESIZE);
		hold_tar_filename[TARPREFIXSIZE + TARNAMESIZE] = '\0';
	}
	return hold_tar_filename;
}

char *
taru_dup_tar_name(void * pt)
{
		char * tar_hdr = (char *)pt;
		if (strncmp(tar_hdr + 257,TMAGIC,5))
			return stash_tar_filename(NULL, tar_hdr);
		else
			return stash_tar_filename(tar_hdr+345, tar_hdr);
}

int 
taru_is_tar_linkname_too_long(char *name, int tarheaderflags, int * p_do_gnu_long_link)
{

	int too_long = 1;
	int do_gnu_longlinks = 0;
	int do_exthdr = 0;
	do_gnu_longlinks = tarheaderflags & TARU_TAR_GNU_LONG_LINKS;
	do_exthdr = tarheaderflags & TARU_TAR_PAXEXTHDR;

	if (p_do_gnu_long_link) *p_do_gnu_long_link = 0;
	if (strlen(name) <= TARLINKNAMESIZE) {
		too_long = 0;
	} else if (strlen(name) > TARLINKNAMESIZE) {
		if (do_gnu_longlinks) {
			if (strlen(name) < TARRECORDSIZE) {
				too_long = 0;
				if (p_do_gnu_long_link) *p_do_gnu_long_link = 1;
			} else {
				/* FIXME, support using gnulonglinks for links
 				 * greater than 512
 				 */
				;
			}
		} else if (do_exthdr) {
			/* Pax extended headers */
			too_long = 0;
		} else {
			;
		}
	} else {
		;
		/* too long, FIXME, support names longer than 512 in gnu longlink */	
	}
	return too_long;
}


/* Return TRUE if the filename is too long to fit in a tar header.
   For old tar headers, if the filename's length is less than or equal
   to 100 then it will fit, otherwise it will not.  For POSIX tar headers,
   if the filename's length is less than or equal to 100 then it
   will definitely fit, and if it is greater than 256 then it
   will definitely not fit.  If the length is between 100 and 256,
   then the filename will fit only if it is possible to break it
   into a 155 character "prefix" and 100 character "name".  There
   must be a slash between the "prefix" and the "name", although
   the slash is not stored or counted in either the "prefix" or
   the "name", and there must be at least one character in both
   the "prefix" and the "name".  If it is not possible to break down
   the filename like this then it will not fit.  */

int 
taru_is_tar_filename_too_long(char *name, int tarheaderflags,
				int * p_do_gnu_long_link, int is_dir)
{
	int retval=1;
	int whole_name_len;
	char *p;
	int did_squash = 0;
	int tarnamesize = TARNAMESIZE;
	int do_gnu_longlinks = 0;
	int do_exthdr = 0;

	if (strlen(name) > 1 && name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = '\0';
		did_squash = 1;
	}

	do_gnu_longlinks = tarheaderflags & TARU_TAR_GNU_LONG_LINKS;
	do_exthdr = tarheaderflags & TARU_TAR_PAXEXTHDR;

	if (
		(
			do_gnu_longlinks &&
			(tarheaderflags & TARU_TAR_NAMESIZE_99)
		)
		||
		(
			(tarheaderflags & TARU_TAR_GNU_OLDGNUTAR)
		)
	)
	{
		tarnamesize = TARNAMESIZE - 1;
	} else {
		tarnamesize = TARNAMESIZE;
	}

	if (is_dir) tarnamesize--;

	if (p_do_gnu_long_link) *p_do_gnu_long_link = 0;
	whole_name_len = strlen(name);
	
	if (whole_name_len < tarnamesize) {
		/*
		 * This is a simple short name
		 */
		retval = 0;
	} else if (
			(whole_name_len > (tarnamesize + TARPREFIXSIZE + 1)) &&
			(do_gnu_longlinks == 0 && do_exthdr == 0)
	) {
		/*
		 * Really, really too long
		 * too bad.
		 */
		retval = 1;
	} else  {
		/*
	 	 * Ok, try to use the ustar header prefix area.
		 */
		char *oldp;
		char * start = name;

		/*
		 *   /[^/]{99}   OK
		 *   /[^/]{100}  too long
		 */

		p = oldp = start;
		while(*p != '\0') {
			while (*p != '/' && *p != '\0') ++p; 
			if (*p == '\0') break;	
			if (
				((p - start) < TARPREFIXSIZE) && 
				(((int)(strlen(p) - 1) <= tarnamesize) &&
				p != start)
			) {
				/*
				* Good split.
				*/
				retval = 0;
				break;
			} else {
				p++;
			}
		}
		if (*p == '\0') retval = 1;
	}	
	

	if (retval) {
		if (do_gnu_longlinks) {
			/*
			 * Use Gnu long link if required.
			 */
			if (whole_name_len >= 512) {
				/*
				 * Filenames longer than 512 bytes not supported.
				 */
				fprintf(stderr,
					"tar filename too long, location = 3\n");
				retval = 1;
			} else if (whole_name_len > tarnamesize) {
				/*
				 * Use GNU long link. This must match the GNU tar policy.
				 */
				if (p_do_gnu_long_link) *p_do_gnu_long_link = 1;
				retval = 0;
			} else {
				/*
				 */
				retval = 0;
			}
		} else if (do_exthdr) {
			if (p_do_gnu_long_link) *p_do_gnu_long_link = 0;
			retval = 0;
		}
	}
	if (did_squash) name[strlen(name)] = '/';
	return retval;
}

int
taru_filehdr2statbuf(struct stat *statbuf, struct new_cpio_header *file_hdr)
{
	gid_t gid;
	uid_t uid;
	time_t tim = time(NULL);

	statbuf->st_mode = (mode_t) (file_hdr->c_mode);
	statbuf->st_ino = (ino_t) (file_hdr->c_ino);
	statbuf->st_dev = (dev_t)(makedev(file_hdr->c_dev_maj,\
						file_hdr->c_dev_min));
	statbuf->st_rdev = (dev_t)(makedev(file_hdr->c_rdev_maj,\
						 file_hdr->c_rdev_min));
	statbuf->st_nlink = (nlink_t) (file_hdr->c_nlink);
	statbuf->st_uid = (uid_t) (file_hdr->c_uid);
	statbuf->st_gid = (gid_t) (file_hdr->c_gid);

	statbuf->st_size = (off_t)taru_hdr_get_filesize(file_hdr);

	statbuf->st_atime = (time_t) (tim);
	statbuf->st_ctime = (time_t) (tim);
	statbuf->st_mtime = (time_t) (file_hdr->c_mtime);
	statbuf->st_blksize = (long) (512);
	statbuf->st_blocks = (long) ((statbuf->st_size) / 512 + 1);

	if (file_hdr->c_cu == TARU_C_BY_UNAME) {
		if (taru_get_gid_by_name(ahsStaticGetTarGroupname(file_hdr),
							&gid) == 0) {
			statbuf->st_gid = (gid);
		}
	}
	if (file_hdr->c_cg == TARU_C_BY_GNAME) {
		if (taru_get_uid_by_name(ahsStaticGetTarUsername(file_hdr),
							&uid) == 0) {
			statbuf->st_uid = (uid);
		}
	}
	return 0;
}

int
taru_filehdr2filehdr(struct new_cpio_header *file_hdr_dst,
				struct new_cpio_header *file_hdr_src)
{

	file_hdr_dst->c_magic = file_hdr_src->c_magic;
	file_hdr_dst->c_ino = file_hdr_src->c_ino;
	file_hdr_dst->c_mode = file_hdr_src->c_mode;

	file_hdr_dst->c_uid = file_hdr_src->c_uid;
	file_hdr_dst->c_gid = file_hdr_src->c_gid;
	file_hdr_dst->c_nlink = file_hdr_src->c_nlink;
	file_hdr_dst->c_mtime = file_hdr_src->c_mtime;
	file_hdr_dst->c_ctime = file_hdr_src->c_ctime;
	file_hdr_dst->c_ctime_nsec = file_hdr_src->c_ctime_nsec;
	file_hdr_dst->c_atime = file_hdr_src->c_atime;
	file_hdr_dst->c_atime_nsec = file_hdr_src->c_atime_nsec;
	file_hdr_dst->c_filesize = file_hdr_src->c_filesize;

	file_hdr_dst->c_dev_maj = file_hdr_src->c_dev_maj;
	file_hdr_dst->c_dev_min = file_hdr_src->c_dev_min;
	file_hdr_dst->c_rdev_maj = file_hdr_src->c_rdev_maj;
	file_hdr_dst->c_rdev_min = file_hdr_src->c_rdev_min;
	file_hdr_dst->c_namesize = file_hdr_src->c_namesize;
	file_hdr_dst->c_chksum = file_hdr_src->c_chksum;
	file_hdr_dst->c_cu = file_hdr_src->c_cu;
	file_hdr_dst->c_cg = file_hdr_src->c_cg;
	file_hdr_dst->c_is_tar_lnktype = file_hdr_src->c_is_tar_lnktype;

	ahsStaticSetPaxLinkname(file_hdr_dst,
					ahsStaticGetTarLinkname(file_hdr_src));
	ahsStaticSetTarFilename(file_hdr_dst,
					ahsStaticGetTarFilename(file_hdr_src));
	ahsStaticSetTarUsername(file_hdr_dst,
					ahsStaticGetTarUsername(file_hdr_src));
	ahsStaticSetTarGroupname(file_hdr_dst,
					ahsStaticGetTarGroupname(file_hdr_src));
	if (file_hdr_src->extattlistM)
		((CPLOB*)(file_hdr_src->extattlistM))->refcountM++;
	file_hdr_dst->extattlistM = file_hdr_src->extattlistM;
	return 0;
}

int
taru_statbuf2filehdr (struct new_cpio_header *file_hdr,
			struct stat *statbuf, char * sourcefilename,
			char *filename, char *linkname) {
	char symbuf[1024];
	int amount;

	file_hdr->c_mode = (statbuf->st_mode & 07777);

	if (!sourcefilename) {
		sourcefilename = filename;
	}

	if (S_ISREG(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFREG;
	else if (S_ISDIR(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFDIR;
#ifdef S_ISBLK
	else if (S_ISBLK(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFBLK;
#endif
#ifdef S_ISCHR
	else if (S_ISCHR(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFCHR;
#endif
#ifdef S_ISFIFO
	else if (S_ISFIFO(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFIFO;
#endif
#ifdef S_ISDOOR
	else if (S_ISDOOR(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFDOOR;
#endif
#ifdef S_ISLNK
	else if (S_ISLNK(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFLNK;
#endif
#ifdef S_ISSOCK
	else if (S_ISSOCK(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFSOCK;
#endif
#ifdef S_ISNWK
	else if (S_ISNWK(statbuf->st_mode))
		file_hdr->c_mode |= CP_IFNWK;
#endif
	/* --------------------------------------------- */

	file_hdr->c_ino = statbuf->st_ino;
	file_hdr->c_nlink = statbuf->st_nlink;
	file_hdr->c_uid = statbuf->st_uid;
	file_hdr->c_gid = statbuf->st_gid;
	file_hdr->c_filesize = (uintmax_t)(statbuf->st_size);
	file_hdr->c_mtime = statbuf->st_mtime;
	file_hdr->c_ctime = statbuf->st_ctime;
	file_hdr->c_atime = statbuf->st_atime;
	file_hdr->c_atime_nsec = 0;
	file_hdr->c_ctime_nsec = 0;
	file_hdr->c_dev_maj = major(statbuf->st_dev);
	file_hdr->c_dev_min = minor(statbuf->st_dev);
	file_hdr->c_rdev_maj = major(statbuf->st_rdev);
	file_hdr->c_rdev_min = minor(statbuf->st_rdev);
	
	if (filename) {
		ahsStaticSetTarFilename(file_hdr, filename);
		file_hdr->c_namesize = strlen(filename) + 1;
	} else {
		ahsStaticSetTarFilename(file_hdr, "");
		file_hdr->c_namesize = 1;
	}

	if (filename && strlen(filename)) {
		if (S_ISLNK(statbuf->st_mode) &&
				linkname && !strlen(linkname)) {
			amount = readlink(sourcefilename, symbuf,
						sizeof(symbuf));
			if (amount < 0) {
				fprintf(stderr, "readlink failed on %s\n",
							filename);
				return -1;
			}
			if (amount > 100) {
				fprintf(stderr, 
					"warning: linkname too long for tar format: %s\n",
						sourcefilename);
				/*
				* If this limit is lifted check all 
				* usages for buffer overrun.
				*/
				/* return -1; */
			}
			strncpy(linkname, symbuf, amount);
			linkname[amount] = '\0';
			ahsStaticSetPaxLinkname(file_hdr, linkname);
		} else if (S_ISLNK(statbuf->st_mode) &&
				 linkname && strlen(linkname)) {
			ahsStaticSetPaxLinkname(file_hdr, linkname);
		} else {
			/*
			* Clear the file_hdr linkname.
			*/
			ahsStaticSetPaxLinkname(file_hdr, linkname);
		}
	}
	file_hdr->c_is_tar_lnktype = -1;
	return 0;
}

int
taru_set_tar_header_sum(struct tar_header * fp_tar_hdr, int tar_iflags)
{
	int termch;
	unsigned long sum;
	struct tar_header * tar_hdr;
	int tar_iflags_like_star;
	int tar_iflags_like_pax;
	
	/* tar_iflags = taru->taru_tarheaderflagsM; */
	tar_iflags_like_star = (tar_iflags & TARU_TAR_BE_LIKE_STAR);
	tar_iflags_like_pax = (tar_iflags & TARU_TAR_BE_LIKE_PAX);
	
	if (tar_iflags_like_star) {
		termch = '\040';  /* space */
	} else {
		termch = 0;   /* NUL */
	}
	
	if (fp_tar_hdr) {
		tar_hdr = fp_tar_hdr;
	} else {
		fprintf(stderr, "%s: internal error in taru_set_header_sum\n", swlib_utilname_get());
		exit(34);
		/* tar_hdr = (struct tar_header *)(strob_str(taru->headerM)); */
	}


	sum = taru_tar_checksum(tar_hdr);

	if (tar_iflags_like_pax || tar_iflags_like_star) {
		/* This mimics pax v3.0 */
		uintmax_to_chars(sum, tar_hdr->chksum, 8, POSIX_FORMAT, termch);
	} else {
		uintmax_to_chars(sum, tar_hdr->chksum, 7, POSIX_FORMAT, termch);
	}
	return 0;
}

int
taru_set_new_linkname(TARU * taru, struct tar_header * fp_tar_hdr, char * name)
{
	int retval = 0;
	struct tar_header * tar_hdr;
	if (strlen(name) > sizeof(tar_hdr->linkname)) {
		retval = -1;
	}
	if (fp_tar_hdr)
		tar_hdr = fp_tar_hdr;
	else
		tar_hdr = (struct tar_header *)(strob_str(taru->headerM));
	memset(tar_hdr->linkname, '\0', sizeof(tar_hdr->linkname));
	strncpy(tar_hdr->linkname, name, sizeof(tar_hdr->linkname));
	return retval;
}

int
taru_set_new_name(/* TARU * taru */ struct tar_header * fp_tar_hdr, int len, char * fpname, int tar_iflags)
{
	int ret;
	struct tar_header * tar_hdr;
	char * name = fpname;
	STROB * tmp = NULL;
	/* int tar_iflags; */

	if (strstr(fpname, "\\x")) {
		tmp = strob_open(64);
		strob_strcpy(tmp, name);
		swlib_process_hex_escapes(strob_str(tmp));
		name = strob_str(tmp);
	}

	/*
	if (len <= 0) len = taru->header_lengthM;
	if (len > TARRECORDSIZE) {
		/ *
		* handle extended headers here
		* /
		SWLIB_INTERNAL("no extended header support");
		return -1;
	}
	*/


	if (fp_tar_hdr) {
		tar_hdr = fp_tar_hdr;
	} else {
		fprintf(stderr, "%s: internal error in taru_set_new_name\n", swlib_utilname_get());
		exit(33);
		/* tar_hdr = (struct tar_header *)(strob_str(taru->headerM)); */
	}


	/* tar_iflags = taru->taru_tarheaderflagsM; */

        if (strlen(name) <= TARNAMESIZE) {
                strncpy(tar_hdr->name, name, TARNAMESIZE);
		ret = 0;
        } else {
		ret = taru_split_name_ustar(tar_hdr,
				name,
				tar_iflags);
		if (ret) {
			fprintf(stderr, "%s: error splitting name\n", swlib_utilname_get());
			return ret;
		}
	}
	taru_set_tar_header_sum(fp_tar_hdr, tar_iflags);
	if (tmp) strob_close(tmp);
	return ret;
}

int
taru_write_long_link_member(TARU * taru, int out_file_des,
			char * filename, int gnu_long_type, unsigned long c_type, int tarheaderflags)
{
	int ret;
	int is_dir;
	struct stat foo;
	char namebuf[512];
	struct new_cpio_header * file_hdr = ahsStaticCreateFilehdr();
	int do_add_trailing_slash = 0;

	is_dir = (c_type & CP_IFMT) == CP_IFDIR;

	E_DEBUG2("filename=[%s]", filename);
	E_DEBUG2("type dir : [%d]", (c_type & CP_IFMT) == CP_IFDIR);
	E_DEBUG2("type reg : [%d]", (c_type & CP_IFMT) == CP_IFREG);
	E_DEBUG2("type sym : [%d]", (c_type & CP_IFMT) == CP_IFLNK);

	if (is_dir && strlen(filename) &&
		filename[strlen(filename) - 1] != '/' &&
					strlen(filename) < 511 ) {
		do_add_trailing_slash = 1;
	}

	if (strlen(filename) >= 510) {
		/* FIXME don't support real long names at this time */
		return -1;
	}

	memset(&foo, '\0', sizeof(foo));

	taru_statbuf2filehdr(file_hdr, &foo, NULL, NULL, "");

	if (
		(c_type & CP_IFMT) == CP_IFREG ||
		(c_type & CP_IFMT) == CP_IFDIR
	) {
		ahsStaticSetTarFilename(file_hdr, GNU_LONG_LINK);
		if (gnu_long_type == GNUTYPE_LONGNAME) {
			ahsStaticSetPaxLinkname(file_hdr, "");
		} else {
			/* hard link */
			ahsStaticSetPaxLinkname(file_hdr, filename);
		}
		file_hdr->c_mode = 0; 
	} else {
		ahsStaticSetTarFilename(file_hdr, GNU_LONG_LINK);
		ahsStaticSetPaxLinkname(file_hdr, filename);
		file_hdr->c_mode = c_type;
	}

	file_hdr->c_namesize = strlen(GNU_LONG_LINK) + 1;
	file_hdr->c_ino = 0;
	file_hdr->c_nlink = 0;
	file_hdr->c_uid = 0;
	file_hdr->c_gid = 0;
	file_hdr->c_filesize = strlen(filename) + 1 + do_add_trailing_slash;
	file_hdr->c_mtime = 0;
	file_hdr->c_dev_maj = 0;
	file_hdr->c_dev_min = 0;
	file_hdr->c_rdev_maj = 0;

	ret = taru_write_out_tar_header2(taru, file_hdr, out_file_des,
				(char*)NULL, "root", "root", tarheaderflags);

	if (ret == 512) {
		memset(namebuf, '\0', sizeof(namebuf));	
		strncpy(namebuf, filename, sizeof(namebuf));
		namebuf[sizeof(namebuf) - 1] = '\0';
		if (do_add_trailing_slash) {
			namebuf[strlen(namebuf)] = '/';
		}
		if (taru_safewrite(out_file_des, namebuf, 512) !=512) {
			;
		} else {
			ret += 512;
		}
	} else {
		/* error */
		return -1;
	}
	ahsStaticDeleteFilehdr(file_hdr);
	return ret;
}
