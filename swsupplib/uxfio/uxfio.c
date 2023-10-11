/* uxfio.c : buffered u*ix I/O functions.
 */

/*
 * Copyright (C) 1997-2004,2006 James H. Lowe, Jr. <jhlowe@acm.org>
 * All Rights Reserved.
 *
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

#include "swuser_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "uxfio.h"

#define o__inline__

#include "uxfio_i.h"

#include "debug_config.h"

#define UXFIONEEDDEBUG 1
#undef UXFIONEEDDEBUG 

#ifdef UXFIONEEDFAIL
#define UXFIO_E_FAIL(format) SWBISERROR("UXFIO INTERNAL ERROR: ", format)
#define UXFIO_E_FAIL2(format, arg) SWBISERROR2("UXFIO INTERNAL ERROR: ", format, arg)
#define UXFIO_E_FAIL3(format, arg, arg1) SWBISERROR3("UXFIO INTERNAL ERROR: ", format, arg, arg1)
#else
#define UXFIO_E_FAIL(arg)
#define UXFIO_E_FAIL2(arg, arg1)
#define UXFIO_E_FAIL3(arg, arg1, arg2)
#endif

#ifdef UXFIONEEDDEBUG
#define UXFIO_E_DEBUG(format) SWBISERROR("UXFIO DEBUG: ", format)
#define UXFIO_E_DEBUG2(format, arg) SWBISERROR2("UXFIO DEBUG: ", format, arg)
#define UXFIO_E_DEBUG3(format, arg, arg1) \
			SWBISERROR3("UXFIO DEBUG: ", format, arg, arg1)
#else
#define UXFIO_E_DEBUG(arg)
#define UXFIO_E_DEBUG2(arg, arg1)
#define UXFIO_E_DEBUG3(arg, arg1, arg2)
#endif /* UXFIONEEDDEBUG */


static unsigned char uxg_status_table[UXFIO_MAX_OPEN];
static int uxg_desc_table[UXFIO_MAX_OPEN];
static UXFIO * uxg_file_table[UXFIO_MAX_OPEN];
static int uxg_did_init = 0;
static int uxg_nullfd = 0;


#define UXFIO_REMOVE 0
#define UXFIO_ADD 1
#define UXFIO_FIND 2
#define UXFIO_FINDINDEX 3
#define UXFIO_INIT 4
#define UXFIO_QUERY_NOPEN 5

#define UXFIO_BUFFER_CMD_GET 0
#define UXFIO_BUFFER_CMD_SET 1
#define UXFIO_ALLOC_AHEAD 20480

static int table_mon(int cmd, int *uxfio_fildes, UXFIO **uxfio_addr);
static int internal_uxfio_open(UXFIO ** uxfio_aa);
static int internal_uxfio_close(UXFIO * uxfio);
static int bufferstate(UXFIO * uxfio, Xint *left, Xint *right, Xint *unused);
static int fix_buffer(int uxfiofd, UXFIO * uxfio, Xint totalbytes, char *p, Xint new_bytes);
static int fix_dynamic_buffer(int uxfiofd, UXFIO * uxfio, Xint totalbytes, char *p, Xint totalnewbytes);
static ssize_t uxfio_common_read(UXFIO * uxfio,
		int uxfio_fildes, void *buf, size_t nbyte, int pollingread);
static void *uxfio__memmove(void *dest, Xint dest_offset,
					void *src, Xint src_offset,
			 		   Xint amount, UXFIO * uxfio);
static void *uxfio__memcpy(void *dest, Xint dest_offset,
			void *src, Xint src_offset,
			   Xint amount, UXFIO * uxfio);
static int uxfio__delete_buffer_file(UXFIO * uxfio);
static int uxfio__init_buffer_file(UXFIO * uxfio, void *buf, int len);
static void *do_file_buffer_move(void *dest, Xint dest_offset,
				void *src, Xint src_offset,
					 Xint amount, UXFIO * uxfio);
static int uxfio__read_bytes(int uxfio_fildes, int amount);
static Xint enforce_veof(UXFIO * uxfio, Xint nbyte, int * einval);
static Xint enforce_set_position(UXFIO * uxfio, Xint nbyte, int * einval);
static void uxfio__change_state(UXFIO * uxfio, int u);
static void copy_stat_members (struct stat * buf, struct stat * statbuf);
static int uxfio__get_state(UXFIO * uxfio);
static int uxfio__unlink_tmpfile(UXFIO *uxfio);
static int uxfio__close(int uxfio_fildes, int closeit);
char * uxfio_dump_string_from_object(UXFIO * object);
int uxfio_decr_use_count(int fd);
static int do_buffered_flush(UXFIO * uxfio);

static
int
zero_fill(int fd, int amount)
{
	int ret;
	int rem;
	size_t am;
	char buf[512];
	memset(buf, '\0', sizeof(buf));
	rem = amount;
	while (rem > 0) {
		if (rem > (int)sizeof(buf))
			am = sizeof(buf);
		else
			am = (size_t)rem;
		ret = uxfio_write(fd, (void*)buf, am);
		if (ret < 0) return -1;
		rem -= ret;
	}
	return 0;	
}

static 
Xint 
virtual_eof_active(UXFIO * uxfio) {
	return (
		uxfio->offset_eofM >= 0 || 
		uxfio->v_endM >= 0 ||
		0	
		);
}

static 
Xint 
virtual_file_active(UXFIO * uxfio) {
	return (
		uxfio->offset_eofM >= 0 ||
		uxfio->offset_eof_savedM >= 0 ||
		0	
		);
}

static 
int 
active_virtual_file_offset(UXFIO * uxfio) {
	if (virtual_file_active(uxfio)) {
		return  uxfio->offset_bofM;
	} else {
		return 0;
	}
}

static 
int 
bufferstate(UXFIO * uxfio, Xint *left, Xint *right, Xint *unused)
{
	if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
		if (lseek(uxfio->buffdM, (off_t) 0, SEEK_CUR) != uxfio->posM) {
			UXFIO_E_DEBUG("error");
			fprintf(stderr,
			"uxfio internal error: buffer file position.\n");
		}
	}	
			/* #bytes left of curr pos */
	*left = uxfio->posM;
			/* #bytes right of and including current position */
	*right = uxfio->endM - uxfio->posM;	
			/* #bytes past the end of valid data */
	*unused = uxfio->lenM - uxfio->endM;
	UXFIO_E_DEBUG3("left=%d right=%d", (int)(*left), (int)(*right));
	return 0;
}

static
void
alloc_file(int i)
{
	if (uxg_file_table[i] == NULL) {
		uxg_file_table[i] = (UXFIO*)malloc(sizeof(UXFIO));
	}
}

static 
int 
table_find(int uxfio_fildes, UXFIO ** uxfio_addr, int v)
{
	static int last_used_fd_index;
	int  i;

	if (uxg_desc_table[last_used_fd_index] == uxfio_fildes) {
		if (last_used_fd_index < UXFIO_MAX_OPEN &&
				uxg_status_table[last_used_fd_index] > 0) {
			alloc_file(last_used_fd_index);
			*uxfio_addr = (void *)(uxg_file_table[last_used_fd_index]);
			return 0;
		}
	}

	if (uxfio_fildes >= UXFIO_FD_MIN &&
		uxfio_fildes < UXFIO_MAX_OPEN + UXFIO_FD_MIN) {
		for (i = 0; i < UXFIO_MAX_OPEN; i++) {
			if (uxg_desc_table[i] == uxfio_fildes) {
				break;
			}
		}
		if (i < UXFIO_MAX_OPEN && uxg_status_table[i] > 0) {
			alloc_file(i);
			*uxfio_addr = (void *)(uxg_file_table[i]);
			last_used_fd_index = i;
			return 0;
		}
	}
	*uxfio_addr = (void *) (NULL);
	if (uxfio_fildes == -1) return -1;

	UXFIO_E_DEBUG2("error %d", uxfio_fildes);
	if (v) fprintf(stderr, "uxfio error [%d]: descriptor %d not found.\n", __LINE__, uxfio_fildes);
	return -1;
}

static 
int 
table_mon(int cmd, int *uxfio_fildes, UXFIO ** uxfio_addr)
{
	int p_index, index=0, i;

	p_index = *uxfio_fildes;

	/* make uxfio descriptors begin at UXFIO_FD_MIN */

	if (cmd == UXFIO_FIND) {
		return table_find(p_index, uxfio_addr, 1);
	} else if (cmd == UXFIO_REMOVE) {
		for (i = 0; i < UXFIO_MAX_OPEN; i++) {
			if (uxg_desc_table[i] == p_index) {
				index = i;
				break;
			}
		}
		if (i < UXFIO_MAX_OPEN && uxg_status_table[index] > 0) {
			uxg_status_table[index] = 0;
			return 0;
		} else {
			return -1;
		}
	} else if (cmd == UXFIO_ADD) {
		for (i = 0; i < UXFIO_MAX_OPEN; i++) {
			if (uxg_status_table[i] == 0) {
				uxg_status_table[i] = 1;
				uxg_desc_table[i] = UXFIO_FD_MIN + i;
				alloc_file(i);
				*uxfio_addr = (void *)(uxg_file_table[i]);
				*uxfio_fildes = UXFIO_FD_MIN + i;
				return 0;
			}
		}
		fprintf(stderr, "uxfio: too many open files, UXFIO_MAX_OPEN=%d\n", UXFIO_MAX_OPEN);
		*uxfio_addr = (void *) (NULL);
		return -1;
	} else if (cmd == UXFIO_INIT) {
		for (i = 0; i < UXFIO_MAX_OPEN; i++) {
			uxg_file_table[i] = (UXFIO*)NULL;
		}
		uxg_did_init = 1;
		return 0;
	} else if (cmd == UXFIO_QUERY_NOPEN) {
		int nopen;
		nopen = 0;
		for (i = 0; i < UXFIO_MAX_OPEN; i++) {
			if (uxg_status_table[i] != 0) {
				nopen++;
			}
		}
		return nopen;
	} else {
		UXFIO_E_DEBUG("error");
		fprintf(stderr,
			"uxfio internal error: invalid table command.\n");
		return -2;
	}
}

static 
int 
internal_uxfio_open(UXFIO ** uxfio_aa)
{
	int uxfio_fd;
	UXFIO *uxfio;

	if (!uxg_did_init) table_mon(UXFIO_INIT, &uxfio_fd, uxfio_aa);
	if (table_mon(UXFIO_ADD, &uxfio_fd, uxfio_aa)) {
		return -1;
	}
	/* fprintf(stderr, "OPENING %d\n", uxfio_fd); */

	uxfio = *uxfio_aa;
	uxfio->uxfio_fildesM = uxfio_fd;
	uxfio->buffertypeM = UXFIO_BUFTYPE_MEM;
	uxfio->posM = 0;
	uxfio->lenM = UXFIO_LEN;
	uxfio->startM = 0;
	uxfio->endM = 0;
	uxfio->errorM = 0;
	uxfio->buffer_activeM = 1;
	uxfio->buffdM = -1;
	uxfio->offset_eofM = -1;
	uxfio->offset_bofM = 0;
	uxfio->offset_eof_savedM = -1;
	uxfio->bytesreadM = 0;
	uxfio->current_offsetM = 0;
	uxfio->virtual_offsetM = 0;
	uxfio->auto_arm_delayM = 0;
	uxfio->use_countM=1;
	uxfio->auto_disableM = 0;
	uxfio->write_insertM = 0;
	uxfio->buffilenameM[0] = '\0';
	uxfio->tmpfile_rootdirM = (char *)(NULL);
	uxfio->v_endM = -1;
	uxfio->statbufM=NULL;
	uxfio->did_dupe_fdM = -1;
	uxfio->lock_buf_fatalM = 0;
	uxfio->output_block_sizeM = 0;
	uxfio->output_buffer_cM = 0;
	uxfio->output_bufferM = (char*)NULL;
	UXFIO_E_DEBUG3("malloc %d %d", uxfio_fd, UXFIO_LEN);
	if ((uxfio->bufM = (char *) malloc(UXFIO_LEN + 1)) == (char *) (NULL)) {
		table_mon(UXFIO_REMOVE, &uxfio_fd, &uxfio);
		return -1;
	}
	uxfio__change_state(uxfio, 0);
	return uxfio_fd;
}

