/* readinheader.c - read in an archive header

   Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.
   Copyright (C) 2002 Jim Lowe

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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG


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
#include "taruib.h"
#include "taru_debug.h"
#include "swutilname.h"
#include "strob.h"
#include "cplob.h"
/* #include "swgp.h" */

#ifndef HAVE_LCHOWN
#define lchown chown
#endif

/* #define TARU_DEBUG_LINKS */

void taru_read_in_binary (struct new_cpio_header *  file_hdr, int in_des);
void taru_swab_array (char * ptr, int count);

/* Return 16-bit integer I with the bytes swapped.  */
#define swab_short(i) ((((i) << 8) & 0xff00) | (((i) >> 8) & 0x00ff))


static int
set_unsigned_long_value(unsigned long *al, unsigned long * al_nsec, char * value, int valuelen)
{
	unsigned long int ret;
	int result;
	char * endptr;

	if (al_nsec)
		*al_nsec = 0;	

	ret = swlib_atoul(value, &result, &endptr);
	if (result) {
		*al = 0; /*error */
		return result;
	} else {
		*al = ret;
	}

	if (al_nsec && *endptr == '.') {
		/* parse the nanosecond part */
		ret = swlib_atoul(endptr + 1, &result, (char**)NULL);
		if (result) {
			return result;
		} else {
			*al_nsec = ret;
		}
	}
	return result;
}

intmax_t
taru_pump_amount2(int discharge_fd, int suction_fd,
			intmax_t amount, int adjunct_ofd)
{
	intmax_t ret;
	intmax_t i = amount;
	if ((ret=swlib_i_pipe_pump(suction_fd, discharge_fd,
				&i, adjunct_ofd, NULL, taru_tape_buffered_read))) {
		return -1;
	}
	return i;
}

intmax_t
taru_read_amount(int suction_fd, intmax_t amount)
{
	return taru_pump_amount2(-1, suction_fd, amount, -1);
}

ssize_t
taru_tape_buffered_read(int fd, void * buf, size_t pcount)
{
	int taruib_fd;
	int count;
	
	E_DEBUG("ENTERING");
	
	taruib_fd = taruib_get_fd();
	count = uxfio_sfa_read(fd, buf, pcount);

	E_DEBUG3("taruib_fd=%d,  count=%d", taruib_fd, (int)count);

	if (taruib_fd > 0 && count > 0) {
		E_DEBUG("buffer taruib");
		char * buffer = taruib_get_buffer();
		int data_len = taruib_get_datalen();	
		int buffer_reserve = taruib_get_nominal_reserve() - data_len;	
		char * buffer_dst;

		if (buffer_reserve < count &&
				taruib_get_overflow_release() == 0) {
			E_DEBUG3("CASE1 res %d  count %d", buffer_reserve, count);
			buffer_dst = buffer;
			taruib_set_datalen(count);
			uxfio_write(taruib_fd, buffer_dst, data_len); 
		} else {
			E_DEBUG3("CASE2 res %d  count %d", buffer_reserve, count);
			if (data_len + (int)pcount > taruib_get_reserve()) {
				fprintf(stderr,
		"taru_tape_buffered_read fatal error: stop, stop, stop.\n");
				exit(2);
			}
			buffer_dst = buffer + data_len;
			taruib_set_datalen(data_len + count);
		}
		memcpy(buffer_dst, buf, count);
	}
	E_DEBUG("LEAVING");
	return count;
}

