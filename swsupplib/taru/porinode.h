/* porinode.h - make a 32-bit virtual inode using two 16-bit fields.
 
   Copyright (C) 2000  James H. Lowe, Jr.

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

#ifndef porinode_h_JUL
#define porinode_h_JUL
#include "swuser_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "hllist.h"

#define PORINODE_MAX	((0xFFFF) - 1)

typedef struct {
	int current_inodeM;
	int current_deviceM;
	int max_inodeM;
	int max_deviceM;
	HLLIST * hllistM;
} PORINODE;

PORINODE *porinode_open(void);
void	porinode_close(PORINODE *porinode);
void	porinode_add_vrecord(PORINODE *porinode, char * path, dev_t dev, ino_t ino, dev_t v_dev, ino_t v_ino);
void  	porinode_make_next_inode(PORINODE *porinode, int nlinks, dev_t dev, ino_t ino, dev_t *p_dev, ino_t *p_ino);
#endif
