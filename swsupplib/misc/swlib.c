/* swlib.c - General Purpose Routines.
 */

/*
   Copyright (C) 1998-2005,2007,2011  James H. Lowe, Jr.
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
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>		/* needed for mkdir(2) prototype! */
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "misc-fnmatch.h"
#include "taru.h"
#include "uxfio.h"
#include "strob.h"
#include "swfork.h"
#include "shcmd.h"
#include "swpath.h"
#include "swlib.h"
#include "md5.h"
#include "ugetopt_help.h"
#include "atomicio.h"
#include "swutillib.h"
#include "swutilname.h"
#include "inttostr.h"
#include "strtoint.h"
#include "sha.h"
#include "sha512.h"
/*#include "config_remains.h" */

static int verbose_levelG;
static int verbose_swbis_event_thresholdG = SWC_VERBOSE_4;

static uintmax_t * g_pstatbytes = NULL;
static struct timespec * io_req = (struct timespec*)NULL;
static int g_burst_adjust = 65000;

static pid_t g_pax_header_pidG = 0;


void
swlib_set_pax_header_pid(pid_t n)
{
	g_pax_header_pidG = n;
}

pid_t
swlib_get_pax_header_pid(void)
{
	if (g_pax_header_pidG == 0) {
		g_pax_header_pidG = getpid();
	}
	return g_pax_header_pidG;
}


static
ssize_t
synct_i_no_null_write(int fd, char * buf, int sz)
{
	int ret;
	int remains;
	int excl_region;
	int amount;
	int retval;
	char * p;
	char * x;

	remains = sz;
	p = buf;
	retval = 0;
	
	/* buf assumed to be and is NUL terminated */
	while (remains > 0) {
		amount = 0;
		x = p;
	
		/* find the length of potentially not-NUL terminated region */
		while((int)(x - buf) < sz && *x) {
			amount++;
			x++;
		}
		if (amount > sz) amount = sz;
		excl_region = remains - amount;
		E_DEBUG3("amount=%d excl_region=%d", amount, excl_region);
		ret = atomicio(uxfio_write, fd, p, amount);
		if (ret < 0) return -1;
		if (ret != amount) return -2;
		p += ret;
		remains -= ret;
		retval += ret;
		while (excl_region > 0 && *p == '\0') {
			excl_region--;
			remains--;
			p++;
		}
	}
	return retval;
}
 	
static
ssize_t
synct_i_read_block(SYNCT * synct, ssize_t (*iof) (int, void *, size_t), int fd, char * buf)
{
	ssize_t ret;
	E_DEBUG2("fd=%d", fd);
	ret = (iof) (fd, buf, SWLIB_SYNCT_BLOCKSIZE);
	if (synct->debugfdM > 0) write(synct->debugfdM, buf, ret);
	E_DEBUG("");
	if (ret != SWLIB_SYNCT_BLOCKSIZE) {
		fprintf(stderr, "%s: synct_i_read_block() error: ret=%d\n", swlib_utilname_get(), (int)ret);
		if (ret < 0) return -1;
		return -ret;
	}
	E_DEBUG("");
	synct->countM++;
	return ret;
}

static
int
synct_check_for_possible_eof(SYNCT * synct)
{
	unsigned char * ibuf;
	unsigned char * pos;
	int i;

	ibuf = synct->bufM;
	pos = ibuf;
	i = 0;

	E_DEBUG("");
	while (
		(int)(pos - ibuf) < SWLIB_SYNCT_BLOCKSIZE &&
		synct->mtM[i] != '\0'
	) {

	/* 
	 *03MAR2014: BUG workaround:  for reason not known,
	 * the TRAILER string has the final \r\n pair truncated
	 * on rare oocasion. Hence to workaround, relax the requirement to only
	 * look for TRAILER!!!\r\n not TRAILER!!!\r\n\r\n
	 */     

		if (*pos && *pos == synct->mtM[i]) {
			/* so far so good */
			E_DEBUG("finding trailer string, so far so good");
			i++;
		} else if (*pos == '\0' && i > 11) {
			/*
			 * Found mangled TRAILER!!!\r\n\0
			 * Accept this as a bug work around.
			 * This happens about 1% of the time and is the
			 * source of program hangs when accessing the catalog.
			 */
			return SYNCT_EOF_CONDITION_2;
		} else {
			/* reset, bad match */
			E_DEBUG("reset, bad match of trailer string");
			return SYNCT_EOF_CONDITION_0;
		}
		pos++;
	}

	if (synct->mtM[i] == '\0') {
		/* Found the entire trailer string */
		E_DEBUG("found trailer");
		return SYNCT_EOF_CONDITION_1;
	} else {
		E_DEBUG("not found");
		return SYNCT_EOF_CONDITION_0;
	}
}

void
swlib_set_swbis_verbose_threshold(int s)
{
	verbose_swbis_event_thresholdG = s;
}

int
swlib_get_nopen(void)
{
	int nopen = 0;
	int ret;
	int i;
	for (i = 0; i < (200); ++i) {
		ret = fcntl(i, F_GETFL);
		if (ret < 0 && errno == EBADF) {
			;
		} else {
			nopen++;
		}
	}
	return nopen;
}

void
e_msg(char * class, char * reason, char * file,
			int line, char * function)
{
	fprintf(stderr, "%s: %s: %s: at %s:%d\n",
		 swlib_utilname_get(), class, reason, file, line);
}

static
int
convert_hex_seq(int d1, int d2)
{
	int n;
	n = d1;	
	if ( isdigit(n) == 0 &&  (n < '\x41' || n > '\x46')) { return -1; }
	if (isdigit(n) == 0) d1 -= 7;
	n = d2;	
	if ( isdigit(n) == 0 &&  (n < '\x41' || n > '\x46')) { return -1; }
	if (isdigit(n) == 0) d2 -= 7;
	return (16 * (d1 - 48)) + (d2 - 48);
}

static
char *
does_have_hex_escape(char * src, int * value)
{
	char * t;
	char *seq;
	int d1;
	int d2;
	
	*value = 0;
	t = strstr(src, "\\x");
	if (t && (t == src || *(t-1) != '\\')) {
		seq = t+2;
		if (*seq == '\0') return (char*)NULL;
		if (*(seq+1) == '\0') return (char*)NULL;
		d1 = toupper((int)(*seq));
		d2 = toupper((int)(*(seq+1)));
		*value = convert_hex_seq(d1, d2);
		if (*value < 0) {
			return (char*)NULL;
		} else {
			return t;
		}
	}
	return (char*)NULL;
}

static
void
process_all_hex_escapes(char * src)
{
	unsigned char * s;
	unsigned char * r;
	int value;

	s = (unsigned char*)src;
	while((r = (unsigned char*)does_have_hex_escape((char*)s, &value))) {
		memmove((void*)(r+1), (void*)(r+4), strlen((char*)(r+4)) + 1);
		*r = (unsigned char)(value);
		s = (r+1);
	}
}

int
swlib_doif_writeap(int fd, STROB * buffer, char * format, va_list * pap)
{
	int ret;
	int aret;
	if (fd < 0) return 0;
	ret = strob_vsprintf(buffer, 1, format, *pap);
	if (ret < 0) {
		return -1;
	}
	ret = strob_strlen(buffer);
	aret = atomicio(uxfio_write, fd, 
		(void*)strob_str(buffer), strob_strlen(buffer));

	if (aret != ret) return -1;	
	return ret;
}

static
int
nano_nanosleep(long nsec)
{
	struct timespec tt;
	tt.tv_sec = 0;
	tt.tv_nsec = nsec;
	return nanosleep(&tt, NULL);
}

static
void
delay_sleep_pattern2(struct timespec * io_req, int * sleepbytes, int c_amount, int  byteswritten)
{
	static int doneone = 0;
	const int leaddiv = 0;
	int openssh_rcvd_adjust = g_burst_adjust;
	if ( 
		*sleepbytes > openssh_rcvd_adjust && 
		(
			leaddiv == 0 ||
			(
				/* disabled */
			byteswritten > (c_amount/leaddiv) &&
			byteswritten < (c_amount - c_amount/leaddiv)
			)
		)
	)
 	{
		if (
			(verbose_levelG >= SWC_VERBOSE_3 && !doneone) ||
			(verbose_levelG >= SWC_VERBOSE_8)
		) {
			doneone = 1;
			fprintf(stderr,
				"%s: sshd efd race delay: sleeping %d microseconds every %d bytes\n",
					swlib_utilname_get(), (int)(io_req->tv_nsec)/1000, g_burst_adjust);
			fprintf(stderr,
				"%s: sshd efd race delay: is not needed for openssh >=5.1\n", swlib_utilname_get());
		}
		nano_nanosleep(io_req->tv_nsec);
		*sleepbytes = 0;
	}
}

static
int
form_abspath(STROB * sb1, char * s1, char * pcwd)
{
	char cwd[200];
	if (*s1 == '/') return 1;
	if (pcwd == NULL) {
		if (getcwd(cwd, sizeof(cwd) - 2) == NULL) {
			SWLIB_FATAL("path too long");
		}
	} else {
		strncpy(cwd, pcwd, sizeof(cwd) - 2);
	}
	cwd[sizeof(cwd) - 1] = '\0';
	strob_strcpy(sb1, cwd);
	swlib_unix_dircat(sb1, s1);
	return 0;
}

