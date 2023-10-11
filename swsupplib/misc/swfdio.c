/* swfdio.c - Set and get the stdio file descriptors
 *
 */

/*
   Copyright (C) 2005 James H. Lowe, Jr.
   All rights reserved.
  
   COPYING TERMS AND CONDITIONS
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


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "swfdio.h"

static int swfdM[3];

void
swfdio_init(void)
{
	swfdM[0] = STDIN_FILENO;
	swfdM[1] = STDOUT_FILENO;
	swfdM[2] = STDERR_FILENO;
}

void
swfdio_set(int index, int fd)
{
	if (index < 0 || index > 2)
		return;
	swfdM[index] = fd;
}

int
swfdio_get(int index)
{
	if (index < 0 || index > 2)
		return -1;
	return swfdM[index];
}
