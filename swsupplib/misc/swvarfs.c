/* swvarfs.c  --  File I/O calls on an archive stream or directory file.

   Copyright (C) 2003  Jim Lowe 
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "ahs.h"
#include "swvarfs.h"
#include "swutilname.h"
#include "strar.h"
#include "swlib.h"
#include "fnmatch_u.h"

#include "debug_config.h"

#undef SWVARFSNEEDDEBUG

#ifdef SWVARFSNEEDDEBUG
#define SWVARFS_E_DEBUG(format) SWBISERROR("SWVARFS DEBUG: ", format)
#define SWVARFS_E_DEBUG2(format, arg) SWBISERROR2("SWVARFS DEBUG: ", format, arg)
#define SWVARFS_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWVARFS DEBUG: ", format, arg, arg1)
#else
#define SWVARFS_E_DEBUG(arg)
#define SWVARFS_E_DEBUG2(arg, arg1)
#define SWVARFS_E_DEBUG3(arg, arg1, arg2)
#endif /* SWVARFSNEEDDEBUG */


/* +++++++++++++++++++++++++++++++++++
NOTE:
    This tar reading code breaks at the 2^31 byte limit.

+++++++++++++++++++++++++++++++++++ */


/* ---------------  Private functions ----------------------- */


#define u_fd_array_len(a)	((int)(sizeof(a->u_fd_setM)/sizeof(SWVARFS_U_FD_DESC)))

static
int
show_nopen(void)
{
	int ret;
	ret = /*--*/open("/dev/null",O_RDWR);
	if (ret<0)
		fprintf(stderr, "fcntl error: %s\n", strerror(errno));
	close(ret);
	return ret;
}

static void
ud_init(SWVARFS * swvarfs)
{
	int i;
	for (i=0; i<u_fd_array_len(swvarfs); i++) {
		swvarfs->u_fd_setM[i].u_fdM = -1;
		swvarfs->u_fd_setM[i].u_current_name_ = (char*)NULL;
		swvarfs->u_fd_setM[i].u_linkname_ = (char*)NULL;
	}
}

static
int
ud_set_fd(SWVARFS * swvarfs, int fd)
{
	int i;
	for (i=0; i<u_fd_array_len(swvarfs); i++) {
		if (swvarfs->u_fd_setM[i].u_fdM < 0) {
			swvarfs->u_fd_setM[i].u_fdM = fd;
			return fd;
		}
	}
	return -1;
}

static
int
ud_unset_fd(SWVARFS * swvarfs, int fd)
{
	int i;
	for (i=0; i<u_fd_array_len(swvarfs); i++) {
		if (swvarfs->u_fd_setM[i].u_fdM == fd) {
			swvarfs->u_fd_setM[i].u_fdM = -1;
			return fd;
		}
	}
	return -1;
}

static
int
ud_find_fd(SWVARFS * swvarfs, int fd)
{
	int i;
	for (i=0; i<u_fd_array_len(swvarfs); i++) {
		if (swvarfs->u_fd_setM[i].u_fdM == fd) {
			return i;
		}
	}
	return -1;
}


static
void
u_reject(SWVARFS * swvarfs) {
	SWVARFS_E_DEBUG("ENTERING");
	if (swvarfs->u_current_name_) swbis_free(swvarfs->u_current_name_);
	swvarfs->u_current_name_ = (char*)NULL;
	swvarfs->current_filechainM = NULL;
	SWVARFS_E_DEBUG("LEAVING");
}


static
int
is_name_a_path_prefix(char * npath, char * nprefix)
{
	int ret = 0;
	char * trl = NULL;
	char * s;
	char * path;
	char * prefix;

	/* fprintf(stderr, "npath=[%s] nprefix=[%s]\n", npath, nprefix);
	*/

	if (strcmp(nprefix, ".") == 0 || strcmp(nprefix, "./") == 0) {
		return 1;
	}

	if (!nprefix || *nprefix == '\0') return 0;


	if (nprefix[strlen(nprefix) - 1] == '/') {
		trl = nprefix + (strlen(nprefix) - 1);
		*trl = '\0';
	}

	if (strncmp(npath, "./", 2) == 0) {
		path = npath + 2;
	} else {
		path = npath;
	}
	
	if (strncmp(nprefix, "./", 2) == 0) {
		prefix =  nprefix + 2;
	} else {
		prefix =  nprefix;
	}

	/*
	fprintf(stderr, "path=[%s] prefix=[%s]\n", path, prefix);
	*/

	if (strlen(prefix) == 0 || 
		strlen(path) == 0 || 
			strcmp(path, prefix) == 0) { 
		;
	} else {

		/*
		fprintf(stderr, "newprefix=[%s]\n", prefix);
		*/	
		if (
			(s=strstr(path, prefix)) &&
			s == path &&
			( strlen(path) == strlen(prefix) ||
				path[strlen(prefix)] == '/' )
		
		) {
			ret = 1;
		}
	}
	if (trl) *trl = '/';
	return ret;
}

static
void
handle_leading_slash(SWVARFS * swvarfs, char * mode, char * name, int *pflag)
{

	/*
	* If the serial access archive has leading '/' then the name must be
	* matched exactly.  If no leading slash, then an absolute path name 
	* will match a name in the archive iff it differs only in the absense
	* of a leading slash.
	*/
	
	if (swvarfs->formatM !=UINFILE_FILESYSTEM &&
				swvarfs->has_leading_slashM == 0) {
		/*
		* mangle the name with a leading
		* slash if the archive has no leading slash
		* inorder to produce a logical match since this is
		* what the user might resonably expect.
		*/
		if (strcmp(mode, "drop") == 0) {
			if (*name == '/') {
				memmove(name, name+1, strlen(name));
				*pflag = 1;
			} else {
				*pflag = 0;
			}
		} else if (strcmp(mode, "restore") == 0) {
			if (*pflag) {
				memmove(name+1, name, strlen(name));
				*name = '/';
				*pflag = 0;
			}
		}
	} else {
		/*
		* Nothing done.
		*/
		*pflag = 0;
	}
}

