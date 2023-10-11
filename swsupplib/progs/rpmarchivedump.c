#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "um_rpmlib.h"
#include "um_header.h"

#include "uxfio.h"
#include "rpmpsf.h"
#include "swlib.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int
main(int argc, char **argv)
{
	int fd;
	TOPSF *topsf;

	if (argc <= 1) {
		topsf = topsf_open("-", UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, NULL);	/* stdin */
	} else {
		topsf = topsf_open(*(++argv), UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, NULL);
	}
	if (!topsf) {
		exit(1);
	}
	fd = topsf_get_fd(topsf);
	swlib_pump_amount(STDOUT_FILENO, fd, -1);	
	topsf_close(topsf);
	exit(0);
}
