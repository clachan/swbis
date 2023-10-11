/* swgpg.c  --  routines involving gpg
 */

/*
 Copyright (C) 2007 Jim Lowe
 All Rights Reserved.
  
 COPYING TERMS AND CONDITIONS:
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vplob.h"
#include "swlib.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swinstall.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "strob.h"
#include "progressmeter.h"
#include "swevents.h"
#include "etar.h"
#include "swicol.h"
#include "swutillib.h"
#include "swproglib.h"
#include "swicat.h"
#include "swgpg.h"
#include "fmgetpass.h"
	
static char * g_swgpg_dirname = NULL;
static char * g_swgpg_fifopath = NULL;
static int swgpg_passfd;

static
int
get_stderr_fd(void)
{
	return STDERR_FILENO;
}

static
void
close_passfd(void)
{
	if (swgpg_passfd >= 0) {
		if (close(swgpg_passfd) < 0) {
			;
		}
	}
}

static
char *
make_dir(char ** ma)
{
	int ret;
	char * template;
	char * filename;
	char * sret;
	mode_t um;
	int try;
	const int max_tries = 10;
#undef  have__mkdtemp 
#define have__mkdtemp
#ifdef have__mkdtemp
	E_DEBUG("begin using mkdtemp");
	template = (char*)malloc(
			strlen(SWGPG_FIFO_DIR) + 1 +
			strlen(SWGPG_FIFO_PFX) + 8);
	if (!template) return NULL;
	*ma = template;
	strcpy(template, SWGPG_FIFO_DIR);
	strcat(template, "/");
	strcat(template, SWGPG_FIFO_PFX);	
	strcat(template, "XXXXXX");	
	filename = mkdtemp(template); 	
	if (filename == NULL) {
		fprintf(stderr, "%s: mkdtemp error for template [%s]: %s\n", swlib_utilname_get(), template, strerror(errno));
	}
	E_DEBUG2("end using mkdtemp: filename=[%s]", filename);
#else
	E_DEBUG("begin using tempnam");
	um = swlib_get_umask();
	umask(0002);
	try = 0;
	E_DEBUG("");
	do {
		E_DEBUG("");
		try++;
		filename = tempnam (SWGPG_FIFO_DIR, SWGPG_FIFO_PFX);
		umask(um);
		ret = mkdir(filename, (mode_t)(0700));
		if (ret < 0) {
			fprintf(stderr, "%s: mkdir failed with error: %s\n", swlib_utilname_get(), strerror(errno));
			free (filename);
			filename = NULL;
		}
	} while (ret < 0 && try < max_tries);
	E_DEBUG("");
	if (try >= max_tries && ret < 0) {
		fprintf(stderr, "%s: failed to make temp dir in %d tries\n", swlib_utilname_get(), max_tries);
		return NULL;
	}
	umask(um);
	E_DEBUG2("end using tempnam: filename=[%s]", filename);
#endif
	return filename;
}

static
SHCMD * 
gpg_fifo_command(char * fifofilename, char * gpg_prog, int uverbose, int logger_fd)
{
	char * absname;
	SHCMD * cmd;

	E_DEBUG("");
	cmd = shcmd_open();
	absname = shcmd_find_in_path(getenv("PATH"), gpg_prog);
	if (!absname) {
		fprintf(stderr,
			"swbis: %s: Not found in current path\n", gpg_prog);
		return NULL;
	}
	if (uverbose >= 3) {
		STROB * tmp = strob_open(32);
		swlib_writef(logger_fd, tmp,
			"%s: using GNU Privacy Guard : %s\n", swlib_utilname_get(), absname);
		strob_close(tmp);
	}
	shcmd_add_arg(cmd, absname);		/* GPG command */
	shcmd_add_arg(cmd, "--status-fd=1");

	E_DEBUG("");
	if (logger_fd != STDERR_FILENO) {
		STROB * x = strob_open(32);
		strob_sprintf(x, 0, "--logger-fd=%d", logger_fd);
		shcmd_add_arg(cmd, strob_str(x));
		strob_close(x);
	}

	shcmd_add_arg(cmd, "--verify");
	shcmd_add_arg(cmd, fifofilename); 	/* Other end of FIFO */
	shcmd_add_arg(cmd, "-");
	if (uverbose < SWC_VERBOSE_2) {
		shcmd_set_errfile(cmd, "/dev/null");
	} 
	E_DEBUG("");
	return cmd;
}

static
int
get_number_of_blobs(SWGPG_VALIDATE * w)
{
	int n = 0;
	while(strar_get(w->list_of_status_blobsM, n) != NULL) {
		n++;
	}
	return n;
}

