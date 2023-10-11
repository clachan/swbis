#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <utime.h>
#include "swparser_global.h"
#include "swlib.h"
#include "shcmd.h"
#include "swevents_array.h"
#include "swcommon_options.h"

static
char * get_package_signature(SHCMD * sigcmd, char * user, int * statusp)
{
	SHCMD *cmd[3];	
	char * sig;
	int ret = 0;
	pid_t pid[6];
	int status[6];
	int passphrase[2];
	int input[2];
	int output[2];
	int wret;
	int done = 0;
	int sigfd;
	int i = 0;
	char * fdmem;

	*statusp = 255;
	cmd[1] = (SHCMD*)(NULL);

	if (sigcmd == NULL) {
		cmd[0] = shcmd_open();
		shcmd_add_arg(cmd[0], "/usr/bin/gpg");
		shcmd_add_arg(cmd[0], "--no-verbose");
		shcmd_add_arg(cmd[0], "--no-secmem-warning");
		shcmd_add_arg(cmd[0], "--quiet");
		shcmd_add_arg(cmd[0], "--armor");
		shcmd_add_arg(cmd[0], "--passphrase-fd");
		shcmd_add_arg(cmd[0], "3");
		shcmd_add_arg(cmd[0], "-u");
		shcmd_add_arg(cmd[0], user);
		shcmd_add_arg(cmd[0], "-sb");
		shcmd_add_arg(cmd[0], "-o");
		shcmd_add_arg(cmd[0], "-");
	} else {
		cmd[0] = sigcmd;
	}


	pipe(input);
	pipe(output);
	pipe(passphrase);


	pid[0] = swndfork((sigset_t*)(NULL), NULL);
	if (pid[0] < 0) return (char *)NULL;
	if (pid[0] == 0) 
	{
		close(input[1]);
		close(output[0]);
		shcmd_set_srcfd(cmd[0], input[0]);
		shcmd_set_dstfd(cmd[0], output[1]);
		shcmd_set_errfile(cmd[0], "/dev/null");
		shcmd_apply_redirection(cmd[0]);
		dup2(passphrase[0], 3);
		close(passphrase[1]);
		shcmd_unix_exec(cmd[0]);
		fprintf(stderr, "%s not run.\n", cmd[0]->argv_[0]);
		_exit(2); 
	}
	close(input[0]);
	close(output[1]);
	close(passphrase[0]);

	pid[1] = swndfork((sigset_t*)(NULL), NULL);
	if (pid[1] < 0) return (char*)NULL;
	if (pid[1] == 0) {
		FILE * fpipe;
		
		close(input[1]);
		close(output[0]);
		fpipe = fdopen(passphrase[1], "w");
		if (fpipe) {
			fprintf(fpipe, "%s\n", getpass("Enter Password: "));
			fclose(fpipe);
		} else {
			fprintf(stderr, "fdopen failed\n");
			_exit(3);
		}
		close(passphrase[1]);
		_exit(0);
	}
	close(passphrase[1]);


	pid[2] = swndfork((sigset_t*)(NULL), NULL);
	if (pid[2] < 0) return (char *)NULL;
	if (pid[2] == 0) {
		int ret = 0;
		int fd;
		close(input[0]);
		fd = open("/etc/passwd", O_RDONLY, 0);
		if (fd < 0) _exit(2);
		swlib_pipe_pump(input[1], fd);
		close(fd);
		close(input[1]);
		close(output[0]);
		_exit(ret);
	}
	close(input[1]);

	sigfd = uxfio_open("/dev/null", O_RDONLY, 0);
	uxfio_fcntl(sigfd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	uxfio_fcntl(sigfd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

	ret = swlib_pump_amount(sigfd, output[0], 1024);
	close(output[0]);
	if (ret < 0 || ret > 1000) return (char*)NULL;
	uxfio_write(sigfd, "\0", 1);	

	while (!done) {
		done = 1;
		for(i=0; i<3; i++) {
			if (pid[i] > 0) {
				wret = waitpid(pid[i], &status[i], WNOHANG);
				if (wret < 0) {
					fprintf(stderr, "error : %d %s\n", (int)pid[i], strerror(errno));
					break;
				} else if (wret == 0) {
					done = 0;
				} else {
					pid[i] = -pid[i];
				}
			}
		}
	}
	if (pid[0] < 0) {
		ret =  WEXITSTATUS(status[0]);
	} else {
		ret = 100;
	}
	*statusp = ret;
	if (ret) return (char*)NULL;

	uxfio_get_dynamic_buffer(sigfd, &fdmem, (int*)NULL, (int*)NULL);
	sig = strdup(fdmem);
	uxfio_close(sigfd);	
	return sig;
}
		
		
int
main (int argc, char ** argv)
{
	int ret;
	char * buf;
	int status;

	if (argc < 2) {
		fprintf(stderr, "usage %s username\n", argv[0]);	
		exit(2);
	}

	buf = get_package_signature(NULL, argv[1], &status);
	fprintf(stdout, ">>>>\n%s>>>>\n", buf ? buf : "");
	fprintf(stderr, "gpg exit status: %d\n", status);

	exit(ret);
}