static
intmax_t
swlib_i_digs_copy(int ofd, int ifd, intmax_t count, FILE_DIGS * digs, int adjunct_ofd, ssize_t (*f) (int, void *, size_t))
{
	intmax_t n;
	intmax_t am;
	intmax_t wr;
	unsigned char buf[SHA_BLOCKSIZE];
	intmax_t blocksize = SHA_BLOCKSIZE;  /* this must match the sha_block size */
	intmax_t amount;
	intmax_t readamount;
	intmax_t ret;
	intmax_t retval = 0;
	int i;
	char * p5;
	char * p;
	MD5_CTX md5ctx;
	int digest_hex_bytes = 40;
	int sha512digest_hex_bytes = 128;
	unsigned char res_sha1[21];  /* binary result */
	unsigned char res_sha512[65];  /* binary result */
	unsigned char sha1ctx[512];
	unsigned char sha512ctx[512];
	int sleepbytes = 0;
	int timebytes = 0;
	time_t newt;
	time_t oldt;
	
	oldt = newt = time(NULL);

	if (digs && digs->do_md5 == DIGS_ENABLE_ON)
		swlib_md5_from_memblocks(&md5ctx, digs->md5, (unsigned char*)NULL, -1);

	if (digs && digs->do_sha1 == DIGS_ENABLE_ON)
		sha_block((void*)sha1ctx, res_sha1, NULL, -1);

	if (digs && digs->do_sha512 == DIGS_ENABLE_ON)
		sha512_block((void*)sha512ctx, res_sha512, NULL, -1);

	do {
		if (count >= 0)
			readamount = (count - retval) > blocksize ? blocksize : (count - retval);
		else
			readamount = blocksize;

		n = (*f)(ifd, buf, readamount);
		if (n < 0) return -1;
		if (n) {
			if (digs && digs->do_md5 == DIGS_ENABLE_ON) {
				p5 = (char*)buf;
				am = n;
				while(am > 0) {
					amount = am > 1024 ?  1024 : am;
					swlib_md5_from_memblocks(&md5ctx, digs->md5,
						 (unsigned char*)p5, amount);
					p5 += amount;
					am -= amount;
				}
			}
	
			if (digs && digs->do_sha1 == DIGS_ENABLE_ON)
				sha_block((void*)sha1ctx, res_sha1, buf, n);
	
			if (digs && digs->do_sha512 == DIGS_ENABLE_ON)
				sha512_block((void*)sha512ctx, res_sha512, buf, n);

			if (ofd > 0) {
				wr = (intmax_t)atomicio((ssize_t (*)(int, void *, size_t))(uxfio_write),
					ofd, buf, n);
				if (wr < 0) return -1;
			} else {
				wr = n;
			}
			retval += wr;

			if (adjunct_ofd > 0) {
				ret = atomicio((ssize_t (*)(int, void *, size_t))(uxfio_write),
					adjunct_ofd, buf, n);
				if (ret < 0) return -1;
			}

			timebytes += wr;

			E_DEBUG("");
			if (io_req && io_req->tv_nsec) {
				/*
				* This is the bug delay
				*/
				E_DEBUG("");
				sleepbytes += wr;
				delay_sleep_pattern2(io_req, &sleepbytes, count, retval);
			}

			E_DEBUG("");
			if (g_pstatbytes && timebytes > 10000) {
				/*
				* Reduce the number of checks to see if it
				* time to update the progress bar.
				*/
				E_DEBUG("");
				if ((newt=time(NULL)) > oldt) {
					timebytes = 0;
					oldt = newt;
					*g_pstatbytes = retval;
					(void)update_progress_meter(SIGALRM);
       		         	}
			}
		}
		if (n == 0) break;
	} while (retval < count || count < 0);
	if (n < 0) return -1;

	if (digs && digs->do_md5 == DIGS_ENABLE_ON) {
		swlib_md5_from_memblocks(&md5ctx, digs->md5, (unsigned char*)NULL, 0);
		digs->md5[32] = '\0';
	}

	if (digs && digs->do_sha1 == DIGS_ENABLE_ON)
		sha_block((void*)sha1ctx, res_sha1, (unsigned char*)NULL, 0); 

	if (digs && digs->do_sha512 == DIGS_ENABLE_ON)
		sha512_block((void*)sha512ctx, res_sha512, (unsigned char*)NULL, 0); 

	if (digs && digs->do_size) {
		STROB * tmp;
		STROB * ubuf;
		tmp = strob_open(24);
		ubuf = strob_open(24);
		strob_sprintf(tmp, 0, "%s", swlib_imaxtostr(retval, ubuf));
		swlib_strncpy(digs->size, strob_str(tmp), sizeof(digs->size));
		strob_close(ubuf);
		strob_close(tmp);
	}

	if (digs && digs->do_sha1 == DIGS_ENABLE_ON) {
		p = digs->sha1;
		for (i = 0; i < (digest_hex_bytes / 2); ++i) {
			sprintf(p,"%02x", res_sha1[i]);
			p += 2;
		}
		digs->sha1[digest_hex_bytes] = '\0';
		digs->sha1[40] = '\0';
	}

	if (digs && digs->do_sha512 == DIGS_ENABLE_ON) {
		p = digs->sha512;
		for (i = 0; i < (sha512digest_hex_bytes / 2); ++i) {
			sprintf(p,"%02x", res_sha512[i]);
			p += 2;
		}
		digs->sha512[sha512digest_hex_bytes] = '\0';
	}
	return retval;
}

int
squash_trailing_char(char *path, char ch, int min_len)
{
	if (path && ((int)strlen(path) > min_len ) && (*(path + strlen(path) - 1)) == (int)ch) {
		(*(path + strlen(path) - 1)) = '\0';
		return 0;
	} else {
		return 1;
	}
}


int
swlib_atoi(const char *nptr, int * result)
{
	return swlib_atoi2(nptr, (char**)(NULL), result);
}

int
swlib_atoi2(const char *nptr, char ** fp_endptr, int * result)
{
	long ret;
	char * endptr;
	char ** a_endptr;
	
	if (fp_endptr)
		a_endptr = fp_endptr;
	else
		a_endptr = &endptr;

	ret = strtol(nptr, a_endptr, 10);

	if (result) *result = 0;
	if (
		(*a_endptr == nptr && ret == 0) ||
		((ret == LONG_MIN || ret == LONG_MAX) && errno == ERANGE)
	) {
		if (result) {
			*result = 1;
		}
		fprintf(stderr, "%s: strtol error when converting [%s]\n", swlib_utilname_get(), nptr);
	}
	return (int)ret;
}

unsigned long int
swlib_atoul(const char *nptr, int * result, char ** fp_endptr)
{
	unsigned long int ret;
	char * endptr;
	char ** p_endptr;

	if (fp_endptr == NULL)
		p_endptr = &endptr;
	else
		p_endptr = fp_endptr;

	ret = strtoul(nptr, p_endptr, 10);

	if (result) *result = 0;
	if (
		(endptr == nptr && ret == 0) ||
		((ret == ULONG_MAX) && errno == ERANGE)
	) {
		if (result) {
			*result = 1;
		}
		fprintf(stderr, "%s: strtoul error when converting [%s]\n", swlib_utilname_get(), nptr);
	}
	return ret;
}

uintmax_t **
swlib_pump_get_ppstatbytes(void)
{
	return &g_pstatbytes;
}

intmax_t
swlib_pump_amount8(int ofd, int ifd, intmax_t amount, int adjunct_ofd, FILE_DIGS * digs)
{
	intmax_t n;
	n = swlib_i_digs_copy(ofd, ifd, amount, digs, adjunct_ofd, taru_tape_buffered_read);
	return n;
}

struct timespec * swlib_get_io_req(void)
{
	return io_req;
}

int * swlib_burst_adjust_p(void)
{
	return &g_burst_adjust;
}

struct timespec ** swlib_get_io_req_p(void)
{
	return &io_req;
}

int
swlib_test_verbose(struct swEvents * ev, int verbose_level, int swbis_event,
		int is_swi_event, int event_status, int is_posix_event)
{
	if (
		(ev && (verbose_level >= ev->verbose_threshholdM)) ||
		(verbose_level >= SWC_VERBOSE_1 && event_status != 0) ||
		(verbose_level >= SWC_VERBOSE_3 && is_posix_event) ||
		(verbose_level >= verbose_swbis_event_thresholdG && swbis_event) ||
		(verbose_level >= SWC_VERBOSE_6 && is_swi_event) ||
		(verbose_level >= verbose_swbis_event_thresholdG && !is_swi_event)
	) {
		return 1;
	} else {
		return 0;
	}
}

int
swlib_get_verbose_level(void)
{
	return verbose_levelG;
}

void
swlib_set_verbose_level(int n)
{
	verbose_levelG = n;
}

