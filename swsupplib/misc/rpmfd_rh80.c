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


#include "swuser_config.h"
#include "rpmfd.h"

static void rpmfd_FD_init(RPMFD * rpmfd);

RPMFD * rpmfd_open(FD_t fd, int fd_fd)
{
	/* fd is not used */
	RPMFD * rpmfd=(RPMFD*)malloc(sizeof(RPMFD));
	if (!rpmfd) return NULL;
	rpmfd->fd_t_=fdNew("rpmpsf");
	rpmfd->id_= 0; /* not used */
	rpmfd_FD_init(rpmfd);
	rpmfd_setfd(rpmfd, (FD_t)NULL, fd_fd);
	return rpmfd;
}

void
rpmfd_close(RPMFD * rpmfd)
{
	if (rpmfd->fd_t_) swbis_free((void*)(rpmfd->fd_t_));
	swbis_free(rpmfd);
}

int
rpmfd_get_fd_fd(RPMFD * rpmfd) 
{
	return fdFileno(rpmfd->fd_t_);
}

FD_t
rpmfd_getfd(RPMFD * rpmfd) 
{
	return rpmfd->fd_t_;
}

void
rpmfd_setfd(RPMFD * rpmfd, FD_t fd, int fd_fd)
{
	if (fd) {
		rpmfd->fd_t_->fps[0].fdno = fd->fps[0].fdno;
		rpmfd->fd_t_->fps[1].fdno = fd->fps[1].fdno;
		rpmfd->fd_t_->fps[2].fdno = fd->fps[2].fdno;
		rpmfd->fd_t_->fps[3].fdno = fd->fps[3].fdno;
	} 
	else {
		/* return fd->fps[0].fdno */
		rpmfd->fd_t_->fps[0].fdno=fd_fd;
	} 
}

static
void
rpmfd_FD_init(RPMFD * rpmfd)
{
	rpmfd->fd_t_->fps[0].fdno=-1;
	rpmfd->fd_t_->fps[1].fdno=-1;
	rpmfd->fd_t_->fps[2].fdno=-1;
	rpmfd->fd_t_->fps[3].fdno=-1;
}
