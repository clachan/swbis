/* rpmbis.c - RPM file format routines
*/

/*
   Copyright (C) 2005  James H. Lowe, Jr.
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swlib.h"
#include "atomicio.h"
#include "rpmbis.h"
#include "rpmbis_i.h"
#include "swattributes.h"

struct rpm_type_size
{
	int typeM;
	int sizeM;
};

static struct rpm_type_size rpmTypeSizes[] = {
	{RPM_NULL_TYPE,         0 },
	{RPM_CHAR_TYPE,         1 },
	{RPM_INT8_TYPE,         1 },
	{RPM_INT16_TYPE,        2 },
	{RPM_INT32_TYPE,        4 },
	{RPM_INT64_TYPE,        8 },
	{RPM_STRING_TYPE,       0 },
	{RPM_BIN_TYPE,          0 },
	{RPM_STRING_ARRAY_TYPE, 0 },
	{RPM_I18NSTRING_ARRAY_TYPE, 0 }
};

static
int
get_size_by_type(int type)
{
	if (type > RPM_I18NSTRING_ARRAY_TYPE) return -1;
	return rpmTypeSizes[type].sizeM;
}

void
rpmbis_px_lead_init(RPM_PX_LEAD * px_lead)
{
	memset(px_lead, '\0', RPM_LEAD_SIZE);
	memcpy(px_lead->px_magic, RPM_MAGIC, 4);
}

static
int
rpmbis_i_get_type(int table_id, int rpmtag)
{
	int type;
	type = swatt_get_rpmtype(table_id, rpmtag);
	return type;
}

/*
 * ========================================
 * Create/Delete Routines
 * ========================================
 */

RPMHDRINDEX *
rpm_hdrindex_create(int which_header, int rpmtag)
{
	RPMHDRINDEX * hdrindex;
	hdrindex = (RPMHDRINDEX *)malloc(sizeof(RPMHDRINDEX *));
	hdrindex->tag = rpmtag;

	/*
	 * which_header is one of:
	 *	SWATMAP_ATT_Header
	 *	SWATMAP_ATT_Signature
	 */

	hdrindex->type = rpmbis_i_get_type(which_header, rpmtag);
	if (hdrindex->type < 0) {
		return NULL;
	}

	hdrindex->offset = 0;
	hdrindex->count = 0;
	return hdrindex;
}

void
rpm_hdrindex_delete(RPMHDRINDEX * hdrindex)
{
	free(hdrindex);
}

RPMBIS_RECORD *
rpmbis_record_create(int which_header, int tag)
{
	RPMBIS_RECORD * rcd;
	rcd = (RPMBIS_RECORD*)malloc(sizeof(RPMBIS_RECORD));

	rcd->tag_valueM = strob_open(10);
	rcd->indexM = rpm_hdrindex_create(which_header, tag);
	return rcd;
}

int
rpmbis_record_add_value(RPMBIS_RECORD * rcd, void * value, int table_id)
{
	int type;
	int size;

	if (table_id >= 0) {
		/*
		 * this is a test and development code branch
		 */

		type = rpmbis_i_get_type(table_id, rcd->indexM->tag);
		/*
		 * sanity check of the type
		 */
		if (type != rcd->indexM->type) {
			exit(254);
		}
	} else {
		type = rcd->indexM->type;
	}



	return 0;
}

void
rpmbis_record_delete(RPMBIS_RECORD * rcd)
{
	strob_close(rcd->tag_valueM);
	rpm_hdrindex_delete(rcd->indexM);
	free(rcd);
}

RPMLEAD * 
rpmbis_rpmlead_create(void)
{
	return (RPMLEAD *) malloc (sizeof(RPMLEAD));
}

void
rpmbis_rpmlead_delete(RPMLEAD * rpmlead)
{
	free(rpmlead);
}

