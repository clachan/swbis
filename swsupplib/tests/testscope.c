#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swparser_global.h"
#include "xformat.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int 
main (int argc, char ** argv ) {
	int ret = 0;
	int depth;
	int c;
	STROB * tmp = strob_open(10);	
	if (argc < 3) exit (2);

	ret = swlib_vrealpath(argv[2], argv[1], &depth, tmp);

	fprintf(stdout, "path = [%s] depth = [%d]\n", strob_str(tmp), depth);

	exit(0);
}