static 
int
uxfio__get_state(UXFIO * uxfio)
{
	if (uxfio->vir_fsyncM==uxfio_fsync)
		return 1;
	else
		return 0;
}

static 
void
uxfio__change_state(UXFIO * uxfio, int u)
{
	if (u) {
		uxfio->vir_fsyncM = uxfio_fsync;
		uxfio->vir_readM = uxfio_read;
		uxfio->vir_tread_readM = uxfio_sfread;
		uxfio->vir_writeM = uxfio_write;
		uxfio->vir_closeM = uxfio_close;
		uxfio->vir_lseekM = uxfio_lseek;
		uxfio->vir_ftruncateM = uxfio_ftruncate;
	} else {
		uxfio->vir_fsyncM = fsync;
		uxfio->vir_readM = (ssize_t (*)
			(int  filedes, void * buf, size_t nbytes))
					(uxfio_unix_safe_read);
		uxfio->vir_tread_readM = (ssize_t (*)
		      (int  filedes, void * buf, size_t nbytes))
						(uxfio_unix_safe_atomic_read);
		uxfio->vir_writeM = (ssize_t (*)
			(int  filedes, void * buf, size_t nbytes))
						(uxfio_unix_safe_write);
		uxfio->vir_closeM = close;
		uxfio->vir_lseekM = lseek;
		uxfio->vir_ftruncateM = ftruncate;
	}
}

static 
int 
fix_buffer(int uxfiofd, UXFIO * uxfio, Xint totalbytes,
					char *p, Xint totalnewbytes)
{
	Xint n, left, right, unused, ret;

	UXFIO_E_DEBUG2("ENTERING %d", uxfiofd);
	/* p: is the user's buffer in the _read and _write functions */
	/* totalbytes: are the total # of bytes in the transaction */

	if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
		UXFIO_E_DEBUG("BUFTYPE_FILE");
		if (uxfio->posM + totalnewbytes > uxfio->lenM) {
			/* won't try to shift the buffer,
		   	   its probably a big file */
			UXFIO_E_DEBUG("LEAVING");
			return -1;
		}
		if (totalnewbytes) {	/* some of the user file was read */
			/* store it in uxfio->buffdM */
			UXFIO_E_DEBUG("in totalbewbytes");

			n = (Xint)lseek(uxfio->buffdM, (off_t) 0, SEEK_END);
			if (n != uxfio->endM) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
	"uxfio internal error: fix_buffer uxfio->endM invalid value.\n");
				UXFIO_E_DEBUG("LEAVING");
				return -1;
			}
			n = (size_t)uxfio_unix_atomic_write(uxfio->buffdM,
				p + (int)totalbytes - totalnewbytes,
				totalnewbytes);
			if (n < 0) {
				perror("exiting in uxfio fix_buffer");
				UXFIO_E_DEBUG("LEAVING");
				exit(errno);
			}
			uxfio->posM += totalbytes;
/*+*/			uxfio->virtual_offsetM += totalbytes;
			uxfio->endM = uxfio->posM;
			uxfio->startM = 0;
		} else {
			UXFIO_E_DEBUG("in totalbewbytes else");
			uxfio->posM += totalbytes;
/*+*/			uxfio->virtual_offsetM += totalbytes;
			/* uxfio->endM stays the same */
			uxfio->startM = 0;
		}
	} else {
		UXFIO_E_DEBUG("not BUFTYPE_FILE");
		/* below services the memory segment */

		UXFIO_E_DEBUG("in totalbewbytes else");
		bufferstate(uxfio, &left, &right, &unused);
		/* read past available buffer data */
		if (totalnewbytes) {
			UXFIO_E_DEBUG("entire buffer gets replaced");
			/* entire buffer gets replaced */
			if (totalbytes >= uxfio->lenM) {
	
				if (uxfio->buffertypeM ==
						UXFIO_BUFTYPE_DYNAMIC_MEM) {
					ret = fix_dynamic_buffer(uxfiofd,
						uxfio, totalbytes, p,
						totalnewbytes);
					UXFIO_E_DEBUG("LEAVING");
					return ret;
				}	
				uxfio__memcpy(uxfio->bufM, 0, p,
						totalbytes - uxfio->lenM,
						uxfio->lenM, uxfio);
				uxfio->posM = uxfio->lenM;
/*+*/				uxfio->virtual_offsetM = uxfio->lenM;
				uxfio->endM = uxfio->lenM;
				uxfio->startM = 0;
				/* return 0; */
	
				/* the read fits into the buffer but it has
				  to be shifted somewhat to the left */
			} else if (totalbytes > right + unused) {
				UXFIO_E_DEBUG("totalbytes > right + unused");
				if (uxfio->buffertypeM ==
						UXFIO_BUFTYPE_DYNAMIC_MEM) {
					UXFIO_E_DEBUG("BUFTYPE_DYNAMIC_MEM");
					ret = fix_dynamic_buffer(uxfiofd,
						 uxfio, totalbytes, p,
						totalnewbytes);
					UXFIO_E_DEBUG("LEAVING");
					return ret;
				}	
				uxfio__memmove(uxfio->bufM, 0,
					uxfio->bufM,
					totalbytes - unused - right,
					uxfio->lenM - totalbytes + right,
					uxfio);
				uxfio->posM = uxfio->lenM;
/*+*/				uxfio->virtual_offsetM = uxfio->lenM;
				uxfio->endM = uxfio->posM;
				uxfio->startM = 0;
				uxfio__memcpy(uxfio->bufM,
					(uxfio->lenM - totalbytes), p,
					0, totalbytes, uxfio);
				/* return 0; */
	
				/* no shifting is required, sufficient space */
				/* in buffer. */
			} else if (totalbytes > right) {
				UXFIO_E_DEBUG("totalbytes > right");
				uxfio__memcpy(uxfio->bufM, uxfio->posM,
						p, 0, totalbytes, uxfio);
				uxfio->posM = uxfio->posM + totalbytes;
/*+*/				uxfio->virtual_offsetM += totalbytes;
				uxfio->endM = uxfio->posM;
				uxfio->startM = 0;
				/* read came entirely from exiting buffer. */
			} else {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
					"uxfio internal error: 10005.3 \n");
				UXFIO_E_DEBUG("LEAVING");
				return -1;
			}
		} else {
		 	/* the unix read was not called,
			all data was in buffer 
			because position was somewhere in middle
			and totalbytes
			was less than 'right' */
			uxfio->posM += totalbytes;
		}
	}
	UXFIO_E_DEBUG2("LEAVING %d", uxfiofd);
	return 0;
}

static 
int 
fix_dynamic_buffer(int uxfiofd, UXFIO * uxfio, Xint totalbytes,
			char *p, Xint totalnewbytes)
{
	Xint left, right, unused;
	char *newbuf;

	UXFIO_E_DEBUG2("ENTERING %d", uxfiofd);
	bufferstate(uxfio, &left, &right, &unused);

	if (totalnewbytes > 0) {
		UXFIO_E_DEBUG3("realloc %d %d",
				uxfiofd,
			(int)(uxfio->lenM + totalnewbytes + UXFIO_ALLOC_AHEAD + 1));
		newbuf = (char *) SWBIS_REALLOC(uxfio->bufM,
			uxfio->lenM + totalnewbytes + UXFIO_ALLOC_AHEAD + 1,
					uxfio->lenM);
		if (!newbuf) {
			fprintf(stderr, "uxfio: fatal: out of memory at line %d\n",
					__LINE__);
			exit(22);
		}
		memcpy(newbuf + uxfio->posM, p, totalbytes);
		uxfio->lenM += (totalnewbytes + UXFIO_ALLOC_AHEAD);
	} else {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio internal error: fix_dynamic_buffer.\n");
		exit(1);
	}

	uxfio->posM += totalbytes;
	if  (uxfio->posM > uxfio->endM)
		uxfio->endM = uxfio->posM;

	/* set the  unused memory to all NULS including a sneak
	   extra byte that is untracked but always present.  */
	memset(newbuf + uxfio->endM, (int)'\0', uxfio->lenM + 1 - uxfio->endM);

	uxfio->startM = 0;
	if (newbuf != uxfio->bufM) {
		if (uxfio->lock_buf_fatalM) {
			fprintf(stderr,
"uxfio: Realloc moved pointer, fatal according to current configuration.\n");
			fprintf(stderr,
				"uxfio: Fatal.. exiting with status 38.\n");
			exit(38);
		}
		UXFIO_E_DEBUG2("ADDR CHANGE current bufM = %p", uxfio->bufM);
	}
	UXFIO_E_DEBUG2("current bufM = %p", uxfio->bufM);
	UXFIO_E_DEBUG2("newbuf       = %p", newbuf);
	uxfio->bufM = newbuf;
	UXFIO_E_DEBUG3("posM = %d   endM = %d", (int)(uxfio->posM), (int)uxfio->endM);
	UXFIO_E_DEBUG2("LEAVING %d", uxfiofd);
	return 0;
}

static 
int 
uxfio__read_bytes(int uxfio_fildes, int amount)
{
	char buf[4096];
	int n, bite, haveread = 0;

	while (amount) {
		bite = (amount > (int)sizeof(buf)) ? (int)sizeof(buf) : amount;
		if ((n = uxfio_read(uxfio_fildes, buf, bite)) < 0) {
			return -1;
		}
		haveread += n;
		amount -= n;
	}
	return haveread;
}

static 
int 
internal_uxfio_close(UXFIO * uxfio)
{
	if (uxfio->use_countM > 1) {
		uxfio->use_countM--;
		return -1;
	}
	if (uxfio->bufM) swbis_free(uxfio->bufM);
	if (uxfio->statbufM) {
		swbis_free(uxfio->statbufM);
		uxfio->statbufM = NULL;
	}
	uxfio__delete_buffer_file(uxfio);
	if (uxfio->tmpfile_rootdirM) swbis_free(uxfio->tmpfile_rootdirM);
	return uxfio->uxfdM;
}

static 
int 
uxfio__close(int uxfio_fildes, int closeit)
{
	UXFIO *uxfio;
	int fd;
	int ret = 0;

	if (uxfio_fildes == uxg_nullfd)
		return 0;

	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0) {
		UXFIO_E_DEBUG2("unix filedes=%d", uxfio_fildes);
		return close(uxfio_fildes);
	}

	if (table_find(uxfio_fildes, &uxfio, 1)) {
		return -1;
	}

	if (uxfio->did_dupe_fdM >= 0) {
		uxfio_decr_use_count(uxfio->did_dupe_fdM);
	}

	fd=internal_uxfio_close(uxfio);
	if (fd < 0 && fd != UXFIO_NULL_FD) {  /* still in use */
		UXFIO_E_DEBUG3("file still in use: filedes=%d use_count=%d",
					uxfio_fildes, uxfio->use_countM);
		if (closeit) {
			UXFIO_E_DEBUG2("filedes=%d", uxfio_fildes);
			return 0;
		} else {
			UXFIO_E_DEBUG2("filedes=%d", uxfio_fildes);
			return -1;
		}
	} else if (fd >= 0) {
		if (closeit) {
			UXFIO_E_DEBUG2("filedes=%d", uxfio_fildes);
			ret = do_buffered_flush(uxfio);
			ret = (*(uxfio->vir_closeM))(fd);
		} else {
			UXFIO_E_DEBUG2("filedes=%d", uxfio_fildes);
			ret = fd;
		}
		table_mon(UXFIO_REMOVE, &uxfio_fildes, &uxfio);
		return ret;
	} else {
		table_mon(UXFIO_REMOVE, &uxfio_fildes, &uxfio);
		return 0;
	}
}