static
void
handle_trailing_slash(char * mode, char * name, int *pflag)
{
	if (strcmp(mode, "drop") == 0) {
		if (name[strlen(name) - 1] == '/') {
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

static
int 
filename_does_match(char * name, char * candidate)
{
	int ret;
	ret = swlib_compare_8859(name, candidate);
	return ret;
}


static
int
is_name_in_scope(char * dirscope, char * name, struct stat * namest) {
	int ret;
	char *s, *name1;
	
	if (strlen(dirscope) == 0 || strcmp(dirscope, "/") == 0) {
		/*
		* No scope specified, Then Yes, in scope.
		*/
		return 1;
	}
		
	s = strstr(name, "./");
	if (s != NULL && s == name && strlen(name) > 2) {
		name1 = name + 2;
	} else {
		name1 = name;
	}

	if (strlen(name1) + 4 < strlen(dirscope)) {
		/*
		* Obviosly not a match.
		* +4 because of the "\/\*" on dirscopeM and a possible
		* leading slash in name1.
		*/
		return 0;
	}
			
	s = strstr(dirscope, name1);
	if (s != NULL &&
		strlen(name1) >= strlen(dirscope) - 2) {
		/*
		* This handles the special case of a exact match
		*
		*  -2 because of the trailing "\/\*" on dirscope
		*
		* i.e  name1 is "a/b/c" or "a/b/c/" and dirscope
		*      is "a/b/c\/\*"
		*/
		if (namest != NULL && S_ISDIR(namest->st_mode) == 0) {
			/*
			* Not a match, because its not a directory.
			*/
			return 0;
		} else {
			return 1;	
		}
	}

	{
	STROB * tmp = strob_open(10);
	STROB * tmp1 = strob_open(10);
	strob_strcpy(tmp, dirscope);
	strob_strcpy(tmp1, name1);

	if (!strstr(strob_str(tmp), "/*")) {
		strob_strcat(tmp, "/*");
	}	

	strob_strcpy(tmp1, name1);
	if (strstr(strob_str(tmp), "./") == strob_str(tmp)) {
		strob_strcpy(tmp1, "./");
		strob_strcat(tmp1, name1);
	}

	ret = fnmatch(strob_str(tmp), strob_str(tmp1), 0);
	strob_close(tmp);
	strob_close(tmp1);
	}
	return !ret;
}


static
void
process_vcwd(STRAR * pathcomps, char * source)
{
	char * s, *pc;
	STROB * tmp = strob_open(10);

	/*
	* handle special case of leading / which is now
	* really a '//'
	*/

	if (strstr(source, "//") == source) {
		strar_add(pathcomps, source);
		*(source+1) = '\0';
		s = source + 2;
	} else {
		s = source;
	}

	/*
	* Now split path
	*/

	pc = strob_strtok(tmp, s, "/");
	while (pc) {
		strar_add(pathcomps, pc);
		pc = strob_strtok(tmp, (char*)NULL, "/");
	}

	/*
	* done.
	*/
	strob_close(tmp);
	return;
}

static
int
construct_new_path_from_pathspec(SWVARFS * swvarfs, STROB * newvcwd,
							char * pathspec)
{
	STRAR * pathcomps;
	STRAR * cmdcomps;
	char * sx;
	char * cx;
	int ncomp;
	int i = 0;
	int j = 0;
	int ret = 0;

	strob_strcpy(newvcwd, "");
	if (*pathspec == '/') {
		if (strstr(pathspec, "..")) return -1;
		if (strstr(pathspec, "./")) return -1;
		strob_strcpy(newvcwd, pathspec);
		return 0;
	}

	pathcomps = strar_open();
	cmdcomps = strar_open();
	sx = malloc(strob_strlen(swvarfs->vcwdM) + 2);
	if (sx == NULL) exit(31);

	if (*strob_str(swvarfs->vcwdM) == '/') {
		strcpy(sx, "/");	
	} else {
		strcpy(sx, "");	
	}
	strcat(sx, strob_str(swvarfs->vcwdM));

	process_vcwd(pathcomps, sx);
	
	swlib_squash_all_dot_slash(pathspec);
	process_vcwd(cmdcomps, pathspec);

	ncomp = strar_num_elements(pathcomps);

	j = 0;
	cx = strar_get(cmdcomps, i++);
	while(cx) {
		if (strcmp(cx, "..") == 0) j++;
		cx = strar_get(cmdcomps, i++);
	}

	/*
	* Now assemble the new path.
	*/
	ncomp -= j;
	if (ncomp <= 0) {
		/*
		* error
		*/
		ret = -1;
	} else {
		strob_strcpy(newvcwd, "");
		i = 0;
		while (i < ncomp) {
			if (i > 0) strob_strcat(newvcwd, "/");
			strob_strcat(newvcwd, strar_get(pathcomps, i++));
		}
		strob_strcat(newvcwd, "/");
		i = j;
		while (i < strar_num_elements(cmdcomps)) {
			if (i > j) strob_strcat(newvcwd, "/");
			strob_strcat(newvcwd, strar_get(cmdcomps, i++));
		}

		ret = 0;
	}
	strar_close(pathcomps);
	strar_close(cmdcomps);
	free(sx);
	return ret;
}


static
void
dump_filechain_link(struct fileChain *  link)
{
	STROB * ubuf = strob_open(24);
	fprintf(stderr, "%s header_offset=%d data_offset=%s\n",
		link->nameFC, link->header_offsetFC, swlib_imaxtostr(link->data_offsetFC, ubuf));
	strob_close(ubuf);
}

static
int
check_seek_violation(SWVARFS * swvarfs)
{
	int uxfio_buffer_type = swvarfs_uxfio_fcntl(swvarfs,
						UXFIO_F_GET_BUFTYPE, 0);
	if (
		(
			swvarfs->is_unix_pipeM && swvarfs->fdM < UXFIO_FD_MIN
		) ||
		(
			swvarfs->is_unix_pipeM && 
			swvarfs->fdM >= UXFIO_FD_MIN &&
			(uxfio_buffer_type != UXFIO_BUFTYPE_FILE &&
				uxfio_buffer_type != UXFIO_BUFTYPE_DYNAMIC_MEM) 
		)	
	) {
		fprintf(stderr, 
	"\nThe pending operation is invalid for use with a pipe.\n");
		fprintf(stderr, 
	"Re-try with a disk file or stdin redirected from a disk file\n");
		fprintf(stderr, 
	"or, file/dynamic-mem buffering, or, in-order traversal only.\n");
		return 10;		
	}
	return 0;
}

static void
set_vcwd(STROB * cw, char * newvcwd)
{
	strob_strcpy(cw, newvcwd);
	if (strcmp(strob_str(cw), "./") == 0) {
		*(strob_str(cw) + 1) = '\0';
	}
}

static
int
set_linkname(SWVARFS * swvarfs, int ufd, char * path)
{
	int c;
	int retval = 0;
	struct new_cpio_header * file_hdr;
	char * s;
	char * vlinkp;
	int vlen = 1024;  /* FIXME arbitrary limit */

	SWVARFS_E_DEBUG2("ENTERING [%s]", path); 
	if (ufd < 0) {
		vlinkp = swvarfs->g_linkname_;
	} else {
		int u_index = ud_find_fd(swvarfs, ufd);
		if(u_index >= 0) {
			vlinkp = swvarfs->u_fd_setM[u_index].u_linkname_;
		} else {
			fprintf(stderr, 
			"internal error,"
			" %d desc not found in swvarfs:set_linkname()\n",
				ufd);
			return -1;
		}
	}
	file_hdr = ahs_vfile_hdr(swvarfs->ahsM);

	ahsStaticSetTarLinknameLength(file_hdr, vlen);
	s = ahsStaticGetTarLinkname(file_hdr);
	s[vlen-1] = '\0';	
	c=readlink(path, s, vlen-1);
	if (c < 0) {
	SWBIS_E_FAIL3("error: readlink on [%s] returned %d.\n", path, c);
		return -1;
	}
	if (c >= vlen-1) {
		c = vlen -1;
		fprintf(stderr, "Warning, linkname [%s] truncated.\n", s);
	}
	s[c] = '\0';	
	if (strlen(s) > TARNAMESIZE) {
		fprintf(stderr, 
		"Warning, linkname [%s] is too long for tar archives\n", s);
	}
	if (vlinkp){ 
		swbis_free(vlinkp); 
		vlinkp = (char*)NULL; 
	}
	vlinkp = strdup(s);
	return retval;
}

static
void
swvarfs_i_delete_link(struct fileChain * link)
{
	swbis_free(link->nameFC);
	swbis_free(link);
}

static
void 
swvarfs_i_delete_filechain(SWVARFS * swvarfs)
{
	struct fileChain * last = swvarfs->tailM;
	struct fileChain * newlast;

	while(last) {
		newlast = last->prevFC;
		swvarfs_i_delete_link(last);
		last = newlast;	
	}
}

static
void 
swvarfs_i_clear_filechain(SWVARFS * swvarfs)
{
	swvarfs_i_delete_filechain(swvarfs);
	swvarfs->headM=NULL;
	swvarfs->tailM=NULL;
}

static
void 
swvarfs_i_attach_filechain(SWVARFS * swvarfs, struct fileChain *new)
{
	new->prevFC = swvarfs->tailM;
	new->nextFC = NULL;
	SWVARFS_E_DEBUG("ENTERING"); 
	if (swvarfs->tailM) {
		swvarfs->tailM->nextFC = new;
	}
	swvarfs->tailM = new;
	if (!swvarfs->headM) {
		swvarfs->headM = swvarfs->tailM;
	}
}

static
int
stat_node(SWVARFS * swvarfs, char * fp_path, struct stat * statbuf)
{
	char * path;
	int i;
	STROB * linkbuf;
	int bufsize;
	STROB * totallinkbuf;
	int ret;

	SWVARFS_E_DEBUG("BEGIN"); 
	bufsize=300;
	linkbuf = NULL;
	totallinkbuf = NULL;
	path = fp_path;
	i = 0;
	SWVARFS_E_DEBUG(""); 

	/* ... actually, the stat() system call follows multiple links
	   so this while loop is probably not required as it will
	   never loop more that once (on a Linux kernel at least). */

	while (i++ < swvarfs->n_loop_symlinksM) {
		SWVARFS_E_DEBUG2("in sumlink loop, i=%d", i); 
		if ((*(swvarfs->f_statM))(path, statbuf) < 0) {
			fprintf(stderr,"%s: getnext_dirent: %s() failed: %s\n",
				swlib_utilname_get(), swvarfs_get_stat_syscall(swvarfs), path);
			swvarfs->derrM = -2;
			return -1;
		}

		if (*swvarfs_get_stat_syscall(swvarfs) == 'l') {
			/* lstat() system call being used */
			/* If the stat call is 'lstat' only do the loop once */
			break;
		} else {
			/* The stat() system call being used */
			; 
		}

		if (S_ISLNK(statbuf->st_mode)) {
			SWVARFS_E_DEBUG("Never gets here"); 
			/* This code will never happen on a kernel 
			   stat() call resolves multiple symbolic links */
		
			SWVARFS_E_DEBUG("ISLNK is true"); 
			if (linkbuf == NULL) {
				linkbuf = strob_open(bufsize+2);
			}
			if (totallinkbuf == NULL) {
				totallinkbuf = strob_open(bufsize);
				strob_strcpy(totallinkbuf, path);
			}
			strob_memset(linkbuf, (int)('\0'), bufsize+2);
			ret = readlink(strob_str(totallinkbuf), strob_str(linkbuf), bufsize);
			SWVARFS_E_DEBUG("");
			if (ret < 0 || ret >= bufsize-1) {
				if (ret < 0) {
					fprintf(stderr,"%s: readlink failed: %s on %s\n",
						swlib_utilname_get(), strob_str(totallinkbuf), strerror(errno));
				} else {
					fprintf(stderr,"%s: readlink failed: program buffer: %s\n",
						swlib_utilname_get(), strob_str(totallinkbuf));
				}
				return -1;
			}
			SWVARFS_E_DEBUG(""); 
			if (*strob_str(linkbuf) == '/') {
				strob_strcpy(totallinkbuf, strob_str(linkbuf));
			} else {
				strob_strcat(totallinkbuf, "/");
				strob_strcat(totallinkbuf, strob_str(linkbuf));
			}
			path = strob_str(totallinkbuf);
		} else {
			break;
		}
	}

	if (linkbuf) strob_close(linkbuf);
	if (totallinkbuf) strob_close(totallinkbuf);

	if (i >= swvarfs->n_loop_symlinksM) {
		fprintf(stderr,"%s: getnext_dirent: recursive %s failed on %s\n",
				swlib_utilname_get(), swvarfs_get_stat_syscall(swvarfs), fp_path);
			swvarfs->derrM = -2;
		return -2;
	}

	taru_statbuf2filehdr(ahs_vfile_hdr(swvarfs->ahsM), statbuf,
							 NULL, NULL, NULL);
	ahsStaticSetTarFilename(ahs_vfile_hdr(swvarfs->ahsM), fp_path);
	if (S_ISLNK(statbuf->st_mode)) {
		set_linkname(swvarfs, -1, fp_path);
	} else {
		ahsStaticSetPaxLinkname(ahs_vfile_hdr(swvarfs->ahsM), NULL);
	}
	SWVARFS_E_DEBUG("END"); 
	return 0;
}

static
void
delete_last_component(SWVARFS * swvarfs, char * path) {
	char *s;
	int index_of_slash = strob_strlen(swvarfs->dirscopeM) - 2;

	strob_str(swvarfs->dirscopeM)[index_of_slash] = '\0';
	s = strstr(path, strob_str(swvarfs->dirscopeM));
	strob_str(swvarfs->dirscopeM)[index_of_slash] = '/';
	if (s == (char*)(NULL)) {
		SWBIS_E_FAIL3("delete_last_component path=[%s] dirscope=[%s]",
				path, strob_str(swvarfs->dirscopeM));
		return;
	}

	s += (strob_strlen(swvarfs->dirscopeM) - 2);

	if (strcmp(s, "/") == 0) {
		return;
	}

	s =  strrchr(s, '/');
	if (s) {
		*(s) = '\0';
	}
	return;
}

static
void
get_dir_context(SWVARFS * swvarfs, struct dirContext * dirx)
{
	int i = swvarfs->stackixM;
	unsigned char * s = (unsigned char*)strob_str(swvarfs->stackM);
	struct dirContext *ds = 
		(struct dirContext*)(s + (i * sizeof(struct dirContext)));
	*dirx = *ds;
}

static
void
add_dir_context(SWVARFS * swvarfs, struct dirContext * dirx)
{
	int i = ++(swvarfs->stackixM);
	unsigned char * s;
	struct dirContext *ds;
	
	strob_set_memlength(swvarfs->stackM, 
				((i+1) * sizeof(struct dirContext)));
	s = (unsigned char*)strob_str(swvarfs->stackM);
	ds = (struct dirContext*)(s + (i * sizeof(struct dirContext)));
	*ds = *dirx;
}

static
void
raise_dir_context(SWVARFS * swvarfs, struct dirContext * dirx)
{
	struct dirContext ds;
	get_dir_context(swvarfs, &ds);
	if (ds.dp != NULL) closedir(ds.dp);
	swvarfs->stackixM--;
	if (!swvarfs->stackixM) {
		dirx->dp = NULL;
	} else {
		get_dir_context(swvarfs, dirx);
	}
}

static
char *
swvarfs_get_next_dirent_fs(SWVARFS * swvarfs, struct stat *st)
{
	struct stat sto;
	struct stat *statbuf;
	struct dirent *dirp;
	DIR *dp=NULL, *olddp;
	char * fullpath = strob_str(swvarfs->direntpathM);
	struct dirContext dirx;	

	SWVARFS_E_DEBUG("ENTERING"); 
	if (st == NULL) {
		statbuf=&sto;
	} else {
		statbuf=st;
	}

	if (swvarfs->stackixM) {  /* The 0th index is the not-used index. */
		SWVARFS_E_DEBUG(""); 
		get_dir_context(swvarfs, &dirx);
 		dirp = dirx.dirp;
		dp = dirx.dp;
		/*
		 * dp may be NULL due a directory that could not be accessed.
		 */
		while(dp == NULL) {
			delete_last_component(swvarfs, fullpath);
			raise_dir_context(swvarfs, &dirx);
 			dirp = dirx.dirp;
			dp = dirx.dp;
		}
		delete_last_component(swvarfs, fullpath);
	} else {
		/* Open the root (given) directory.
		 */
		SWVARFS_E_DEBUG(""); 

		swvarfs_setdir(swvarfs, fullpath);

		get_dir_context(swvarfs, &dirx);
		dp = dirx.dp;
		stat_node(swvarfs, fullpath, statbuf);
		return fullpath;
	}


	do {	
		while ( (dirp = readdir(dp)) != NULL) {
			SWVARFS_E_DEBUG(""); 
			if (strcmp(dirp->d_name, ".") == 0  ||
				strcmp(dirp->d_name, "..") == 0) {
				continue;
			}
			SWVARFS_E_DEBUG(""); 
			strob_strcat(swvarfs->direntpathM, "/");
			strob_strcat(swvarfs->direntpathM, dirp->d_name);
			fullpath = strob_str(swvarfs->direntpathM);
			stat_node(swvarfs, fullpath, statbuf);

			SWVARFS_E_DEBUG(""); 
			if (S_ISDIR(statbuf->st_mode)) {
						/* is a directory */
				olddp = dp;
				if ( (dp = opendir(fullpath)) == NULL) {
					if (errno == EACCES) {
						fprintf(stderr,
						"Permission denied: %s\n",
						fullpath);
					} else {
						SWVARFS_E_DEBUG2("internal error -3: [%s]", fullpath);
						swvarfs->derrM = -3;
						return (char*)(NULL);
					}
				}
				dirx.dp = dp;
				add_dir_context(swvarfs, &dirx);
				strob_strcat(swvarfs->direntpathM, "/");
				fullpath = strob_str(swvarfs->direntpathM);
			}
			swvarfs->derrM = 0;
			return fullpath;	
			break;
		}
		SWVARFS_E_DEBUG(""); 
		if (dirp == NULL) {  /* reach last entry */
			raise_dir_context(swvarfs, &dirx);
			delete_last_component(swvarfs, fullpath);
			dp = dirx.dp;
		}
	} while (dp);
	SWVARFS_E_DEBUG("LEAVING"); 
	swvarfs->derrM = 0;
	return (char*)(NULL);
}

static
int 
do_stop_reading(void * xx, char * member_name, char * name)
{
	int ret;
	int did_squash=0;
	int did_squash1=0;
	int did_squash2=0;
	SWVARFS * swvarfs = (SWVARFS*)xx;

	/*
	 * If name is NULL, then return next.
	 */
	
	SWVARFS_E_DEBUG2("ENTERING [%s]", name); 
	if (strncmp(member_name, "./", 2) == 0) {
		member_name += 2;
	}

	if (!name) {
		return 0;
	}

	if (strncmp(name, "./", 2) == 0) {
		name += 2;
	}

	handle_trailing_slash("drop", name, &did_squash);
	handle_trailing_slash("drop", member_name, &did_squash1);
	handle_leading_slash(swvarfs, "drop", name, &did_squash2);

	SWVARFS_E_DEBUG3("strcmp/fnmatch pattern=[%s] name=[%s]", name, member_name); 
	/* ret = fnmatch(name, member_name, 0); */
	ret = strcmp(name, member_name) ? 1 : 0;
	SWVARFS_E_DEBUG2("strcmp/fnmatch returned [%d]", ret); 
	handle_leading_slash(swvarfs, "restore", name, &did_squash2);
	handle_trailing_slash("restore", name, &did_squash);
	handle_trailing_slash("restore", member_name, &did_squash1);

	SWVARFS_E_DEBUG("LEAVING"); 
	return ! ret;
}

static
void
swvarfs_i_init(SWVARFS * swvarfs)
{
	/*
			Cannot set this NULL here!!  
			swvarfs->format_descM = (UINFORMAT*)NULL; 
	*/
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->uinformat_close_on_deleteM = 1;
	swvarfs->opencwdpathM = strob_open(32);
	swvarfs->openpathM = strob_open(32);
	swvarfs->dirscopeM = strob_open(32);
	swvarfs->tmpM = strob_open(128);
	swvarfs->vcwdM = strob_open(128);
	strob_strcpy(swvarfs->opencwdpathM, "");
	strob_strcpy(swvarfs->openpathM, "");
	swvarfs->u_nameM = strob_open(32);
	swvarfs->ahsM = ahs_open();
	swvarfs->do_close_ahsM = 1;
	swvarfs->headM=NULL;
	swvarfs->tailM=NULL;
	swvarfs->g_linkname_ = NULL;
	swvarfs->u_current_name_ = NULL;
	swvarfs->u_fdM=-1;
	swvarfs->formatM= 0;
	swvarfs->f_do_stop_=do_stop_reading;
	swvarfs->stackM = strob_open(16);
	swvarfs->direntpathM = strob_open(16);
	swvarfs->stackixM = 0;
	swvarfs->derrM = 0;
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->have_read_filesM = 0;
	swvarfs->current_filechainM = NULL;
	swvarfs->uxfio_buftype = UXFIO_BUFTYPE_FILE;
	swvarfs->current_data_offsetM = 0;
	swvarfs->is_unix_pipeM = 0;
	swvarfs->current_data_offsetM = -1;
	swvarfs->current_header_offsetM = -1;
	swvarfs->makefilechainM = 0;
	swvarfs->eoaM = 0;
	swvarfs->has_leading_slashM = 0;
	swvarfs->did_u_openM = -1;
	ud_init(swvarfs);
	swvarfs->f_statM = (int (*)(char *, struct stat *))(lstat);
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->link_recordM = hllist_open();
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->taruM = taru_create();
	swvarfs->taruM->taru_tarheaderflagsM = 0;
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->n_loop_symlinksM = 20;
}

static
int
swvarfs_i_u_open_fs(SWVARFS * swvarfs, char * name)
{
	int fd;
	int ret = 0;
	struct stat st;
	int u_index;

	SWVARFS_E_DEBUG2("ENTERING [%s]", name); 
	if (!name) {
		/* 
		* Get next Dirent 
		*/
		if ((ret=swvarfs_dirent_err(swvarfs))) {
			SWVARFS_E_DEBUG("Internal error status returned from swvarfs_get_next_dirent");
			return -2;
		}
		name = swvarfs_get_next_dirent(swvarfs, NULL);
		if (!name && !swvarfs_dirent_err(swvarfs)) {
			/* No more files */
			return 0;
		}
		if (swvarfs_dirent_err(swvarfs)) {
			SWVARFS_E_DEBUG("Internal error status returned from swvarfs_get_next_dirent");
			return -2;
		}
	}
	if (*name != '/') {
		/*
		* Relative path.
		*/

		if ( swvarfs->stackixM == 0 && 
			is_name_a_path_prefix(strob_str(swvarfs->direntpathM),
					strob_str(swvarfs->openpathM)) == 0)
			{
			/*
			* Hack.
			* The get_next_dirent function is not in use 
			* and swvarfs_setdir has not been called.
			*/
			strob_strcpy(swvarfs->u_nameM, 
					strob_str(swvarfs->openpathM));
		} else {
			strob_strcpy(swvarfs->u_nameM, "");
		}
		swlib_unix_dircat(swvarfs->u_nameM, name);
	} else {
		/*
		* Absolute path.
		*/
		strob_strcpy(swvarfs->u_nameM, name);
	}

	fd=uxfio_open(strob_str(swvarfs->u_nameM), O_RDONLY, (mode_t)(0));
	if (swvarfs->u_current_name_) swbis_free(swvarfs->u_current_name_);
	swvarfs->u_current_name_ = swlib_strdup(strob_str(swvarfs->u_nameM));        /* CNS */
	
	if (fd < 0) {
		ahsStaticSetTarFilename(ahs_vfile_hdr(swvarfs->ahsM),
						strob_str(swvarfs->u_nameM));
		return -1;
	}
	SWLIB_ASSERT(ud_set_fd(swvarfs, fd) >= 0);
	u_index = ud_find_fd(swvarfs, fd);
	SWLIB_ASSERT(u_index >= 0);


	swvarfs_uxfio_fcntl(swvarfs, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_NOBUF);
	if((*(swvarfs->f_statM))(strob_str(swvarfs->u_nameM), &st)) return -2;

	ahsStaticSetTarFilename(ahs_vfile_hdr(swvarfs->ahsM),
					strob_str(swvarfs->u_nameM));

	if (swvarfs->u_fd_setM[u_index].u_current_name_)
			swbis_free(swvarfs->u_fd_setM[u_index].u_current_name_);
	swvarfs->u_fd_setM[u_index].u_current_name_ = 
				swlib_strdup(strob_str(swvarfs->u_nameM));

	if (S_ISLNK(st.st_mode)) {
		set_linkname(swvarfs, fd, strob_str(swvarfs->u_nameM));
	}
	taru_statbuf2filehdr(ahs_vfile_hdr(swvarfs->ahsM), &st, NULL,
			/*ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM))*/ strob_str(swvarfs->u_nameM), 
			/*ahsStaticGetTarLinkname(ahs_vfile_hdr(swvarfs->ahsM))*/ NULL );
	uxfio_ioctl(fd, UXFIO_IOCTL_SET_STATBUF, (void*)&st);
	swvarfs->u_fdM = fd;
	return fd;
}

static
int
swvarfs_i_u_lstat_fs(SWVARFS * swvarfs, char * path, struct stat * st)
{
	int ret;
	SWVARFS_E_DEBUG("ENTERING");
	if (*path != '/') {
		/* ??? FIXME What should the policy be.*/
		strob_strcpy(swvarfs->u_nameM, ""); 
		strob_strcat(swvarfs->u_nameM, path);
	} else {
		strob_strcpy(swvarfs->u_nameM, path);
	}
	ret = (*(swvarfs->f_statM))(strob_str(swvarfs->u_nameM), st);
	SWVARFS_E_DEBUG("LEAVING");
	return ret;
}


static int
swvarfs_i_u_lstat_archive(SWVARFS * swvarfs, char * path, struct stat * st)
{
	int fd;
	SWVARFS_E_DEBUG("ENTERING");
	fd = swvarfs_u_open(swvarfs, path);
	if (fd < 0) {
		return -3;
	}

	if (swvarfs_file_has_data(swvarfs)) {
		taru_pump_amount2(-1, fd, -1, -1);
	}
	if (st) if (swvarfs_u_fstat(swvarfs, fd, st)) return -2;
	swvarfs_u_close(swvarfs, fd);
	SWVARFS_E_DEBUG("LEAVING");
	return 0;
}

static
char *
swvarfs_get_next_dirent_archive_NEW(SWVARFS * swvarfs, 
				struct stat *st,
				int *peoa)
{
	char * name;
	intmax_t old_pos;
	intmax_t new_pos;
	*peoa = 0;

	if (swvarfs->did_u_openM == 0 && swvarfs->u_current_name_) {
		/*
		* Open the file.
		*/
		swvarfs_i_u_lstat_archive(swvarfs,
				swvarfs->u_current_name_, NULL);
	}

	SWVARFS_E_DEBUG("ENTERING");
	SWVARFS_E_DEBUG2("OFFSET = %d", (int)uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR));

	old_pos = (intmax_t)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR);
	swvarfs->current_header_offsetM = uxfio_lseek(swvarfs->fdM, 0, SEEK_CUR);

	if (taru_read_header(swvarfs->taruM,
				ahs_vfile_hdr(swvarfs->ahsM),
				swvarfs->fdM,
				swvarfs->formatM,
				peoa,
				swvarfs->taruM->taru_tarheaderflagsM) < 0) {
		SWBIS_E_FAIL("header read error");
		swvarfs->derrM = -3;
		return NULL;
	}
	if (*peoa) {
		return (char*)NULL;
	}

	/*
	* save this offset.
	*
	* Use of the UXFIO_SEEK_VCUR whence value returns
	* the byte offset in the file (as if it were a regular file
	* even if it is a pipe).
	*/
	swvarfs->current_data_offsetM = uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);

	/*
	* seek back to the start of the header because
	* the <>_u_open routine reads the header again.
	*/

	new_pos = swvarfs->current_data_offsetM;
	SWVARFS_E_DEBUG3("swvarfs->fdM = [%d] old_pos = %d ", swvarfs->fdM, old_pos); 
	SWVARFS_E_DEBUG3("swvarfs->fdM = [%d] new_pos = %d ", swvarfs->fdM, new_pos); 
	SWVARFS_E_DEBUG3("swvarfs->fdM = [%d] old_pos - new_pos = %d ", swvarfs->fdM, old_pos - new_pos); 
	if (uxfio_lseek(swvarfs->fdM, (off_t)(old_pos - new_pos), UXFIO_SEEK_VCUR) < 0) {
		SWBIS_E_FAIL("swvarfs_get_next_dirent_archive: seek error");
		exit(88);
		return NULL;
	}

	if (st) {
		taru_filehdr2statbuf(st, ahs_vfile_hdr(swvarfs->ahsM));
	}
	name = ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM));
	if (swvarfs->u_current_name_) {
		swbis_free(swvarfs->u_current_name_);
		swvarfs->u_current_name_ = NULL;
	}
	/*
	* Check for out of scope.
	*/
	if (is_name_in_scope(strob_str(swvarfs->dirscopeM), name, 
					(struct stat *)NULL) == 0) {
		/*
		* Out of scope.
		*/
		SWVARFS_E_DEBUG2("SCOPE REJECT HERE name = %s", name); 
		u_reject(swvarfs);
		return (char*)NULL;
	}

	if (swvarfs->u_current_name_) swbis_free(swvarfs->u_current_name_);
	swvarfs->u_current_name_ = swlib_strdup(name);					/* CNS */
	swvarfs->did_u_openM = 0;
	return name;
}

