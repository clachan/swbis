/* shcmd.c  --  Shell-less Command pipeline object.
*/

/*
   Copyright (C) 2001-2004 Jim Lowe
   All Rights Reserved.
  
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#define FILENEEDDEBUG 1 /* controls the E_DEBUG macros */
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include "strob.h"
#include "swfork.h"
#include "shcmd.h"
#include "swgp.h"
#include "swutilname.h"
#include "swlib.h"


#define SHC_DIR_DOWN 0
#define SHC_DIR_UP 1
#define CLOSE_LIST_LEN ((int)(sizeof(shcmd->close_list_) / sizeof(int)))

int	swlib_pipe_pump(int ofd, int suction_fd);

static long
shcmd_getgid(SHCMD *cmd)
{
	struct group * pw;	
	if (strlen(cmd->group_)) {
		pw = getgrnam(cmd->group_);
		if (pw) {
			return pw->gr_gid;
		} else {
			return -2;
		}	
	} else {
		/*
		 * Not set, this is the default
		 */
		return -1;
	}
}

static long
shcmd_getuid(SHCMD *cmd)
{
	struct passwd * pw;	
	if (strlen(cmd->user_)) {
		pw = getpwnam(cmd->user_);
		if (pw) {
			return (long)(pw->pw_uid);
		} else {
			/*
			 * error
			 */
			return (long)-2;
		}	
	} else {
		/*
		 * not set, this is the default
		 */
		return (long)-1;
	}
}

static void
internal_redirect(
		SHCMD * cmd, 
		int srcfd, 
		char * srcfile, 
		int dstfd, 
		char * dstfile, 
		int append1, 
		int errfd, 
		char * errfile, 
		int append2)
{
	int fd;
	int flags;
	mode_t mode;

	if (srcfd != 0 || srcfile) {
		if (srcfd == STDIN_FILENO && !srcfile) {
			;
		} else if (srcfd > 0 && !srcfile) {
			if (dup2(srcfd, STDIN_FILENO)) {
				fprintf (stderr,"src dup error\n");
			}
			close(srcfd);
		} else if (srcfile) {
			if ((fd=open(srcfile, O_RDONLY, 0)) == -1) {
				fprintf (stderr,
					"fatal: error opening %s\n", srcfile);
				_exit(1);
			}
			if (fd != STDIN_FILENO) {
				dup2(fd, STDIN_FILENO);
			}
		} else {
			fprintf (stderr,
			"fatal: internal error, invalid stdin redirection. \n");
			_exit(1);
		}
	}

	if (dstfd != STDOUT_FILENO || dstfile) {
		if (dstfd == STDOUT_FILENO && !dstfile) {
			;
		} else if (dstfd > STDOUT_FILENO && !dstfile) {
			if (dup2(dstfd, STDOUT_FILENO) != STDOUT_FILENO) {
				fprintf (stderr,"dst dup error\n");
			}
			close(dstfd);
		} else if (dstfile) {
			flags=O_WRONLY|O_CREAT;
			if (append1) {
				flags |= O_APPEND;
			} else {
				flags |= O_TRUNC;
			}
			mode = umask(cmd->umask_);		
			if ((fd=open(dstfile, flags, 0666)) == -1) {
				fprintf (stderr,
					"fatal: error opening %s\n", dstfile);
				_exit(1);
			}
			umask(mode);
			if (fd != STDOUT_FILENO) {
				dup2(fd, STDOUT_FILENO);
			}
			if (append1) {
				if (lseek(1, 0L, SEEK_END) == -1) {
					fprintf (stderr,
					"error seeking to end of %s\n",
						dstfile);
				}
			}
		} else {
			fprintf (stderr,
				"fatal: internal error,"
				" invalid stdout redirection. \n");
			_exit(1);
		}
	}

	if (errfd != STDERR_FILENO || errfile) {
		if (errfd == STDERR_FILENO && !errfile) {
			;
		} else if ((errfd > STDERR_FILENO && !errfile) || errfd == STDOUT_FILENO) {
			if (dup2(errfd, STDERR_FILENO) != STDERR_FILENO) {
				fprintf (stderr,"err dup error\n");
			}
			close(errfd);
		} else if (errfile) {
			flags=O_WRONLY|O_CREAT;
			if (append2) {
				flags |= O_APPEND;
			} else {
				flags |= O_TRUNC;
			}
			mode = umask(cmd->umask_);		
			if ((fd = open(errfile, flags, 0666)) == -1) {
				fprintf (stderr,
					"fatal: error opening %s\n", errfile);
				_exit(1);
			}
			umask(mode);
			if (fd != STDERR_FILENO) {
				dup2(fd, STDERR_FILENO);
			}
			if (append2) {
				lseek(fd, 0L, SEEK_END);
			}
		} else {
			fprintf (stderr,"fatal: internal error,"
					" invalid stderr redirection. \n");
			_exit(1);
		}
	}
}

