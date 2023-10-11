/* ahs.c - Archive header accessor.

   Copyright (C) 1998, 1999 Jim Lowe
 
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
#include "ahs.h"
#include "strob.h"
#include "swlib.h"
#include "swutilname.h"
#include "cpiohdr.h"

#include "debug_config.h"
#ifdef AHSNEEDDEBUG
#define AHS_E_DEBUG(format) SWBISERROR("AHS DEBUG: ", format)
#define AHS_E_DEBUG2(format, arg) SWBISERROR2("AHS DEBUG: ", format, arg)
#define AHS_E_DEBUG3(format, arg, arg1) SWBISERROR3("AHS DEBUG: ", format, arg, arg1)
#else
#define AHS_E_DEBUG(arg)
#define AHS_E_DEBUG2(arg, arg1)
#define AHS_E_DEBUG3(arg, arg1, arg2)
#endif /* AHSNEEDDEBUG */


#ifndef S_ISVTX
#define S_ISVTX	00
#endif

static STROB * s_tmpM = NULL;
static STROB * s_error_owner_storeM = NULL;
static STROB * s_error_group_storeM = NULL;

/*	
  private: // ---------------- Private Functions ---------------
*/

static
int
is_numeric_name(char * name)
{
	char * s;
	s = name;
	if (!s || *s == 0) return 0;
	while (s && *s) {
		if (isalpha((int)*s)) {
			return 0;
		}
		s++;
	}
	return 1;
}

static void
ahsStaticSetTarname_length_i(STROB ** c_sb, int len)
{
	AHS_E_DEBUG("");
	if (*c_sb == NULL) {
		*c_sb = strob_open(10);
	}
	strob_set_memlength(*c_sb, len);
}


static void
ahsStaticSetTarname_i(STROB ** c_sb, char * name, int len)
{
	AHS_E_DEBUG("");
	if (name == NULL) {
		if (*c_sb)
			strob_close(*c_sb);
		*c_sb = (STROB*)NULL;
		return;
	}
	if (*c_sb == NULL) {
		*c_sb = strob_open(100);
	}
	if (len < 0) {
		/* Nul terminated of undetermined length */
		if (name != (char*)((*c_sb)->str_)) strob_strcpy(*c_sb, name);
	} else {
		/* Copy at most len bytes, NUL terminate at len+1 */
		strob_set_memlength(*c_sb, len+1);
		if (name != (char*)((*c_sb)->str_)) strob_strncpy(*c_sb, name, len);
		*(strob_str(*c_sb) + len) = '\0';
	}
	return;
}

static char *
ahsStaticGetTarname_i(STROB ** c_sb)
{
	AHS_E_DEBUG("");
	if (*c_sb == (STROB*)NULL) {
		ahsStaticSetTarname_i(c_sb, "", -1);
	}
	return strob_str(*c_sb);
}

static void 
ahsStatic_strip_leading_slash_i(char *name)
{
	char *p;
	/* Strip leading `/' from the filename.  */

	AHS_E_DEBUG("");
	p = name;
	while (*p == '/' && *(p + 1)) {
		++p;
	}
	if (p != name) memmove(name, p, strlen(p) + 1);
}

static
int
count_newlines(char * store, char * key)
{
	int ret = 0;
	char * s;
	char * n;
	
	s = store;

	if (key && strstr(store, key) == store) {
		/*
		 *  Its the first message,
		 *  return 0
		 */
		 return 0;
	}
	while ((n=strchr(s, '\n'))) {
		ret++;
		s = n + 1;
		if (key && strstr(s, key) == s) {
			/*
			 * Stop because the answer we want is the
			 * number of messages prior to the 'key' message.
			 */
			break;
		}
	}
	return ret;
}

