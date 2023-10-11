/* xformat.c - read/write archives
 
   Copyright (C) 2000,2004 Jim Lowe

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
#include "xformat.h"
#include "inttostr.h"
#include "swlib.h"
#include "swutilname.h"

#include "debug_config.h"
#define XFORMATNEEDDEBUG 1
#undef XFORMATNEEDDEBUG

#ifdef XFORMATNEEDDEBUG
#define XFORMAT_E_DEBUG(format) SWBISERROR("XFORMAT DEBUG: ", format)
#define XFORMAT_E_DEBUG2(format, arg) \
			SWBISERROR2("XFORMAT DEBUG: ", format, arg)
#define XFORMAT_E_DEBUG3(format, arg, arg1) \
			SWBISERROR3("XFORMAT DEBUG: ", format, arg, arg1)
#else
#define XFORMAT_E_DEBUG(arg)
#define XFORMAT_E_DEBUG2(arg, arg1)
#define XFORMAT_E_DEBUG3(arg, arg1, arg2)
#endif /* XFORMATNEEDDEBUG */

#define XFORMAT_E_FAIL(format) SWBISERROR("INTERNAL ERROR: ", format)
#define XFORMAT_E_FAIL2(format, arg) \
			SWBISERROR2("INTERNAL ERROR: ", format, arg)
#define XFORMAT_E_FAIL3(format, arg, arg1) \
			SWBISERROR3("INTERNAL ERROR: ", format, arg, arg1)

int
is_all_digits(char * value)
{
	char * w = value;

	while (*w  &&  isdigit(*w)) w++;
	if (*w == '\0') return 1;
	return 0;
}

static
int
local_v_close(XFORMAT * xux, int fd)
{
	int ret;
	if (xux->swvarfsM) {
		ret = xformat_u_close_file(xux, fd);
	} else {
		ret = close(fd);
	}
	return ret;
}

static
int 
local_v_open(XFORMAT * xux, char * source, int flags, mode_t mode)
{
	int ret;
	if (xux->swvarfsM) {
		ret = xformat_u_open_file(xux, source);
	} else {
		ret = open(source, flags, mode);
	}
	return ret;
}

static
int
local_v_lstat(XFORMAT * xux, char * path, struct stat * st)
{
	int ret;
	if (xux->swvarfsM) {
		ret = xformat_u_lstat(xux, path, st);
	} else {
		ret = lstat(path, st);
	}
	return ret;
}

static 
PORINODE *
return_do_inodes(XFORMAT * xux)
{
	return ((xux->make_false_inodesM !=0 ) || 
		((int)(xux->output_format_codeM) == arf_oldascii)) \
					?  xux->porinodeM : (PORINODE*)(NULL);
}

static 
void 
internal_set_format (XFORMAT * xux, int * instance_format, int format)
{
	if (format <= (int)(arf_crcascii) && !xux->deferM) {
		xux->deferM = defer_open(format);
		defer_set_taru(xux->deferM, xux->taruM);
	}
	if (format > (int)(arf_crcascii) && xux->deferM) {
		defer_close(xux->deferM);
		xux->deferM = NULL;
	}
        /* xux->format_codeM = (enum archive_format)(format); */
	*instance_format = (enum archive_format)(format);
}

static 
int
common_setup_write_header(XFORMAT * xux, struct new_cpio_header **p_hdr1, 
			struct new_cpio_header *hdr0, 
				struct stat *t_stat, char * t_name, 
						char * t_source)
{
		XFORMAT_E_DEBUG("");
		/* 
		// hdr0 is never null; 
                // only lstat the source if name is null.
	        */ 
		*strob_str(xux->linkname2M) = '\0';
		/* taru_exattlist_init(xformat_vfile_hdr(xux)); */
		/* taru_exattlist_init(hdr0); */
		*p_hdr1=(struct new_cpio_header*)(xformat_vfile_hdr(xux));
		if (!t_name ) {
                    	char * p1_name = ahsStaticGetTarFilename(*p_hdr1);
			if (strlen(p1_name)) {
				strob_strcpy(xux->name2M, p1_name); 
		  	}
       		} else {
			strob_strcpy(xux->name2M, t_name);
		} 
	
		if (t_name == NULL && t_stat == NULL && t_source == NULL) {
			/*
			* //
			* // Use the object vfile_hdr.
			* //
			*/
			XFORMAT_E_DEBUG("[0 0 0]");
		} else if (t_name == NULL  && t_stat == NULL && t_source) {
			/*
			* Use the vfile header for stats.
			* but get the size from the source.
			*/
			XFORMAT_E_DEBUG("[0 0 1]");
			XFORMAT_E_DEBUG2("[0 0 1] %s", t_source);
			if (lstat (t_source, &(xux->lt2M)) != 0 ) {
				fprintf(stderr,
				"lstat [xformat] %s: %s\n",
					t_source, strerror(errno));
				return -1;
			} else {
				int tartypeflag;
				/*
				* set the size and filetype because you
				* probably don't want to lie about these.
				*/
		 		xformat_set_filesize(xux, (intmax_t)((xux->lt2M).st_size));
				tartypeflag = taru_get_tar_filetype(
							(xux->lt2M).st_mode);
				if (tartypeflag < 0) {
					fprintf(stderr, "%s: unrecognized file type in mode [%d] for file: %s\n",
						swlib_utilname_get(), (int)((xux->lt2M).st_mode), t_source);
				}
				xformat_set_filetype_from_tartype(xux,
								tartypeflag);
				taru_filehdr2filehdr(hdr0, *p_hdr1);
				#ifdef TARUNEEDDEBUG
				XFORMAT_E_DEBUG2("[0 0 1] %s", taru_header_dump_string_s(hdr0, "[0 0 1] hdr0 "));
				#endif
			}
		} else if (t_name && t_stat == NULL && t_source == NULL) {
			/*
			* //
			* // name2 has already been set from t_name.
			* //
			*/
			XFORMAT_E_DEBUG("[1 0 0]");
			XFORMAT_E_DEBUG2("[1 0 0] %s", t_name);
			taru_filehdr2statbuf(&(xux->lt2M), *p_hdr1);
			#ifdef TARUNEEDDEBUG
			XFORMAT_E_DEBUG2("[1 0 0] %s", taru_header_dump_string_s(*p_hdr1, "[1 0 0] *p_hdr1 "));
			#endif
			strob_strcpy(xux->linkname2M, 
					ahsStaticGetTarLinkname(*p_hdr1));
			taru_statbuf2filehdr(hdr0, &(xux->lt2M),
					(char *)(NULL),
					strob_str(xux->name2M),
					strob_str(xux->linkname2M));
			taru_filehdr2filehdr(hdr0, *p_hdr1);
			#ifdef TARUNEEDDEBUG
			XFORMAT_E_DEBUG2("[1 0 0] %s", taru_header_dump_string_s(hdr0, "[1 0 0] hdr0 "));
			#endif
			ahsStaticSetTarFilename(hdr0, strob_str(xux->name2M));
			ahsStaticSetPaxLinkname(hdr0, strob_str(xux->linkname2M));
			*p_hdr1 = hdr0;
		 } else if ( (t_stat && t_name && t_source == NULL) ||
					(t_stat && t_name && t_source) ) {
			/*
			* //
		        * // Use statbuf on function param list.
		        * //
			*/
			/* 
			* t_source may be null and thats OK.
			*/

			XFORMAT_E_DEBUG("[1 1 0] || [1 1 1]");
			XFORMAT_E_DEBUG3("[1 1 0] || [1 1 1] %s %s", t_name , t_source);
			taru_statbuf2filehdr(hdr0, t_stat, t_source, strob_str(xux->name2M) , strob_str(xux->linkname2M));
			*p_hdr1 = hdr0;
			#ifdef TARUNEEDDEBUG
			XFORMAT_E_DEBUG2("[1 1 0]||[1 1 1]  %s", taru_header_dump_string_s(hdr0, "[1 1 0]||[1 1 1] hdr0 "));
			#endif
		 } else if (t_source) { 
			/*
			* //
			* // stat the source
		        * //
			*/
			XFORMAT_E_DEBUG("[x x 1]");
			XFORMAT_E_DEBUG2("[x x 1] %s", t_source);
			if (local_v_lstat(xux, t_source, 
						&(xux->lt2M)) != 0 ) {
				fprintf(stderr,
					"lstat (loc=2) %s %s\n",
						t_source, strerror(errno));
					return -1;
			} else {
				taru_statbuf2filehdr(hdr0, &(xux->lt2M),
					t_source, strob_str(xux->name2M),
						strob_str(xux->linkname2M));
				*p_hdr1 = hdr0;
		 	}
			#ifdef TARUNEEDDEBUG
			XFORMAT_E_DEBUG2("[x x 1]  %s", taru_header_dump_string_s(hdr0, "[x x 1] hdr0 "));
			#endif
		 } else {
		 	fprintf(stderr,
			"bad use of common_setup_write, last else-if.\n");
			return -1;
		 }
		return 0;
}

