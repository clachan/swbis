/* taru_util.h - Several utility routines for cpio.
   Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.

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

/*
Modified by jhl@richmond.infi.net to integrate with other software.
This file is no longer functional with GNU cpio.  10APR98
*/


#ifndef taru_util_h
#define taru_util_h

#include "swuser_config.h"

#include <stdio.h>
#include <sys/types.h>

#define FALSE 0
#define TRUE 1

/*
#ifdef HPUX_CDF
#include <sys/stat.h>
#endif
*/


#include "system.h"
#include "cpiohdr.h"
#include <sys/ioctl.h>


struct ihash_inode_val
{
  unsigned long inode;
  unsigned long major_num;
  unsigned long minor_num;
  char *file_name;
};

char * ihash_find_inode_file (unsigned long node_num,unsigned long major_num, unsigned long minor_num);
void ihash_add_inode (unsigned long node_num,char *  file_name, unsigned long major_num, unsigned long minor_num);
void ihash_insert (struct ihash_inode_val * new_value);


#endif