intmax_t
swlib_i_pipe_pump(int suction_fd, int discharge_fd, 
		intmax_t *amount, int adjunct_ofd, STROB * obuf,
		ssize_t (*thisfpread)(int, void*, size_t))
{
	int commandFailed = 0;
	int pumpDead = 0;
	int bytes;
	int ibytes;
	intmax_t byteswritten = 0;
	intmax_t remains;
	intmax_t c_amount;
	char buf[SWLIB_PIPE_BUF];
	int sleepbytes = 0;

	if (obuf) {
		strob_strcpy(obuf, "");
	}
	E_DEBUG("");
	c_amount = *amount;
	if (c_amount < 0) {
		E_DEBUG("");
		do {
			E_DEBUG("");
			bytes = (*thisfpread)(suction_fd, buf, sizeof(buf));
			if (bytes < 0) {
				E_DEBUG("");
				SWBIS_ERROR_IMPL();
				commandFailed = 1;	
				pumpDead = 1;
			} else if (bytes == 0) {
				/* if it's not dead yet, it will be when we 
				 * close the pipe 
				*/
				E_DEBUG("");
				pumpDead = 1;
			} else {
				E_DEBUG("");
				if (discharge_fd >= 0) {
					E_DEBUG("");
					if (uxfio_write(discharge_fd, 
						buf, 
						bytes) != bytes) {
						E_DEBUG("");
						SWBIS_ERROR_IMPL();
						commandFailed = 2;
						/* discharge failure */
						pumpDead = 1;
					}
				}

				if (adjunct_ofd >= 0) {
					if (uxfio_write(adjunct_ofd, 
						buf, bytes) != bytes) {
						SWBIS_ERROR_IMPL();
						commandFailed = 3;
						/* discharge failure */
						pumpDead = 1;
					}
				}

				if (obuf) {
					strob_memcpy_at(obuf, (size_t)byteswritten, buf, (size_t)(bytes));
				}
			}
			E_DEBUG("");
			byteswritten += bytes;
			if (io_req && io_req->tv_nsec) {
				sleepbytes += bytes;
				delay_sleep_pattern2(io_req, &sleepbytes, c_amount, byteswritten);
			}
		} while (!pumpDead);
		if (obuf) {
			strob_append_hidden_null(obuf);
		}
		E_DEBUG("");
		*amount = byteswritten;
		return commandFailed;
	} else {
		E_DEBUG("");
		remains = sizeof(buf);
		if ((c_amount - byteswritten) < remains) {
			remains = c_amount - byteswritten;
		}
		E_DEBUG("");
		do {
			E_DEBUG("");
			bytes = (*thisfpread)(suction_fd, buf, remains);
			if (bytes < 0) {
				E_DEBUG("");
				SWBIS_ERROR_IMPL();
				commandFailed = 1;
				pumpDead = 1;
			} else if (bytes == 0) {
				/* if it's not dead yet, it will 
				* be when we close the pipe 
				*/
				E_DEBUG("");
				pumpDead = 1;
			} else if (bytes) {
				E_DEBUG("");

				if (discharge_fd >= 0) {
					E_DEBUG("");
					ibytes = uxfio_write(discharge_fd, 
							buf, bytes);
					if (adjunct_ofd >= 0) {
						ibytes = uxfio_write(
							adjunct_ofd, 
							buf, bytes);
					}	
				} else {
					ibytes = bytes;
				}

				if (obuf) {
					strob_memcpy_at(obuf, (size_t)byteswritten, buf, (size_t)(bytes));
				}

				if (ibytes != bytes) {
					SWBIS_ERROR_IMPL();
					E_DEBUG("");
					commandFailed = 2;
					/* discharge failure */
					pumpDead = 1;
				} else {
					E_DEBUG("");
					byteswritten += bytes;
					if ((c_amount - byteswritten) < 
								remains) {
						if (
					(remains = c_amount - byteswritten) > 
						(int)(sizeof(buf))
						) {
							remains = sizeof(buf);
						}
					}
				}
			}
			if (io_req && io_req->tv_nsec) {
				sleepbytes += bytes;
				delay_sleep_pattern2(io_req, &sleepbytes, c_amount, byteswritten);
			}
			E_DEBUG("");
		} while (!pumpDead && remains);
		*amount = byteswritten;
		if (obuf) {
			strob_append_hidden_null(obuf);
		}
		return commandFailed;
	}
	return -1;
}

char *
swlib_printable_safe_ascii(char * sf) {
	char * s;
	/*
	 * Now check if all char are ascii
	 */
	s = sf;

	while (s && *s) {
		if (*s > 126 || *s < 33) {
			/*
			 * For now, just set the byte
			 * to a single '.'
			 */
			*s = '.';
		}
		s++;
	}
	return sf;
}

int
swlib_is_ascii_noaccept(char * str, char * acc, int minlen) {
	unsigned char *s;
	int ret;
	if (!str || ((int)strlen(str) < minlen)) return 1;
	ret = strpbrk(str, acc) ? 1 : 0;
	if (ret) return ret;
	s = (unsigned char*)str;
	while (s && *s) {
		if (*s > 126 || *s < 33) {
			return 1;
		}
		s++;
	}
	return 0;
}

int
swlib_tr(char * src, int to, int from)
{
	int ret = 0;
	char * p1;

	p1 = src;
	while (*(p1++)) {
		if (*(p1 - 1) == from) {
			*(p1 - 1) = to;
			ret ++;
		}
	}
	return ret;
}

int
swlib_check_safe_path(char * s)
{
	if (!s) return 1;
	if (!strlen(s)) return 5;
	if (strstr(s, "..")) return 6;
	if (strchr(s, ' ')) return 7;
	return swlib_is_sh_tainted_string(s);
}

int
swlib_check_clean_path(char * s)
{
	if (!s) return 1;
	if (!strlen(s)) return 2;
	if (strstr(s, "//")) return 3;
	if (strstr(s, "..")) return 4;
	if (strchr(s, ' ')) return 5;
	if (swlib_is_sh_tainted_string(s)) return 6;
	return strpbrk(s, SWBIS_WS_TAINTED_CHARS) ? 7 : 0;
}

int
swlib_check_clean_absolute_path(char * s)
{
	if (!s) return 1;
	if (!strlen(s)) return 2;
	if (*s != '/') return 3;
	return swlib_check_clean_path(s);
}

int
swlib_check_clean_relative_path(char * s)
{
	if (!s) return 1;
	if (*s == '/') return 2;
	return swlib_check_clean_path(s);
}

int
swlib_check_legal_tag_value(char * s) {
	if (!s) return 0; /* Ok */
	return strpbrk(s, ":.,") ? 1 : 0;
}

int
swlib_is_sh_tainted_string(char * s) {
	if (!s) return 0;
	return strpbrk(s, SWBIS_TAINTED_CHARS) ? 1 : 0;
}

void
swlib_is_sh_tainted_string_fatal(char * s) {
	if (!s) return;
	if (strpbrk(s, SWBIS_TAINTED_CHARS) ? 1 : 0) {
		SWLIB_FATAL("tainted string");
	}
	return;
}

ssize_t
swlib_safe_read(int fd, void * buf, size_t nbyte)
{
	int n=0, nret=1;
	char *p = (char*)(buf);
	
	while(n < (int)nbyte && nret){
		nret = uxfio_read(fd, p+n, nbyte-n); 
		if (nret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			return nret;
		}
		n+=nret;	
	}
	return n;
}

void
swlib_swprog_assert(int error_code, int status, char * reason,
		char * version, char * file, int line, char * function)
{
	if (error_code != 0 && 
		error_code < SWBIS_PROGS_USER_ERROR && 
			error_code >= SWBIS_PROGS_IMPLEMENTATION_ERROR) {
		fprintf(stderr, 
	"%s: Error: code=[%d] : %s: version=[%s] file=[%s] line=[%d]\n",
			swlib_utilname_get(), error_code, reason, version, file, line);
		exit(status);
	} else if (error_code != 0 && 
			error_code >= SWBIS_PROGS_USER_ERROR) {
		exit(status);
	} else if (error_code != 0 && 
			error_code < SWBIS_PROGS_IMPLEMENTATION_ERROR ) {
		fprintf(stderr, 
		"Internal Informative Warning: code=%d : %s version=[%s]"
		", file=[%s], line=[%d]\n", 
			error_code, reason, version, file, line);
	}
}

void
swlib_exception(char * reason, char * file,
			int line, char * function)
{
	e_msg("program exception", reason, file, line, function);
}

void
swlib_internal_error(char * reason, char * file,
			int line, char * function)
{
	e_msg("internal implementation error", reason, file, line, function);
}

void
swlib_resource(char * reason, char * file,
			int line, char * function)
{
	e_msg("resource exception", reason, file, line, function);
}

void
swlib_fatal(char * reason, char * file,
			int line, char * function)
{
	e_msg("fatal error", reason,file, line, function);
	exit(252);
}

void
swlib_assertion_fatal(int assertion_result, char * reason, char * file,
			int line, char * function)
{
	if (assertion_result == 0)
		swlib_fatal(reason, file, line, function);
}

int
swlib_squash_trailing_char(char *path, char ch)
{
	return squash_trailing_char(path, ch, 0);
}

void
swlib_squash_all_trailing_vnewline(char *path)
{
	while (1) 
		if (squash_trailing_char(path, '\n', 0) == 0) {
			squash_trailing_char(path, '\r', 0);
		} else {
			break;
		}
}

void
swlib_squash_trailing_vnewline(char *path)
{
	squash_trailing_char(path, '\r', 0);
	squash_trailing_char(path, '\n', 0);
}

void
swlib_squash_trailing_slash(char *path)
{
	squash_trailing_char(path, '/', 1);
	return;
}

void
swlib_squash_embedded_dot_slash(char *path)
{
	char *p1;
	p1 = strstr(path, "/./");
	if (p1 && p1 != path) {
		memmove(p1+1, p1 + 3, strlen(p1 + 2));
	} else if (p1 && p1 == path) {
		memmove(p1+1, p1 + 3, strlen(p1 + 2));
	}
}

void
swlib_squash_double_slash(char *path)
{
	char *p1;
	while ((p1 = strstr(path, "//")) != (char *) (NULL))
		memmove(p1, p1 + 1, strlen(p1));
}

char *
swlib_return_no_leading(char *path)
{
	char * s;
	/* This function may truncate the path to zero length */
	if (((s = strstr(path, "/")) == path) && 1 /*strlen(s) > 1*/) return ++path;
	if (((s = strstr(path, "./")) == path) && 1 /*strlen(s) > 2*/) return path+=2;
	return path;
}

void
swlib_squash_all_dot_slash(char *path)
{
	char * s;

	swlib_squash_leading_dot_slash(path);
	s = strstr(path, "/./");
	while(s) {
		swlib_squash_embedded_dot_slash(path);
		s = strstr(path, "/./");
	}
}

void
swlib_squash_leading_dot_slash(char *path)
{
	char *p1;
	/* 
	* squash leading "./" 
	*/
	if (strlen(path) >= 3) {
		if (!strncmp(path, "./", 2)) {
			p1 = path;
			memmove(p1, p1 + 2, strlen(p1 + 1));
		}
	}
}

void
swlib_toggle_trailing_slashdot(char * mode, char * name, int *pflag)
{
	if (strcmp(mode, "drop") == 0) {
		if (strlen(name) < 2) {
			*pflag = 0;
			return;
		}
		if (strcmp(name + strlen(name) - 2,  "/.") == 0) {
			*(name + strlen(name) - 2) = '\0';
			*pflag = 1;
		}
	} else {
		if (*pflag) {
			strcat(name, "/.");
			*pflag = 0;
		}
	}
}

void
swlib_toggle_leading_dotslash(char * mode, char * name, int *pflag)
{
	if (strcmp(mode, "drop") == 0) {
		if (strlen(name) < 2) {
			*pflag = 0;
			return;
		} else if (strncmp(name,  "./", 2) == 0 && strlen(name) > 2) {
			memmove(name, name+2, strlen(name)-1);
			*pflag = 1;
		} else {
			*pflag = 0;
			return;
		}
	} else {
		if (*pflag) {
			if (strcmp(name, ".") == 0) {
				strcat(name, "/");
			} else {
				memmove(name+2, name, strlen(name)+1);
				*name = '.';
				*(name +1) = '/';
				*pflag = 0;
			}
		}
	}
}

