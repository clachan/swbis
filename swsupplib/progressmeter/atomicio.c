/*
 * Copyright (c) 1995,1999 Theo de Raadt.  All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/****  This file has been modified by JHL for swbis *****/


/* ChangeLog

2004 thru 2007-08-03 <jhlowe@acm.org>:  Changes for inclusion in swbis
2007-08-04 <jhlowe@acm.org>: updated atomicio() routine
2008-01-02 <jhlowe@acm.org>: added timed_atomicio()

*/

#include "includes.h"
/* RCSID("$OpenBSD: atomicio.c,v 1.12 2003/07/31 15:50:16 avsm Exp $"); */

#include "atomicio.h"

/*
 * ensure all of data on socket comes through. f==read || f==vwrite
 */

ssize_t
timed_atomic_read5(int fd, void * buf, size_t n)
{
	return timed_atomicio(read, fd, buf, n, (int)5);
}

ssize_t
timed_atomicio(ssize_t (*f) (int, void *, size_t),  int fd, void * buf, size_t n, int timeout)
{
	const int pipe_buf_size = 512;
	int flags;
	int ret;
	int readret;
	char * s;
	ssize_t res;
	ssize_t pos;
	size_t rq;
	time_t tm0;
	time_t tm;

	flags = fcntl(fd, F_GETFL, O_NONBLOCK);
	ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (flags < 0 || ret < 0) {
		fprintf(stderr, "%s: non-blocking I/O request ignored, fcntl() status=%d: %s\n", swlib_utilname_get(), flags, strerror(errno));
		E_DEBUG("");
		return atomicio(f, fd, buf, n);
	}
	
	tm0 = time(NULL);
	tm = tm0;
	s = buf;
	pos = 0;
	while ((int)(tm - tm0) < timeout && n > (size_t)pos) {
		tm = time(NULL);
		rq = (size_t)(n - pos);
		if ((int)rq > pipe_buf_size) {
			rq = pipe_buf_size;
		}
		res = safeio(f, fd, s+pos, rq);
		switch (res) {
		case -1:
			if (errno == EAGAIN) {
				continue;
			}
			return -1;
		case 0:
			return 0;
		default:
			if (res < 0) return -1; /* impossible OS error */
			pos += res;
		}
	}
	fcntl(fd, F_SETFL, flags);
	if ((int)(tm - tm0) >= timeout) {
		fprintf(stderr, "%s: %d second I/O time limit exceeded: %d bytes transferred, %d expected\n", swlib_utilname_get(), (int)timeout, (int)pos, (int)n);
	}
	return (pos);
}


ssize_t
atomicio(ssize_t (*f) (int, void *, size_t),  int fd, void * _s, size_t n)
{
	const int pipe_buf_size = 512;
	char * s;
	ssize_t res;
	ssize_t pos;
	size_t rq;
	
	s = _s;
	pos = 0;
	while (n > (size_t)pos) {
		rq = (size_t)(n - pos);
		if ((int)rq > pipe_buf_size) {
			rq = pipe_buf_size;
		}
		res = safeio(f, fd, s+pos, rq);
		switch (res) {
		case -1:
			return -1;
		case 0:
			return 0;
		default:
			if (res < 0) return -1; /* impossible OS error */
			pos += res;
		}
	}
	return (pos);
}

ssize_t
safeio(f, fd, _s, n)
	ssize_t (*f) (int, void *, size_t);
	int fd;
	void *_s;
	size_t n;
{
	const int pipe_buf_size = 512;
	char *s = _s;
	ssize_t res, pos = 0;

	while (pos == 0) {
		res = (f) (fd, s, n);
		switch (res) {
		case -1:
#ifdef EWOULDBLOCK
			if (errno == EINTR /* || errno == EAGAIN || errno == EWOULDBLOCK */ ) {
#else
			if (errno == EINTR /* || errno == EAGAIN */ ) {
#endif
			/*
			fprintf(stderr, "safeio: res=%d errno=%d: %s\n", (int)res, errno, strerror(errno));
			*/
				continue;
			}
		case 0:
			return (res);
		default:
			/*
			 * we are assuming that 'res' will never be negative here.
			 */ 
			pos += res;
		}
	}
	return (pos);
}
