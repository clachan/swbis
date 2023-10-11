/* copyout.c:

   Copyright (C) 1998, 1999, 2014 Jim Lowe

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "filetypes.h"
#include "tarhdr.h"
#include "system.h"
#include "cpiohdr.h"
#include "uxfio.h"
#include "ahs.h"
#include "taru.h"
#include "strob.h"
#include "hllist.h"
#include "defer.h"
#include "swlib.h"
#include "inttostr.h"
#include "strtoint.h"
#include "swutilname.h"
#include "etar.h"
#include "atomicio.h"

static
intmax_t
taru_write_archive_member_data_(TARU * taru,
				struct new_cpio_header *file_hdr0,
			      	int out_file_des,
			      	int pad_out_file_des,
			      	int input_fd,
				int (*fout)(int),
			  	enum archive_format archive_format_out,
				int adjuct_ofd, FILE_DIGS * digs)
{
	int fd = -1;
	intmax_t ret = 0;
	char * tmpp = ahsStaticGetTarLinkname(file_hdr0);

	/* E_DEBUG(""); */

	if ( (((file_hdr0->c_mode & CP_IFMT) == CP_IFREG) &&
				(tmpp == (char*)NULL || !strlen(tmpp)))
	) {
		/* E_DEBUG(""); */
		if (input_fd < 0) {
			E_DEBUG("");
			fd = open(ahsStaticGetTarFilename(file_hdr0),
					O_RDONLY, 0);
			if (fd < 0) {
				fprintf(stderr,
					"open %s: %s\n",
					ahsStaticGetTarFilename(file_hdr0),
					strerror(errno));
				return -1;
			}
			input_fd = fd;
		}

		if (fout == NULL) {
			/* E_DEBUG(""); */
			ret = swlib_pump_amount8(out_file_des, input_fd, (intmax_t)(file_hdr0->c_filesize), adjuct_ofd, digs);
			/* E_DEBUG2("ret=%s", swlib_imaxtostr(ret, NULL)); */

			if (ret < 0) {
				E_DEBUG("");
				if (fd >= 0) close(fd);
				fprintf(stderr, "%s: error in taru_write_archive_member_data: ret=%s\n",
					swlib_utilname_get(), swlib_imaxtostr(ret, NULL));
				return -1;
			} else {
				/* E_DEBUG(""); */
				if (ret != (intmax_t)(file_hdr0->c_filesize)) {
					E_DEBUG2("Error: ret=%s", swlib_imaxtostr(ret, NULL));
					E_DEBUG2("Error: filesize=%s", swlib_umaxtostr(file_hdr0->c_filesize, NULL));
					fprintf(stderr,
					"%s: taru_write_archive_member_data_ internal error. ret=%s\n",
						swlib_utilname_get(), swlib_imaxtostr(ret, NULL));
					return -1;
				} else {
					/* E_DEBUG(""); */
					/*
					fprintf(stderr, "%s loaded OK\n", ahsStaticGetTarFilename(file_hdr0));
					*/
				}
				/* E_DEBUG(""); */
				if ((ret += taru_tape_pad_output(
						pad_out_file_des,
						(intmax_t)(file_hdr0->c_filesize),
						archive_format_out)) < 0) {
					E_DEBUG("");
					if (fd >= 0) close(fd);
					return -1;
				}
				/* E_DEBUG(""); */
				if (adjuct_ofd >= 0) {
					if (taru_tape_pad_output(
							adjuct_ofd,
							(intmax_t)(file_hdr0->c_filesize),
							archive_format_out) < 0
					) {
						if (fd >= 0) close(fd);
						return -1;
					}
				}
			}
			/* E_DEBUG(""); */
		} else {
			/* E_DEBUG(""); */
			ret = (*fout)(out_file_des);
			/* E_DEBUG2("ret=%d", (int)ret); */
		}
		if (fd >= 0) close(fd);
	} 
	/* E_DEBUG2("ret=%s", swlib_imaxtostr(ret, NULL)); */
	return ret;
}

static int
tarui_tape_buffered_write(char *in_buf, int out_des, long num_bytes,
						char *header_buffer)
{

	if (header_buffer) {
		memcpy(header_buffer, in_buf, num_bytes);
		return 0;
	} else {
		return taru_safewrite(out_des, in_buf, num_bytes);
	}

}

static int
tape_clear_rest_of_block(int out_file_des, char * zeros_512,
			int io_block_size, uintmax_t output_size)
{
	intmax_t rem = output_size % (intmax_t)io_block_size; 
	intmax_t n = rem;
  	int ret = 0;
	int retval = 0;
	while (n < io_block_size && ret >= 0) {
		if ((io_block_size - n) > 512) {
			ret = tarui_tape_buffered_write(zeros_512,
					out_file_des, 512, (char*)(NULL));
		} else {
			ret = tarui_tape_buffered_write(zeros_512,
				out_file_des,
				io_block_size - n,
				(char*)(NULL));
		}
		if (ret > 0) {
			n += ret;
			retval += ret;
		} else {
			fprintf(stderr,
	"taru: tape_clear_rest_of_block: error writing trailer block.\n");
		}
	}
	return retval;
}

unsigned long
taru_read_for_crc_checksum (int in_file_des, int file_size, char * file_name)
{
   unsigned long crc;
   char buf[TARU_BUFSIZ];
   int bytes_left;
   int bytes_read;
   int i;

   crc = 0;
   for (bytes_left = file_size; bytes_left > 0; bytes_left -= bytes_read)
   {
	   bytes_read = uxfio_sfread(in_file_des, buf, TARU_BUFSIZ);
	   if (bytes_read < 0)
		   fprintf (stderr, "cannot read checksum for %s", file_name);
	   if (bytes_read == 0)
		   break;
	   for (i = 0; i < bytes_read; ++i)
	   crc += buf[i] & 0xff;
   }
   return crc;
}

static intmax_t
tarui_write_archive_member_(TARU * taru, 
				struct stat *file_stat,
				struct new_cpio_header *file_hdr0,
				HLLIST * linkrecord, 
				DEFER * defer, 
				PORINODE * porinode,
				int out_file_des, 
				int input_fd,
				enum archive_format archive_format_out,
				int tarheaderflags)
{
	intmax_t ret = 0;
	intmax_t ret1 = 0;
	size_t sz;
	off_t old_pos;
	struct new_cpio_header * file_hdr = ahsStaticCreateFilehdr();
	unsigned long sum;
	
	file_hdr0->c_chksum=0;	
	if (archive_format_out == arf_crcascii && input_fd >= 0) {
		if (file_stat) {
			sz=file_stat->st_size;
		} else {
			sz=file_hdr0->c_filesize;
		}
		old_pos=uxfio_lseek(input_fd, (off_t)0, SEEK_CUR);
		sum=taru_read_for_crc_checksum (input_fd, sz, "");
		uxfio_lseek(input_fd, old_pos, SEEK_SET);
		file_hdr0->c_chksum=sum;	
	}
	ret=taru_write_archive_member_header(taru, file_stat,
					 file_hdr0,
					 linkrecord, defer, porinode,
					 out_file_des,
					 archive_format_out, file_hdr,
					tarheaderflags);

	if (ret < 0) {
		fprintf(stderr, 
		"error in taru_write_archive_member_header() called from"
		" tarui_write_archive_member_().\n");
		ahsStaticDeleteFilehdr(file_hdr);
		return ret;
	}
	if (((file_hdr->c_mode & CP_IFMT) == CP_IFREG) && 
		(ahsStaticGetTarLinkname(file_hdr) == (char*)NULL || 
			strlen(ahsStaticGetTarLinkname(file_hdr)) == 0) && 
					ret > 0) {
		ret1 = taru_write_archive_member_data(taru, file_hdr,
				out_file_des, input_fd,
				(int(*)(int))(NULL),
				archive_format_out, -1, file_hdr0->digsM);
		if (ret1 < 0) {
			ahsStaticDeleteFilehdr(file_hdr);
			return ret1;
		}
	} else {
		if (file_hdr0->digsM)
			taru_digs_init(file_hdr0->digsM, DIGS_ENABLE_CLEAR, 0);
	}
	ahsStaticDeleteFilehdr(file_hdr);
	return ret + ret1;
}