static int
make_pgp_env(char * env[], int len) {
	int i;
	char * valu;
	STROB * tmp = strob_open(2);

	for (i=0; i<len; i++) {
		env[i] = (char*)NULL;
	}

	env[0] = strdup("PGPPASSFD=3");
	valu = getenv("HOME");
	if (valu) {
		strob_sprintf(tmp, 0, "HOME=%s", valu);
		env[1] = strdup(strob_str(tmp));
	}
	
	valu = getenv("MYNAME");
	if (valu) {
		strob_sprintf(tmp, 0, "MYNAME=%s", valu);
		env[2] = strdup(strob_str(tmp));
	}
	
	strob_close(tmp);
	return 0;
}

static
int
get_does_use_gpg_agent(SHCMD * cmd)
{
	char ** args;
	char * arg;

	args = shcmd_get_argvector(cmd);
	arg = *args;
	while (arg) {
		if (strstr(arg, "--use-agent")) return 1;
		arg = *(++args);
	}
	return 0;
}

void
swgpg_init_passphrase_fd(void)
{
	swgpg_set_passphrase_fd(-1);
}

void
swgpg_set_passphrase_fd(int fd)
{
	swgpg_passfd = fd;
}

int
swgpg_get_passphrase_fd(void)
{
	return swgpg_passfd;
}


SWGPG_VALIDATE *
swgpg_create(void)
{
	SWGPG_VALIDATE * w = (SWGPG_VALIDATE *)malloc(sizeof(SWGPG_VALIDATE));
	E_DEBUG("");
	if (!w) return NULL;
	w->gpg_prognameM = swlib_strdup(SWGPG_GPG_BIN);
	w->list_of_sigsM = strar_open();
	w->list_of_sig_namesM = strar_open();
	w->list_of_status_blobsM = strar_open();
	w->list_of_logger_blobsM = strar_open();
	w->status_arrayM = strob_open(12);
	return w;
}

void
swgpg_delete(SWGPG_VALIDATE * w)
{
	E_DEBUG("");
	free(w->gpg_prognameM);
	strar_close(w->list_of_sigsM);
	strar_close(w->list_of_sig_namesM);
	strar_close(w->list_of_status_blobsM);
	strar_close(w->list_of_logger_blobsM);
	strob_close(w->status_arrayM);
	E_DEBUG("");
	free(w);
}

int
swgpg_show_all_signatures(SWGPG_VALIDATE * w, int fd)
{
	int n;
	STROB * tmp;
	tmp = strob_open(100);
	n = 0;
	while(strar_get(w->list_of_sigsM, n) != NULL) {
		swlib_writef(fd, tmp, "%s%s",
			strar_get(w->list_of_logger_blobsM, n),
			strar_get(w->list_of_status_blobsM, n));
		n++;
	}
	strob_close(tmp);
	return 0;
}

int
swgpg_show(SWGPG_VALIDATE * w, int index, int sig_fd, int status_fd, int logger_fd)
{
	int n;
	STROB * tmp;
	int num;
	char * s;

	tmp = strob_open(100);
	num = get_number_of_blobs(w);
	E_DEBUG2("index is %d", index);
	E_DEBUG2("number of sigs is %d", num);
	if (index < 0 || index >= num) return -1; 
	n = index;

	s = strar_get(w->list_of_logger_blobsM, n);
	if (logger_fd > 0 && s)
		swlib_writef(logger_fd, tmp, "%s", s);

	s = strar_get(w->list_of_status_blobsM, n);
	if (status_fd > 0)
		swlib_writef(status_fd, tmp, "%s", s);

	s = strar_get(w->list_of_sigsM, n);
	if (sig_fd > 0)
		swlib_writef(sig_fd, tmp, "%s", s);

	strob_close(tmp);
	return 0;
}

void swgpg_reset(SWGPG_VALIDATE * swgpg)
{
	E_DEBUG("");
	strar_reset(swgpg->list_of_sigsM);
	strar_reset(swgpg->list_of_sig_namesM);
	strar_reset(swgpg->list_of_status_blobsM);
	strar_reset(swgpg->list_of_logger_blobsM);
	strob_strcpy(swgpg->status_arrayM, "");
}

int
swgpg_get_status(SWGPG_VALIDATE * w, int index)
{
	int ret;
	ret = strob_get_char(w->status_arrayM, index);
	return ret;
}

int
swgpg_get_number_of_sigs(SWGPG_VALIDATE * w)
{
	/* FIXME, this should be integrated with get_number_of_blob() above */
	int n = 0;
	E_DEBUG("");
	while(strar_get(w->list_of_sigsM, n) != NULL) {
		n++;
	}
	return n;
}

