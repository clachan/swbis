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
#ifdef USE_WITH_RPM
#include "um_rpmlib.h"
#include "um_header.h"
#endif
#ifdef USE_WITH_SELF_RPM
#include "um_rpmlib.h"
#include "um_header.h"
#endif

#ifdef HAVE_RPM_RPMIO_H
#include "um_rpmio.h"
#else
#define FD_t int
#endif

#ifdef USE_NEW_FDNEW62
#include "rpmfd_rh62.h"
#endif

#ifdef USE_NEW_FDNEW72
#include "rpmfd_rh72.h"
#endif

#ifdef USE_NEW_FDNEW80
#include "rpmfd_rh80.h"
#endif


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
