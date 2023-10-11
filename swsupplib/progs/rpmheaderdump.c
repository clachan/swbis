#include "swuser_config.h"
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "um_rpmlib.h"
#include "um_header.h"
#include "uxfio.h"
#include "topsf.h"

#include "uinfile.h"
#include "rpmpsf.h"
#include "swfork.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"


int
main(int argc, char **argv)
{
	Header h;
	int c, swdump = 0, optionerror = 0;
	TOPSF *topsf;

	while ((c = getopt(argc, argv, "s")) != EOF) {
		switch (c) {
		case 's':
			swdump = 1;
			break;
		default:
			optionerror = 1;
			break;
		}
	}
	argv += optind;
	argc -= optind;

	if (optionerror) {
		fprintf(stderr, "Usage: rpmheaderdump [-s]\n");
		fprintf(stderr, "    -s  Second form Ascii dump.\n");
		fprintf(stderr, "rpmheaderdump reads an rpm from stdin.\n");
		exit(1);
	}

	if (argc <= 1) {
		topsf = topsf_open("-", UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM|TOPSF_OPEN_NO_AUDIT, NULL);	/* stdin */
	} else {
		topsf = topsf_open(*(++argv), UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM|TOPSF_OPEN_NO_AUDIT, NULL);
	}
	if (!topsf) {
		exit(1);
	}
	if (swdump) {
		headerDumpSw(topsf, stdout, 1, rpmTagTable);
	} else {
		h = topsf_get_rpmheader(topsf);
		headerDump(h, stdout, 1, rpmTagTable);
		headerFree(h);
	}
	exit(0);
}


/* this is so we can link RPM without -lintl. It makes it about 10k
   smaller, so its worth doing for tight corners (like install time) */

void bindtextdomain(char * a, char * b);
void textdomain(char * a);
char * gettext(char * a);

void bindtextdomain(char * a, char * b) {
}

char * gettext(char * a) {
    return a;
}

void textdomain(char * a) {
}


