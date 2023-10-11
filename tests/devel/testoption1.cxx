
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "stream_config.h"
#include "swparser.h"
#include "swpsf.h"
#include "swparser_global.h"
#include "swexcat.h"
#include "swexdistribution.h"

extern "C" {
#include "swcommon.h"
#include "usgetopt.h"
}
#include "swmain.h"
int
main (int argc, char ** argv)
{
	int ret = 9;

	fprintf(stderr, "this  test program %s is broken\n", argv[0]);
	exit(1);
	/*
	if (argc < 3) {
		fprintf(stderr, "usage %s utilname optionfile\n", argv[0]);
		exit(2);
	}

  	ret = ::parseDefaultsFile(argv[1], argv[2], ::optionsArray, 0);

	if (ret == 0)
		::swextopt_writeExtendedOptions(STDOUT_FILENO, ::optionsArray, 0);

	exit(ret);
	*/
}