static 
void
setup_1 (XFORMAT * xux, int fmt)
{
	xux->eoaM = 0;
	xux->trailer_bytesM = 0;
	xux->bytes_writtenM = 0;
	xux->ifdM=xux->ofdM=-1;
	xux->link_recordM = hllist_open();
	xux->porinodeM = porinode_open();
	xux->use_false_inodesM = xux->porinodeM;
	xux->deferM = NULL;
	internal_set_format(xux, (int*)(&(xux->format_codeM)), fmt);
	internal_set_format(xux, (int*)(&(xux->output_format_codeM)), fmt);
	xux->swvarfsM = NULL;
	xux->ahsM = ahs_open();
	xux->make_false_inodesM = 1;
	xux->swvarfs_is_externalM = 0;
	xux->last_header_sizeM = 0;
	xux->taruM = taru_create();
	xux->taruM->taru_tarheaderflagsM = 0;
	xux->name2M = strob_open(10);
	xux->linkname2M = strob_open(10);
}

static
int
u_open_file_common(XFORMAT * xux, int fd)
{
	char *s;
	struct stat st;
	XFORMAT_E_DEBUG("");
	swvarfs_u_fstat(xux->swvarfsM, fd, &st);	
	taru_statbuf2filehdr(xformat_vfile_hdr(xux), &st,
				(char*)NULL, (char*)NULL, (char*)NULL);
	XFORMAT_E_DEBUG("");
	ahs_set_name(xux->ahsM,
		(s = swvarfs_u_get_name(xux->swvarfsM, fd)) ? s : "");
	XFORMAT_E_DEBUG("");
	ahs_set_linkname(xux->ahsM,
		(s = swvarfs_u_get_linkname(xux->swvarfsM, fd)) ? s : "");
	XFORMAT_E_DEBUG("");
	return fd;
}

static
void
open_archive_init(XFORMAT * xux)
{
	swvarfs_set_ahs(xux->swvarfsM, xux->ahsM);
	xformat_set_format(xux, swvarfs_get_format(xux->swvarfsM));
	xformat_set_output_format(xux, xux->format_codeM);
	xux->ifdM = swvarfs_fd(xux->swvarfsM);
}

static
int 
open_archive_dir(XFORMAT * xux, char * pathname, int flags, mode_t mode)
{
	XFORMAT_E_DEBUG("");
	xux->swvarfsM = swvarfs_open_directory(pathname);
	if (xux->swvarfsM == NULL) return -1; 
	open_archive_init(xux);
	return 0;
}

static
int
internal_open_archive_file(mode_t filetype, XFORMAT * xux, 
			char * pathname, int flags, mode_t mode)
{
	int ret;
	struct stat st;
	int rc;

	if (filetype && !pathname) {
		/*
		* Invalid.
		*/
		return -11;
	}

	if (filetype && pathname) {
		rc = stat(pathname, &st);
		if (rc < 0) {
			fprintf(stderr, 
				"stat : %s : %s\n", pathname, strerror(errno));
			return -9;
		}
	}

	if (filetype == 0) {
		/*
		* stdin.
		*/
		ret = xformat_open_archive(xux, "-", flags, mode);
	} else if (S_ISDIR(st.st_mode) && filetype == S_IFDIR) {
		/*
		* a directory containing file system hierarchy.
		*/
		ret = open_archive_dir(xux, pathname, flags, mode);
	} else if (S_ISREG(st.st_mode) && filetype == S_IFREG) {
		/*
		* a portable archive file.
		*/
		ret = xformat_open_archive(xux, pathname, flags, mode);
	} else {
		fprintf(stderr, "archive file is wrong type.\n");
		ret = -20;
	}
	return ret;
}

