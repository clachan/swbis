/* defer.c - handle "defered" links in newc and crc archives
   Copyright (C) 1999 Jim Lowe


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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "system.h"
#include "cpiohdr.h"
#include "ahs.h"
#include "taru.h"
#include "defer.h"

static struct deferment *def_create_deferment (struct new_cpio_header *file_hdr);
static void              def_free_deferment (struct deferment *d);

static
int
def_writeout_defered_file (DEFER * def, struct new_cpio_header *header, int infd, int outfd, int format)
{
	int ret = taru_write_out_header((TARU*)(def->taruM), header, outfd, format, (char *)(NULL)/*header_buffer*/, 0);
	ret += taru_write_archive_member_data ((TARU*)(def->taruM), header, outfd, infd, (int(*)(int))NULL, format, -1, NULL);
	return ret;
}

static struct deferment *
def_create_deferment(struct new_cpio_header *file_hdr)
{
	struct deferment *d;
	d = (struct deferment *) malloc(sizeof(struct deferment));
	d->headerP = ahsStaticCreateFilehdr();
	taru_filehdr2filehdr((d->headerP), file_hdr);	
  	ahsStaticSetPaxLinkname((d->headerP), NULL);
	return d;
}

static void
def_free_deferment(struct deferment *d)
{
  	ahsStaticSetPaxLinkname((d->headerP), NULL);
  	ahsStaticSetTarFilename((d->headerP), NULL);
	ahsStaticDeleteFilehdr(d->headerP);
	swbis_free(d);
}

static
int
def_count_defered_links_to_dev_ino(DEFER * def, struct new_cpio_header *file_hdr)
{
        struct deferment *d;
        int ino;
        int maj;
        int min;
        int count;
        ino = file_hdr->c_ino;
        maj = file_hdr->c_dev_maj;
        min = file_hdr->c_dev_min;
        count = 0;
        for (d = def->deferoutsM; d != NULL; d = d->nextP) {
                if (((int)(d->headerP->c_ino) == ino) && ((int)(d->headerP->c_dev_maj) == maj)
                    && ((int)(d->headerP->c_dev_min) == min))
                        ++count;
        }
        return count;
}

static
int
def_writeout_defers(DEFER *def, struct new_cpio_header *file_hdr, int out_des, int infd, int is_final)
{
	int ret;
	int retval = 0;
        struct deferment *d;
        struct deferment *d_prev;
	struct deferment *d_free;
	int remaining_links;
        int ino;
        int maj;
        int min;
        ino = file_hdr->c_ino;
        maj = file_hdr->c_dev_maj;
        min = file_hdr->c_dev_min;
        d_prev = NULL;
        d = def->deferoutsM;
        while (d != NULL) {
                if (((int)(d->headerP->c_ino) == ino) && ((int)(d->headerP->c_dev_maj) == maj)
                    && ((int)(d->headerP->c_dev_min) == min)) {
               		remaining_links = def_count_defered_links_to_dev_ino(def, d->headerP);
			if (is_final == 0 || (is_final == 1 && remaining_links > 1)) {
				d->headerP->c_filesize = 0;
				ret = taru_write_out_header((TARU*)(def->taruM), (d->headerP), out_des, def->formatM, (char*)(NULL), 0); 
				retval += ret;
			} 
			else if (is_final == 1 && remaining_links == 1) {
				/* Last link for a file that does not have all its links in the archive.
				 * We must write out the last file here
				 */
                        	ret = def_writeout_defered_file(def, (d->headerP), infd, out_des, def->formatM);
				retval += ret;
			}	
			if (d_prev != NULL)
				d_prev->nextP = d->nextP;
			else
				def->deferoutsM = d->nextP;
                        d_free = d;
			d = d->nextP;
			def_free_deferment(d_free);
               } else {
                        d_prev = d;
                        d = d->nextP;
                }
        }
        return retval;
}


/* ----------- Public Routines ------------------------ */

void defer_set_taru(DEFER * defer, void * taru) { defer->taruM = (void*)(taru); }

DEFER * defer_open(int format)
{
	DEFER * def=(DEFER*)malloc(sizeof(DEFER));
	def->deferoutsM=NULL;
	def->formatM=format;
	def->taruM = taru_create();
	return def;
}

void
defer_close(DEFER * def)
{
        struct deferment *oldd;
        struct deferment *d=def->deferoutsM;
	while (d) {
		oldd=d;
		d=d->nextP;
		def_free_deferment(oldd);
	}
	swbis_free(def);
	return;
}

void
defer_set_format(DEFER * def, int format)
{
	def->formatM=format;
}

int
defer_is_last_link(DEFER * def, struct new_cpio_header *file_hdr)
{
        int other_files_sofar;
        
	other_files_sofar = def_count_defered_links_to_dev_ino(def, file_hdr);
        if ((int)(file_hdr->c_nlink) == (other_files_sofar + 1)) {
                return 1;
        }
        return 0;
}

void
defer_add_link_defer(DEFER *def, struct new_cpio_header *file_hdr)
{
        struct deferment *lasted;
        struct deferment *ed;
        struct deferment *d;
        
	d = def_create_deferment(file_hdr);
        ed = def->deferoutsM;
	lasted = NULL;
	while (ed) {
		lasted = ed;
		ed = ed ->nextP;
	}
	if (lasted) {
        	lasted->nextP = d;
	} else {
        	def->deferoutsM = d;
	}
        d->nextP = NULL;
}

int
defer_writeout_zero_length_defers(DEFER *def, struct new_cpio_header *file_hdr, int out_des)
{
	return def_writeout_defers(def, file_hdr, out_des, -1, 0 /* not final */);
}

int
defer_writeout_final_defers(DEFER *def, int out_des)
{
	int ret = 0;
	int retval = 0;
        while (def->deferoutsM != NULL && ret >= 0){
		ret = def_writeout_defers(def, (def->deferoutsM->headerP), out_des, -1, 1 /* is_final */);
		retval += ret;
	}
	if (ret < 0) return ret;
	return retval;
}