RPMBIS *
rpmbis_create(void)                /* Main RPMBIS  */ 
{
	RPMBIS * rpmbis;
	rpmbis = (RPMBIS*)malloc(sizeof(RPMBIS));
	rpmbis->h_leadM = (RPMLEAD *)malloc(sizeof(RPMLEAD));
	rpmbis->n_leadM = (RPM_PX_LEAD *)malloc(sizeof(RPM_PX_LEAD));
	rpmbis->h_headerM = (RPM_HSHEADER *)malloc(sizeof(RPM_HSHEADER));
	rpmbis->n_headerM = (RPM_PX_HSHEADER *)malloc(sizeof(RPM_PX_HSHEADER));
	return rpmbis;	
}

void
rpmbis_delete(RPMBIS * rpmbis)         /* Main RPMBIS */
{
	free(rpmbis->h_leadM);
	free(rpmbis->n_leadM);
	free(rpmbis->h_headerM);
	free(rpmbis->n_headerM);
	free(rpmbis);
}

RPMBIS_RPMH *
rpmbis_rpmh_create(void)   /* Header */
{
	RPMBIS_RPMH * rpmh;
	rpmh = (RPMBIS_RPMH*)malloc(sizeof(RPMBIS_RPMH));
	rpmh->hshM = rpmbis_hsh_create();
	rpmh->host_recordsM = cplob_open(12);
	rpmh->hdrindex_fdM = swlib_open_memfd();
	rpmh->store_fdM = swlib_open_memfd();
	return rpmh;
}

void
rpmbis_rpmh_delete(RPMBIS_RPMH * rpmh)  /* Header delete */
{
	uxfio_close(rpmh->store_fdM);
	uxfio_close(rpmh->hdrindex_fdM);
	free(rpmh);
}

/*
 * Create header structure header
 */
RPM_HSHEADER *
rpm_hsh_create(void)  /* Header structure header */
{
	RPM_HSHEADER * hsh;
	hsh = (RPM_HSHEADER*)malloc(sizeof(RPM_HSHEADER));
	memcpy(&(hsh->magic), RPM_HEADER_MAGIC, 4);
	memset(hsh->reserved, '\0', 4);
	hsh->nindex = 0;
	hsh->hsize = 0;
	return hsh;
}

void
rpm_hsh_delete(RPM_HSHEADER * hsh)  /* Header structure header */
{
	free(hsh);
}


/*
 * ========================================
 * RPM data structure conversion routines
 * ========================================
 */

int
rpmbis_ntoh_lead(RPMBIS * rpmbis, RPMLEAD * dst, RPM_PX_LEAD * src)
{
	memcpy(dst, src, RPM_LEAD_SIZE);
	return 0;
}

int
rpmbis_ntoh_header(RPMBIS * rpmbis, RPM_HSHEADER * dst, RPM_PX_HSHEADER * src)
{
	memcpy(&(dst->magic), &(src->px_magic), sizeof(src->px_magic));
	memcpy(&(dst->reserved), &(src->px_reserved), sizeof(src->px_reserved));
	dst->nindex = (int)ntohl(*((uint32_t*)(src->px_nindex)));
	dst->hsize  = (int)ntohl(*((uint32_t*)(src->px_hsize)));
	return 0;
}

int
rpmbis_hton_header(RPMBIS * rpmbis, RPM_PX_HSHEADER * dst, RPM_HSHEADER * src)
{
	uint32_t tmp;

	memcpy(&(dst->px_magic), &(src->magic), sizeof(src->magic));
	memcpy(&(dst->px_reserved), &(src->magic), sizeof(src->reserved));

	tmp = htonl((uint32_t)src->nindex);
	memcpy(&(dst->px_nindex), &(tmp), sizeof(uint32_t));
	
	tmp = htonl((uint32_t)src->hsize);
	memcpy(&(dst->px_hsize), &(tmp), sizeof(uint32_t));

	return 0;
}

int
rpmbis_hton_hdrindex(RPMBIS * rpmbis, RPM_PX_HDRINDEX * dst, RPMHDRINDEX * src)
{
	uint32_t tmp;

	tmp = htonl((uint32_t)src->tag);	
	memcpy(&(dst->px_tag), &(tmp), sizeof(uint32_t));

	tmp = htonl((uint32_t)src->type);	
	memcpy(&(dst->px_type), &(tmp), sizeof(uint32_t));
	
	tmp = htonl((uint32_t)src->offset);	
	memcpy(&(dst->px_offset), &(tmp), sizeof(uint32_t));
	
	tmp = htonl((uint32_t)src->count);	
	memcpy(&(dst->px_count), &(tmp), sizeof(uint32_t));

	return 0;
}

