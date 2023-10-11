#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "swlib.h"
#include "xformat.h"
#include "strtoint.h"
#include "swmain.h"  /* this file should be included by the main program file */
#include "inttostr.h"
#include "config_remains.h" 

int
main(int argc, char ** argv)
{
	int largefile = 0;
	int fileoffset = 0;

	char u_buf[UINTMAX_STRSIZE_BOUND];


#ifdef _FILE_OFFSET_BITS
	fileoffset = 1;
#endif

#ifdef _LARGEFILE64_SOURCE
	largefile = 1;
#endif

	fprintf(stdout, "        intmax_t   = %d\n", (int)sizeof(intmax_t));
	fprintf(stdout, "        uintmax_t  = %d\n", (int)sizeof(uintmax_t));
	fprintf(stdout, "        int        = %d\n", (int)sizeof(int));
	fprintf(stdout, "        long       = %d\n", (int)sizeof(long));
	fprintf(stdout, "        off_t      = %d\n", (int)sizeof(off_t));
	fprintf(stdout, "        LONG_MAX   = %s\n", imaxtostr(LONG_MAX, u_buf));
	fprintf(stdout, "        LLONG_MAX  = %s\n", imaxtostr(LLONG_MAX, u_buf));
	fprintf(stdout, "        ULONG_MAX  = %s\n", umaxtostr(ULONG_MAX, u_buf));
	fprintf(stdout, "        ULLONG_MAX = %s\n", umaxtostr(ULLONG_MAX, u_buf));

	if (sizeof(off_t) < sizeof(uintmax_t) && sizeof(uintmax_t) > 5) {
		fprintf(stdout, "intsizes: warning: Large File Support not fully enabled.\n");
		fprintf(stdout, "intsizes: warning: off_t is smaller that intmax_t\n");
		fprintf(stdout, "intsizes: warning: This should have happened automatically...?, unless\n");
		fprintf(stdout, "intsizes: warning: the --disable-largefile configure option was used\n");
		fprintf(stdout, "intsizes: warning: To force the issue, try:\n");
		fprintf(stdout, "intsizes: warning: Try  ''./configure CFLAGS=\"-D_FILE_OFFSET_BITS=64\" && make''\n");
	} else if (
		sizeof(off_t) == sizeof(uintmax_t) &&
		sizeof(intmax_t) != 8 &&
		(fileoffset == 0) 
	) {
		fprintf(stdout, "intsizes: warning: Large File Support may not be fully enabled.\n");
		fprintf(stdout, "intsizes: warning: The integer sizes are OK, but you may have to\n");
		fprintf(stdout, "intsizes: warning: define _FILE_OFFSET_BITS=64\n");
		fprintf(stdout, "intsizes: warning: This should have happened automatically...?, unless\n");
		fprintf(stdout, "intsizes: warning: the --disable-largefile configure option was used\n");
		fprintf(stdout, "intsizes: warning: To force the issue, try:\n");
		fprintf(stdout, "intsizes: warning: ''./configure CFLAGS=\"-D_FILE_OFFSET_BITS=64\" && make''\n");
	} else if (sizeof(off_t) == sizeof(uintmax_t) && sizeof(uintmax_t) >= 6 && fileoffset == 1) {
		fprintf(stdout, "intsizes: Large File Support enabled\n");
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif
		fprintf(stdout, "intsizes: _FILE_OFFSET_BITS=%d\n", _FILE_OFFSET_BITS);
	} else if (sizeof(off_t) == sizeof(uintmax_t) && sizeof(uintmax_t) >= 6 && sizeof(int) >= 6) {
		fprintf(stdout, "intsizes: Looks like a 64-bit machine, you don't need Large File Support.\n");
	} else {
		;
	}
	exit(0);
}
