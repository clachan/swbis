/* porinode.c - make a 32-bit virtual inode using two 16-bit fields.
 
   Copyright (C) 2000  Jim Lowe

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
#include "porinode.h"

#define swbis_free free

static
void
porinode_i_get_next_vinode(PORINODE * porinode, dev_t *p_dev, ino_t *p_ino)
{

	if (porinode->current_inodeM >= porinode->max_inodeM - 1) {
		if (porinode->current_deviceM >= porinode->max_deviceM) {
			fprintf(stderr,
				"porinode: virtual inode space overflow.\n");
		} else {
			++(porinode->current_deviceM);
			porinode->current_inodeM = 0;
		}
	}
	*p_ino = ++(porinode->current_inodeM);
	*p_dev = porinode->current_deviceM;
}


PORINODE * porinode_open(void)
{
	PORINODE * porinode=(PORINODE*)malloc(sizeof(PORINODE));
	porinode->hllistM = hllist_open();
	porinode->current_inodeM=1;
	porinode->current_deviceM=1;
	porinode->max_inodeM = PORINODE_MAX;
	porinode->max_deviceM = PORINODE_MAX;
	return porinode;
}

void
porinode_close(PORINODE * porinode)
{
	hllist_close(porinode->hllistM);
	swbis_free(porinode);
	return;
}

void
porinode_add_vrecord(PORINODE *porinode, char * path, dev_t dev, 
					ino_t ino, dev_t v_dev, ino_t v_ino)
{
	hllist_add_vrecord(porinode->hllistM, path, dev, ino, v_dev, v_ino);
}


void
porinode_make_next_inode(PORINODE *porinode, int nlinks, dev_t dev, 
					ino_t ino, dev_t *p_dev, ino_t *p_ino)
{
	hllist_entry * link_record_buf=NULL;
	int nfound = 0;

	if (nlinks < 2) {
		porinode_i_get_next_vinode(porinode, p_dev, p_ino);
	} else {
		link_record_buf = hllist_find_file_entry(porinode->hllistM,
							dev, ino, 1, &nfound);
		if (nfound == 0) {
			porinode_i_get_next_vinode(porinode, p_dev, p_ino);
			porinode_add_vrecord(porinode, "", dev, ino,
							*p_dev, *p_ino);
		} else {
			*p_dev = link_record_buf->v_dev_;
			*p_ino = link_record_buf->v_ino_;
		}
	}
}