static
int
get_ilen(int rlen)
{
	int i = 0;
	int j = 0;
	int r = rlen;
	
	while (r/10 >= 1) {
		i++;
		r=r/10;
	}
	i++;

	/* i is now the approximate guessed length of the record
 	 * Now it must be determinied iteratively if this it is correct
 	 * when the length of the leading decimal length string is included.
 	 */  

	r = rlen + i;
	
	/* r is the first guess at the total length */

	while (i < 99 /*arbitrarily large number to stop infinite loop*/) {
		j = 0;
		while (r/10 >= 1) {
			j++;
			r=r/10;
		}
		j++;
		if (i == j)
			return i+rlen;
		i++;
		r = rlen + i;
	}
	/* should never get here */
	return -1;
}


static
int
construct_exthdr_record(STROB * buf, int initial_offset, char * keyword, char * value, int len)
{
	int rlen;
	int ret;

	/*
	"%d %s=%s\n", length, keyword, value
	*/

	rlen = 1/*space*/ + strlen(keyword) + len + 1/*equals_sign*/ + 1/*newline*/;

	/* rlen is the length not including the rendering of the decimal length */

	ret = get_ilen(rlen);
	if (ret < 0) {
		/* This should really never happen */
		fprintf(stderr, "internal error in construct_exthdr_record loc=1\n");
		exit(120);
	}
	rlen = ret;

	/* rlen is now the record length */
	strob_sprintf_at(buf, initial_offset, "%d %s=", rlen, keyword);

	ret = strlen(strob_str(buf) + initial_offset);
	strob_memcpy_at(buf, initial_offset + ret /*offset*/, (void *)value, (size_t)len);

	/* sanity check */
	if ((ret + len + 1/*newline*/ ) != rlen) {
		/* This should really never happen */
		fprintf(stderr, "internal error in construct_exthdr_record loc=2\n");
		exit(121);
	}

	/* Now append the '\n' newline */

	strob_memcpy_at(buf, initial_offset + rlen-1, (void *)"\n", 1);

	return rlen;
}

static int
write_exthdrs_xattr_records(TARU * taru, struct new_cpio_header * file_hdr, int data_len)
{
	int ret;
	STROB * buf;
	CPLOB * extlist;
	EXATT  *p;
	char * attr;
	char * value;
	int valuelen;
	int index; 

	buf = taru->exthdr_dataM;
	/* strob_strcpy(buf, ""); */
	extlist = (CPLOB*)(file_hdr->extattlistM);

	index = 0;
	while(extlist && (p=(EXATT*) cplob_val(extlist, index)) && p->is_in_useM) {
		
		/*
 		 * construct the dot delimted attribute name e.g. "SCHILLY.selinux.access"
 		 */
		strob_sprintf(taru->localbufM, 0, "%s%s%s%s%s",
				p->vendorM,
				strlen(p->vendorM) ? "." : "",
				p->namespaceM,
				strlen(p->namespaceM) ? "." : "",
				strob_str((STROB *)p->attrnameM));
		value = strob_str(p->attrvalueM);
		valuelen = p->lenM;
		ret = construct_exthdr_record(buf, data_len, strob_str(taru->localbufM), value, valuelen);
		if (ret < 0) return -1;
		data_len += ret;
		/* p->is_in_useM == 0; */
		index++;
	}
	return data_len;
}


static int
write_exthdrs_ustar_records(TARU * taru, struct new_cpio_header * file_hdr)
{
	int data_len;
	int ret;
	STROB * buf;

	buf = taru->exthdr_dataM;
	/* strob_strcpy(buf, ""); */

	data_len = 0;
	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_LINKPATH) {
		ret = construct_exthdr_record(buf, data_len, PAX_KW_linkpath,
						ahsStaticGetTarLinkname(file_hdr),
						strlen(ahsStaticGetTarLinkname(file_hdr)));
		if (ret < 0)
			return -1;

		E_DEBUG2("LINKPATH: [%s]", ahsStaticGetTarLinkname(file_hdr));
		/*
 		 * Now truncate the linkname to TARNAMESIZE,  this seems to be what GNU tar does
 		 */
		strob_chr_index((STROB*)(file_hdr->c_tar_linkname),  TARNAMESIZE, '\0');
		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_PATH) {
		ret = construct_exthdr_record(buf, data_len, PAX_KW_path,
						ahsStaticGetTarFilename(file_hdr),
						strlen(ahsStaticGetTarFilename(file_hdr)));
		if (ret < 0)
			return -2;

		E_DEBUG2("PATH: [%s]", ahsStaticGetTarFilename(file_hdr));
		/*
 		 * Now truncate the name to TARNAMESIZE,  this seems to be what GNU tar does
 		 */
		strob_chr_index((STROB*)(file_hdr->c_name),  TARNAMESIZE, '\0');
		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_SIZE) {
		strob_sprintf(taru->localbufM, 0, "%lu", (unsigned long)(file_hdr->c_filesize));
		ret = construct_exthdr_record(buf, data_len, PAX_KW_size,
				strob_str(taru->localbufM), strob_strlen(taru->localbufM));
		if (ret < 0) return -3;

		/*
 		 * Now set the filesize to a arbitrary number that fits
 		 */

		file_hdr->c_filesize = TAR_MAX_SIZE;

		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_UID) {
		strob_sprintf(taru->localbufM, 0, "%lu", (unsigned long)(file_hdr->c_uid));
		ret = construct_exthdr_record(buf, data_len, PAX_KW_uid,
				strob_str(taru->localbufM), strob_strlen(taru->localbufM));
		if (ret < 0) return -4;
		/*
 		 * Now set the value to a number that will fit
 		 */
		file_hdr->c_uid = TAR_MAX_UID;
		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_GID) {
		strob_sprintf(taru->localbufM, 0, "%lu", (unsigned long)(file_hdr->c_gid));
		ret = construct_exthdr_record(buf, data_len, PAX_KW_gid,
				strob_str(taru->localbufM), strob_strlen(taru->localbufM));
		if (ret < 0) return -5;
		/*
 		 * Now set the value to a number that will fit
 		 */
		file_hdr->c_uid = TAR_MAX_GID;
		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_UNAME) {
		ret = construct_exthdr_record(buf, data_len, PAX_KW_uname,
			ahsStaticGetTarUsername(file_hdr), strlen(ahsStaticGetTarUsername(file_hdr)));
		if (ret < 0) return -6;
		/*
 		 * Now set the string that will fit in a ustar header by truncation
 		 */
		strob_chr_index((STROB*)(file_hdr->c_username),  THB_FL_uname - 1, '\0');

		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_GNAME) {
		ret = construct_exthdr_record(buf, data_len, PAX_KW_gname,
			ahsStaticGetTarGroupname(file_hdr), strlen(ahsStaticGetTarGroupname(file_hdr)));
		if (ret < 0) return -7;
		/*
 		 * Now set the string that will fit in a ustar header by truncation
 		 */
		strob_chr_index((STROB*)(file_hdr->c_groupname),  THB_FL_gname - 1, '\0');
		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_DEV_MAJ) {
		strob_sprintf(taru->localbufM, 0, "%lu", (unsigned long)(file_hdr->c_dev_maj));
		ret = construct_exthdr_record(buf, data_len, SWBIS_VENDOR "." PAX_KW_devmajor,
				strob_str(taru->localbufM), strob_strlen(taru->localbufM));
		if (ret < 0) return -8;
		/*
 		 * Now set the value to a number that will fit
 		 */
		file_hdr->c_dev_maj = TAR_MAX_DEVICE;
		data_len += ret;
	}

	if (file_hdr->extHeader_needs_maskM & TARU_EHUM_DEV_MIN) {
		strob_sprintf(taru->localbufM, 0, "%lu", (unsigned long)(file_hdr->c_dev_min));
		ret = construct_exthdr_record(buf, data_len, SWBIS_VENDOR "." PAX_KW_devminor,
				strob_str(taru->localbufM), strob_strlen(taru->localbufM));
		if (ret < 0) return -9;
		/*
 		 * Now set the value to a number that will fit
 		 */
		file_hdr->c_dev_min = TAR_MAX_DEVICE;
		data_len += ret;
	}

	return data_len;
}

