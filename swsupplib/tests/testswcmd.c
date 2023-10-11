#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "swparser_global.h"
#include "swlib.h"
#include "shcmd.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int
main (int argc, char ** argv)
{
	int status;
	int i, child;
	SHCMD *cmd[100];	
	int pip[2];

	fprintf(stderr, "this test program is busted\n");
	exit(2);
	if (argc < 2) exit(2);

	/*
	for (i=1;i<argc;i++)
		cmd[i-1]=shcmd_open(argv[i]);
	*/

	cmd[argc-1]=NULL;
	
	pipe(pip);	
	
	if((child = swfork((sigset_t*)(NULL))) > 0) {
		close(pip[1]);
		swlib_pump_amount(STDOUT_FILENO, pip[0], -1);
	} else if (!child) {
		close(pip[0]);
		shcmd_set_dstfd(cmd[argc-2], pip[1]);
		fprintf(stderr, "last child is %d\n", shcmd_command(cmd));
		shcmd_wait(cmd);
		for (i=1;i<argc;i++) {
			fprintf(stderr, "process %s %d : %d\n", *((cmd[i-1]->argv_) + 1), shcmd_get_pid(cmd[i-1]), shcmd_get_exitval(cmd[i-1]));
		}
		_exit(0);	
	} else {
		_exit(22);
	}

	wait(&status);	
	exit(0);
}