int
taru_exatt_override_ustar (struct new_cpio_header * file_hdr, EXATT * exatt)
{
	char * attr;
	char * value;
	int ret = 0;
	int valuelen;
	if (
		strlen(exatt->namespaceM) == 0 &&
		islower((int)(*((char*)strob_str((STROB*)exatt->attrnameM))))
	) {
		/*
 		 * POSIX ustar attribute
 		 * Update the file_hdr object with its value
 		 */


		/*
 		 * Since the value is being transferred to the  file_hdr structure proper
 		 * set the is_in_useM flag to zero
 		 */
		exatt->is_in_useM = 0;

		attr = strob_str((STROB*)exatt->attrnameM);
		value = strob_str((STROB*)exatt->attrvalueM);
		valuelen = exatt->lenM;

		if (strcmp(attr, PAX_KW_path) == 0) {
			ahsStaticSetTarFilename(file_hdr, value);
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_PATH;
		} else if (strcmp(attr, PAX_KW_linkpath) == 0) {
			ahsStaticSetPaxLinkname(file_hdr, value);
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_LINKPATH;
		} else if (strcmp(attr, PAX_KW_gname) == 0) {
			ahsStaticSetTarGroupname(file_hdr, value);
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_GNAME;
		} else if (strcmp(attr, PAX_KW_uname) == 0) {
			 ahsStaticSetTarUsername(file_hdr, value);
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_UNAME;
		} else if (strcmp(attr, PAX_KW_atime) == 0) {
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_ATIME;
  			ret = set_unsigned_long_value(&(file_hdr->c_atime), &(file_hdr->c_atime_nsec), value, valuelen);
	  		if (ret) fprintf(stderr, "%s: error evaluating attribute in extended header: %s\n", swlib_utilname_get(), "atime");
		} else if (strcmp(attr, PAX_KW_mtime) == 0) {
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_MTIME;
			ret = set_unsigned_long_value(&(file_hdr->c_mtime), NULL, value, valuelen);
	  		if (ret) fprintf(stderr, "%s: error evaluating attribute in extended header: %s\n", swlib_utilname_get(), "mtime");
		} else if (strcmp(attr, PAX_KW_ctime) == 0) {
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_CTIME;
			ret = set_unsigned_long_value(&(file_hdr->c_ctime), &(file_hdr->c_ctime_nsec), value, valuelen);
	  		if (ret) fprintf(stderr, "%s: error evaluating attribute in extended header: %s\n", swlib_utilname_get(), "ctime");
		} else if (strcmp(attr, PAX_KW_gid) == 0) {
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_GID;
			ret = set_unsigned_long_value(&(file_hdr->c_gid), NULL, value, valuelen);
	  		if (ret) fprintf(stderr, "%s: error evaluating attribute in extended header: %s\n", swlib_utilname_get(), "gid");
		} else if (strcmp(attr, PAX_KW_uid) == 0) {
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_UID;
			ret = set_unsigned_long_value(&(file_hdr->c_uid), NULL, value, valuelen);
	  		if (ret) fprintf(stderr, "%s: error evaluating attribute in extended header: %s\n", swlib_utilname_get(), "uid");
		} else if (strcmp(attr, PAX_KW_size) == 0) {
			file_hdr->extHeader_usage_maskM |= TARU_EHUM_SIZE;
			ret = set_unsigned_long_value(&(file_hdr->c_uid), NULL, value, valuelen);
	  		if (ret) fprintf(stderr, "%s: error evaluating attribute in extended header: %s\n", swlib_utilname_get(), "size");
		} else {
			;
			/* this should be an error or atleast a warning*/
			exatt->is_in_useM = 1;
	  		fprintf(stderr, "%s: warning: lower case attribute name not processed: %s\n", swlib_utilname_get(), attr);
		}
	}
	return ret;
}

char * 
taru_read_ext_header_record (struct new_cpio_header * file_hdr, char * data, int * errorcode)
{
	char * n_data;
	char * pend;
	char * pos;
	char * p;
	int result;
	int len;
	char * attr = NULL;
	char * value = NULL;	
	int valuelen;
	int attrlen;
	int retval;
	int ret;
	EXATT * exatt;

	n_data = data;
	len = swlib_atoi2(n_data, &pend, &result);

	pos = pend;
	if (!isspace(*pos)) {
		/* error */
		if (errorcode) *errorcode = -1;
		return NULL;
	}

	/*
 	 * Skip over the space, there really should only be one
 	 */	
	if (isspace(*pos)) {
		pos++;
	}

	/*
 	 * pos now points to the first charactor of the attribute name
 	 */
	attr = pos;
	p = strchr(attr, '=');
	if (!p) {
		if (errorcode) *errorcode = -2;
		return NULL;
	}
	attrlen = p - attr;
	if (attrlen < 0) {
		if (errorcode) *errorcode = -3;
		return NULL;
	}

	p++;

	/*
 	 * p now points to the first char of the value
 	 */
	value = p;
	valuelen = len - (int)(p - data) - 1 /*newline*/;
	if (valuelen < 0) {
		if (errorcode) *errorcode = -4;
		return NULL;
	}

	exatt = taru_exattlist_get_free(file_hdr);

	retval = taru_exatt_parse_fqname(exatt, attr, attrlen);
	if (retval < 0) {
		if (errorcode) *errorcode = -5;
		return NULL;
	}

	/*
 	 * Now copy the value
 	 */
	strob_memcpy(exatt->attrvalueM, (void *)value, valuelen);

	/*
 	 * append a NUL to the end of the value which is not part of the value
 	 */
	strob_chr_index(exatt->attrvalueM, valuelen, (int)'\0');
	exatt->lenM = valuelen;
	exatt->is_in_useM = 1;

	/*
 	 * return first byte after the trailing newline of the
 	 * extended header record.
 	 */
	return value + valuelen + 1/*newline*/;
}