int 
taru_safewrite(int fd, void *vbuf, size_t amount)
{
	int rc = 0;
	char *buf = vbuf;

	while (amount && 
		((rc = uxfio_write(fd, buf, amount)) < (int)amount) &&
						 rc > 0) {
		buf += rc;
		amount -= rc;
	}

	if (rc < 0) return -((int)(amount));
	return (int)amount;
}

int
taru_write_archive_trailer(TARU * taru,
				enum archive_format archive_format_out,
				int fd, int io_block_size,
				uintmax_t bytes_written, int flags)
{
	int ret;
	int retval = 0;
	struct new_cpio_header * file_hdr = ahsStaticCreateFilehdr();
					/* Output header information.  */
	char zeros_512[512];
	int gnu_tar_block_size = 10240;
	
	memset(zeros_512, 0x00, 512);

	if (io_block_size)  gnu_tar_block_size = io_block_size;

	if (archive_format_out != arf_tar && archive_format_out != arf_ustar) {
		io_block_size = 512;
		file_hdr->c_ino = 0;
		file_hdr->c_mode = 0;
		file_hdr->c_uid = 0;
		file_hdr->c_gid = 0;
		file_hdr->c_nlink = 1;	/* Must be 1 for crc format.  */
		file_hdr->c_dev_maj = 0;
		file_hdr->c_dev_min = 0;
		file_hdr->c_rdev_maj = 0;
		file_hdr->c_rdev_min = 0;
		file_hdr->c_mtime = 0;
		file_hdr->c_chksum = 0;
		file_hdr->c_filesize = 0;
		file_hdr->c_namesize = 11;
		ahsStaticSetTarFilename(file_hdr, CPIO_INBAND_EOA_FILENAME);
		ret = taru_write_out_header(taru, file_hdr, fd,
				archive_format_out, (char *)(NULL), 0);
		if (bytes_written) {
			if (ret > 0) bytes_written += ret;
			ret += tape_clear_rest_of_block(fd, zeros_512,
						io_block_size, bytes_written);
		}
	} else {
		int nblock;
		if (bytes_written % 512) {
			char tmpbuf[UINTMAX_STRSIZE_BOUND];
			fprintf(stderr, 
			"internal error, bytes written not multiple of 512, bytes=%s\n",
				umaxtostr(bytes_written, tmpbuf));
			flags = 0;	
			retval = 1;
		}
		if (flags & TARU_TAR_GNU_BLOCKSIZE_B1) {
			nblock = 2;
			if (flags & TARU_TAR_BE_LIKE_STAR) {
				nblock = 3;
			}
		} else if (flags & TARU_TAR_GNU_OLDGNUTAR) {
			int n_tar_blocks = gnu_tar_block_size / 512;
			nblock = ( bytes_written % gnu_tar_block_size ) / 512;
			nblock = n_tar_blocks - nblock;
			if (nblock < 2) nblock += n_tar_blocks;
		} else {
			nblock = 2;
		}
		ret = 0;
		while (nblock > 0) {
			ret += tarui_tape_buffered_write(zeros_512, fd,
						512, (char *) (NULL));
			nblock--;
		}
	}
	ahsStaticDeleteFilehdr(file_hdr);
	if (retval) return -1;
	return ret;
}

intmax_t
taru_write_archive_member(TARU * taru, char *filename, 
				struct stat *statbuf, 
				struct new_cpio_header * file_hdr0, 
				HLLIST * linkrec,
				DEFER * defer,
				PORINODE * porinode,
				int ofd, int in_fd, 
				enum archive_format format, 
				int tarheaderflags)
{
	char linkname[101];
	struct stat st;
	struct new_cpio_header * file_hdr = ahsStaticCreateFilehdr();
	int fd = -1;
	intmax_t ret = 0;
	int nullfd = -1;
	char * p;
	int vlen = 1024;

	E_DEBUG("ENTERING");
	E_DEBUG2("filename=[%s]", filename);
	*linkname='\0';
	if (statbuf) {
		E_DEBUG("statbuf");
		taru_statbuf2filehdr(file_hdr, statbuf, NULL,
					filename, linkname);
	} else if (file_hdr0) {
		E_DEBUG("file_hdr0");
		taru_filehdr2filehdr(file_hdr, file_hdr0);
	} else {
		E_DEBUG("else");
		if(lstat (filename, &st) == 0) {
			statbuf=&st;
			E_DEBUG("lstat");
			taru_statbuf2filehdr(file_hdr, statbuf,
						NULL, filename, linkname);
		} else {
			fprintf(stderr, "swbis: %s %s\n",
						filename, strerror(errno));
			ahsStaticDeleteFilehdr(file_hdr);
			return -1;
		}
	}
	fd=in_fd;
	if (fd < 0) {	
		switch (file_hdr->c_mode & CP_IFMT) {
		case CP_IFDIR:
			if (access(filename, R_OK|X_OK) != 0) {
				fprintf(stderr, "swbis: %s: %s\n",
						strerror(errno), filename);
				ahsStaticDeleteFilehdr(file_hdr);
				return 0;
			}
			break;
		case CP_IFREG:
			if (file_hdr->c_filesize > 0) {
				fd = open(filename, O_RDONLY, 0);
			} else {
				fd = swbis_devnull_open("/dev/null", O_RDWR, 0);
				nullfd = fd;
			}
			if (fd < 0) {
				fprintf(stderr, "swbis: %s: %s\n",
						strerror(errno), filename);
				ahsStaticDeleteFilehdr(file_hdr);
				return 0;
			}
			break;
		case CP_IFLNK:
			/* FIXME arbitrary limit */
			ahsStaticSetTarLinknameLength(file_hdr, vlen);
			p = ahsStaticGetTarLinkname(file_hdr);
			*(p+vlen-1) = '\0';
			ret = swvarfs_u_readlink(NULL, filename, p, vlen-1);
			if (ret < 0) {
				fprintf(stderr, "swbis: %s: %s\n",
						strerror(errno), filename);
			}
			break;
		default:
			fd = -1;
			break;
	
		}
	}
	ret = tarui_write_archive_member_(taru, statbuf, file_hdr,
					linkrec, defer, porinode, ofd,
					fd, format, tarheaderflags);

	if (fd >= 0 && nullfd != fd) {
		close(fd);
	} else {
		swbis_devnull_close(fd);
	}

	ahsStaticDeleteFilehdr(file_hdr);
	return ret;
}

