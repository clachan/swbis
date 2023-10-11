/* swfork.c  --  specialized routines for fork'ing

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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <grp.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif
#include <utmp.h>
#include <sys/types.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include "swfork.h"
#include "swgp.h"
#include "swlib.h"
#include "swutillib.h"
#include "swutilname.h"
        			

static
int
my_openpty(int * p_fdm, int * p_fds)
{
#ifdef HAVE_OPENPTY
	return openpty(p_fdm, p_fds, NULL, NULL, NULL);
#else
	return -1;
#endif
}

static
void
release_tty_name(char * slave_pty_name, int be_verbose)
{
	if (chown(slave_pty_name, (uid_t)(0), (gid_t)(0)) < 0)
		if (be_verbose)
			fprintf(stderr, "%s: chown %.100s 0 0 failed: %.100s",
				swlib_utilname_get(),
				slave_pty_name,
				strerror(errno));
	if (chmod(slave_pty_name, (mode_t)0666) < 0)
		if (be_verbose)
			fprintf(stderr, "%s: chmod %.100s 0666 failed: %.100s",
				swlib_utilname_get(),
				slave_pty_name,
				strerror(errno));
}


static
int
swlib_ptym_open(char *pts_name, int * start)
{
	int		fdm;
	char	*ptr1, *ptr2;
	char ptr2char[]="0123456789abcdef";
	char * ptr2start;

	if (*start < 0 || (unsigned int)(*start) >= strlen(ptr2char)) return -1;

	ptr2start = ptr2char + (*start);

	strcpy(pts_name, "/dev/ptyXY");
	  /* array index: 0123456789 (for references in following code) */
	for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
		pts_name[8] = *ptr1;
		for (ptr2 = ptr2start; *ptr2 != 0; ptr2++) {
		
			if (swlib_get_verbose_level() >= SWC_VERBOSE_7) {
				fprintf(stderr, "%s: ptr2start is [%s]\n",
					swlib_utilname_get(), ptr2start);
			}
	
			pts_name[9] = *ptr2;
						/* try to open master */
			if ( (fdm = open(pts_name, O_RDWR)) < 0) {
				if (errno == ENOENT) {
					return(-1);
				} else {
					(*start)++;
					if (swlib_get_verbose_level() >= SWC_VERBOSE_7) 
						fprintf(stderr,
						"%s: open attempt failed on pty (master) : %s\n",
						swlib_utilname_get(), pts_name);
					continue;
				}
			}

			if (swlib_get_verbose_level() >= SWC_VERBOSE_7) {
				fprintf(stderr, "%s: Using pty (master) : %s\n",
					swlib_utilname_get(), pts_name);
			}
			pts_name[5] = 't'; /* change "pty" to "tty" */
			return(fdm);	   /* got it, return fd of master */
		}
	}
	/* 
	* out of pty devices 
	*/
	return(-1);
}

static
int
swlib_ptys_open(/*int fdm,*/ char *pts_name)
{
	struct passwd	*pw;
	struct group	*grp;
        mode_t mode;
	uid_t	uid;
	gid_t	gid;
	int	fds;

	if ( (fds = open(pts_name, O_RDWR)) < 0) {
		fprintf(stderr, 
			"%s: swlib_ptys_open: open failed on %s : %s\n", 
					swlib_utilname_get(),
					pts_name,
					strerror(errno));
		return(-1);
	}

	if ((pw = getpwuid(getuid())) == NULL) {
		/*
		 * fatal
		 */
		SWLIB_FATAL("passwd entry lookup failed for a uid");
		return -1;
	}

	if ( (grp = getgrnam("tty")) != NULL) {
		gid = grp->gr_gid;
		mode = S_IRUSR | S_IWUSR | S_IWGRP;
	} else {
		gid = pw->pw_gid;
		mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH;
	}

	uid = pw->pw_uid;

	/* following two functions don't work unless we're root */
	if (chown(pts_name, uid, gid) < 0) {
		if (uid == 0) {
			SWLIB_FATAL("chown failed for slave pty");
			return -1;
		} else {
			fprintf(stderr, 
			"%s: warning: using pty %s with insecure ownership\n", 
					swlib_utilname_get(),
					pts_name);
		}
	}

	if (chmod(pts_name, mode) < 0) {
		if (uid == 0) {
			SWLIB_FATAL("chmod failed for slave pty");
			return -1;
		} else {
			fprintf(stderr, 
			"%s: warning: using pty %s with insecure permissions\n", 
					swlib_utilname_get(),
					pts_name);
		}
	}

	return(fds);
}