static
struct fileChain * 
swvarfs_i_make_link(struct new_cpio_header * file_hdr,
				intmax_t header_offset, intmax_t data_offset) {
	struct fileChain * link = (struct fileChain*)malloc
						(sizeof(struct fileChain));
	
	SWVARFS_E_DEBUG("ENTERING"); 
	if (link == NULL) return NULL;
	link->nameFC = swlib_strdup(ahsStaticGetTarFilename(file_hdr));
	swlib_slashclean(link->nameFC);
	swlib_process_hex_escapes(link->nameFC);  /* Collapse Hex sequences into single non-ascii octet */
	SWVARFS_E_DEBUG3("link made name=[%s] header_offset = [%s]", link->nameFC, swlib_imaxtostr(header_offset, NULL)); 
	SWVARFS_E_DEBUG3("link made name=[%s]   data_offset = [%s]", link->nameFC, swlib_imaxtostr(data_offset, NULL)); 
	link->header_offsetFC = header_offset;
	link->data_offsetFC = data_offset;
	link->prevFC=link->nextFC = NULL;
	link->uxfio_u_fdFC = -1;
	return link;
}

static
struct fileChain * 
swvarfs_i_readin_filechain(SWVARFS * swvarfs, char * name)
{
	struct fileChain * last = swvarfs->tailM, *new_link;
	intmax_t header_offset;
	intmax_t data_offset;
	
	SWVARFS_E_DEBUG2("ENTERING [%s]", name); 
	if (last) {
		SWVARFS_E_DEBUG2("OFFSET BEFORE= %d", (int)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR));
	
		uxfio_lseek(swvarfs->fdM, (off_t)(last->header_offsetFC), SEEK_SET);
	
		SWVARFS_E_DEBUG2("OFFSET AFTER= %d", (int)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR));
	
		header_offset = uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);

		if (taru_read_header(
				swvarfs->taruM,
				ahs_vfile_hdr(swvarfs->ahsM),
				swvarfs->fdM,
				swvarfs->formatM,
				 NULL,
				 swvarfs->taruM->taru_tarheaderflagsM) < 0) {
			SWBIS_E_FAIL("header read error");
			return NULL;
		}
		SWVARFS_E_DEBUG2("I-section AFTER taru_read_header name=[%s]", ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM)));
		swvarfs->current_data_offsetM = uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);
	
		data_offset = uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);
	
		if ((ahs_vfile_hdr(swvarfs->ahsM)->c_mode & CP_IFMT) == 
								CP_IFREG) {
			/*
			* Dump the data
			*/
			if (
				taru_write_archive_member_data
					(swvarfs->taruM,
					ahs_vfile_hdr(swvarfs->ahsM),
					-1,
					swvarfs->fdM,
					(int(*)(int))NULL, 
					uinfile_get_type(swvarfs->format_descM),
					 -1, NULL) < 0
			) {
				SWBIS_E_FAIL2("data error %s\n", ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM)));
				return NULL;
			}
			taru_tape_skip_padding(
				swvarfs->fdM,
				ahs_vfile_hdr(swvarfs->ahsM)->c_filesize, 
				uinfile_get_type(swvarfs->format_descM));
		} else {
			data_offset=-1;
		}
	} else {
		uxfio_lseek(swvarfs->fdM, 0, SEEK_SET);
	}
		
	do{
		header_offset = uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);
		
		SWVARFS_E_DEBUG3("readin name = [%s]  header offset = %d", name, (int)header_offset);
	
		if (taru_read_header(swvarfs->taruM, 
				ahs_vfile_hdr(swvarfs->ahsM),
				swvarfs->fdM,
				swvarfs->formatM,
				NULL,
				swvarfs->taruM->taru_tarheaderflagsM) < 0){
			new_link=NULL;
			break;
		}

		swvarfs->current_data_offsetM = (int)uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);
		
		SWVARFS_E_DEBUG2("Loop-section AFTER taru_read_header name=[%s]", ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM)));

		if (!strcmp(
			ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM)),
			"TRAILER!!!")
		) {
			new_link=NULL;
			break;
		}
		
		data_offset=uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);
		if ((ahs_vfile_hdr(swvarfs->ahsM)->c_mode & CP_IFMT) ==
				CP_IFREG) {
			/*
			* Dump the data
			*/
			if (taru_write_archive_member_data(swvarfs->taruM,
					ahs_vfile_hdr(swvarfs->ahsM),
					-1,
					swvarfs->fdM,
					(int(*)(int))NULL, 
					uinfile_get_type(swvarfs->format_descM),
					 -1, NULL) < 0){
				SWBIS_E_FAIL2("data error %s\n", ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM)));
				return NULL;
			}
			taru_tape_skip_padding(swvarfs->fdM,
				ahs_vfile_hdr(swvarfs->ahsM)->c_filesize, 
				uinfile_get_type(swvarfs->format_descM));
		} else {
			data_offset=-1;
		}
		new_link=swvarfs_i_make_link(ahs_vfile_hdr(swvarfs->ahsM),
						header_offset, data_offset);
		swvarfs_i_attach_filechain(swvarfs, new_link);
	} while(do_stop_reading((void*)swvarfs,
			ahsStaticGetTarFilename(ahs_vfile_hdr(swvarfs->ahsM)),
					name) == 0);
	SWVARFS_E_DEBUG("LEAVING");
	return new_link;
}

