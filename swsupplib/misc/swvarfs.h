/* swvarfs.h - File commands on an archive stream or directory.
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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


#ifndef SWVARFS_20030131_H
#define SWVARFS_20030131_H

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "ahs.h"
#include "uinfile.h"
#include "cpiohdr.h"
#include "taru.h"
#include "hllist.h"
#include "uxfio.h"
#include "swuser_config.h"

struct fileChain {
	char *nameFC;
	int header_offsetFC;
	intmax_t data_offsetFC; 
	struct fileChain * prevFC;
	struct fileChain * nextFC;
	int uxfio_u_fdFC;
};

#define SWVARFS_OPEN			8
#define SWVARFS_DIR_WALK_LENGTH		100

#define SWVARFS_S_IFDIR	0040000
#define SWVARFS_S_IFREG	0100000
#define SWVARFS_VSTAT_LSTAT		"lstat"
#define SWVARFS_VSTAT_STAT		"stat"

struct dirContext {		/* Stack for directory-tree walking routine. */
	struct stat * statbuf;
	struct dirent * dirp;
	DIR	* dp;
	char * ptr;
};


typedef struct {
	char * u_current_name_;		/* u_<*> are attributes for the opening of */
	char * u_linkname_;		/* individual files. */
	int    u_fdM;			/* file descriptor, maybe a uxfio_fd */
} SWVARFS_U_FD_DESC;

typedef struct {
	UINFORMAT * format_descM;	/* The Format descriptor, see uinfile.c */
	int uinformat_close_on_deleteM; /* close format_descM  on close */  
	int formatM;			/* The format,  see uinfile.h */
	STROB * openpathM;		/* The opening path. */
	STROB * opencwdpathM;		/* The current working directory. */
	STROB * u_nameM;		/* Temporary name of current user file. */
	STROB * dirscopeM;		/* The current scope, trversals are limited to this directory. */
	STROB * tmpM;			/* Tempory string memory. */
	STROB * vcwdM;			/* The virtual swvarfs cwd */
	AHS * ahsM;			/* File metadata object. */
	int do_close_ahsM;
	int fdM;
	int did_dupM;

	SWVARFS_U_FD_DESC  u_fd_setM[4];	/* Internal descriptors for archive members */
	/* Archive member files. */
	char * u_current_name_;		/* u_<*> are attributes for the opening of */
	char * g_linkname_;		/* individual files. */
	int    u_fdM;			/* file descriptor, maybe a uxfio_fd */
	int    did_u_openM;

	/* File chain record */
	struct fileChain * headM;
	struct fileChain * tailM;
	int (*f_do_stop_)(void *, char * pattern, char * name);

	/* Directory walking */
	STROB * direntpathM;  		/* The current dirent directory path. */
	int stackixM;			/* index into the list of dirContext objects stored in (STROB *)stackM */
	STROB * stackM; 		/* list of dirContext objects */
	int derrM;      		/* error flag */
	int have_read_filesM;
	struct fileChain *current_filechainM; 
	
	/* General */	
	int uxfio_buftype;
	intmax_t current_header_offsetM;
	intmax_t current_data_offsetM;
	int is_unix_pipeM;
	int eoaM;			/* End of archive reached ?? */
	int makefilechainM;		/* Override in-order traversal optimization. */
	int has_leading_slashM;		/* Archive has leading slash. */
	int (*f_statM)(char * path, struct stat *);
	HLLIST *link_recordM;		/* Hard link list object */
	TARU * taruM;
	int n_loop_symlinksM;		/* number of allowable symlinks */
} SWVARFS;

/* ++++  Public API  +++++ */

SWVARFS * 	swvarfs_open		(char * name, int oflags, mode_t mode);
SWVARFS * 	swvarfs_open_directory	(char * name);
SWVARFS * 	swvarfs_open_archive_file(char * name, int oflags);
SWVARFS * 	swvarfs_opendup		(int ifd, int oflags, mode_t mode);
SWVARFS * 	swvarfs_opendup_with_name (int ifd, int oflags, mode_t mode, char * slack_name);
int 		swvarfs_close		(SWVARFS * swvarfs);
struct new_cpio_header * swvarfs_header	(SWVARFS * swvarfs);
AHS * 		swvarfs_get_ahs		(SWVARFS * swvarfs);
void 		swvarfs_set_ahs		(SWVARFS * swvarfs, AHS * ahs);
int 		swvarfs_fd		(SWVARFS * swvarfs);
int 		swvarfs_setdir		(SWVARFS * swvarfs, char * path);
int 		swvarfs_vchdir		(SWVARFS * swvarfs, char * path);
int 		swvarfs_u_open		(SWVARFS * swvarfs, char * name);
int		swvarfs_u_close		(SWVARFS * swvarfs, int fd);
int 		swvarfs_u_ftruncate	(SWVARFS * swvarfs, int fd, off_t size);
int 		swvarfs_u_usr_stat	(SWVARFS * swvarfs, int fd, int ofd);
int		swvarfs_u_lstat		(SWVARFS * swvarfs, char * path, struct stat * st);
int 		swvarfs_u_fstat		(SWVARFS * swvarfs, int fd, struct stat *st);
int 		swvarfs_u_readlink	(SWVARFS * swvarfs, char * path, char * buf, size_t bufsize);
char * 		swvarfs_u_get_linkname	(SWVARFS * swvarfs, int fd);
char * 		swvarfs_u_get_name	(SWVARFS * swvarfs, int fd);
void 		swvarfs_stop_function	(SWVARFS * swvarfs, int (*fc)(void*, char *, char *));
char * 		swvarfs_get_next_dirent (SWVARFS * swvarfs, struct stat *st);
int 		swvarfs_dirent_reset	(SWVARFS * swvarfs);
int 		swvarfs_dirent_err	(SWVARFS * swvarfs);
int		swvarfs_get_format	(SWVARFS * swvarfs);
int 		swvarfs_file_has_data	(SWVARFS * swvarfs);
int 		swvarfs_uxfio_fcntl	(SWVARFS * swvarfs, int cmd, int value);
void 		swvarfs_debug_dump_filechain(SWVARFS * swvarfs);
/*D char *		swvarfs_dump_string_s(SWVARFS * swvarfs, char * prefix); */
int 		swvarfs_debug_list_files(SWVARFS * swvarfs, int ofd);
void		swvarfs_set_stat_syscall(SWVARFS * swvarfs, char * s);
char *		swvarfs_get_stat_syscall(SWVARFS * swvarfs);
HLLIST * 	swvarfs_get_hllist	(SWVARFS * swvarfs);
UINFORMAT * 	swvarfs_get_uinformat	(SWVARFS *swvarfs);
int 		swvarfs_get_tarheader_flags(SWVARFS * xux);
void 		swvarfs_set_tarheader_flags(SWVARFS * xux, int flags);
void 		swvarfs_set_tarheader_flag(SWVARFS * xux, int flag, int n);

#endif