pid_t
swlib_pty_fork2(int *ptrfdm, int ofd, int ifd, int efd, 
		 const struct termios *slave_termios,
		 const struct winsize *slave_winsize, 
		int * pc_pid, 
		int make_raw, sigset_t * sigmask)
{
	int	fdm, fds;
	pid_t	pid;
	pid_t	c_pid;
	char	pts_name[20];
	char *  tmpc;
	char * pts_type;
	char    slave_pty_name[50];
	int     op[2];
	int     ip[2];
	int     pcpipe[2];
	int 	testfds = -1;
	int	start = 0;
	int have_openpty = 0;
	int have_posix_openpt = 0;
	int	ret;

#ifdef HAVE_POSIX_OPENPT
	have_posix_openpt = 1;
#else
	have_posix_openpt = 0;
#endif

#ifdef HAVE_OPENPTY
	have_openpty = 1;
#else
	have_openpty = 0;
#endif

	slave_pty_name[0] = '\0';

	if (have_posix_openpt) {
		E_DEBUG("");
		pts_type = "posix_openpt";
		ret = posix_openpt(O_RDWR|O_NOCTTY);
		if (ret < 0) {
			fprintf(stderr, "%s: posix_openpt failed: %s\n",
				swlib_utilname_get(), strerror(errno));
			return -1;
		}
		fdm = ret;
		E_DEBUG2("fdm=%d", fdm);
		tmpc = ptsname(fdm);
		if (tmpc == NULL) {
			fprintf(stderr, "%s: ptsname failed\n", swlib_utilname_get());
			return -1;
		}
		E_DEBUG2("ptsname=%s", tmpc);
		strncpy(slave_pty_name, tmpc, sizeof(slave_pty_name)-1);
		slave_pty_name[sizeof(slave_pty_name)-1] = '\0';
	} else if (have_openpty) {
		/* SVR4 acquires controlling terminal on open() */
		pts_type = "openpty";
		swlib_doif_writef(swlib_get_verbose_level(),  SWC_VERBOSE_7,
			(struct sw_logspec *)(NULL), STDERR_FILENO,
                               "using system openpty()\n");
		ret = my_openpty(&fdm, &fds);
		if (ret < 0) {
			fprintf(stderr, "%s: openpty failed\n",
				swlib_utilname_get());
			return -1;
		}
		tmpc = ttyname(fds);		
		if (tmpc == NULL) {
			fprintf(stderr, "%s: ttyname failed\n",
				swlib_utilname_get());
			return -1;
		}
		strncpy(slave_pty_name, tmpc, sizeof(slave_pty_name)-1);
		slave_pty_name[sizeof(slave_pty_name)-1] = '\0';
		swlib_doif_writef(swlib_get_verbose_level(),  SWC_VERBOSE_7,
			(struct sw_logspec *)(NULL), STDERR_FILENO,
                               "opened master pty [%s] using openpty\n", ttyname(fdm));
		swlib_doif_writef(swlib_get_verbose_level(),  SWC_VERBOSE_7,
			(struct sw_logspec *)(NULL), STDERR_FILENO,
                               "opened slave pty [%s] using openpty\n", slave_pty_name);
	} else {
		/*
		 * This is for systems that don't have openpty()
		 */

		pts_type = "SVR4";
		while(testfds < 0) {
			if (swlib_get_verbose_level() >= SWC_VERBOSE_7) {
				fprintf(stderr, "%s: pty name search index: %d\n", 
					swlib_utilname_get(), start);
			}
			if ( (fdm = swlib_ptym_open(pts_name, &start)) < 0) {
				fprintf(stderr, "can't open master pty: %s\n", pts_name);
				fprintf(stderr, "fatal: out of ptys\n");
				return -1;
			}
	
			if ( (testfds = swlib_ptys_open(/*fdm,*/ pts_name)) < 0) {
				close(fdm);
				if (swlib_get_verbose_level() >= SWC_VERBOSE_7) {
					fprintf(stderr, "%s: can't open slave pty %s\n", 
						swlib_utilname_get(), pts_name);
				}
			}
			if (testfds >= 0) close(testfds);
			start++;
		}
	}

	fprintf(stderr, 
		"%s: @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
		"%s: Warning: this program's usage of pty's (%s) may be insecure.\n"
		"%s: This feature of swcopy is meant for testing only.\n"
		"%s: @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" ,
		swlib_utilname_get(), swlib_utilname_get(), pts_type, swlib_utilname_get(), swlib_utilname_get());

	if (ifd < 0) ifd = STDIN_FILENO;
	if (ofd < 0) ofd = STDOUT_FILENO;
	if (efd < 0) efd = STDERR_FILENO;

	pipe(op);
	pipe(ip);
	pipe(pcpipe);
	if ( (pid = swndfork(sigmask, NULL)) < 0) {
		return(-1);
	} else if (pid == 0) {
		if (setsid() < 0) {
			fprintf(stderr, "setsid error");
		}
		if (have_posix_openpt) {
			if (grantpt(fdm) < 0 || unlockpt(fdm) < 0) {
				fprintf(stderr, "%s: pts gramtpt or unlockpt failed\n", swlib_utilname_get());
				return -1;
			}
			fds = open(slave_pty_name, O_RDWR);
			if (fds < 0) {
				fprintf(stderr, "%s: open failed on %s, reason: %s\n",
					swlib_utilname_get(), slave_pty_name, strerror(errno));

				close(fdm);
				return -1;
			}
			E_DEBUG2("fds=%d", fds);
		} else if (have_openpty) {
			/*
			 */

		} else {
			/*
			 * This is for systems that don't have openpty()
			 */
			if ( (fds = swlib_ptys_open(/*fdm,*/ pts_name)) < 0) {
				close(fdm);
				fprintf(stderr, "can't open slave pty %s : %s\n",
						pts_name,
						strerror(errno));
			}

			if (fds >= 0) {
				swlib_doif_writef(swlib_get_verbose_level(),  SWC_VERBOSE_7,
					(struct sw_logspec *)(NULL), STDERR_FILENO,
       		                         "Using pty (slave) : %s\n", pts_name);
			}
		}
#if	defined(TIOCSCTTY) && !defined(CIBAUD)
				/* 44BSD way to acquire controlling terminal */
				/* !CIBAUD to avoid doing this under SunOS */
		E_DEBUG("calling  TIOCSCTTY on fds");
		if (ioctl(fds, TIOCSCTTY, (char *) 0) < 0) {
			fprintf(stderr, 
				"TIOCSCTTY error: %s: %s\n",
						pts_name, 
						strerror(errno));
		}
#endif
		E_DEBUG("HERE 4");
		if (slave_termios != NULL) {
			if (tcsetattr(fds, TCSANOW, slave_termios) < 0)
				fprintf(stderr, 
					"tcsetattr error on slave pty : %s: %s\n",
						pts_name, 
						strerror(errno));
		}
		E_DEBUG("HERE 5");
		if (slave_winsize != NULL) {
			if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0)
				fprintf(stderr, 
					"TIOCSWINSZ error on slave pty %s: %s\n",
						pts_name,
						strerror(errno));
		}
		E_DEBUG("HERE 6");

		c_pid = swndfork(sigmask, NULL);
		if (c_pid < 0) {
			fprintf(stderr, "fork error\n");
			exit(1);
		} else if (c_pid > 0) {
			close(fdm);	/* all done with master in child */
			close(op[0]);
			close(op[1]);
			close(ip[0]);
			close(ip[1]);
			close(pcpipe[0]);
			if (uxfio_unix_safe_write(pcpipe[1], (void*)(&c_pid), 
					sizeof(c_pid)) != sizeof(c_pid)) {
				fprintf(stderr, 
				"write error : %s \n", strerror(errno));
			}
			close(pcpipe[1]);
			E_DEBUG("at parent");
			if (dup2(fds, 0) != 0)
				fprintf(stderr, "dup2 error to ifd = %d", ifd);
			if (dup2(fds, 1) != 1)
				fprintf(stderr, "dup2 error to ofd = %d", ofd);
			if (dup2(efd, 2) != 2)
				fprintf(stderr, "dup2 error to efd = %d", efd);
			swgp_close_all_fd(3); 
			return(0);	/* child returns 0 just like fork() */
		} else if (c_pid == 0) {
			int ret;
			uintmax_t sb;
			close(pcpipe[0]);
			close(pcpipe[1]);
			close(fds);
			close(op[0]);
			close(ip[1]);
			if (make_raw)

			if (swlib_tty_raw(fdm) < 0) {
				fprintf(stderr, 
				"tty_raw error : source_fdar[0]\n");
				exit(2);
			}
			E_DEBUG("at swgp_stdioPump");
			ret = swgp_stdioPump(
				op[1], /* stdout */
				fdm,   /* output from remote host */
				fdm,   /* input to remote host */
				ip[0], /* stdin */
				0 /* verbose */,
				1 /* be_silent */,
				0, NULL, NULL, NULL, &sb);
			close(fdm);
			close(op[1]);
			close(ip[0]);
			if (have_openpty) {
				/*
				 * reset permissions on the tty
				 */
				/* release_tty_name(slave_pty_name, 0); Not Required ?? */
			}
			_exit(ret);
		}
	} else {
		close(pcpipe[1]);
		if (read(pcpipe[0], (void*)(pc_pid), sizeof(c_pid)) != 
				sizeof(c_pid)) {
			fprintf(stderr, 
				"read error : %s \n", strerror(errno));
		}
		close(pcpipe[0]);
		close(fdm);
		close(op[1]);
		close(ip[0]);
		ptrfdm[0] = op[0];	
		ptrfdm[1] = ip[1];
		ptrfdm[2] = efd;
		return(pid);
	}
	/*
	* Should never get here
	*/
	return -1;
}

