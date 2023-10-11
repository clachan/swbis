/* Based on util.c - Several utility routines for cpio.
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
*/

/*
Modified by jhl@richmond.infi.net to integrate with other software.
This file is no longer functional with GNU cpio.  10APR98
*/


#include "swuser_config.h"
#include <stdio.h>
#include <sys/types.h>
#ifdef HPUX_CDF
#include <sys/stat.h>
#endif
#include "system.h"
#include "cpiohdr.h"
#include "taru_util.h"

#ifndef __MSDOS__
#include <sys/ioctl.h>
#else
#include <io.h>
#endif


static void hash_insert ();

/* Support for remembering inodes with multiple links.  Used in the
   "copy in" and "copy pass" modes for making links instead of copying
   the file.  */

/* Inode hash table.  Allocated by first call to add_inode.  */
static struct ihash_inode_val **hash_table = NULL;

/* Size of current hash table.  Initial size is 47.  (47 = 2*22 + 3) */
static int hash_size = 22;

/* Number of elements in current hash table.  */
static int hash_num;

/* Find the file name associated with NODE_NUM.  If there is no file
   associated with NODE_NUM, return NULL.  */

char *
ihash_find_inode_file (unsigned long node_num, unsigned long major_num, unsigned long minor_num minor_num)
{
  int start;			/* Initial hash location.  */
  int temp;			/* Rehash search variable.  */

  if (hash_table != NULL)
    {
      /* Hash function is node number modulo the table size.  */
      start = node_num % hash_size;

      /* Initial look into the table.  */
      if (hash_table[start] == NULL)
	return NULL;
      if (hash_table[start]->inode == node_num
	  && hash_table[start]->major_num == major_num
	  && hash_table[start]->minor_num == minor_num)
	return hash_table[start]->file_name;

      /* The home position is full with a different inode record.
	 Do a linear search terminated by a NULL pointer.  */
      for (temp = (start + 1) % hash_size;
	   hash_table[temp] != NULL && temp != start;
	   temp = (temp + 1) % hash_size)
	{
	  if (hash_table[temp]->inode == node_num
	      && hash_table[start]->major_num == major_num
	      && hash_table[start]->minor_num == minor_num)
	    return hash_table[temp]->file_name;
	}
    }
  return NULL;
}

/* Associate FILE_NAME with the inode NODE_NUM.  (Insert into hash table.)  */

void
ihash_add_inode (unsigned long node_num, char * file_name,unsigned long  major_num, unsigned long minor_num)
{

  struct ihash_inode_val *temp;

  /* Create new inode record.  */
  temp = (struct ihash_inode_val *) malloc(sizeof (struct ihash_inode_val));
  temp->inode = node_num;
  temp->major_num = major_num;
  temp->minor_num = minor_num;
  temp->file_name = xstrdup (file_name);

  /* Do we have to increase the size of (or initially allocate)
     the hash table?  */
  if (hash_num == hash_size || hash_table == NULL)
    {
      struct ihash_inode_val **old_table;	/* Pointer to old table.  */
      int i;			/* Index for re-insert loop.  */

      /* Save old table.  */
      old_table = hash_table;
      if (old_table == NULL)
	hash_num = 0;

      /* Calculate new size of table and allocate it.
         Sequence of table sizes is 47, 97, 197, 397, 797, 1597, 3197, 6397 ...
	 where 3197 and most of the sizes after 6397 are not prime.  The other
	 numbers listed are prime.  */
      hash_size = 2 * hash_size + 3;
      hash_table = (struct ihash_inode_val **)
	malloc(hash_size * sizeof (struct ihash_inode_val *));
      bzero (hash_table, hash_size * sizeof (struct ihash_inode_val *));

      /* Insert the values from the old table into the new table.  */
      for (i = 0; i < hash_num; i++)
	ihash_insert (old_table[i]);

      if (old_table != NULL)
	swbis_free(old_table);
    }

  /* Insert the new record and increment the count of elements in the
      hash table.  */
  ihash_insert (temp);
  hash_num++;
}

/* Do the hash insert.  Used in normal inserts and resizing the hash
   table.  It is guaranteed that there is room to insert the item.
   NEW_VALUE is the pointer to the previously allocated inode, file
   name association record.  */

void
ihash_insert (struct ihash_inode_val * new_value)
{
  int start;			/* Home position for the value.  */
  int temp;			/* Used for rehashing.  */

  /* Hash function is node number modulo the table size.  */
  start = new_value->inode % hash_size;

  /* Do the initial look into the table.  */
  if (hash_table[start] == NULL)
    {
      hash_table[start] = new_value;
      return;
    }

  /* If we get to here, the home position is full with a different inode
     record.  Do a linear search for the first NULL pointer and insert
     the new item there.  */
  temp = (start + 1) % hash_size;
  while (hash_table[temp] != NULL)
    temp = (temp + 1) % hash_size;

  /* Insert at the NULL.  */
  hash_table[temp] = new_value;
}