intmax_t
taru_write_archive_member_data(TARU * taru, 
				struct new_cpio_header *file_hdr0,
			      	int out_file_des, 
			      	int input_fd, int (*fout)(int),
			  	enum archive_format archive_format_out,
				int adjuct_ofd, FILE_DIGS * digs)
{
	intmax_t ret;
	ret = taru_write_archive_member_data_(taru, 
				file_hdr0,
			      	out_file_des, 
			      	out_file_des, 
			      	input_fd,
				fout,
			  	archive_format_out,
				adjuct_ofd, digs);
	return ret;
}

intmax_t
taru_write_archive_file_data(TARU * taru, 
				struct new_cpio_header *file_hdr0,
			      	int out_file_des, 
			      	int input_fd,
				int (*fout)(int),
			  	enum archive_format archive_format_out,
				int adjuct_ofd)
{
	intmax_t ret;
	int nullfd;

	if (!taru) 
		nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	else
		nullfd = taru->nullfdM;

	/* adjunct_ofd is not used and should be removed */
	if (adjuct_ofd >= 0) {
		SWLIB_FATAL("adjuct_ofd");
	}
	ret = taru_write_archive_member_data_(taru, 
				file_hdr0,
			      	out_file_des, 
			      	nullfd, 
			      	input_fd,
				fout,
			  	archive_format_out,
				-1/* adjuct_fd */,
				file_hdr0->digsM);
	if (!taru) swbis_devnull_close(nullfd);
	return ret;
}

