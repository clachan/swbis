#include	"config.h"
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<sys/wait.h>
#include	<signal.h>
#ifdef HAVE_ERROR_H
#include	<error.h>
#endif
#include	<errno.h>
#include	<termios.h>
#ifndef	TIOCGWINSZ
#include	<sys/ioctl.h>	/* 44BSD requires this too */
#endif
#include	"ourhdr.h"

static void	set_noecho(int);	/* at the end of this file */
int		do_driver(char *, int *, int* );	/* in the file driver.c */
int		loop(int, int, int, int, int, int, int*, int*, int);		/* in the file loop.c */

int
main(int argc, char *argv[])
{
	int ret;
	int xret;
	int loopcount = 0;
	int				fdm, c, ignoreeof, interactive, noecho, verbose;
	pid_t			apid;
	char			*driver, slave_name[20];
	struct termios	orig_termios;
	struct winsize	size;
	pid_t driver_pid = 0;
	int pgmexitval=99;
	int exitval=99;
	int waitret;
	int status;
	int ofd, ifd;

	interactive = isatty(STDIN_FILENO);
	ignoreeof = 0;
	noecho = 0;
	verbose = 0;
	driver = NULL;
	
	opterr = 0;		/* don't want getopt() writing to stderr */
	while ( (c = getopt(argc, argv, "d:einv")) != EOF) {
		switch (c) {
		case 'd':		/* driver for stdin/stdout */
			driver = optarg;
			break;

		case 'e':		/* noecho for slave pty's line discipline */
			noecho = 1;
			break;

		case 'i':		/* ignore EOF on standard input */
			ignoreeof = 1;
			break;
		case 'n':		/* not interactive */
			interactive = 0;
			break;

		case 'v':		/* verbose */
			verbose = 1;
			break;

		case '?':
			err_quit("unrecognized option: -%c", optopt);
		}
	}
	if (optind >= argc) {
		fprintf(stderr, 
			"usage: pty [ -d driver -weinv ] program [ arg ... ]\n"
			"	-d pgm   execute driver and use output as program input.\n"
			"	         Note: Sometimes a delay is needed the driver pgm.\n"
			"	-e     no echo.\n"
			"	-i     ignore EOF.\n"
			"	-n     non interactive.\n"
			"	-v     be verbose.\n"
			);
		exit(1);
	}

	if (interactive) {	/* fetch current termios and window size */
		if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
			err_sys("tcgetattr error on stdin");
		if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size) < 0)
			err_sys("TIOCGWINSZ error");
		apid = pty_fork(&fdm, slave_name, &orig_termios, &size);
	} else {
		apid = pty_fork(&fdm, slave_name, NULL, NULL);
	}

	if (apid < 0)
		err_sys("fork error");

	else if (apid == 0) {		/* child */
		if (noecho)
			set_noecho(STDIN_FILENO);	/* stdin is slave pty */

		if (execvp(argv[optind], &argv[optind]) < 0)
			err_sys("can't execute: %s", argv[optind]);
		_exit(23);
	}

	if (verbose) {
		fprintf(stderr, "slave name = %s\n", slave_name);
		if (driver != NULL)
			fprintf(stderr, "driver = %s\n", driver);
	}

	if (interactive && driver == NULL) {
		if (tty_raw(STDIN_FILENO) < 0)	/* user's tty to raw mode */
			err_sys("tty_raw error");
		if (atexit(tty_atexit) < 0)		/* reset user's tty on exit */
			err_sys("atexit error");
	}

	if (driver) {
		driver_pid = (pid_t)do_driver(driver, &ofd, &ifd);	/* changes our stdin/stdout */
			/*
			ofd is the driver output which must be read from.
			*/
		xret = loop(fdm, ofd, STDOUT_FILENO, fdm, ignoreeof, apid, &waitret, &status, verbose);
		close(ifd);
		close(ofd);
		close(fdm);
	} else {
		ofd = STDOUT_FILENO;
		ifd = STDIN_FILENO;
		xret = loop(ofd, fdm, fdm, ifd, ignoreeof, apid, &waitret, &status, verbose);
		close(fdm);
	}

	if (waitret > 0) {
		if (WIFEXITED(status)) {
			pgmexitval=WEXITSTATUS(status);
		} else {
			pgmexitval=220;
		}
	}

	if (driver_pid) {
		if (kill(driver_pid, SIGTERM) < 0) {
			fprintf(stderr, "kill error: %s\n", strerror(errno));
		}
	}

	if (driver_pid) {
		loopcount = 0;
		ret = waitpid(driver_pid, &status, WNOHANG);
		if (verbose) fprintf(stderr, "HERE 1\n");
		while(ret == 0 && loopcount < 30) {
			sleep(1);
			loopcount ++;
			if (verbose) fprintf(stderr, "HERE 2 loopcount = %d\n", loopcount);
			ret = waitpid(driver_pid, &status, WNOHANG);
		}	
		if (ret > 0) {
			if (verbose) fprintf(stderr, "HERE 3 \n");
			if (WIFEXITED(status)) {
				exitval=WEXITSTATUS(status);
			} else {
				exitval=240;
				exitval=pgmexitval;
			}		
		} else {
			if (verbose) fprintf(stderr, "HERE 4 \n");
			kill(driver_pid, SIGTERM);
			waitpid(driver_pid, &status, 0);
			if (WIFEXITED(status)) {
				exitval=WEXITSTATUS(status);
			} else {
				exitval=241;
				exitval=pgmexitval;
			}		
		}
	} else {
		exitval=pgmexitval;
	}

	if (driver_pid) {
		/*
		fprintf(stderr, "%s exitval = [%d]\n", argv[optind], pgmexitval);
		*/
	}
	exit(exitval);
}

static void
set_noecho(int fd)		/* turn off echo (for slave pty) */
{
	struct termios	stermios;

	if (tcgetattr(fd, &stermios) < 0)
		err_sys("tcgetattr error");

	stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	stermios.c_oflag &= ~(ONLCR);
			/* also turn off NL to CR/NL mapping on output */

	if (tcsetattr(fd, TCSANOW, &stermios) < 0)
		err_sys("tcsetattr error");
}
