/* uxfio.h:  buffered u*ix I/O functions.
 */

#ifndef uxfio_20020419_h
#define uxfio_20020419_h

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* uxfio ---  Unix  eXtended  File  Input/Output  API */

#define UXFIO_FLEN 	2000000000    /* default max length of tmpfile */
#define UXFIO_LEN 	1024        /* default buffer length for memory buffer */
#define UXFIO_MAX_OPEN	128        /* maximum number of open uxfio files */
#define UXFIO_FD_MIN 	2000000000 /* uxfio_fd's starts at 2 Billion */
#define UXFIO_OFF 	0
#define UXFIO_ON 	1
#define UXFIO_BUFTYPE_NOBUF 	0
#define UXFIO_BUFTYPE_MEM 	1	/* default length is UXFIO_LEN  */
#define UXFIO_BUFTYPE_FILE 	2
#define UXFIO_BUFTYPE_DYNAMIC_MEM 3       /* length is infinite */
#define UXFIO_BUFTYPE_VEOF 	4
#define UXFIO_RET_EFAIL 	-9999
#define UXFIO_RET_EBADF 	-9998
#define UXFIO_RDONLY 		0
#define UXFIO_WRONLY 		1
#define UXFIO_RDWR 		2
#define UXFIO_SEEK_VCUR		3081
#define UXFIO_SEEK_VSET		4082

/*   File Controls       Hopefully none of these match a real uxix system. */
#define UXFIO_F_SETBL	-11725     /* Deprecated Use UXFIO_F_SET_BUFFER_LENGTH */
#define UXFIO_F_SET_INITTR  -11726     /* initialize byte counter in tread */
#define UXFIO_F_SET_DISATR  -11727     /* disable byte counter in tread */
#define UXFIO_F_GET_BYTETR  -11728     /* depricated */
#define UXFIO_F_SET_CANSEEK 	-11729 /* deprecated */
#define UXFIO_F_SET_LTRUNC 	-11730 /*truncate everything left 
						of current offset */
#define UXFIO_F_SET_BUFACTIVE 	-11731   /* set buffer_active flag */
#define UXFIO_F_SET_BUFTYPE 	-11732  /* 0 is memory, 1 is diskfile */
#define UXFIO_F_ATTACH_FD 	-11733  /* set uxfio->uxfd */
#define UXFIO_F_SET_VEOF 	-11734  /* set uxfio->offset_eof to value */
#define UXFIO_F_ARM_AUTO_DISABLE -11735 /* ARM auto-buffer disable feature */
#define UXFIO_F_GET_BUFTYPE 	-11736  /* return buffer type */
#define UXFIO_F_GET_CANSEEK 	-11737  /* true if seekable or if buffer
						size is atleast 512 bytes. */
#define UXFIO_F_WRITE_INSERT	-11738  /* insert into file if
						dynamic_mem buffered. */
#define UXFIO_F_SET_BUFFER_LENGTH -11725  /* set buffer length,
						either file or memory */
#define UXFIO_F_GET_BUFFER_LENGTH -11739  /* set buffer length, either
							file or memory */
#define UXFIO_F_GET_VBOF 	-11740 	/* get the offset of the current
					   virtual file beginning. */
#define UXFIO_F_DO_MEM_REALLOC  -11741  /* Do a realloc presumably
						specifying more memory. */
#define UXFIO_F_SET_LOCK_MEM_FATAL   -11742  /* If call to realloc moves
					  	memory then exit. */
#define UXFIO_F_SET_OUTPUT_BLOCK_SIZE -11743  /* Set output block size,
						Zero (0) for no buffering. */
#define UXFIO_F_GET_VEOF 	-11744 

/* I/O Controls  Hopefully none of these match a real uxix system. */
#define UXFIO_IOCTL_SET_STATBUF 		-12725
#define UXFIO_IOCTL_GET_STATBUF 		-12726
#define UXFIO_IOCTL_SET_TMPFILE_ROOTDIR		-12727
#define UXFIO_IOCTL_SET_IMEMBUF			-12728

/* Default tmp file name components */
#define UXFIO_TMPFILE_ROOTDIR		"/usr/tmp/"
#define UXFIO_TMPFILE_PFX		"uxfio"

#define UXFIO_NULL_FD 	-10001
/*-------------------------------------------------------------*/
/*------- UXFIO API (PUBLIC FUNCTIONS)------------------------*/

void uxfio_show_all_open_fd(FILE * f);

int uxfio_uxfio_get_nopen(void);

int uxfio_open ( char * path, int oflag, mode_t mode );

int uxfio_opendup ( int unix_fd, int BUF_TYPE );
					/* open via already open unix_fd 
					or uxfio_fd instead of filename */
int uxfio_close ( int uxfio_fides );

ssize_t uxfio_read ( int uxfio_fildes, void *buf, size_t nbyte );

ssize_t uxfio_sfa_read(int uxfio_fildes, void *buf, size_t nbyte ); /* actually a polling read */

ssize_t uxfio_sfread(int uxfio_fildes, void *buf, size_t nbyte ); /* actually a polling read */

ssize_t uxfio_unix_safe_atomic_read(int unix_fd, void * buf, size_t nbyte);

/* ssize_t uxfio_polling_read(int unix_fd, void * buf, int nbyte); */

ssize_t uxfio_write(int uxfio_fildes, void *buf, size_t nbyte );

off_t uxfio_lseek(int uxfio_fildes, off_t offset, int whence );

int uxfio_ftruncate(int uxfio_fildes, off_t offset );

int uxfio_fcntl(int uxfio_fildes, int cmd , int value); 

int uxfio_ioctl(int uxfio_fd, int request, void * arg);

int uxfio_fstat(int uxfio_fd, struct stat * statbuf);

int uxfio_fsync(int uxfio_fildes);

int uxfio_espipe(int uxfio_fildes);

int uxfio_incr_use_count(int uxfio_fd);

int uxfio_getfd (int uxfio_fildes, int * will_read_next);
			/* return the unix file descriptor */
			/* if will_read_next is true, the file */
			/* position was at  end of buffer. */

int uxfio_free ( int uxfio_fides ); /* close the UXFIO object but don't 
                                       close the unix file. */

int uxfio_get_dynamic_buffer(int unix_fildes ,char **buffer_ptr,
					int *buffer_end, int *data_len);

/* D int  uxfio_debug_dump(int uxfio_fildes ); */
					/* show object, for debugging */
char *  uxfio_dump_string(int uxfio_fildes );
char *  uxfio_dump_string_s(int uxfio_fildes, char * prefix);
/* int uxfio_filecpy(int out_fd, int in_fd, int count); */
int uxfio_fileinsert(int dst_fd, int src_fd, int count);
void * uxfio_get_object_address(int uxfiofd);
ssize_t uxfio_full_write (int desc, const char *ptr, size_t len);
ssize_t uxfio_unix_atomic_read(int fd, void * buf, size_t nbyte);
ssize_t uxfio_unix_atomic_write(int fd, void * buf, size_t nbyte);
ssize_t uxfio_unix_safe_read(int fd, void * buf, size_t nbyte);
ssize_t uxfio_unix_safe_write(int fd, void * buf, int nbyte);
char * uxfio_get_fd_mem(int uxfio_fildes, int * data_len);
int uxfio_devnull_open(char *path, int oflag, mode_t mode);
int uxfio_devnull_close(int fd);

#endif
