/* rpmfd.c --

   Copyright (C) 1999  Jim Lowe  <jhlowe@acm.org>
   All rights reserved.

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

#include "rpmfd.h"

#ifdef USE_NEW_FDNEW62
#include "./rpmfd_rh62.c"
#elif USE_NEW_FDNEW72
#include "./rpmfd_rh72.c"
#elif USE_NEW_FDNEW80
#include "./rpmfd_rh80.c"
#else
#include "./rpmfd_rh61.c"
#endif