static
int
error_msg_control(char * name, int id, char * dbname, int * is_new)
{
	int n;
	STROB * tmp;
	STROB * store;

	if (s_tmpM == NULL) s_tmpM = strob_open(80);
	if (s_error_owner_storeM == NULL) s_error_owner_storeM = strob_open(80);
	if (s_error_group_storeM == NULL) s_error_group_storeM = strob_open(80);

	tmp = s_tmpM;
	if (strcmp(dbname, "user") == 0) {
		store = s_error_owner_storeM;
	} else {
		store = s_error_group_storeM;
	}

	if (name) {
		strob_sprintf(tmp, 0, "%s: %s name [%s] not found in system database\n",
					 swlib_utilname_get(), dbname, name);
	} else {
		strob_sprintf(tmp, 0,
			"%s: %s id [%d] not found in system database\n",
				swlib_utilname_get(), dbname, (int)(id));
	}
	if (strstr(strob_str(store), strob_str(tmp))) {
		/*
		 * not a new error mesasge
		 */
		if (is_new) *is_new = 0;
		n = count_newlines(strob_str(store), strob_str(tmp));
	} else {
		/*
		 * new error message
		 */
		strob_strcat(store, strob_str(tmp));	
		/* fprintf(stderr, "%s", strob_str(tmp)); */
		if (is_new) *is_new = 1;
		n = count_newlines(strob_str(store), NULL);
	}	
	return n;	
}

/*	
  public: // ---------------- Public Functions ---------------
*/

int
ahs_copy(AHS * ahs_to, AHS * ahs_from)
{
	AHS_E_DEBUG("");
	taru_filehdr2filehdr(ahs_to->file_hdrM, ahs_from->file_hdrM);
	/* FIXME */
	return 0;
}

struct new_cpio_header *
ahsStaticCreateFilehdr(void)
{
	struct new_cpio_header * file_hdr = taru_make_header();
	AHS_E_DEBUG("");
	ahsStaticSetPaxLinkname(file_hdr, NULL);
	ahsStaticSetTarFilename(file_hdr, NULL);
	ahsStaticSetTarUsername(file_hdr, NULL);
	ahsStaticSetTarGroupname(file_hdr, NULL);
	return file_hdr;
}

void
ahsStaticDeleteFilehdr(struct new_cpio_header * file_hdr)
{
	AHS_E_DEBUG("");
	ahsStaticSetPaxLinkname(file_hdr, NULL);
	ahsStaticSetTarFilename(file_hdr, NULL);
	ahsStaticSetTarUsername(file_hdr, NULL);
	ahsStaticSetTarGroupname(file_hdr, NULL);
	taru_free_header(file_hdr);
}

void
ahsStaticSetTarLinknameLength(struct new_cpio_header * file_hdr, int len)
{
	AHS_E_DEBUG("");
	ahsStaticSetTarname_length_i((STROB**)(&(file_hdr->c_tar_linkname)), len);
}

void
ahsStaticSetTarFilenameLength(struct new_cpio_header * file_hdr, int len)
{
	AHS_E_DEBUG("");
	ahsStaticSetTarname_length_i((STROB**)(&(file_hdr->c_name)), len);
}

void 
ahsStatic_strip_name_leading_slash(struct new_cpio_header * file_hdr)
{
	char * name;
	
	AHS_E_DEBUG("");
	name = ahsStaticGetTarFilename(file_hdr);
	ahsStatic_strip_leading_slash_i(name);
}

char *
ahsStaticGetTarLinkname(struct new_cpio_header * file_hdr)
{
	AHS_E_DEBUG("");
	return ahsStaticGetTarname_i((STROB**)&(file_hdr->c_tar_linkname));
}

char *
ahsStaticGetTarFilename(struct new_cpio_header * file_hdr)
{
	AHS_E_DEBUG("");
	return ahsStaticGetTarname_i((STROB**)&(file_hdr->c_name));
}

char *
ahsStaticGetTarUsername(struct new_cpio_header * file_hdr)
{
	AHS_E_DEBUG("");
	return ahsStaticGetTarname_i((STROB**)&(file_hdr->c_username));
}

char *
ahsStaticGetTarGroupname(struct new_cpio_header * file_hdr)
{
	AHS_E_DEBUG("");
	return ahsStaticGetTarname_i((STROB**)&(file_hdr->c_groupname));
}

void
ahsStaticSetPaxLinkname(struct new_cpio_header * file_hdr, char * name)
{
	AHS_E_DEBUG("");
	ahsStaticSetTarname_i((STROB**)&(file_hdr->c_tar_linkname), name, -1);
}