static
SHCMD* 
shcmd_common_open(/* char * cmdstring */ void)
{
	mode_t mode = (mode_t)0;
	int i;
	SHCMD * shcmd;

	shcmd=(SHCMD*)malloc(sizeof(SHCMD));
       	if (shcmd == NULL) return NULL;
 
	shcmd->argvector_ = cplob_open(10);
	shcmd->argv_set_=0;

	shcmd->argv_ = (char**)(NULL);
	shcmd->argstring_ = NULL;
	shcmd->cmdstring_= NULL;
	shcmd->closefd_ = -1;
	shcmd->pid_ = 0;
	shcmd->status_ = 0;
	shcmd->srcfd_ = STDIN_FILENO;
	shcmd->dstfd_ = STDOUT_FILENO;
	shcmd->errfd_ = STDERR_FILENO;
	shcmd->dstfile_ = (char*)NULL;
	shcmd->srcfile_ = (char*)NULL;
	shcmd->errfile_ = (char*)NULL;
	shcmd->append1_ = 0;
	shcmd->append2_ = 0;
	shcmd->async_ = 0;
	shcmd->close_all_fd_ = -1;
	shcmd->child_gone_ = 0;
	shcmd->proc_error_ = 0;
	shcmd->esrch_ = 0;
	shcmd->envp_ = environ;
	mode = umask(mode);
	umask(mode);
	shcmd->umask_ = mode;
	(shcmd->user_)[0] = '\0';  /* empty string */
	(shcmd->group_)[0] = '\0';  /* empty string */
	shcmd->f_filter_ =  (int (*) (int, int, void*))(NULL);
	shcmd->f_filter_ctrl_ = (void*)NULL;
	shcmd->f_exec_ = shcmd_unix_execve;
	for (i=0; i < CLOSE_LIST_LEN; i++) {
		shcmd->close_list_[i] = -1;
	}
	return shcmd;
}

static
void
close_list(SHCMD * shcmd)
{
	int fd;
	int i = 0;
	while (i < CLOSE_LIST_LEN) {
		/* fprintf(stderr, "shcmd : closing [%d]\n", fd); */
		fd = shcmd->close_list_[i];
		if (fd >= 0)
			if (close(fd) < 0) 
				fprintf(stderr,"shcmd : close_list_: fd=%d : %s\n",
							fd, strerror(errno));
		i++;
	}
}

