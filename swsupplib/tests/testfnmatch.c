#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"


int 
main (int argc, char ** argv ) {
	
	if (argc < 4) {
		fprintf(stderr, "Usage:  pattern string flags\n");
		exit(1);
	}
	fprintf(stderr, "FNM_NOESCAPE = %d\n", FNM_NOESCAPE);
	fprintf(stderr, "FNM_PATHNAME = %d\n", FNM_PATHNAME);
	fprintf(stderr, "FNM_PERIOD = %d\n", FNM_PERIOD);
/*	fprintf(stderr, "FNM_LEADING_DIR = %d\n", FNM_LEADING_DIR); */
	fprintf(stderr, "fnmatch ( %s , %s , %d )\n", argv[1], argv[2], atoi(argv[3]));
	fprintf(stderr, "result = %d\n", fnmatch(argv[1], argv[2], atoi(argv[3])));
	
	exit(0);
}

