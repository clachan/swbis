#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swparser_global.h"
#include "usgetopt.h"
#include "swutillib.h"
#include "swlib.h"
#include "swi.h"
#include "xformat.h"
#include "swevents_array.h"
#include "swcommon_options.h"


int 
main (int argc, char ** argv )
{
	FILE_DIGS digs;
	taru_digs_init(&digs, DIGS_ENABLE_ON, 0);

	digs.do_md5 = DIGS_ENABLE_ON;
	digs.do_sha1 = DIGS_ENABLE_ON;
	digs.do_sha512 = DIGS_ENABLE_ON;
	digs.do_size = DIGS_ENABLE_ON;
	
	swlib_digs_copy(STDOUT_FILENO, STDIN_FILENO, -1, &digs, -1);

	fprintf(stderr, "%s\n", digs.size);
	fprintf(stderr, "%s\n", digs.md5);
	fprintf(stderr, "%s\n", digs.sha1);
	fprintf(stderr, "%s\n", digs.sha512);

	exit(0);
}

