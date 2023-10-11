/* rpmbis_i.h -- implementation of the LSB RPM file format */
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

#ifndef rpmbis_i_h_20050611
#define rpmbis_i_h_20050611

#include "rpmbis.h"

#include "uxfio.h"
#include "cplob.h"
#include "strob.h"
#include "xformat.h"

/* Lead Section, LSB 1.1.1
 */
typedef struct rpmlead {
	unsigned char magic[4]; /* always "\355\253\356\333" */
	unsigned char major, minor;
	short type;
	short archnum;
	char name[66];
	short osnum;
	short signature_type;
	char reserved[16];	
}; /* total length implementation dependent */

/* Header Record, LSB 1.1.2.1
 */
struct rpmheader {
	unsigned char magic[4];
	unsigned char reserved[4];
	int nindex;
	int hsize;
}; /* total length is implementation dependent */

/* Index Record, LSB 1.1.2.2
 */
struct rpmhdrindex {
	int tag;
	int type;
	int offset;
	int count;
}; /* total length is implementation dependent */

/*
 * These are typedefs of LSB data structures
 */
typedef struct rpmlead RPMLEAD;          /* Lead */
typedef struct rpmheader RPM_HSHEADER;   /* Header structure header */
typedef struct rpmhdrindex RPMHDRINDEX;  /* header indexes */

/*
 * +++++++++++++++++++++++++++++++++++++++++
 *  RPMBIS implementation data structures
 * +++++++++++++++++++++++++++++++++++++++++
 */

typedef struct {	/* An implentation Tag object */
	RPMHDRINDEX * indexM;   /* hdrindex object for one tag */
	STROB * tag_valueM;     /* network ordered data for this tag */
} RPMBIS_RECORD;

typedef struct {   	/* Header Proper */
	RPM_HSHEADER * hshM;   /* header structure header */	
	CPLOB * host_recordsM; /* array of RPMBIS_RECORD objects */
	int hdrindex_fdM;      /* mem file containing network index records */
	int store_fdM;         /* mem file containing the network data */
} RPMBIS_RPMH;

typedef struct {	/* Package */
	RPMLEAD * leadM;           /* Lead */
	RPMBIS_RPMH * signatureM;  /* Signature */
	RPMBIS_RPMH * headerM;     /* Header */
	XFORMAT * payloadM;        /* Paylaod */
} RPMBIS_PACKAGE;

typedef struct {	/* Main implementation control object */
	RPMLEAD * h_leadM;            		/* host ordered lead */
	RPM_PX_LEAD * n_leadM;	    		/* network ordered lead */
	RPM_HSHEADER * h_headerM;       	/* host header */
	RPM_PX_HSHEADER * n_headerM;    	/* network header */
	RPMBIS_PACKAGE * packageM;
} RPMBIS;

/*
 * +++++++++++++++++++++++++++++++++++++++++
 *  Public Routines 
 * +++++++++++++++++++++++++++++++++++++++++
 */

RPMLEAD * rpmbis_rpmlead_create(void);

RPMBIS *  rpmbis_create(void);
void      rpmbis_delete(RPMBIS * rpmbis);

void      rpmbis_pkg_load(RPMBIS * rpmbis, int ifd);
void      rpmbis_pkg_unload(RPMBIS * rpmbis, int ofd);

int       rpmbis_hton_header(RPMBIS * rpmbis, RPM_PX_HSHEADER * dst, RPM_HSHEADER * src);
int       rpmbis_ntoh_header(RPMBIS * rpmbis, RPM_HSHEADER * dst, RPM_PX_HSHEADER * src);

int       rpmbis_hton_hdrindex(RPMBIS * rpmbis, RPM_PX_HDRINDEX * dst, RPMHDRINDEX * src);
int       rpmbis_ntoh_hdrindex(RPMBIS * rpmbis, RPMHDRINDEX * dst, RPM_PX_HDRINDEX * src);

RPMLEAD *      rpmbis_rpmlead_create(void);
RPMBIS *       rpmbis_create(void);
void           rpmbis_delete(RPMBIS * rpmbis);
RPM_HSHEADER * rpmbis_hsh_create(void);
void           rpmbis_hsh_delete(RPM_HSHEADER * hsh);

RPMBIS_RPMH *  rpmbis_rpmh_create(void);
void           rpmbis_rpmh_delete(RPMBIS_RPMH * rpmh);

void           rpmbis_px_lead_init(RPM_PX_LEAD * px_lead);

int rpmbis_ntoh_lead(RPMBIS * rpmbis, RPMLEAD * dst, RPM_PX_LEAD * src);
int rpmbis_ntoh_header(RPMBIS * rpmbis, RPM_HSHEADER * dst, RPM_PX_HSHEADER * src);
int rpmbis_hton_header(RPMBIS * rpmbis, RPM_PX_HSHEADER * dst, RPM_HSHEADER * src);
int rpmbis_hton_hdrindex(RPMBIS * rpmbis, RPM_PX_HDRINDEX * dst, RPMHDRINDEX * src);
int rpmbis_ntoh_hdrindex(RPMBIS * rpmbis, RPMHDRINDEX * dst, RPM_PX_HDRINDEX * src);

int rpmbis_io_write_lead(RPMBIS * rpmbis, int ofd);
int rpmbis_io_read_lead(RPMBIS * rpmbis, int ifd);
int rpmbis_io_read_header(RPMBIS * rpmbis, int ifd);
int rpmbis_io_write_header(RPMBIS * rpmbis, int ofd);
int rpmbis_io_read_hdrindexes(RPMBIS * rpmbis, int ofd);
int rpmbis_io_write_hdrindexes(RPMBIS * rpmbis, int ofd);
int rpmbis_io_write_blob(RPMBIS * rpmbis, int ofd);


#endif