static 
int
cmd_invoke(SHCMD *cmd)
{
	int uid;
	int gid;
	int child_exitval;
	int child_pid;
	int srcfd=cmd->srcfd_;
	int dstfd=cmd->dstfd_;

	child_pid = swfork((sigset_t*)(NULL));
	if (child_pid < 0) {
		fprintf (stderr,"Can't create new process.\n");
		return -1;
	} else if (child_pid == 0) {

		if (cmd->closefd_ != -1) close(cmd->closefd_);	
		close_list(cmd);

		gid = (gid_t)shcmd_getgid(cmd);
		if (shcmd_getgid(cmd) >= 0) {
			if (setgid((gid_t)(shcmd_getgid(cmd)))) {
				fprintf(stderr, "setgid failed in"
					" shcmd_cmd_invoke, group=%s %s\n", 
						cmd->group_, strerror(errno));
			}
		} else if (gid == -2) {
			fprintf(stderr, "shcmd_getgid failed:"
					" gid not found for group=%s",
							cmd->group_);
		} else {
			/* 
			 * no change, group not set
			 */
			;
		}

		uid = (uid_t)shcmd_getuid(cmd);
		if (uid >= 0) {
			if(setuid((uid_t)(uid))) {
				fprintf(stderr, "setuid failed in"
					" shcmd_cmd_invoke, user=%s %s\n", 
						cmd->user_, strerror(errno));
			}
		} else if (uid == -2) {
			/*
			 * error
			 */
			fprintf(stderr, "shcmd_getuid failed:"
					" uid not found for user=%s",
							cmd->user_);
		} else {
			/* 
			 * no change, user not set
			 */
			;
		}


		internal_redirect(cmd, srcfd, cmd->srcfile_, 
					dstfd, 
					cmd->dstfile_, 
					cmd->append1_, 
					cmd->errfd_, 
					cmd->errfile_, 
					cmd->append2_);
		umask(cmd->umask_);
		if (cmd->argv_ && cmd->argv_[0] && strcmp(cmd->argv_[0], SHCMD_INTERNAL_FILTER) == 0) {
			int ret;
			if (cmd->f_filter_) {
				ret = (*(cmd->f_filter_))(STDOUT_FILENO, STDIN_FILENO, cmd->f_filter_ctrl_);
			} else {		
				ret = swlib_pipe_pump(STDOUT_FILENO, STDIN_FILENO);
			}
			child_exitval = ret < 0 ? 1 : 0;
		} else if (cmd->argv_ && cmd->argv_[0] && strlen(cmd->argv_[0])) {
 			(*(cmd->f_exec_))(cmd);
			fprintf (stderr,"%s: unable to execute %s\n", swlib_utilname_get(), cmd->argv_[0]);	
			child_exitval = 126;
		} else {
			fprintf(stderr,	"shcmd.c: illegal or null command name\n");
			child_exitval = 1;
		}
		_exit(child_exitval);
	} else {
		if (srcfd > 0)
			close (srcfd);
		if (dstfd > 1)
			close (dstfd);
		cmd->pid_ = child_pid;
		return child_pid;
	}
	return (-1);
}

static 
int
command_recurse(SHCMD ** cmd_vector, int *wpid, int makepipe, int * pipefd)
{
	int pid;
	int pfd[2];
	for(;;) {
		if (*(cmd_vector+1) != NULL) {
			command_recurse(cmd_vector+1, wpid, 1, 
						&(cmd_vector[0]->dstfd_));
		}
			
		if (makepipe) {
			pipe(pfd);	
			*pipefd=pfd[1];
			cmd_vector[0]->srcfd_=pfd[0];
			cmd_vector[0]->closefd_=pfd[1];
		} 
		pid = cmd_invoke(*cmd_vector);
		if(*wpid == 0) 
			*wpid = pid;
		return *wpid;
	}
}

/*
 * ====================================================================
 * ====================================================================
 * 
 * +++++++++++++++ Public Interface +++++++++++++++++++++++++++++++++++
 * 
 * ====================================================================
 * ====================================================================
 */

void
shcmd_add_close_fd(SHCMD * shcmd, int ifd)
{
	int fd;
	int i = 0;
	fd = shcmd->close_list_[i];
	while (fd >= 0 && i < CLOSE_LIST_LEN) {
		fd = shcmd->close_list_[++i];
	}
	if (i < CLOSE_LIST_LEN) {
		shcmd->close_list_[i] = ifd;
	} else {
		fprintf(stderr, "shcmd_add_close_fd() internal error\n");
	}
}