void
swlib_toggle_trailing_slash(char * mode, char * name, int *pflag)
{
	if (strcmp(mode, "drop") == 0) {
		if (strlen(name) > 1 && name[strlen(name) - 1] == '/') {
			name[strlen(name) - 1] = '\0';
			*pflag = 1;
		} else {
			*pflag = 0;
		}
	} else {
		if (*pflag) {
			name[strlen(name)] = '/';
		}
	}
}

void
swlib_toggle_all_trailing_slash(char * mode, char * name, int *pflag)
{
	int pf = 0;
	*pflag = 0;
	swlib_toggle_trailing_slash(mode, name, &pf);
	while (pf) {
		(*pflag) ++;
		pf = 0;
		swlib_toggle_trailing_slash(mode, name, &pf);
	}
}

void
swlib_squash_leading_slash(char * name)
{
	if (*name == '/') {
		memmove(name, name+1, strlen(name));
	}
}

char *
swlib_return_relative_path(char * path) {
	char * s = path;
	while(*s == '/') s++;
	return s;
}

void
swlib_squash_all_leading_slash(char * name)
{
	int s;
	int i;
	if (!name) return;

	s = strlen(name);
	i = (*name == '/');

	while(*name == '/') swlib_squash_leading_slash(name);

	if (i && s && strlen(name) == 0) {
		/*
		* squashed '/' down to nothing.
		*/
		strcpy(name, ".");
	}
}

void
swlib_toggle_leading_slash(char * mode, char * name, int *pflag)
{

	if (strcmp(mode, "drop") == 0) {
		if (*name == '/' && strlen(name) > 1) {
			memmove(name, name+1, strlen(name));
			*pflag = 1;
		} else {
			*pflag = 0;
		}
	} else if (strcmp(mode, "restore") == 0) {
		if (*pflag) {
			memmove(name+1, name, strlen(name)+1);
			*name = '/';
			*pflag = 0;
		}
	}
}

void
swlib_slashclean(char *path)
{
	/* 
	* squash double slashes in path 
	*/
	swlib_squash_double_slash(path);

	swlib_squash_leading_dot_slash(path);

	/* squash leading slash
	* if (*path == '/' && strlen(path) > 1)
	* 	memmove(path, path + 1, strlen(path));
	*/

	swlib_squash_trailing_slash(path);
	return;
}

int
swlib_process_hex_escapes(char * s1)
{
	process_all_hex_escapes(s1);
	return 0;	
}

int
swlib_compare_8859(char * s1, char * s2)
{
	int ret;
	STROB * so1;
	STROB * so2;

	so1 = NULL;	
	so2 = NULL;	
	if (strchr(s1, '\\') != NULL) {
		E_DEBUG2("unexpanding S1 [%s]", s2);
		so1 = strob_open(strlen(s1));
		swlib_unexpand_escapes(so1, s1);
		s1 = strob_str(so1);
		process_all_hex_escapes(s1);
	}

	if (strchr(s2, '\\') != NULL) {
		E_DEBUG2("unexpanding S2 [%s]", s2);
		so2 = strob_open(strlen(s2));
		swlib_unexpand_escapes(so2, s2);
		s2 = strob_str(so2);
		process_all_hex_escapes(s2);
	}

	if (strchr(s1, '#') || strchr(s2, '#')) {
		E_DEBUG3("Comparing [%s] [%s]", s1, s2);
	}

	ret = swlib_dir_compare(s1, s2, SWC_FC_NOAB /*ignore absolute path*/);
	if (so1) strob_close(so1);
	if (so2) strob_close(so2);
	return ret;
}

int
swlib_vrelpath_compare(char * s1, char * s2, char * cwd)
{
	if (
		(
		(*s1 != '/')  &&
		(*s2 != '/')
		) ||
		(
		(*s1 == '/')  &&
		(*s2 == '/')
		) 
	) {
		return swlib_dir_compare(s1, s2, SWC_FC_NOOP);
	} else {
		STROB * tmp = strob_open(100);
		if (form_abspath(tmp, s1, cwd) == 0) {
			/*
			* s1 was the relative path, it is now 
			* in (STROB*)(tmp) as an absolute path.
			*
			* s2 is already absolute.
			*/
			return swlib_dir_compare(strob_str(tmp), s2, SWC_FC_NOOP);
		} else if (form_abspath(tmp, s2, cwd) == 0) {
			return swlib_dir_compare(s1, strob_str(tmp), SWC_FC_NOOP);
		} else {
			/* Never gets here.
			*/
			SWLIB_ALLOC_ASSERT(0);	
			;
		}
	}
	/* Never gets here.
	*/
	SWLIB_ALLOC_ASSERT(0);	
	return -1;
}

int
swlib_basename_compare(char * s1, char * s2)
{
	char *s1b;
	char *s2b;
	int ret;

	s1b = strrchr(s1, '/');
	s2b = strrchr(s2, '/');
	if (s1b && strlen(s1) && *(s1b+1)) {
		s1 = s1b + 1;
	}
	if (s2b && strlen(s2) && *(s2b+1)) {
		s2 = s2b + 1;
	}
	ret = swlib_dir_compare(s1, s2, SWC_FC_NOOP);
	return ret;
}

int
swlib_dir_compare(char * s1, char * s2, int FC_compare_flag)
{
	int ret;
	int leading_p1 = 0;
	int leading_p2 = 0;
	int leading_d1 = 0;
	int leading_d2 = 0;
	int trailing_p1 = 0;
	int trailing_d1 = 0;
	int trailing_p2 = 0;
	int trailing_d2 = 0;


	if (FC_compare_flag == SWC_FC_NOAB) {
		swlib_toggle_leading_slash("drop", s1, &leading_p1);
		swlib_toggle_leading_slash("drop", s2, &leading_p2);
	} else if (FC_compare_flag == SWC_FC_NORE) {
		/*
		* Not used, dead code.
		*/
		;
	} else {
		;
	}

	swlib_toggle_trailing_slash("drop", s1, &trailing_p1);
	swlib_toggle_trailing_slash("drop", s2, &trailing_p2);

	swlib_toggle_trailing_slashdot("drop", s1, &trailing_d1);
	swlib_toggle_trailing_slashdot("drop", s2, &trailing_d2);
	
	swlib_toggle_leading_dotslash("drop", s1, &leading_d1);
	swlib_toggle_leading_dotslash("drop", s2, &leading_d2);

	ret = strcmp(s1, s2);	
	
	swlib_toggle_leading_dotslash("restore", s1, &leading_d1);
	swlib_toggle_leading_dotslash("restore", s2, &leading_d2);
	
	swlib_toggle_trailing_slashdot("restore", s1, &trailing_d1);
	swlib_toggle_trailing_slashdot("restore", s2, &trailing_d2);
	
	swlib_toggle_trailing_slash("restore", s1, &trailing_p1);
	swlib_toggle_trailing_slash("restore", s2, &trailing_p2);


	if (FC_compare_flag == SWC_FC_NOAB) {
		swlib_toggle_leading_slash("restore", s1, &leading_p1);
		swlib_toggle_leading_slash("restore", s2, &leading_p2);
	} else if (FC_compare_flag == SWC_FC_NORE) {
		/*
		* never gets here
		*/
		;
	} else {
		;
	}

	return ret;
}

char *
swlib_dirname(STROB * dest, char * source)
{
	char * s;
	strob_strcpy(dest, source);
	s = strrchr(strob_str(dest), '/');
	if (!s) {
		strob_strcpy(dest, ".");
	} else {
		if (s != strob_str(dest))
			*s = '\0';
		else
			*(s+1) = '\0';
	}
	return strob_str(dest);
}

char *
swlib_basename(STROB * dest, char * source)
{
	char * s;

	if (dest == NULL) {
		s = strrchr(source, '/');
		if (!s) return source;
		s++;	
		while (*s == '/') s++;
		if (*s == '\0') {
			if (s > source) return s-1;
			return source;
		}
		return s;
	} else {
		strob_strcpy(dest, source);
		s = swlib_basename(NULL, strob_str(dest));
		memmove(strob_str(dest), s, strlen(s)+1);
		return strob_str(dest);
	}
}

int
swlib_unix_dirtrunc(STROB * buf)
{
	char * s;

	swlib_squash_trailing_slash(strob_str(buf));
	swlib_squash_all_dot_slash(strob_str(buf));

	s = strrchr(strob_str(buf), '/');
	if (
		s == NULL ||
		s == strob_str(buf) ||
		(s == strob_str(buf)+1 && (int)(*strob_str(buf)) == '.') ||
		0
	) {
		return 0;
	}
	*s = '\0';
	return 1;
}

int
swlib_unix_dirtrunc_n(STROB * buf, int n)
{
	int i;
	int ret;
	i = n;
	while(i > 0) {
		ret = swlib_unix_dirtrunc(buf);
		if (ret == 0) break;
		i--;
	}
	return n - i;
}

int
swlib_unix_dircat(STROB * dest, char * dirname)
{
	char * newp;
	char * s = strob_str(dest);
	
	if (!dirname || strlen(dirname) == 0) return 0;
	if (strlen(s)) {
		if (s[strlen(s) - 1] != '/')
			strob_strcat(dest, "/");
	}
	newp = swlib_strdup(dirname);
	SWLIB_ALLOC_ASSERT(newp != NULL);	
	
	if (strob_strlen(dest))
		swlib_squash_leading_dot_slash(newp);
	strob_strcat(dest, newp);
	swlib_squash_double_slash(strob_str(dest));
	swbis_free(newp);
	return 0;
}