int
taru_write_archive_member_header(TARU * taru,
				struct stat * file_stat,
				struct new_cpio_header * file_hdr0,
				HLLIST * linkrecord,
				DEFER * defer,
				PORINODE * porinode,
				int out_file_des,
				enum archive_format archive_format_out,
				struct new_cpio_header * file_hdr_return, 
				int tarheaderflags)
{
	int do_gnu_long_link = 0;
	int do_gnu_long_name = 0;
	int do_gnu_long_link_name_bytes = 0;
	int do_gnu_long_link_link_bytes = 0;
	int do_exthdr_bytes = 0;
	int nfound = 0, retval = 0;
	int ret;
	hllist_entry * link_record_buf = NULL;
	char * tmpp;
	struct new_cpio_header * file_hdr;
	dev_t v_dev;
	ino_t v_ino;
	int is_dir;
	unsigned long c_type;
	int do_strip_leading_slash = 0;
	int linkrecord_active;
	int is_too_long;
	char link_name[128];	
	int link_size;
	char * sbase;
	ETAR * etar;
	int tar_iflags_numeric_uids = (tarheaderflags & TARU_TAR_NUMERIC_UIDS); 
	int needy_tar_header = 0;
	int is_pax_header_allowed = 0;

	E_DEBUG("BEGIN");
	file_hdr = ahsStaticCreateFilehdr();
	linkrecord_active = (linkrecord != NULL && taru->linkrecord_disableM == 0);

	if (taru) {
		do_strip_leading_slash = 
			tarheaderflags & TARU_TAR_DO_STRIP_LEADING_SLASH;
	}

	E_DEBUG2("HERE tarheaderflags=%d", tarheaderflags);

	/* 
	 * Set Info from the file_hdr0
	 */
	taru_filehdr2filehdr(file_hdr, file_hdr0);
	SWLIB_ASSERT(file_stat == NULL);

	/*
	 * If file_stat is not given, we assume we are not archiving a
	 * real file system in which
	 * case we need psuedo inodes.
	 * Also, If the format is oldascii then use psuedo inodes.
	 */
	if (
		porinode && 
		!file_stat && 
		(archive_format_out == arf_oldascii ||
		archive_format_out == arf_newascii ||
		archive_format_out == arf_crcascii)
	) {
		int mylink = file_hdr->c_nlink;
		E_DEBUG("");
		switch (file_hdr->c_mode & CP_IFMT) {
			case CP_IFLNK:
			case CP_IFDIR:
				mylink = 1;
				break;
		}
		porinode_make_next_inode(porinode, mylink,
			(dev_t) makedev(file_hdr->c_dev_maj,\
				file_hdr->c_dev_min), 
					file_hdr->c_ino,  &v_dev, &v_ino);
		file_hdr->c_dev_maj = major(v_dev);
		file_hdr->c_dev_min = minor(v_dev);
		file_hdr->c_ino = v_ino;
	}	

	if (do_strip_leading_slash) {
		E_DEBUG("");
		ahsStatic_strip_name_leading_slash(file_hdr);
	}

	is_dir = (file_hdr->c_mode & CP_IFMT) == CP_IFDIR;
	c_type = (file_hdr->c_mode & CP_IFMT);

	/*
	* Check the name length.
	*/
	E_DEBUG2("name is [%s]", ahsStaticGetTarFilename(file_hdr));
	file_hdr->c_namesize = strlen(ahsStaticGetTarFilename(file_hdr)) + 1;
	

	if (archive_format_out == arf_tar || archive_format_out == arf_ustar)
	{
		is_too_long =  taru_is_tar_filename_too_long(ahsStaticGetTarFilename(file_hdr),
						tarheaderflags, &do_gnu_long_name, 0 /*is_dir*/);
		if (is_too_long) {
			E_DEBUG("");
			fprintf(stderr, "%s: file name too long, not dumped: %s\n",
					swlib_utilname_get(),
					ahsStaticGetTarFilename(file_hdr));
			ahsStaticDeleteFilehdr(file_hdr);
			return -2;
		}

		/* determine if the linkname is too long */
		is_too_long =  taru_is_tar_linkname_too_long(ahsStaticGetTarLinkname(file_hdr),
								tarheaderflags, &do_gnu_long_link);
		if (is_too_long) {
			fprintf(stderr, "%s: link name too long, not dumped: %s\n",
					swlib_utilname_get(),
					ahsStaticGetTarFilename(file_hdr));
			return -3;
		}

		/*
		 * Determine if pax headers are allowed per the output format policy
		 */

		is_pax_header_allowed = ( tarheaderflags & TARU_TAR_PAXEXTHDR || 
					    tarheaderflags & TARU_TAR_PAXEXTHDR_MD
					);	

		/*
		 * Determine if tar header needs extended header to store any of the
		 * other attributes and write a message if verbose.
		 */

		needy_tar_header = taru_fill_exthdr_needs_mask(file_hdr,
					tarheaderflags,
					is_pax_header_allowed,
					/* verbose_messages */ 1);


		/*
		 * If needy, and overflow prohibited by the user
		 * then return with error
		 */

		if (
			needy_tar_header &&
			is_pax_header_allowed == 0 &&
			(tarheaderflags & TARU_TAR_NO_OVERFLOW)
		) {
			/*
			 * process as error
			 */
			return -14;
		}


		/* 
		 * initialize the file_hdr->extHeader_needs_maskM 
		 */

		file_hdr->extHeader_needs_maskM = needy_tar_header;


		E_DEBUG2("do_gnu_long_link=%d", do_gnu_long_link);
		E_DEBUG("");
		
		if (tarheaderflags & TARU_TAR_PAXEXTHDR || tarheaderflags & TARU_TAR_PAXEXTHDR_MD) {
			if (tar_iflags_numeric_uids == 0) {
				/*
 				 * Fill in the user and group
 				 */
				taru_set_filehdr_sysdb_nameid_by_policy("G"/* group */, file_hdr,
						'\0' /*termch*/, tar_iflags_numeric_uids);
				taru_set_filehdr_sysdb_nameid_by_policy("U"/* group */, file_hdr,
						'\0' /*termch*/, tar_iflags_numeric_uids);
			}
			/*
 			 * Set the flags indicating an extended header record 
 			 * is required due to the length limitation in the ustar
 			 * header field
 			 */
			/* file_hdr->extHeader_needs_maskM = taru_fill_exthdr_needs_mask(file_hdr, tarheaderflags); */

			/*
 			 * GNU LongLinks and PAX Ext headers are mutually exclusive.
 			 */
			do_gnu_long_name = 0;
			do_gnu_long_link = 0;
		}

		/*
 		 * GNU tar appears not to like a longLink link type 'K' without a type 'L'
 		 * i.e. don't create a 'K' without preceeding it with a 'L' type
 		 */
		if (do_gnu_long_name | do_gnu_long_link) {
			/*
 			 * GNU LongLink type
 			 */
			E_DEBUG2("in do_gnu_long_name: %s", ahsStaticGetTarFilename(file_hdr));
			if ((do_gnu_long_link_name_bytes=taru_write_long_link_member
					(taru, out_file_des,
						ahsStaticGetTarFilename(file_hdr), GNUTYPE_LONGNAME,
						c_type, tarheaderflags)) != 1024) {
				fprintf(stderr, 
				"internal error writing GNU extension LongLink: %s\n",
						ahsStaticGetTarFilename(file_hdr));
				return -3;
			}
			/*
			* truncate the name in ahsStaticGetTarFilename()
			*/
			if (
				(tarheaderflags & TARU_TAR_GNU_OLDGNUTAR) ||
				(tarheaderflags & TARU_TAR_NAMESIZE_99) ||
				0
			) {
				/*
				 * This is what GNU tar 1.13.25 does
				 * as well as swpackage formats:  oldgnu, gnutar
				 */
				ahsStaticSetTarFilenameLength(file_hdr, OLDGNU_TARNAMESIZE+1);
				*(ahsStaticGetTarFilename(file_hdr) + OLDGNU_TARNAMESIZE) = '\0';
			} else {
				/*
				 * This is what GNU tar 1.15.1 does
				 * as well as swpackage formats:  gnu
				 */
				ahsStaticSetTarFilenameLength(file_hdr, TARNAMESIZE+1);
				*(ahsStaticGetTarFilename(file_hdr) + TARNAMESIZE) = '\0';
			}
		} else if (
		((tarheaderflags & TARU_TAR_PAXEXTHDR ) || (tarheaderflags & TARU_TAR_PAXEXTHDR_MD)) &&
				(file_hdr->extHeader_needs_maskM || file_hdr->extattlistM)
				/* FIXME */
			) {

			/*
 			 * POSIX Pax Headers
 			 */
			E_DEBUG("in pax headers");

			/*
 			 * Set the User and group names per the policy
 			 */ 
			taru_set_filehdr_sysdb_nameid_by_policy("G"/* group */, file_hdr,
					'\0' /*termch*/, tar_iflags_numeric_uids);
			taru_set_filehdr_sysdb_nameid_by_policy("U"/* group */, file_hdr,
					'\0' /*termch*/, tar_iflags_numeric_uids);


			/*
 			 * initialize the buffer where the extended records will be written.
 			 */
			strob_strcpy(taru->exthdr_dataM, "");
			
			/*
 			 * Do Pax Exttended Header POSIX attributes for the fields that don't fit.
 			 */

			do_exthdr_bytes = write_exthdrs_ustar_records(taru, file_hdr); 
			if (do_exthdr_bytes < 0) {
				fprintf(stderr, "internal error writing pax extended header POSIX attributes: %s: retval=%d\n",
						ahsStaticGetTarFilename(file_hdr), do_exthdr_bytes);
				return -3;
			}

			/*
 			 * Write the extended attributes
 			 */

			do_exthdr_bytes = write_exthdrs_xattr_records(taru, file_hdr, do_exthdr_bytes);
			if (do_exthdr_bytes < 0) {
				fprintf(stderr, "internal error writing pax extended header xattr attributes: %s: retval=%d\n",
						ahsStaticGetTarFilename(file_hdr), do_exthdr_bytes);
				return -4;
			}

			/*
 			 * If there are no extended headers to written then skip the writes
 			 */

			if (do_exthdr_bytes == 0)  goto NO_EA; /* goto */

			/*
 			 * Now construct the header
 			 */

			etar = taru->etarM;
			etar->etar_tarheaderflagsM = taru->taru_tarheaderflagsM;	 
			etar_init_hdr(etar);
			strob_strcpy(taru->u_name_bufferM, "");
			strob_sprintf(taru->u_name_bufferM, STROB_DO_APPEND, "./PaxHeaders.%d/", (int)swlib_get_pax_header_pid());
			sbase = swlib_basename((STROB *)NULL, ahsStaticGetTarFilename(file_hdr));
			strob_sprintf(taru->u_name_bufferM, STROB_DO_APPEND, "%s", sbase);
			etar_set_uid(etar, 0);
			etar_set_gid(etar, 0);
			etar_set_uname(etar, "");
			etar_set_gname(etar, "");
			etar_set_mode_ul(etar, (unsigned int)(0644));
			if (etar_set_pathname(etar, strob_str(taru->u_name_bufferM))) SWLIB_FATAL("name too long");
			etar_set_typeflag(etar, XHDTYPE);
			etar_set_size(etar, do_exthdr_bytes);
			etar_set_chksum(etar);
			
			/*
 			 * Now write the header
 			 */

			if (out_file_des >= UXFIO_FD_MIN)
				ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write, out_file_des,
					(void*)(etar_get_hdr(etar)), (size_t)(TARRECORDSIZE));
			else
				ret = atomicio((ssize_t (*)(int, void *, size_t))write, out_file_des,
					(void*)(etar_get_hdr(etar)), (size_t)(TARRECORDSIZE));


			if (ret != TARRECORDSIZE) {
				/* Big error */
				fprintf(stderr, "%s: error writing extended header, out_file_des=%d\n", swlib_utilname_get(), out_file_des);
				fprintf(stderr, "%s: error writing extended header, ret=%d\n", swlib_utilname_get(), ret);
				return -11;
			}

			/*
 			 * Now write the data
 			 */

			if (out_file_des >= UXFIO_FD_MIN)
				ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write, out_file_des,
					(void*)strob_str(taru->exthdr_dataM), do_exthdr_bytes);
			else
				ret = atomicio((ssize_t (*)(int, void *, size_t))write, out_file_des,
					(void*)strob_str(taru->exthdr_dataM), do_exthdr_bytes);



			if (ret != do_exthdr_bytes) {
				/* Big error */
				fprintf(stderr, "%s: error writing extended data\n", swlib_utilname_get());
				return -12;
			}
	
			/*
 			 * Now pad the output to nearest block
 			 */

			ret = taru_tape_pad_output(out_file_des, (intmax_t)ret, arf_ustar);
			if (ret < 0) {
				/* Big error */
				fprintf(stderr, "%s: error writing extended data padding\n", swlib_utilname_get());
				return -13;
			}

			NO_EA:
			; /* required  to avert at compiler error of goto label at end of conditional block */
		} else {
			;
			/* Just continue per normal */
		}

	} /* Tar Format */

	E_DEBUG2("c_is_link = %d", file_hdr->c_is_tar_lnktype);
	if (file_hdr->c_is_tar_lnktype == 1) {
		;
		E_DEBUG2("linkname is [%s]", ahsStaticGetTarLinkname(file_hdr));
	}

	E_DEBUG("");
	switch (file_hdr->c_mode & CP_IFMT) {
	case CP_IFREG:
	{
	E_DEBUG("CP_IFREG");
		if ((archive_format_out == arf_oldascii ||
		archive_format_out == arf_newascii ||
		archive_format_out == arf_crcascii)
	    && (file_hdr->c_nlink > 1)) {
		E_DEBUG("");
			/* CPIO format */
			if (defer){
				defer_set_taru(defer, taru);
				if (defer_is_last_link(defer,
							file_hdr)) {
				defer_writeout_zero_length_defers(defer,
					file_hdr, out_file_des);
				} else {
				defer_add_link_defer(defer, file_hdr);
					break;
				}
			}
			else {
				fprintf(stderr,
					"taru internal error: cpio hard link handling is disabled (loc=2).\n");
			}
		} else if (archive_format_out == arf_ustar || archive_format_out == arf_tar) {
			/* Tar Format */

			if (linkrecord_active) {
			   E_DEBUG("tar format");
			   nfound=0;	
			   if (file_hdr->c_nlink > 1) {
				link_record_buf = hllist_find_file_entry(linkrecord, 
					(dev_t) makedev(file_hdr->c_dev_maj,\
				   	    file_hdr->c_dev_min),
	 				file_hdr->c_ino,
					1,
					&nfound);
				if (nfound || file_hdr->c_is_tar_lnktype == 1) {
					/* hllist_show_to_file(linkrecord, stderr); */
					E_DEBUG("");
					
					if (nfound) {
						ahsStaticSetPaxLinkname(file_hdr,
							link_record_buf->path_);

					} else {
						; /* already set */
					}	

					retval = taru_write_out_header
						(taru, file_hdr, 
						out_file_des, 
						archive_format_out, 
						(char *)(NULL),
						tarheaderflags);
					break;
				} else {
					E_DEBUG("not found");
				}
			    }
			} else {
				E_DEBUG("");
				/* dead code */
				if (!linkrecord && 0 /*disable this warning 6/14/2014*/ )
					fprintf(stderr, 
					"taru internal error: tar hard link handling is disabled (loc=3).\n");
			}
			tmpp = ahsStaticGetTarLinkname(file_hdr);
			if (tmpp && strlen(tmpp)) {
				/* Hard link */
				E_DEBUG2("in Hard link: %s", ahsStaticGetTarFilename(file_hdr));
				if (do_gnu_long_link) {
					E_DEBUG2("in do_gnu_long_link: %s", ahsStaticGetTarFilename(file_hdr));
					if ((do_gnu_long_link_link_bytes=taru_write_long_link_member(taru,
							out_file_des,
							ahsStaticGetTarLinkname(file_hdr), GNUTYPE_LONGLINK,
							c_type, tarheaderflags)) != 1024) {

						fprintf(stderr, 
							"internal error writing GNU extension LongLink for linkname: %s\n",
							ahsStaticGetTarLinkname(file_hdr));
						return -3;
					}

					/* truncate the linkname to 99 chars so the remaining tar operations 
 					 * do not complain.
 					 */
					tmpp = ahsStaticGetTarLinkname(file_hdr);
					*(tmpp+(TARLINKNAMESIZE-1)) = '\0';
				}
			}
		} else {
			E_DEBUG("");
			/* ?? dead */
		}

		E_DEBUG2("Entering taru_write_out_header: %s", ahsStaticGetTarFilename(file_hdr));
		retval = taru_write_out_header
			(taru, file_hdr, out_file_des, 
				archive_format_out,
				(char *)(NULL), tarheaderflags);
		if (archive_format_out == arf_ustar ||
			archive_format_out == arf_tar) 
		{
			E_DEBUG("");
			if (linkrecord_active) {
				if (file_hdr->c_nlink > 1) {
					E_DEBUG("");
					hllist_add_record(linkrecord,
					ahsStaticGetTarFilename(file_hdr), 
					(dev_t)makedev(file_hdr->c_dev_maj,\
						file_hdr->c_dev_min),
					file_hdr->c_ino);
				}
			}
			else {
				E_DEBUG("");
				/* dead code */
				if (!linkrecord && 0 /* disable this warning 6/14/2014 */)
				fprintf(stderr,
					"taru internal error: tar hard link handling is disabled (loc=4).\n");
			}
		}
		break;
	} /*case CP_IFREG */
	case CP_IFDIR:
		E_DEBUG("CP_IFDIR");
		file_hdr->c_filesize = 0;
		retval = taru_write_out_header
			(taru, file_hdr, out_file_des, 
				archive_format_out, 
				(char *) (NULL), tarheaderflags);
		break;

#ifdef CP_IFSOCK
	case CP_IFSOCK:
		E_DEBUG("CP_IFSOCK");
		fprintf(stderr, 
		"%s: %s: socket ignored\n", swlib_utilname_get(),
				ahsStaticGetTarFilename(file_hdr));
		retval = 0;
		break;
#endif
#ifdef CP_IFIFO
	case CP_IFIFO:
#endif
	case CP_IFCHR:
	case CP_IFBLK:
		E_DEBUG("CP_FIFO CP_IFCHR CP_IFBLK");
	if (archive_format_out == arf_tar) {
		   fprintf (stderr, "%s not dumped: not a regular file",
		   		ahsStaticGetTarFilename(file_hdr));
		   ahsStaticDeleteFilehdr(file_hdr);
		   return 0;
	} else if (archive_format_out == arf_ustar) {
		E_DEBUG("");
		if (linkrecord_active) {
			E_DEBUG("");
			link_record_buf = hllist_find_file_entry(
				linkrecord,
				(dev_t)makedev(file_hdr->c_dev_maj,\
					file_hdr->c_dev_min),
					file_hdr->c_ino, 1, &nfound);
			if (nfound) {
				E_DEBUG("");
				file_hdr->c_mode |= CP_IFREG;
				ahsStaticSetPaxLinkname(file_hdr,
						link_record_buf->path_);
				retval = taru_write_out_header
					(taru, file_hdr, 
					out_file_des,
					archive_format_out,
					(char *)(NULL),
					tarheaderflags);
				break;
			}
		}
		else {
			E_DEBUG("");
		/*
			fprintf(stderr,
		"taru internal error: tar hard link handling is disabled (loc=5).\n");
		*/
		}
	}
	file_hdr->c_filesize = 0;	
	E_DEBUG("");
	retval = taru_write_out_header
		(taru, file_hdr, out_file_des,
		 archive_format_out,
		(char *)(NULL),
		tarheaderflags);
	if (archive_format_out == arf_ustar || archive_format_out == arf_tar) {
			E_DEBUG("");
			if (linkrecord_active) {
				if (file_hdr->c_nlink > 1) {
					hllist_add_record(linkrecord,
						ahsStaticGetTarFilename(file_hdr), 
						(dev_t)makedev(file_hdr->c_dev_maj,\
							file_hdr->c_dev_min),
							file_hdr->c_ino);
				}
			}
			else {
				E_DEBUG("");
				/*
				fprintf(stderr, 
					"taru internal error: tar hard link handling is disabled (loc=1).\n");
				*/
			}
	}
	break;

#ifdef CP_IFLNK
	
	case CP_IFLNK:
	E_DEBUG("CP_IFLNK");
	{
		E_DEBUG("");
		link_size = strlen(ahsStaticGetTarLinkname(file_hdr));
		file_hdr->c_filesize = link_size;
		if (archive_format_out == arf_tar ||
				archive_format_out == arf_ustar) {
			if (do_gnu_long_link == 0) {
				E_DEBUG("do_gnu_long_link == 0");
				if (link_size + 1 > TARNAMESIZE) {
					fprintf(stderr, "%s: symbolic link too long\n",
						ahsStaticGetTarFilename(file_hdr));
       	                        	link_name[TARNAMESIZE] = '\0';
					return TARU_HE_LINKNAME_4;  /* -4; */
				} else {
					retval = taru_write_out_header
						(taru, file_hdr, out_file_des,
							archive_format_out,
							(char *)(NULL), 
							tarheaderflags);
				}
			} else {
				E_DEBUG("do_gnu_long_link == 1");
				E_DEBUG("calling taru_write_long_link_member");
				if ((do_gnu_long_link_link_bytes=taru_write_long_link_member(taru, out_file_des,
					ahsStaticGetTarLinkname(file_hdr), GNUTYPE_LONGLINK,
					c_type, tarheaderflags)) != 1024) {
					fprintf(stderr, 
					"internal error writing GNU extension LongLink: %s\n",
							ahsStaticGetTarLinkname(file_hdr));
					return -6;
				}
				/*
 				 * Truncate the name so as to make the remaining machinery believe
 				 * everything fits OK.
 				 */
				*(ahsStaticGetTarLinkname(file_hdr)+99) = '\0';
				file_hdr->c_filesize = 0;
				retval = taru_write_out_header
					(taru, file_hdr, out_file_des, 
						archive_format_out, 
						(char *) (NULL), tarheaderflags);
				if (retval < 0)
					return -7;
			}
		} else /* cpio format */{
			E_DEBUG("");
			retval = taru_write_out_header
				(taru, file_hdr, out_file_des,
					archive_format_out,	
					(char *) (NULL), 
					tarheaderflags);
			retval += tarui_tape_buffered_write(
					ahsStaticGetTarLinkname(file_hdr),
					out_file_des, link_size, NULL);
			retval += taru_tape_pad_output(
					out_file_des, (intmax_t)link_size,
					archive_format_out);
		}	
	}
	break;
#endif
	default:
		E_DEBUG("");
		fprintf(stderr, 
		"%s: unknown file type in archive header: mode=%o: FMT=%o: name=%s \n",
			swlib_utilname_get(), 
			(unsigned int)(file_hdr->c_mode),
			(unsigned int)(file_hdr->c_mode & CP_IFMT), ahsStaticGetTarFilename(file_hdr)
			);
	}
	if (file_hdr_return) {
		E_DEBUG("");
		taru_filehdr2filehdr(file_hdr_return, file_hdr);
	}
	E_DEBUG("");
	ahsStaticDeleteFilehdr(file_hdr);
	return retval + do_gnu_long_link_name_bytes + do_gnu_long_link_link_bytes;
}