/*
//////////////////////////////////////////////////////////////////////////////
public:
//////////////////////////////////////////////////////////////////////////////
*/

int
xformat_close(XFORMAT * xux)
{
	int ret = 0;
	if (xux->link_recordM) hllist_close(xux->link_recordM);
	if (xux->porinodeM) porinode_close(xux->porinodeM);
	if (xux->deferM) defer_close(xux->deferM);
      	if (xux->ahsM) ahs_close(xux->ahsM);
	if (xux->swvarfsM && xux->swvarfs_is_externalM == 0) {
		ret = swvarfs_close(xux->swvarfsM);
	}
      	if (xux->taruM) {
		taru_delete(xux->taruM);
		xux->taruM = NULL;
	}
      	strob_close(xux->name2M);
      	strob_close(xux->linkname2M);
	return ret;
}
	
XFORMAT *
xformat_open(int ifd, int ofd, int format)
{
	XFORMAT * xux = (XFORMAT*)malloc(sizeof(XFORMAT));
	setup_1(xux, format);    
	xux->ifdM = ifd;
	xux->ofdM = ofd;
	return xux;
}
	
AHS *
xformat_ahs_object(XFORMAT * xux)
{
	return xux->ahsM;
}

char *
xformat_get_header_buffer(XFORMAT * xux, char * buf)
{
     if (buf) {
         return buf;
     } else {
         static char a[512];
         return a;
     }
}

void xformat_set_tar_chksum (XFORMAT * xux) { }
	
void 
xformat_set_format (XFORMAT * xux, int format)
{
	XFORMAT_E_DEBUG("");
	internal_set_format(xux, (int*)(&(xux->format_codeM)), format);
}
	
int
xformat_get_format(XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return (int)(xux->format_codeM);
}
	
void
xformat_set_output_format (XFORMAT * xux, int format)
{
	XFORMAT_E_DEBUG("");
	internal_set_format(xux, (int*)(&(xux->output_format_codeM)), format);
}
	
TARU * 
xformat_get_taru_object(XFORMAT * xux)
{
	return (xux->taruM);
}
	
int
xformat_get_output_format(XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return (int)(xux->output_format_codeM);
}

void
xformat_init_vfile_header(XFORMAT * xux)
{
	taru_init_header(xformat_vfile_hdr(xux));
}


struct new_cpio_header *
xformat_vfile_hdr(XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_vfile_hdr(xux->ahsM);
}

void
xformat_set_mode (XFORMAT * xux, mode_t mode)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_mode(xux->ahsM, mode);
}
	
void
xformat_set_perms (XFORMAT * xux, mode_t mode)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_perms(xux->ahsM, mode);
}

void
xformat_set_filetype_from_tartype (XFORMAT * xux, char  s)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_filetype_from_tartype(xux->ahsM, s);
}

void
xformat_set_uid (XFORMAT * xux, uid_t uid )
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_uid(xux->ahsM, uid);
}
	
void
xformat_set_username (XFORMAT * xux, char * name)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_tar_username(xux->ahsM, name);
}
	
int
xformat_set_user_systempair(XFORMAT * xux, char * name)
{ 
	int ret;
	XFORMAT_E_DEBUG2("name=[%s]", name);
	ret = ahs_set_user_systempair(xux->ahsM, name);
	/*
	* If the user was not in the database, the name that
	* is set in ahs_set_tar_username() is nobody.
	* Override this here.
	*/
	if (ret && is_all_digits(name) == 0) {
		/*
		* If not found and is not a uid, then
		* set the name.  Because we atleast already know
		* the name part of the system pair.
		*/
		XFORMAT_E_DEBUG("");
		ahs_set_tar_username(xux->ahsM,  name);
	}
	XFORMAT_E_DEBUG("");
	return ret;
}
	
int
xformat_set_group_systempair(XFORMAT * xux, char * name)
{ 
	int ret;
	ret = ahs_set_group_systempair(xux->ahsM, name);
	/*
	* If the user was not in the database, the name that
	* is set in ahs_set_tar_username() is nobody.
	* Override this here.
	*/
	if (ret && is_all_digits(name) == 0) {
		/*
		* If not found and is not a uid, then
		* set the name.  Because we atleast already know
		* the name part of the system pair.
		*/
		ahs_set_tar_groupname(xux->ahsM,  name);
	}
	return ret;
}
	
void
xformat_set_groupname (XFORMAT * xux, char * name) 
{
	XFORMAT_E_DEBUG("");
        ahs_set_tar_groupname(xux->ahsM, name);
}    

void
xformat_set_uid_by_name (XFORMAT * xux, char * username)
{
	XFORMAT_E_DEBUG("");
	ahs_set_uid_by_name(xux->ahsM, username);
}    

void
xformat_set_gid_by_name (XFORMAT * xux, char * groupname) 
{
	XFORMAT_E_DEBUG("");
	ahs_set_gid_by_name(xux->ahsM, groupname);
}    

void
xformat_set_gid (XFORMAT * xux, gid_t gid)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_gid(xux->ahsM, gid);
}    
  
void
xformat_set_filesize (XFORMAT * xux, intmax_t filesize)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_filesize(xux->ahsM, filesize);
}    

void
xformat_set_nlink (XFORMAT * xux, int  nlink)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_nlink(xux->ahsM, nlink);
}    
	
void
xformat_set_inode(XFORMAT * xux, ino_t ino)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_inode(xux->ahsM, ino);
}    

void
xformat_set_mtime (XFORMAT * xux, time_t mtime)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_mtime(xux->ahsM, mtime);
}    
	
void
xformat_set_devmajor (XFORMAT * xux, dev_t dev)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_devmajor(xux->ahsM, dev);
}    
    
void
xformat_set_devminor (XFORMAT * xux, dev_t dev)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_devminor(xux->ahsM, dev);
}    

void
xformat_set_name (XFORMAT * xux, char *name)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_name(xux->ahsM, name);
}
    
void
xformat_set_linkname (XFORMAT * xux, char * linkname)
{ 
	XFORMAT_E_DEBUG("");
        ahs_set_linkname(xux->ahsM, linkname);
}