static
struct fileChain * 
swvarfs_i_search_filechain(SWVARFS * swvarfs, char * name_p,
						int * scope_rejection)
{
	int did_drop = 0;
	int did_omit_lslash = 0;
	char * name;
	char * p;
	struct fileChain *last=swvarfs->headM;

	name = name_p;
	if (!name) return NULL;
	strob_strcpy(swvarfs->tmpM, name);
	name = strob_str(swvarfs->tmpM);
	swlib_process_hex_escapes(name);

	SWVARFS_E_DEBUG2("ENTERING [%s]", name); 
	*scope_rejection = 0;
	
	while(last) {
		p = last->nameFC;
		
		/*
		 * FIXME: name must have ascii hex sequence converted to non-ascii chars
		 */

		if (strncmp(p, "./", 2) == 0) {
			p += 2;
		}

		if (strcmp(name, "./") == 0) {
			name[1] = '\0';
		} else if (strncmp(name, "./", 2) == 0) {
			name += 2;
		}

		handle_trailing_slash("drop", name, &did_drop);
		handle_leading_slash(swvarfs, "drop", name, &did_omit_lslash);
		SWVARFS_E_DEBUG3("strcmp/fnmatch pattern=[%s] name=[%s]", name, p);
		/* if (fnmatch(name, p, 0) == 0) { */
		if (strcmp(name, p) == 0) {
			/*
			* Its a match, but it may be out of scope.
			*/
			SWVARFS_E_DEBUG3("Got match strcmp/fnmatch pattern=[%s] name=[%s]", name, p); 
			if (is_name_in_scope(strob_str(swvarfs->dirscopeM),
						name, (struct stat *)NULL)) {
				handle_trailing_slash("restore", name, 
								&did_drop);
				handle_leading_slash(swvarfs, "restore",
							name, &did_omit_lslash);
				SWVARFS_E_DEBUG("LEAVING Found in scope OK"); 
				return last;
			} else {
				*scope_rejection = 1;
				handle_trailing_slash("restore", name,
								&did_drop);
				handle_leading_slash(swvarfs,
					"restore", name, &did_omit_lslash);
				SWVARFS_E_DEBUG("LEAVING returning null, out of scope"); 
				return NULL;
			}
		}
		handle_trailing_slash("restore", name, &did_drop);
		handle_leading_slash(swvarfs, "restore", 
						name, &did_omit_lslash);
		last=last->nextFC;	
	}
	SWVARFS_E_DEBUG("LEAVING"); 
	return NULL;
}

