// swptrlist.cxx

/*

 Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>

*/

/*
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#define swptrlist_cxx_200301

#include "swuser_config.h"
#include "swptrlist.h"
extern "C" {
#include "taru.h"
#include "swlib.h"
#include "swheaderline.h"
#include "md5.h"
#include "ahs.h"
}

template<class T> swPtrList<T>::swPtrList() {
      listlen_=0;
      reslen_=RESERVE_SIZE;
      list_ = static_cast<T**> (operator new[] (sizeof(T*)*(RESERVE_SIZE)));
}

template<class T> swPtrList<T>::~swPtrList() {
	operator delete[](list_);
}


template<class T> T* swPtrList<T>::get_pointer_from_index(int index) {
	if (index >= listlen_ || index < 0) return static_cast<T*>(NULL); 
	return list_[index];
}

template<class T> int swPtrList<T>::list_insert(T * before, T * node) {
	int i=0, j=0;
  
	if (listlen_ >= reslen_) re_allocate();
  
	while (i++ < listlen_) {
		if (list_[i] == before) {
			for (j=listlen_+1; j>i; j--) {
				list_[j] = list_[j-1]; 
			}
			list_[i] = node;
			listlen_++;
			return 0;
		}
	}
	return -1;
}

template<class T> int swPtrList<T>::list_add(T * node) {
	if (listlen_ >= reslen_) re_allocate();
	list_[listlen_] = node; 
	listlen_++;
	return 0;
}

template<class T> int  swPtrList<T>::list_del(int index) {
	return list_del_P(index);
}

template<class T> int  swPtrList<T>::list_del(T * node) {
	int index = get_index_from_pointer (node);
	return list_del_P(index);
}

template<class T> int swPtrList<T>::get_index_from_pointer (T * p) {
	int i=0;
	while ( i < listlen_ ) {
		if (p == list_[i])
		return i;
		i++;
	}
	return -1;
}

//D template<class T> char * swPtrList<T>::swptrlist_dump_string_s(char * prefix, void* xxswptrlistbuf)
//D {
//D 	STROB * swptrlistbuf = static_cast<STROB*>(xxswptrlistbuf);
//D 	void * x;
//D 	int i;
//D 	swPtrList * pf = this;
//D 
//D 	strob_sprintf(swptrlistbuf, 0, "%s%p (swPtrList*)\n", prefix,  (void*)pf);
//D 	strob_sprintf(swptrlistbuf, 1, "%s%p->listlen_           = [%d]\n",  prefix, (void*)pf, pf->listlen_);
//D 	strob_sprintf(swptrlistbuf, 1, "%s%p->reslen_            = [%d]\n",  prefix, (void*)pf, pf->reslen_);
//D 	strob_sprintf(swptrlistbuf, 1, "%s%p->list_              = [%p]\n",  prefix, (void*)pf, (void*)(pf->list_));
//D 
//D 	i = 0;
//D 	x = static_cast<void*>(pf->get_pointer_from_index(i));
//D 	while (x) {
//D 		strob_sprintf(swptrlistbuf, 1, "%s%p->list_[%d]           = [%p]\n",  prefix, (void*)pf, i, x);
//D 		x = static_cast<void*>(pf->get_pointer_from_index(++i));
//D 	}
//D 	
//D 	return strob_str(swptrlistbuf);
//D }

// -------- Private Functions ----------------------------------


//template<class T> void set_setnextfunc(void (*setnext_f)(T *))
//{
//	set_next_fM = setnext_f;
//}

//template<class T> void set_getnextfunc(void (*getnext_f)(T *))
//{
//	get_next_fM = getnext_f;
//}

template<class T> int  swPtrList<T>::list_del_P (int index) {
       if (index + 1 == listlen_ ) {
           listlen_--;
       } else {
          ::memmove (list_ + index, list_ + index + 1, sizeof(T*) * (listlen_ - index -1) );
          listlen_--;
       }
      return 0;
}

template<class T> int swPtrList<T>::re_allocate(void) {
      T ** p=static_cast<T**> (operator new[] (sizeof(T*)*(listlen_+RESERVE_SIZE)));
      if (!p) {
          return -1;
      }
      reslen_=listlen_+RESERVE_SIZE;
      ::memmove (p, list_, sizeof(T*)*listlen_);
      operator delete[](list_);
      list_ = p;
      return 0;
}