void
ahsStaticSetTarLinkname(struct new_cpio_header * file_hdr, char * name)
{
	AHS_E_DEBUG("");
	/*
 	 * Deliberately store more than TARLINKNAMESIZE so the routines that censor
 	 * this size will see the overlength field and complain.
 	 * Use ahsStaticSetPaxLinkname() to set a  long link name
 	 */
	ahsStaticSetTarname_i((STROB**)&(file_hdr->c_tar_linkname), name, TARLINKNAMESIZE);
}

void
ahsStaticSetTarFilename(struct new_cpio_header * file_hdr, char * name)
{
	AHS_E_DEBUG("");
	ahsStaticSetTarname_i((STROB**)&(file_hdr->c_name),  name, -1);
}

void
ahsStaticSetTarUsername(struct new_cpio_header * file_hdr, char * name)
{
	AHS_E_DEBUG("");
	ahsStaticSetTarname_i((STROB**)&(file_hdr->c_username),  name, -1);
}

void
ahsStaticSetTarGroupname(struct new_cpio_header * file_hdr, char * name)
{
	AHS_E_DEBUG("");
	ahsStaticSetTarname_i((STROB**)&(file_hdr->c_groupname),  name, -1);
}

void ahs_close(AHS * xhs) 
{
	struct new_cpio_header * file_hdr;
	
	AHS_E_DEBUG("");
	file_hdr = ahs_vfile_hdr(xhs);
	ahsStaticDeleteFilehdr(file_hdr);
	swbis_free(xhs);
}
	
AHS * ahs_open(void) {
	AHS * xhs;
	
	AHS_E_DEBUG("");
	xhs  = (AHS *)malloc(sizeof(AHS));
	if (!xhs) return NULL;
	xhs->file_hdrM = ahsStaticCreateFilehdr();
	taru_init_header(ahs_vfile_hdr(xhs));
	return xhs;
}

void
ahs_init_header(AHS * ahs)
{
	taru_init_header(ahs_vfile_hdr(ahs));
}
	
struct new_cpio_header * ahs_vfile_hdr(AHS * xhs) {
      AHS_E_DEBUG("");
      return (struct new_cpio_header*)(xhs->file_hdrM);
}
	
char *  ahs_get_header_buffer(char * buf) {
      AHS_E_DEBUG("");
     if (buf) {
         return buf;
     } else {
         static char a[512];
         return a;
     }
}
	
void ahs_set_tar_chksum(void) { return; }
	
void ahs_set_mode(AHS * xhs, mode_t mode) { 
      AHS_E_DEBUG("");
      ahs_vfile_hdr(xhs)->c_mode = (unsigned long)(mode);
}    

void ahs_set_perms(AHS * xhs, mode_t perms) { 
	mode_t mode;
	AHS_E_DEBUG("");
	mode = ahs_get_mode(xhs);

	mode &= ~(S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX);
	mode |= perms; 
	ahs_set_mode(xhs, mode);
}    

void ahs_set_filetype_from_tartype(AHS * xhs, char  s) { 
	mode_t modet;
	AHS_E_DEBUG("");
	modet = (mode_t)(ahs_vfile_hdr(xhs)->c_mode);
	taru_set_filetype_from_tartype(s, &modet, (char*)(NULL));
	ahs_vfile_hdr(xhs)->c_mode = (unsigned long)modet;
}

void ahs_set_uid(AHS * xhs, uid_t uid) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_uid = uid;
}

void ahs_set_uid_by_name(AHS * xhs, char * username) { 
	uid_t x;
	
	AHS_E_DEBUG("");
	if (taru_get_uid_by_name(username, &x)) {
		AHS_E_DEBUG("");
		ahs_vfile_hdr(xhs)->c_uid = AHS_UID_NOBODY;
	} else {
		AHS_E_DEBUG("");
		ahs_vfile_hdr(xhs)->c_uid = (unsigned long)x;
	}
	/* ahs_vfile_hdr(xhs)->c_uid = taru_get_tar_user(0, username, NULL); */
}
	