int
xformat_set_virtual_eof (XFORMAT * xux, size_t len)
{ 
	XFORMAT_E_DEBUG("");
	if (xux->ifdM < UXFIO_FD_MIN) return -3;
	if (uxfio_fcntl(xux->ifdM, UXFIO_F_SET_BUFACTIVE, UXFIO_ON)) return -2;
	if (uxfio_fcntl(xux->ifdM, UXFIO_F_SET_VEOF, (int)len)) return -1;
	return 0; 
}

int
xformat_set_to_statbuf(XFORMAT * xux, struct stat *st)
{
	XFORMAT_E_DEBUG("");
	ahs_set_to_statbuf(xux->ahsM, st);
	return 0;
}

int
xformat_set_from_statbuf_path(XFORMAT * xux, char * path)
{ 
	struct stat st;
	XFORMAT_E_DEBUG("");
	XFORMAT_E_DEBUG("Running lstat");
	if (lstat(path, &st)) return -1;
	xformat_set_from_statbuf(xux, &st);
	return 0;
}

int
xformat_set_from_statbuf_fd(XFORMAT * xux, int fd)
{ 
	struct stat st;
	XFORMAT_E_DEBUG("");
	XFORMAT_E_DEBUG("Running fstat");
	if (fstat(fd, &st)) return -1;
	xformat_set_from_statbuf(xux, &st);
	return 0;
}

void
xformat_set_from_statbuf(XFORMAT * xux, struct stat *st)
{ 
	XFORMAT_E_DEBUG("");
	ahs_set_from_statbuf(xux->ahsM, st);
}

void
xformat_set_preview_level(XFORMAT * xux, int fd)
{ 
	XFORMAT_E_DEBUG("");
	if (!(xux->taruM)) {
		fprintf(stderr, "AAARRRRGG xux->taruM is null at line %d\n", __LINE__);
		return;
	}
	taru_set_preview_level(xux->taruM, fd);
}

void
xformat_set_preview_fd(XFORMAT * xux, int fd)
{ 
	XFORMAT_E_DEBUG("");
	if (!(xux->taruM)) {
		fprintf(stderr, "AAARRRRGG xux->taruM at line %d\n", __LINE__);
		return;
	}
	taru_set_preview_fd(xux->taruM, fd);
}

void
xformat_set_ofd(XFORMAT * xux, int fd)
{ 
	XFORMAT_E_DEBUG("");
	xux->ofdM = fd;
}

void
xformat_set_ifd(XFORMAT * xux, int fd)
{ 
	XFORMAT_E_DEBUG("");
	xux->ifdM = fd;
}

SWVARFS *
xformat_get_swvarfs(XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return xux->swvarfsM;
}

int
xformat_get_preview_fd(XFORMAT * xux)
{ 
	XFORMAT_E_DEBUG("");
	return taru_get_preview_fd(xux->taruM);
}

int
xformat_get_ofd(XFORMAT * xux)
{ 
	XFORMAT_E_DEBUG("");
	return xux->ofdM;
}

int
xformat_get_ifd(XFORMAT * xux)
{ 
	XFORMAT_E_DEBUG("");
	return xux->ifdM;
}

void
xformat_set_pass_fd(XFORMAT * xux, int fd)
{ 
	XFORMAT_E_DEBUG("");
	taruib_set_fd(fd);
}

int
xformat_get_pass_fd(XFORMAT * xux)
{ 
	XFORMAT_E_DEBUG("");
	return taruib_get_fd();
}
	
int
xformat_clear_pass_buffer(XFORMAT * xux)
{ 
	XFORMAT_E_DEBUG("");
	return taruib_clear_buffer();
}
	
char *
xformat_get_next_dirent(XFORMAT * xux, struct stat * st)
{
	char * name;
	XFORMAT_E_DEBUG("");
	name = swvarfs_get_next_dirent(xux->swvarfsM, st);
	xux->eoaM = xux->swvarfsM->eoaM;
	return name;
}

void
xformat_decrement_bytes_written(XFORMAT * xux, int amount)
{
	xux->bytes_writtenM -= amount;
}

int xformat_setdir(XFORMAT * xux, char * path)
{
	XFORMAT_E_DEBUG("");
	return swvarfs_setdir(xux->swvarfsM, path);
}

intmax_t 
xformat_read_file_data(XFORMAT * xux, int dst_fd)
{
	struct  new_cpio_header *file_hdr0;
	intmax_t ret; 
	intmax_t retval; 

	XFORMAT_E_DEBUG("");
	file_hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux));
	ret = taru_write_archive_member_data(xux->taruM, file_hdr0,
		dst_fd, xux->ifdM, (int(*)(int))NULL, xux->format_codeM, -1, NULL);
	if (ret < 0)
       		return ret;
	retval = ret;
	ret = taru_tape_skip_padding(xux->ifdM, file_hdr0->c_filesize,
						xux->format_codeM);
	if (ret < 0)
       		return -retval;
	retval += ret;
	XFORMAT_E_DEBUG("LEAVING");
	return retval;
}

intmax_t
xformat_write_file_data(XFORMAT * xux, int source_fd)
{
	intmax_t ret;
	struct new_cpio_header *file_hdr0;
	    
	XFORMAT_E_DEBUG("");
	file_hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux)); 
	ret = taru_write_archive_member_data(xux->taruM, file_hdr0,
				xux->ofdM, source_fd, (int(*)(int))NULL,
					xux->output_format_codeM, -1, NULL);
	if (ret < 0) return ret;
	if (source_fd == xux->ifdM ) { 
		taru_tape_skip_padding(xux->ifdM, file_hdr0->c_filesize,
						xux->output_format_codeM);
	}
	xux->bytes_writtenM += ret;
	XFORMAT_E_DEBUG("LEAVING");
	return ret;
}

int
xformat_write_header(XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return xformat_write_header_wn(xux, (char*)NULL);
}

int
xformat_write_header_wn(XFORMAT * xux, char * name)
{
	struct new_cpio_header *hdr1;
        struct  new_cpio_header * hdr0;
	int ret;
	
	XFORMAT_E_DEBUG("");
        hdr0 = ahsStaticCreateFilehdr();
	ret = common_setup_write_header(xux, &hdr1, hdr0, (struct stat*)(NULL), name, (char*)NULL);
	if (ret == 0) {
		ret = taru_write_archive_member_header(xux->taruM, 
						NULL, hdr1, 
						xux->link_recordM,
						xux->deferM, 
						xux->use_false_inodesM,
						xux->ofdM,
						xux->output_format_codeM, hdr0, 
					xux->taruM->taru_tarheaderflagsM);
		ahsStaticDeleteFilehdr(hdr0);
		if (ret < 0) return ret;
		xux->bytes_writtenM += ret;
		XFORMAT_E_DEBUG("LEAVING");
		return ret;
	} else {
		ahsStaticDeleteFilehdr(hdr0);
		XFORMAT_E_DEBUG("LEAVING");
		return -1;
	}
}