int
swlib_resolve_path(char * ppath, int * depth, STROB * resolved_path)
{
	STROB * tmp;
	int startnames = 0;
	int count = 0;
	int numcomponents = 0;
	char * path = swlib_strdup(ppath);	
	char * s;

	swlib_slashclean(path);
	tmp = strob_open(10);

	if (resolved_path) strob_strcpy(resolved_path, "");

	s = strob_strtok(tmp, path, "/");
	while (s) {
		if (strcmp(s, "..") == 0 && !startnames) {
			count++;
		} else if (strcmp(s, ".") == 0) {
			/* do nothing */
		} else if (strcmp(s, "..") == 0 && startnames) {
			count--;
		} else {
			count++;
			startnames = 1;
		}
		
		if (resolved_path) {
			strob_strcat(resolved_path, s);
			strob_strcat(resolved_path, "/");
		}

		numcomponents ++;
		s = strob_strtok(tmp, NULL, "/");
	}

	if (depth) *depth = count;
	strob_close(tmp);
	swbis_free(path);
	return numcomponents;
}

int
swlib_exec_filter(SHCMD ** cmd, int src_fd, STROB * output)
{
	int input_pipe[2];
	int ifd, ofd, i=0;
	int childi;
	int retval;
	int status;
	int cret;
	int ret;

	retval = 0;
	ifd = src_fd;
	while (cmd[i]) i++;
	ofd=shcmd_get_dstfd(cmd[--i]);
	input_pipe[0] = -1;
	input_pipe[1] = -1;

	if (ifd >= 0) {
		E_DEBUG("");
		pipe(input_pipe);
		childi = swfork((sigset_t*)(NULL));
		if (childi == 0) {
			int ret;
			swgp_signal(SIGPIPE, SIG_DFL);
			swgp_signal(SIGINT, SIG_DFL);
			swgp_signal(SIGTERM, SIG_DFL);
			swgp_signal(SIGUSR1, SIG_DFL);
			swgp_signal(SIGUSR2, SIG_DFL);
			if (ifd != 0) close(0);
			if (ifd != 1) close(1);
			close(input_pipe[0]);
			close(ofd);
			retval = swlib_pump_amount(input_pipe[1], ifd, -1);
			if (retval < 0)
				ret = 1;
			else
				ret = 0;
			_exit(ret);
		}
		E_DEBUG("");
		if (childi < 0) {
			SWLIB_INTERNAL("swlib_exec_filter: 0001.");
			return -(INT_MAX);
		}
		close(input_pipe[1]);
		shcmd_set_srcfd(cmd[0], input_pipe[0]);
		if (output) {
			E_DEBUG("");
			swlib_shcmd_output_strob(output, cmd);
		} else {
			E_DEBUG("");
			shcmd_command(cmd);
		}
		close(input_pipe[0]);
		E_DEBUG("");

		cret = 126;
		E_DEBUG("");
		ret = waitpid(childi, &status, 0);
		E_DEBUG("");
		if (ret == childi) {
			E_DEBUG("");
			if (WIFEXITED(status)) {
				cret = WEXITSTATUS(status);
				if (cret != 0) {
					SWLIB_ERROR("");
				} else {
					E_DEBUG("child OK");
				}
			} else {
				SWLIB_ERROR("");
				cret = 127;
			}
		} else {
			E_DEBUG("");
			if (ret < 0) {
				SWLIB_ERROR2("%s", strerror(errno));
			}
		}
		E_DEBUG("");
	} else {
		E_DEBUG("");
		cret = 0;
		if (output)
			swlib_shcmd_output_strob(output, cmd);
		else
			shcmd_command(cmd);
	}
	E_DEBUG("");
	if (cret == 127) {
		/* FIXME */
		/* this happens when, for example, the signature in a package
		   has a CRC error, shcmd_wait() below will hang, so we avoid
		   doing it */
		E_DEBUG("");
		;
	} else {
		E_DEBUG("");
		retval = shcmd_wait(cmd);
	}
	E_DEBUG2("retval=%d", retval);
	E_DEBUG2("cret=%d", cret);
	return (retval || cret);
}

SYNCT *
swlib_synct_create(void)
{
	SYNCT * synct = (SYNCT *)malloc(sizeof(SYNCT));
	int len;
	len = (3 * SWLIB_SYNCT_BLOCKSIZE) + 1;
	synct->bufM = (unsigned char *)malloc((size_t)len);
	if (!synct->bufM) _exit(44);
	memset(synct->bufM, (int)('\0'), len);
	synct->tailM =  synct->bufM + SWLIB_SYNCT_BLOCKSIZE;
	synct->countM = 0;
        synct->mtM = strdup(SWBIS_SYNCT_EOF);
	synct->do_debugM = 0;
	synct->debugfdM = -1; /*open("/tmp/swbis_synct.dump", O_RDWR|O_CREAT|O_APPEND|O_TRUNC, 0755);  */
	return synct;
}

void
swlib_synct_delete(SYNCT * synct)
{
	if (synct->countM % 2) {
		/* if block count is odd give a warning */
		fprintf(stderr, "%s: Warning: swlib_synct_delete() block count error: %d\n",
			swlib_utilname_get(), synct->countM);
	}
	if (synct->debugfdM > 0) close(synct->debugfdM);
	free(synct->mtM);
	free(synct->bufM);
	free(synct);
}

/**
 * swlib_synct_read - read an ascii stream with an in-band EOF string
 * @fd: file descriptor 
 * @buf: file descriptor containing the archive
 *
 * Returns -1 on error, or number of bytes read which always is
 * SWLIB_SYNCT_BLOCKSIZE.  The writer of the stream must always write
 * in 2*SWLIB_SYNCT_BLOCKSIZE blocks therefore the total number of
 * SWLIB_SYNCT_BLOCKSIZE size blocks is always even.  The stream may
 * contain interspersed NULs which are assumed to be not part of the
 * file.
 * The EOF string (TRAILER!!!\r\n\r\n) must begin a 2*SWLIB_SYNCT_BLOCKSIZE
 * size block and be entirely NUL filled after.
 * 
 * The size of void * userbuf must be 2*SWLIB_SYNCT_BLOCKSIZE or more.
 * 
 * The routine will return SWLIB_SYNCT_BLOCKSIZE, or 2*SWLIB_SYNCT_BLOCKSIZE
 * or 0, or -1
 */

int
swlib_synct_read(SYNCT * synct, int fd, void * userbuf)
{
	unsigned char * ibuf;
	unsigned char * tbuf;
	char * z;
	int ret;
	int tret;
	int check_ret;

	ibuf = synct->bufM;
	tbuf = synct->tailM;	

	/* The output end (sender) must be sync'ed to a block size of
	   two times SWLIB_SYNCT_BLOCKSIZE */
	
	E_DEBUG2("fd = %d", fd);
	ret = synct_i_read_block(synct, uxfio_sfa_read, fd, (char*)ibuf);
	E_DEBUG2("synct_i_read_block() returned %d", ret);
	if (ret < 0) return -1;
	if (ret == 0) return 0;
	check_ret = synct_check_for_possible_eof(synct);
	E_DEBUG("");
	if (check_ret == SYNCT_EOF_CONDITION_0) {
		/* EOF not found */
		memcpy(userbuf, ibuf, ret);
		return (int)SWLIB_SYNCT_BLOCKSIZE;
	} else {
		/* EOF found */
		/* read 1 more block which should be NULs.
		   If so, its the trailer block therefore
		   return 0 bytes to the user */
		E_DEBUG2("fd = %d", fd);
		ret = synct_i_read_block(synct, uxfio_sfa_read,  fd, (char*)tbuf);
		if (ret < 0) return -2;
		z = malloc((size_t)SWLIB_SYNCT_BLOCKSIZE);
		memset(z, '\0', (size_t)SWLIB_SYNCT_BLOCKSIZE);
		ret = memcmp(z, tbuf, SWLIB_SYNCT_BLOCKSIZE);
		free(z);
		if (ret == 0) {
			/* really EOF */
			if (check_ret == SYNCT_EOF_CONDITION_2) {
				/* Bug workaround:
				 * This condition occurs due to a mangled TRAILER!!!\r\n string and
				 * seems to include two (or more??) extra blocks at the end of the pipe.
				 * These need to be read and discarded to prevent
				 * a block'ing read() and an apparent program hang.
				 */
				tret = 1;
				while(tret > 0)
					tret = synct_i_read_block(synct, timed_atomic_read5, fd, (char*)tbuf);
			}
			return 0;
		} else {
			/* we've been fooled !!, Note: this could be normal */
			/* we now have read two (2) blocks which must be 
			   returned to the user */
			memcpy(userbuf, ibuf, SWLIB_SYNCT_BLOCKSIZE);
			memcpy(((char*)userbuf)+SWLIB_SYNCT_BLOCKSIZE, tbuf, SWLIB_SYNCT_BLOCKSIZE);
			return (int)(2*SWLIB_SYNCT_BLOCKSIZE);	
		}
	}
	/* never gets here */
	return -2;
}

/**
 * swlib_synct_suck - rewrites a stream containing an in-band EOF string
 * The stream is written by dd sync=conv to 2*SWLIB_SYNCT_BLOCKSIZE size blocks
 * NULs '\0' are assumed to be not part of the data.  The trailer string
 * must begin a 2*SWLIB_SYNCT_BLOCKSIZE size block and be filled with NULs
 */

int
swlib_synct_suck(int ofd, int ifd)
{
	int ret;
	int wret;
	int wtotal = 0;
	char buf[SWLIB_SYNCT_BLOCKSIZE+SWLIB_SYNCT_BLOCKSIZE + 1];
	SYNCT * synct;
	synct = swlib_synct_create();
	do {
		ret = swlib_synct_read(synct, ifd, buf);
		E_DEBUG2("swlib_synct_read returned %d", ret);
		if (ret > 0) {
			wret = synct_i_no_null_write(ofd, buf, ret);
			if (wret < 0) {
				E_DEBUG("error");
				return -2;
			}	
			wtotal += wret;
		}
	} while (ret > 0);
	swlib_synct_delete(synct);
	if (ret < 0) return ret;
	return wtotal;
}

int
swlib_read_amount(int suction_fd, intmax_t amount)
{
	return swlib_pump_amount(-1, suction_fd, amount);
}

