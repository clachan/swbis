#ifndef SWFORK_H_02a
#define SWFORK_H_02a
#include "swuser_config.h"
#include	<termios.h>
#include	<unistd.h>
#include	<signal.h>

#define swfork(arg) swndfork(((sigset_t*)(arg)), (sigset_t*)NULL)

#ifndef	TIOCGWINSZ
#include	<sys/ioctl.h>	/* 44BSD requires this too */
#endif

#define SWFORK_PTY				"pty" /* the old pty fork */
#define SWFORK_PTY2				"pty2"
#define SWFORK_NO_PTY				"no-pty"
#define SWFORK_FORK				"fork"
#define SWFORK_NO_FORK				"no-fork"

pid_t swlib_fork(char * type, int *ptrfdm, int ofd, int ifd, int efd, 
	const struct termios *slave_termios, 
	const struct winsize *slave_winsize,
		pid_t * pump_pid, int make_raw, sigset_t * blockset);
pid_t swlib_pty_fork2(int *ptrfdm, int ofd, int ifd, int efd, 
	const struct termios *slave_termios, 
		const struct winsize *slave_winsize, int * pump_pid, int make_raw, sigset_t * blockset);
pid_t swlib_no_pty_fork(int *ptrfdm, int ofd, int ifd,
					int efd, sigset_t * blockset);
int 	swlib_tty_cbreak(int fd);
int 	swlib_tty_raw(int fd);
int 	swlib_tty_reset(int fd);
void 	swlib_tty_atexit(void);
struct 	termios * swlib_tty_termios(void);
pid_t 	swndfork(sigset_t * mask_to_block, sigset_t * mask_to_set_dfl);

#endif