pid_t
swlib_no_pty_fork(int *ptrfdm, int ofd, int ifd, int efd, sigset_t * sigmask)
{
	int	opipe[2];
	int	ipipe[2];
	pid_t	pid;

	pipe(opipe);
	pipe(ipipe);
	if ( (pid = swndfork(sigmask, NULL)) < 0) {
		return(-1);
	} else if (pid == 0) {	
		close(opipe[0]);
		close(ipipe[1]);
		if (dup2(ipipe[0], 0) != 0)
			fprintf(stderr, "dup2 error to ifd = %d", ifd);
		if (dup2(opipe[1], 1) != 1)
			fprintf(stderr, "dup2 error to ofd = %d", ofd);
		if (dup2(efd, 2) != 2)
			fprintf(stderr, "dup2 error to efd = %d", efd);
		close(efd);
		close(opipe[1]);
		close(ipipe[0]);
		swgp_close_all_fd(3); 
		return(0);
	} else {
		close(opipe[1]);
		close(ipipe[0]);
		ptrfdm[0] = opipe[0];
		ptrfdm[1] = ipipe[1];
		ptrfdm[2] = efd;
		return(pid);
	}
}


#define SWFORK_PTY				"pty" /* the old pty fork */
#define SWFORK_PTY2				"pty2"
#define SWFORK_NO_PTY				"no-pty"
#define SWFORK_NO_FORK				"no-fork"