void
swgpg_set_status(SWGPG_VALIDATE * w, int index, int value)
{
	strob_set_length(w->status_arrayM, index+1);	
	*(strob_str(w->status_arrayM) + index) = (unsigned char)value;	
}

void
swgpg_set_status_array(SWGPG_VALIDATE * w)
{
	char * sig;
	int i;
	int ret;
	strob_strcpy(w->status_arrayM, "");
        i = 0;
        while((sig=strar_get(w->list_of_sigsM, i)) != NULL) {
                ret = swgpg_determine_signature_status(strar_get(w->list_of_status_blobsM, i), -1);
                swgpg_set_status(w, i, ret);
                i++;
        }
}

int
swgpg_disentangle_status_lines(SWGPG_VALIDATE * w, char * gpg_output_lines)
{
	char * line;
	STROB * tmp;
	STROB * status_lines;
	STROB * stderr_lines;

	E_DEBUG("");
	tmp = strob_open(10);
	status_lines = strob_open(10);
	stderr_lines = strob_open(10);

	line = strob_strtok(tmp, gpg_output_lines, "\n\r");
        while(line) {
		E_DEBUG("");
		if (strstr(line, GPG_STATUS_PREFIX) == line) {
			strob_sprintf(status_lines, STROB_DO_APPEND, "%s\n", line);
		} else {
			strob_sprintf(stderr_lines, STROB_DO_APPEND, "%s\n", line);
		}
		line = strob_strtok(tmp, NULL, "\n\r");
	}

	E_DEBUG("");
	strar_add(w->list_of_status_blobsM, strob_str(status_lines));
	strar_add(w->list_of_logger_blobsM, strob_str(stderr_lines));

	strob_close(tmp);
	strob_close(status_lines);
	strob_close(stderr_lines);
	E_DEBUG("");
	return 0;
}

char *
swgpg_create_fifo(STROB * buf)
{
	int ret;
	char * filename;
	char * dirname;
	char * freeit = NULL;

	E_DEBUG("");
	if (!buf) return NULL;
	if (g_swgpg_dirname != NULL) return NULL;
	if (g_swgpg_fifopath != NULL) return NULL;
	strob_strcpy(buf, "");
	dirname = make_dir(&freeit);
	if (!dirname) return NULL;
	strob_strcpy(buf, dirname);
	g_swgpg_dirname = strdup(dirname);
	swlib_unix_dircat(buf, SWGPG_FIFO_NAME);
	g_swgpg_fifopath = strdup(strob_str(buf));

	filename = g_swgpg_fifopath;
	if (filename) {
		ret = mkfifo(filename, (mode_t)(0600));
		if (ret < 0) {
			fprintf(stderr, "%s: %s\n", swlib_utilname_get(), strerror(errno));
		}
	}

	E_DEBUG("");
	if (freeit) free(freeit);
	return strob_str(buf);
}

int
swgpg_remove_fifo(void)
{
	int ret;
	E_DEBUG("");
	if (g_swgpg_dirname == NULL) return -1;
	if (g_swgpg_fifopath == NULL) return -1;
	ret = unlink(g_swgpg_fifopath);
	if (ret == 0) {
		free(g_swgpg_fifopath);
		g_swgpg_fifopath = NULL;
		ret = rmdir(g_swgpg_dirname);
		if (ret == 0) {
			free(g_swgpg_dirname);
			g_swgpg_dirname = NULL;
		}
	}
	E_DEBUG("");
	return ret;
}

