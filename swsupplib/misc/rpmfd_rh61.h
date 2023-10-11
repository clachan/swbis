/* rpmfd.h  --  The RPM virtual file descriptor object.

   Copyright (C) 1999  Jim Lowe 

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

#ifndef RPMFD_19990918_H
#define RPMFD_19990918_H

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "um_rpmlib.h"

#define RPMFD_FD_TYPE_R25	25
#define RPMFD_FD_TYPE_R30	30

typedef struct {
	FD_t fd_t_;
	int id_;
} RPMFD;

RPMFD * rpmfd_open	(FD_t fd, int fd_fd);
void	rpmfd_close	(RPMFD * rpmfd);
FD_t	rpmfd_getfd	(RPMFD * rpmfd);
int 	rpmfd_get_fd_fd	(RPMFD * rpmfd);
void 	rpmfd_setfd	(RPMFD * rpmfd, FD_t fd, int fd_fd);

#endif

