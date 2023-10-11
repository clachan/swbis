#ifndef swgp_h_20031006
#define swgp_h_20031006

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include "swlib.h"

#define DO_APPEND 1
#define DO_NOT_APPEND 0

typedef void Sigfunc(int);
/* Sigfunc *signal(int, Sigfunc *); */
typedef RETSIGTYPE (*SIGHANDLERTYPE) (int);

void 	swgp_sig_handler(int signo);

/* int	swgp_sigaction(int signo, Sigfunc * func, sigset_t * blocked_set); */
Sigfunc * swgp_signal(int signo, Sigfunc * func);
int       swgp_signal_block(int signo, sigset_t * oldmask);
int       swgp_signal_unblock(int signo, sigset_t * mask);
int 
swgp_stdioPump(int stdoutfd, 
		int outputpipefd, 
		int inputpipefd, 
		int stdinfd, 
		int opt_verbose, 
		int be_silent, 
		int pid, 
		int * waitret, 
		int * statusp, 
		int * cancel,
		uintmax_t * statbytes);
int swgp_write_as_echo_line(int fd, char * buf);
int swgp_read_line(int fd, STROB * buf, int do_append);
int swgp_close_all_fd(int start);
int swgpReadFdNonblock(char * pipe_buf, int efd_from, int * len);
int swgpReadLine(STROB * buf, int fd_from, int * readReturn);
void swgp_check_fds(void);

/*
ssize_t swgp_eread(int fd, void *, size_t);
ssize_t swgp_ewrite(int fd, void *, size_t);
*/

#endif