static int
swvarfs_i_u_open_archive_seek_to_name(SWVARFS * swvarfs, char * name)
{
	int tret;
	intmax_t header_offset = 0;
	intmax_t data_offset;
	int * peoa = &(swvarfs->eoaM);
	struct fileChain * new_link;
	struct fileChain * existing = NULL;
	char * current_name = swvarfs->u_current_name_;
	int scope_rejection;

	SWVARFS_E_DEBUG2("ENTERING [%s]", name); 

	if (current_name == (char*)NULL) {
		/*
		* Peek at the archive.
		* This case is followed for a user specified traversal.
		*/
		SWVARFS_E_DEBUG("current name is NULL"); 

		/*
		* Check the  file chain to see if this file has beed read.
		*/
		scope_rejection = 0;
		if ((existing=swvarfs_i_search_filechain(swvarfs,
					name, &scope_rejection)) == NULL) {
			/*
			* Not already read.
			* Make one peek at the archive.
			*/
			if (scope_rejection) {
				/*
				* Normal, file out of scope.
				*/
				SWVARFS_E_DEBUG2("SCOPE REJECT HERE name = %s", name); 
				u_reject(swvarfs);
				return -1;
			}
			current_name = swvarfs_get_next_dirent_archive_NEW(
					swvarfs, 
						(struct stat *)(NULL),
							peoa);
		} else {
			/*
			* Found.
			*/
			SWVARFS_E_DEBUG("HERE"); 
			swvarfs->current_filechainM = existing;
			current_name = existing->nameFC;	
		}
	}
	SWVARFS_E_DEBUG3("name = [%s] peoa=%d", current_name, *peoa);
	
	/*if (*peoa != 0) return 0; */

	if (current_name && 
		!filename_does_match(name, current_name) &&
				existing == NULL) {
		/*
		* special case for inorder traversal.
		*/
		SWVARFS_E_DEBUG3("NAME MATCH [%s] offset = %d", name, (int)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR));
		/*
		* Nothing to do.
		*/
		if (is_name_in_scope(strob_str(swvarfs->dirscopeM),
				current_name, (struct stat *)NULL) == 0) {
			/*
			* Out of scope.
			*/
			SWVARFS_E_DEBUG2("SCOPE REJECT HERE name = %s", current_name); 
			u_reject(swvarfs);
			return -1;
		} else {
			swvarfs->current_filechainM = existing; /* i.e. NULL */
		}
	} else {
		/*
		* Have to seek in the archive to find the specified file.
		*/
		if (check_seek_violation(swvarfs)) {
			SWVARFS_E_DEBUG("LEAVING");
			return -3;
		}
		SWVARFS_E_DEBUG("NOT a match"); 

		if ( (existing=swvarfs_i_search_filechain(swvarfs, name, 
						&scope_rejection)) == NULL) {
			scope_rejection = 0;
			if (scope_rejection || 
				((existing=swvarfs_i_readin_filechain(swvarfs,
							name)) == NULL)
			) {
				/*
				* Normal, file not found, or, out of scope.
				*/
				SWVARFS_E_DEBUG2("REJECT HERE name = %s", name); 
				u_reject(swvarfs);
				SWVARFS_E_DEBUG("LEAVING");
				return -1;
			} else {
				/*
				* Check for out of scope.
				*/
				if (is_name_in_scope(
					strob_str(swvarfs->dirscopeM), 
					name, 
					(struct stat *)NULL) == 0) {
					/*
					* Out of scope.
					*/
					SWVARFS_E_DEBUG2("SCOPE REJECT HERE name = %s", name); 
					u_reject(swvarfs);
					SWVARFS_E_DEBUG("LEAVING");
					return -1;
				}
			}
		}
		swvarfs->current_filechainM = existing;
		SWVARFS_E_DEBUG2("OFFSET BEFORE = %d", (int)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR));
		SWVARFS_E_DEBUG2("header_offsetFC is = %d", (int)(existing->header_offsetFC));

		tret = uxfio_lseek(swvarfs->fdM,
				(off_t)(existing->header_offsetFC), SEEK_SET);
		if (tret != existing->header_offsetFC) {
			SWBIS_E_FAIL3("seek error tret = %d header_offsetM = %d", tret, existing->header_offsetFC);
			u_reject(swvarfs);
			SWVARFS_E_DEBUG("LEAVING");
			return -2;
		}
	
		SWVARFS_E_DEBUG2("OFFSET AFTER = %d", (int)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR));
	}

	/*
	* read the header.
	*/
	
	if (swvarfs->makefilechainM && swvarfs->current_filechainM == NULL) {
		SWVARFS_E_DEBUG("");
		header_offset = (int)uxfio_lseek(swvarfs->fdM, 
						0, 
						UXFIO_SEEK_VCUR);
	}

	SWVARFS_E_DEBUG("about to read the archive header");
	SWVARFS_E_DEBUG2("offset in swvarfs->fdM is %d", (int)uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR));
	if (taru_read_header(swvarfs->taruM, 
			ahs_vfile_hdr(swvarfs->ahsM),
				swvarfs->fdM,
				swvarfs->formatM,
				peoa,
				swvarfs->taruM->taru_tarheaderflagsM) < 0) {
		SWBIS_E_FAIL("header read error");
		if (swvarfs->u_current_name_)
				swbis_free(swvarfs->u_current_name_);
		swvarfs->u_current_name_ = (char*)NULL;
		SWVARFS_E_DEBUG("LEAVING");
		return -3;
	}
	
	swvarfs->current_data_offsetM = uxfio_lseek(swvarfs->fdM, 0, SEEK_CUR);
	SWVARFS_E_DEBUG2("swvarfs->current_data_offsetM = [%s]", swlib_imaxtostr(swvarfs->current_data_offsetM, NULL)); 

	if (swvarfs->makefilechainM && swvarfs->current_filechainM == NULL) {
		data_offset = uxfio_lseek(swvarfs->fdM, 0, UXFIO_SEEK_VCUR);
		new_link=swvarfs_i_make_link(ahs_vfile_hdr(swvarfs->ahsM),
						header_offset, data_offset);
		swvarfs_i_attach_filechain(swvarfs, new_link);
		swvarfs->current_filechainM = new_link;
	}

	SWVARFS_E_DEBUG("LEAVING");
	return 0;
}

static int
swvarfs_i_u_open_archive_bh(SWVARFS * swvarfs, 
				struct new_cpio_header * file_hdr)
{
	int fd;
	struct stat st;
	int u_index;

	SWVARFS_E_DEBUG2("ENTERING [%p]", file_hdr); 
	if (ud_find_fd(swvarfs, -1) < 0) {
		fprintf(stderr, "too many open swvarfs:u_fd descriptors.\n");
		SWVARFS_E_DEBUG("return -4");
		return -4;
	}

	fd = uxfio_opendup(swvarfs->fdM, UXFIO_BUFTYPE_NOBUF);
	if (fd < 0) {
		SWVARFS_E_DEBUG("return -5");
		return -5;
	}
	
	SWLIB_ASSERT(ud_set_fd(swvarfs, fd) >= 0);
	u_index = ud_find_fd(swvarfs, fd);
	SWLIB_ASSERT(u_index >= 0);
	
	taru_filehdr2statbuf(&st, file_hdr);	
	if (strlen(ahsStaticGetTarFilename(file_hdr))) {
		if (swvarfs->u_current_name_)
				swbis_free(swvarfs->u_current_name_);
		swvarfs->u_current_name_ = 
			swlib_strdup(ahsStaticGetTarFilename(file_hdr));
							/* CNS */
		if (swvarfs->u_fd_setM[u_index].u_current_name_)
			swbis_free(swvarfs->u_fd_setM[u_index].u_current_name_);
		swvarfs->u_fd_setM[u_index].u_current_name_ = 
			swlib_strdup(ahsStaticGetTarFilename(file_hdr));
	} else {
		if (swvarfs->u_current_name_)
				swbis_free(swvarfs->u_current_name_);
		swvarfs->u_current_name_= NULL;
		if (swvarfs->u_fd_setM[u_index].u_current_name_)
			swbis_free(swvarfs->u_fd_setM[u_index].u_current_name_);
		swvarfs->u_fd_setM[u_index].u_current_name_ = NULL;
	}

	if (swvarfs->g_linkname_){ 
		swbis_free(swvarfs->g_linkname_); 
		swvarfs->g_linkname_ = NULL; 
	}
	if (strlen(ahsStaticGetTarLinkname(file_hdr))) {
		swvarfs->g_linkname_ = 
			swlib_strdup(ahsStaticGetTarLinkname(file_hdr));
		swvarfs->u_fd_setM[u_index].u_linkname_ =
			swlib_strdup(ahsStaticGetTarLinkname(file_hdr));
	}
	uxfio_ioctl(fd, UXFIO_IOCTL_SET_STATBUF, (void*)&st);
	swvarfs->u_fdM = fd;
	swvarfs->u_fd_setM[u_index].u_fdM = fd;

	SWVARFS_E_DEBUG("LEAVING");
	SWVARFS_E_DEBUG2("return at last with fd=%d", fd);
	return fd;
}

static
int
swvarfs_i_u_open_archive(SWVARFS * swvarfs, char * name)
{
	int ret;
	int fd;

	SWVARFS_E_DEBUG2("ENTERING [%s]", name); 
	swvarfs->did_u_openM = 1;

	ret = swvarfs_i_u_open_archive_seek_to_name(swvarfs, name);
	if (ret) {
		SWVARFS_E_DEBUG2("error ret=%d", ret);
		return ret;
	}
	if (swvarfs->eoaM) {
		SWVARFS_E_DEBUG("End of archive");
		return -1;
	}
	fd = swvarfs_i_u_open_archive_bh(swvarfs,
				ahs_vfile_hdr(swvarfs->ahsM));
	SWVARFS_E_DEBUG("LEAVING");

	return fd;
}

static
int
swvarfs_i_iopen(SWVARFS * swvarfs, int oflags)
{
	SWVARFS_E_DEBUG2("ENTERING oflags=[%d]", oflags); 
	SWVARFS_E_DEBUG2("ENTERING swvarfs->fdM = [%d]", swvarfs->fdM); 
	if (swvarfs->fdM >= UXFIO_FD_MIN){
		/*
		* It's a uxfio descriptor.
		*/
		if (uxfio_lseek(swvarfs->fdM, (off_t)0, SEEK_SET) != 0) {
			SWBIS_E_FAIL("error in initial uxfio_lseek");
			swvarfs->fdM=-1;
			return -1;
		}
		if (uxfio_espipe(swvarfs->fdM)) {
			int current_buftype;
			/*
			* Its a pipe, need to turn on the window buffering.
			* uxfio_fcntl(swvarfs->fdM, UXFIO_F_GET_BUFTYPE, 0);
			*/
			current_buftype = uxfio_fcntl(swvarfs->fdM, 
						UXFIO_F_GET_BUFTYPE, 0);
			if (
				current_buftype != UXFIO_BUFTYPE_DYNAMIC_MEM &&
				current_buftype != UXFIO_BUFTYPE_MEM && 
				current_buftype != UXFIO_BUFTYPE_FILE 
			) {
				SWVARFS_E_DEBUG("set buftype DYNAMIC_MEM"); 
				swvarfs->is_unix_pipeM = 1;
				swvarfs_uxfio_fcntl(swvarfs,
						UXFIO_F_SET_BUFTYPE,
						UXFIO_BUFTYPE_DYNAMIC_MEM);
				swvarfs_uxfio_fcntl(swvarfs,
						UXFIO_F_SET_BUFACTIVE,
						UXFIO_ON);
			}
		} else {
			/*
			* Its not a pipe, turn buffering off.
			*/
			swvarfs->is_unix_pipeM = 0;
			/*
			uxfio_fcntl(swvarfs->fdM,
					UXFIO_F_SET_BUFACTIVE,
					UXFIO_OFF);
			*/
		}
	}	
	return 0;
}