int
swgpg_determine_signature_status(char * gpg_status_lines, int which_sig)
{
	STROB * tmp;
	char * line;
	int good_count;
	int bad_count;
	int nodata;
	int nokey;

	E_DEBUG("----------------------------------");
	E_DEBUG("----------------------------------");
	
	E_DEBUG2("%s", gpg_status_lines);

	E_DEBUG("----------------------------------");
	E_DEBUG("----------------------------------");
	tmp = strob_open(100);

	good_count=0;
	nodata=0;
	nokey=0;
	bad_count=0;
	line = strob_strtok(tmp, gpg_status_lines, "\n\r");
	E_DEBUG("");
        while(line) {
		/* fprintf(stderr, "%s\n", line); */
		E_DEBUG2("LINE=[%s]", line);
		if (strstr(line, GPG_STATUS_PREFIX) == line) {
			if (strstr(line, GPG_STATUS_PREFIX GPG_STATUS_GOODSIG)) {
				E_DEBUG("good_count++");
				good_count++;
			} else if (strstr(line, GPG_STATUS_BADSIG)) {
				E_DEBUG("bad_count++");
				bad_count++;
			} else if (strstr(line, GPG_STATUS_NO_PUBKEY)) {
				E_DEBUG("nokey++");
				nokey++;
			} else if (strstr(line, GPG_STATUS_EXPSIG)) {
				E_DEBUG("bad_count++");
				bad_count++;
			} else if (strstr(line, GPG_STATUS_NODATA)) {
				E_DEBUG("no_data++");
				nodata++;
			} else {
				; /* allow all other lines ?? */
			}
		} else {
			;
			E_DEBUG("do_nothing");
			/* now stderr is mixed this output so just
			   ignore other lines */
			/* return SWGPG_SIG_ERROR; */
		}
		line = strob_strtok(tmp, NULL, "\n\r");
        }
	E_DEBUG("");
	strob_close(tmp);
	if (
		(which_sig < 0 && good_count >= 1 && bad_count == 0 && nokey == 0 && nodata == 0) ||
		(which_sig >= 0 && good_count == 1 && bad_count == 0 && nokey == 0 && nodata == 0)
	) {
		E_DEBUG("return SWGPG_SIG_VALID");
		return SWGPG_SIG_VALID;
	} else if (good_count == 0 && bad_count == 0 && nokey > 0) {
		E_DEBUG("return SWGPG_SIG_NO_PUBKEY");
		return SWGPG_SIG_NO_PUBKEY;
	} else if (good_count == 0 && bad_count == 0 && nodata > 0) {
		E_DEBUG("return SWGPG_SIG_NODATA");
		return SWGPG_SIG_NODATA;
	} else {
		E_DEBUG("return SWGPG_SIG_NOT_VALID");
		return SWGPG_SIG_NOT_VALID;
	}
	E_DEBUG("return SWGPG_SIG_ERROR");
	return SWGPG_SIG_ERROR;
}

int
swgpg_run_checksig2(char * sigfilename, char * thisprog,
		char * filearg, char * gpg_prog, int uverbose,
		char * which_sig_arg)
{
	pid_t pid;
	int ret;
	int retval;
	int sbfd[2];
	int logger_fd;
	int status_fd;
	int status;
	SWGPG_VALIDATE * w;
	SHCMD * cmd[3];
	int u_verbose = uverbose;
	STROB * tmp = strob_open(30);
	int atoiret=1;
	int which_sig;

	if (which_sig_arg) {
		which_sig = swlib_atoi(which_sig_arg, &atoiret);
		if (atoiret) return -3;  /* never should happen */
	} else {
		which_sig = -1;
	}

	cmd[0] = shcmd_open();
	cmd[1] = shcmd_open();
	cmd[2] = NULL;
	E_DEBUG("");

	retval = 1;
	pipe(sbfd);
	pid = swfork(NULL);
	if (pid == 0) {
		close(sbfd[0]);
		E_DEBUG("");
		swgp_signal(SIGPIPE, SIG_DFL);
		swgp_signal(SIGINT, SIG_DFL);
		swgp_signal(SIGTERM, SIG_DFL);
		swgp_signal(SIGUSR1, SIG_DFL);
		swgp_signal(SIGUSR2, SIG_DFL);
		shcmd_add_arg(cmd[0], thisprog);  
		strob_sprintf(tmp, 0, "--util-name=%s", swlib_utilname_get());
		shcmd_add_arg(cmd[0], strob_str(tmp));
		shcmd_add_arg(cmd[0], "-G");
		shcmd_add_arg(cmd[0], sigfilename); /* FIFO or /dev/tty */
		shcmd_add_arg(cmd[0], "-n");
		shcmd_add_arg(cmd[0], which_sig_arg);
		while (u_verbose > 1) {
			shcmd_add_arg(cmd[0], "-v");
			u_verbose--;
		}
		shcmd_add_arg(cmd[0], "--sleep");
		shcmd_add_arg(cmd[0], "1");
		/* sbfd[1] is the write fd for the signed bytes */
		strob_sprintf(tmp, 0, "--signed-bytes-fd=%d", sbfd[1]);
		shcmd_add_arg(cmd[0], strob_str(tmp));
		strob_sprintf(tmp, 0, "--logger-fd=%d", STDOUT_FILENO);
		shcmd_add_arg(cmd[0], strob_str(tmp));
		shcmd_add_arg(cmd[0], filearg); /* may be a "-" for stdin */
		E_DEBUG("");
		shcmd_unix_exec(cmd[0]);
		E_DEBUG("");
		fprintf(stderr, "exec error in swgp_run_checksig\n");
		_exit(1);
	} else if (pid < 0) {
		E_DEBUG("");
		retval = 1;
		close(sbfd[1]);
		goto out;
	} 
	close(sbfd[1]);

	E_DEBUG("");
	cmd[1] = gpg_fifo_command(sigfilename, gpg_prog, uverbose, STDOUT_FILENO);
	if (cmd[1] == NULL) {
		E_DEBUG("");
		retval = 1;	
		kill(pid, SIGTERM);
		waitpid(pid, &status, 0);
		goto out;
	}

	E_DEBUG("");
	strob_strcpy(tmp, "");
	swlib_exec_filter(cmd+1, sbfd[0], tmp);
	E_DEBUG("");

	ret = swgpg_determine_signature_status(strob_str(tmp), which_sig);
	retval = ret;
	E_DEBUG2("signature status is %d", ret);

	E_DEBUG2("status line: %s", strob_str(tmp));
	w = swgpg_create();
	E_DEBUG("");
	swgpg_disentangle_status_lines(w, strob_str(tmp));
	E_DEBUG("");
	logger_fd = -1;
	status_fd = -1;
	if (retval) {
		status_fd = STDERR_FILENO;
		logger_fd = STDERR_FILENO;
	} else {
		if (uverbose >= SWC_VERBOSE_2) {
			logger_fd = STDOUT_FILENO;
		} 
		if (uverbose >= SWC_VERBOSE_3) {
			logger_fd = STDOUT_FILENO;
			status_fd = STDOUT_FILENO;
		}
	}
	E_DEBUG3("show: status_fd=%d, logger_fd=%d", status_fd, logger_fd);
	swgpg_show(w, 0, -1, status_fd, logger_fd);
	E_DEBUG("");
	swgpg_delete(w);	
out:
	if (cmd[0]) shcmd_close(cmd[0]);
	if (cmd[1]) shcmd_close(cmd[1]);
	close(sbfd[0]);
	strob_close(tmp);
	E_DEBUG2("retval=%d", retval);
	return retval;
}

