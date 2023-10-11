#include	"config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <getopt.h>
#include <utime.h>
#ifdef HAVE_ERROR_H
#include	<error.h>
#endif
#include <errno.h>
#include	"ourhdr.h"
#include	<sys/types.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include <sys/select.h>

int gl_verbose = 0;
char  gl_who[100];
char  gl_jobNumber[1000];
static int gl_close_wait_time=25;
static int g_sigpipe_ret = 0;
static int g_pid1 = 0;
static int g_tarpid = 0;

#define PIPE_BUF 	512
#define	BUFFSIZE	512

static void	sig_term(int);
static volatile sig_atomic_t	sigcaught;	/* set by signal handler */
static int stdioPump(int stdoutfd, int outputpipefd, int inputpipefd, int stdinfd, int opt_verbose, int pid, int * waitret, int * statusp);
static int swlib__pipe_pump(int suction_fd, int discharge_fd, int *status, int *childGotStatus, int *amount);

static int
check_pid(int pid, int * do_break)
{
	int status;
	int ret;
	int exitval = 0;
	*do_break = 0;
	ret = waitpid((pid_t)pid, &status, WNOHANG);
	if (ret < 0) {
		*do_break = 1;
		return ret;
	}
	if (ret > 0) {
		if (WIFEXITED(status)) {
			exitval=WEXITSTATUS(status);
		} else {
			exitval=220;
		}
		*do_break = 1;
	}
	return exitval;
}

int
loop(int ptymin, int ofd, int ifd, int ptymout, int ignoreeof, int pid, int * waitret, int * statusp, int verbose)
{ 
	int ret;
	/*
		ptymin <- ofd ; ifd <- ptymout
	*/
	ret = stdioPump (ptymin, ofd, ifd, ptymout,  verbose, pid, waitret, statusp);
	return ret;
}

/* The child sends us a SIGTERM when it receives an EOF on
 * the pty slave or encounters a read() error. */

static void
sig_term(int signo)
{
	sigcaught = 1;		/* just set flag and return */
	return;				/* probably interrupts read() of ptym */
}

static void
sig_fatal_handler(int signo)
{
	int status;
	if (signo == SIGPIPE) {
		fprintf(stderr, "Caught SIGPIPE on %s (fatal).\n", gl_who);
		if (gl_verbose) fprintf(stderr, "waiting on tar process %d\n", g_pid1);
		if (g_pid1 > 0) kill(g_pid1, SIGTERM);
		if (g_pid1 > 0) waitpid(g_pid1, &status, 0);	
		exit(2);
	}
}
static void
subsig_handler(int signo)
{
	if (signo == SIGTERM) {
		fprintf(stderr, "Caught SIGTERM on (sub) %s.\n", gl_who);
		if (gl_verbose) fprintf(stderr, "Sending sigterm to tar process %d\n", g_tarpid);
		if (g_tarpid > 0) kill(g_tarpid, SIGTERM);
		exit(0);
	}
}

static void
sig_handler(int signo)
{
	if (signo == SIGTERM) {
		fprintf(stderr, "Caught SIGTERM on %s.\n", gl_who);
		sleep(1);
		exit(20);
	}
	else if (signo == SIGHUP || signo == SIGINT) {
		fprintf(stderr, "Caught SIGINT on %s.\n", gl_who);
		sleep(1);
		exit(21);
	}
	else if (signo == SIGPIPE) {
		fprintf(stderr, "Caught SIGPIPE on %s.\n", gl_who);
		if (strcmp(gl_who, "client") == 0) {
			exit(0);
		}
		g_sigpipe_ret = 0;
	} else {
		fprintf(stderr, "Caught signal %d on %s.\n", signo, gl_who);
		sleep(1);
		exit(23);
	}
}

static int
fdgo(int select_ret, int read_ret, char * location)
{
	if (select_ret > 0 && read_ret <= 0) {
		if (read_ret < 0) {
			/* 
			* Error 
			fprintf(stderr, "Uh-oh [read] read_ret at %s.\n", location);
			*/
		}
		/* 
		* EOF 
		*/
		return 0;
	} else if (select_ret < 0) {
		/* 
		* error 
		*/
		fprintf(stderr, "Uh-oh [select] ret at %s.\n", location);
		return 0;
	} else if (select_ret > 0 && read_ret > 0) {
		/* 
		* Good. Got data. 
		*/
		return 1;
	} else if (select_ret == 0 && read_ret == 0) {
		/* 
		* Nothing happening,  keep checking 
		*/
		return 1;
	} else {
		return 1;
	}
}

int
doWritefdSelectTest(int fd, char * location, int * fReturn, int verbose)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	FD_ZERO(&rfds);	
	FD_SET(fd, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);	
	FD_SET(fd, &rfds);

	retval = select(fd + 1, NULL, &rfds, NULL, &tv);
	if (verbose) {
		fprintf(stderr, "doWritefdSelectTest: [%s], select returned %d.\n", location, retval);
	}
	return retval;
}

