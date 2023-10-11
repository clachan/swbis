/* rpmfd_rh62.h  --  The RPM virtual file descriptor object.

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

#ifndef RPMFD_RH62_20000909_H
#define RPMFD_RH62_20000909_H

#include <sys/time.h>

typedef struct _FDSTACK_s {
        FDIO_t          io;
/*@dependent@*/ void *  fp;
        int             fdno;
} FDSTACK_t;

typedef struct {
        int             count;
        off_t           bytes;
        time_t          msecs;
} OPSTAT_t;

typedef struct {
        struct timeval  create;
        struct timeval  begin;
        OPSTAT_t        ops[4];
#define FDSTAT_READ     0
#define FDSTAT_WRITE    1
#define FDSTAT_SEEK     2
#define FDSTAT_CLOSE    3
} FDSTAT_t;

struct _FD_s {
/*@refs@*/ int          nrefs;
        int             flags;
#define RPMIO_DEBUG_IO          0x40000000
#define RPMIO_DEBUG_REFS        0x20000000
        int             magic;
#define FDMAGIC         0xbeefdead

        int             nfps;
        FDSTACK_t       fps[8];
        int             urlType;        /* ufdio: */

/*@dependent@*/ void *  url;            /* ufdio: URL info */
        int             rd_timeoutsecs; /* ufdRead: per FD_t timer */
        ssize_t         bytesRemain;    /* ufdio: */
        ssize_t         contentLength;  /* ufdio: */
        int             persist;        /* ufdio: */
        int             wr_chunked;     /* ufdio: */

        int             syserrno;       /* last system errno encountered */
/*@observer@*/ const void *errcookie;   /* gzdio/bzdio/ufdio: */

        FDSTAT_t        *stats;         /* I/O statistics */

        int             ftpFileDoneNeeded; /* ufdio: (FTP) */
        unsigned int    firstFree;      /* fadio: */
        long int        fileSize;       /* fadio: */
        long int        fd_cpioPos;     /* cpio: */
};


#endif