char *
shcmd_find_in_path(char * ppath, char * pgm)
{
	char * retval = NULL;
	struct stat st;
	STROB * tmp;
	STROB * name;
	char * s;
	int ret;

	if (strchr(pgm, '/')) {
		ret = stat(pgm, &st);
		if (ret < 0) return NULL;
		if (!S_ISREG(st.st_mode)) return NULL;
		return pgm;
	}

	if (!ppath) return (char*)NULL;
	tmp = strob_open(10);
	name = strob_open(10);

	s = strob_strtok(tmp, ppath, ":");
	while (s) {
		strob_strcpy(name, s);
		strob_strcat(name, "/");
		strob_strcat(name, pgm);
		if (access(strob_str(name), X_OK) == 0) {
			retval = strdup(strob_str(name));	
			break;
		}
		s = strob_strtok(tmp, NULL, ":");
	}
	strob_close(name);
	strob_close(tmp);
	return retval;
}

int
shcmd_apply_redirection(SHCMD * cmd)
{
	internal_redirect(cmd, 
			cmd->srcfd_, 
			cmd->srcfile_, 
			cmd->dstfd_, 
			cmd->dstfile_, 
			cmd->append1_, 
			cmd->errfd_, 
			cmd->errfile_, 
			cmd->append2_);
	return 0;
}

int
shcmd_reap_child(SHCMD * cmd, int flag)
{
	pid_t pid;

	if (cmd->status_ || cmd->child_gone_) {
		cmd->child_gone_ = 1;
		return 0;
	}

	E_DEBUG("");
	pid = waitpid(cmd->pid_, &(cmd->status_), flag);
	if (pid < 0) {
		E_DEBUG("ERROR");
		if (errno == ECHILD) {
			E_DEBUG("ECHILD");
			cmd->child_gone_ = 1;
		} 
		cmd->proc_error_ = 1;
		fprintf(stderr, "shcmd: waitpid return error (pid=%d) for %s: %s\n", 
			(int)(cmd->pid_), cmd->argv_[0], strerror(errno));
	} else if (pid > 0) {
		E_DEBUG3("Normal stop: pid=%d: %s", (int)(cmd->pid_), cmd->argv_[0]);
		E_DEBUG3("exit value for %s is %d", cmd->argv_[0], shcmd_get_exitval(cmd));
		cmd->child_gone_ = 1;
	}
	return pid;
}

int
shcmd_cmdvec_wait(SHCMD ** cmd_vec)
{
	return shcmd_wait(cmd_vec);
}


int
shcmd_cmdvec_kill(SHCMD ** cmd_vec, int from, int direction)
{
	int ret = 0;
	int i = from;
	while(i >= 0 && cmd_vec[i]) {
		if (cmd_vec[i]->status_ == 0 &&
		    cmd_vec[i]->esrch_ == 0 &&
		    cmd_vec[i]->child_gone_ == 0) {
			ret = kill(cmd_vec[i]->pid_, SIGTERM);
			if (ret < 0) {
				if (errno == ESRCH) {
		    			cmd_vec[i]->esrch_ = 1;
				}
			}
		}
		if (direction == SHC_DIR_UP) i++;
		else i--;
	}
	return ret;
}