static
int 
swvarfs_i_u_close_archive(SWVARFS * swvarfs, int fd)
{
	int ret;
	int u_index;
	SWVARFS_E_DEBUG2("ENTERING [%d]", fd); 
	SWVARFS_E_DEBUG3("u_fdM=%d  fdM=%d", swvarfs->u_fdM, swvarfs->fdM); 


	if ((u_index = ud_find_fd(swvarfs, fd)) < 0) {
		fprintf(stderr,"close error, desc %d not found.\n", fd);
		return -1;
	}

	if (fd != swvarfs->u_fdM && fd != swvarfs->u_fd_setM[u_index].u_fdM) {
		/*
		* Usage error.
		*/
		fprintf(stderr,"close error, desc %d not found.\n", fd);
		return -1;
	}

	if ((ahs_vfile_hdr(swvarfs->ahsM)->c_mode & CP_IFMT) != CP_IFLNK) {
		SWVARFS_E_DEBUG("(ahs_vfile_hdr(swvarfs->ahsM)->c_mode & CP_IFMT) != CP_IFLNK is true."); 
		/*
		* Hack, Test above is a hack to fix problem with
		* sym link in cpio formats.
		*/
		if (swvarfs->uxfio_buftype != UXFIO_BUFTYPE_NOBUF) {
			SWVARFS_E_DEBUG("swvarfs->uxfio_buftype != UXFIO_BUFTYPE_NOBUF"); 
			/* SWVARFS_E_DEBUG2("uxfio dump %s", uxfio_dump_string(fd)); */
			ret = uxfio_lseek(fd, (off_t)0, SEEK_END);
			if (ret < 0) {
				fprintf(stderr, "%s: swvarfs_i_u_close_archive: uxfio_lseek error\n", swlib_utilname_get());
				return -1;
			}
			/* SWVARFS_E_DEBUG2("uxfio dump %s", uxfio_dump_string(fd)); */
			SWVARFS_E_DEBUG(""); 
		}
	}

	/* SWVARFS_E_DEBUG2("uxfio dump on swvarfs->fdM %s", uxfio_dump_string(swvarfs->fdM)); */
	if ((ahs_vfile_hdr(swvarfs->ahsM)->c_mode & CP_IFMT) == CP_IFREG) {
		/*
		* We assume the data has already been read by
		* the user's program.
		*/
		SWVARFS_E_DEBUG2("SKIP running taru_tape_skip_padding name=%s.", swvarfs->u_current_name_ );
		taru_tape_skip_padding(swvarfs->fdM,
				ahs_vfile_hdr(swvarfs->ahsM)->c_filesize,
					swvarfs->formatM);
	} else {
		/* Not a regular file, i.e. a file with a data portion. */
		/*
		* Need to skip over the header [and padding].
		*/
		int current_pos;
		current_pos = (int)uxfio_lseek(swvarfs->fdM,
						0, UXFIO_SEEK_VCUR);

		if (current_pos < 0) {
			/*
			* Get this errro when writing big archives.
			*/
			SWBIS_E_FAIL("uxfio_lseek failed-1");
		}

		SWVARFS_E_DEBUG3("XXX running uxfio_lseek fd=%d offset=%d", swvarfs->fdM, (int)(swvarfs->current_data_offsetM - current_pos));
		if (uxfio_lseek(swvarfs->fdM, (off_t)(swvarfs->current_data_offsetM - current_pos), SEEK_CUR) < 0) {
			SWBIS_E_FAIL("uxfio_lseek failed-2");
		}
	}
	swvarfs->u_fdM=-1;
	if (swvarfs->g_linkname_) { 
		swbis_free(swvarfs->g_linkname_);
		swvarfs->g_linkname_ = (char*)NULL;
	}
	if (swvarfs->u_current_name_) {
		swbis_free(swvarfs->u_current_name_);
		swvarfs->u_current_name_ = (char*)NULL;
	}
	if (swvarfs->u_fd_setM[u_index].u_current_name_) {
		swbis_free(swvarfs->u_fd_setM[u_index].u_current_name_); 
		swvarfs->u_fd_setM[u_index].u_current_name_ = NULL;
	}
	ud_unset_fd(swvarfs, fd);
	uxfio_free(fd);	
	SWVARFS_E_DEBUG("LEAVING");
	return 0;		
}

static
void
clear_all_dir_context(SWVARFS * swvarfs)
{
	struct dirContext dirx;
	while (swvarfs->stackixM > 0) raise_dir_context(swvarfs, &dirx);
}

static
void
set_dirscope(SWVARFS * swvarfs, char * path)
{
	strob_strcpy(swvarfs->dirscopeM, path);
	swlib_squash_trailing_slash(strob_str(swvarfs->dirscopeM));
	swlib_squash_leading_dot_slash(strob_str(swvarfs->dirscopeM));

	{
		char *s, *p;
		s = strob_str(swvarfs->dirscopeM);
		p = strstr(s, "/*");
		if (strcmp(s, "/") && (p == NULL || p != s) ) {
			/*
			* Add a \\/\\* to the end of swvarfs->dirscopeM
			* because this will be used as the pattern for
			* fnmatch().
			*/
			strob_strcat(swvarfs->dirscopeM, "/*");
		} else if (strcmp(s, "/") == 0) {
			strob_strcat(swvarfs->dirscopeM, "*");
		}
	}
}

static
SWVARFS *
swvarfs_open_serial_access_file_internal(SWVARFS *swvarfs,
						char * name, int oflags)
{	
	/* 
	* open an archive file.
	*/
	strob_strcpy(swvarfs->opencwdpathM, "");
	strob_strcpy(swvarfs->dirscopeM, "");
	swvarfs->fdM = uinfile_open(name, (mode_t)(0),
				&(swvarfs->format_descM), oflags);
	swvarfs->did_dupM=1;
	if (swvarfs->fdM < 0) {
		fprintf(stderr,"error (swvarfs_open_serial_access_file_internal) loc=1\n");
		swbis_free(swvarfs);
		return (SWVARFS*)(NULL);
	}
	if (swvarfs_i_iopen(swvarfs, oflags))  {
		fprintf(stderr,"error (swvarfs_open_serial_access_file_internal) loc=2\n");
		swbis_free(swvarfs);
		return (SWVARFS*)(NULL);
	}
	swvarfs->formatM=uinfile_get_type(swvarfs->format_descM);
	swvarfs->has_leading_slashM = uinfile_get_has_leading_slash(
						swvarfs->format_descM);
	return swvarfs;
}	

static
SWVARFS *
swvarfs_open_directory_internal(SWVARFS *swvarfs, char * name)
{	
	/* 
	* open a directory in file system.
	*/
	swvarfs->formatM=UINFILE_FILESYSTEM;
		
	strob_set_length(swvarfs->opencwdpathM, 256);
	strob_str(swvarfs->opencwdpathM)[255] = '\0';
	if(getcwd(strob_str(swvarfs->opencwdpathM), 255) == NULL) {
		fprintf(stderr,"swvarfs_open: cwd name too long.\n");
		swbis_free(swvarfs);
		return (SWVARFS*)NULL;
	}

	strob_strcpy(swvarfs->direntpathM, name); 

	set_dirscope(swvarfs, name);
	swlib_slashclean(strob_str(swvarfs->direntpathM));
	swlib_slashclean(strob_str(swvarfs->opencwdpathM));
	swvarfs->did_dupM=0;
	swvarfs->fdM=-1;
	return swvarfs;
}

/* ------------------------------------------------------------------- */
/* ------------------------  Public Functions ------------------------ */
/* ------------------------------------------------------------------- */

UINFORMAT *
swvarfs_get_uinformat(SWVARFS *swvarfs)
{
	return swvarfs->format_descM;
}

void
swvarfs_debug_dump_filechain(SWVARFS * swvarfs)
{
	struct fileChain *last=swvarfs->headM;
	while(last) {
		dump_filechain_link(last);
		last=last->nextFC;
	}
}

SWVARFS *
swvarfs_open_archive_file(char * name, int oflags)
{
	SWVARFS *swvarfs=(SWVARFS*)malloc(sizeof(SWVARFS));
	SWVARFS * ret;
	if(swvarfs == NULL) return (SWVARFS*)(NULL);
	swvarfs_i_init(swvarfs);
	ret = swvarfs_open_serial_access_file_internal(swvarfs, name, oflags);
	if (ret) {
		strob_strcpy(swvarfs->openpathM, name);
		set_vcwd(swvarfs->vcwdM, name);
	}
	return ret;
}

SWVARFS *
swvarfs_open_directory(char * name)
{
	SWVARFS *swvarfs=(SWVARFS*)malloc(sizeof(SWVARFS));
	SWVARFS * ret;
	if(swvarfs == NULL) return (SWVARFS*)(NULL);
	swvarfs_i_init(swvarfs);
	ret = swvarfs_open_directory_internal(swvarfs, name);
	if (ret) {
		strob_strcpy(swvarfs->openpathM, name);
		set_vcwd(swvarfs->vcwdM, name);
	}
	swvarfs->format_descM = (UINFORMAT*)NULL;
	return ret;
}

SWVARFS *
swvarfs_open(char * name, int oflags, mode_t p_mode)
{
	SWVARFS * rval;
	int statrc = 0;
	mode_t mode = 0;
	struct stat stbuf;
	SWVARFS_E_DEBUG3("ENTERING name=[%s] mode=%d", name?name:"", oflags); 

	if (p_mode == 0) {
		if (name && strcmp(name, "-") != 0) 
			statrc = stat(name, &stbuf);
		if (statrc == 0) 
			mode = stbuf.st_mode;
	} else {
		mode = p_mode;
	}
	
	if (name && strcmp(name,"-") && statrc == 0 &&
			(S_ISDIR(mode) || mode == SWVARFS_S_IFDIR)
	) {
		rval = swvarfs_open_directory(name);
	} else {				
		rval = swvarfs_open_archive_file(name, oflags);
	}
		
	return rval;
}