intmax_t
xformat_write_file_by_fd (XFORMAT * xux,
			struct stat *t,
			char * name,
			int(*fout)(int),
			int source_fd) 
{
        struct  new_cpio_header * hdr0 = ahsStaticCreateFilehdr();
	struct new_cpio_header *hdr1;
	intmax_t ret;
	intmax_t ret1;
	XFORMAT_E_DEBUG("");

        if (source_fd < 0 && fout == NULL) {
		/*
		//
		// Legacy.
		//
		*/
		ret = xformat_write_file(xux, t, name, (char*)NULL);
		if (ret < 0) return ret;
		ahsStaticDeleteFilehdr(hdr0);
		return ret;
	}
	if((ret=common_setup_write_header(xux, &hdr1,
				hdr0, t, name, (char*)NULL)) == 0) {
		XFORMAT_E_DEBUG("");
		if ((ret=taru_write_archive_member_header(xux->taruM, NULL, 
					hdr1, 
					xux->link_recordM, 
					xux->deferM, 
					xux->use_false_inodesM, 
					xux->ofdM, 
					xux->output_format_codeM, 
					hdr0, 
					xux->taruM->taru_tarheaderflagsM)) > 0)
		{
			XFORMAT_E_DEBUG("");
			ret1 = taru_write_archive_member_data(xux->taruM,
					hdr0, xux->ofdM, source_fd,
					(int(*)(int))fout,
					xux->output_format_codeM, -1, NULL);
			if (ret1 < 0) {
				fprintf(stderr, "%s: error writing file member data\n",
					swlib_utilname_get());
				return -1;
			}
			ret += ret1;
		} else {
			fprintf(stderr, "%s: tar header write failed,  return code=%s\n",
				swlib_utilname_get(),
					swlib_imaxtostr(ret, NULL));
			return -1;
		}
	} else {
		fprintf(stderr, "common_setup_write_header failed\n");
		return -1;
	}
	xux->bytes_writtenM += ret;
	ahsStaticDeleteFilehdr(hdr0);
	XFORMAT_E_DEBUG("LEAVING");
        return ret;
}

intmax_t
xformat_write_by_name(XFORMAT * xux, char * name, struct stat *st)
{
	intmax_t ret;
        struct  new_cpio_header * hdr0 = ahsStaticCreateFilehdr();
	
	XFORMAT_E_DEBUG("");

	taru_statbuf2filehdr(hdr0, st, (char*)NULL, name, (char*)NULL);
	ret = taru_write_archive_member(xux->taruM, name,
					(struct stat*)(NULL) /* st */,
					hdr0,
					xux->link_recordM, 
					xux->deferM,
					xux->use_false_inodesM,
					xux->ofdM, 
					-1,
					xux->output_format_codeM,
					xux->taruM->taru_tarheaderflagsM	
					);
	
	if (ret > 0) xux->bytes_writtenM += ret;
	XFORMAT_E_DEBUG("LEAVING");
	ahsStaticDeleteFilehdr(hdr0);
	return ret;
}

intmax_t
xformat_write_by_fd(XFORMAT * xux, int srcfd,
			struct new_cpio_header * file_hdr)
{
	intmax_t ret;
	XFORMAT_E_DEBUG("");
	ret = taru_write_archive_member(xux->taruM, (char*)(NULL),
					(struct stat*)(NULL),
					file_hdr,
					xux->link_recordM, 
					xux->deferM,
					xux->porinodeM,
					xux->ofdM, 
					srcfd,
					xux->output_format_codeM,
					xux->taruM->taru_tarheaderflagsM	
					);
	if (ret > 0) xux->bytes_writtenM += ret;
	XFORMAT_E_DEBUG("LEAVING");
	return ret;
}

intmax_t
xformat_write_file(XFORMAT * xux, struct stat *t, char * name, char * source)
{
        intmax_t ret=0, ret1, fd=-1; 
        struct new_cpio_header * hdr0 = ahsStaticCreateFilehdr();
	struct new_cpio_header *hdr1;
		
	XFORMAT_E_DEBUG("");
	ret = common_setup_write_header(xux, &hdr1, hdr0, t, name, source);
    	/*
	* hdr1 is now the object vfile header.
	*/

	if (!name) {
                  name = ahsStaticGetTarFilename(hdr1);
	}

	if (ret < 0) return ret;
        if (!source) source = name; 
	/*
	//GDB: printf "%s", taru_header_dump_string_s(hdr1, "")
	//
	// FIXME hdr1->c_nlink is not set >1 for hard links.
	//
	*/
	if (strlen(ahsStaticGetTarLinkname(hdr1)) == 0) {
		switch (hdr1->c_mode & CP_IFMT)
	        {
	            case CP_IFREG:
			XFORMAT_E_DEBUG("Running open");
	                fd = local_v_open(xux, source, O_RDONLY, 0);
	                if (fd < 0) {
	                  fprintf(stderr,"open %s: %s\n",
					source, strerror(errno));
			  ahsStaticDeleteFilehdr(hdr0);
	                  return -ret;
	                }
	        }
	}

	XFORMAT_E_DEBUG("calling taru_write_archive_member_header");
	ret=taru_write_archive_member_header(xux->taruM, NULL, 
					hdr1, 
					xux->link_recordM, 
					xux->deferM, 
					xux->use_false_inodesM, 
					xux->ofdM, 
					xux->output_format_codeM, 
					hdr0, 
					xux->taruM->taru_tarheaderflagsM);
	XFORMAT_E_DEBUG2("taru_write_archive_member_header ret=%d", ret);
        if (ret > 0 && fd >= 0) {
		XFORMAT_E_DEBUG("calling taru_write_archive_member_data");
		ret1 = taru_write_archive_member_data(xux->taruM, hdr0,
						xux->ofdM, fd,
						(int(*)(int))NULL,
						xux->output_format_codeM, -1, NULL);
		if (ret1 < 0) return -ret;
		ret += ret1;
		local_v_close(xux, fd); 
	}
	if (ret > 0) xux->bytes_writtenM += ret;
	ahsStaticDeleteFilehdr(hdr0);
	XFORMAT_E_DEBUG2("LEAVING with ret=%d", ret);
	return ret;
}

