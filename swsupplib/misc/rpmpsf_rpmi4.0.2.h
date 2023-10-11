/* rpmpsf_rpmi4.0.2.c - Rpm Version Specific Internals
*/
#ifndef RPMPSF_RPMI402_H
#define RPMPSF_RPMI402_H

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

/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* These functions and structs are taken directly from rpm-4.0.2 */
static struct indexEntry *findEntry(Header h, int_32 tag, int_32 type);
static int indexCmp(const void *ap, const void *bp);

#define struct_MI_headerTagTableEntry MIheaderTagTableEntry
typedef struct headerTagTableEntry * MIheaderTagTableEntry;


struct headerToken {
	struct indexEntry *index;
	int indexUsed;
	int indexAlloced;
	int region_allocated;
	int sorted;
	int legacy;
	int nrefs;
};

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


/**
 * Find matching (tag,type) entry in header.
 * @param h		header
 * @param tag		entry tag
 * @param type		entry type
 * @return 		header entry
 */
static struct indexEntry *findEntry(Header h, int_32 tag, int_32 type)
{
    struct indexEntry * entry, * entry2, * last;
    struct indexEntry key;

    if (!h->sorted) headerSort(h);

    key.info.tag = tag;

    entry2 = entry = 
	bsearch(&key, h->index, h->indexUsed, sizeof(*h->index), indexCmp);
    if (entry == NULL)
	return NULL;

    if (type == RPM_NULL_TYPE)
	return entry;

    /* look backwards */
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

static int indexCmp(const void *avp, const void *bvp)   /*@*/
{
    const struct indexEntry * ap = avp, * bp = bvp;
        return (ap->info.tag - bp->info.tag);
}

/* end of rpm internals */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
#endif