int
swgpg_run_gpg_verify(SWGPG_VALIDATE * swgpg, int signed_bytes_fd, char * signature, int uverbose, STROB * gpg_status)
{
	SHCMD * gpg_cmd;
	SHCMD * cmdvec[2];
	STROB * fifo;
	STROB * output;
	char * fifo_path;
	int ret;
	int ret1;
	int ret2;
	int ret3;
	pid_t pid;
	int status;

	E_DEBUG("");
	E_DEBUG2("signed_bytes_fd = %d", signed_bytes_fd);
	fifo = strob_open(32);
	if (gpg_status == NULL)
		output = strob_open(32);
	else
		output = gpg_status;

	cmdvec[0] = NULL;
	cmdvec[1] = NULL;
	E_DEBUG("");
	fifo_path = swgpg_create_fifo(fifo);
	if (fifo_path == NULL) return -1;

	E_DEBUG("");
	gpg_cmd = gpg_fifo_command(fifo_path, swgpg->gpg_prognameM, uverbose, STDOUT_FILENO);
	E_DEBUG("");
	if (gpg_cmd == NULL) {
		swgpg_remove_fifo();
		return -1;
	}
	cmdvec[0] = gpg_cmd;
	
	E_DEBUG("");
	pid = fork();
	if (pid == 0) {
		int ffd;
		close(0);
		close(1); 
		ffd = open(fifo_path, O_WRONLY);
		if (ffd < 0) _exit(1);
		ret = uxfio_unix_safe_write(ffd, signature, strlen(signature));
		if (ret != (int)strlen(signature)) _exit(2);
		close(ffd);
		_exit(0);
	} else if (pid < 0) {
		return -1;
	}
	E_DEBUG("");
	strob_strcpy(output, "");
	ret1 = swlib_exec_filter(cmdvec, signed_bytes_fd, output);

	E_DEBUG("");
	if (ret1 == SHCMD_UNSET_EXITVAL) {
		/* FIXME abnormal exit for gpg, don't no why */
		E_DEBUG2("swlib_exec_filter ret=%d", ret1);
		ret1 = 0;
	}

	E_DEBUG2("swlib_exec_filter ret1=%d", ret1);

	ret2 = swgpg_determine_signature_status(strob_str(output), -1);
	E_DEBUG2("swgpg_determine_signature_status ret2=%d", ret2);

	ret3 = waitpid(pid, &status, 0);
	if (ret3 < 0) {
		/*
		fprintf(stderr, "waitpid error: %s\n", strerror(errno));
		*/
		;
	} else if (ret3 == 0) {
		ret3 = 2;
	} else {
        	if (WIFEXITED(status)) {
			ret3 = WEXITSTATUS(status);
			E_DEBUG2("exit value = %d", ret3);
		} else {
			ret3 = 1;
		}
	}

	swgpg_remove_fifo();
	strob_close(fifo);
	if (gpg_status == NULL)
		strob_close(output);

	E_DEBUG2("RESULT ret1 = %d", ret1);
	E_DEBUG2("RESULT ret2 = %d", ret2);
	E_DEBUG2("RESULT ret3 = %d", ret3);
	if (ret1 == 0 && ret2 == 0 && ret3 == 0) {
		E_DEBUG("RETURNING returning 0");
		return 0;
	} else {
		E_DEBUG("RETURNING returning 1");
		return 1;
	}
}