int
swlib_pipe_pump(int ofd, int ifd)
{
	E_DEBUG("");
	return 
	swlib_pump_amount(ofd, ifd, -1);
}

intmax_t
swlib_pump_amount(int discharge_fd, int suction_fd, intmax_t amount)
{
	intmax_t i = amount;
	if (swlib_i_pipe_pump(suction_fd, discharge_fd,  &i, 
			-1, NULL, uxfio_read)) {
		return -1;
	}
	return i;
}

int
swlib_fork_to_make_unixfd(int uxfio_fd, sigset_t * blockmask, 
		sigset_t * defaultmask, int * ppid)
{
	int upipe[2];
	pid_t upid;

#ifndef OPEN_MAX
#define OPEN_MAX 256
#endif
	if (ppid) *ppid = (int)0;
	if (uxfio_fd <= OPEN_MAX) 
		return uxfio_fd;

	if (pipe(upipe) < 0) {
		return -1;
	}
	if ((upid = swndfork(blockmask, defaultmask)) > 0) {	/* parent */
		uxfio_close(uxfio_fd);
		close(upipe[1]);
		if (ppid) *ppid = (int)upid;
		return upipe[0];
	} else if (upid == 0) {	/* child */
		int ret = 0;
		close(upipe[0]);
		ret = swlib_pipe_pump(upipe[1], uxfio_fd);
		if (ret >= 0) ret = 0;
		else ret = 255;
		_exit(ret);
	} else {
		SWLIB_RESOURCE("fork failed");
		return -1;
	}
}

int
swlib_is_c701_escape(int c) {
	/* 
	* Is it a c701 escapeable character 
	*/
	if (
		(c == '\"') ||
		(c == '#') ||
		(c == '\\') ||
		0
	) {
		return 1;
	} else {
		return 0;
	}
}

int 
swlib_is_ansi_escape(int c) {
    /* 
    * Is it a ANSI C escapeable character 
    */
     if ( (c == 'n') ||
          (c == 't') ||
          (c == 'v') ||
          (c == 'b') ||
          (c == 'r') ||
          (c == 'f') ||
          (c == 'x') ||
          (c == 'a') ||
          (c == '\\') ||
          (c == '?') ||
          (c == '\'') ||
          (c == '\"')
        ) return 1;
     else
          return 0;
}

int 
swlib_c701_escaped_value(int src, int * is_escape) {
	*is_escape = 1;
	switch (src) {
		case '\\': return (int)'\\'; break;
		case '#': return (int)'#'; break;
		case '\"': return (int)'\"'; break;
	}
	*is_escape = 0;
	return (int)(src);
}


int 
swlib_ansi_escaped_value(int src, int * is_escape) {
	*is_escape = 1;
	switch (src) {
		case '\\': return (int)'\\'; break;
		case 'n': return (int)'\n'; break;
		case 'r': return (int)'\r'; break;
		case 'v': return (int)'\v'; break;
		case 'b': return (int)'\b'; break;
		case 'f': return (int)'\f'; break;
	}
	*is_escape = 0;
	return (int)(src);
}

o__inline__
char *
swlib_strdup(char *s)
{
	return strdup(s);
}

char *
swlib_strncpy(char * dst, const char * src, size_t n)
{
	char *p = strncpy(dst, src, n-1);
	dst[n-1] = '\0';
	return p;
}

int
swlib_writef(int fd, STROB * buffer, char * format, ...)
{
	int ret;
	int newret;
	va_list ap;
	va_start(ap, format);
	ret = strob_vsprintf(buffer, 0, format, ap);
	va_end(ap);
	if (ret < 0) return -1;
	newret = atomicio(uxfio_write, fd, 
			(void*)strob_str(buffer), (size_t)(ret));
	if (newret != ret) return -1;
	return newret;
}

/*
*     swlib_write_catalog_stream
*  write out the catalog half of the package.
*  This does not decode/re-encode the tar header, it
*  passes thru the original bits.
*   * version 0 OBSOLETE *
*/
int
swlib_write_OLDcatalog_stream(XFORMAT * package, int ofd)
{
	int ret;
	ret = taruib_write_catalog_stream((void*)package, 
				ofd, /* version */ 0, /*verbose*/ 0);
	return  ret;
}

int
swlib_write_catalog_stream(XFORMAT * package, int ofd)
{
	int ret;
	ret = taruib_write_catalog_stream((void*)package, 
				ofd, /* version */ 1, /*verbose*/ 0);
	return  ret;
}

/*
*     swlib_write_storage_stream
*  write out the storage half of the package.
*  This does not decode/re-encode the tar header, it
*  passes thru the original bits.
*   * version 0 OBSOLETE *
*
*/
int
swlib_write_OLDstorage_stream(XFORMAT * package, int ofd)
{
	int ret;
	ret = taruib_write_storage_stream((void *)package, ofd, 
			 /*version*/ 0, -1, /*verbose*/ 0, 0 /* md5sum */);
	return ret;
}

int
swlib_write_storage_stream(XFORMAT * package, int ofd)
{
	int ret;
	ret = taruib_write_storage_stream((void *)package, ofd,
			/*version*/ 1, -1, /*verbose*/ 0, 0 /* md5sum */);
	return ret;
}

/*
*     swlib_write_signing_files
*   Does decode and reencode the headers, if the input format is identical
*   to the swbis writing implementation then the output will be the same
*   format as the input.
*/
int
swlib_write_signing_files(XFORMAT * package, int ofd, 
		int which_file /* 0=catalog  1=storage */,
		int do_adjunct_md5)
{
	int nullfd;
	int ifd = xformat_get_ifd(package);
	int ret;
	int bytesret;
	int retval = 0;
	STROB * namebuf = strob_open(100);
	SWPATH * swpath = swpath_open("");
	char * name;
	char nullblock[512];
	int writeit;
	int do_trailer = 0;
	int format = xformat_get_format(package);
	long int bytes = 0;

	memset(nullblock, '\0', sizeof(nullblock));

	if (!swpath) return -21;
	if (ifd < 0) return -32;

	nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	if (nullfd < 0) return -2;

	while ((ret = xformat_read_header(package)) > 0) {
		if (xformat_is_end_of_archive(package)){
			break;
		}
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		swpath_parse_path(swpath, name);

		/*
		* swpath_get_is_catalog() returns 
		*            -1 for leading directories
		*             1 for /catalog/
		*             0 for storage
		*/
		if (swpath_get_is_catalog(swpath) == SWPATH_CTYPE_DIR) {
			/*
			* Leading directories.  They belong in the storage
			* file.
			*/
			if (which_file == 0)
				writeit = nullfd;
			else 
				writeit = ofd;
		} else if (swpath_get_is_catalog(swpath) == SWPATH_CTYPE_CAT) {
			/*
			* /catalog/
			*/
			if (which_file == 0)
				writeit = ofd;
			else	
				writeit = nullfd;
		} else if (swpath_get_is_catalog(swpath) == SWPATH_CTYPE_STORE) {
			/*
			* storage section.
			*/
			if (which_file == 0) {
				/*
				* Must be finished writing the catalog section.
				* Now write out the null blocks if tar format.
				*/
				do_trailer = 1;
				break;
			} else { 			
				if (do_adjunct_md5) {
					int filetype = 
						xformat_get_tar_typeflag(
								package);
					if (filetype != REGTYPE && 
							filetype != DIRTYPE)
						writeit = nullfd;
					else
						writeit = ofd;
				} else {
					writeit = ofd;
				}
			}
		} else {
			/*
			* Internal error.
			*/
			SWLIB_INTERNAL("internal error returned by swpath_get_is_catalog");
			retval = -1;
			break;
		}

		bytesret = 0;
		xformat_set_ofd(package, writeit);
		bytesret += xformat_write_header(package);
		bytesret += xformat_copy_pass(package, writeit, ifd);
		if (writeit != nullfd) {
			bytes += bytesret;
		}
	}

	if (do_trailer && ( format == arf_ustar || format == arf_tar )) {
		uxfio_write(ofd, nullblock, 512);
		uxfio_write(ofd, nullblock, 512);
	}

	if (which_file == 1) {
		/*
		* write out the trailer.
		*/
		retval += taru_write_archive_trailer(package->taruM, 
				arf_ustar, ofd, 512, 
				(int)bytes, 
				xformat_get_tarheader_flags(package));
	}

	swbis_devnull_close(nullfd);
	strob_close(namebuf);
	swpath_close(swpath);
	return retval;
}

int
swlib_kill_all_pids(pid_t * pid, int num, int signo, int verbose_level)
{
	int i;
	int ret = 0;
	for(i=0; i<num; i++) {
		if (pid[i] > 0) {
			swlib_doif_writef(verbose_level,  SWC_VERBOSE_SWIDB,
				/* (struct sw_logspec *) */(NULL), STDERR_FILENO,
				"swlib_kill_all_pids: kill[%d] signo=%d\n",
							(int)pid[i], signo);
			E_DEBUG3("killing pid %d with signal %d", (int)pid[i], signo);
			if (kill(pid[i], signo) < 0) {
				swlib_doif_writef(verbose_level,  SWC_VERBOSE_SWIDB,
					/* (struct sw_logspec *)*/ (NULL), STDERR_FILENO,
					"kill[%d] signo=%d : error : %s\n",
						(int)pid[i],
						signo, strerror(errno));
				ret++;
			}
		}
	}
	return ret;
}

int
swlib_update_pid_status(pid_t keypid, int value, pid_t * pid, int * status, int len)
{
	int i = 0;
	if (len == 0) return 0;
	for(i=0; i<len; i++) {
		if (pid[i] == keypid) {
			status[i] = value;
			pid[i] = -pid[i];
			return 0;
		}
	}
	return -1;
}
int
swlib_wait_on_all_pids(pid_t * pid, int num, int * status, 
				int flags, int verbose_level)
{
	return
		swlib_wait_on_all_pids_with_timeout(pid, num, status, flags, verbose_level, 0);
}

int
swlib_wait_on_pid_with_timeout(pid_t pid, int * status, int flags, int verbose_level, int fp_tmo)
{
	pid_t apid;
	int ret;
	apid = pid;
	ret = swlib_wait_on_all_pids_with_timeout(&apid, 1, status, flags, verbose_level, fp_tmo);
	return ret;
}

