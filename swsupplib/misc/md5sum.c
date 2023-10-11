/*
 * md5sum.c	- Generate/check MD5 Message Digests
 *
 * Compile and link with md5.c.  If you don't have getopt() in your library
 * also include getopt.c.  For MSDOS you can also link with the wildcard
 * initialization function (wildargs.obj for Turbo C and setargv.obj for MSC)
 * so that you can use wildcards on the commandline.
 *
 * Written March 1993 by Branko Lankester
 * Modified June 1993 by Colin Plumb for altered md5.c.
 * Modified October 1995 by Erik Troan for RPM
 * Modified October 1998 by Jim Lowe to use uxfio_fd unix-like descriptor.
 */
#include "swuser_config.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "uxfio.h"
#include "md5.h"

static int domd5(int uxfio_fd, unsigned char * digest, int asAscii, int broken);

static int domd5(int uxfio_fd, unsigned char * digest, int asAscii, int brokenEndian) {
    unsigned char buf[1024];
    unsigned char bindigest[16];
    MD5_CTX ctx;
    int n;

    MD5Init(&ctx, brokenEndian);
    while ((n = uxfio_read(uxfio_fd, buf, sizeof(buf))) > 0)
	    MD5Update(&ctx, buf, n);
    MD5Final(bindigest, &ctx);
    
    if (!asAscii) {
	memcpy(digest, bindigest, 16);
    } else {
	sprintf((char*)digest, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
			"%02x%02x%02x%02x%02x",
		bindigest[0],  bindigest[1],  bindigest[2],  bindigest[3],
		bindigest[4],  bindigest[5],  bindigest[6],  bindigest[7],
		bindigest[8],  bindigest[9],  bindigest[10], bindigest[11],
		bindigest[12], bindigest[13], bindigest[14], bindigest[15]);

    }
    return 0;
}

int 
swlib_md5_from_memblocks(void * thisisa, char *digest, unsigned char * iblock, int icount) {
    MD5_CTX * ctx = (MD5_CTX *)thisisa;
    int n;

    if (iblock == NULL && icount < 0) {
	    MD5Init(ctx, /* brokenEndian */ 0);
            return 0;
    }
   
    if (icount > 0) {
	    n = icount;
	    MD5Update(ctx, iblock, icount);
            return 0;
    }
   
    {
    unsigned char bindigest[16];

    MD5Final(bindigest, ctx);
    
    sprintf(digest, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
			"%02x%02x%02x%02x%02x",
		bindigest[0],  bindigest[1],  bindigest[2],  bindigest[3],
		bindigest[4],  bindigest[5],  bindigest[6],  bindigest[7],
		bindigest[8],  bindigest[9],  bindigest[10], bindigest[11],
		bindigest[12], bindigest[13], bindigest[14], bindigest[15]);
    return 0;
    }
}

int swlib_md5(int uxfio_fd, unsigned char *bindigest, int do_ascii) {
    int ret;
    ret = domd5(uxfio_fd, bindigest, do_ascii, 0);
    bindigest[32] = '\0';
    return ret;
}