int
taru_read_all_ext_header_records (struct new_cpio_header * file_hdr, char * data, int len, int * errorcode)
{
	char * tarheaderdata;
	char * current_pos;
	char * retpos;
	int current_offset;
	int retval;
	int num_of_records;

	tarheaderdata = data;
	current_pos = data;	
	current_offset = 0;
	retval = 0;
	num_of_records = 0;

	file_hdr->extHeader_usage_maskM |= TARU_EHUM_ANY;
	retpos = data;
	while (retpos && (current_pos - data) < len) {
		retpos = taru_read_ext_header_record (file_hdr, current_pos, errorcode);
		if (retpos == NULL) {
			return -num_of_records;
		}
		/*
 		 * Note: it is possible retpos points one byte after the last byte of allocated memory
 		 * when the last byte of a 512 byte tarblock is the trailing newline of the last
 		 * extended header record.  This is OK here because it is never derefenced.
 		 */
		current_pos = retpos;	
		num_of_records++;
	}
	return num_of_records;
}

int 
taru_read_header (TARU * taru, struct new_cpio_header * file_hdr,
		int in_des, enum archive_format format, int * eoa, int flags)
{
	int retval;
	char * link_name;
	int ret;

	E_DEBUG("ENTERING");
	E_DEBUG2("fildes=%d", in_des);

	ret=taru_read_in_header(taru, file_hdr, in_des, format, eoa, flags);
	if (ret < 0)
		return -1;
	retval = ret;
	if (format != arf_tar && format != arf_ustar) {
		if ((file_hdr->c_mode & CP_IFMT) == CP_IFLNK) {
			link_name = (char *)malloc((unsigned int)
						file_hdr->c_filesize + 1);
			link_name[(int)(file_hdr->c_filesize)] = '\0';
			ret = taru_tape_buffered_read(in_des, link_name, (size_t)(file_hdr->c_filesize));
			if (ret < 0) return -retval;
			retval += ret;
			ret = taru_tape_skip_padding(in_des,
						file_hdr->c_filesize, format);
			if (ret < 0) return -retval;
			retval += ret;
			ahsStaticSetPaxLinkname(file_hdr, link_name);
		} else {
			ahsStaticSetPaxLinkname(file_hdr, NULL);
		}
	}
	E_DEBUG("LEAVING");
	return retval;
}