int
swlib_wait_on_all_pids_with_timeout(pid_t * pid, int num, int * status, 
				int flags, int verbose_level, int fp_tmo)
{
	time_t now;
	time_t start;
	int wret = -99;
	int done = 0;
	int i = 0;
	int got_one = 0;
	int tmo;
	int do_kill;

	do_kill = 0;
	if (fp_tmo < 0) {
		do_kill = 1;
		tmo = -fp_tmo;
	} else {
		tmo = fp_tmo;
	}

	if (num == 0) return 0;
	E_DEBUG("");
	start = time(NULL);
	now = start;

	while (!done && (tmo == 0 || (int)(now - start) < tmo)) {
		done = 1;
		now = time(NULL);
		E_DEBUG("");
		for(i=0; i<num; i++) {
			if (pid[i] > 0) {
				E_DEBUG2("pid=%d", (int)pid[i]);
				wret = waitpid(pid[i], &status[i], flags);
				E_DEBUG2("wret=%d", wret);
				if (wret < 0) {
					swlib_doif_writef(verbose_level,
						SWC_VERBOSE_SWIDB, 
						NULL, STDERR_FILENO,
				"swlib_wait_on_all_pids[%d]: error : %d %s\n",
						(int)pid[i], (int)pid[i], 
						strerror(errno));
					status[i] = 0;
					pid[i] = -pid[i];
				} else if (wret == 0) {
					swlib_doif_writef(verbose_level,
						SWC_VERBOSE_SWIDB,
						NULL, STDERR_FILENO,
"swlib_wait_on_all_pids[%d]: returned zero waiting for process id %d\n",
					(int)pid[i], (int)pid[i]);
					done = 0;
				} else {
					swlib_doif_writef(verbose_level,
					SWC_VERBOSE_SWIDB,
					NULL, STDERR_FILENO,
		"swlib_wait_on_all_pids[%d]: returned exitval=%d\n",
					(int)pid[i], WEXITSTATUS(status[i]));
					got_one = 1;
					pid[i] = -pid[i];
				}
			} else {
				E_DEBUG2("already processed: pid=%d", (int)pid[i]);
				;
			}
		}
		if (flags == WNOHANG)
			done = 1;
	}
	if (do_kill) {
		E_DEBUG("killing pids SIGINT");
		swlib_kill_all_pids(pid, num, SIGINT, 3);
		sleep(1);
		E_DEBUG("killing pids SIGKILL");
		swlib_kill_all_pids(pid, num, SIGKILL, 3);
	}
	return wret; /* Now retval of last waitpid()*/   /* was status[num-1]; */
}

int 
swlib_sha1(int uxfio_fd, char *digest) {
	int i;
	int ret;	
	unsigned char resblock[21];
	int digest_hex_bytes = 40;
	char * p;

	ret = sha_stream(uxfio_fd, resblock);
	p = digest;
	for (i = 0; i < (digest_hex_bytes / 2); ++i) {
		sprintf(p,"%02x", resblock[i]);
		p += 2;
	}
	digest[digest_hex_bytes] = '\0';
	return ret;
}

int
swlib_digests(int ifd, char * md5, char * sha1, char * size, char * sha512)
{
	intmax_t im;
	FILE_DIGS * digs;
	digs = taru_digs_create();
	if (md5)
		digs->do_md5 = DIGS_ENABLE_ON;	
	if (sha1)
		digs->do_sha1 = DIGS_ENABLE_ON;	
	if (sha512)
		digs->do_sha512 = DIGS_ENABLE_ON;	
	if (size)
		digs->do_size = DIGS_ENABLE_ON;	

	im = swlib_digs_copy(-1, ifd, -1, digs, -1);

	if (md5) strcpy(md5, digs->md5);
	if (sha1) strcpy(sha1, digs->sha1);
	if (sha512) strcpy(sha512, digs->sha512);
	if (size) strcpy(size, digs->size);
	taru_digs_delete(digs);
	if (im >= 0) return 0;
	return -1;
}

intmax_t
swlib_md5_copy(int ifd, intmax_t count, char * md5, int ofd)
{
	intmax_t im;
	FILE_DIGS * digs;
	digs = taru_digs_create();
	im = swlib_digs_copy(ofd, ifd, count, digs, -1);
	strcpy(md5, digs->md5);
	taru_digs_delete(digs);
	return im;
}

intmax_t
swlib_digs_copy(int ofd, int ifd, intmax_t count, FILE_DIGS * digs, int adjunct_ofd)
{
	intmax_t n;
	n = swlib_i_digs_copy(ofd, ifd, count, digs, adjunct_ofd, swlib_safe_read);
	return n;
}

