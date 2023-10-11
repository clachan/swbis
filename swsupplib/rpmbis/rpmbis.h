/* rpmbis.h -- LSB RPM public file format data structures */
/* This file is part of GNU swbis

   Copyright (C) 2005  James H. Lowe, Jr.
   All Rights Reserved.
   
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef rpmbis_h_20050611
#define rpmbis_h_20050611

#include <netinet/in.h>

#define RPM_MAGIC "\355\253\356\333"  /* ED AB EE DB */
#define RPM_HEADER_MAGIC "\216\255\350\001"  /* 8E AD E8 01 */

#define RPM_NULL_TYPE 0
#define RPM_CHAR_TYPE 1
#define RPM_INT8_TYPE 2
#define RPM_INT16_TYPE 3
#define RPM_INT32_TYPE 4
#define RPM_INT64_TYPE 5
#define RPM_STRING_TYPE 6
#define RPM_BIN_TYPE 7
#define RPM_STRING_ARRAY_TYPE 8
#define RPM_I18NSTRING_ARRAY_TYPE 9
#define RPM_I18NSTRING_TYPE 9

#define RPM_HEADER_SIZE 16
#define RPM_HDRINDEX_SIZE 16
#define RPM_LEAD_SIZE 96

/*
 * --------------------------------------------------
 * RPM portable byte-offset oriented data structures
 * ---------------------------------------------------
 */

/* Lead, portable form */
struct rpmbis_px_rpmlead {                  /* byte offset */
	unsigned char px_magic[4];		/*  0 */
	unsigned char px_major;			/*  4 */
	unsigned char px_minor;			/*  5 */
	unsigned char px_type[2];		/*  6 */
	unsigned char px_archnum[2];		/*  8 */
	unsigned char px_name[66];		/* 10 */ 
	unsigned char px_osnum[2];		/* 76 */
	unsigned char px_signature_type[2];	/* 78 */
	unsigned char px_reserved[16];		/* 80 */
};  /* total size of 96 bytes */

/* Header Record, portable form, Length 16 bytes */ 
struct rpmbis_px_rpmheader {
	unsigned char px_magic[4];  /* always "\216\255\350\001" */
	unsigned char px_reserved[4];   /*  4 */
	unsigned char px_nindex[4];     /*  8 */
	unsigned char px_hsize[4];      /* 12 */
};   /* total length is 16 bytes */

/* Index Record, portable form Length 16 bytes */ 
struct rpmbis_px_rpmhdrindex {
	unsigned char px_tag[4];
	unsigned char px_type[4];
	unsigned char px_offset[4];
	unsigned char px_count[4];
};

typedef struct rpmbis_px_rpmlead RPM_PX_LEAD;
typedef struct rpmbis_px_rpmheader RPM_PX_HSHEADER;
typedef struct rpmbis_px_rpmhdrindex RPM_PX_HDRINDEX;

/*
 * +++++++++++++++++++++++++++++++++++++++++
 *  Tags
 * +++++++++++++++++++++++++++++++++++++++++
 */

/* RPM tag values, from Linux Standard Base Core Specification 2.1 */

/*   Table 1-4. Header Private Tag Values
        Name           Tag Value     Type     Count  Status */
#define RPMTAG_HEADERSIGNATURES 62    /*  BIN          16    Optional */
#define RPMTAG_HEADERIMMUTABLE  63    /*  BIN          16    Optional */
#define RPMTAG_HEADERI18NTABLE  100   /*  STRING_ARRAY       Required */

/*   Table 1-5. Signature Tag Values
        Name        Tag Value Type  Count  Status */
#define SIGTAG_SIGSIZE     1000   /*   INT32 1     Required */
#define SIGTAG_PAYLOADSIZE 1007   /*   INT32 1     Optional */

/*   Table 1-7. Signature Signing Tag Values
       Name       Tag Value Type Count  Status */
#define SIGTAG_PGP       1002  /*  BIN  1     Optional */
#define SIGTAG_GPG       1005  /*  BIN  65    Optional */
#define SIGTAG_DSAHEADER 1011  /*  BIN  1     Optional */
#define SIGTAG_RSAHEADER 1012  /*  BIN  1     Optional */

/*   Table 1-8. Package Info Tag Values
           Name           Tag Value    Type    Count  Status */
#define RPMTAG_NAME              1000  /*  STRING     1     Required  */
#define RPMTAG_VERSION           1001  /*  STRING     1     Required  */
#define RPMTAG_RELEASE           1002  /*  STRING     1     Required  */
#define RPMTAG_SUMMARY           1004  /*  I18NSTRING 1     Required  */
#define RPMTAG_DESCRIPTION       1005  /*  I18NSTRING 1     Required  */
#define RPMTAG_SIZE              1009  /*  INT32      1     Required  */
#define RPMTAG_LICENSE           1014  /*  STRING     1     Required  */
#define RPMTAG_GROUP             1016  /*  I18NSTRING 1     Required  */
#define RPMTAG_OS                1021  /*  STRING     1     Required  */
#define RPMTAG_ARCH              1022  /*  STRING     1     Required  */
#define RPMTAG_SOURCERPM         1044  /*  STRING     1     Optional  */
#define RPMTAG_ARCHIVESIZE       1046  /*  INT32      1     Optional  */
#define RPMTAG_RPMVERSION        1064  /*  STRING     1     Optional  */
#define RPMTAG_COOKIE            1094  /*  STRING     1     Optional  */
#define RPMTAG_PAYLOADFORMAT     1124  /*  STRING     1     Required  */
#define RPMTAG_PAYLOADCOMPRESSOR 1125  /*  STRING     1     Required  */
#define RPMTAG_PAYLOADFLAGS      1126  /*  STRING     1     Required  */