static 
ssize_t 
uxfio_common_read(UXFIO *uxfio, int uxfio_fildes, void *buf,
					size_t nbyte_, int pollingread)
{
	char *p;
	int nbyte=(int)nbyte_;
	int n;
	int einval=0;
	Xint bufcount;
	Xint left, right, unused, oldpos;
	ssize_t (*foo_read)(int  filedes, void * buf, size_t nbytes);


	/*
		WARNING: pollingread == 1 may be broken
	*/

	UXFIO_E_DEBUG3("ENTERING fildes = %d  nbyte=%d",
					uxfio_fildes, (int)nbyte);
	p = (char *)buf;

	if (uxfio == NULL) {
		if (table_find(uxfio_fildes, &uxfio, 1)) {
			UXFIO_E_DEBUG2("error %d", uxfio_fildes);
			fprintf(stderr,
		"uxfio error [%d]: uxfio_common_read: file desc %d not found.\n", __LINE__,
						uxfio_fildes);
			UXFIO_E_DEBUG("LEAVING");
			return -1;
		}
	} else {
		uxfio_fildes = uxfio->uxfio_fildesM;
	}
	
	if (  uxfio->buffertypeM == UXFIO_BUFTYPE_FILE || 
	      uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM || 
	      uxfio->offset_eofM >= 0 || 
	      uxfio->v_endM >= 0 )
	{
		UXFIO_E_DEBUG("running enforce_veof");
		nbyte = enforce_veof(uxfio, nbyte, &einval);
		UXFIO_E_DEBUG2("enforce_veof  nbyte(now) = %d", nbyte);
	}
	
	UXFIO_E_DEBUG2("einval = %d", einval);
	if (nbyte <= 0 && einval) {   /* was || */
		UXFIO_E_DEBUG("FOUND VEOF");
		UXFIO_E_DEBUG3("nbyte = %d einval = %d", nbyte, einval);
		if (uxfio->offset_eofM >= 0) {	/* reset the virtual EOF */
			/* so the next read is not subject to the */
			/* VEOF that is already fulfilled */
			UXFIO_E_DEBUG("reseting eof");
			uxfio->offset_eofM = -(uxfio->offset_eofM);
			uxfio->current_offsetM = 0;
			uxfio->virtual_offsetM = 0;
		}
		if (nbyte <= 0) {
			UXFIO_E_DEBUG2("LEAVING fildes=%d", uxfio_fildes);
			return 0;
		}
	}
	if (uxfio->buffer_activeM == 0) {
		UXFIO_E_DEBUG("buffer_active is zero");
		if (
			(pollingread && (uxfio->uxfd_can_seekM==0 || uxfio__get_state(uxfio)))
		) {
			UXFIO_E_DEBUG2("fildes=%d foo_read is tread.",
						uxfio_fildes);
			foo_read=uxfio->vir_tread_readM;
		} else {
			UXFIO_E_DEBUG2("fildes=%d foo_read is read.",
								uxfio_fildes);
			foo_read=uxfio->vir_readM;
		}
		UXFIO_E_DEBUG3("fildes=%d uxfd=%d ",
						uxfio_fildes, uxfio->uxfdM);
		UXFIO_E_DEBUG3("fildes=%d nbyte=%d ",
						uxfio_fildes, nbyte);
		n=(*foo_read)(uxfio->uxfdM, buf, nbyte);
		if (n < 0) {
			UXFIO_E_DEBUG3("LEAVING fildes=%d ret = %d",
						uxfio_fildes, n);
			fprintf(stderr,
"uxfio: read failed at line %d uxfio_fildes=[%d] uxfd=[%d] return value=[%d]\n",
				__LINE__, uxfio_fildes, uxfio->uxfdM, n);
			if (foo_read == uxfio_unix_safe_read)
				fprintf(stderr,
					"uxfio_common_read: fd=%d  %s\n",
						uxfio->uxfdM, strerror(errno));
			return n;
		}
		uxfio->bytesreadM += (Xint)n; /* bug */
		uxfio->current_offsetM += (Xint)n;
		uxfio->virtual_offsetM += (Xint)n;
		UXFIO_E_DEBUG3("LEAVING fildes=%d ret = %d", uxfio_fildes, n);
		return n;
	}
	UXFIO_E_DEBUG2("posM = %d", (int)(uxfio->posM));
	oldpos = uxfio->posM;

	bufferstate(uxfio, &left, &right, &unused);

				/* pointer in middle of valid data */
	if (uxfio->posM < uxfio->endM) {
				/* bufcount is amount be taken from buffer */
		bufcount = nbyte >= right ? right : nbyte;

				/* copy this ammount */
		if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			n = uxfio_unix_atomic_read(uxfio->buffdM, (void *) (p),
								bufcount);
			if (n != bufcount) {
				UXFIO_E_DEBUG2("error %d", uxfio_fildes);
				fprintf(stderr,
			"uxfio internal error: file buffer read error\n");
				if (n > 0)
					lseek(uxfio->buffdM, -n, SEEK_CUR);
				UXFIO_E_DEBUG2("LEAVING fildes=%d",
								uxfio_fildes);
				return -1;
			}
		} else {
			uxfio__memcpy((void *) p, 0, uxfio->bufM,
						uxfio->posM, bufcount, uxfio);
		}
		uxfio->current_offsetM += bufcount;
		uxfio->virtual_offsetM += bufcount;
		/* if there is more to read, read the real file */
		if (bufcount < nbyte) {
			if (pollingread && (uxfio->uxfd_can_seekM==0 ||
						uxfio__get_state(uxfio))) {
				UXFIO_E_DEBUG2("fildes=%d foo_read is tread.",
								uxfio_fildes);
				foo_read=uxfio->vir_tread_readM;
			} else {
				UXFIO_E_DEBUG2("fildes=%d foo_read is read.",
								uxfio_fildes);
				foo_read=uxfio->vir_readM;
			}
			if (uxfio->uxfdM != UXFIO_NULL_FD)
				n = (*foo_read)(uxfio->uxfdM,
					(void *)(p + bufcount),
						nbyte - bufcount);
			else
				n = 0;
			if (n < 0) {
				fprintf(stderr,	
					"uxfio: read failed line=%d\n",
						__LINE__);
				return n;	
			}
			uxfio->bytesreadM += n;
			uxfio->current_offsetM += n;
			uxfio->virtual_offsetM += n;
			if (fix_buffer(uxfio_fildes, uxfio,
							bufcount + n, p, n)) {
				UXFIO_E_DEBUG2("error %d", uxfio_fildes);
				fprintf(stderr,
					"uxfio internal error: 10004.0 \n");
			}
			return bufcount + n;

			/* the read is satisfied, fixup and return */
		} else {
			if (fix_buffer(uxfio_fildes, uxfio, bufcount, p, 0)) {
				UXFIO_E_DEBUG2("error %d", uxfio_fildes);
				fprintf(stderr,
					"uxfio internal error: 10004.1 \n");
			}
			UXFIO_E_DEBUG2("LEAVING fildes=%d", uxfio_fildes);
			return bufcount;
		}

	} else {		/* pointer at end of data, nothing in buffer
				   to give to user */
		
		if ((pollingread && uxfio->uxfd_can_seekM==0) ||
						uxfio__get_state(uxfio)) {
			UXFIO_E_DEBUG2("fildes=%d foo_read is tread.",
						uxfio_fildes);
			foo_read=uxfio->vir_tread_readM;
		} else {
			UXFIO_E_DEBUG2("fildes=%d foo_read is read.",
								uxfio_fildes);
			foo_read=uxfio->vir_readM;
		}

		if (uxfio->uxfdM != UXFIO_NULL_FD)
			n = (*foo_read)(uxfio->uxfdM, (void *)(p), nbyte);
		else
			n = 0;
		if (n < 0) {
			return n;
		}
		uxfio->bytesreadM += n;
		uxfio->current_offsetM += n;
		uxfio->virtual_offsetM += n;
		if (fix_buffer(uxfio_fildes, uxfio, n, p, n)) {
			UXFIO_E_DEBUG2("error %d", uxfio_fildes);
			fprintf(stderr, "uxfio internal error: 10004.2 \n");
		}
		if (uxfio->auto_disableM) {
			uxfio->auto_disableM = 0;
			if (uxfio_fcntl(uxfio_fildes,
					UXFIO_F_SET_BUFACTIVE, UXFIO_OFF)) {
				UXFIO_E_DEBUG2("error %d", uxfio_fildes);
				fprintf(stderr,
				"uxfio internal error: auto_disable error\n");
			}
		}
		UXFIO_E_DEBUG3("LEAVING fildes=%d ret=%d", uxfio_fildes, n);
		return n;
	}
	UXFIO_E_DEBUG2("error %d", uxfio_fildes);
	UXFIO_E_DEBUG2("LEAVING ret=%d", UXFIO_RET_EFAIL);
	return UXFIO_RET_EFAIL;
}

static
ssize_t
uxfio_common_safe_read(UXFIO *uxfio, int fd, void * buf, int nbyte)
{
	int n=0, nret=1;
	char *p = (char*)(buf);
	while(n < nbyte && nret){
		nret = uxfio_common_read(uxfio, fd, p+n, nbyte-n, 0); 
		if (nret < 0) return nret;
		n+=nret;	
	}
	return n;
}

static 
Xint
enforce_veof(UXFIO * uxfio, Xint nbyte, int * einval)
{
	Xint remaining = 0;

	if (einval) *einval=0;
	if (virtual_eof_active(uxfio)) {
		UXFIO_E_DEBUG("virtual eof active");

		/* There are, I think, two schemes to make a file within
		   a file.  Scheme One uses offset_eofM and virtual_offsetM
		   and scheme Two uses uxfio->v_endM and uxfio->current_offsetM

		   Scheme Two was added circa 2007-07 with only partial support
		   existing before then, scheme One is the original.
		   The joke is they must be separate.  */

		if (uxfio->offset_eofM >= 0) {
			remaining = uxfio->offset_eofM - uxfio->virtual_offsetM;
		} else if (uxfio->v_endM >= 0) {
			remaining = uxfio->v_endM - uxfio->current_offsetM;
		}
	}
	else {
		uxfio->offset_eof_savedM = -1;
		return nbyte;
	}

	if (remaining <= nbyte) {
		UXFIO_E_DEBUG("setting einval");
		*einval=1;
		UXFIO_E_DEBUG2("enforcing veof %d\n", (int)remaining);
		return remaining;
	} else {
		UXFIO_E_DEBUG2("no end found. returning %d", (int)nbyte);
		return nbyte; 
	}
}

