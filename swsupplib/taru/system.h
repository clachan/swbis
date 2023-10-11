/* System dependent declarations.  Requires sys/types.h.
   Copyright (C) 1992 Free Software Foundation, Inc.

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

#ifndef system_taru_h_19990124
#define system_taru_h_19990124
#include "swuser_config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef HAVE_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#ifdef HAVE_SYSMKDEV_H
#include <sys/mkdev.h>
#endif

#ifdef major
#define HAVE_MAJOR
#endif

/* On most systems symlink() always creates links with rwxrwxrwx
   protection modes, but on some (HP/UX 8.07; I think maybe DEC's OSF
   on MIPS too) symlink() uses the value of umask, so links' protection modes
   aren't always rwxrwxrwx.  There doesn't seem to be any way to change
   the modes of a link (no system call like, say, lchmod() ), it seems
   the only way to set the modes right is to set umask before calling
   symlink(). */
/*
#ifndef SYMLINK_USES_UMASK
#define UMASKED_SYMLINK(name1,name2,mode)    symlink(name1,name2)
#else
#define UMASKED_SYMLINK(name1,name2,mode)    umasked_symlink(name1,name2,mode)
#endif
*/


#endif