int
shcmd_cmdvec_wait2(SHCMD ** cmd_vec)
{
	SHCMD ** cmdv = cmd_vec;
	int ncmds = 0;
	int ncmds_exited = 0;
	int i;
	int ret;
	int highest_index_with_error;
	
	while(*cmdv) {
		cmdv++;
		ncmds++;
	}
	E_DEBUG2("No. of commands in pipeline: ncmds = [%d]", ncmds);

	i = 0;
	highest_index_with_error = 0;
	do {
		E_DEBUG("Loop Begin");
		for(i=ncmds-1; i>=0; i--) {
			E_DEBUG3("calling reap_child for for cmd_vec[%d]: %s", i, cmd_vec[i]->argv_[0]);
			ret = shcmd_reap_child(cmd_vec[i], WNOHANG); 
			E_DEBUG3("finished reap_child for for cmd_vec[%d]: ret=%d", i, ret);
			if (ret > 0) {
				E_DEBUG2("good stop for %s", cmd_vec[i]->argv_[0]);
				ncmds_exited++;
			} else if (ret == 0) {
				if (cmd_vec[i]->child_gone_ == 0) {
					E_DEBUG2("still waiting on pid %d", (int)(cmd_vec[i]->pid_));
					;
				}
				;
			} else {
				if (errno == ECHILD) {
					E_DEBUG2("ECHILD on pid %d", (int)(cmd_vec[i]->pid_));
					cmd_vec[i]->proc_error_ = 1;
					ncmds_exited++;
				} else {
					E_DEBUG("");
					fprintf(stderr, "shcmd: waitpid return error (line=%d) for [%d]: %s\n", 
						__LINE__, (int)(cmd_vec[i]->pid_), cmd_vec[i]->argv_[0]);
				}
			}
			if (cmd_vec[i]->proc_error_) {
				if (i > highest_index_with_error)
					highest_index_with_error = i;
			}
		}
		if (highest_index_with_error) {
			E_DEBUG2("Killing commands at index=%d", highest_index_with_error);
			shcmd_cmdvec_kill(cmd_vec, highest_index_with_error, SHC_DIR_UP);
		}
		highest_index_with_error = 0;
		E_DEBUG("Sleeping");
		usleep(100000);
		E_DEBUG("Loop End");
	} while (ncmds_exited < ncmds);
	E_DEBUG("Done");
	return shcmd_get_exitval(cmd_vec[ncmds-1]);
}


/* Depricated name */
int
shcmd_wait(SHCMD ** cmd_vec)
{
	SHCMD ** cmdv = cmd_vec;
	pid_t pid;
	int ncmds = 0;
	int i;
	int ret;
	
	while(*cmdv) {
		cmdv++;
		ncmds++;
	}
	cmdv--;	
	E_DEBUG("");
	if (shcmd_reap_child(*cmdv, 0) > 0) {
		E_DEBUG("");
		ncmds--;	
		for(i=0; i<ncmds; i++) {
			E_DEBUG("");
			pid=shcmd_reap_child(cmd_vec[i], 0);
		}
	}
	ret = shcmd_get_exitval(*cmdv);
	return ret;
}

int
shcmd_cmdvec_exec(SHCMD ** cmd_vector)
{
	return shcmd_command(cmd_vector);
}

int
shcmd_unix_exec(SHCMD * cmd)
{
	if (((SHCMD*)cmd)->close_all_fd_ >= 0) 
		swgp_close_all_fd(((SHCMD*)cmd)->close_all_fd_);
	return execve(cmd->argv_[0], cmd->argv_, cmd->envp_);	
}

int
shcmd_unix_execvp(void * cmd)
{
	if (((SHCMD*)cmd)->close_all_fd_ >= 0) 
		swgp_close_all_fd(((SHCMD*)cmd)->close_all_fd_);
	return execvp(((SHCMD*)cmd)->argv_[0], ((SHCMD*)cmd)->argv_);	
}

int
shcmd_unix_execve(void * cmd)
{
	if (((SHCMD*)cmd)->close_all_fd_ >= 0) 
		swgp_close_all_fd(((SHCMD*)cmd)->close_all_fd_);
	return execve(((SHCMD*)cmd)->argv_[0], 
			((SHCMD*)cmd)->argv_, ((SHCMD*)cmd)->envp_);	
}

/* Depricated name */
int
shcmd_command(SHCMD ** cmd_vector)
{
	int pids = 0;
	return command_recurse(cmd_vector, &pids, 0, NULL);
}

int
shcmd_do_tainted_data_check(char * cmdstring) 
{
	#define SHCMD_TAINTED_CHARS	"'\"|*?;&<>`$"
	if (strpbrk(cmdstring, SHCMD_TAINTED_CHARS)) {
		return 1;
	}
	return 0;
}

