#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "xformat.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"
#include "swlib.h"
#include "swgpg.h"

int
main (int argc, char *argv[])
{
	int ret;
	STROB * tmp = strob_open(32);
	STROB * gpgstatus = strob_open(32);
	int file_fd;
	int sig_fd;
	int memfd;
	char * sig;

	if (argc < 3) exit(1);

	if (strcmp(argv[1], "-") == 0) {
		file_fd = STDIN_FILENO;
	} else {
		file_fd = open(argv[1], O_RDONLY);
	}
	if (file_fd < 0) exit(2);

	sig_fd = open(argv[2], O_RDONLY);
	if (sig_fd < 0) exit(3);

	memfd = swlib_open_memfd();

	swlib_pipe_pump(memfd, sig_fd);
	sig = uxfio_get_fd_mem(memfd, NULL);

	ret = swgpg_run_gpg_verify(file_fd, sig, 2 /* uverbose */, "gpg", gpgstatus);
	fprintf(stderr, "swgpg_run_gpg_verify returned %d\n", ret);
	fprintf(stderr, "%s\n", strob_str(gpgstatus));
	if (ret == 0)
		exit(0);
	else
		exit(1);
}