SWVARFS *
swvarfs_opendup_with_name(int ifd, int oflags, mode_t mode, char * name)
{
	SWVARFS *swvarfs=(SWVARFS*)malloc(sizeof(SWVARFS));

	SWVARFS_E_DEBUG2("ENTERING mode=%d", oflags); 
	if (swvarfs == NULL) return NULL;

	if (name == NULL)
		return swvarfs_opendup(ifd, oflags, mode);

	/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
	swvarfs->fdM = uinfile_opendup_with_name(ifd, (mode_t)(0),
				&(swvarfs->format_descM), oflags, name);
	if (swvarfs->fdM < 0) {
		swbis_free(swvarfs);
		return (SWVARFS*)(NULL);
	} else if (swvarfs->fdM == ifd) {
		swvarfs->did_dupM=0;
	} else {
		swvarfs->did_dupM=1;
	}

	swvarfs_i_init(swvarfs);
	if (swvarfs_i_iopen(swvarfs, oflags))
		return NULL;
	swvarfs->formatM=uinfile_get_type(swvarfs->format_descM);
	return swvarfs;
}

SWVARFS *
swvarfs_opendup(int ifd, int oflags, mode_t mode)
{
	SWVARFS *swvarfs=(SWVARFS*)malloc(sizeof(SWVARFS));

	SWVARFS_E_DEBUG2("ENTERING mode=%d", oflags); 
	if (swvarfs == NULL) return NULL;

	SWVARFS_E_DEBUG(""); 
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->fdM = uinfile_opendup(ifd, (mode_t)(0),
				&(swvarfs->format_descM), oflags);
	SWVARFS_E_DEBUG(""); 
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	if (swvarfs->fdM < 0) {
		SWVARFS_E_DEBUG(""); 
		swbis_free(swvarfs);
		return (SWVARFS*)(NULL);
	} else if (swvarfs->fdM == ifd) {
		SWVARFS_E_DEBUG("did_dup = 0"); 
		swvarfs->did_dupM=0;
	} else {
		SWVARFS_E_DEBUG("did_dup = 1"); 
		swvarfs->did_dupM=1;
	}

	SWVARFS_E_DEBUG(""); 
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs_i_init(swvarfs);
	SWVARFS_E_DEBUG(""); 
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	if (swvarfs_i_iopen(swvarfs, oflags))
		return NULL;
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	swvarfs->formatM=uinfile_get_type(swvarfs->format_descM);
	return swvarfs;
}

int
swvarfs_u_readlink(SWVARFS * swvarfs, char * path, char * buf, size_t bufsize)
{
	struct stat st;
	int fd;
	char * s;
	int ret;

	if (swvarfs && swvarfs->formatM !=UINFILE_FILESYSTEM) {
		fd = swvarfs_u_open(swvarfs, path);
		if (fd < 0) return -4;

		if (swvarfs->formatM !=UINFILE_FILESYSTEM) {
			if (swvarfs_file_has_data(swvarfs)) {
				taru_pump_amount2(-1, fd, -1, -1);
			}
		}

		if (swvarfs_u_fstat(swvarfs, fd, &st)) {
			swvarfs_u_close(swvarfs, fd);
			return -3;
		}
		if (S_ISLNK(st.st_mode)) {
			s = ahsStaticGetTarLinkname(
					ahs_vfile_hdr(swvarfs->ahsM));
		} else {
			swvarfs_u_close(swvarfs, fd);
			return -2;
		}
		strncpy(buf, s, bufsize);
		buf[bufsize -1] = '\0';

		if (strlen(s) >= bufsize - 1) {
			ret = -strlen(s);
		} else {
			ret = strlen(s);
		}
		swvarfs_u_close(swvarfs, fd);
	} else {
		ret = readlink(path, buf, bufsize);
		if (ret < 0) {
			fprintf(stderr,
				"readlink (%s) : %s\n", path, strerror(errno));
		} else if ((int)ret >= (int)bufsize) {
			fprintf(stderr,
				"readlink (%s) : linkname to long\n", path);
			 buf[bufsize - 1] = '\0';
			ret = -ret;
		} else if (ret == 0) {
			fprintf(stderr,
				"readlink (%s) : has empty linkname\n", path);
			ret = -1;
		 	*buf = '\0';
		} else {
		 	buf[ret] = '\0';
		}
	}
	return ret;
}

int
swvarfs_vchdir(SWVARFS * swvarfs, char * path)
{
	int ret;
	STROB  * newvcwd;
	
	newvcwd = strob_open(10);
	ret = construct_new_path_from_pathspec(swvarfs, newvcwd, path);
	if (ret == 0) {
		set_vcwd(swvarfs->vcwdM, strob_str(newvcwd));
		ret = swvarfs_setdir(swvarfs, strob_str(swvarfs->vcwdM));
	}
	strob_close(newvcwd);
	return ret;
}

int
swvarfs_setdir(SWVARFS * swvarfs, char * path)
{
	DIR *dp = NULL;
	int ret = 0;

	if ((path == (char*)(NULL) || strlen(path) == 0) &&
				swvarfs->formatM != UINFILE_FILESYSTEM) {
		/*
		* turn off scope test for archive files.
		*/
		strob_strcpy(swvarfs->dirscopeM, "");
		return 0;
	}

	{
		int depth = 0;
		swlib_resolve_path(path, &depth, (STROB *)NULL);
		if (depth < 0)  {
			/*
			* Opps, somebody tried to "../" to far.
			*/
			return -1;
		}
	}

	if (swvarfs->formatM == UINFILE_FILESYSTEM) {
		if (is_name_in_scope(strob_str(swvarfs->openpathM),
					path, (struct stat *)NULL) == 0) {
			fprintf(stderr,
		"directory [%s] not in current scope of %s openpath=%s\n", 
				path, strob_str(swvarfs->dirscopeM),
					strob_str(swvarfs->openpathM));
			return -2;
		}
	}
	if (swvarfs->formatM == UINFILE_FILESYSTEM) {
		/*
		* (char*)path must be a directory.
		*/
		struct dirContext dirx;	
		int statrc;
		struct stat stbuf;

	        dirx.statbuf = NULL;
		dirx.dirp = NULL;
		dirx.dp = NULL;
		dirx.ptr = NULL;

		clear_all_dir_context(swvarfs);

		statrc = stat(path, &stbuf);

		if (statrc || S_ISDIR(stbuf.st_mode) == 0) {
			fprintf(stderr,
				"swvarfs_setdir : %s : %s\n",
						path, strerror(errno));
			ret = -1;
		} else {
			swvarfs->stackixM = 0;
			strob_strcpy(swvarfs->direntpathM, path);
			ret = 0;
		}

		if ( (dp = opendir(path)) == NULL) {
			fprintf(stderr, "%s : %s\n",path, strerror(errno));
			swvarfs->derrM = -1;
			return -3;
		}

		dirx.dp = dp;
		add_dir_context(swvarfs, &dirx);

		ret = 0;
	} else {
		int header_offset;
		intmax_t old_pos;
		struct fileChain * existing = NULL;
		struct stat st;
		int espipe = 0;
		/*
		* turn off the no-file-chain optimization.
		*/
		swvarfs->makefilechainM = 1;

		/*
		* Set the current_filechain to NULL, so
		* it can be tested later.
		*/
		swvarfs->current_filechainM = (struct fileChain*)NULL;
		
		/*
		* Seek in the archive to path.
		*/
		if ((espipe=uxfio_espipe(swvarfs->fdM))) {
			/*
			* can't seek.
			* Only can go forward.
			*/
			;	
		} else {
			/*
			* Seek back to the beginning.
			*/
			if (uxfio_lseek(swvarfs->fdM, (off_t)0, SEEK_SET)) {
				SWBIS_E_FAIL("uxfio_lseek error");
				/* return -1; */
			}
		}
		
		/*
		* if path is './' or '.' then make this have the
		* meaning of seeking to the beginning.
		*/
		if (strcmp(path, ".") == 0 || strcmp(path, "./") == 0) {
			if (espipe) {
				/*
				* error, cannot comply.
				*/
				return -4;	
			} else {
				/*
				* already seeked to beggining, done.
				*/
				return 0;
			}
		}

		/*
		* Now search to the first file that is the scope
		* of (char*)path.
		*
		* Use the lstat function to search.
		*/
		if (swvarfs_u_lstat(swvarfs, path, &st)) {
			return -5;
		}


		/*
		* Found this file.  Now seek back so it is returned
		* in the first dirent call.
		*/

		existing = swvarfs->current_filechainM;
		if (existing == (struct fileChain*)NULL) {
			SWBIS_E_FAIL("(struct fileChain*)existing is null. fatal.");
			return -6;
		}

		/*
		* Get the current position.
		*/
		old_pos = (intmax_t)uxfio_lseek(swvarfs->fdM, 0 , UXFIO_SEEK_VCUR);
		if (old_pos < 0) {
			SWBIS_E_FAIL2("uxfio_lseek error ret = %d", (int)old_pos);
			return -7;
		}
			
		header_offset = existing->header_offsetFC;
		
		/*
		* Perform sanity check.
		*/
		if (header_offset - old_pos >= 0) {
			SWBIS_E_FAIL3("internal error old_pos=%s  header_offset=%s", swlib_imaxtostr(old_pos, NULL),
										swlib_imaxtostr(header_offset, NULL));
			return -8;
		}

		/* 
		* Seek back to the header offset
		*/	
		if (uxfio_lseek(swvarfs->fdM, header_offset - old_pos, UXFIO_SEEK_VCUR) < 0) {
			SWBIS_E_FAIL("uxfio_lseek error");
			return -9;
		}

		/*
		* Now, clear the old file chain.
		*/
		swvarfs_i_clear_filechain(swvarfs);
		ret = 0;
	}
	
	set_vcwd(swvarfs->vcwdM, path);
	set_dirscope(swvarfs, path);
	return ret;
}


int
swvarfs_u_lstat(SWVARFS * swvarfs, char * path, struct stat * st)
{
	if (swvarfs->formatM==UINFILE_FILESYSTEM) {
		return swvarfs_i_u_lstat_fs(swvarfs, path, st);
	} else {
		return swvarfs_i_u_lstat_archive(swvarfs, path, st);
	}
}

int
swvarfs_close(SWVARFS * swvarfs)
{
	int ret = 0;
	SWVARFS_E_DEBUG("ENTERING"); 
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	if (swvarfs->did_dupM) {
		SWVARFS_E_DEBUG("did_dupM: CLOSING swvarfs->fdM"); 
		uxfio_close(swvarfs->fdM);
	} else {
		SWVARFS_E_DEBUG("did_dupM: NOT CLOSING swvarfs->fdM"); 
		;
	}
	strob_close(swvarfs->opencwdpathM);
	strob_close(swvarfs->openpathM);
	strob_close(swvarfs->u_nameM);
	strob_close(swvarfs->tmpM);
	strob_close(swvarfs->dirscopeM);
	if (swvarfs->stackM != (STROB*)(NULL))
		strob_close(swvarfs->stackM);
	if (swvarfs->direntpathM != (STROB*)(NULL))
		strob_close(swvarfs->direntpathM);
	if (swvarfs->do_close_ahsM)
		ahs_close(swvarfs->ahsM);
	if (swvarfs->uinformat_close_on_deleteM) {
		if (swvarfs->format_descM) 
			ret = uinfile_close(swvarfs->format_descM);
	} else {
		ret = 0;
	}
	swvarfs_i_delete_filechain(swvarfs);
	hllist_close(swvarfs->link_recordM);
	taru_delete(swvarfs->taruM);
	swbis_free(swvarfs);
	SWVARFS_E_DEBUG2("LOWEST FD=%d", show_nopen()); 
	return ret;
}

