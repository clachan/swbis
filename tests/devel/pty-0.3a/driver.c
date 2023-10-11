#include	<sys/types.h>
#include	<signal.h>
#include	"ourhdr.h"

int
do_driver(char *driver, int * ofd, int * ifd)
{
	pid_t	child;
	int		pipe0[2];
	int		pipe1[2];

		/* create a stream pipe to communicate with the driver */
	if (pipe(pipe0) < 0)
		err_sys("can't create stream pipe");
	
	if (pipe(pipe1) < 0)
		err_sys("can't create stream pipe");

	if ( (child = fork()) < 0)
		err_sys("fork error");

	else if (child == 0) {			/* child */
		close(pipe0[1]);
		close(pipe1[0]);

				/* stdin for driver */
		if (dup2(pipe0[0], STDIN_FILENO) != STDIN_FILENO)
			err_sys("dup2 error to stdin");

				/* stdout for driver */
		if (dup2(pipe1[1], STDOUT_FILENO) != STDOUT_FILENO)
			err_sys("dup2 error to stdout");
		close(pipe0[0]);
		close(pipe1[1]);

				/* leave stderr for driver alone */

		execlp(driver, driver, (char *) 0);
		err_sys("execlp error for: %s", driver);
		_exit(0);
	}

	close(pipe0[0]);		/* parent */
	close(pipe1[1]);		/* parent */

	/*
	if (dup2(pipe[1], STDIN_FILENO) != STDIN_FILENO)
		err_sys("dup2 error to stdin");

	if (dup2(pipe[1], STDOUT_FILENO) != STDOUT_FILENO)
		err_sys("dup2 error to stdout");
	close(pipe[1]);
	*/

	*ifd =  pipe0[1];
	*ofd =  pipe1[0];

	/* Parent returns, but with stdin and stdout connected
	   to the driver. */
	return (int)child;
}