SHCMD *
shcmd_open(void)
{
	return shcmd_common_open(/*(char*)NULL*/);
}

int 
shcmd_cmdvec_debug_show_to_file(SHCMD ** vec, FILE * file) 
{
	SHCMD ** pv = vec;
	while(*pv) {
		shcmd_debug_show_to_file(*pv, file);
		pv++;
	}
	return 0;	
}

int
shcmd_debug_show(SHCMD *cmd)
{
	return shcmd_debug_show_to_file(cmd, stderr);
}

int
shcmd_debug_show_to_file(SHCMD *cmd, FILE * file)
{
	char **vec = cmd->argv_;

	if (cmd->cmdstring_)
		fprintf(file,"%p: command string = [%s]\n",
					(void*)cmd, cmd->cmdstring_);
	else	
		fprintf(file,"%p: command string = nil\n",  (void*)cmd);

	if (vec) {
		while(*vec) {
			fprintf(file, "<[%s]>",  *vec);
			vec++;
		}
	} else {
		fprintf(file, "nil\n");
	}
	fprintf(file,"\n     pid=%d exit value=%d\n",
				(int)(cmd->pid_), shcmd_get_exitval(cmd));
	fprintf(file,"     status=%d, srcfd=%d; dstfd=%d\n",
				cmd->status_, cmd->srcfd_, cmd->dstfd_);
	fprintf(file,"     user=[%s] group=[%s]\n",
				cmd->user_, cmd->group_);
	return 0;
}

int
shcmd_write_command_to_buf(SHCMD *cmd, STROB * tmp)
{
	char **arg = cmd->argv_;
	if (arg) {
		while(*arg) {
			if (strpbrk(*arg, "; &*?$|<>`\"")) {
				strob_sprintf(tmp, STROB_DO_APPEND, "'%s'",  *arg);
			} else if (strlen(*arg) == 0) {
				strob_sprintf(tmp, STROB_DO_APPEND, "''");
			} else {
				strob_sprintf(tmp, STROB_DO_APPEND, "%s",  *arg);
			}
			arg++;
			if (*arg) strob_sprintf(tmp, STROB_DO_APPEND," ");
		}
	} 
	return 0;
}

int
shcmd_debug_show_command(SHCMD *cmd, int fd)
{
	int ret;
	STROB * tmp = strob_open(10);
	shcmd_write_command_to_buf(cmd, tmp);
	strob_sprintf(tmp, STROB_DO_APPEND, "\n");
	ret = uxfio_write(fd, strob_str(tmp), strob_strlen(tmp));
	strob_close(tmp);
	return ret;
}


void
shcmd_close(SHCMD * shcmd)
{
	cplob_close(shcmd->argvector_);
	if (shcmd->dstfile_)
		swbis_free(shcmd->dstfile_);
	if (shcmd->srcfile_)
		swbis_free(shcmd->srcfile_);
	if (shcmd->errfile_)
		swbis_free(shcmd->errfile_);
	if (shcmd->cmdstring_)
		swbis_free(shcmd->cmdstring_);
	if (shcmd->argv_ && !shcmd->argv_set_)
		swbis_free(shcmd->argv_);
	if (shcmd->argstring_)
		swbis_free(shcmd->argstring_);

	swbis_free(shcmd);
}

void
shcmd_set_exec_function(SHCMD * shcmd, char * form)
{
	if (strcmp(form, "execvp") == 0) {
		shcmd->f_exec_ = shcmd_unix_execvp;
	} else if (strcmp(form, "execve") == 0) {
		shcmd->f_exec_ = shcmd_unix_execve;
	} else {
		fprintf(stderr, "shcmd_set_exec_function:"
					" no effect, invalid function\n");
	}
}

void
shcmd_set_dstfd(SHCMD * shcmd, int fd)
{
	shcmd->dstfd_=fd;
}

