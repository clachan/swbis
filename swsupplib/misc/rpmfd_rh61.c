/* rpmfd_rh61.c --

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
	RPMFD * rpmfd=(RPMFD*)malloc(sizeof(RPMFD));
	if (!rpmfd) return NULL;
#ifdef HAVE_RPM_RPMIO_H
	rpmfd->fd_t_=fdNew();
	rpmfd->id_= RPMFD_FD_TYPE_R30;
#else
	rpmfd->id_= RPMFD_FD_TYPE_R25;
#endif
	rpmfd_FD_init(rpmfd);
	rpmfd_setfd(rpmfd, fd, fd_fd);
	return rpmfd;
}

void
rpmfd_close(RPMFD * rpmfd)
{
	if (rpmfd->id_ == RPMFD_FD_TYPE_R30) {
		if (rpmfd->fd_t_) swbis_free((void*)(rpmfd->fd_t_));
	}
	swbis_free(rpmfd);
}

int
rpmfd_get_fd_fd(RPMFD * rpmfd) 
{
#ifdef HAVE_RPM_RPMIO_H
	return fdFileno(rpmfd->fd_t_);
#else
	return rpmfd->fd_t_;		
#endif
}

FD_t
rpmfd_getfd(RPMFD * rpmfd) 
{
	return rpmfd->fd_t_;
}

void
rpmfd_setfd(RPMFD * rpmfd, FD_t fd, int fd_fd)
{

#ifdef HAVE_RPM_RPMIO_H
	if (rpmfd->id_ == RPMFD_FD_TYPE_R30 && fd) {
		rpmfd->fd_t_->fd_fd=fd->fd_fd;
		rpmfd->fd_t_->fd_bzd=fd->fd_bzd;
		rpmfd->fd_t_->fd_gzd=fd->fd_gzd;
		rpmfd->fd_t_->fd_url=fd->fd_url;
	} 
	else if (rpmfd->id_ == RPMFD_FD_TYPE_R30 && fd==NULL) {
		rpmfd->fd_t_->fd_fd=fd_fd;
	} else return;
#else	
		rpmfd->fd_t_=fd_fd;
#endif	
}

static
void
rpmfd_FD_init(RPMFD * rpmfd)
{
#ifdef HAVE_RPM_RPMIO_H
	if (rpmfd->id_ == RPMFD_FD_TYPE_R30)
	{
		rpmfd->fd_t_->fd_fd=-1;
		rpmfd->fd_t_->fd_bzd=NULL;
		rpmfd->fd_t_->fd_gzd=NULL;
		rpmfd->fd_t_->fd_url=NULL;
	}  else
		return;
#else
	if (rpmfd->id_ <  RPMFD_FD_TYPE_R30) 
	{
		rpmfd->fd_t_=-1;	
	} else
		return;
#endif
}