int
taru_tape_pad_output(int out_file_des, intmax_t offset, 
				enum archive_format archive_format)
{
	intmax_t pad;
	char zeros_512[512];

	memset(zeros_512, 0x00, 512);

	if (archive_format == arf_newascii ||
				archive_format == arf_crcascii)
		pad = (4 - (offset % 4)) % 4;
	else if (archive_format == arf_tar ||
				archive_format == arf_ustar)
		pad = (512 - (offset % 512)) % 512;
	else if (archive_format != arf_oldascii &&
				archive_format != arf_hpoldascii)
		pad = (2 - (offset % 2)) % 2;
	else
		pad = 0;

	if (out_file_des < 0) {
		return 0;
		/* return pad; */
	}

	if (pad != 0) {
		return tarui_tape_buffered_write(zeros_512,
				out_file_des,
				pad, (char *)(NULL));
	} else {
		return 0;
	}
}

int
taru_write_out_header(TARU * taru, struct new_cpio_header *file_hdr, 
		int out_des, 
		enum archive_format archive_format_out, 
		char *header_buffer, int tarheaderflags)
{
	int ret = 0;
	char *magic_string;
	int do_strip_leading_slash = 0;

	E_DEBUG("BEGIN");
	if (taru) { 
		do_strip_leading_slash = 
			tarheaderflags & TARU_TAR_DO_STRIP_LEADING_SLASH;
	}

	if (do_strip_leading_slash) {
		ahsStatic_strip_name_leading_slash(file_hdr);
	}

	if (taru && (taru->preview_levelM > TARU_PV_0)) {
		taru_write_preview_line(taru, file_hdr);
	}
	
	if (archive_format_out == arf_newascii ||
				archive_format_out == arf_crcascii) {
		char ascii_header[112];

		if (archive_format_out == arf_crcascii) {
			magic_string = "070702";
		} else {
			magic_string = "070701";
			file_hdr->c_chksum = 0;
		}
		sprintf(ascii_header,
	"%6s%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx",
			magic_string,
		      file_hdr->c_ino, file_hdr->c_mode, file_hdr->c_uid,
		   file_hdr->c_gid, file_hdr->c_nlink, file_hdr->c_mtime,
			(unsigned long)(file_hdr->c_filesize), file_hdr->c_dev_maj,
			file_hdr->c_dev_min, file_hdr->c_rdev_maj,
			file_hdr->c_rdev_min, file_hdr->c_namesize,
			file_hdr->c_chksum);
		ret += tarui_tape_buffered_write(ascii_header,
					out_des, 110L, header_buffer);
		if (ret < 0) return ret;
		/* Write file name to output.  */
		ret += tarui_tape_buffered_write(
				ahsStaticGetTarFilename(file_hdr),
				out_des,
				(long)(file_hdr->c_namesize),
				header_buffer);
		if (ret < 0) return ret;
		ret += taru_tape_pad_output(
				out_des,
				(intmax_t)(file_hdr->c_namesize + 110),
				archive_format_out);
		return ret;
	} else if (archive_format_out == arf_oldascii ||
				archive_format_out == arf_hpoldascii) {
		char ascii_header[78];
#ifndef __MSDOS__
		dev_t dev = 0;
		dev_t rdev = 0;

		if (archive_format_out == arf_oldascii) {
			magic_string = "070707";
			dev = makedev(file_hdr->c_dev_maj,\
						file_hdr->c_dev_min);
			rdev = makedev(file_hdr->c_rdev_maj,\
						file_hdr->c_rdev_min);
		} else {
			/* HP/UX cpio creates archives that look just 
			  like ordinary archives, but for devices it 
			  sets major = 0, minor = 1, and puts the
			  actual major/minor number in the filesize field.  */
			magic_string = "070707"; /* FIXME. is this correct? */
			switch (file_hdr->c_mode & CP_IFMT) {
			case CP_IFCHR:
			case CP_IFBLK:
#ifdef CP_IFSOCK
			case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
			case CP_IFIFO:
#endif
				file_hdr->c_filesize = 
					makedev(file_hdr->c_rdev_maj,\
					   file_hdr->c_rdev_min);
				rdev = 1;
				break;
			default:
				dev = makedev(file_hdr->c_dev_maj,\
							file_hdr->c_dev_min);
				rdev = makedev(file_hdr->c_rdev_maj,\
							file_hdr->c_rdev_min);
				break;
			}
		}
#else
		int dev = 0, rdev = 0;
#endif

		if ((file_hdr->c_ino >> 16) != 0)
			fprintf(stderr, "%s: truncating inode number",
					ahsStaticGetTarFilename(file_hdr));

		sprintf(ascii_header,
		"%6s%06o%06lo%06lo%06lo%06lo%06lo%06o%011lo%06lo%011lo",
			magic_string, (int) (dev & 0xFFFF),
		     file_hdr->c_ino & 0xFFFF, file_hdr->c_mode & 0xFFFF,
		      file_hdr->c_uid & 0xFFFF, file_hdr->c_gid & 0xFFFF,
			file_hdr->c_nlink & 0xFFFF, (int) (rdev & 0xFFFF),
			file_hdr->c_mtime, file_hdr->c_namesize & 0xFFFF,
			(unsigned long)(file_hdr->c_filesize));
		ret+=tarui_tape_buffered_write(ascii_header,
						out_des,
						76L, header_buffer);
		if (ret < 0) return ret;
		ret+=tarui_tape_buffered_write(
				ahsStaticGetTarFilename(file_hdr),
				out_des, (long)file_hdr->c_namesize,
				header_buffer);
		return ret;	
	/* TAR */
	} else if (archive_format_out == arf_tar ||
				archive_format_out == arf_ustar) {
		E_DEBUG("calling taru_write_out_tar_header2");
		ret = taru_write_out_tar_header2(taru, 
						file_hdr,
						out_des,
						(char *)(NULL), 
					ahsStaticGetTarUsername(file_hdr),
					ahsStaticGetTarGroupname(file_hdr),
							tarheaderflags);
		return ret;
	} else {
		struct old_cpio_header short_hdr;

		short_hdr.c_magic = 070707;
		short_hdr.c_dev = makedev(file_hdr->c_dev_maj,\
						 file_hdr->c_dev_min);

		if ((file_hdr->c_ino >> 16) != 0)
			fprintf(stderr, "%s: truncating inode number",
					 ahsStaticGetTarFilename(file_hdr));

		short_hdr.c_ino = file_hdr->c_ino & 0xFFFF;
		short_hdr.c_mode = file_hdr->c_mode & 0xFFFF;
		short_hdr.c_uid = file_hdr->c_uid & 0xFFFF;
		short_hdr.c_gid = file_hdr->c_gid & 0xFFFF;
		short_hdr.c_nlink = file_hdr->c_nlink & 0xFFFF;
		if (archive_format_out != arf_hpbinary)
			short_hdr.c_rdev = makedev(file_hdr->c_rdev_maj,\
							file_hdr->c_rdev_min);
		else {
			switch (file_hdr->c_mode & CP_IFMT) {
				/* HP/UX cpio creates archives that 
				   look just like ordinary archives, but 
				   for devices it sets major = 0, minor = 1,
				   and puts the actual major/minor number
				   in the filesize field.  */
			case CP_IFCHR:
			case CP_IFBLK:
#ifdef CP_IFSOCK
			case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
			case CP_IFIFO:
#endif
				file_hdr->c_filesize = 
					makedev(file_hdr->c_rdev_maj,\
						   file_hdr->c_rdev_min);
				short_hdr.c_rdev = makedev(0, 1);
				break;
			default:
				short_hdr.c_rdev = 
					makedev(file_hdr->c_rdev_maj,\
						   file_hdr->c_rdev_min);
				break;
			}
		}
		short_hdr.c_mtimes[0] = file_hdr->c_mtime >> 16;
		short_hdr.c_mtimes[1] = file_hdr->c_mtime & 0xFFFF;

		short_hdr.c_namesize = file_hdr->c_namesize & 0xFFFF;

		short_hdr.c_filesizes[0] = file_hdr->c_filesize >> 16;
		short_hdr.c_filesizes[1] = file_hdr->c_filesize & 0xFFFF;

		/* Output the file header.  */
		ret+=tarui_tape_buffered_write((char *)&short_hdr,
						out_des, 26L,
						header_buffer);

		/* Write file name to output.  */
		ret+=tarui_tape_buffered_write(
					ahsStaticGetTarFilename(file_hdr),
					out_des,
					(long)file_hdr->c_namesize,
					header_buffer);
		if (ret < 0) return ret;
		ret+=taru_tape_pad_output(out_des,
					 (intmax_t)(file_hdr->c_namesize + 26),
					archive_format_out);
		return ret;
	}
	return -254;
}