static 
Xint
enforce_set_position(UXFIO * uxfio, Xint offset, int *einval)
{
	Xint passed;
	
	passed = uxfio->bytesreadM;
	*einval=0;
	
	UXFIO_E_DEBUG2("offset = %d", (int)offset);
	UXFIO_E_DEBUG2("passed = %d", (int)passed);
	/* UXFIO_E_DEBUG2("%s", uxfio_dump_string_from_object(uxfio)); */
	
	if (offset >= 0){
		UXFIO_E_DEBUG2("returning offset = %d", (int)offset);
		return offset;
	}

	if (virtual_eof_active(uxfio)) {
		if (offset < 0) {
			UXFIO_E_DEBUG("");
			if (!passed) {
				if (uxfio->offset_eofM >= 0) {
					UXFIO_E_DEBUG("");
					passed=uxfio->offset_eofM;	
				} else if (uxfio->v_endM >= 0) {
					UXFIO_E_DEBUG("");
					passed=uxfio->v_endM;	
				} else {
					passed=0;
					UXFIO_E_DEBUG("error");
					fprintf(stderr,
		"uxfio warning: internal exception enforce_set_position()\n");
				}
			}
			if ( -offset > passed ) {
				/* invalid */
				UXFIO_E_DEBUG("");
				*einval=1;
				UXFIO_E_DEBUG("returning 0");
				return 0;
			} 
		}
	} 
	UXFIO_E_DEBUG2("returning %d", (int)offset);
	return offset;
}

static 
int
uxfio__delete_buffer_file(UXFIO * uxfio)
{

	if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
		if (uxfio->buffdM < 0 || !(*(uxfio->buffilenameM))) {
			return -1;
		}
		close(uxfio->buffdM);
		uxfio__unlink_tmpfile(uxfio);
		uxfio->buffdM = -1;
		(*(uxfio->buffilenameM)) = '\0';
		return 0;
	} else {
		return 0;
	}
}

static 
int
uxfio__init_buffer_file(UXFIO * uxfio, void *buf, int len)
{
	int fd, ret;
	char * dirtmp;
	char * rootdir;
	static char default_rootdir[] = UXFIO_TMPFILE_ROOTDIR;

	UXFIO_E_DEBUG("ENTERING");
		if (uxfio->buffdM > 0 && (*(uxfio->buffilenameM))) {
			/*
			* already buftype file.
			*/
			UXFIO_E_DEBUG("already have tmpfile");
			return 0;
		}
		if ((uxfio->buffdM < 0 && (*(uxfio->buffilenameM))) ||
		    (uxfio->buffdM > 0 && !(*(uxfio->buffilenameM)))
		    ) {
			UXFIO_E_FAIL("invalid condition");
			return -1;
		}

		if (uxfio->tmpfile_rootdirM) {
			rootdir = uxfio->tmpfile_rootdirM;
		} else {
			rootdir = default_rootdir;
		}
		dirtmp = malloc(strlen(rootdir) + strlen(UXFIO_TMPFILE_PFX) + 7);
		if (!dirtmp) {
			fprintf(stderr,
				"uxfio: line %d: out of memory.\n", __LINE__);
			exit(22);
		}
		strcpy(dirtmp, rootdir);
		strcat(dirtmp, UXFIO_TMPFILE_PFX);
		strcat(dirtmp, "XXXXXX");

		if (strlen(dirtmp) - 1 > sizeof(uxfio->buffilenameM)) {
			swbis_free(dirtmp);
			uxfio->buffdM = -1;
			fprintf(stderr, "temp file name too long.\n");
			return -1;
		} else {
			strncpy(uxfio->buffilenameM, dirtmp,
						sizeof(uxfio->buffilenameM));
			uxfio->buffilenameM[sizeof(uxfio->buffilenameM) - 1] =
									'\0';
			swbis_free(dirtmp);
		}

		fd = mkstemp(uxfio->buffilenameM);
		SWBISERROR3("Informative Message:",
		"%p Making tmpfile: [%s]", (void*)uxfio, uxfio->buffilenameM);
		if (fd < 0) {
			UXFIO_E_DEBUG("error");
			fprintf(stderr,
				"mkstemp failed: %s\n", strerror(errno));
			exit(errno);
		}
		uxfio->buffdM = fd;
		uxfio->lenM = UXFIO_FLEN;
		uxfio->startM = uxfio->endM = uxfio->posM = 0;
	
		if (buf && (len > 0)) {
			UXFIO_E_DEBUG3("fd=%d len=%d", (int)(uxfio->buffdM), (int)len);
			if ((ret=uxfio_unix_atomic_write(uxfio->buffdM, buf, len))
								!= len) {
				close(uxfio->buffdM);
				if (uxfio__unlink_tmpfile(uxfio)) {
					UXFIO_E_DEBUG("error");
					fprintf(stderr,
				"uxfio error: error unlinking file %s\n",
							uxfio->buffilenameM);
				}
				UXFIO_E_FAIL2("fatal, write failed on [%s]",
							uxfio->buffilenameM);
				exit(errno);
			} else {
				uxfio->startM = 0;
				uxfio->endM = len;
/*+*/				uxfio->virtual_offsetM = 0;
				uxfio->posM = 0;
				if (lseek(uxfio->buffdM, uxfio->posM, SEEK_SET)
									< 0) {
					UXFIO_E_FAIL2("%s",
							strerror(errno));
				}
				UXFIO_E_DEBUG2("uxfio->endM=%d", (int)(uxfio->endM));
			}
		}
		return 0;
}

static 
void *
uxfio__memcpy(void *dest, Xint dest_offset, void *src, Xint src_offset,
	      Xint amount, UXFIO * uxfio)
{
	if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
		return do_file_buffer_move(dest, dest_offset, src, src_offset,
					   amount, uxfio);
	} else {
		return memcpy((void*)((char*)dest + dest_offset),
				(void*)((char*)src + src_offset), (size_t)amount);
	}
}

static 
void *
uxfio__memmove(void *dest, Xint dest_offset, void *src, Xint src_offset,
	       Xint amount, UXFIO * uxfio)
{
	if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
		return do_file_buffer_move(dest, dest_offset, src, src_offset,
					   amount, uxfio);
	} else {
		return memmove((void*)((char*)dest + dest_offset),
				(void*)((char*)src + src_offset), (size_t)amount);
	}
}

static 
void *
do_file_buffer_move(void *dest, Xint dest_offset,
			void *src, Xint src_offset,
		    			Xint amount, UXFIO * uxfio)
{
	/*
	 * Not yet implemented.
	 */
	return (void *) (NULL);
}

static
void
copy_stat_members (struct stat * buf, struct stat * statbuf)
{
	buf->st_mode=statbuf->st_mode;   
	buf->st_ino=statbuf->st_ino;   
	buf->st_dev=statbuf->st_dev;  
	buf->st_rdev=statbuf->st_rdev; 
	buf->st_nlink=statbuf->st_nlink; 
	buf->st_uid=statbuf->st_uid;  
	buf->st_gid=statbuf->st_gid; 
	buf->st_size=statbuf->st_size;  
	buf->st_atime=statbuf->st_atime; 
	buf->st_mtime=statbuf->st_mtime; 
	buf->st_ctime=statbuf->st_ctime; 
	buf->st_blksize=statbuf->st_blksize; 
	buf->st_blocks=statbuf->st_blocks; 
}

static 
int
uxfio__unlink_tmpfile(UXFIO *uxfio)
{
	int ret=0;
	SWBISERROR3("Informative Message:",
		"%p unlink tmpfile: [%s]", (void*)uxfio, uxfio->buffilenameM);
	if (unlink(uxfio->buffilenameM)) {
		ret=-1;
		if (errno != ENOENT) {
			UXFIO_E_DEBUG("error");
			fprintf(stderr,
				"uxfio error: error unlinking file %s: %s\n",
					uxfio->buffilenameM, strerror(errno));
		}
	}
	return ret;
}

static
int 
uxfio_internal_ftruncate(int uxfio_fildes, off_t nbyte)
{
	int ret;
	UXFIO *uxfio;
	intmax_t amount;
	off_t pos;

	if (table_find(uxfio_fildes, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr,
			"uxfio error [%d]:  file desc %d not found.\n", __LINE__,
					uxfio_fildes);
		return -1;
	}
	if (uxfio->statbufM && uxfio->v_endM > 0) {
		/*
		 * Its a compound file, therefore don't really truncate it.
		 */
		if (nbyte > uxfio->v_endM) {
			uxfio->v_endM = (Xint)nbyte;
			uxfio->statbufM->st_size = nbyte;
			if (uxfio->offset_eofM > nbyte) {
				uxfio->offset_eofM = (int)nbyte;
			}
			if (uxfio->bytesreadM > nbyte) {
				uxfio->bytesreadM = (int)nbyte;
				uxfio->current_offsetM = (int)nbyte;
				uxfio->virtual_offsetM = (int)nbyte;
			}
		}
		return 0;
	} else if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			/* don't cuurently support partial truncation */
		if (uxfio->posM >= uxfio->endM) {
			return ftruncate(uxfio->buffdM, (off_t) (0));
		} else {
			return -2;
		}
	} else if (uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM) {
		Xint left, right, unused;
		bufferstate(uxfio, &left, &right, &unused);
		pos = uxfio_lseek(uxfio_fildes, 0, SEEK_CUR);
		if (pos < 0)
			return -1;
		if (nbyte > uxfio->endM) {
			char buf[512];

			/* extend the file and fill with zeros */
			if (pos < 0)
				return -1;
			if (uxfio_lseek(uxfio_fildes, 0, SEEK_END) < 0)
				return -1;

			memset(buf, '\0', sizeof(buf));
			amount = (intmax_t)(nbyte - uxfio->endM);
			ret = zero_fill(uxfio_fildes, (off_t)amount);
			if (uxfio_lseek(uxfio_fildes, pos, SEEK_SET) < 0)
				return -1;
		} else {
			/* truncate it */
			amount = (intmax_t)(uxfio->endM - nbyte);
			if (uxfio_lseek(uxfio_fildes, (off_t)nbyte, SEEK_SET) < 0)
				return -1;
			ret = zero_fill(uxfio_fildes, (off_t)amount);
			if (uxfio_lseek(uxfio_fildes, pos, SEEK_SET) < 0)
				return -1;
			if (nbyte > uxfio->posM) {
				uxfio_lseek(uxfio_fildes, (off_t)nbyte, SEEK_SET);
			}
			uxfio->endM = nbyte;
		}
		return ret;
	}
	/* No other cases supported, yet */
	return -1;
}

static
int
do_buffered_write2(UXFIO * uxfio, void * userbuf, size_t nbyte)
{
	int write_ret;
	char * buf;
	int bs;
	size_t initial_remains;
	char * userp;
	char * bufp;
	int remains;
	int user_remains;
	int udid;
	char * sub_p;
	int sub_remains;
	int sub_write_ret;
	
	buf = uxfio->output_bufferM;
	bs = uxfio->output_block_sizeM;
	initial_remains = (size_t)(uxfio->output_buffer_cM);

	userp = userbuf;
	bufp = buf + initial_remains;
	remains = initial_remains + (int)nbyte;
	user_remains = (int)nbyte;

	while(remains >= bs) {
		udid = bs - initial_remains;
		memmove(bufp, userp, udid);

		sub_p = buf;
		sub_remains = bs;
		write_ret = 0;
		while (sub_remains > 0) {
			sub_write_ret = uxfio_write(uxfio->uxfdM, (void*)sub_p, sub_remains);
			if (sub_write_ret < 0) {
				/*  FIXME check for EINTR and EAGAIN */
				write_ret = -1;
				break;
			} else if (sub_write_ret == 0) {
				write_ret = -1;
				break;
			} else {
				write_ret += sub_write_ret;
				sub_remains -= sub_write_ret;
				sub_p += sub_write_ret;
			}	
		}

		if (write_ret < 0) {
			return -1;
		} else if (write_ret != bs) {
			uxfio->output_buffer_cM = bs;
			return -1;
		}
		remains  -= write_ret;
		user_remains  -= udid;
		userp += udid;
		bufp = buf;
		initial_remains = 0;
	}
	memcpy(bufp, userp, user_remains);
	uxfio->output_buffer_cM = remains;
	return  (int)nbyte;
}

