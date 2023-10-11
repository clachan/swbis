/* swptrlist.h
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#ifndef swptrlist_19990215_h
#define swptrlist_19990215_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <typeinfo>

template<class T> class swPtrList
{
    T ** list_;
    int listlen_;  // current length;
    int reslen_;  // current total reserve

  public:
    swPtrList ();
    ~swPtrList ();
    enum class_constants { RESERVE_SIZE = 10 };
    
    int list_insert(T * before, T * node);
    int list_add(T * node);
    int list_del(int index);
    int list_del(T* node);
    int length(void) { return listlen_; }
    int get_index_from_pointer (T * p);
    T * get_pointer_from_index(int index);
  
    // char * swptrlist_dump_string_s(char * prefix, void * strob_buffer);


    //void * get_list(void) { return static_cast<void>(list_); }
    //int get_listlen(void) { return listlen_; }
    //int get_reslen(void) { return reslen_; }


 private:
    int list_del_P(int index);
    int re_allocate(void);

};
#ifndef swptrlist_cxx_200301
#include "swptrlist.cxx"
#endif
#endif
