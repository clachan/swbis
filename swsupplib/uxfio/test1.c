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
	int nullfd = open("/dev/null", O_RDWR, 0);
	int fd1;
	int fd;
	int offset;
	int amount;
	int testnumber;

	if (argc < 4) exit(3);

	testnumber = atoi(argv[1]);
	offset = atoi(argv[2]);
	amount = atoi(argv[3]);


	fd1 = STDIN_FILENO;
	fd = uxfio_opendup(fd1, UXFIO_BUFTYPE_DYNAMIC_MEM);
	
	/* uxfio_fcntl(fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
	*/

	if (uxfio_lseek(fd, offset, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error 1\n");
		exit(2);
	}

	if (uxfio_fcntl(fd, UXFIO_F_SET_VEOF, amount) < 0) 
		fprintf(stderr, "fd = %d\n", fd);

	/* 
	uxfio_debug_dump(fd);
	uxfio_debug_dump(fd);
	swlib_pipe_pump(STDOUT_FILENO, fd);
	*/
	
	if (testnumber == 1) {

	} else if (testnumber == 2) {	
		swlib_pipe_pump(nullfd, fd);
		if (uxfio_lseek(fd, 0, UXFIO_SEEK_VSET) != 0) fprintf(stderr, "error 2\n");
		if (uxfio_fcntl(fd, UXFIO_F_SET_VEOF, amount) < 0) fprintf(stderr, "fd = %d\n", fd);
	}

	swlib_pipe_pump(STDOUT_FILENO, fd);

	uxfio_close(fd);
	return 0;
}
