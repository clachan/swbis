/* pcopyin.c - cpio2tar translator function

   Copyright (C) 1998, 1999  Jim Lowe

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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>
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
#include "taru_util.h"
#include "swlib.h"
#include "strob.h"
#include "ahs.h"
#include "hllist.h"

#ifndef HAVE_LCHOWN
#define lchown chown
#endif


static struct new_cpio_header *file_hdr_copy(struct new_cpio_header *dst, struct new_cpio_header *src);

static void
delete_trio(struct new_cpio_header *f1, struct new_cpio_header *f2, struct new_cpio_header *f3)
{
	ahsStaticDeleteFilehdr(f1);
	ahsStaticDeleteFilehdr(f2);
	ahsStaticDeleteFilehdr(f3);
}


/* read any format on input_fd, write a tar archive on output_fd */
int
taru_process_copy_in(TARU * fp_taru, int i_fd, int output_fd)
{
	char done = FALSE;	/* True if trailer reached.  */
	char magbuf[10];
	char done_links = FALSE;	/* True if trailer reached.  */
	struct utimbuf times;	/* For setting file times.  */
	struct new_cpio_header * file_hdr = ahsStaticCreateFilehdr();	/* Output header information.  */
	struct new_cpio_header * file_hdr_links = ahsStaticCreateFilehdr();
	struct new_cpio_header * file_hdr_links_last = ahsStaticCreateFilehdr();
	int input_fd;
	int found_data;
	char skip_file;		/* Flag for use with patterns.  */
	int existing_dir;	/* True if file is a dir & already exists.  */
	char *link_name = NULL;	/* Name of hard and symbolic links.  */
	UINFORMAT *uinfile;
	int convert_flags = 0;
	char zeros_512[512];
	enum archive_format archive_format_in = arf_unknown;
	HLLIST *link_record = NULL;
	int orig_offset, nfound, nmore, count, data_offset, data_size = -1;
	dev_t devno;
	hllist_entry * link_record_buf=NULL;
	int end_of_archive = 0;
	TARU * taru;

#ifdef HPUX_CDF
	int cdf_flag;		/* True if file is a CDF.  */
	int cdf_char;		/* Index of `+' char indicating a CDF.  */
#endif

	if (fp_taru == NULL) {
		taru = taru_create();
	} else {
		taru = fp_taru;
	}

	/* Initialize the copy in.  */

	ahsStaticSetTarFilename(file_hdr, NULL);
	ahsStaticSetTarFilename(file_hdr_links, NULL);
	ahsStaticSetTarFilename(file_hdr_links_last, NULL);
	ahsStaticSetPaxLinkname(file_hdr, NULL);
	ahsStaticSetPaxLinkname(file_hdr_links, NULL);
	ahsStaticSetPaxLinkname(file_hdr_links_last, NULL);


	E_DEBUG2("taru->taru_tarheaderflagsM=%d", taru->taru_tarheaderflagsM);


	convert_flags |= UINFILE_DETECT_FORCEUNIXFD;
	convert_flags |= UINFILE_DETECT_NATIVE;

	/* Initialize this in case it has members we don't know to set.  */

	/* bzero (&times, sizeof (struct utimbuf)); */
	memset(zeros_512, 0x00, 512);
	memset(&times, 0x00, sizeof(struct utimbuf));

	/* While there is more input in the collection, process the input.  */

	if ((input_fd = uinfile_opendup(i_fd, (mode_t)(0), &uinfile, convert_flags)) < 0) {
		delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
		return -1;
	}
	uxfio_fcntl(input_fd, UXFIO_F_SET_BUFACTIVE, 0);
	/* translate type to the archive_format enumeration */
	switch (uinfile->typeM) {
	case CPIO_CRC_FILEFORMAT:
		archive_format_in = arf_crcascii;
		break;
	case CPIO_NEWC_FILEFORMAT:
		archive_format_in = arf_newascii;
		break;
	case CPIO_POSIX_FILEFORMAT:
		archive_format_in = arf_oldascii;
		break;
	case USTAR_FILEFORMAT:
		archive_format_in = arf_ustar;
		break;
	default:
		ahsStaticDeleteFilehdr(file_hdr);
		ahsStaticDeleteFilehdr(file_hdr_links);
		ahsStaticDeleteFilehdr(file_hdr_links_last);
		return -1;
	}

	if (archive_format_in != arf_ustar) {
		link_record = hllist_open();
	}
	while (!done) {
		ahsStaticSetTarFilename(file_hdr, NULL);
		link_name = NULL;

		/* Start processing the next file by reading the header.  */
		if (taru_read_in_header(taru, file_hdr, input_fd, archive_format_in, &end_of_archive, 0) < 0){
			fprintf (stderr, "error returned by taru_read_in_header\n");
			return -1;
		}

#ifdef DEBUG_CPIO
		if (debug_flag) {
			struct new_cpio_header *h;
			h = file_hdr;
			fprintf(stderr,
				"magic = 0%o, ino = %d, mode = 0%o, uid = %d, gid = %d\n",
				h->c_magic, h->c_ino, h->c_mode, h->c_uid, h->c_gid);
			fprintf(stderr,
				"nlink = %d, mtime = %d, filesize = %d, dev_maj = 0x%x\n",
				h->c_nlink, h->c_mtime, h->c_filesize, h->c_dev_maj);
			fprintf(stderr,
				"dev_min = 0x%x, rdev_maj = 0x%x, rdev_min = 0x%x, namesize = %d\n",
				h->c_dev_min, h->c_rdev_maj, h->c_rdev_min, h->c_namesize);
			fprintf(stderr,
				"chksum = %d, name = \"%s\", tar_linkname = \"%s\"\n",
				h->c_chksum, ahsStaticGetTarFilename(h),
						ahsStaticGetTarLinkname(h));

		}
#endif
	
		E_DEBUG2("processing PATH [%s]", ahsStaticGetTarFilename(file_hdr));
		E_DEBUG2("processing with LINKPATH [%s]", ahsStaticGetTarLinkname(file_hdr));

		/* Is this the header for the TRAILER file?  */
		if (strcmp("TRAILER!!!", ahsStaticGetTarFilename(file_hdr)) == 0) {
			done = TRUE;
			break;
		}
		skip_file = FALSE;
		if (skip_file) {
			exit(4);
		} else {
			enum archive_format archive_format_tar = arf_ustar;
			/* Copy the input file into the directory structure.  */

			/* See if the file already exists.  */
			existing_dir = FALSE;

			if (archive_format_in != arf_ustar && archive_format_in != arf_tar) {

				if (file_hdr->c_nlink > 1 && (((file_hdr->c_mode & CP_IFMT) == CP_IFREG) ||
							     ((file_hdr->c_mode & CP_IFMT) == CP_IFBLK) ||
							     ((file_hdr->c_mode & CP_IFMT) == CP_IFCHR) ||
							     ((file_hdr->c_mode & CP_IFMT) == CP_IFSOCK))
				    ) {

					/* in cpio format, the last entry holds the data, so read ahead
					   and get it */
					hllist_add_record(link_record, ahsStaticGetTarFilename(file_hdr), 
								devno = (dev_t) makedev(file_hdr->c_dev_maj, file_hdr->c_dev_min),
					    				file_hdr->c_ino);

					if (input_fd < UXFIO_FD_MIN) {
						if ((input_fd = uxfio_opendup(input_fd, UXFIO_BUFTYPE_DYNAMIC_MEM)) < 0) {
							delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
							return -1;
						}
					}
					uxfio_fcntl(input_fd, UXFIO_F_ARM_AUTO_DISABLE, 0);
					uxfio_fcntl(input_fd, UXFIO_F_SET_BUFACTIVE, 1);
					uxfio_fcntl(input_fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
					orig_offset = uxfio_lseek(input_fd, 0, SEEK_CUR);
					/* determine number of entries encountered so far */
					nfound = 0;
					link_record_buf=hllist_find_file_entry(link_record, devno, file_hdr->c_ino, -1, &nfound);

					if (nfound == 1 && (((file_hdr->c_mode & CP_IFMT) == CP_IFREG) ||
							    ((file_hdr->c_mode & CP_IFMT) == CP_IFBLK) ||
							    ((file_hdr->c_mode & CP_IFMT) == CP_IFCHR) ||
							    ((file_hdr->c_mode & CP_IFMT) == CP_IFSOCK))) {

						/* first occurance, therefore must read ahead in the cpio archive
						   to get the data, which cpio stores in the last entry and tar stores
						   in the first entry. */

						nmore = file_hdr->c_nlink - 1;	/* must now go and find nmore occurances of the file
										   in the cpio archive because thats where the data is. */
						data_offset = orig_offset;
						data_size = (int)(file_hdr->c_filesize);
						count = 0;
						found_data = 0;
						done_links = FALSE;
						/* must check for data here */
						if (data_size) {
							done_links = TRUE;
							found_data = 1;
						}
						while (!done_links) {
							taru_read_header(taru, file_hdr_links, input_fd, archive_format_in, &end_of_archive, 0);

							if (strcmp("TRAILER!!!", ahsStaticGetTarFilename(file_hdr_links)) == 0) {
								done_links = TRUE;
								break;
							}
							if (file_hdr_links->c_ino == file_hdr->c_ino &&
							    devno == (dev_t) makedev(file_hdr_links->c_dev_maj, file_hdr_links->c_dev_min)) {
								/* LINK FOUND */
								count++;
								file_hdr_copy(file_hdr_links_last, file_hdr_links);
								data_offset = uxfio_lseek(input_fd, 0, SEEK_CUR);
								data_size = file_hdr_links->c_filesize;
								if ((file_hdr_links->c_filesize > 0) || count == nmore) {
									found_data = 1;
									done_links = TRUE;
									break;
								}
							}
							
							if (
								   (
									   (file_hdr_links->c_mode & CP_IFMT) == CP_IFREG
									   /* (file_hdr_links.c_mode & CP_IFMT) == CP_IFLNK */
								   )
								   && file_hdr_links->c_filesize
							    ) {
								if (uxfio_lseek(input_fd, file_hdr_links->c_filesize, SEEK_CUR) < 0) {
									fprintf(stderr, "error from uxfio_lseek 100a.1.\n");
									delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
									return -1;
								}
								taru_tape_skip_padding(input_fd, file_hdr_links->c_filesize, archive_format_in);
							}
						}	/* while */


						if (!found_data && count) {
							/* could be a archive that didn't store all the links to a 
							   zero length file, this a legitimate case */
							/* the header is in file_hdr_links_last and the data is at data_offset */

							file_hdr_copy(file_hdr_links, file_hdr_links_last);
							if (uxfio_lseek(input_fd, data_offset, SEEK_SET) < 0) {
								fprintf(stderr, "taru_process_copy_in, read ahead failed, lseek\n");
								delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
								return -1;
							}
						} else if (!found_data && !count) {
							/* first and only occurance of link, the data must be there */

							file_hdr_copy(file_hdr_links, file_hdr_links_last);
							if (uxfio_lseek(input_fd, orig_offset, SEEK_SET) < 0) {
								fprintf(stderr, "taru_process_copy_in, read ahead failed, lseek\n");
								delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
								return -1;
							}
						} else if (found_data) {
							/* file pointer at the data already */
							file_hdr_copy(file_hdr_links, file_hdr);
						}
						file_hdr->c_filesize = data_size;
						/* taru_write_out_tar_header2(taru, file_hdr, output_fd, (char *)(NULL), (char *)(NULL), (char *)(NULL), 0); */
						taru_write_archive_member_header(taru, NULL, file_hdr, NULL/*link_record*/ , NULL, 0, output_fd, arf_ustar, NULL, taru->taru_tarheaderflagsM);
						taru_pump_amount2(output_fd, input_fd, data_size, -1);
						taru_tape_skip_padding(input_fd, data_size, archive_format_in);
						taru_tape_pad_output(output_fd, data_size, archive_format_tar);
								/* just store a link in the tar file, already stored the file */	
					} else {	/* just store a link in the tar file, already stored the file */
						nfound = 0;
						link_record_buf=hllist_find_file_entry(link_record, devno, file_hdr->c_ino, 1, &nfound);
						ahsStaticSetPaxLinkname(file_hdr, link_record_buf->path_);
						/* taru_write_out_tar_header2(taru, file_hdr, output_fd, (char *)(NULL), (char *)(NULL), (char *)(NULL), 0); */
						taru_write_archive_member_header(taru, NULL, file_hdr, NULL /*link_record*/, NULL, 0, output_fd, arf_ustar, NULL, taru->taru_tarheaderflagsM);
					}

					if (uxfio_lseek(input_fd, orig_offset, SEEK_SET) < 0) {
						fprintf(stderr, "taru_process_copy_in, read ahead failed, lseek\n");
						delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
						return -1;
					}
					/* skip over the data if any, there shouldn't be any except for the linked last entry */
					/* but some versions of cpio left data here, so we really don't know yet if there is 
					   data to skip over */

					if (taru_tape_buffered_read(input_fd, magbuf, 6) != 6) {
						fprintf(stderr, "taru_tape_buffered_read error in taru_process copy out\n");
						delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
						return -1;
					}
					uxfio_lseek(input_fd, orig_offset, SEEK_SET);

					if ((!strncmp(magbuf, "070701", 6)) ||
					(!strncmp(magbuf, "070702", 6)) ||
					  (!strncmp(magbuf, "070707", 6))
					    ) {		/* probably the next header */

						if (taru_read_in_header(taru, file_hdr_links, input_fd, archive_format_in, &end_of_archive, 0) < 0) {	/* error, probably file data */
							fprintf(stderr, "cpio header magic probably match accidentally, attempting to recover.\n");
							if (uxfio_lseek(input_fd, orig_offset, SEEK_SET)) {
								fprintf(stderr, "lseek failed\n");
								delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
								return -1;
							}
							taru_read_amount(input_fd, file_hdr->c_filesize);
							/* tape_toss_input (input_fd, file_hdr->c_filesize); */
							taru_tape_skip_padding(input_fd, file_hdr->c_filesize, archive_format_in);
							taru_tape_buffered_read(input_fd, magbuf, 6);
							if ((!strncmp(magbuf, "070701", 6)) || (!strncmp(magbuf, "070702", 6))) {
								/* Ok */
								uxfio_lseek(input_fd, -6, SEEK_CUR);
							}
						} else {
							if (uxfio_lseek(input_fd, orig_offset, SEEK_SET) < 0) {
								fprintf(stderr, "taru_process_copy_in, read ahead failed, lseek\n");
								delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
								return -1;
							}
						}
					} else {	/* skip over the data */
						taru_read_amount /*tape_toss_input */ (input_fd, file_hdr->c_filesize);
						taru_tape_skip_padding(input_fd, file_hdr->c_filesize, archive_format_in);
					}
					uxfio_fcntl(input_fd, UXFIO_F_ARM_AUTO_DISABLE, 1);
					continue;
				} else if (file_hdr->c_nlink > 1 && (file_hdr->c_mode & CP_IFMT) == CP_IFLNK) {
					fprintf(stderr, "linked soft link, HEEEE exiting..\n"); exit(1);
				} else if (file_hdr->c_nlink > 1 && (file_hdr->c_mode & CP_IFMT) != CP_IFDIR) {
					fprintf(stderr, "something else linked. EEEEE exiting..\n"); exit(1);
				}
			}	/* end of handling links */
			/* Do the real copy or link.  */
			switch (file_hdr->c_mode & CP_IFMT) {
			case CP_IFREG:
				/* taru_write_out_tar_header2(taru, file_hdr, output_fd, (char *)(NULL), (char *)(NULL), (char *)(NULL), 0); */
				taru_write_archive_member_header(taru, NULL, file_hdr, NULL /*link_record*/, NULL, 0, output_fd, arf_ustar, NULL, taru->taru_tarheaderflagsM);
				taru_pump_amount2(output_fd, input_fd, (intmax_t)(file_hdr->c_filesize), -1);
				taru_tape_skip_padding(input_fd, file_hdr->c_filesize, archive_format_in);
				taru_tape_pad_output(output_fd, file_hdr->c_filesize, archive_format_tar);
				break;

			case CP_IFLNK:
				link_name = NULL;
				if (archive_format_in != arf_tar && archive_format_in != arf_ustar) {
					link_name = (char *) malloc((unsigned int) file_hdr->c_filesize + 1);
					link_name[file_hdr->c_filesize] = '\0';
					taru_tape_buffered_read(input_fd, link_name, file_hdr->c_filesize);
					/* tape_buffered_read (link_name, input_fd, file_hdr->c_filesize); */
					taru_tape_skip_padding(input_fd, file_hdr->c_filesize, archive_format_in);
					ahsStaticSetPaxLinkname(file_hdr, link_name);
				}
				/* taru_write_out_tar_header2(taru, file_hdr, output_fd, (char *)(NULL), (char *)(NULL), (char *)(NULL), 0); */
				taru_write_archive_member_header(taru, NULL, file_hdr, NULL /*link_record*/, NULL, 0, output_fd, arf_ustar, NULL, taru->taru_tarheaderflagsM);
				if (link_name) {
					swbis_free(link_name);
					link_name = NULL;
				}
				break;

			case CP_IFDIR:
			case CP_IFCHR:
			case CP_IFBLK:
			case CP_IFSOCK:
			case CP_IFIFO:
				/* taru_write_out_tar_header2(taru, file_hdr, output_fd, (char *)(NULL), (char *)(NULL), (char *)(NULL), 0); */
				taru_write_archive_member_header(taru, NULL, file_hdr, NULL /*link_record*/, NULL, 0, output_fd, arf_ustar, NULL, taru->taru_tarheaderflagsM);
				break;

			default:
				fprintf(stderr, "%s: unknown file type", ahsStaticGetTarFilename(file_hdr));
				taru_read_amount /*tape_toss_input */ (input_fd, file_hdr->c_filesize);
				taru_tape_skip_padding(input_fd, file_hdr->c_filesize, archive_format_in);
			}

		}
	}
	uxfio_unix_safe_write(output_fd, zeros_512, 512);
	uxfio_unix_safe_write(output_fd, zeros_512, 512);


	if (archive_format_in != arf_ustar) {
		hllist_close(link_record);
	}
	uxfio_close(input_fd);
	delete_trio(file_hdr, file_hdr_links, file_hdr_links_last);
	if (fp_taru == NULL) taru_delete(taru);
	return 0;
}


static struct new_cpio_header *
file_hdr_copy(struct new_cpio_header *dst, struct new_cpio_header *src)
{

	dst->c_magic = src->c_magic;
	dst->c_ino = src->c_ino;
	dst->c_mode = src->c_mode;
	dst->c_uid = src->c_uid;
	dst->c_gid = src->c_gid;
	dst->c_nlink = src->c_nlink;
	dst->c_mtime = src->c_mtime;
	dst->c_filesize = src->c_filesize;
	dst->c_dev_maj = src->c_dev_maj;
	dst->c_dev_min = src->c_dev_min;
	dst->c_rdev_maj = src->c_rdev_maj;
	dst->c_rdev_min = src->c_rdev_min;
	dst->c_namesize = src->c_namesize;
	dst->c_chksum = src->c_chksum;

	ahsStaticSetTarFilename(dst, ahsStaticGetTarFilename(src));
	ahsStaticSetPaxLinkname(dst, ahsStaticGetTarLinkname(src));
	return dst;
}