void ahs_set_gid_by_name(AHS * xhs, char * groupname) { 
	gid_t x;

	AHS_E_DEBUG("");
	if (taru_get_gid_by_name(groupname, &x)) {
		AHS_E_DEBUG("");
		ahs_vfile_hdr(xhs)->c_gid = AHS_GID_NOBODY;
	} else {
		AHS_E_DEBUG("");
		ahs_vfile_hdr(xhs)->c_gid = (unsigned long)x;
	}
}

void ahs_set_gid(AHS * xhs, gid_t gid) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_gid = gid;
}
  
void ahs_set_filesize(AHS * xhs, intmax_t filesize) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_filesize = (uintmax_t)filesize;
}    

void ahs_set_nlink(AHS * xhs, int  nlink) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_nlink = nlink;
}

void ahs_set_inode(AHS * xhs, ino_t ino) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_ino = ino;
}

void ahs_set_mtime(AHS * xhs, time_t mtime) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_mtime = mtime;
}    
	
void ahs_set_devmajor(AHS * xhs, dev_t dev) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_rdev_maj = (dev);
}
    
void ahs_set_devminor(AHS * xhs, dev_t dev) { 
	AHS_E_DEBUG("");
        ahs_vfile_hdr(xhs)->c_rdev_min = (dev);
}

void ahs_set_name(AHS * xhs, char *name) { 
	AHS_E_DEBUG("");
	ahsStaticSetTarFilename(ahs_vfile_hdr(xhs), name);
}
    
void ahs_set_linkname(AHS * xhs, char *linkname) { 
	AHS_E_DEBUG("");
	ahsStaticSetPaxLinkname(ahs_vfile_hdr(xhs), linkname);
}

void ahs_set_from_statbuf(AHS * xhs, struct stat *st) { 
	AHS_E_DEBUG("");
	taru_statbuf2filehdr((struct new_cpio_header*)(ahs_vfile_hdr(xhs)), st, NULL, NULL, NULL);
}

void ahs_set_to_statbuf(AHS * xhs, struct stat *st) { 
	AHS_E_DEBUG("");
	taru_filehdr2statbuf(st, (struct new_cpio_header*)(ahs_vfile_hdr(xhs)));
}
	
void ahs_set_from_new_cpio_header(AHS * xhs, void *vfh) { 
	AHS_E_DEBUG("");
	taru_filehdr2filehdr((struct new_cpio_header*)(ahs_vfile_hdr(xhs)), (struct new_cpio_header*)(vfh));
}
	
void * ahs_get_new_cpio_header(AHS * xhs) {
	AHS_E_DEBUG("");
	return (void*)ahs_vfile_hdr(xhs);
}
	
char ahs_get_tar_typeflag(AHS * xhs) {
	AHS_E_DEBUG("");
	return taru_get_tar_filetype((mode_t)(ahs_vfile_hdr(xhs)->c_mode));
}
	
unsigned ahs_get_tar_chksum(AHS * xhs, void * tarhdr) {
	AHS_E_DEBUG("");
	return taru_tar_checksum (tarhdr);
}
	
char* 
ahs_get_system_username(AHS * xhs, char * buf) {
	AHS_E_DEBUG("");
	if (taru_get_tar_user_by_uid(ahs_vfile_hdr(xhs)->c_uid,  buf)) {
		return NULL;
	} else {
		return buf;
	}
}
	
char* 
ahs_get_system_groupname(AHS * xhs, char * buf) {
	AHS_E_DEBUG("");
	if (taru_get_tar_group_by_gid(ahs_vfile_hdr(xhs)->c_gid, buf)) {
		return NULL;
	} else {
		return buf;
	}
}

int
ahs_set_user_systempair(AHS * xhs, char * name) 
{
	int ret = 0;
	int id_decrement;
	int is_new;
	uid_t x;
	
	AHS_E_DEBUG("");
	if (!is_numeric_name(name)) {
		AHS_E_DEBUG("");
		ahs_set_tar_username(xhs,  name);
		if (taru_get_uid_by_name(name, &x) < 0) {
			id_decrement = error_msg_control(name, 0, "user", &is_new);
			x = AHS_UID_NOBODY - id_decrement;
			if (is_new)
				fprintf(stderr, "%s: warning: user [%s] not found, setting uid to %d\n", 
					swlib_utilname_get(), name, (int)(x));
			/* ahs_set_tar_username(xhs,  AHS_USERNAME_NOBODY); */
			ret = 1;
		}
		ahs_set_uid(xhs, (uid_t)(x));
	} else {
		AHS_E_DEBUG("");
		ahs_set_tar_username(xhs,  "");
		ahs_set_uid(xhs, (uid_t)(atol(name)));
	}
	return ret;
}