/* reads a tar archive on input_fd and writes the archive_format to
   output_fd */


int
taru_process_copy_out(TARU * taru, int input_fd, int out_file_des,
			DEFER * defer, PORINODE * porinode,
			enum archive_format archive_format_out,
			int ls_fd, int ls_verbose_level, intmax_t * nb, FILE_DIGS * digs)
{
	STROB * ls_tmp;
	struct new_cpio_header * file_hdr0;
	union tar_record tar_rec;
	struct tar_header *tar_hdr = (struct tar_header *) &tar_rec;
	char zeros_512[512];
	int block_number = 0;
	int eoa = 0;
	int ret;
	intmax_t retx;
	intmax_t retvalx;
	int read_ret;
	HLLIST * hllist;

	E_DEBUG("");
	file_hdr0 = ahsStaticCreateFilehdr();
	memset(zeros_512, 0x00, 512);
	hllist = hllist_open();
	hllist_disable_add(hllist);
	hllist_disable_find(hllist);
	retvalx = 0;

	E_DEBUG("");
	taru_digs_delete(file_hdr0->digsM);
	file_hdr0->digsM = digs;

	E_DEBUG("");
	if (ls_fd >= 0) {
		ls_tmp = strob_open(120);
	} else {
		ls_tmp = NULL;
	}

	E_DEBUG("");
	eoa = 0;
	read_ret = 0;
	while (eoa == 0) {
		E_DEBUG("");
		read_ret = taru_read_in_tar_header2(taru, file_hdr0, input_fd, NULL, &eoa, 0, -1);
		if (read_ret < 0)
			break;
		if (!eoa)  {
			E_DEBUG("");
			retx = tarui_write_archive_member_
				(taru, (struct stat*)NULL, file_hdr0,
					hllist, defer, porinode,
					out_file_des, 
					input_fd, archive_format_out,
					taru->taru_tarheaderflagsM /* Was 0 Apr2014*/);
	
			if (retx < 0) {
				return -9;
			}

			retvalx += retx;
			if ((retx=taru_tape_skip_padding(input_fd, file_hdr0->c_filesize, arf_ustar)) < 0) {
				return -8;
			}

			if (ls_fd >= 0) {
				taru_print_tar_ls_list(ls_tmp, file_hdr0, ls_verbose_level);
				uxfio_write(ls_fd, strob_str(ls_tmp), strob_strlen(ls_tmp));
			} else {
				;
			}
		}
		E_DEBUG("");
	} /* while not at EOA end of archive */
	E_DEBUG("");
	if (read_ret < 0) return -11;

	/* The collection is complete; append the trailer.  */
	ret = taru_write_archive_trailer(taru,
				archive_format_out,
				out_file_des, 512, 0, 0);
	E_DEBUG("");
	if (ret <= 0) return -10;
	retvalx += (intmax_t)ret;
	if (nb) *nb = (intmax_t)retvalx;
	if (ls_tmp) strob_close(ls_tmp);
	file_hdr0->digsM = NULL; /* Prevents deleting passed-in object */
	ahsStaticDeleteFilehdr(file_hdr0);
	hllist_close(hllist);
	E_DEBUG("");
	return 0;
}
