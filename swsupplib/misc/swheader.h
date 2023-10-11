/* swheader.h -  Routines for reading the parser output.
                 in the 'swbisparse -n' style.

 Copyright (C) 1998, 1999  Jim Lowe <jhlowe@acm.org>

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

#ifndef SWHEADER_122998_H
#define SWHEADER_122998_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "strob.h"
#include "uxfio.h"
#include "strar.h"
#include "cpiohdr.h"
#include "swverid.h"

#define SW_ARCHIVE_TAR_USTAR         USTAR_FILEFORMAT
#define SW_ARCHIVE_CPIO_NEWC         CPIO_NEWC_FILEFORMAT
#define SW_ARCHIVE_CPIO_CRC          CPIO_CRC_FILEFORMAT
#define SWHEADER_IMAGE_STACK_LEN	5

#define SWHEADER_GET_NEXT	0
#define SWHEADER_PEEK_NEXT	1

typedef struct {
  int  * save_current_offset_pM; 
  int    save_current_offsetM; 
  int    save_current_offset_vM; 
} SWHEADER_STATE;

typedef char  * (*F_GOTO_NEXT)(void*, int *, int);

typedef struct {              /* swheader object */
  char * image_head_;         /* pointer to the first line of metadata, i.e. offset zero */
  void * image_object_;       /* pointer to object that knows how to iterate the metadata. */ 
  void * image_object_stack_[SWHEADER_IMAGE_STACK_LEN + 1];
  int  * current_offset_p_;   /* current offset pointer */              
  int    current_offset_;     /* current offset */              
  SWHEADER_STATE saved_;
  /* char * (*f_goto_next_)(void * cpp_object, int * current_offset, int peek_only); */
  F_GOTO_NEXT  f_goto_next_;

                              /* `f_goto_next' provides virtualness over the
			         iterator method, for C users `cpp_object' is NULL.
				 For C++ users, f_goto_next it is set to the appropriate 
				 static iterator function, current_offset is the current line of
				 which the relative next is found and returned. cpp_object 
				 is the C++ object that manages (or is) the list of metadata lines. */
} SWHEADER;


SWHEADER *swheader_open                 (char * (*f)(void *cpp_function, int * object, int peek_only), void * image_object);
void      swheader_close                (SWHEADER * swheader);
void	  swheader_store_state(SWHEADER *, SWHEADER_STATE*);
void	  swheader_restore_state(SWHEADER *, SWHEADER_STATE*);
char *	  swheader_f_goto_next		(SWHEADER * swheader);
void      swheader_set_current_offset   (SWHEADER * swheader, int offset);
void      swheader_set_current_offset_p (SWHEADER * swheader, int *offset_ptr);
void      swheader_set_current_offset_p_value (SWHEADER * swheader, int i); /* depricated */
void      swheader_set_iter_function    (SWHEADER * swheader, char * (*func)(void * cpp_obj, int * line, int peek_only));
void      swheader_set_image_head	(SWHEADER * swheader, void * image);
int       swheader_set_image_object	(SWHEADER * swheader, void * image, int stack_index);
void      swheader_set_image_object_active(SWHEADER * swheader, int stack_index);
F_GOTO_NEXT  swheader_get_iter_function (SWHEADER * swheader);
char *    swheader_get_image_head       (SWHEADER * swheader);
char **   swheader_get_attribute_by_tag (SWHEADER * swheader, char *object_keyword, char *idtag, char *attribute_keyword);
char *    swheader_get_object_by_tag    (SWHEADER * swheader, char *object_keyword, char *idtag);
char **   swheader_get_attribute_list   (SWHEADER *swheader, char *attribute_keyword, int * is_multi);
char *	  swheader_get_attribute	(SWHEADER *swheader, char *attribute_keyword, int * is_multi);
void 	  swheader_cploblist_close(char ** list);
char *    swheader_get_next_object      (SWHEADER * swheader , int relative_level_min, int relative_level_max);
char *    swheader_get_current_line     (SWHEADER * swheader);
int       swheader_get_current_offset   (SWHEADER * swheader);
int  *    swheader_get_current_offset_p (SWHEADER * swheader);
void      swheader_get_current_offset_p_value (SWHEADER * swheader, int offset);
char *    swheader_get_next_attribute   (SWHEADER * swheader);
int       swheader_generate_swverid     (SWHEADER * swheader, SWVERID * swverid, char * object_line);
char  *   swheader_goto_next_line       (void * vheader, int * output_line_offset, int peek_only);
void      swheader_reset(SWHEADER * swheader);
char *    swheader_raw_get_attribute_in_object(SWHEADER * swheader, char *keyword);
char * 	  swheader_get_attribute_in_current_object(SWHEADER * swheader, char * keyword, char *object_keyword, int * is_multi_line);
char swheader_getTarTypeFromTypeAttribute(char ch);
int swheader_fileobject2filehdr(SWHEADER * fileheader, struct new_cpio_header * file_hdr);
void swheader_print_header(SWHEADER * swheader);
/* int swheader_find_by_swpath_ex(SWHEADER * swheader, SWPATH_EX * swpath_ex);
*/
int swheader_get_object_offset_by_control_directory(SWHEADER * swheader,  STRAR * pairs);
char * swheader_get_single_attribute_value(SWHEADER * swheader, char *keyword);
SWHEADER_STATE * swheader_state_create(void);
void swheader_state_delete(SWHEADER_STATE *);
void swheader_state_copy(SWHEADER_STATE * to, SWHEADER_STATE * from);
char * swverid_get_object_by_swverid(SWHEADER * swheader, SWVERID * swverid,  int * nmatches);

/* ---------------- internal debugging function ----------------------*/

void swheader__dump(SWHEADER * swheader);

#endif