int
ahs_set_group_systempair(AHS * xhs, char * name)
{
	int ret = 0;
	int id_decrement;
	int is_new;
	gid_t x;

	AHS_E_DEBUG("");
	if (!is_numeric_name(name)) {
		AHS_E_DEBUG("");
		ahs_set_tar_groupname(xhs,  name);
		if (taru_get_gid_by_name(name, &x) < 0) {
			id_decrement = error_msg_control(name, 0, "group", &is_new);
			x = AHS_GID_NOBODY - id_decrement;
			if (is_new)
				fprintf(stderr, "%s: warning: group [%s] not found, setting gid to %d\n",
					 swlib_utilname_get(), name, (int)(x));
			/* ahs_set_tar_groupname(xhs,  AHS_GROUPNAME_NOBODY); */
			ret = 1;
		}
		ahs_set_gid(xhs, (gid_t)(x));
	} else {
		AHS_E_DEBUG("");
		ahs_set_tar_groupname(xhs,  "");
		ahs_set_gid(xhs, (gid_t)(atol(name)));
	}
	return ret;
}

void
ahs_set_sys_db_g_policy(AHS * xhs, int c) 
{
	AHS_E_DEBUG("");
	xhs->file_hdrM->c_cg = (unsigned char)c;
}

void
ahs_set_sys_db_u_policy(AHS * xhs, int c) 
{
	AHS_E_DEBUG("");
	xhs->file_hdrM->c_cu = (unsigned char)c;
}

void
ahs_set_tar_username(AHS * xhs, char * name) 
{
	AHS_E_DEBUG("");
	ahsStaticSetTarUsername(xhs->file_hdrM, name);
}

void
ahs_set_tar_groupname(AHS * xhs, char * name) 
{
	AHS_E_DEBUG("");
	ahsStaticSetTarGroupname(xhs->file_hdrM, name);
}

char *
ahs_get_tar_username(AHS * xhs) 
{
	AHS_E_DEBUG("");
	return ahsStaticGetTarUsername(xhs->file_hdrM);
}

char *
ahs_get_tar_groupname(AHS * xhs) 
{
	AHS_E_DEBUG("");
	return ahsStaticGetTarGroupname(xhs->file_hdrM);
}

intmax_t ahs_get_filesize(AHS * xhs) {
	AHS_E_DEBUG("");
	return taru_hdr_get_filesize(ahs_vfile_hdr(xhs));
}

time_t ahs_get_mtime(AHS * xhs) {
	AHS_E_DEBUG("");
	return (time_t)(ahs_vfile_hdr(xhs)->c_mtime);
}

char * ahs_get_name(AHS * xhs, STROB * buf) {
	char * name = ahsStaticGetTarFilename(ahs_vfile_hdr(xhs));
	AHS_E_DEBUG("");
	if (buf) {
		AHS_E_DEBUG("");
		strob_strcpy(buf, name);
		return strob_str(buf);
	} else {
		AHS_E_DEBUG("");
		return name;
	}
}

char * ahs_get_linkname(AHS * xhs, char * buf) {
	char * name = ahsStaticGetTarLinkname(ahs_vfile_hdr(xhs));
	if (strlen(name) > 99) {
		fprintf(stderr, "Warning, Link name [%s] is too long for tar format archives, truncating linkname.\n", name);
	}
	if (buf) {
		strncpy(buf, name, 100);
		buf[99] = '\0';
		name = buf;
	}
	return name;
}

mode_t ahs_get_mode(AHS * xhs) {
	return (mode_t)(ahs_vfile_hdr(xhs)->c_mode);
}

mode_t ahs_get_perms(AHS * xhs) {
	mode_t mode = ahs_get_mode(xhs);
	return mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX);
}

char * ahs_get_source_filename(AHS * xhs, char * buf) {
	return buf;
}
