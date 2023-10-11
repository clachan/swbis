/* rpmpsf_rpmi.c - Rpm Version Specific Internals
*/
#ifndef RPMPSF_RPMI255_H
#define RPMPSF_RPMI255_H

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
/* These functions and structs are taken directly from rpm-2.5.5. */
static struct indexEntry *findEntry(Header h, int_32 tag, int_32 type);
static int indexCmp(const void *ap, const void *bp);

#define struct_MI_headerTagTableEntry MIheaderTagTableEntry
typedef struct headerTagTableEntry * MIheaderTagTableEntry;

struct headerToken {
	struct indexEntry *index;
	int indexUsed;
	int indexAlloced;

	int sorted;
	int langNum;
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
};

static struct indexEntry *findEntry(Header h, int_32 tag, int_32 type)
{
    struct indexEntry * entry, * entry2, * last;
    struct indexEntry key;

    if (!h->sorted) headerSort(h);

    key.info.tag = tag;

    entry2 = entry =
        bsearch(&key, h->index, h->indexUsed, sizeof(struct indexEntry),
                indexCmp);
    if (!entry) return NULL;

    if (type == RPM_NULL_TYPE) return entry;

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

static int indexCmp(const void *ap, const void *bp)
{
    int_32 a, b;

    a = ((struct indexEntry *)ap)->info.tag;
    b = ((struct indexEntry *)bp)->info.tag;

    if (a > b) {
        return 1;
    } else if (a < b) {
        return -1;
    } else {
        return 0;
    }
}
/* end of rpm internals */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
#endif