int
do_buffered_flush(UXFIO * uxfio)
{
	int remains;
	int write_ret;
	int save_bs;

	if (uxfio->output_block_sizeM == 0 || uxfio->output_buffer_cM == 0)
		return 0;

	save_bs = uxfio->output_block_sizeM;

	remains = uxfio->output_buffer_cM;
	uxfio->output_buffer_cM = 0;
	uxfio->output_block_sizeM = remains;

	if (remains > save_bs) {
		fprintf(stderr, "%s: internal flush error: do_buffered_flush2: line=%d\n", __FILE__, __LINE__);
		return -1;
	}

	write_ret = do_buffered_write2(uxfio, (void*)(uxfio->output_bufferM), (size_t)remains);

	if (write_ret != (int)remains) {
		uxfio->output_block_sizeM = save_bs;
		fprintf(stderr, "%s: flush error: do_buffered_flush2: line=%d\n", __FILE__, __LINE__);
		return -1;
	}
	if (uxfio->output_buffer_cM > 0) {
		uxfio->output_block_sizeM = save_bs;
		fprintf(stderr, "%s: flush error: do_buffered_flush2: line=%d\n", __FILE__, __LINE__);
		return -1;
	}
	uxfio->output_block_sizeM = save_bs;
	return 0;
}

/* ------------------------------------------------------  */
/* ------------- Public Function Definitions ------------  */


void
uxfio_show_all_open_fd(FILE * f)
{
	UXFIO * uxfio;
	int i;
	int n = 0;
	for (i=UXFIO_FD_MIN; i<(UXFIO_MAX_OPEN + UXFIO_FD_MIN); i++) {
		if (table_find(i, &uxfio, 0)) {
			/* error, fd not found */
			;
		} else {
			fprintf(f, "%d %d\n", n++, i);
		}
	}
}

int
uxfio_uxfio_get_nopen(void)
{
	UXFIO * uxfio;
	int n;
	int i;
	i = -1;
	n = table_mon(UXFIO_QUERY_NOPEN, &i, &uxfio);
	return n;
}

ssize_t
uxfio_unix_safe_read(int fd, void * buf, size_t nbyte) {
	if (fd < UXFIO_FD_MIN && fd >= 0) {
		return safeio((ssize_t (*)(int, void *, size_t))read,
					fd, buf, nbyte);
	} else {
		return safeio((ssize_t (*)(int, void *, size_t))uxfio_read,
					fd, buf, nbyte);
	}
}

ssize_t
uxfio_unix_safe_write(int fd, void * buf, int nbyte) {
	return safeio((ssize_t (*)(int, void *, size_t))write,
					fd, buf, nbyte);
}

ssize_t
uxfio_unix_atomic_read(int fd, void * buf, size_t nbyte)
{
	return atomicio((ssize_t (*)(int, void *, size_t))read,
					fd, buf, nbyte);
}

ssize_t
uxfio_unix_atomic_write(int fd, void * buf, size_t nbyte)
{
	if (fd < UXFIO_FD_MIN && fd >= 0)
		return atomicio((ssize_t (*)(int, void *, size_t))write,
					fd, buf, nbyte);
	else
		return atomicio((ssize_t (*)(int, void *, size_t))uxfio_write,
					fd, buf, nbyte);
}

ssize_t
uxfio_unix_safe_atomic_read(int fd, void * buf, size_t nbyte)
{
	return atomicio((ssize_t (*)(int, void *, size_t))read,
					fd, buf, nbyte);
	/*
	int n=0, nret=1;
	char *p = (char*)(buf);
	UXFIO_E_DEBUG("ENTERING");
	UXFIO_E_DEBUG2("nbyte is %d", nbyte);
	while(n < (int)nbyte && nret){
		nret = read(fd, p+n, nbyte-n); 
		if (nret < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			} else {
				return -1;
			}
		}
		n+=nret;	
	}
	return (ssize_t)n;
	*/
}

UXFIO *
uxfio_debug_get_object_address(int uxfiofd)
{
	UXFIO *uxfio;
	UXFIO_E_DEBUG("");
	if (table_find(uxfiofd, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]:  file desc %d not found.\n", __LINE__,
				uxfiofd);
		return (UXFIO*)NULL;
	}
	return uxfio;
}

void *
uxfio_get_object_address(int uxfiofd)
{
	return uxfio_debug_get_object_address(uxfiofd);
}

int 
uxfio_devnull_close(int fd)
{
	return 0;
}

int 
uxfio_devnull_open(char *path, int oflag, mode_t mode)
{
	if (uxg_nullfd == 0)
		uxg_nullfd = open ("/dev/null", O_RDWR, 0);
	if (uxg_nullfd < 0) {
		fprintf (stderr, "fatal error: can't open /dev/null: %s\n", strerror(errno));
		exit(1);
	}
	return uxg_nullfd;
}

int 
uxfio_open(char *path, int oflag, mode_t mode)
{
	int uxfio_fd, u_fd;
	UXFIO *uxfio;

	UXFIO_E_DEBUG("");
	uxfio_fd = internal_uxfio_open(&uxfio);
	if (uxfio_fd < 0)
		return -1;
	if (path) {
		if (strlen(path)) {
			if ((u_fd = open(path, oflag, mode)) < 0) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
					"open ( %s ) : %s\n",
						path, strerror(errno));
				return -1;
			}
		} else {
			/*
			*  /dev/null simulator
			*/
			u_fd = UXFIO_NULL_FD;
		}
		uxfio__change_state(uxfio, 0);	
		uxfio->uxfd_can_seekM = 1;
	} else {
		u_fd = -1;
		uxfio->buffer_activeM = 0;
		uxfio->uxfd_can_seekM = 0;
	}
	uxfio->uxfdM = u_fd;
	return uxfio_fd;
}

int 
uxfio_opendup(int uxfd, int buffertype)
{
	UXFIO *uxfio;
	int uxfio_fd, r;
	int bufon=UXFIO_ON;

	UXFIO_E_DEBUG3("uxfd=%d  buffertype=%d", uxfd, buffertype);
	uxfio_fd = uxfio_open((char *) (NULL), O_RDONLY, 0);
	if (uxfio_fd < 0)
		return uxfio_fd;

	if ((r = uxfio_fcntl(uxfio_fd, UXFIO_F_ATTACH_FD, uxfd)) < 0)
		return -1;
	
	if (table_find(uxfio_fd, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]: file desc %d not found.\n", __LINE__,
			uxfio_fd);
		return -1;
	}

	uxfio->did_dupe_fdM = uxfd;

	if (uxfd >= UXFIO_FD_MIN) {
		UXFIO * uxfd_uxfio = NULL;
		
		uxfd_uxfio = uxfio_get_object_address(uxfd);
		uxfio_incr_use_count(uxfd);
		if (
			uxfio_lseek(uxfd, 0L, SEEK_CUR) == 0 ||
			(
			uxfd_uxfio && (
				uxfd_uxfio->uxfd_can_seekM ||
				(	uxfd_uxfio->buffer_activeM &&
					(
					uxfd_uxfio->buffertypeM == UXFIO_BUFTYPE_FILE ||
					uxfd_uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM ||
					0
					)
				)
				)
			)
		) 
		{
			UXFIO_E_DEBUG("uxfio->uxfd_can_seekM setting to 1");
			uxfio->uxfd_can_seekM = 1;
		} else {
			UXFIO_E_DEBUG("uxfio->uxfd_can_seekM setting to 0");
			/* UXFIO_E_DEBUG2("UXFD___\n%s\nUXFD^^^\n", uxfio_dump_string(uxfd)); */
			uxfio->uxfd_can_seekM = 0;
		}
	} else {
		if ((lseek(uxfd, 0L, SEEK_CUR) == -1) && (errno == ESPIPE)){
			uxfio->uxfd_can_seekM = 0;
		} else {
			uxfio->uxfd_can_seekM = 1;
		}
	}
		

	if (buffertype == UXFIO_BUFTYPE_FILE) {
		if ((r = uxfio_fcntl(uxfio_fd, UXFIO_F_SET_BUFTYPE,
					UXFIO_BUFTYPE_FILE)) < 0) {
			uxfio_close(uxfio_fd);
			return -1;
		}
	} else if (buffertype == UXFIO_BUFTYPE_MEM) {
		if ((r = uxfio_fcntl(uxfio_fd, UXFIO_F_SET_BUFTYPE,
						UXFIO_BUFTYPE_MEM)) < 0) {
			uxfio_close(uxfio_fd);
			return -1;
		}
	} else if (buffertype == UXFIO_BUFTYPE_DYNAMIC_MEM) {
		if ((r = uxfio_fcntl(uxfio_fd, UXFIO_F_SET_BUFTYPE,
					UXFIO_BUFTYPE_DYNAMIC_MEM)) < 0) {
			uxfio_close(uxfio_fd);
			return -1;
		}
	} else {
		bufon=UXFIO_OFF;	
	}
	if ((r = uxfio_fcntl(uxfio_fd, UXFIO_F_SET_BUFACTIVE, bufon)) < 0) {
		uxfio_close(uxfio_fd);
		return -1;
	}
	return uxfio_fd;
}

o__inline__
int 
uxfio_close(int uxfio_fildes)
{
	int ret;
	ret = uxfio__close(uxfio_fildes, 1);
	if (ret < 0)
		return -1;
	else
		return 0;
}

int 
uxfio_free(int uxfio_fildes)
{
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0){
		return 0;
	}	
	if (uxfio_fcntl(uxfio_fildes, UXFIO_F_SET_BUFACTIVE, 0) < 0) {
		fprintf(stderr, "error in uxfio_free\n");
		return -1;
	}
	return uxfio__close(uxfio_fildes, 0);
}

int
uxfio_decr_use_count(int fd)
{
	UXFIO *uxfio;
	UXFIO_E_DEBUG2("uxfio_filedes=%d", fd);
	if (fd < UXFIO_FD_MIN ){
		return -1;
	}	
	if (table_find(fd, &uxfio, 1)) {
		return -1;
	}
	return --(uxfio->use_countM);
}

int
uxfio_incr_use_count(int fd)
{
	UXFIO *uxfio;
	UXFIO_E_DEBUG2("uxfio_filedes=%d", fd);
	if (table_find(fd, &uxfio, 1)) {
		return -1;
	}
	return ++(uxfio->use_countM);
}

int 
uxfio_espipe(int uxfio_fildes)
{
	int ret;
	UXFIO *uxfio;

	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN) {
		if (lseek(uxfio_fildes, 0L, SEEK_CUR) == -1){
			return (errno == 0) ? -1 : errno;
		} else {
			return 0;
		}
	}
	
	if (table_find(uxfio_fildes, &uxfio, 1)) {
		return -1;
	}
	ret = uxfio->uxfd_can_seekM ? 0 : ESPIPE;

	if (ret == 0) {
		ret = uxfio_fcntl(uxfio_fildes, UXFIO_F_GET_CANSEEK,  0);
		if (ret)
			ret = 0;
		else
			ret = ESPIPE;
	}

	UXFIO_E_DEBUG2("return value = %d", ret);
	return ret;
}

