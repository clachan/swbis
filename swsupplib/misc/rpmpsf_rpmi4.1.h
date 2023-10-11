/* rpmpsf_rpmi4.1.c - Rpm Version Specific Internals
*/
#ifndef RPMPSF_RPMI41_H
#define RPMPSF_RPMI41_H

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "um_rpmlib.h"
#include "um_rpmts.h"

/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* These functions and structs are taken directly from rpm-4.0.3 
   and modified alittle bit.
*/

static
int rpmReadPackageHeader(FD_t fd, Header * hdr,  int * isSource, int * major,  int * minor);

static struct indexEntry *findEntry(Header h, int_32 tag, int_32 type);
static int indexCmp(const void *ap, const void *bp);


#define struct_MI_headerTagTableEntry headerTagTableEntry

struct entryInfo {
	int_32 tag;
	int_32 type;
	int_32 offset;		/* Offset from beginning of data segment,
				   only defined on disk */
	int_32 count;
};

struct indexEntry {
	struct entryInfo info;
	void *data;
	int length;		/* Computable, but why bother? */
	int rdlen;
};

struct headerToken {
    struct HV_s hv;             /*!< Header public methods. */
    void * blob;                /*!< Header region blob. */
    struct indexEntry * index;  /*!< Array of tags. */
    int indexUsed;              /*!< Current size of tag array. */
    int indexAlloced;           /*!< Allocated size of tag array. */
    int flags;
#define HEADERFLAG_SORTED       (1 << 0) /*!< Are header entries sorted? */
#define HEADERFLAG_ALLOCATED    (1 << 1) /*!< Is 1st header region allocated? */
#define HEADERFLAG_LEGACY       (1 << 2) /*!< Header came from legacy source? */
    int nrefs;                           /*!< Reference count. */
};

/**
 * Find matching (tag,type) entry in header.
 * @param h		header
 * @param tag		entry tag
 * @param type		entry type
 * @return 		header entry
 */

static struct indexEntry  * findEntry(Header h, int_32 tag, int_32 type)
{
    struct indexEntry * entry;
    struct indexEntry * entry2;
    struct indexEntry * last;
    struct indexEntry key;

    if (h == NULL) return NULL;
    if (!(h->flags & HEADERFLAG_SORTED)) headerSort(h);

    key.info.tag = tag;

    entry2 = entry = 
	bsearch(&key, h->index, h->indexUsed, sizeof(*h->index), indexCmp);
    if (entry == NULL)
	return NULL;

    if (type == RPM_NULL_TYPE)
	return entry;

    while (entry->info.tag == tag && entry->info.type != type &&
	   entry > h->index) entry--;

    if (entry->info.tag == tag && entry->info.type == type)
	return entry;

    last = h->index + h->indexUsed;
    while (entry2->info.tag == tag && entry2->info.type != type &&
	   entry2 < last) entry2++;

    if (entry->info.tag == tag && entry->info.type == type)
	return entry;

    return NULL;
}

static 
int indexCmp(const void * avp, const void * bvp)
{
    struct indexEntry *ap = (struct indexEntry*) avp; 
    struct indexEntry *bp = (struct indexEntry*) bvp;
    return (ap->info.tag - bp->info.tag);
}

static
int 
rpmReadPackageHeader(FD_t fd, Header * hdr,  int * isSource, int * major,  int * minor)
{

	int rc;
	rpmts ts = rpmtsCreate();
	rpmVSFlags vsflags = 0;
	
	vsflags |= _RPMVSF_NODIGESTS;
	vsflags |= _RPMVSF_NOSIGNATURES;
	vsflags |= RPMVSF_NOHDRCHK;
	(void) rpmtsSetVSFlags(ts, vsflags);

	/*
	rc = rpmReadHeader(ts, fd, hdr, &msg);
	*/
	
	rc =  rpmReadPackageFile(ts, fd, "rpmReadPackageHeader", hdr);
	ts = rpmtsFree(ts);
	return rc;
}

/* end of rpm internals */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
#endif