pid_t
swlib_fork(char * type, int *ptrfdm, int ofd, int ifd, int efd, 
		const struct termios *slave_termios,
		const struct winsize *slave_winsize, 
		pid_t * pcpid, 
		int make_raw,
		sigset_t * sigmask)
{
	if (strcmp(type, SWFORK_PTY) == 0) {
		*pcpid = 0;
		return -1;
	} else if (strcmp(type, SWFORK_PTY2) == 0) {
		return swlib_pty_fork2(
				ptrfdm,
				ofd,
				ifd,
				efd,
				slave_termios,
				slave_winsize, pcpid, make_raw,
				sigmask);
	} else if (strcmp(type, SWFORK_NO_PTY) == 0) {
		*pcpid = 0;
		return swlib_no_pty_fork(
				ptrfdm,
				ofd,
				ifd,
				efd,
				sigmask);
	} else if (strcmp(type, SWFORK_FORK) == 0) {
		pid_t	pid;
		*pcpid = 0;
		if ((pid = swndfork(sigmask, NULL)) < 0) {
			return(-1);
		} else if (pid == 0) {	
			return(0);
		} else {
			ptrfdm[0] = ifd;
			ptrfdm[1] = ofd;
			ptrfdm[2] = efd;
			return(pid);
		}
	} else if (strcmp(type, SWFORK_NO_FORK) == 0) {
			ptrfdm[0] = ifd;
			ptrfdm[1] = ofd;
			ptrfdm[2] = efd;
			return(0);
	}
	return -1;
}

pid_t
swndfork(sigset_t * sigmask, sigset_t * sigdfl)
{
	int ret;
	ret = fork();
	if (ret == 0) {
		if (sigdfl) {
			struct sigaction act;
			sigemptyset(&act.sa_mask);
			act.sa_handler = SIG_DFL;
			act.sa_flags = 0;
			if (sigismember(sigdfl, SIGTERM)) 
					sigaction(SIGTERM, &act, NULL); 
			if (sigismember(sigdfl, SIGINT)) 
					sigaction(SIGINT, &act, NULL); 
			if (sigismember(sigdfl, SIGPIPE)) 
					sigaction(SIGPIPE, &act, NULL); 
			if (sigismember(sigdfl, SIGALRM)) 
					sigaction(SIGALRM, &act, NULL); 
		}
		if (sigmask) {
			sigprocmask(SIG_BLOCK, sigmask, NULL);
		}
	}
	return ret;
}
