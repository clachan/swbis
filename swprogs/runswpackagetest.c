#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "swlib.h"
#include "swparser_global.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swi.h"
#include "swcommon_options.h"
#include "swcommon.h"
#include "swparse.h"
#include "swgp.h"
#include "swssh.h"
#include "swevents_array.h"
#include "fmgetpass.h"

#define PW_FD "3"
#define GPGNAME "Test User"
#define PASSPHRASE "Iforgot\n"

int
main(int argc, char **argv)
{
	int pw[2];
	pid_t pid;
	int status;

	char cwd[400];
	STROB * tmp = strob_open(1);
	SHCMD * cmd[2];
	cmd[0] = shcmd_open();
	cmd[1] = NULL;

	getcwd(cwd, sizeof(cwd));
	strob_strcpy(tmp, cwd);
	strob_strcat(tmp, "/swprogs/swpackage");
	shcmd_add_arg(cmd[0], strob_str(tmp));
	argv++;
	while(*argv) {
		shcmd_add_arg(cmd[0], *argv);
		argv++;
	}	
	shcmd_add_arg(cmd[0], "-Wgpg-name=" GPGNAME);
	shcmd_add_arg(cmd[0], "-Wsign");
	shcmd_add_arg(cmd[0], "-Wpassphrase-fd=" PW_FD);

	pipe(pw);	
	pid = fork();

	if (pid == 0) {
		close(pw[1]);
		dup2(pw[0], atoi(PW_FD));
		shcmd_apply_redirection(cmd[0]);
		shcmd_unix_execve(cmd[0]);
		fprintf(stderr, "exec error %s not found.\n", strob_str(tmp));
		_exit(2);
	}
	close(pw[0]);
	write(pw[1], PASSPHRASE, strlen(PASSPHRASE));
	close(pw[1]);
	waitpid(pid, &status, 0);
	exit(WEXITSTATUS(status));
}
