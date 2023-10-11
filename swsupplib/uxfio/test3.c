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
#include "uxfio.h"

int main(int argc, char ** argv)
{
	int fd1;
	int fd2;

	fd1 = uxfio_opendup(STDIN_FILENO, UXFIO_BUFTYPE_NOBUF);
	fd2 = uxfio_opendup(STDOUT_FILENO, UXFIO_BUFTYPE_NOBUF);

	 fprintf(stderr, "fd1 = %d   fd2 = %d\n", fd1, fd2);
	if (argc > 1)
	 	fprintf(stderr, "ret = %d\n", uxfio_fcntl(fd2,  UXFIO_F_SET_OUTPUT_BLOCK_SIZE, atoi(argv[1])));

	swlib_pipe_pump(fd2, fd1);

	/* uxfio_fsync(fd2); */
	uxfio_close(fd2);
	uxfio_close(fd1);
	exit(0);
}