void
shcmd_set_append(SHCMD * shcmd, int do_append)
{
	shcmd_set_append1(shcmd, do_append);
}

void
shcmd_set_append1(SHCMD * shcmd, int do_append)
{
	shcmd->append1_=do_append;
}

void
shcmd_set_append2(SHCMD * shcmd, int do_append)
{
	shcmd->append2_ = do_append;
}

void
shcmd_set_srcfile(SHCMD * shcmd, char * name)
{
	shcmd->srcfile_ = strdup(name);
}

void
shcmd_set_dstfile(SHCMD * shcmd, char * name)
{
	shcmd->dstfile_ = strdup(name);
}

void
shcmd_set_errfile(SHCMD * shcmd, char * name)
{
	shcmd->errfile_ = strdup(name);
}

void
shcmd_set_srcfd(SHCMD * shcmd, int fd)
{
	shcmd->srcfd_=fd;
}

void
shcmd_set_errfd(SHCMD * shcmd, int fd)
{
	shcmd->errfd_=fd;
}

int
shcmd_get_srcfd(SHCMD * shcmd)
{
	return shcmd->srcfd_;
}

int
shcmd_get_dstfd(SHCMD * shcmd)
{
	return shcmd->dstfd_;
}

int
shcmd_get_errfd(SHCMD * shcmd)
{
	return shcmd->errfd_;
}

pid_t
shcmd_get_pid(SHCMD * shcmd)
{
	return shcmd->pid_;
}

int
shcmd_get_exitval(SHCMD * cmd)
{
	if (cmd->pid_ && WIFEXITED(cmd->status_)) {
		return WEXITSTATUS(cmd->status_);
	} else {
		return SHCMD_UNSET_EXITVAL;
	}
}

char **
shcmd_get_envp(SHCMD * cmd)
{
	return cmd->envp_;
}

void
shcmd_set_envp(SHCMD * cmd, char ** env)
{
	cmd->envp_ = env;
}

mode_t
shcmd_get_umask(SHCMD * cmd)
{
	return cmd->umask_;
}

void
shcmd_set_umask(SHCMD * cmd, mode_t mode)
{
	cmd->umask_ = (int)mode;
}

char **
shcmd_get_argvector(SHCMD * cmd) {
	return cplob_get_list(cmd->argvector_);
}

char **
shcmd_add_arg(SHCMD * cmd, char * arg)
{
	cplob_add_nta(cmd->argvector_, strdup(arg));
	cmd->argv_set_=1;
	cmd->argv_ = cplob_get_list(cmd->argvector_);
	return cmd->argv_;
}

void
shcmd_set_argv(SHCMD * cmd, char ** argv)
{
	cmd->argv_set_=1;
	cmd->argv_ = argv;
	if (cmd->cmdstring_) swbis_free(cmd->cmdstring_);
	cmd->cmdstring_ = strdup(argv[0]);
}

void
shcmd_set_user(SHCMD * cmd, char * name)
{
	strncpy(cmd->user_, name, sizeof(cmd->user_) - 1);
	cmd->user_[sizeof(cmd->user_) - 1] = '\0';
}

void
shcmd_set_group(SHCMD * cmd, char * name)
{
	strncpy(cmd->group_, name, sizeof(cmd->group_) - 1);
	cmd->group_[sizeof(cmd->group_) - 1] = '\0';
}

char *
shcmd_get_user(SHCMD * cmd)
{
	return cmd->user_;
}

char *
shcmd_get_group(SHCMD * cmd)
{
	return cmd->group_;
}

void
shcmd_set_lowest_close_fd(SHCMD * cmd, int fd)
{
	cmd->close_all_fd_ = fd;
}

SHCMD *
shcmd_get_last_command(SHCMD ** cmdvec)
{
	SHCMD ** v;
	SHCMD * last;
	last = NULL;
	v = cmdvec;
	while (*v) {
		last = *v;
		v++;
	}
	return last;
}
