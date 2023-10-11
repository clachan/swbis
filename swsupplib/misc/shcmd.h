/* shcmd.h  --  Shell-less Command Pipeline object.
*/


#ifndef SHCMD_112901_H
#define SHCMD_112901_H

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
#include <limits.h>
#include "swuser_config.h"
#include "cplob.h"

#define o__inline__
extern char ** environ;

typedef struct {
	char * cmdstring_;
	char * argstring_;
	CPLOB * argvector_;
	char ** argv_;
	int argv_set_;
	int srcfd_;
	int closefd_;
	int dstfd_;
	int errfd_;
	char * dstfile_;
	char * srcfile_;
	char * errfile_;
	int append1_;
	int append2_;
	int async_;
	pid_t pid_;
	int status_;
	char ** envp_;
	char user_[64];
	char group_[64];
	mode_t umask_;
	int close_list_[10];
	int (*f_exec_)(void * cmd);
	int (*f_filter_)(int ofd, int ifd, void * ctrl);
	void * f_filter_ctrl_;
	int close_all_fd_;
	int child_gone_;
	int esrch_;
	int proc_error_;
} SHCMD;

#define SHCMD_TAINTED_CHARS	"'\"|*?;&<>`$"
#define SHCMD_UNSET_EXITVAL 1000
#define SHCMD_INTERNAL_FILTER	"/."  /* special never-used name, to mean run internal function */

SHCMD* 		shcmd_open(void);
void   		shcmd_close(SHCMD * shcmd);
void 		shcmd_set_srcfd(SHCMD * shcmd, int fd);
void 		shcmd_set_dstfile(SHCMD * shcmd, char * filename);
void 		shcmd_set_srcfile(SHCMD * shcmd, char * filename);
void 		shcmd_set_errfile(SHCMD * shcmd, char * filename);
void 		shcmd_set_append(SHCMD * shcmd, int do_append);
void 		shcmd_set_append1(SHCMD * shcmd, int do_append);
void 		shcmd_set_append2(SHCMD * shcmd, int do_append);
void 		shcmd_set_dstfd(SHCMD * shcmd, int fd);
void 		shcmd_set_errfd(SHCMD * shcmd, int fd);
int 		shcmd_apply_redirection(SHCMD * cmd);
int 		/* Depricated name, see shcmd_cmdvec_exec */ shcmd_command(SHCMD ** cmd_vector);
int 		/* Depricated name, see shcmd_cmdvec_wait */ shcmd_wait(SHCMD ** cmd_vector);
int 		shcmd_reap_child(SHCMD * shcmd, int);
int 		shcmd_get_srcfd(SHCMD * shcmd);
int		shcmd_get_dstfd(SHCMD * shcmd);
int		shcmd_get_errfd(SHCMD * shcmd);
pid_t		shcmd_get_pid(SHCMD * shcmd);
int		shcmd_get_exitval(SHCMD * shcmd);
void		shcmd_set_envp(SHCMD * shcmd, char ** envp);
void		shcmd_set_umask(SHCMD * shcmd, mode_t mode);
void		shcmd_set_argv(SHCMD * shcmd, char ** argv);
char **		shcmd_get_envp(SHCMD * shcmd);
mode_t		shcmd_get_umask(SHCMD * shcmd);
int 		shcmd_debug_show(SHCMD *cmd);
int 		shcmd_debug_show_to_file(SHCMD *cmd, FILE * file);
int 		shcmd_debug_show_command(SHCMD *cmd, int fd);
int		shcmd_cmdvec_debug_show_to_file(SHCMD ** vec, FILE * file);
void		shcmd_set_user(SHCMD *cmd, char * username);
void		shcmd_set_group(SHCMD *cmd, char * groupname);
char *		shcmd_get_user(SHCMD *cmd);
char *		shcmd_get_group(SHCMD *cmd);
char ** 	shcmd_add_arg(SHCMD * cmd, char * arg);
int 		shcmd_do_tainted_data_check(char * cmdstring);
int		shcmd_do_run(char * cmdstring);
int 		shcmd_cmdvec_exec (SHCMD ** cmd_vector);
int 		shcmd_cmdvec_wait (SHCMD ** cmd_vector);
int 		shcmd_cmdvec_wait2 (SHCMD ** cmd_vector);
int		shcmd_unix_exec(SHCMD *);
int 		shcmd_unix_execvp(void * cmd);
int 		shcmd_unix_execve(void * cmd);
void 		shcmd_set_exec_function(SHCMD * shcmd, char * form);
char ** 	shcmd_get_argvector(SHCMD * cmd);
char * 		shcmd_find_in_path(char * ppath, char * pgm);
void 		shcmd_add_close_fd(SHCMD * shcmd, int fd);
void 		shcmd_set_lowest_close_fd(SHCMD * cmd, int fd); 
SHCMD *		shcmd_get_last_command(SHCMD **);
int		shcmd_write_command_to_buf(SHCMD *cmd, STROB * tmp);

#endif
