/* swgp.c  --  General purpose routines.

   Copyright (C) 2003-2004 James H. Lowe, Jr. <jhlowe@acm.org>
   All Rights Reserved.
 
   COPYING TERMS AND CONDITIONS
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
/* #include <sys/wait.h> */
/* #include <utime.h> */

#ifdef  HAVE_SYSSELECT_H 
#include <sys/select.h>
#endif

#include "swgp.h"
#include "strob.h"
#include "shcmd.h"
#include "uxfio.h"
#include "strob.h"
#include "uxfio.h"
#include "swutilname.h"
#include "atomicio.h"

extern char ** environ;

static int Gverbose;
static int Gbe_silent;

#define SWGP_USEC 20

static
int
fdgo(   int select_ret, 
	int read_ret, 
	char * location, 
	int Gbe_silent)
{
	if (select_ret > 0 && read_ret <= 0) {
		if (read_ret < 0) {
			/* 
			 * Error 
			 */
			if (Gbe_silent < 1)
				fprintf(stderr, "%s: fdgo(): read error, loc=%s, errno=%d: %s\n",
					swlib_utilname_get(), location, errno, strerror(errno));
			return -1;
		} else {
			/* 
			 * EOF 
			 */
			return 0;
		}
	} else if (select_ret < 0) {
		/* 
		 * error 
		 */
		if (Gbe_silent < 1)
			fprintf(stderr, "%s: fdgo(): select error, loc=%s, errno=%d: %s\n",
				swlib_utilname_get(), location, errno, strerror(errno));
		return -1;
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

static
ssize_t
read_line(int fd, void * buf, size_t len)
{
	return swgp_read_line(fd, (STROB*)buf, DO_NOT_APPEND);
}

static
void
read_pipe(int ofd, int ifd, void * buf, int *readReturn,
		char * location, uintmax_t * statbytes,
		ssize_t (*wf)(int, void *, size_t),
		ssize_t (*rf)(int, void *, size_t)
		)
{
	int ret;
	int wret;

	ret = (*rf)(ifd, buf, SWLIB_PIPE_BUF);
	if (readReturn) *readReturn = ret;
	if (ret < 0)  {
		if (!Gbe_silent) fprintf(stderr, 
			"read error ret = %d location=[%s]\n",
			ret, location);	
	} else if (ret == 0) {
		if (Gverbose) 
			fprintf(stderr, 
			"GOT EOF read returned [%d] location=[%s]"
				" fd=[%d]\n", 
				ret, location, ifd);	
	} else if (ret > 0) {
		if (ofd >= 0) {
			if  ((wret=(*wf)(ofd, buf, ret)) != ret) {
				if (!Gbe_silent)
				fprintf(stderr, 
				"write error ret=%d at location %s\n",
					wret, location);
				if (readReturn) *readReturn = wret;
			} else {
				if (statbytes) (*statbytes) += wret;
			}
		} 
	}
}

static
int
doExhaustFd_i(
	void * buf,
	int fd_from,
	char * location, 
	int sec, 
	int usec, 
	int * readReturn,
	ssize_t (*f_read)(int, void *, size_t))
{
	fd_set rfds;
	fd_set efds;
	struct timeval tv;
	int retval;
	int maxfd = fd_from;

	FD_ZERO(&rfds);	
	FD_ZERO(&efds);	
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	FD_ZERO(&rfds);	
	FD_ZERO(&efds);	
	if (fd_from > 0) FD_SET(fd_from, &rfds);
	if (fd_from > 0) FD_SET(fd_from, &efds);
	*readReturn = 0;
	retval = select(maxfd + 1, &rfds, NULL, &efds, &tv);

	if (retval > 0) {
		if (FD_ISSET(fd_from, &efds)) {
			*readReturn = -1;
			return -1;
		}
		if (FD_ISSET(fd_from, &rfds)) {
			if (fd_from > 0)
				read_pipe(-1, fd_from, (void*)buf,
					readReturn, "ret2", NULL,
					(ssize_t(*)(int, void *, size_t))(NULL),
					(ssize_t(*)(int, void *, size_t))(f_read)
					);
		}
	} else if (retval < 0 && errno == EINTR) {
		if (Gverbose > 6) {
		fprintf(stderr, "%s: select: (errno=EINTR): %s\n",
				swlib_utilname_get(), strerror(errno));
		fprintf(stderr, 
			"%s: %s:%d select retval = %d fd_from=%d"
			" location=[%d] : %s\n", swlib_utilname_get(), __FILE__, __LINE__,
				retval, fd_from, __LINE__, 
				strerror(errno));
		}
		retval = 0;
	} else if (retval < 0 && errno != EINTR) {
		fprintf(stderr, "%s: select error (errno=%d): %s\n",
				swlib_utilname_get(), (int)errno, strerror(errno));
		fprintf(stderr, 
			"%s: %s:%d select retval = %d fd_from=%d"
			" location=[%d] : %s\n", swlib_utilname_get(), __FILE__, __LINE__,
				retval, fd_from, __LINE__, 
				strerror(errno));	
	} else { 
		/*
		* Nothing happening.
		*/
		retval = 0;
	}
	return retval;
}

static
int
doPumpCycle(
	char * buf,
	int ofd, 
	int ifd,
	char * location, 
	int sec, 
	int usec, 
	int * readReturn, 
	uintmax_t * statbytes)
{
	fd_set rfds;
	struct timeval tv;
	int retval;
	int maxfd = ifd;

	FD_ZERO(&rfds);	
	FD_SET(ifd, &rfds);
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	FD_ZERO(&rfds);	
	FD_SET(ifd, &rfds);
	*readReturn = 0;
	retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);

	if (retval > 0) {
		retval = 1;
		if (FD_ISSET(ifd, &rfds)) {
			read_pipe(ofd, ifd, (void*)buf,
				readReturn, location, statbytes,
				(ssize_t(*)(int, void *, size_t))(uxfio_unix_safe_write),
				(ssize_t(*)(int, void *, size_t))(uxfio_unix_safe_read)
				);
		} 
	} else if (retval < 0 && errno != EINTR) {
		/* if (!Gbe_silent)  */
			fprintf(stderr, 
				"select retval = %d ofd=%d ifd=%d"
				" location=[%d] : %s\n", 
					retval, ofd, ifd, __LINE__, 
					strerror(errno));	
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
sumpPump(
	char * buf,
	int ofd, 
	int ifd, 
	char * location, 
	int sec, 
	int usec, 
	int loopcount, 
	uintmax_t * statbytes)
{
	int count = 0;
	int rret, selret, retval = 1;
	
	while(loopcount == 0 || count < loopcount) {
		selret = doPumpCycle(buf, ofd, ifd, 
				location, sec, usec, &rret, statbytes);
		count ++;
		retval=fdgo(selret, rret, location, Gbe_silent);
		
		if (Gverbose) {
			fprintf(stderr, 
			"swssh: sumpPump: fdgo(%d, %d, %s) returned %d\n", 
				selret, rret, location, retval);
		}
		if (retval < 0) {
			if (Gbe_silent < 1)
				fprintf(stderr, "sumpPump internal error: loc=1\n");
			break;
		} else if (retval == 0) {
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
swgp_stdioPump(int stdoutfd,      /* local stdout */
		int outputpipefd, /* output from remote host */
		int inputpipefd,  /* input to remote host */
		int stdinfd,      /* local stdin */ 
		int opt_verbose, 
		int opt_be_silent, 
		int pid, 
		int * waitret, 
		int * statusp, 
		int * cancel,
		uintmax_t * statbytes)
{
	int ret0, ret1;
	int retval = 0;
	int haveclose0 = 0;
	int haveclose1 = 0;
	int have_checked_for_epipe = 0;
	char buf[SWLIB_PIPE_BUF];

	Gverbose = opt_verbose;
	Gbe_silent = opt_be_silent;
	
	if (stdoutfd >= 0)  {
		ret1 = sumpPump(buf, stdoutfd, outputpipefd, 
				"ret1", 0, SWGP_USEC, 1, statbytes); 
	} else {
		ret1 = 0;
	}
	if (ret1 < 0) retval--;

	ret0 = sumpPump(buf, inputpipefd, stdinfd,
			"ret0", 0, SWGP_USEC, 1, statbytes);
	while(
		(ret0 > 0 || ret1 > 0) && 
		(cancel == (int*)NULL || *cancel == (int)0)
	) {
		if (ret1 > 0) {
			/* 
			* Inbound (relative to locahost) bytes.
			*/
			ret1 = sumpPump(buf, stdoutfd, outputpipefd,
					"ret1", 0, SWGP_USEC, 1, statbytes);
		} else {
			if (ret1 < 0) retval--;
			if (haveclose1 == 0) { 
				if (opt_verbose) {
					fprintf(stderr,
					"closing (ret1) outputpipefd fd=%d\n",
					outputpipefd);
				}
				close(outputpipefd); 
				haveclose1 = 1; 
			}
			if (opt_verbose) {
				fprintf(stderr, 
					"fd0=%d fd1=%d ret"
					" (ret1)finishvalue=%d loc=d\n", 
					stdoutfd, outputpipefd, ret1);
			}
		}
		if (ret0 > 0) {
			/* 
			* Outbound bytes
			*/
			ret0 = sumpPump(buf, inputpipefd, stdinfd,
					"ret0", 0, SWGP_USEC, 1, statbytes);
		} else {
			if (ret0 < 0) retval--;
			if (haveclose0 == 0) { 
				if (opt_verbose) {
					fprintf(stderr,
					"closing (ret0) inputpipefd fd=%d\n",
					inputpipefd);
				}
				close(inputpipefd); 
				haveclose0 = 1; 
			}
			if (opt_verbose) {
				fprintf(stderr, 
					"fd2=%d fd3=%d ret"
					" (ret0)finishvalue=%d loc=e\n",
						inputpipefd, stdinfd, ret0);
			}
		}

		/*
		* If the incoming (relative to localhost) stream is 
		* finished, then check the outbound descriptor
		* for closure, but check this only once.
		*/
		if (ret1 <= 0 && ret0 > 0 && have_checked_for_epipe == 0) {
			have_checked_for_epipe = 1;

			if (stdinfd != STDIN_FILENO) close(stdinfd);

			if (1 
				/* || doWritefdSelectTest(inputpipefd, 
					"closetest0", 
					&r_1, 
					opt_verbose) */
			) {
				if (pid > 0 && waitret && statusp) {
					if (opt_verbose)  
						fprintf(stderr, 
					"CHECKING pid %d in swgp_stdioPump\n",
							(int)pid);
					*waitret = waitpid(pid, 
							statusp,
							WNOHANG);
					if (*waitret > 0) {
						/*
						* Good stop.
						*/
						if (opt_verbose)
						fprintf(stderr, "Good stop.\n");
						ret0 = 0;
					} else if (*waitret == 0) {
						ret0 = 0;
					} else {
						fprintf(stderr,
						"swbis: expected a child"
						" status: %s\n",
						strerror(errno));
						fprintf(stderr,
						"swbis: may require SIGINT or"
						" a EOF to terminate.\n");
						ret0 = -1;
					}
				} else {
					ret0 = -1;
				}
			} else {
				ret0 = 1;	/* continue. */
			}
		}
	}

	if (cancel != (int*)NULL && *cancel != (int)0) { 
		return 0;
	}

	if (ret0 < 0) {
		/*
		* Mask this message on BSD systems.
		*/
		if (opt_be_silent == 0) {
			fprintf(stderr,
				"swssh: stdioPump ERROR on ret0\n");
		}
	}

	if (ret1 < 0) {
		if (Gbe_silent < 1)
			fprintf(stderr, "swssh: stdioPump ERROR on ret1\n");
	}

	if (opt_verbose) {
		fprintf(stderr, "fd0=%d fd1=%d ret1 finishvalue=%d loc=f\n",
						stdoutfd, outputpipefd, ret1);
		fprintf(stderr, "fd2=%d fd3=%d ret0 finishvalue=%d loc=g\n",
						inputpipefd, stdinfd, ret0);
	}

	if (haveclose0 == 0) { 
		if (opt_verbose) {
			fprintf(stderr, "closing2 (ret0) inputpipefd fd=%d\n",
								inputpipefd);
		}
		close(inputpipefd); 
		haveclose0 = 1; 
	}

	if (haveclose1 == 0) { 
		if (opt_verbose) {
			fprintf(stderr, "closing2 (ret1) outputpipefd fd=%d\n",
								 outputpipefd);
		}
		close(outputpipefd); 
		haveclose1 = 1; 
	}
	
	if (stdoutfd != STDOUT_FILENO && stdoutfd >= 0) close(stdoutfd);
	if (retval) {
		fprintf(stderr, "got error darnit in stdioPump\n");
	}
	return retval;
}

Sigfunc *
swgp_signal(int signo, Sigfunc * func)
{
        struct sigaction act, oact;

        act.sa_handler = func;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
                act.sa_flags |= SA_INTERRUPT;   /* SUn OS */
#endif
        } else {
#ifdef SA_RESTART
                act.sa_flags |= SA_RESTART;
#endif
        }
        if (sigaction(signo, &act, &oact) < 0)
                return(SIG_ERR);
        return(oact.sa_handler);
}

int
swgp_signal_block(int signo, sigset_t * oset)
{
	int ret;
	sigset_t newmask;

	sigemptyset(&newmask);
	sigaddset(&newmask, signo);
	
	if ((ret=sigprocmask(SIG_BLOCK, &newmask, oset)) < 0)
		fprintf(stderr, 
			"swgp_signal_block error, signal=%d: %s\n",
				signo, strerror(errno));

	return ret;
}

int
swgp_signal_unblock(int signo, sigset_t * oset)
{
	int ret;
	sigset_t newmask;

	sigemptyset(&newmask);
	sigaddset(&newmask, signo);
	if ((ret=sigprocmask(SIG_UNBLOCK, &newmask, oset)) < 0)
			fprintf(stderr, 
			"swgp_signal_block error, signal=%d: %s\n", 
				signo, strerror(errno));
	return ret;		
}

void swgp_sig_handler_ignore(int signo){
	return;
}

void swgp_sig_handler_exit(int signo)
{
	exit(0);
}

int
swgp_write_as_echo_line(int fd, char * buf)
{
	int ret;
	int len;
	STROB * btmp;

	btmp = strob_open(90);
	strob_sprintf(btmp, 0, "%s\n", buf);
	len = (int)strob_strlen(btmp);
	ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write,
			fd, strob_str(btmp), len);
	strob_close(btmp);
	if (ret != len) return -1;
	return 0;
}

int
swgp_read_line(int fd, STROB * buf, int do_append)
{
	int n;
	char c[2];
	int count = 0;

	c[1] = '\0';
	if (do_append == 0) {
		strob_strcpy((STROB*)buf, "");
	} else {
		;
	}	
	n = uxfio_unix_safe_read(fd, (void*)&c, 1);
	while (n == 1 && c[0] != '\n') {
		strob_strcat((STROB*)buf, c);
		count ++;
		n = uxfio_unix_safe_read(fd, (void*)&c, 1);
	}
	if (n == 1 && c[0] == '\n') {
		count ++;
		strob_strcat((STROB*)buf, c);
	} else if (n < 0) {
		return -1;
	}
	return count;
}

int
swgp_find_lowest_fd(int start)
{
	int ret;
	ret = dup(0);
	if (ret < 0) {
		fprintf(stderr,
		"%s: %s\n",
		swlib_utilname_get(),
		strerror(errno)
		);
		exit(1);
	}
	close(ret);
	return ret;
}

void
swgp_check_fds(void)
{
	int open_max;
	int tmpfd;
	int max_fd;

	open_max = swgp_close_all_fd(-1);
	/*
	 * now make sure sysconf aint lying
	 */
	tmpfd = dup(STDIN_FILENO);
	if (tmpfd < 0) {
		fprintf(stderr, "%s: %s\n", swlib_utilname_get(), strerror(errno));
		fprintf(stderr, "%s: available file descriptors look suspicious, exiting now\n", swlib_utilname_get());
		exit(1);
	}
	max_fd = dup2(tmpfd, open_max - 1);
	close(tmpfd);
	if (max_fd < 0) {
		fprintf(stderr, "%s: %s\n", swlib_utilname_get(), strerror(errno));
		fprintf(stderr, "%s: available file descriptors look suspicious, exiting now\n", swlib_utilname_get());
		exit(1);
	}
	close(max_fd);
	if (max_fd - 3 < SWBIS_MIN_FD_AVAIL ) {
		fprintf(stderr, "%s: max fd available is %d\n", swlib_utilname_get(), max_fd);
		fprintf(stderr, "%s: available file descriptors look suspicious, exiting now\n", swlib_utilname_get());
		exit(1);
	}
	return;
}

int
swgp_close_all_fd(int start)
{
	int i;
	int open_max;
	open_max = sysconf(_SC_OPEN_MAX);
	if (open_max < 0) {
		if (errno != 0)
			fprintf(stderr, 
			"%s: sysconf: open_max not determined.\n",
				swlib_utilname_get());
		open_max = SWBIS_SC_OPEN_MAX; 
	} else {
		;
		/*
		 * use what sysconf says
		 */
	}
	if (open_max < 16) {
		fprintf(stderr,
		"%s: the maximum number of open file descriptors\n"
		"%s: was determined to be %d.\n"
		"%s: swbis requires more than this for its checks\n"
		"%s: against file descriptor leaks, and its normal\n"
		"%s: operations.\n"
		"%s"
		,
		swlib_utilname_get(),
		swlib_utilname_get(), open_max,
		swlib_utilname_get(),
		swlib_utilname_get(),
		swlib_utilname_get(),
		""
		);
		exit(1);
	}
	if (start >= (open_max -3 /* 3 is arbitrary buffer*/)) {
		/*
		 * This may happen during a fd leakage attack
		 */
		fprintf(stderr,
		"%s: Oops, something is using all the file descriptors\n"
		"%s: swbis chooses not to continue\n"
		"%s",
		swlib_utilname_get(),
		swlib_utilname_get(),
		""
		);
		exit(1);
	}
	if (start >= 0) {
		for (i=start; i < open_max; i++) close(i);
	} else {
		/*
		 * support dual use for just finding the 
		 * max_open value.
		 */
	}
	return open_max;
}

int
swgpReadFdNonblock(char * pipe_buf, int fd_from, int * len)
{
	int ret;
	*len = 0;
	*pipe_buf = '\0';
	ret = doExhaustFd_i((void*)pipe_buf, fd_from,
		"ret2", 0, 1000, len,
		(ssize_t(*)(int, void *, size_t))(uxfio_unix_safe_read));
	return ret;
}

int
swgpReadLine(STROB * buf, int fd_from, int * readReturn)
{
	int ret;
	ret = doExhaustFd_i((void*)buf, fd_from,
		"ReadLine", 0, 40000, readReturn,
		(ssize_t(*)(int, void *, size_t))(read_line));
	return ret;
}