int
doPumpCycle(int ofd, int ifd, char * location, int sec, int usec, int * readReturn, int verbose)
{
	fd_set rfds;
	struct timeval tv;
	int retval, ret = 0;
	int wret;
	char buf[PIPE_BUF];

	FD_ZERO(&rfds);	
	FD_SET(ifd, &rfds);

	tv.tv_sec = sec;
	tv.tv_usec = usec;
	FD_ZERO(&rfds);	
	FD_SET(ifd, &rfds);

	*readReturn = 0;
	retval = select(ifd + 1, &rfds, NULL, NULL, &tv);

	if (retval > 0) {
		ret = read(ifd, buf, sizeof(buf));
		*readReturn = ret;
		if (ret < 0)  {
			/* fprintf(stderr, "read error ret = %d location=[%s]\n", ret, location);	
			*/
		} else if (ret == 0) {
			if (verbose) fprintf(stderr, "GOT EOF read returned [%d] location=[%s] fd=[%d]\n", ret, location, ifd);	
		}
		if (ret > 0) {
			if (ofd >= 0 &&  (wret=write(ofd, buf, ret)) != ret) {
				fprintf(stderr, "write error ret=%d at location %s\n", wret, location);
			}
		}
	} else if (retval < 0) {
		fprintf(stderr, "select retval = %d location=[%s]\n", retval, location);	
	} else { 
		/*
		Nothing happening.
		*/
		retval = 0;
	}
	return retval;
}

static
int 
sumpPump(int ofd, int ifd, char * location, int sec, int usec, int verbose, int loopcount)
{
	int count = 0;
	int rret, selret, retval = 1;
	
	while(loopcount == 0 || count < loopcount) {
		selret = doPumpCycle(ofd, ifd, location, sec, usec, &rret, verbose);
		count ++;
		retval=fdgo(selret, rret, location);
		
		if (verbose) {
			fprintf(stderr, "sumpPump: fdgo(%d, %d, %s) returned %d\n", selret, rret, location, retval);
		}
		if (retval <= 0) {
			break;
		} else if (loopcount == 0 && retval > 0 && rret == 0) {
			retval = 0;
			break;
		} else {
			;
		}
	}	
	/*
	* 0 means finished.
	* 1 means not finished.
	* <0 means error.
	*/
	return retval;
}


int
swlib_pump_amount(int discharge_fd, int suction_fd, int amount)
{
	int i = amount;
	if (swlib__pipe_pump(suction_fd, discharge_fd, (int *) (NULL), (int *) (NULL), &i)) {
		return -1;
	}
	return i;
}

static int
swlib__pipe_pump(int suction_fd, int discharge_fd, int *status, int *childGotStatus, int *amount)
{
	int commandFailed = 0, pumpDead = 0, bytes, ibytes, byteswritten = 0;
	int remains;
	int c_amount = *amount;
	char buf[512];

	if (c_amount < 0) {
		do {
			bytes = read(suction_fd, buf, sizeof(buf));
			if (bytes < 0) {
				commandFailed = 1;	/* cavitation failure */
				pumpDead = 1;
			} else if (bytes == 0) {
				/* if it's not dead yet, it will be when we close the pipe */
				pumpDead = 1;
			} else {
				if (discharge_fd >= 0) {
					if ((bytes=write(discharge_fd, buf, bytes)) != bytes) {
						commandFailed = 2;	/* discharge failure */
						pumpDead = 1;
					}
				}
			}
			byteswritten += bytes;
		} while (!pumpDead);
		*amount = byteswritten;
		return commandFailed;
	} else {
		remains = sizeof(buf);
		if ((c_amount - byteswritten) < remains) {
			remains = c_amount - byteswritten;
		}
		do {
			bytes = read(suction_fd, buf, remains);
			if (bytes < 0) {
				commandFailed = 1;	/* cavitation failure */
				pumpDead = 1;
			} else if (bytes == 0) {
				/* if it's not dead yet, it will be when we close the pipe */
				pumpDead = 1;
			} else if (bytes) {

				if (discharge_fd >= 0) {
					ibytes = write(discharge_fd, buf, bytes);
				} else {
					ibytes = bytes;
				}

				if (ibytes != bytes) {
					commandFailed = 2;	/* discharge failure */
					pumpDead = 1;
				} else {
					byteswritten += bytes;
					if ((c_amount - byteswritten) < remains) {
						if ((remains = c_amount - byteswritten) > sizeof(buf)) {
							remains = sizeof(buf);
						}
					}
				}
			}
		} while (!pumpDead && remains);
		*amount = byteswritten;
		return commandFailed;
	}
	return -1;
}

int
swlib_pipe_pump(int ofd, int ifd)
{
	return 
	swlib_pump_amount(ofd, ifd, -1);
}