int 
uxfio_getfd(int uxfio_fildes, int * use_next)
{
	UXFIO *uxfio;

	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN) {
		return uxfio_fildes;
	}
	if (table_find(uxfio_fildes, &uxfio, 1)) {
		return -1;
	}
	if (use_next) {
		if (uxfio->buffer_activeM && (uxfio->posM < uxfio->endM)) {
			*use_next=0;
		} else {
			*use_next=1;
		}
	}
	return uxfio->uxfdM;
}

int 
uxfio_ftruncate(int uxfio_fildes, off_t nbyte)
{
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0) {
		return ftruncate(uxfio_fildes, nbyte);
	} else if (uxfio_fildes >= UXFIO_FD_MIN ) {
		return uxfio_internal_ftruncate(uxfio_fildes, nbyte);
	} else {
		return -1;
	}
}

ssize_t 
uxfio_read(int uxfio_fildes, void *buf, size_t nbyte)
{
	int ret;
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0) {
		ret = uxfio_unix_safe_read(uxfio_fildes, buf, nbyte);
		if (ret < 0)
			fprintf(stderr,
				"uxfio_read fd=%d : %s\n",
					uxfio_fildes, strerror(errno));
		return ret;
	} else if (uxfio_fildes >= UXFIO_FD_MIN ) {
		return uxfio_common_read(NULL, uxfio_fildes, buf, nbyte, 0);
	} else {
		return -1;
	}
}

ssize_t 
uxfio_sfa_read(int uxfio_fildes, void *buf, size_t nbyte)
{
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN)
		return uxfio_unix_atomic_read(uxfio_fildes, buf, nbyte); 
	else
		return uxfio_common_safe_read(NULL, uxfio_fildes, buf, nbyte);
}

ssize_t 
uxfio_sfread(int uxfio_fildes, void *buf, size_t nbyte)
{
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN)
		return uxfio_unix_safe_read(uxfio_fildes, buf, nbyte); 
	else
		return uxfio_common_safe_read(NULL, uxfio_fildes, buf, nbyte);
}

ssize_t 
uxfio_write(int uxfio_fildes, void *buf, size_t nbyte)
{
	UXFIO *uxfio;
	int n;
	int einval;
	Xint left, right, unused;
	Xint endoffset;

	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes == UXFIO_NULL_FD) {
		return (ssize_t) nbyte;
	} else if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0) {
		return (size_t) uxfio_unix_safe_write(uxfio_fildes, buf, nbyte);
	} else {
		if (table_find(uxfio_fildes, &uxfio, 1)) {
			UXFIO_E_DEBUG("error");
			fprintf(stderr,
				"uxfio error [%d]:  file desc %d not found.\n", __LINE__,
					uxfio_fildes);
			return -1;
		}
		if (virtual_eof_active(uxfio)) {
			endoffset = uxfio->bytesreadM + nbyte;
			if (enforce_veof(uxfio, endoffset, &einval) !=
								endoffset) {
				return -1;
			}
		}
		if (uxfio->write_insertM && uxfio->buffertypeM !=
						UXFIO_BUFTYPE_DYNAMIC_MEM) {
			UXFIO_E_DEBUG("error");
			fprintf(stderr,
				"uxfio warning: file insert only supported for"
				" UXFIO_BUFTYPE_DYNAMIC_MEM buffer.\n");
		}

		if (uxfio->buffer_activeM == 0 &&
				uxfio->output_block_sizeM == 0) {
			return (size_t)(*(uxfio->vir_writeM))
						(uxfio->uxfdM, buf, nbyte);
		} else if (uxfio->output_block_sizeM > 0 &&
					uxfio->buffer_activeM == 0) {
			n = do_buffered_write2(uxfio, buf, nbyte);
			return n;
		} else if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE &&
						uxfio->uxfd_can_seekM == 1) {
			nbyte = 
		((int)(uxfio->posM + nbyte) > (int)(uxfio->endM)) ?
				(size_t)(uxfio->endM - uxfio->posM) : nbyte;
			n = uxfio_unix_safe_write(uxfio->buffdM, buf, nbyte);
			if (n < 0) {
				perror("exiting in uxfio_write");
				exit(errno);
			} else {
				(uxfio->posM) += (Xint)n;
				(uxfio->bytesreadM) += (Xint)n;
				(uxfio->current_offsetM) += (Xint)n;
				(uxfio->virtual_offsetM) += (Xint)n;
				return n;
			}
		} else if (uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM
					/* && uxfio->can_seek == 1 */ ) {

			/* when writing into a file with dynamic 
			   (unspecified length) buffer bytes not yet read 
			   will not be overwritten, writes past the
			   buffered length will extend the buffer therby
			   be inserted into the file. */

			bufferstate(uxfio, &left, &right, &unused);
			if ((int)nbyte > right + unused) {
				if (fix_dynamic_buffer(uxfio_fildes,
						uxfio, nbyte, buf,
						nbyte - right - unused)) {
					UXFIO_E_DEBUG("error");
					fprintf(stderr,
	"uxfio internal error: error returned by fix_dynamic_buffer.\n");
					return -1;
				}
				return nbyte;
			} else {
				if (uxfio->write_insertM) {
					memmove(
					uxfio->bufM + uxfio->posM + nbyte,
						uxfio->bufM + uxfio->posM,
								right);
					if (uxfio->posM + (int)nbyte + right >
								uxfio->endM)
						uxfio->endM = uxfio->posM +
									nbyte;
				}
				memcpy(uxfio->bufM + uxfio->posM, buf, nbyte);
				uxfio->posM += nbyte;
				(uxfio->bytesreadM) += nbyte;
				(uxfio->current_offsetM) += nbyte;
				(uxfio->virtual_offsetM) += nbyte;
				if (uxfio->posM > uxfio->endM) {
					uxfio->endM = uxfio->posM;
				}
				return nbyte;
			}
		} else {
			return -1;
		}
	}
	return -1;
}