void
xformat_write_archive_stats(XFORMAT * xux, char * name, int fd)
{
	char * format;
	char buf[UINTMAX_STRSIZE_BOUND];
	char buf2[UINTMAX_STRSIZE_BOUND];
	char * buf_p;
	char * buf2_p;
	STROB * tmp = strob_open(10);

	switch(xux->output_format_codeM) {
		case arf_ustar:
			format="ustar";
			if (xux->taruM->taru_tarheaderflagsM &
					TARU_TAR_GNU_OLDGNUTAR) {
				format="gnutar";
			}
			break;
		case arf_newascii:
			format="newc";
			break;
		case arf_crcascii:
			format="crc";
			break;
		case arf_oldascii:
			format="odc";
			break;
		case arf_unknown:
		case arf_binary:
		case arf_tar: 
		case arf_hpoldascii: 
		case arf_hpbinary: 
		case arf_filesystem:
			default:
			format="unknown";
			break;
	}

	buf_p = umaxtostr(xux->bytes_writtenM, buf);
	buf2_p = umaxtostr((uintmax_t)(xux->bytes_writtenM - xux->trailer_bytesM), buf2);

	swlib_writef(fd, tmp, "%s: %s vol 1, %s+%d %s octets out.\n", 
		name, format, buf2_p, xux->trailer_bytesM, buf_p);
	strob_close(tmp);
}

int
xformat_write_trailer (XFORMAT * xux)
{
	int ret = 0, ret1;
		
	XFORMAT_E_DEBUG("");
	if (xux->deferM && (int)(xux->output_format_codeM) != (arf_ustar)) {
		ret = defer_writeout_final_defers(xux->deferM, xux->ofdM);
		if (ret < 0) return ret;
	}
	ret1 = taru_write_archive_trailer(xux->taruM,
				xux->output_format_codeM, xux->ofdM, 
				0 /*use blocksize from flags*/,
				xux->bytes_writtenM,
				xux->taruM->taru_tarheaderflagsM);
	if (ret1 < 0) return ret1;
	ret += ret1;
	xux->bytes_writtenM += ret;
	XFORMAT_E_DEBUG("LEAVING");
	xux->trailer_bytesM = ret;
	return ret;
}

int 
xformat_u_open_file(XFORMAT * xux, char * name)
{
	int fd = swvarfs_u_open(xux->swvarfsM, name);
	
	XFORMAT_E_DEBUG("");
	if (fd <= 0) {
		return fd;
	}
	fd = u_open_file_common(xux, fd);
	XFORMAT_E_DEBUG("LEAVING");
	return fd;
}

int
xformat_u_lstat(XFORMAT * xux, char * path, struct stat * st)
{
	XFORMAT_E_DEBUG("");
	return swvarfs_u_lstat(xux->swvarfsM, path, st);
}

int
xformat_u_fstat(XFORMAT * xux, int fd, struct stat * st)
{
	XFORMAT_E_DEBUG("");
	return swvarfs_u_fstat(xux->swvarfsM, fd, st);
}

int
xformat_u_readlink(XFORMAT * xux, char * path, char * buf, size_t bufsize)
{
	XFORMAT_E_DEBUG("");
	return swvarfs_u_readlink(xux->swvarfsM, path, buf, bufsize);
}

int 
xformat_u_close_file(XFORMAT * xux, int fd)
{
	XFORMAT_E_DEBUG("");
	return swvarfs_u_close(xux->swvarfsM, fd);
}

int 
xformat_open_archive(XFORMAT * xux,
			char * pathname,
			int flags,
			mode_t mode)
{
	XFORMAT_E_DEBUG("");
	xux->swvarfsM = swvarfs_open(pathname, flags, mode);
	if (xux->swvarfsM == NULL) return -1; 
	open_archive_init(xux);
	return 0;
}

int 
xformat_open_archive_stdin(XFORMAT * xux,
			char * pathname,
			int flags)
{
	int ret;
	XFORMAT_E_DEBUG("");
	ret = internal_open_archive_file((mode_t)0, xux, pathname, flags, 0);
	return ret;
}

int 
xformat_open_archive_regfile(XFORMAT * xux,
			char * pathname,
			int flags,
			mode_t mode)
{
	int ret;
	XFORMAT_E_DEBUG("");
	ret = internal_open_archive_file(S_IFREG, xux, pathname, flags, mode);
	return ret;
}

int 
xformat_open_archive_dirfile(XFORMAT * xux,
		char * pathname,
		int flags,
		mode_t mode)
{
	int ret;
	XFORMAT_E_DEBUG("");
	ret = internal_open_archive_file(S_IFDIR, xux, pathname, flags, mode);
	return ret;
}

int
xformat_close_archive(XFORMAT * xux)
{
	int ret = 0;
	ret = swvarfs_close(xux->swvarfsM);
	xux->swvarfsM = NULL;
	xux->ifdM = -1;
	return ret;
}

int 
xformat_open_archive_by_fd_and_name(XFORMAT * xux, int fd, int flags, mode_t mode, char * name)
{
	
	XFORMAT_E_DEBUG("");

	if (name == NULL)
		return xformat_open_archive_by_fd(xux, fd, flags, mode);

	/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
	xux->swvarfsM = swvarfs_opendup_with_name(fd, flags, mode, name);
	if (xux->swvarfsM == NULL) return -1; 
	open_archive_init(xux);
	return 0;
}

int 
xformat_open_archive_by_fd(XFORMAT * xux, int fd, int flags, mode_t mode)
{
	
	XFORMAT_E_DEBUG("");
	xux->swvarfsM = swvarfs_opendup(fd, flags, mode);
	if (xux->swvarfsM == NULL) return -1; 
	open_archive_init(xux);
	return 0;
}

int 
xformat_open_archive_by_swvarfs(XFORMAT * xux, SWVARFS * sfs)
{
	
	XFORMAT_E_DEBUG("");
	xux->swvarfsM = sfs;
	if (xux->swvarfsM == NULL) return -1; 
	open_archive_init(xux);
	xux->swvarfs_is_externalM = 1;
	return 0;
}