int
swvarfs_uxfio_fcntl(SWVARFS * swvarfs, int cmd, int value)
{
	int ret;
	if (swvarfs->fdM >= UXFIO_FD_MIN) {
		ret = uxfio_fcntl(swvarfs->fdM, cmd, value);
		if (ret == 0) {
			switch (cmd) {
				case UXFIO_F_SET_BUFTYPE:
	    	     			uxfio_fcntl(swvarfs->fdM,
						UXFIO_F_SET_BUFACTIVE,
							UXFIO_ON);
					swvarfs->uxfio_buftype = value;
					break;
				case UXFIO_F_ARM_AUTO_DISABLE:
					if (value) {
						swvarfs->uxfio_buftype =
							UXFIO_BUFTYPE_NOBUF;
					}
					break;
			}
		}
		return ret;
	} else {
		return 0;
	}
}

struct new_cpio_header *
swvarfs_header(SWVARFS * swvarfs)
{
	return ahs_vfile_hdr(swvarfs->ahsM);
}

AHS *
swvarfs_get_ahs(SWVARFS * swvarfs)
{
	return swvarfs->ahsM;
}

void
swvarfs_set_ahs(SWVARFS * swvarfs, AHS * ahs)
{
	if (swvarfs->do_close_ahsM)
		ahs_close(swvarfs->ahsM);
	swvarfs->ahsM = ahs;
	swvarfs->do_close_ahsM = 0;
}

int
swvarfs_fd(SWVARFS * swvarfs)
{
	return swvarfs->fdM;
}

int
swvarfs_u_open(SWVARFS * swvarfs, char * name)
{
	int ret;
	SWVARFS_E_DEBUG("ENTERING"); 
	SWVARFS_E_DEBUG2("name = %s", name); 

	if (ud_find_fd(swvarfs, -1) < 0) {
		fprintf(stderr, "too many open swvarfs:u_fd descriptors.\n");
		return -1;
	}


	if (swvarfs->formatM==UINFILE_FILESYSTEM) {
		ret = swvarfs_i_u_open_fs(swvarfs, name);
	} else {
		ret = swvarfs_i_u_open_archive(swvarfs, name);
	}

	/* SWLIB_ASSERT(ud_set_fd(swvarfs, ret) >= 0); */

	SWVARFS_E_DEBUG("LEAVING"); 
	return ret;
}

int 
swvarfs_u_close(SWVARFS * swvarfs, int fd)
{
	int ret;
	int u_index;
	SWVARFS_E_DEBUG("ENTERING"); 
	if (swvarfs->formatM == UINFILE_FILESYSTEM) {
		SWVARFS_E_DEBUG("file system"); 
		if ((u_index = ud_find_fd(swvarfs, fd)) < 0) {
			SWBIS_E_FAIL3("close error, expecting desc=%d. given=%d\n", swvarfs->u_fdM, fd);
			return -1;
		}
		if (fd != swvarfs->u_fdM &&
				fd != swvarfs->u_fd_setM[u_index].u_fdM) {
			SWBIS_E_FAIL3("close error, expecting desc=%d. given=%d\n", swvarfs->u_fdM, fd);
			return -1;
		}
		if (swvarfs->u_fd_setM[u_index].u_linkname_){ 
			swbis_free(swvarfs->u_fd_setM[u_index].u_linkname_); 
			swvarfs->u_fd_setM[u_index].u_linkname_ = NULL; 
		}
		if (swvarfs->g_linkname_){ 
			swbis_free(swvarfs->g_linkname_); 
			swvarfs->g_linkname_ = NULL; 
		}
		if (swvarfs->u_current_name_) {
			swbis_free(swvarfs->u_current_name_); 
			swvarfs->u_current_name_ = NULL;
		}
		if (swvarfs->u_fd_setM[u_index].u_current_name_) {
			swbis_free(swvarfs->u_fd_setM[u_index].u_current_name_); 
			swvarfs->u_fd_setM[u_index].u_current_name_ = NULL;
		}

		ud_unset_fd(swvarfs, fd);
		swvarfs->u_fdM=-1;
		ret = uxfio_close(fd);
		SWVARFS_E_DEBUG("LEAVING"); 
		return ret;
	} else {
		SWVARFS_E_DEBUG("archive"); 
		ret =  swvarfs_i_u_close_archive(swvarfs, fd);
		SWVARFS_E_DEBUG("LEAVING"); 
		return ret;
	}
}

int  /* used for debuging */
swvarfs_u_usr_stat(SWVARFS * swvarfs, int fd, int ofd)
{
	char * name;
	char * linkname;
	struct stat st;
	if (swvarfs_u_fstat(swvarfs, fd, &st)) {
		return 1;
	}

	name = swvarfs_u_get_name(swvarfs, fd);
	linkname = swvarfs_u_get_linkname(swvarfs, fd);
	swlib_write_stats(
		name,
		linkname,
		&st,
		0 /* terse */,
		"" /* markup_prefix */,
		ofd,
		(STROB *)NULL);
	return 0;
}

char *
swvarfs_u_get_name(SWVARFS * swvarfs, int fd)
{
	int u_index;
	if ((u_index = ud_find_fd(swvarfs, fd)) < 0) {
		fprintf(stderr, "swvarfs invalid file descriptor %d.\n", fd);
		return NULL;
	}
	/* 
	* if (fd != swvarfs->u_fdM){
	* 	SWBIS_E_FAIL2("error, expecting %d.\n", swvarfs->u_fdM);
	* 	return NULL;
	* }
	*/
	return swvarfs->u_fd_setM[u_index].u_current_name_;
}

char * 
swvarfs_u_get_linkname(SWVARFS * swvarfs, int fd)
{
	if (fd != swvarfs->u_fdM){
		SWBIS_E_FAIL2("error, expecting %d.\n", swvarfs->u_fdM);
		return NULL;
	}
	return swvarfs->g_linkname_;
}

int
swvarfs_u_fstat(SWVARFS * swvarfs, int fd, struct stat *st)
{
	int ret;
	ret = uxfio_fstat(fd, st);
	return ret;
}

void
swvarfs_stop_function(SWVARFS * swvarfs, int (*fc)(void *, char *, char *))
{
	swvarfs->f_do_stop_ = fc;
}

int
swvarfs_dirent_reset(SWVARFS * swvarfs)
{
	if (swvarfs->formatM==UINFILE_FILESYSTEM) {
		;  /* nothing to do */
	} else {
		uxfio_lseek(swvarfs->fdM, (off_t)0, SEEK_SET);
		swvarfs->eoaM = 0;
	}
	return 0;
}


char *
swvarfs_get_next_dirent(SWVARFS * swvarfs, struct stat *st)
{
	int tartype;
	int is_at_end;
	char * name;
	char *s;
	SWVARFS_E_DEBUG("ENTERING"); 
	if (swvarfs->formatM==UINFILE_FILESYSTEM) {
		SWVARFS_E_DEBUG(""); 
		name = swvarfs_get_next_dirent_fs(swvarfs, st);
		is_at_end = 0;
		SWVARFS_E_DEBUG(""); 
	} else {
		int * peoa = &(swvarfs->eoaM);
		SWVARFS_E_DEBUG("entering swvarfs_get_next_dirent_archive_NEW"); 
		name = swvarfs_get_next_dirent_archive_NEW(swvarfs, st, peoa);
		is_at_end = *peoa;
		SWVARFS_E_DEBUG2("SWVARFS (ARCHIVE) EOA=[%d]", *peoa); 
	}

	SWVARFS_E_DEBUG(""); 
	if (is_at_end == 0) {
		SWVARFS_E_DEBUG(""); 
		tartype = ahs_get_tar_typeflag(swvarfs->ahsM);
		if (tartype < 0) {
			fprintf(stderr, "%s: unrecognized file type [%d] for file: %s\n",
				swlib_utilname_get(), (int)tartype, name?name:"<null>");
			set_vcwd(swvarfs->vcwdM, name?name:"");
		} else if (tartype == DIRTYPE) {
			SWVARFS_E_DEBUG(""); 
			set_vcwd(swvarfs->vcwdM, name?name:"");
		} else {
			/*
			* strip off last component.
			*/
			SWVARFS_E_DEBUG(""); 
			set_vcwd(swvarfs->vcwdM, name?name:"");
			if ((s=strrchr(strob_str(swvarfs->vcwdM), '/'))) {
				*s = '\0';
			}
		}
		SWVARFS_E_DEBUG(""); 
	} else {
		/*
		* end of archive.
		*/
		SWVARFS_E_DEBUG2("GOT EOA [%d]", is_at_end); 
		name = NULL;
		;
	}
	SWVARFS_E_DEBUG2("LEAVING !!!!!!!!!!!!!!! [%s]", name?name:"<nil>"); 
	return name;
}

int
swvarfs_dirent_err(SWVARFS * swvarfs)
{
	return swvarfs->derrM;
}

HLLIST * 
swvarfs_get_hllist(SWVARFS * swvarfs)
{
	return swvarfs->link_recordM;
}

int
swvarfs_get_format(SWVARFS * swvarfs)
{
	return swvarfs->formatM;
}

int
swvarfs_file_has_data(SWVARFS * swvarfs)
{
	int type = ahs_get_tar_typeflag(swvarfs->ahsM);
	return (type == REGTYPE);
}

void
swvarfs_set_stat_syscall(SWVARFS * swvarfs, char * s)
{
	if (*s == 'l') {
		swvarfs->f_statM = (int (*)(char *, struct stat *))lstat;
	} else if (*s == 's') {
		swvarfs->f_statM = (int (*)(char *, struct stat *))stat;
	}
}

char *
swvarfs_get_stat_syscall(SWVARFS * swvarfs)
{
	if ( swvarfs->f_statM == (int (*)(char *, struct stat *))lstat) {
		return SWVARFS_VSTAT_LSTAT;
	} else if ( swvarfs->f_statM == (int (*)(char *, struct stat *))stat) {
		return SWVARFS_VSTAT_STAT;
	} else {
		fprintf(stderr, "swvarfs: fatal error in swvarfs_get_stat_syscall\n");
		exit(1);
	}
}

int swvarfs_get_tarheader_flags(SWVARFS * swvarfs) {
	return swvarfs->taruM->taru_tarheaderflagsM;
}	
	
void swvarfs_set_tarheader_flags(SWVARFS * swvarfs, int flags) {
	swvarfs->taruM->taru_tarheaderflagsM = flags;
}

void swvarfs_set_tarheader_flag(SWVARFS * swvarfs, int flag, int n) {
	taru_set_tarheader_flag(swvarfs->taruM, flag,  n);
}