off_t 
uxfio_lseek(int uxfio_fildes, off_t poffset, int pwhence)
{
	UXFIO *uxfio;
	int n;
	Xint left, right, unused;
	int einval;
	off_t retval = -1;
	off_t offset = 0;
	int do_virtual_set = 0;
	int whence;

	/*
	int ts1 = 0;
	int ts2 = 0;
	*/

	UXFIO_E_DEBUG3("ENTER FUNCTION  fd=%d offset=%d", uxfio_fildes,
						(int)poffset);
	UXFIO_E_DEBUG3("%d pwhence=%d", uxfio_fildes, pwhence);
	/* UXFIO_E_DEBUG2("\n%s\n.", uxfio_dump_string(uxfio_fildes)); */
	whence = pwhence;
	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0) {
		if (whence == UXFIO_SEEK_VCUR) {
			whence = SEEK_CUR;
		}
		return lseek(uxfio_fildes, poffset, whence);
	}
	if (table_find(uxfio_fildes, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]: file desc %d not found.\n", __LINE__,
			uxfio_fildes);
		return -1;
	}
	
	UXFIO_E_DEBUG3("%d posM=%d", uxfio_fildes, (int)(uxfio->posM));
	if (whence == SEEK_CUR || whence == UXFIO_SEEK_VCUR) {
		offset=(off_t)(poffset);
	} else if (whence == SEEK_SET || whence == UXFIO_SEEK_VSET) {
		UXFIO_E_DEBUG("in SEEK_SET");
		UXFIO_E_DEBUG3("poffset = %d  posM = %d",
					(int)poffset, (int)(uxfio->posM));
		if (
			virtual_eof_active(uxfio) == 0 &&
			uxfio->offset_eof_savedM >= 0 &&
			pwhence == UXFIO_SEEK_VSET &&
			1
		) {
			/* Now allow the VSET if the position is exactly at the end
			   of the virtual file end */
			if (
				uxfio->posM - uxfio->offset_eof_savedM == uxfio->offset_bofM &&
				uxfio->offset_eof_savedM >= 0 &&
				1
			) {
				uxfio->posM = uxfio->offset_bofM;
				uxfio->virtual_offsetM = 0;
				uxfio->offset_eofM = uxfio->offset_eof_savedM;
			} else {
				fprintf(stderr, "uxfio:  unsupported case: %d\n", __LINE__);
				return -1;
			}
		}
	
		if (!virtual_eof_active(uxfio) /* FIXME ??? why !() */ &&
			uxfio->offset_eof_savedM > 0 &&
				pwhence == UXFIO_SEEK_VSET) {
			UXFIO_E_DEBUG("in SEEK_SET VSAVED virtual eof.");
/*+*/			whence=SEEK_CUR;
			offset=(off_t)(poffset);
			/* 
			allow an immediate reset of the file
			indicated by offset_eof_savedM > 0
			*/
			offset -=  uxfio->offset_eof_savedM;
			uxfio->offset_eofM = uxfio->offset_eof_savedM;
			do_virtual_set = 1;

			UXFIO_E_DEBUG2("now offset = %d", (int)offset);

		} else if (!uxfio->buffer_activeM && uxfio->uxfd_can_seekM) {
			UXFIO_E_DEBUG("in SEEK_SET virtual eof active.");
			UXFIO_E_DEBUG3("offset = %d bytesread = %d",
					(int)offset, (int)(uxfio->bytesreadM));
				
/*+*/			whence=SEEK_CUR;
			offset = poffset - uxfio->bytesreadM;
		
			UXFIO_E_DEBUG2("now offset = %d", (int)offset);
			offset = enforce_set_position(uxfio, offset, &einval);
			UXFIO_E_DEBUG2("in SEEK_SET now offset = %d",
								(int)offset);
			if (einval) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
					"uxfio internal exception 100a.\n");
				return -1;
			}
		} else if (virtual_eof_active(uxfio)) {
/*+*/			/* whence=SEEK_CUR; */
			if (pwhence == UXFIO_SEEK_VSET) {
				if (poffset < 0) return -1;
				if (
					poffset > uxfio->offset_eofM &&
					poffset > uxfio->v_endM
				) return -1;
				offset = uxfio->offset_bofM + poffset;
			} else {
				fprintf(stderr, "uxfio:  unsupported case: %d\n", __LINE__);
				return -1;
			}
		} else {
			/* WAS offset=(off_t)(poffset); */
/*+*/			whence=SEEK_CUR;
			offset=(off_t)(poffset - uxfio->posM);
		}
	} else if (whence == SEEK_END) {
		UXFIO_E_DEBUG("in SEEK_END");
		offset=(off_t)(uxfio->endM - uxfio->posM + poffset);
		whence=SEEK_CUR;
		if (virtual_eof_active(uxfio)) {
			int r_i;
			int r_offset;
			UXFIO_E_DEBUG("in SEEK_END virtual eof active.");
			r_i = (uxfio->offset_eofM >=0) ?
					uxfio->offset_eofM : uxfio->v_endM;
			
			offset=(off_t)(r_i - uxfio->bytesreadM + offset);
			r_offset=offset;
			offset = enforce_veof(uxfio, r_offset, &einval);
			if (offset < 0 || (einval && (r_offset - offset))) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
			"uxfio internal exception: negative offset 100b.\n");
				return -1;	
			}
		}
		else if (!uxfio->buffer_activeM && uxfio->uxfd_can_seekM) {
			UXFIO_E_DEBUG("in SEEK_END error");
			fprintf(stderr,
			"uxfio internal exception: broken case: 100c.\n");
		}
	}
	UXFIO_E_DEBUG2("offset is now %d", (int)offset);

	/*
	* no active buffer but can seek  -OR- dynamicmem is in use.
	*/
	if (
		(
			uxfio->buffer_activeM == 0 &&
			/* uxfio->v_endM < 0 && */
			uxfio->offset_eof_savedM < 0 &&
			uxfio->uxfd_can_seekM &&
			1
		) ||
		(0)
	) {
		UXFIO_E_DEBUG("no active buffer and uxfd_can_seekM is true.");
		uxfio->bytesreadM += offset;
		uxfio->current_offsetM += offset;
		uxfio->virtual_offsetM += offset;
		whence = SEEK_CUR;
		/* 
		* move the under lying file.
		*/
		UXFIO_E_DEBUG3("uxfio_lseek of uxfdM=%d whence=%d",
				uxfio->uxfdM, whence);
		UXFIO_E_DEBUG2("         offset = %d", (int)offset);
		retval = (*(uxfio->vir_lseekM))(uxfio->uxfdM, offset, whence);
		UXFIO_E_DEBUG2("vir_lseekM returned %d", (int)retval);
		UXFIO_E_DEBUG2("uxfio_lseek returning %d", (int)retval);
		return retval;
	} else if (
		(0 && 
			!uxfio->uxfd_can_seekM &&
			(
			uxfio->buffer_activeM &&
			uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM
			)
		) || (0)
	) {
		/* Dead Code    Disabled disabled */

		UXFIO_E_DEBUG2("dynamic mem is in use. offset = %d",
							(int)offset);
		/*OK uxfio->bytesreadM += offset; */

		if (offset > uxfio->posM) {
			int xxn;
			UXFIO_E_DEBUG2("reading ahead %d bytes",
						(int)(offset - uxfio->posM));
			xxn = uxfio__read_bytes(uxfio_fildes,
						offset - uxfio->posM);
			/* FIXME check return value */
			/*
			* BUG
			* uxfio->posM += n;
			* uxfio->current_offsetM += n;
			*/
		} else {
			uxfio->current_offsetM += offset;
			uxfio->virtual_offsetM += offset;
			uxfio->posM += offset;
		}
		retval = uxfio->current_offsetM;
		UXFIO_E_DEBUG2("uxfio_lseek returning %d", (int)retval);
		return retval;
	} else {
		;
		UXFIO_E_DEBUG("NOT IN SHORT SPECIAL.");
		/*
		UXFIO_E_DEBUG2("NOT IN SHORT SPECIAL\n%s\n.",
				uxfio_dump_string(uxfio_fildes));
		*/
	}


	bufferstate(uxfio, &left, &right, &unused);

	/*
	* The command is either SEEK_CUR or UXFIO_SEEK_VCUR
	* UXFIO_SEEK_VCUR support getting the current virtual position
	* and only applies if the offset is zero..
	*/

	if (offset) {
		/*
		* The only legitimate use of UXFIO_SEEK_VCUR is if offset is 
		* zero (0) and the buffer type does track the offset in posM
		*/
		whence = SEEK_CUR;
	}

	UXFIO_E_DEBUG3("uxfio_lseek bufferstate  left=%d right=%d", (int)left, (int)right);
	UXFIO_E_DEBUG2("uxfio_lseek offset = %d", (int)offset);

	/* 
	* we don't allow backward beyond the end of the buffer
	* file or memory segment 
	*/
	if (offset < 0 && (((-1) * offset) <= left)) {
		UXFIO_E_DEBUG("ELSE LADDER.");
		uxfio->posM += offset;
		uxfio->current_offsetM += offset;
		uxfio->virtual_offsetM += offset;
		if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			if (lseek(uxfio->buffdM, offset, whence) !=
								uxfio->posM) {
				UXFIO_E_DEBUG("LEAVING");
				return -1;
			}
		}
		retval = (off_t)(uxfio->posM);
	} else if (offset > 0 && (offset <= right)) {
		UXFIO_E_DEBUG("ELSE LADDER.");
		if (virtual_eof_active(uxfio)) {
			switch (pwhence) {
				case SEEK_SET:
					fprintf(stderr, "uxfio_lseek: new lseek error in uxfio_lseek (20070714): %d\n", __LINE__);
					return -1;
					break;
				case UXFIO_SEEK_VSET:
					uxfio->posM = (uxfio->offset_bofM + poffset);
					uxfio->virtual_offsetM = poffset;
					retval = 0;
					;
					break;
				case SEEK_CUR:
					/* FIXME: need to test if buffer is active and dynamic */
					uxfio->posM += (Xint)poffset;
					uxfio->current_offsetM += (Xint)poffset;
					uxfio->virtual_offsetM += (Xint)poffset;
					/*
					fprintf(stderr, "uxfio_lseek: case unhandled: SEEK_CUR, poffset=%d\n", (int)poffset);
					fprintf(stderr, "%s", uxfio_dump_string(uxfio_fildes));
					*/
					retval = uxfio->posM;
					break;
				default:
					fprintf(stderr, "uxfio_lseek: error: unhandled case: pwhence=%d\n", pwhence);
					break;
			}
		} else {
			uxfio->posM += offset;
			uxfio->current_offsetM += offset;
			uxfio->virtual_offsetM += offset;
			if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
				if (lseek(uxfio->buffdM, offset, whence) !=
								uxfio->posM) {
					UXFIO_E_DEBUG("LEAVING");
					return -1;
				}
			}
			retval = (off_t)(uxfio->posM);
		}
				/* read up to the new position */
	} else if (offset > 0 && (offset > right)) {
		UXFIO_E_DEBUG("ELSE LADDER.");
		if (uxfio->v_endM >= 0) {
			n = uxfio->v_endM;
			uxfio->posM += (Xint)n;
			uxfio->current_offsetM += (Xint)n;
			uxfio->virtual_offsetM += (Xint)n;
			retval = (off_t)(uxfio->posM);
			return retval;
		} else if (uxfio->buffertypeM != UXFIO_BUFTYPE_FILE &&
						(!uxfio->buffer_activeM)) {
			UXFIO_E_DEBUG("LEAVING");
			return -1;
		}
		if ((n = uxfio__read_bytes(uxfio_fildes, offset)) != offset) {
			uxfio->posM += (Xint)n;
			uxfio->current_offsetM += (Xint)n;
			uxfio->virtual_offsetM += (Xint)n;
		}
		if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			if (uxfio->posM != lseek(uxfio->buffdM, 0, SEEK_CUR)) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
		"uxfio internal error: buffer file position error 50001.0 \n");
				UXFIO_E_DEBUG("LEAVING");
				return -1;
			}
		}
		retval = (off_t)(uxfio->posM);
	} else if (offset == 0) {
		UXFIO_E_DEBUG("ELSE LADDER.");
		if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			whence = SEEK_CUR;
			if (lseek(uxfio->buffdM, offset, whence)
							!= uxfio->posM) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
		"uxfio internal error: buffer file position error 50001.1 \n");
				UXFIO_E_DEBUG("LEAVING");
				return -1;
			}
			retval = (off_t)(uxfio->posM);
		} else if (whence == UXFIO_SEEK_VSET &&
				uxfio->buffertypeM == UXFIO_BUFTYPE_MEM) {
			retval = (off_t)(uxfio->virtual_offsetM);
		} else if (whence == UXFIO_SEEK_VCUR &&
				uxfio->buffertypeM == UXFIO_BUFTYPE_MEM) {
			retval = (off_t)(uxfio->current_offsetM);
		} else if (whence == UXFIO_SEEK_VCUR &&
				uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM) {
			retval = (off_t)(uxfio->posM - uxfio->offset_bofM);
		} else if (
				whence == UXFIO_SEEK_VSET &&
				uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM &&
				virtual_eof_active(uxfio)
		) {
			retval = uxfio->offset_bofM;
			uxfio->posM = (Xint)(uxfio->offset_bofM);
			uxfio->virtual_offsetM = uxfio->posM;
		} else {
			retval = (off_t)(uxfio->posM);
		}
	} else {
		UXFIO_E_DEBUG("LEAVING");
		return UXFIO_RET_EFAIL;
	}

	if ( do_virtual_set ) {
		/*
		* this returns a 0 for a SEEK_SET on a virtual file
		* even though the real offset may be non-zero.
		*/
		retval -= active_virtual_file_offset(uxfio);
	}

	UXFIO_E_DEBUG3("retval = %d virtual bof = %d",
			(int)retval, active_virtual_file_offset(uxfio));
	UXFIO_E_DEBUG("LEAVING");
	return retval;
}
/*
#define UXFIO_E_DEBUG(arg)
#define UXFIO_E_DEBUG2(arg, arg1)
#define UXFIO_E_DEBUG3(arg, arg1, arg2)
*/

int 
uxfio_fsync(int uxfio_fildes)
{
	UXFIO *uxfio;
	int i = 0;

	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0)
		return fsync(uxfio_fildes);

	if (table_find(uxfio_fildes, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr,
			"uxfio error [%d]: file desc %d not found.\n", __LINE__,
				uxfio_fildes);
		return -1;
	}

	if (uxfio->output_block_sizeM > 0 ) {
		i = do_buffered_flush(uxfio);
	} else if (uxfio->uxfd_can_seekM) {
		i = (*(uxfio->vir_fsyncM))(uxfio->uxfdM);
	}  else if ((uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) && (uxfio->buffdM >= 0)) {
		i = (*(uxfio->vir_fsyncM))(uxfio->buffdM);
	} else {
		return -1;
	}
	return i;
}

int
uxfio_ioctl(int uxfio_fd, int request, void * arg)
{
	UXFIO *uxfio;
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fd);
	if (table_find(uxfio_fd, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]: file desc %d not found.\n", __LINE__, uxfio_fd);
		return -1;
	}
	if (request == UXFIO_IOCTL_SET_STATBUF) {
		if (uxfio->statbufM) swbis_free(uxfio->statbufM);
		uxfio->statbufM = (struct stat*)malloc(sizeof(struct stat));
		if (uxfio->statbufM == NULL) {
			fprintf(stderr,
				"uxfio: line %d: out of memory.\n",
					__LINE__);
			exit(22);
		}
		copy_stat_members (uxfio->statbufM, (struct stat *)(arg));
		uxfio->v_endM=(Xint)(uxfio->statbufM->st_size);
	} else if (request == UXFIO_IOCTL_SET_IMEMBUF) {
		uxfio->bufM = arg;
	} else if (request == UXFIO_IOCTL_GET_STATBUF) {
		*((struct stat**)(arg)) = uxfio->statbufM;
	} else if (request == UXFIO_IOCTL_SET_TMPFILE_ROOTDIR) {
		if (uxfio->tmpfile_rootdirM)
				swbis_free(uxfio->tmpfile_rootdirM);
		uxfio->tmpfile_rootdirM = strdup((char*)arg);
	} else {
		return -1;
	}
	return 0;

}

int
uxfio_fstat(int uxfio_fd, struct stat * buf)
{
	UXFIO *uxfio;
	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fd);
	if (table_find(uxfio_fd, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]: file desc %d not found.\n", __LINE__, uxfio_fd);
		return -1;
	}
	if (uxfio->statbufM == NULL)
		return -1;
	copy_stat_members(buf, uxfio->statbufM);
	return 0;
}