int
rpmbis_ntoh_hdrindex(RPMBIS * rpmbis, RPMHDRINDEX * dst, RPM_PX_HDRINDEX * src)
{
	dst->tag = (int)ntohl(*((uint32_t*)(src->px_tag)));
	dst->type = (int)ntohl(*((uint32_t*)(src->px_type)));
	dst->offset = (int)ntohl(*((uint32_t*)(src->px_offset)));
	dst->count = (int)ntohl(*((uint32_t*)(src->px_count)));
	return 0;
}

/*
 * ========================================
 * RPM data structure I/O routines
 * ========================================
 */

int
rpmbis_io_write_lead(RPMBIS * rpmbis, int ofd)
{
	int ret;
	rpmbis_px_lead_init(rpmbis->n_leadM);
	ret = atomicio((ssize_t (*)(int, void *, size_t))(write), ofd, (void*)(rpmbis->n_leadM), RPM_LEAD_SIZE);
	if (ret != RPM_LEAD_SIZE) return -1;
	return ret;
}

int
rpmbis_io_read_lead(RPMBIS * rpmbis, int ifd)
{
	int ret;
	ret = uxfio_unix_safe_atomic_read(ifd,  rpmbis->n_leadM, RPM_LEAD_SIZE);
	if (ret != RPM_LEAD_SIZE) return -1;
	rpmbis_ntoh_lead(rpmbis, rpmbis->h_leadM, rpmbis->n_leadM);
	return ret;
}

int
rpmbis_io_read_header(RPMBIS * rpmbis, int ifd)
{
	int ret;
	ret = uxfio_unix_safe_atomic_read(ifd,  (void*)(rpmbis->n_headerM), RPM_HEADER_SIZE);
	if (ret != RPM_HEADER_SIZE) return -1;
	rpmbis_ntoh_header(rpmbis, rpmbis->h_headerM, rpmbis->n_headerM);
	return ret;
}

int
rpmbis_io_write_header(RPMBIS * rpmbis, int ofd)
{
	int ret;	
	ret = rpmbis_hton_header(rpmbis, rpmbis->n_headerM, rpmbis->h_headerM);
	if (ret)
		return -1;

	memcpy(&(rpmbis->n_headerM->px_magic), RPM_HEADER_MAGIC, sizeof(rpmbis->n_headerM->px_magic));
	memset(&(rpmbis->n_headerM->px_reserved), '\0', sizeof(rpmbis->n_headerM->px_reserved));

	ret = atomicio((ssize_t (*)(int, void *, size_t))(write), ofd, (void*)(rpmbis->n_headerM), RPM_HEADER_SIZE);
	if (ret != RPM_HEADER_SIZE)
		return -1;	
	return RPM_HEADER_SIZE;
}

int
rpmbis_io_read_hdrindexes(RPMBIS * rpmbis, int ofd)
{
	int nhdrs;
	RPMHDRINDEX hdrindex;
	RPM_PX_HDRINDEX * px_hdrindex;
	nhdrs = rpmbis->h_headerM->nindex;
	return 0;
}

int
rpmbis_io_write_hdrindexes(RPMBIS * rpmbis, int ofd)
{
	/*
	rpmbis_hton_hdrindex(rpmbis, RPM_PX_HDRINDEX * dst, RPMHDRINDEX * src)
	*/	
	
	return 0;
}

int
rpmbis_io_write_blob(RPMBIS * rpmbis, int ofd)
{
	int ret;
	int retval;

	retval = 0;
	ret = rpmbis_io_write_lead(rpmbis, ofd);
	if (ret <= 0) {
		return -1;
	}
	retval += ret;

	ret = rpmbis_io_write_header(rpmbis, ofd);
	if (ret <= 0) {
		return -1;
	}
	retval += ret;

	ret = rpmbis_io_write_hdrindexes(rpmbis, ofd);
	if (ret <= 0) {
		return -1;
	}
	retval += ret;

	return retval;
}