int 
xformat_read_header(XFORMAT * xux)
{
	int ret = 0;
	struct  new_cpio_header * hdr0;
	hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux)); 
	XFORMAT_E_DEBUG("");
	ret = taru_read_header(xux->taruM, hdr0, xux->ifdM,
				xux->format_codeM, &xux->eoaM,
				xux->taruM->taru_tarheaderflagsM);
	xux->last_header_sizeM = ret;
	return ret;
}

int 
xformat_unread_header(XFORMAT * xux){
	int ret;

	XFORMAT_E_DEBUG("");
	ret = xux->last_header_sizeM;
	if (xux->last_header_sizeM <= 0) return -1;
	XFORMAT_E_DEBUG2("lseeking back %d bytes.", ret);
	if (uxfio_lseek(xux->ifdM, -ret, UXFIO_SEEK_VCUR) < 0) {
		XFORMAT_E_FAIL("");
	}
	xux->last_header_sizeM = -1;
	return ret;
}

int 
xformat_read(XFORMAT * xux, void * buf, size_t count)
{
	XFORMAT_E_DEBUG("");
	return uxfio_sfread(xux->ifdM, buf, count);
}

intmax_t
xformat_copy_pass_thru(XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return xformat_copy_pass(xux, xux->ofdM, xux->ifdM);
}

intmax_t
xformat_copy_pass_by_dst(XFORMAT * xux, int dst_fd)
{
	XFORMAT_E_DEBUG("");
	return xformat_copy_pass(xux, dst_fd, xux->ifdM);
}

intmax_t 
xformat_copy_pass_md5(XFORMAT * xux, int dst_fd, int src_fd, char * md5buf)
{
	intmax_t retval=0;
	FILE_DIGS * digs;
	FILE_DIGS * old_digs;
	struct  new_cpio_header * file_hdr0;

	file_hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux)); 
	
	XFORMAT_E_DEBUG("");
	if ((file_hdr0->c_mode & CP_IFMT) == CP_IFLNK) {
		if (xux->format_codeM <= (int)(arf_crcascii)) {
			return 0;
		}
	}
     
	if (dst_fd < 0) {
		digs = NULL;
		if (taru_read_amount(src_fd, file_hdr0->c_filesize) !=
					(intmax_t)(file_hdr0->c_filesize)) {
			XFORMAT_E_FAIL("");
			retval = -3;
		}
	} else {
		digs = taru_digs_create();
		taru_digs_init(digs, DIGS_ENABLE_OFF, 0);
		digs->do_md5 = DIGS_ENABLE_ON;
		old_digs = file_hdr0->digsM;
		file_hdr0->digsM = digs;
		retval = taru_write_archive_member_data(xux->taruM,
				file_hdr0, dst_fd, src_fd,
				(int(*)(int))NULL,
				xux->output_format_codeM, -1, digs); 
		file_hdr0->digsM = old_digs;
	}

	if (uxfio_getfd(xux->ifdM, (int*)(NULL)) == src_fd ||
						xux->ifdM == src_fd ) { 
		taru_tape_skip_padding(xux->ifdM,
				file_hdr0->c_filesize, xux->format_codeM);
	}
	if (md5buf && digs) {
		strcpy(md5buf, digs->md5);
	}
	if (retval > 0) xux->bytes_writtenM += retval;
	if (digs) taru_digs_delete(digs);
	return retval;
}

intmax_t
xformat_copy_pass(XFORMAT * xux, int dst_fd, int src_fd)
{
	intmax_t ret;
	ret = xformat_copy_pass_digs(xux, dst_fd, src_fd, (FILE_DIGS *)NULL);
	return ret;
}

intmax_t
xformat_copy_pass_digs(XFORMAT * xux, int dst_fd, int src_fd, FILE_DIGS * digs)
{
	intmax_t retval=0;
	struct  new_cpio_header *file_hdr0;

	file_hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux)); 

	XFORMAT_E_DEBUG("");
	if ((file_hdr0->c_mode & CP_IFMT) == CP_IFLNK) {
		if (xux->format_codeM <= (int)(arf_crcascii)) {
			return 0;
		}
	}
     
	XFORMAT_E_DEBUG("");
	if (dst_fd < 0) {
		XFORMAT_E_DEBUG("");
		if (taru_read_amount(src_fd, file_hdr0->c_filesize) !=
					(intmax_t)(file_hdr0->c_filesize)) {
			XFORMAT_E_FAIL("");
			retval = -3;
		}
	} else {
		XFORMAT_E_DEBUG("");
		retval = taru_write_archive_member_data(xux->taruM,
				file_hdr0, dst_fd, src_fd,
				(int(*)(int))NULL,
				xux->output_format_codeM, -1, digs); 
	} 
	XFORMAT_E_DEBUG("");

	if (uxfio_getfd(xux->ifdM, (int*)(NULL)) == src_fd ||
						xux->ifdM == src_fd ) { 
		XFORMAT_E_DEBUG("");
		taru_tape_skip_padding(xux->ifdM,
				file_hdr0->c_filesize, xux->format_codeM);
	}
	if (retval > 0) xux->bytes_writtenM += retval;
	XFORMAT_E_DEBUG2("retval=%d", retval);
	return retval;
}

intmax_t 
xformat_copy_pass_file_data(XFORMAT * xux, int dst_fd, int src_fd)
{
	intmax_t retval=0;
	struct  new_cpio_header *file_hdr0;

	file_hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux)); 

	XFORMAT_E_DEBUG("");
	if ((file_hdr0->c_mode & CP_IFMT) == CP_IFLNK) {
		if (xux->format_codeM <= (int)(arf_crcascii)) {
			return 0;
		}
	}
     
	if (dst_fd < 0) {
		if (taru_read_amount(src_fd, file_hdr0->c_filesize) !=
					(intmax_t)(file_hdr0->c_filesize)) {
			XFORMAT_E_FAIL("");
			retval = -3;
		}
	} else {
		retval = taru_write_archive_file_data(xux->taruM,
				file_hdr0, dst_fd, src_fd,
				(int(*)(int))NULL,
				xux->output_format_codeM, -1); 
		} 

	if (uxfio_getfd(xux->ifdM, (int*)(NULL)) == src_fd ||
						xux->ifdM == src_fd ) { 
		taru_tape_skip_padding(xux->ifdM,
				file_hdr0->c_filesize, xux->format_codeM);
	}
	if (retval > 0) xux->bytes_writtenM += retval;
	return retval;
}