int 
uxfio_fcntl(int uxfio_fildes, int cmd, int value)
{
	UXFIO *uxfio;
	int ret = 0;
	Xint left, right, unused;
	char *buf;

	UXFIO_E_DEBUG2("uxfio_filedes=%d", uxfio_fildes);
	if (uxfio_fildes < UXFIO_FD_MIN && uxfio_fildes >= 0){
		return fcntl(uxfio_fildes, cmd, value);
	}	
	if (table_find(uxfio_fildes, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]: file desc %d not found.\n", __LINE__, uxfio_fildes);
		return -1;
	}
	
	
	if (cmd == UXFIO_F_SET_BUFFER_LENGTH /*UXFIO_F_SETBL*/) {
						/* set memory buffer length */

		if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			/* allow setting this if no bytes of the
			   file would be lost */
			if (value > uxfio->endM) {
				uxfio->lenM = value;
			}
		} else {  /* memory segment buffer */

			bufferstate(uxfio, &left, &right, &unused);

			/* value is new length for uxfio->bufM */
			if (value < right || value <= 0)
				return -1;
			UXFIO_E_DEBUG3("uxfio_filedes=%d malloc amount=%d", uxfio_fildes, value);
			if ((buf = (char *) malloc(value+1)) == (char *) (NULL)) {
				return -1;
			}
			/* now fix buffer */

			if (value >= (right + left)) {
				uxfio__memcpy(buf, 0, uxfio->bufM,
					uxfio->startM, left + right, uxfio);
				uxfio->lenM = value;
			} else if (value >= right) {
				uxfio__memcpy(buf, 0, uxfio->bufM,
						uxfio->posM - value + right,
								value, uxfio);
				uxfio->lenM = value;
				uxfio->startM = 0;
				uxfio->posM = value - right;
				uxfio->virtual_offsetM = value - right;
				uxfio->endM = value;
			} else {
				return -1;
			}
			swbis_free(uxfio->bufM);
			uxfio->bufM = buf;
		}
		return 0;

	} else if (cmd == UXFIO_F_SET_LTRUNC) {		/* truncate buffer */
		/* Depricated */
		return -1;
	} else if (cmd == UXFIO_F_SET_CANSEEK) {
		uxfio->uxfd_can_seekM = value;
		return 0;
	} else if (cmd == UXFIO_F_GET_BUFFER_LENGTH) {
		return uxfio->lenM;
	} else if (cmd == UXFIO_F_GET_VBOF) {
		if (virtual_file_active(uxfio)) {
			return  uxfio->offset_bofM;
		} else {
			return -1;
		}
	} else if (cmd == UXFIO_F_GET_CANSEEK) {
		if (uxfio->buffer_activeM &&
				uxfio->buffertypeM != UXFIO_BUFTYPE_NOBUF) {
			if (uxfio->buffertypeM == UXFIO_BUFTYPE_MEM) {
				if (uxfio->lenM >= 512) return uxfio->lenM;
			}	
			return 1;
		}
		return 0;
	} else if (cmd == UXFIO_F_GET_BUFTYPE) {
		return uxfio->buffertypeM;

	} else if (cmd == UXFIO_F_SET_BUFACTIVE) {   /* set active state,
						      turn buffer on and off */
		UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE start");
		ret = 0;
		if (uxfio->buffertypeM == UXFIO_BUFTYPE_NOBUF) {
			UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE nobuf");
			value = 0;
		} else if (uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) {
			UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE =file");
			if (!value) {
				UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE =file:off");
				/* can't turn off buffer 
				   if it causes loss of information */
				if (uxfio->buffer_activeM && (uxfio->posM < uxfio->endM))
					return -1;
				UXFIO_E_DEBUG3("state:  pos=%d end=%d", (int)(uxfio->posM), (int)(uxfio->endM));
				ret = uxfio__delete_buffer_file(uxfio);
			} else if (value && uxfio->buffer_activeM == 0) {
				UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE on");
				UXFIO_E_DEBUG3("state:  pos=%d end=%d", (int)(uxfio->posM), (int)(uxfio->endM));
				ret = uxfio__init_buffer_file(uxfio,
							(void *)(NULL), 0);
			} else {
				UXFIO_E_DEBUG(
				"in default else case in SET_BUFACTIVE");
			}
		} else {
			UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE !=file");
			if (value == 0 &&
				uxfio->buffer_activeM &&
					(uxfio->posM < uxfio->endM))
				ret=-1;
		}
		uxfio->buffer_activeM = value ? 1 : 0;
		UXFIO_E_DEBUG("in UXFIO_SET_BUFACTIVE end");
		return ret;
	} else if (cmd == UXFIO_F_SET_BUFTYPE) {
		UXFIO_E_DEBUG2("in UXFIO_SET_BUFTYPE start value=%d", value);

		/* FIXME */ if (value < 0 || value > 4) return -1;

		if (value != uxfio->buffertypeM) {
		if ((uxfio->buffertypeM == UXFIO_BUFTYPE_FILE) && 
					value != UXFIO_BUFTYPE_FILE && 
					uxfio->buffer_activeM) {	
					/* close file, open mem segment */
			Xint aa;
			UXFIO_E_DEBUG("in UXFIO_SET_BUFTYPE mem");
			UXFIO_E_DEBUG3("state:  pos=%d end=%d",
						uxfio->posM, uxfio->endM);
			if (uxfio->bufM) {
				swbis_free(uxfio->bufM);
				uxfio->bufM = NULL;
			}	
			if (uxfio->buffertypeM  != UXFIO_BUFTYPE_NOBUF) {
				aa = uxfio->endM + 2;

				if ((uxfio->bufM = (char *)malloc(aa)) == 
							(char *) (NULL)) {
					fprintf(stderr,
					"uxfio: line %d: out of memory.\n",
							__LINE__);
					exit(22);
					return -1;
				}
				uxfio->lenM = uxfio->endM + 1; /* Track one less, so as to
								  create a sneak untracked NUL */
				lseek(uxfio->buffdM, 0, SEEK_SET);
				aa=read(uxfio->buffdM, uxfio->bufM,
								uxfio->endM);
				/* Construct a sneak NUL at the end */
				memset(uxfio->bufM + uxfio->endM, (int)'\0', 2);
			}
			ret = uxfio__delete_buffer_file(uxfio);
		} else if (
		  ((uxfio->buffertypeM == UXFIO_BUFTYPE_MEM) ||
			(uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM)) &&
				  (value == UXFIO_BUFTYPE_FILE) &&
				  uxfio->buffer_activeM
		    ) {		/* open file */
			/* FIXME Huh what's this. */
			if (!value || value >= UXFIO_BUFTYPE_FILE)
				uxfio->buffertypeM = UXFIO_BUFTYPE_FILE;
			else
				uxfio->buffertypeM = UXFIO_BUFTYPE_MEM;
			
			UXFIO_E_DEBUG("in UXFIO_SET_BUFTYPE file");
			UXFIO_E_DEBUG3("state:  pos=%d end=%d",
						uxfio->posM, uxfio->endM);
			ret = uxfio__init_buffer_file(uxfio, uxfio->bufM,
							uxfio->endM);
		} else if (value == UXFIO_BUFTYPE_NOBUF) {
			UXFIO_E_DEBUG("in UXFIO_SET_BUFTYPE nobuf");
			if (uxfio_fcntl(uxfio_fildes, UXFIO_F_SET_BUFACTIVE,
								UXFIO_OFF)) {
				UXFIO_E_DEBUG("error");
				fprintf(stderr,
					"uxfio internal error: 002.33\n");
			}
			ret = 0;
		} else {
			UXFIO_E_DEBUG(" default else in set buftype");
		}
		uxfio->buffertypeM = value;
		} else {
			ret = 0;
		}
		return ret;
	} else if (cmd == UXFIO_F_ATTACH_FD) {
		uxfio->uxfdM = value;
		uxfio__change_state(uxfio, (value >= UXFIO_FD_MIN));
		return uxfio_fildes;
	} else if (cmd == UXFIO_F_GET_VEOF) {
		if (virtual_file_active(uxfio)) {
			return  uxfio->offset_eof_savedM;
		} else {
			return -1;
		}
	} else if (cmd == UXFIO_F_SET_VEOF) {
		UXFIO_E_DEBUG3("fd=%d  set VEOF value = %d",
						uxfio_fildes, value);
		if (value < 0) {
			uxfio->current_offsetM = 0;
			uxfio->virtual_offsetM = 0;
			uxfio->offset_eofM = -1;
			uxfio->offset_bofM = 0;
			uxfio->offset_eof_savedM = -1;
		} else {
			/* uxfio->bytesreadM = 0; */
			uxfio->current_offsetM = 0;
			uxfio->virtual_offsetM = 0;
			uxfio->offset_eofM = value;
			uxfio->offset_bofM = uxfio->posM;
			uxfio->offset_eof_savedM = value;
		}
		return 0;
	} else if (cmd == UXFIO_F_ARM_AUTO_DISABLE) {
		uxfio->auto_disableM = value ? 1 : 0;
		return 0;
	} else if (cmd == UXFIO_F_WRITE_INSERT) {
		uxfio->write_insertM = value ? 1 : 0;
		return 0;
	} else if (cmd == UXFIO_F_SET_LOCK_MEM_FATAL) {
		uxfio->lock_buf_fatalM = value;
		return 0;
	} else if (cmd == UXFIO_F_DO_MEM_REALLOC) {
		if (value < 0) return -1;
		if (uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM) {
			void * newbuf;
			newbuf = SWBIS_REALLOC(uxfio->bufM,
				uxfio->lenM + value + UXFIO_ALLOC_AHEAD + 1,
					uxfio->lenM);
			if (!newbuf) {
				fprintf(stderr, "out of memory\n");
				exit(4);
			}
			uxfio->bufM = newbuf;
			uxfio->lenM += (value + UXFIO_ALLOC_AHEAD);
			memset(uxfio->bufM + uxfio->endM, (int)'\0', uxfio->lenM + 1 - uxfio->endM);
		} else {
			return -2;
		}
		return 0;
	} else if (cmd == UXFIO_F_SET_OUTPUT_BLOCK_SIZE) {
		if (value < 0) return -1;
		uxfio->output_block_sizeM = value;
		if (uxfio->output_bufferM) free(uxfio->output_bufferM);
		uxfio->output_bufferM = (char *)malloc(value + 1);
		return 0;
	} else if (uxfio->uxfdM != UXFIO_NULL_FD) {
		if (uxfio->uxfdM < UXFIO_FD_MIN) {
			return fcntl(uxfio->uxfdM, cmd, value);
		} else {
			return uxfio_fcntl(uxfio->uxfdM, cmd, value);
		}
	}
	return -1;
}

char *
uxfio_get_fd_mem(int uxfio_fildes, int * data_len)
{
	char * s;	
	int ret;
	int end;
	ret = uxfio_get_dynamic_buffer(uxfio_fildes, 
					&s,
					&end,
					data_len);

	if (ret < 0) return NULL;
	return s;
}

int
uxfio_get_dynamic_buffer(int uxfio_fildes, char **buffer_ptr,
				int *buffer_end, int *buffer_len)
{
	UXFIO *uxfio;
	off_t curpos;

	if (table_find(uxfio_fildes, &uxfio, 1)) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "uxfio error [%d]: file desc %d not found.\n", __LINE__, uxfio_fildes);
		return -1;
	}
	if (uxfio->buffertypeM == UXFIO_BUFTYPE_DYNAMIC_MEM) {
		*(uxfio->bufM + uxfio->lenM) = '\0';
		*(uxfio->bufM + uxfio->endM) = '\0';
		if (buffer_ptr)
			(*buffer_ptr) = uxfio->bufM;
		if (buffer_len) {
			curpos = uxfio_lseek(uxfio_fildes, 0, SEEK_CUR);
			uxfio_lseek(uxfio_fildes, 0, SEEK_END);
			(*buffer_len) = uxfio_lseek(uxfio_fildes, 0, SEEK_CUR);
			uxfio_lseek(uxfio_fildes, curpos, SEEK_SET);
		}
		if (buffer_end)
			(*buffer_end) = uxfio->endM;
		return 0;
	} else {
		return -1;
	}
}