static
int
stdioPump(int stdoutfd, int outputpipefd, int inputpipefd, int stdinfd, int opt_verbose, int pid, int * waitret, int * statusp)
{
	int ret0, ret1;
	int retval = 0;
	int haveclose0 = 0;
	int haveclose1 = 0;
	int have_checked_for_epipe = 0;
	
	ret1 = sumpPump(stdoutfd, outputpipefd, "ret1", 0, 200, opt_verbose, 1);
	ret0 = sumpPump(inputpipefd, stdinfd, "ret0", 0, 200, opt_verbose, 1);
	while(ret0 > 0 || ret1 > 0) {
		if (ret1 > 0) {
			/* 
			* Inbound (relative to locahost) bytes.
			*/
			ret1 = sumpPump(stdoutfd, outputpipefd, "ret1", 0, 200, opt_verbose, 1);
		} else {
			if (haveclose1 == 0) { 
				if (opt_verbose) {
					fprintf(stderr, "closing (ret1) outputpipefd fd=%d\n", outputpipefd);
				}
				close(outputpipefd); 
				haveclose1 = 1; 
			}
			if (opt_verbose) {
				fprintf(stderr, "fd0=%d fd1=%d ret (ret1)finishvalue=%d\n", stdoutfd, outputpipefd, ret1);
			}
		}
		if (ret0 > 0) {
			/* 
			* Outbound bytes
			*/
			ret0 = sumpPump(inputpipefd, stdinfd, "ret0", 0, 200, opt_verbose, 1);
		} else {
			if (haveclose0 == 0) { 
				if (opt_verbose) {
					fprintf(stderr, "closing (ret0) inputpipefd fd=%d\n", inputpipefd);
				}
				close(inputpipefd); 
				haveclose0 = 1; 
			}
			if (opt_verbose) {
				fprintf(stderr, "fd2=%d fd3=%d ret (ret0)finishvalue=%d\n", inputpipefd, stdinfd, ret0);
			}
		}


		/*
		* If the incoming (relative to localhost) stream is finished, then
		* check the outbound descriptor for closure, but check this only once.
		*/
		if (ret1 <= 0 && ret0 > 0 && have_checked_for_epipe == 0) {
			int r_1;
			have_checked_for_epipe = 1;
			if (doWritefdSelectTest(inputpipefd, "closetest0", &r_1, opt_verbose)) {
				/*
				* This could mean one of three things:
				*    1) A non-blocking write is guaranteed not to block.
				*    2) A error has occurred.
				*    3) The descriptor is closed, and any write will generate SIGPIPE.
				*/
				
				/*
				* Since if we are here, then the inbound byte stream has shutdown, hence
				* it probably OK to shutdown the local outbound stream (e.g from the terminal stdin)
				*/ 

				/*
				* Wrong, Testing  for EPIPE by making a zero length write is non-portable. 
				**
				** Test for EPIPE.
				**
				*if (write(inputpipefd, "", 0) < 0) {
				*	if (opt_verbose) {
				*		fprintf(stderr, "outgoing write error: %s\n", strerror(errno));
				*	}
				*	if (errno != EPIPE) {
				*		fprintf(stderr, "expected EPIPE but got: %s\n", strerror(errno));
				*		ret0 = -1;
				*	} else {
				*		*
				*		* Got EPIPE, Good. This is normal and now it
				*		* is definetly time to shutdown.
				*		*
				*		ret0 = 0;
				*	}
				*} else {
				*	fprintf(stderr, "error: write(2) succeeded, Huh? continuing..\n");
				*	ret0 = 1;
				*}
				*/
			
				/*
				* Wait on pid.
				*/
				if (pid > 0 && waitret && statusp) {
					*waitret = waitpid(pid, statusp, WNOHANG);
					if (*waitret > 0) {
						/*
						* Good stop.
						*/
						ret0 = 0;
					} else {
						fprintf(stderr, "expected a child status: %s\n", strerror(errno));
						fprintf(stderr, "may require SIGINT or a EOF to terminate.\n");
						ret0 = -1;
					}
				} else {
					ret0 = -1;
				}
			} else {
				/*
				* Select said no event had occured on the descriptor
				*/
				ret0 = 1;	/* continue. */
			}
		}
	}

	if (ret0 < 0) fprintf(stderr, "stdioPump ERROR on ret0\n");
	if (ret1 < 0) fprintf(stderr, "stdioPump ERROR on ret1\n");

	if (opt_verbose) {
		fprintf(stderr, "fd0=%d fd1=%d ret1 finishvalue=%d\n", stdoutfd, outputpipefd, ret1);
		fprintf(stderr, "fd2=%d fd3=%d ret0 finishvalue=%d\n", inputpipefd, stdinfd, ret0);
	}

	if (haveclose0 == 0) { 
		if (opt_verbose) {
			fprintf(stderr, "closing2 (ret0) inputpipefd fd=%d\n", inputpipefd);
		}
		close(inputpipefd); 
		haveclose0 = 1; 
	}

	if (haveclose1 == 0) { 
		if (opt_verbose) {
			fprintf(stderr, "closing2 (ret1) outputpipefd fd=%d\n", outputpipefd);
		}
		close(outputpipefd); 
		haveclose1 = 1; 
	}
	return retval;
}