SHCMD * 
swgpg_get_package_signature_command(
		char * signer, 
		char * gpg_name, 
		char * gpg_path,
		char * passphrase_fd
) {
	static char * env[10];
	SHCMD * sigcmd;
	char * envpath;
	char * signerpath;

	sigcmd = shcmd_open();
	env[0] = (char *)NULL;
	envpath = getenv("PATH");	
	if (
		strcasecmp(signer, "GPG") == 0 ||
		strcasecmp(signer, "GPG2") == 0
	) {
		if (strcasecmp(signer, "GPG2") == 0) {
			signerpath = shcmd_find_in_path(envpath, SWGPG_GPG2_BIN);
		} else {
			signerpath = shcmd_find_in_path(envpath, SWGPG_GPG_BIN);
		}
		if (signerpath == NULL) return NULL;
		shcmd_add_arg(sigcmd, signerpath);
		if (gpg_name && strlen(gpg_name)) {
			shcmd_add_arg(sigcmd, "-u");
			shcmd_add_arg(sigcmd, gpg_name);
		}
		if (gpg_path && strlen(gpg_path)) {
			shcmd_add_arg(sigcmd, "--homedir");
			shcmd_add_arg(sigcmd, gpg_path);
		}
		shcmd_add_arg(sigcmd, "--no-tty");
		shcmd_add_arg(sigcmd, "--no-secmem-warning");
		shcmd_add_arg(sigcmd, "--armor");
		if (
			passphrase_fd != NULL &&
			strcmp(passphrase_fd, SWGPG_SWP_PASS_AGENT) == 0
		) {
			shcmd_add_arg(sigcmd, "--use-agent");
		} else {	
			shcmd_add_arg(sigcmd, "--passphrase-fd");
			shcmd_add_arg(sigcmd, "3");
		}
		shcmd_add_arg(sigcmd, "-sb");
		shcmd_add_arg(sigcmd, "-o");
		shcmd_add_arg(sigcmd, "-");
	} else if (strcasecmp(signer, "PGP2.6") == 0) {
		make_pgp_env(env, sizeof(env)/sizeof(char*));
		shcmd_set_envp(sigcmd, env);
		signerpath = shcmd_find_in_path(envpath, SWGPG_PGP26_BIN);
		if (signerpath == NULL) return NULL;
		shcmd_add_arg(sigcmd, signerpath);
		if (gpg_name && strlen(gpg_name)) {
			shcmd_add_arg(sigcmd, "-u");
			shcmd_add_arg(sigcmd, gpg_name);
		}
		shcmd_add_arg(sigcmd, "+armor=on");
		shcmd_add_arg(sigcmd, "-sb");
		shcmd_add_arg(sigcmd, "-o");
		shcmd_add_arg(sigcmd, "-");
	} else if (strcasecmp(signer, "PGP5") == 0) {
		make_pgp_env(env, sizeof(env)/sizeof(char*));
		shcmd_set_envp(sigcmd, env);
		signerpath = shcmd_find_in_path(envpath, SWGPG_PGP5_BIN);
		if (signerpath == NULL) return NULL;
		shcmd_add_arg(sigcmd, signerpath);
		if (gpg_name && strlen(gpg_name)) {
			shcmd_add_arg(sigcmd, "-u");
			shcmd_add_arg(sigcmd, gpg_name);
		}
		shcmd_add_arg(sigcmd, "-ab");
		shcmd_add_arg(sigcmd, "-o");
		shcmd_add_arg(sigcmd, "-");
	} else {
		shcmd_close(sigcmd);
		sigcmd = NULL;
	}
	return sigcmd;
}

