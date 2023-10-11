/* hllist.c  -  hard link list object.
   
   Copyright (C) 1999  Jim Lowe 

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

#include "swuser_config.h"
#include "hllist.h"
#include "cplob.h"
#include "swutilname.h"
#include "debug_config.h"

#ifdef HLLISTNEEDDEBUG
#define HLLIST_E_DEBUG(format) SWBISERROR("HLLIST DEBUG: ", format)
#define HLLIST_E_DEBUG2(format, arg) SWBISERROR2("HLLIST DEBUG: ", format, arg)
#define HLLIST_E_DEBUG3(format, arg, arg1) SWBISERROR3("HLLIST DEBUG: ", format, arg, arg1)
#else
#define HLLIST_E_DEBUG(arg)
#define HLLIST_E_DEBUG2(arg, arg1)
#define HLLIST_E_DEBUG3(arg, arg1, arg2)
#endif /* HLLISTNEEDDEBUG */

static void
free_entry (hllist_entry * en) {
	if (en->path_) swbis_free(en->path_);
}

HLLIST *
hllist_open ()
{
	HLLIST * hllist=(HLLIST*)malloc(sizeof(HLLIST));
	HLLIST_E_DEBUG("Entering");
	if (!hllist) return NULL;
	hllist->list_ = cplob_open(8);
        cplob_additem(hllist->list_, 0, NULL);
	HLLIST_E_DEBUG2("Leaving %p", (void*)hllist);
	hllist->disable_find_ = 0;
	hllist->disable_add_ = 0;
	return hllist;
}

void
hllist_close(HLLIST * hllist)
{
	int i=0;
	hllist_entry * en;
	HLLIST_E_DEBUG("Entering");
	while ((en=(hllist_entry*)cplob_val(hllist->list_,i++))) {
		free_entry (en);
	}
	cplob_close(hllist->list_);
	free(hllist);
}

void 
hllist_add_record(HLLIST * hllist, char * path, dev_t dev, ino_t ino)
{
	HLLIST_E_DEBUG("Entering");
	if (hllist->disable_add_) return;
	hllist_add_vrecord(hllist, path, dev, ino, 0, 0);
	HLLIST_E_DEBUG("Leaving");
}

void 
hllist_add_vrecord(HLLIST * hllist, char * path, dev_t dev, ino_t ino, dev_t v_dev, ino_t v_ino)
{
        CPLOB * lob=hllist->list_;
	hllist_entry * en=(hllist_entry*)malloc(sizeof(hllist_entry));
	HLLIST_E_DEBUG2("Entering %p", hllist);
	HLLIST_E_DEBUG2("Adding dev=%d", (int)dev);
	HLLIST_E_DEBUG2("Adding ino=%d", (int)ino);
	HLLIST_E_DEBUG2("Adding path=[%s]", path);
	if (hllist->disable_add_) return;
	en->path_=strdup(path);
	en->dev_=dev;
	en->ino_=ino;
	en->v_dev_ = v_dev;
	en->v_ino_ = v_ino;
        cplob_additem(lob, cplob_get_nused(lob) - 1, (char*)en);
        cplob_additem(lob, cplob_get_nused(lob), NULL);
	HLLIST_E_DEBUG("Leaving");
}

hllist_entry * 
hllist_find_file_entry(HLLIST * hllist, dev_t dev, ino_t ino, int occurance, int * nfound)
{
	int i=0;
	hllist_entry * en;
	
	HLLIST_E_DEBUG2("Entering %p", hllist);
	if (hllist->disable_find_) {
		HLLIST_E_DEBUG("Leaving because disabled, returning NULL");
		return NULL;
	}
	HLLIST_E_DEBUG2("Looking for dev=%d", (int)dev);
	HLLIST_E_DEBUG2("Looking for ino=%d", (int)ino);
	HLLIST_E_DEBUG2("Looking for occurance=%d", (int)occurance);
	while ((en=(hllist_entry*)cplob_val(hllist->list_,i++))) {
		HLLIST_E_DEBUG2("Entry BEGIN Index=%d", i-1);
		HLLIST_E_DEBUG3("Entry: dev=%d ino=%d", (int)dev, (int)ino);
		HLLIST_E_DEBUG2("Entry: path=%s", en->path_);
		HLLIST_E_DEBUG2("Entry END Index=%d", i-1);
		if (en->ino_ == ino && en->dev_ == dev) {
			(*nfound) ++;
			if (occurance > 0 && (*nfound) == occurance) {
				HLLIST_E_DEBUG3("Found dev=%d ino=%d", (int)dev, (int)ino);
				return en;
			}
		}
	}	
	HLLIST_E_DEBUG("Leaving having not found any, returning NULL");
	return NULL;
}

void
hllist_show_to_file(HLLIST * hllist, FILE * fp)
{
	int i=0;
	hllist_entry * en;
	fprintf(fp, "%s: addr=%p: hllist_show_to_file BEGIN\n", swlib_utilname_get(), (void*)hllist);
	while ((en=(hllist_entry*)cplob_val(hllist->list_,i++))) {
		fprintf(fp, "%s: addr=%p: Entry %d: BEGIN\n", swlib_utilname_get(), (void*)hllist, i-1);
		fprintf(fp, "%s: addr=%p: Entry %d: dev=%d ino=%d\n", swlib_utilname_get(), (void*)hllist, i-1, (int)(en->dev_), (int)(en->ino_));
		fprintf(fp, "%s: addr=%p: Entry %d: v_dev=%d v_ino=%d\n", swlib_utilname_get(), (void*)hllist, i-1, (int)(en->v_dev_), (int)(en->v_ino_));
		fprintf(fp, "%s: addr=%p: Entry %d: path=[%s]\n", swlib_utilname_get(), (void*)hllist, i-1, en->path_);
		fprintf(fp, "%s: addr=%p: Entry %d: END\n", swlib_utilname_get(), (void*)hllist, i-1);
	}	
	fprintf(fp, "%s: addr=%p: hllist_show_to_file END\n", swlib_utilname_get(), (void*)hllist);
}

void
hllist_disable_add(HLLIST * hllist)
{
	hllist->disable_add_ = 1;
}

void
hllist_disable_find(HLLIST * hllist)
{
	hllist->disable_find_ = 1;
}

void
hllist_clear_entries_and_disable(HLLIST * hllist)
{
	int i=0;
	hllist_entry * en;
	HLLIST_E_DEBUG("Entering");
	while ((en=(hllist_entry*)cplob_val(hllist->list_,i++))) {
		free_entry (en);
	}
	cplob_close(hllist->list_);
	hllist->list_=cplob_open(8);
        cplob_additem(hllist->list_, 0, NULL);
	hllist->disable_find_ = 1;
}
