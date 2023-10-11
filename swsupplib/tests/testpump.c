#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swlib.h"
#include "swvarfs.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"


int 
main (int argc, char ** argv ) 
{
	int ret;
	int amount;
	int * amount_p;
	int nullfd = open("/dev/null", O_RDWR, 0);

	if (argc > 1) {
		amount = atoi(argv[1]);
		amount_p = &amount;
	} else {
		amount_p = NULL;
	}

	ret = swgp_stdioPump(
			STDOUT_FILENO,	/* local stdout */
			STDIN_FILENO,	/* output on remote host */
			nullfd,
			nullfd,
			0,		/* int opt_verbose,  */
			1,		/* int opt_be_silent,  */
			-1,		/* int pid, */
			NULL, /* int * waitret, */
			NULL, /* int * statusp, */
			NULL, /* int * cancel,*/
			NULL, /* unsigned long int * statbytes, */
			NULL, /* int * incomingRemains */
			amount_p /* int * outgoingRemains, */
			);
	fprintf(stderr, "remains is %d\n", *amount_p);
	exit(ret);
}
