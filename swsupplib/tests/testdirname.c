#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int main (int argc, char ** argv)
{
 	STROB * tmp = strob_open(10);


	if (argc < 2) exit(2);
	
	swlib_dirname(tmp, argv[1]); 
	fprintf(stdout, "%s: dirname=(%s)\n", argv[1], strob_str(tmp));

	swlib_basename(tmp, argv[1]); 
	fprintf(stdout, "%s: basename=(%s)\n", argv[1], strob_str(tmp));

	exit (0);
}