/*   Table 1-9. Installation Tag Values
        Name        Tag Value  Type  Count  Status */
#define RPMTAG_PREIN      1023   /* STRING 1     Optional */
#define RPMTAG_POSTIN     1024   /* STRING 1     Optional */
#define RPMTAG_PREUN      1025   /* STRING 1     Optional */
#define RPMTAG_POSTUN     1026   /* STRING 1     Optional */
#define RPMTAG_PREINPROG  1085   /* STRING 1     Optional */
#define RPMTAG_POSTINPROG 1086   /* STRING 1     Optional */
#define RPMTAG_PREUNPROG  1087   /* STRING 1     Optional */
#define RPMTAG_POSTUNPROG 1088   /* STRING 1     Optional */

/*   Table 1-10. File Info Tag Values
          Name         Tag Value     Type     Count  Status */
#define RPMTAG_OLDFILENAMES  1027  /*  STRING_ARRAY       Optional */
#define RPMTAG_FILESIZES     1028  /*  INT32              Required */
#define RPMTAG_FILEMODES     1030  /*  INT16              Required */
#define RPMTAG_FILERDEVS     1033  /*  INT16              Required */
#define RPMTAG_FILEMTIMES    1034  /*  INT32              Required */
#define RPMTAG_FILEMD5S      1035  /*  STRING_ARRAY       Required */
#define RPMTAG_FILELINKTOS   1036  /*  STRING_ARRAY       Required */
#define RPMTAG_FILEFLAGS     1037  /*  INT32              Required */
#define RPMTAG_FILEUSERNAME  1039  /*  STRING_ARRAY       Required */
#define RPMTAG_FILEGROUPNAME 1040  /*  STRING_ARRAY       Required */
#define RPMTAG_FILEDEVICES   1095  /*  INT32              Required */
#define RPMTAG_FILEINODES    1096  /*  INT32              Required */
#define RPMTAG_FILELANGS     1097  /*  STRING_ARRAY       Required */
#define RPMTAG_DIRINDEXES    1116  /*  INT32              Optional */
#define RPMTAG_BASENAMES     1117  /*  STRING_ARRAY       Optional */
#define RPMTAG_DIRNAMES      1118  /*  STRING_ARRAY       Optional */

/*   Table 1-11. Package Dependency Tag Values
          Name          Tag Value     Type     Count  Status */
#define RPMTAG_PROVIDENAME     1047  /*  STRING_ARRAY 1     Required */
#define RPMTAG_REQUIREFLAGS    1048  /*  INT32              Required */
#define RPMTAG_REQUIRENAME     1049  /*  STRING_ARRAY       Required */
#define RPMTAG_REQUIREVERSION  1050  /*  STRING_ARRAY       Required */
#define RPMTAG_CONFLICTFLAGS   1053  /*  INT32              Optional */
#define RPMTAG_CONFLICTNAME    1054  /*  STRING_ARRAY       Optional */
#define RPMTAG_CONFLICTVERSION 1055  /*  STRING_ARRAY       Optional */
#define RPMTAG_OBSOLETENAME    1090  /*  STRING_ARRAY       Optional */
#define RPMTAG_PROVIDEFLAGS    1112  /*  INT32              Required */
#define RPMTAG_PROVIDEVERSION  1113  /*  STRING_ARRAY       Required */
#define RPMTAG_OBSOLETEFLAGS   1114  /*  INT32        1     Optional */
#define RPMTAG_OBSOLETEVERSION 1115  /*  STRING_ARRAY       Optional */

/*   Table 1-13. Package Dependency Attributes
           Name            Value   Meaning */
#define RPMSENSE_LESS          0x02     
#define RPMSENSE_GREATER       0x04     
#define RPMSENSE_EQUAL         0x08     
#define RPMSENSE_PREREQ        0x40     
#define RPMSENSE_INTERP        0x100    
#define RPMSENSE_SCRIPT_PRE    0x200    
#define RPMSENSE_SCRIPT_POST   0x400    
#define RPMSENSE_SCRIPT_PREUN  0x800    
#define RPMSENSE_SCRIPT_POSTUN 0x1000   
#define RPMSENSE_RPMLIB        0x1000000

/*   Table 1-14. Other Tag Values
           Name          Tag Value     Type     Count   Status */
#define RPMTAG_BUILDTIME       1006  /*  INT32        1     Optional */
#define RPMTAG_BUILDHOST       1007  /*  STRING       1     Optional */
#define RPMTAG_FILEVERIFYFLAGS 1045  /*  INT32              Optional */
#define RPMTAG_CHANGELOGTIME   1080  /*  INT32              Optional */
#define RPMTAG_CHANGELOGNAME   1081  /*  STRING_ARRAY       Optional */
#define RPMTAG_CHANGELOGTEXT   1082  /*  STRING_ARRAY       Optional */
#define RPMTAG_OPTFLAGS        1122  /*  STRING       1     Optional */
#define RPMTAG_RHNPLATFORM     1131  /*  STRING       1     Deprecated */
#define RPMTAG_PLATFORM        1132  /*  STRING       1     Optional */

#endif
