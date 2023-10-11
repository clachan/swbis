/* swverify_lib.h - swverify routines
  
 Copyright (C) 2007 James H. Lowe, Jr.

 COPYING TERMS AND CONDITIONS:
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef swverify_lib_200609_h
#define swverify_lib_200609_h

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vplob.h"
#include "usgetopt.h"
#include "swcommon0.h"
#include "swextopt.h"
#include "swutillib.h"
#include "swverid.h"
#include "globalblob.h"
	

#define SWVERIFY_RP_STATUS_NO_INTEGRITY	4

#ifndef DEFAULT_PAX_REM 
#define DEFAULT_PAX_REM "tar"
#endif

#define SWVERIFY_OUTF_RPM	1  /* selected by --output-form=rpm */
#define SWVERIFY_OUTF_DIFF	0  /* default */
#define SWVERIFY_OUTF_ALL	3  /* both rpm and diff */

#define INL_ATT_SIZE (sizeof(char*) + sizeof(int) + sizeof(int))

#define SHA1_ASCII_LEN          40
#define MD5_ASCII_LEN           32

#define INL_BF_NAME		(1 << 0)
#define INL_BF_LINKNAME		(1 << 1)
#define INL_BF_SIZE		(1 << 2)
#define INL_BF_PERMISSIONS	(1 << 3)
#define INL_BF_OWNERS		(1 << 4)
#define INL_BF_MDATE		(1 << 5)
#define INL_BF_MD5		(1 << 6)
#define INL_BF_SHA1		(1 << 7)
#define INL_BF_SHA512		(1 << 8)

#define INL_NAME_OFFSET			0
#define INL_LINKNAME_OFFSET		((int)((char*)(&(AA.linknameM)) - ((char*)(&AA))))
#define INL_SIZE_OFFSET 		((int)((char*)(&(AA.sizeM)) - ((char*)(&AA))))
#define INL_PERMISSIONS_OFFSET 		((int)((char*)(&(AA.permissionsM)) - ((char*)(&AA))))
#define INL_OWNERS_OFFSET 		((int)((char*)(&(AA.ownersM)) - ((char*)(&AA))))
#define INL_MDATE_OFFSET 		((int)((char*)(&(AA.mdateM)) - ((char*)(&AA))))
#define INL_MD5_OFFSET 			((int)((char*)(&(AA.md5M)) - ((char*)(&AA))))
#define INL_SHA1_OFFSET 		((int)((char*)(&(AA.sha1M)) - ((char*)(&AA))))
#define INL_SHA512_OFFSET 		((int)((char*)(&(AA.sha512M)) - ((char*)(&AA))))

				/* ranked from OK to absolute worst */
#define INL_STATUS_UNSET	5	/*  (OK) */
#define INL_STATUS_0_0_NA	4	/*  (OK) missing from system and INFO */
#define INL_STATUS_1_0_NA	3	/*  if present in INFO and empty in system */
#define INL_STATUS_1_1_EQ	2	/*  if identical */
#define INL_STATUS_0_1_NA	1	/*  if present in system, but missing from INFO */
#define INL_STATUS_1_1_NEQ	0	/*  (worst) present in INFO and system and unequal */
#define INL_STATUS_MISSING_FILE -1	/* even worst */

#define RPMV_RESULT_STRING_LEN	8
#define RPMV_STATUS_STRING	"SM5DLUGTP"
#define RPMV_FORMAT_LEN	(RPMV_RESULT_STRING_LEN+3)   /* ``SM5DLUGTP X '' */

#define RPMV_fail_size		'S'
#define RPMV_fail_mode		'M'
#define RPMV_fail_digest	'5'
#define RPMV_fail_device	'D'
#define RPMV_fail_linkname	'L'
#define RPMV_fail_user		'U'
#define RPMV_fail_group		'G'
#define RPMV_fail_mtime		'T'
#define RPMV_fail_cap		'P'
#define RPMV_fail_NOT		'.'
#define RPMV_fail_unknown	'?'
#define RPMV_fail_fail		'X'  /* used as catch-all value for failed, never displayed to user */
#define RPMV_fail_MISSING	'x'  /* used as catch-all value for failed, never displayed to user */

#define RPMV_offset_size	0
#define RPMV_offset_mode	1	
#define RPMV_offset_digest	2	
#define RPMV_offset_device	3	
#define RPMV_offset_linkname	4	
#define RPMV_offset_user	5	
#define RPMV_offset_group	6	
#define RPMV_offset_mtime	7	

typedef struct {
	char * addrM;   /* C-address within line */
	int lenM;       /* length of data */
	int statusM;
} INFOITEM;

typedef struct {
	INFOITEM nameM;
	INFOITEM linknameM;
	INFOITEM sizeM;
	INFOITEM permissionsM;
	INFOITEM ownersM;
	INFOITEM mdateM;
	INFOITEM md5M;
	INFOITEM sha1M;
	INFOITEM sha512M;
	STROB * rpmvM;  /* RPM-like verification string */
	char * lineM;	/* address of original line */
} INFOLINE;


INFOLINE * swverify_inl_create(void);
void swverify_inl_delete(INFOLINE*);
int swverify_inl_parse(INFOLINE*, char * line);

int swverify_write_source_copy_script2(GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv,
		int delaytime, int nhops, char * pax_write_command_key, char * hostname, char * blocksize);

int swverify_write_source_copy_script(GB * G, int ofd, char * sourcepath, int do_get_file_type, int vlv,
		int delaytime, int nhops, char * pax_write_command_key, char * hostname, char * blocksize);


int swverify_list_distribution_archive(GB * G, struct extendedOptions * opta,
	VPLOB * swspecs, char * target_path, int target_nhops, int efd);

SWI * swverify_list_create_swi(GB * G, struct extendedOptions * opta, VPLOB * swspecs, char * target_path);
int swverify_ls_fileset_from_iscat(SWI * swi);
int swverify_looper_sr_payload(GB * G, char * target_path, SWICOL * swicol, SWICAT_SC * sc, SWICAT_SR * sr,
			int ofd, int ifd, int *, SWUTS * uts, char * pax_write_command_key, int *);

char * swverify_v_looper_prelink_md5_func(STROB * buf);

#endif