char *
swgpg_get_package_signature(
		SHCMD * sigcmd,
		int * statusp,
		char * wopt_passphrase_fd,
		char * passfile,
		int pkg_fd,
		int do_dummy_sign,
		char * g_passphrase
		)
{
	SHCMD *cmd[2];	
	int verboseG = 1;
	char * sig;
	int does_use_agent;
	int ret;
	int atoiret;
	int opt_passphrase_fd;
	pid_t pid[6];
	int status[6];
	int passphrase[2];
	int input[2];
	int output[2];
	int filter[2];
	int sigfd;
	char * fdmem;
	char nullbyte[2];

	*statusp = 255;
	cmd[1] = (SHCMD*)(NULL);
	nullbyte[0] = '\0';

	if (do_dummy_sign) {
		sig = (char*)malloc(ARMORED_SIGLEN);
		memset(sig, '\0', ARMORED_SIGLEN);
		strcat(sig, 
			"-----BEGIN DUMMY SIGNATURE-----\n"
			"Version: swpackage (swbis) " SWPACKAGE_VERSION "\n"
			"dummy signature made using the --dummy-sign option of\n"
			"the swpackage(8) utility.  Not intended for verification.\n"
			"-----END DUMMY SIGNATURE-----\n");
		return sig;
	}

	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
		"Generating package signature ....\n");

	if (sigcmd == NULL) {
		return (char*)NULL;
	} else {
		cmd[0] = sigcmd;
	}

	if (pipe(input)) exit(4);
	if (pipe(output)) exit(4);
	if (pipe(passphrase)) exit(4);
	pipe(filter);

	E_DEBUG("");
	does_use_agent = get_does_use_gpg_agent(cmd[0]);

	/* shcmd_cmdvec_debug_show_to_file(cmd, stderr); 
	 *
	 * Exec Signer.
	 */
	pid[0] = swfork((sigset_t*)(NULL));
	if (pid[0] < 0) exit(5);
	if (pid[0] == 0) 
	{
		close_passfd();
		close(filter[1]);
		close(output[0]);
		shcmd_set_srcfd(cmd[0], filter[0]);
		shcmd_set_dstfd(cmd[0], output[1]);
		shcmd_set_errfile(cmd[0], "/dev/null");
		shcmd_apply_redirection(cmd[0]);
		if (does_use_agent == 0)
			dup2(passphrase[0], 3);
		close(passphrase[1]);
		close(passphrase[0]);
		if (does_use_agent == 0)
			swgp_close_all_fd(4);
		else
			swgp_close_all_fd(3);
		shcmd_unix_exec(cmd[0]);
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"%s not run.\n", cmd[0]->argv_[0]);
		_exit(2); 
	}
	close(output[1]);
	close(filter[0]);
	close(passphrase[0]);

	/*
	 * Ask Passphrase
	 */
	E_DEBUG("");
	pid[1] = swfork((sigset_t*)(NULL));
	if (pid[1] < 0) return (char*)NULL;
	if (pid[1] == 0) {
		close(input[1]);
		close(output[0]);
		if (
			does_use_agent == 0 &&
			strcmp(wopt_passphrase_fd, "-1") == 0 &&
			strcmp(wopt_passphrase_fd, SWGPG_SWP_PASS_ENV) != 0 &&
			strcmp(wopt_passphrase_fd, SWGPG_SWP_PASS_AGENT) != 0 &&
			passfile == (char*)NULL
		) {
			/*
			 * Not using the GPG agent or file desctiptor
			 * of swspackage, therefore, use the getpass
			 * routine which gets the passphrase from tty
			 */
			char pbuf[SWGPG_PASSPHRASE_LENGTH];
			char * pass;
			pass = fm_getpassphrase(
				"Enter Password: ", pbuf, sizeof(pbuf));
			if (pass) {
				pbuf[sizeof(pbuf) - 1] = '\0';
				write(passphrase[1], pass, strlen(pass));
			} else {
				write(passphrase[1], "\n\n", 2);
			}
			memset(pbuf, '\x00', sizeof(pbuf));
			memset(pbuf, '\xff', sizeof(pbuf));
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"swpackage: close: %s\n", strerror(errno));
			}
		} else if (
			does_use_agent == 0 &&
			strcmp(wopt_passphrase_fd, SWGPG_SWP_PASS_AGENT) != 0 &&
			strcmp(wopt_passphrase_fd, SWGPG_SWP_PASS_ENV) != 0
		) {
			/*
			 * Here, passfile may have been given or
			 * wopt_passphrase_fd was given.
			 */
			int did_open = 0;
			int buf[512];
			if (passfile && 
				strcmp(passfile, "-")) {
				opt_passphrase_fd = open(passfile, 
							O_RDONLY, 0);
				if (opt_passphrase_fd < 0) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"passphrase file not found\n");
				}
				did_open = 1;
			} else if (passfile && 
				strcmp(passfile, "-") == 0) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"warning: unsafe use of stdin\n"
					"warning: use --passphrase-fd=0 instead\n");
				opt_passphrase_fd = STDIN_FILENO;
			} else {
				opt_passphrase_fd = swlib_atoi(wopt_passphrase_fd, &atoiret);
				if (atoiret) return (char*)(NULL);
			}
			if (opt_passphrase_fd >= 0) {
				E_DEBUG2("reading opt_passphrase_fd=%d", opt_passphrase_fd);
				ret = read(opt_passphrase_fd, (void*)buf, sizeof(buf) - 1);
				E_DEBUG2("read of passphrase ret=%d", ret);
				if (ret < 0 || ret >= 511) { 
					memset(buf, '\0', sizeof(buf));
					memset(buf, '\xff', sizeof(buf));
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"read (loc=p): %s\n", strerror(errno));
					close(opt_passphrase_fd);
					close(passphrase[1]);
					_exit(1);
				}
				if (did_open) close(opt_passphrase_fd);
				buf[ret] = '\0'; 
				write(passphrase[1], buf, ret);
				memset(buf, '\0', sizeof(buf));
				memset(buf, '\xff', sizeof(buf));
			}
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		} else if (
			does_use_agent == 1 &&
			strcmp(wopt_passphrase_fd, SWGPG_SWP_PASS_AGENT) == 0
		) {
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		} else if (
			does_use_agent == 0 &&
			strcmp(wopt_passphrase_fd, SWGPG_SWP_PASS_ENV) == 0 &&
			g_passphrase
		) {
			write(passphrase[1], g_passphrase, strlen(g_passphrase));
			write(passphrase[1], "\n", 1);
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		} else {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"internal error get_package_signature\n");
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		}
		_exit(0);
	}
	/* 
	 * Close the passphrase in the parent.
	 */
	E_DEBUG("");
	close_passfd();
	close(passphrase[1]);
					/* filter[1] --> */
	/*
	 * Decode signing stream.
	 */
		/* FIXME this fork is not required */
	pid[2] = swfork((sigset_t*)(NULL));
	if (pid[2] < 0) return (char *)NULL;
	if (pid[2] == 0) {
		close_passfd();
		close(input[1]);
		swlib_pipe_pump(filter[1], input[0]);
		close(input[0]);
		close(filter[1]);
		_exit(0);
	}
	close(input[0]);
	close(filter[1]);
	E_DEBUG("");
					/* <<-- input[0] */
					/* input[1] --> */
	/*
	 * Write signed stream
	 */
	E_DEBUG("");
	pid[3] = swfork((sigset_t*)(NULL));
	if (pid[3] < 0) return (char *)NULL;
	if (pid[3] == 0) {
		int ofd1;
		close_passfd();
		close(input[0]);

		ofd1 =  uxfio_opendup(input[1], UXFIO_BUFTYPE_NOBUF);
		uxfio_fcntl(ofd1, 
				UXFIO_F_SET_OUTPUT_BLOCK_SIZE, 
				PIPE_BUF);

		swlib_pipe_pump(ofd1, pkg_fd);

		uxfio_close(ofd1);
		close(output[0]);
		_exit(0);
	}
	close(input[1]);

	E_DEBUG("");
	sigfd = swlib_open_memfd();
	
	ret = swlib_pump_amount(sigfd, output[0], 1024);
	close(output[0]);
	if (ret < 0 || ret > 1000) return (char*)NULL;
	uxfio_write(sigfd, (void*)nullbyte, 1);	

	E_DEBUG("");
	if (swlib_wait_on_all_pids(pid, 4, status, 
				WNOHANG, verboseG - 2) < 0) {
		return NULL;
		/* FIXME return error */
		/* swexdist->setErrorCode(18004, NULL); */
	}

	E_DEBUG("");
	if (pid[0] < 0) {
		ret =  WEXITSTATUS(status[0]);
	} else {
		ret = 100;
	}
	*statusp = ret;
	if (ret) return (char*)NULL;

	E_DEBUG("");
	uxfio_get_dynamic_buffer(sigfd, &fdmem, (int*)NULL, (int*)NULL);
	sig = strdup(fdmem);
	uxfio_close(sigfd);	
	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
		"Generating package signature .... Done.\n");
	E_DEBUG("");
	return sig;
}