int
swlib_shcmd_output_fd(SHCMD ** cmdvec)
{
	int status;
	SHCMD ** vc;
	SHCMD * lastcmd;
	int ot[2];
	pid_t pid;
	int fd;

	vc = cmdvec;
	while(*vc) vc++;
	if (vc == cmdvec) return -1;
	vc--;
	lastcmd = *vc;
	
	fd = uxfio_open("", O_RDONLY, 0);
	if (fd < 0) return fd;
	uxfio_fcntl(fd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	uxfio_fcntl(fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

	pipe(ot);
	pid = swfork(NULL);
	if (pid < 0) return (int)pid;
	if (pid == 0) {
		swgp_signal(SIGPIPE, SIG_DFL);
		swgp_signal(SIGINT, SIG_DFL);
		swgp_signal(SIGTERM, SIG_DFL);
		swgp_signal(SIGUSR1, SIG_DFL);
		swgp_signal(SIGUSR2, SIG_DFL);
		close(ot[0]);
		shcmd_set_dstfd(lastcmd, ot[1]);
		shcmd_cmdvec_exec(cmdvec);
		shcmd_cmdvec_wait2(cmdvec);
		close(ot[1]);
		_exit(0);
	}
	close(ot[1]);
	E_DEBUG("");

	swlib_pipe_pump(fd, ot[0]);

	E_DEBUG("");
	waitpid(pid, &status, 0);

	uxfio_lseek(fd, (off_t)(0), SEEK_SET);
	close(ot[0]);
	return fd;
}

int
swlib_shcmd_output_strob(STROB * output, SHCMD ** cmdvec)
{
	char * base = (char*)NULL;
	int data_len = 0;
	int buffer_len = 0;
	int fd;
	strob_strcpy(output, "");
	fd = swlib_shcmd_output_fd(cmdvec);
	if (fd < 0) return -1;
	if (uxfio_get_dynamic_buffer(fd, &base, &buffer_len, &data_len) < 0)
		return -1;
	strob_strncat(output, base, data_len);
	uxfio_close(fd);
	return 0;
}

mode_t 
swlib_apply_mode_umask(char type, mode_t umask, mode_t mode) { 
	if (mode == 0) {
		/* make up default. */
		if (type == SW_ITYPE_d) {
			mode = 0777;
		} else {
			mode = 0666;
		}
	}
	mode &= ~umask;
	return mode;	
}


int
swlib_open_memfd(void)
{
	int fd;
	fd = uxfio_open("", O_RDWR, 0); 
	if (fd < 0) return fd;
	uxfio_fcntl(fd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	uxfio_fcntl(fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
	return fd;
}

int
swlib_close_memfd(int fd)
{
	return uxfio_close(fd);
}

int
swlib_pad_amount(int fd, int amount)
{
	char nullblock[512];
	int remains = amount;
	int am;
	int ret;
	int count = 0;

	memset(nullblock, '\0', sizeof(nullblock));
	while (remains > 0) {
		if (remains > 512)
			am = 512;
		else
			am = remains;
		if (am <= 0) break;
		ret = atomicio((ssize_t (*)(int, void *, size_t))write,
			fd, nullblock, am);
		
		if (ret <= 0) {
			if (ret < 0)
				fprintf(stderr, "%s: write error: %s\n", swlib_utilname_get(), strerror(errno));
			else
				SWLIB_INTERNAL("");
			if (count <= 0) return -1;
			return -count;
		}
		count += ret;
		remains -= ret;
	}
	return count;
}

int
swlib_drop_root_privilege(void)
{
	int ret = 1;
	if (getuid() == (uid_t)(0)) {
		uid_t nob;
		/*
		 * Only attempt to drop root privilidge if
		 * you are root
		 */
		if (taru_get_uid_by_name(AHS_USERNAME_NOBODY, &nob) < 0) {
			fprintf(stderr, 
	     "%s: Warning: the uname [%s] not found, not dropping privilege.\n",
			swlib_utilname_get(), AHS_USERNAME_NOBODY);
			ret = 3;
		} else {
			if (nob < 10) {
				/*
				 * This is a sanity check, which is only a warning
				 * if it fails.  It assumes that the uid of nobody is >10
				 * this to guard against a hacked /etc/passwd file.
				 */
				fprintf(stderr, "%s: User name 'nobody' has a uid of %d\n",
				swlib_utilname_get(), (int)(nob));
			}
			if(setuid(nob) < 0) {
				fprintf(stderr, 
	     			"%s: setuid(uid=%d) failed at %s:%d: %s\n",
					 swlib_utilname_get(), 
					(int)nob, __FILE__, __LINE__,
					strerror(errno));
				ret = 2;
			} else {
				ret = 0;
			}
		}
	} else {
		ret = 0;
	}
	return ret;
}

void
swlib_add_trailing_slash(STROB *path)
{
	char *p = strob_str(path);
	if (!strlen(p)) {
		strob_strcat(path,"/");
		return;
	}
	if( *(p + strlen(p) -1) != '/')
		strob_strcat(path,"/");
}

int
swlib_altfnmatch(char * s1, char * s2)
{
	STROB * tmp1;
	STROB * tmp2;
	char * t1;
	char * t2;

	tmp1 = strob_open(16);
	tmp2 = strob_open(16);
	t1 = strob_strtok(tmp1, s1, "|\n\r");
	while(t1) {
		t2 = strob_strtok(tmp2, s2, "|\n\r");
		while(t2) {
			if (fnmatch(t1, t2, 0) != FNM_NOMATCH) {
				/*
				 * match
				 */
				strob_close(tmp1);
				strob_close(tmp2);
				return 0;
			}	
			t2 = strob_strtok(tmp2, NULL, "|\n\r");
		}
		t1 = strob_strtok(tmp1, NULL, "|\n\r");
	}
	strob_close(tmp1);
	strob_close(tmp2);
	return 1;
}

int
swlib_unexpand_escapes(STROB * store, char * src)
{
	int n;
	int k;
	char * lp;
	int retval = 0;
	int len;
	int escaped_value;
	int is_recognized;
	char * dst;

	if (store == (STROB*)NULL) {
		dst = src;
	} else {
		strob_set_length(store, strlen(src));
		strob_strcpy(store, "");
		dst = strob_str(store);
	}
	k = 0;
	lp = src;
	len = 0;
	n = strlen(src);
	while (k < n) {
		if (*(lp + k) == '\\' && *(lp + k + 1) != '\0' ) {
			escaped_value = swlib_ansi_escaped_value((int)(*(lp + k + 1)), &is_recognized);
			if (!is_recognized) {
				escaped_value = swlib_c701_escaped_value((int)(*(lp + k + 1)), &is_recognized);
			}
			if (escaped_value < 0) {
				dst[len] = '\0';
				SWLIB_ASSERT(0); /* Fatal Error */	
			}
			if (is_recognized == 0) {
				/* Not a recognized escape */
				dst[len] = *(lp + k);
				len++;
				k++;
			} else {
				/* recognized escape */
				dst[len] = escaped_value;
				len++;
				k+=2;
			}
		} else {
			dst[len] = *(lp + k);
			len++;
			k++;
		}
	}
	dst[len] = '\0';
	return retval;
}

int
swlib_unexpand_escapes2(char * src)
{
	int ret;
	STROB * dst;

	dst = strob_open(10);
	ret = swlib_unexpand_escapes(dst, src);
	if (ret) return ret;
	if (strob_strlen(dst) > strlen(src)) return -1;
	strcpy(src, strob_str(dst));
	strob_close(dst);
	return 0;
}

int
swlib_tee_to_file(char * filename, int ifd, char * buf, int len, int do_append)
{
	int ret;
	int fd;
	int res;
	int flags;
	flags = O_RDWR|O_CREAT|O_TRUNC;
	if (do_append)
		flags = O_RDWR|O_CREAT|O_APPEND;
	if (isdigit((int)(*filename))) {
		fd = swlib_atoi(filename, &res);
		if (res) return -1;
	} else {
		fd = open(filename, flags, 0644);
	}
	if (fd < 0) return fd;

	if (ifd < 0) {
		if (len < 0) {
			ret = uxfio_unix_safe_write(fd, buf, strlen(buf));
			if (ret != (int)strlen(buf)) return -1;
		} else {
			ret = uxfio_unix_safe_write(fd, buf, len);
			if (ret != (int)len) return -1;
		}
	} else {
		ret = swlib_pipe_pump(fd, ifd);
	}
	close(fd);
	return ret;
}

int
swlib_expand_escapes(char **pa, int *newlen, char *src, STROB * ustore)
{
	int n = strlen(src);
	int count = 0;
	int i = 0, j = 0, k = 0;
	char *lp = src;
	STROB * store;

	if (ustore) {
		store = ustore;
		strob_strcpy(store, "");
	} else {
		store = strob_open(20);
	}
	if (!store)
		return -1;
	while (k < n) {
		if (*(lp + k) == '\\') {
			count++;
			if (count % 2 && (*(lp + k + 1) == 'n')) {
				count=0;
				strob_chr_index(store, i + j, '\n');
				 k++;
			} else {
				strob_chr_index(store, i + j, *(lp + k));
				if (count % 2) {
					if (
					!swlib_is_ansi_escape(*(lp + k + 1)) &&
					!swlib_is_c701_escape(*(lp + k + 1))
					) {
						j++;
						strob_chr_index(store,
								i + j,
								*(lp + k));
					} else {
						;
					}
				}
			}
		} else if ((*(lp+k) < (int)7 && *(lp+k) > 0) || *((unsigned char*)(lp+k)) >= 127) {
			strob_sprintf(store, 1, "\\x%02X", *((unsigned char*)(lp+k)));
			j+=3;
		} else {
			count = 0;
			if (*(lp + k) == '#') {
				if (strob_get_char(store,  i + j - 1) != '\\') {
					strob_chr_index(store, i + j, '\\');
					j++;
				}	
				strob_chr_index(store,i + j, '#');
			
			} else if (*(lp + k) == '\"') {
				if (strob_get_char(store, i + j - 1) != '\\') {
					strob_chr_index(store, i + j, '\\');
					j++;
			
				}
				strob_chr_index(store, i + j, '\"');
			} else {
				strob_chr_index(store, i + j, *(lp + k));
			}
		}
		i++;
		k++;
	}
	strob_chr_index(store, i + j, '\0');
	if (!ustore) {
		if (pa)
			*pa = strob_release(store);
	} else {
		if (pa)
			*pa = strob_str(store);
	}
	if (newlen)
		*newlen = i + j;
	return 0;
}

char *
swlib_imaxtostr(intmax_t i, STROB * pbuf)
{
	static STROB * sbuf;
	STROB * buf;
	char * ret;
	/* FIXME, maybe need to check for the case where intmax_t is 64bit
	   but LLONG_MAX is 32 bit, because it was not defined by the system
           but rather by swbis in desparation. */

	if (pbuf == NULL) {
		if (sbuf == NULL)
			sbuf = strob_open(32);
		buf = sbuf;
	} else {
		buf = pbuf;	
	}
	strob_setlen(buf, UINTMAX_STRSIZE_BOUND+1);
	ret = imaxtostr(i, strob_str(buf));
	return ret;
}

char *
swlib_umaxtostr(uintmax_t i, STROB * pbuf)
{
	static STROB * buf;
	char * ret;
	/* FIXME, maybe need to check for the case where intmax_t is 64bit
	   but LLONG_MAX is 32 bit, because it was not defined by the system
           but rather by swbis in desparation. */
	
	if (pbuf == NULL) {
		if (buf == NULL)
			buf = strob_open(32);
	} else {
		buf = pbuf;	
	}
	strob_setlen(buf, UINTMAX_STRSIZE_BOUND+1);
	ret = umaxtostr(i, strob_str(buf));
	return ret;
}

mode_t 
swlib_get_umask(void) {
	mode_t mode = 0777;
	mode = umask(mode);
	umask(mode);
	return mode;
}

int
swlib_ascii_text_fd_to_buf(STROB * pbuf, int ifd)
{
	intmax_t amount;
	int ret;

	amount = -1;
	ret = swlib_i_pipe_pump(ifd,
			-1,
			(intmax_t *)&amount,
			-1,
			pbuf,
			uxfio_read);
	if (
		ret != 0 ||
		(int)amount != (int)strob_strlen(pbuf))
	{
		fprintf(stderr, "swlib_ascii_text_fd_to_buf: warning, size not equal to strlen\n");
		ret = -1;
	}
	return ret;
}

void
swlib_apply_location(STROB * relocated_path, char * path, char * location, char * directory)
{
	char * prefix;
	char * clean_directory;
	char * clean_location;
	char * clean_path;

	strob_strcpy(relocated_path, "");	
	if (strlen(path) == 0) return;

	clean_path = swlib_return_no_leading(path);
	clean_location = swlib_return_no_leading(location);
	clean_directory = swlib_return_no_leading(directory);

	if (strlen(clean_directory) > 0 && strlen(clean_path) > 0) {
		prefix = strstr(clean_path, clean_directory);
		if (prefix != NULL && prefix == clean_path) {
			 clean_path += strlen(clean_directory);
		}
	} else {
		;
	}
	strob_strcpy(relocated_path, location);
	swlib_unix_dircat(relocated_path, clean_path);
	return;
}

char *
swlib_attribute_check_default(char * object, char * keyword, char * value)
{
	char * d;
	if (value) return value;
	d = swsdflt_get_default_value(object, keyword);
	if (d == NULL) {	
		SWLIB_ERROR3("no default value found for attribute: %s %s", object, keyword);
		return "";
	}
	return d;
}

int
swlib_is_option_true(char * s)
{
	if (!s) return 0;
	return swextopt_is_value_true(s);
}

void
swlib_squash_illegal_tag_chars(char * s)
{
	while(*s != (int)'\0') {
		if (*s == '.') *s = '_';
		if (*s == ',') *s = '_';
		if (*s == ':') *s = '_';
		s++;
	}
}

void
swlib_append_synct_eof(STROB * buf)
{
	/* Here is the SWBIS_SYNCT_EOF */
	/*  Here's how to make it:  printf 'TRAILER!!!\r\n\r\n' | dd conv=synv bs=256 */
	E_DEBUG("");

	strob_sprintf(buf, 1, /* append */
		") | dd ibs=%d obs=%d 2>/dev/null | dd ibs=%d obs=%d conv=sync 2>/dev/null |\n"
		"	(\n"
		"		dd bs=%d 2>/dev/null\n" 
		"		( printf \"" CPIO_INBAND_EOA_FILENAME "\\r\\n\\r\\n\"\n" 
		"		) | dd ibs=%d obs=%d conv=sync 2>/dev/null\n"
		"	)\n"
		") | dd ibs=%d obs=%d conv=sync 2>/dev/null\n",
		SWLIB_SYNCT_BLOCKSIZE*3, SWLIB_SYNCT_BLOCKSIZE*3, SWLIB_SYNCT_BLOCKSIZE*2, SWLIB_SYNCT_BLOCKSIZE*2,
		SWLIB_SYNCT_BLOCKSIZE*2,
		SWLIB_SYNCT_BLOCKSIZE*2, SWLIB_SYNCT_BLOCKSIZE*2,
		SWLIB_SYNCT_BLOCKSIZE*2, SWLIB_SYNCT_BLOCKSIZE*2
		);
	return;
}