intmax_t 
xformat_copy_pass2(XFORMAT * xux, int dst_fd, int src_fd, int adjunct_ofd)
{
	intmax_t retval=0;
	struct  new_cpio_header * file_hdr0;

	file_hdr0 = (struct new_cpio_header *)(xformat_vfile_hdr(xux)); 

	XFORMAT_E_DEBUG("");
	if ((file_hdr0->c_mode & CP_IFMT) == CP_IFLNK) {
		if (xux->format_codeM <= (int)(arf_crcascii)) {
			return 0;
		}
	}
     
	if (dst_fd < 0) {
		retval = -3;
	} else {
		retval = taru_write_archive_member_data(xux->taruM,
				file_hdr0, dst_fd, src_fd,
				(int(*)(int))NULL, xux->output_format_codeM,
				adjunct_ofd, NULL); 
	} 

	if (uxfio_getfd(xux->ifdM, (int*)(NULL)) == src_fd ||
						xux->ifdM == src_fd ) { 
		taru_tape_skip_padding(xux->ifdM, file_hdr0->c_filesize,
							xux->format_codeM);
	}

	if (retval > 0) xux->bytes_writtenM += retval;
	return retval;
}

char
xformat_get_tar_typeflag (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_tar_typeflag(xux->ahsM);
}

int
xformat_file_has_data (XFORMAT * xux)
{
	/* return (xformat_get_tar_typeflag(xux) == REGTYPE);
	*/
	XFORMAT_E_DEBUG("");
	return swvarfs_file_has_data(xux->swvarfsM);
}

unsigned
xformat_get_tar_chksum (XFORMAT * xux, void * tarhdr)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_tar_chksum(xux->ahsM, tarhdr);
}

char *
xformat_get_username (XFORMAT * xux, char * buf)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_system_username(xux->ahsM, buf);
}

char *
xformat_get_groupname (XFORMAT * xux, char * buf)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_system_groupname(xux->ahsM, buf);
}

char *
xformat_get_tar_username (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_tar_username(xux->ahsM);
}

char *
xformat_get_tar_groupname (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_tar_groupname(xux->ahsM);
}

void
xformat_set_sys_db_u_policy(XFORMAT * xux, int c)
{
	XFORMAT_E_DEBUG("");
	ahs_set_sys_db_u_policy(xux->ahsM, c);
}

void
xformat_set_sys_db_g_policy(XFORMAT * xux, int c)
{
	XFORMAT_E_DEBUG("");
	ahs_set_sys_db_g_policy(xux->ahsM, c);
}

intmax_t
xformat_get_filesize (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_filesize(xux->ahsM);
}

time_t
xformat_get_mtime (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_mtime(xux->ahsM);
}

char *
xformat_get_name(XFORMAT * xux, STROB * buf)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_name(xux->ahsM, buf);
}

char *
xformat_get_linkname (XFORMAT * xux, char * buf)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_linkname(xux->ahsM, buf);
}

mode_t
xformat_get_mode (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_mode(xux->ahsM);
}

mode_t
xformat_get_perms (XFORMAT * xux)
{
	XFORMAT_E_DEBUG("");
	return ahs_get_perms(xux->ahsM);
}

int
xformat_get_virtual_eof (XFORMAT * xux)
{ 
	XFORMAT_E_DEBUG("");
	return -1;  /* // Can't get it;  */
}

int
xformat_get_layout_type(XFORMAT * xux) { 
       /* FIXME, should should object accessor functions */
	XFORMAT_E_DEBUG("");
	return xux->swvarfsM->format_descM->layout_typeM;
}

char *
xformat_get_source_filename (XFORMAT * xux, char * buf) {
	XFORMAT_E_DEBUG("");
	return ahs_get_source_filename(xux->ahsM, buf);
}

int
xformat_is_end_of_archive(XFORMAT * xux) {
	int ret;
	int trailer_blocks_read_return;
	char * name;
		
	XFORMAT_E_DEBUG("");
	if ( 
		(int)(xux->format_codeM) == (arf_ustar) ||
		(int)(xux->format_codeM) == (arf_tar)
	) {
		ret = xux->eoaM;
	} else {
		name = xformat_get_name(xux, (STROB*)(NULL));
		ret =  ! strncmp(CPIO_INBAND_EOA_FILENAME, name, 25);
	}

	if (ret) {
		/*
		* // read the null trailer blocks so the they get put in
		* // the taruib buffer object.
		*/

		/* the following is now superfulous since the entirety of the trailers
		 * blocks are being read.
		 */
		if (xformat_get_pass_fd(xux)) {
			trailer_blocks_read_return = taru_read_amount(xformat_get_ifd(xux), -1);
			XFORMAT_E_DEBUG2("unread trailer blocks = [%d]", trailer_blocks_read_return);
		}
	}
	return ret;
}

void
xformat_set_strip_leading_slash(XFORMAT * xux, int n) {
	xformat_set_tarheader_flag(xux, TARU_TAR_DO_STRIP_LEADING_SLASH, n);
}

int xformat_get_tarheader_flags(XFORMAT * xux)
{
	return xux->taruM->taru_tarheaderflagsM;
}	

void
xformat_set_tarheader_flags(XFORMAT * xux, int flags)
{
	if (xux->swvarfsM) 
		swvarfs_set_tarheader_flags(xux->swvarfsM, flags);
	xux->taruM->taru_tarheaderflagsM = flags;
}

void
xformat_set_tarheader_flag(XFORMAT * xux, int flag, int n)
{
	if (xux->swvarfsM) 
		swvarfs_set_tarheader_flag(xux->swvarfsM, flag, n);
	taru_set_tarheader_flag(xux->taruM, flag, n);
}

void
xformat_set_numeric_uids(XFORMAT * xux, int n)
{
	xformat_set_tarheader_flag(xux, TARU_TAR_NUMERIC_UIDS, n);
}

void
xformat_set_false_inodes(XFORMAT * xux, int n)
{
	XFORMAT_E_DEBUG("");
	xux->make_false_inodesM = n;
	xux->use_false_inodesM = return_do_inodes(xux);
}

void
xformat_reset_bytes_written(XFORMAT * xux)
{
	xux->bytes_writtenM = 0;
}
