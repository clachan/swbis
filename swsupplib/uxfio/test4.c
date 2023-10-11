#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
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
	int ret;

	fd1 = uxfio_open("/dev/null", O_RDONLY, 0644);
	if (fd1 < 0) exit(1);
	if (uxfio_fcntl(fd1, UXFIO_F_SET_BUFACTIVE, UXFIO_ON) < 0) exit(3);
	if (uxfio_fcntl(fd1, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM) < 0) exit (2);

	swlib_pipe_pump(fd1, STDIN_FILENO);

	ret = uxfio_lseek(fd1, (off_t)0, SEEK_SET);
	if (ret!= 0) { fprintf(stderr, "ret=%d\n", ret); exit(4); }

	fd2 = uxfio_opendup(STDOUT_FILENO, UXFIO_BUFTYPE_NOBUF);
	if (fd2 < 0) exit(5);
	if (argc > 1) {
	 	ret = uxfio_fcntl(fd2, UXFIO_F_SET_OUTPUT_BLOCK_SIZE, atoi(argv[1]));
		if (ret != 0)
		 	fprintf(stderr, "ret = %d\n", ret);
	}

	swlib_pipe_pump(fd2, fd1);

	uxfio_close(fd2);
	uxfio_close(fd1);
	exit(0);
}