int 
taru_read_in_header (TARU * taru, struct new_cpio_header * file_hdr,
			int in_des, enum archive_format archive_format_in0,
						int * p_eoa, int flags)
{
  long bytes_skipped = 0;	/* Bytes of junk found before magic number.  */
  int ret;
  int retval = 0;
  /* Search for a valid magic number.  */

  E_DEBUG("ENTERING");
  if (archive_format_in0 == arf_unknown)
    {
      fprintf (stderr, " format unknown in read_in_header.\n");
      return -1;
    }

  if (archive_format_in0 == arf_tar || archive_format_in0 == arf_ustar)
    {
        E_DEBUG("");
	if ((retval=taru_read_in_tar_header2(taru, file_hdr, in_des,
					(char*)(NULL), p_eoa, flags, TARRECORDSIZE)) < 0) {
		E_DEBUG("LEAVING");
      		return -1;
	} else {
		E_DEBUG("LEAVING");
		return retval;
    	}
    }

  ahsStaticSetPaxLinkname(file_hdr, NULL);

  if ((retval=taru_tape_buffered_read(in_des, (void*)file_hdr, (size_t)(6))) != 6){
        E_DEBUG("");
    	fprintf (stderr, "error reading magic. retval= %d\n", retval);
	E_DEBUG("LEAVING");
  	return -1; 
  } 
  
  while (1)
    {
      if (archive_format_in0 == arf_newascii
	  && !strncmp ((char *) file_hdr, "070701", 6))
	{
          E_DEBUG("");
	  if (bytes_skipped > 0)
	    fprintf (stderr, 
			"warning: skipped %ld bytes of junk", bytes_skipped);
	  if ((ret = taru_read_in_new_ascii(taru, file_hdr, in_des,
						archive_format_in0)) < 0) {
	    	fprintf (stderr, "error from taru_read_in_new_ascii");
		E_DEBUG("LEAVING");
	  	return -retval;
	  }
          if (!strcmp(ahsStaticGetTarFilename(file_hdr),
					CPIO_INBAND_EOA_FILENAME)) 
	  	if (p_eoa) 
			*p_eoa = 1;
	  retval += ret;
	  break;
	}
      if (archive_format_in0 == arf_crcascii
	  && !strncmp ((char *) file_hdr, "070702", 6))
	{
          E_DEBUG("");
	  if (bytes_skipped > 0)
	    fprintf (stderr, 
		"warning: skipped %ld bytes of junk", bytes_skipped);
	  if ((ret = taru_read_in_new_ascii(taru, file_hdr, in_des,
						archive_format_in0)) < 0)  {
		E_DEBUG("LEAVING");
	  	return -retval;
	  }
          if (!strcmp(ahsStaticGetTarFilename(file_hdr),
			CPIO_INBAND_EOA_FILENAME)) 
	  	if (p_eoa)
			*p_eoa = 1;
	  retval += ret;
	  break;
	}
      if ( (archive_format_in0 == arf_oldascii ||
				archive_format_in0 == arf_hpoldascii)
	  			&& !strncmp ((char *) file_hdr, "070707", 6))
	{
          E_DEBUG("");
	  if (bytes_skipped > 0)
	    fprintf  (stderr , 
			"warning: skipped %ld bytes of junk", bytes_skipped);
	  ret = taru_read_in_old_ascii2(taru, file_hdr, in_des, (char*)(NULL));
	  if (ret <= 0) {
		E_DEBUG("LEAVING");
		return -retval;
	  }
          if (!strcmp(ahsStaticGetTarFilename(file_hdr),
			CPIO_INBAND_EOA_FILENAME)) 
	  	if (p_eoa)
			*p_eoa = 1;
	  retval += ret;
	  break;
	}
      if ((archive_format_in0 == arf_binary ||
			archive_format_in0 == arf_hpbinary)
	  && (file_hdr->c_magic == 070707
	      || file_hdr->c_magic == swab_short ((unsigned short) 070707)))
	{
          E_DEBUG("");
	  /* Having to skip 1 byte because of word alignment is normal.  */
	  fprintf(stderr, "arf_binary arf_hpbinary not supported.\n");
	  break; 
	  /* 
	  if (bytes_skipped > 0)
	    fprintf  (stderr,
			"warning: skipped %ld bytes of junk", bytes_skipped);
	  ret = taru_read_in_binary(file_hdr, in_des);
	  if (ret <= 0) {
		E_DEBUG("LEAVING");
		return -retval;
	  }
	  retval += ret;
	  break;
	  */
	}
      bytes_skipped++;
      memmove ((char *) file_hdr, (char *) file_hdr + 1, 5); 
      if ((ret = taru_tape_buffered_read(in_des, (void*) ((char*)file_hdr + 5), (size_t)(1))) <= 0) {
	    fprintf  (stderr, 
		"error: header magic not found and subsequent read error.\n");
	    E_DEBUG("LEAVING");
            return -retval;
      }
      retval += ret;
    }
  E_DEBUG("LEAVING");
  return retval;
}

