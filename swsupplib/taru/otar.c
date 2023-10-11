/*  otar.c - read and write tar headers
 */
   
/*
   Copyright (C) 1985, 1992, 1993, 1994, 1996, 1997, 1999, 2000, 2001, 
   2003, 2004 Free Software Foundation, Inc.
   Copyright (C) 2003,2004,2005,2010,2014 Jim Lowe

   Portions of this code are derived from code 
   copyrighted by the Free Software Foundation.  Retention of their 
   copyright ownership is required by the GNU GPL and does *NOT* signify 
   their support or endorsement of this work.
       jhl

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
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <tar.h>
#include "system.h"
#include "swlib.h"
#include "to_oct.h"
#include "filetypes.h"
#include "cpiohdr.h"
#include "tarhdr.h"
#include "ahs.h"
#include "taru.h"
#include "uxfio.h"
#include "swutilname.h"
#include "swutillib.h"

/*#include "debug_config.h" */

#define TARRECORDSIZE 512
#define FALSE 0
#define TRUE 1

#ifndef bzero
#define bzero(s, n)     memset ((s), 0, (n))
#endif

#define CHKBLANKS       "        "      /* 8 blanks, no null */

/* Return nonzero iff all the bytes in BLOCK are NUL.
   SIZE is the number of bytes to check in BLOCK; it must be a
   multiple of sizeof (long).  */

#define NOBODY_ID		((uid_t)(07777777))
#define CACHE_LEN		10
/* #define SNAME_LEN		32  Now in taru.h */

static int dbpair_user_nset = 0;
static int dbpair_group_nset = 0;

static char * g_pwent_msg[] = {"user", "group"};

typedef struct {
	long int idM;
	STROB * snameM;
	int in_sysM;
} SYSDBPAIR;

static SYSDBPAIR userCacheM[CACHE_LEN] =  {
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1}} ;
static SYSDBPAIR groupCacheM[CACHE_LEN] = {
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1},
{0,NULL,-1}} ;


static size_t
split_long_name (const char *name, size_t length)
{
  size_t i;

  if (length > PREFIX_FIELD_SIZE)
    length = PREFIX_FIELD_SIZE+1;
  for (i = length - 1; i > 0; i--)
    if (ISSLASH (name[i]))
      break;
  return i;
}

static
void
error_msg_control(char * name, int id, char * dbname)
{
	static STROB * tmp = NULL;
	static STROB * store = NULL;
	if (tmp == NULL) tmp = strob_open(80);
	if (store == NULL) store = strob_open(80);

	if (name) {
		strob_sprintf(tmp, 0, "%s: %s name [%s] not found in system db\n",
					 swlib_utilname_get(), dbname, name);
	} else {
		strob_sprintf(tmp, 0,
			"%s: %s id [%d] not found in system db\n",
				swlib_utilname_get(), dbname, (int)(id));
	}
	if (strstr(strob_str(store), strob_str(tmp))) {
		;
	} else {
		strob_strcat(store, strob_str(tmp));	
		fprintf(stderr, "%s", strob_str(tmp));
	}	
}

static 
int 
null_block(long *block, int size)
{
	register long *p = block;
	register int i = size / sizeof(long);

	while (i--)
		if (*p++)
			return 0;
	return 1;
}

static
void 
dbcache_set(int * dbpair_plen, SYSDBPAIR * cache, char * name, int id, int in_sys)
{
	int cell;

	E_DEBUG("");
	if (*dbpair_plen >= CACHE_LEN) {
		*dbpair_plen = 1;
		cell = 0;
	} else {
		cell = *dbpair_plen;
		(*dbpair_plen)++;
	}	
	cache[cell].idM = id;
	if (cache[cell].snameM == NULL) {
		cache[cell].snameM = strob_open(32);
	}
	E_DEBUG3("strcpy cell=[%d] id=[%d]", cell, (int)id);
	E_DEBUG3("strcpy cell=[%d] name=[%s]", cell, name);
	E_DEBUG3("strcpy cell=[%d] in_sys=[%d]", cell, in_sys);
	strob_strcpy(cache[cell].snameM, name /*, TARU_SYSDBNAME_LEN -1*/);
	/* cache[cell].nameM[TARU_SYSDBNAME_LEN -1] = '\0'; */
	cache[cell].in_sysM = in_sys;
}

static
SYSDBPAIR * 
dbcache_lookup(int * dbpair_len, SYSDBPAIR * cache, char * name, int id)
{
	int i;
	SYSDBPAIR * pair;

	for(i=0; i < *dbpair_len; i++) {
		pair = cache + i;
		if (name) {
			if (pair->snameM == NULL)
				pair->snameM = strob_open(32);
			if (strob_strcmp(pair->snameM, name) == 0) {
				E_DEBUG2("found by name: %s in cache", name);
				return pair;
			}
		}
		if (id >= 0 && pair->idM == id) {
			E_DEBUG2("found by id: %d in cache", (int)id);
			return pair;
		}
	}
	return NULL;
}

static
char *
return_name_by_cache(int * dbpair_len, SYSDBPAIR * cache, int id, int * is_in_sysdb)
{
	SYSDBPAIR * pair;
	pair = dbcache_lookup(dbpair_len, cache, NULL, id);
	if (!pair) return NULL;
	*is_in_sysdb = pair->in_sysM;
	return strob_str(pair->snameM);
}


static
int 
return_id_by_cache(int * dbpair_len, SYSDBPAIR * cache, char * name, int * is_in_sysdb)
{
	SYSDBPAIR * pair;
	pair = dbcache_lookup(dbpair_len, cache, name, -1);
	if (!pair) return -1;
	*is_in_sysdb = pair->in_sysM;
	return pair->idM;
}

static 
char *
l_tar_sysdata_getuser(long uid, char *tarbuf, STROB * paxbuf)
{
	struct passwd *pwent;
	pwent = getpwuid(uid);
	if (!pwent) {
		if (tarbuf) memset(tarbuf, '\0', SNAME_LEN);
		if (paxbuf) strob_strcpy(paxbuf, "");
		return (char *) NULL;
	}
	if (tarbuf) swlib_strncpy(tarbuf, pwent->pw_name, SNAME_LEN);
	if (paxbuf) strob_strcpy(paxbuf, pwent->pw_name);
	return pwent->pw_name;
}

static 
int 
l_cache_tar_getuidbyname(char *user, long *puid)
{
	int ret;
	uid_t id;
	ret = taru_get_uid_by_name(user, &id);
	*puid = (long)id;
	return ret;
}

static 
int 
l_getuidbyname(char *user, long *puid)
{
	struct passwd *pwent;
	pwent = getpwnam(user);
	if (!pwent) {
		return -1;
	}
	*puid = pwent->pw_uid;
	return 0;
}

static 
char *
l_tar_sysdata_getgroup(long gid, char *tarbuf, STROB * paxbuf)
{
	struct group *pwent;
	pwent = getgrgid(gid);
	if (!pwent) {
		if (tarbuf) memset(tarbuf, '\0', SNAME_LEN);
		if (paxbuf) strob_strcpy(paxbuf, "");
		return (char *) NULL;
	}
	if (tarbuf) swlib_strncpy(tarbuf, pwent->gr_name, SNAME_LEN);
	if (paxbuf) strob_strcpy(paxbuf, pwent->gr_name);
	return pwent->gr_name;
}

static 
int 
l_cache_tar_getgidbyname(char *user, long *pgid)
{
	int ret;
	gid_t id;
	ret = taru_get_gid_by_name(user, &id);
	*pgid = (long)id;
	return ret;
}

static 
int 
l_getgidbyname(char *group, long *pgid)
{
	struct group *pwent;
	pwent = getgrnam(group);
	if (!pwent) {
		return -1;
	}
	*pgid = pwent->gr_gid;
	return 0;
}

static int 
get_pwent(
		int idx,
		long id, 
		char *userkey, 
		char * tarbuf,
		STROB * paxbuf,
		long *ppid, 
		int  (*v_get_id)(char *, long *),
		char * (*v_get_name)(long, char *, STROB *),
		int * dbpair_len, 
		SYSDBPAIR * cache
	)
{
	char * cname;
	int is_in_sysdb = 0;
	int n;
	int retval = 0;
	if (userkey) {
		if (strlen(userkey) == 0) return -1;
		n = return_id_by_cache(dbpair_len, cache, userkey, &is_in_sysdb);
		if (n >= 0) {
			/*
			 * The lookup was found in cache but it
			 * may not be in /etc/passwd or /etc/group
			 * i.e. The user is looking up a username that
			 * is not in the system data files.
			 */
			*ppid = n;
			return is_in_sysdb;
		}	
		n = (*(v_get_id))(userkey, ppid);
		if (n == 0) {
			/*
			 * found in system database
			 */
			dbcache_set(dbpair_len, cache, userkey, *ppid, 0 /*in sys db*/);
		} else {
			/*
			 * Not found
			 */
			*ppid = AHS_UID_NOBODY;
			error_msg_control(userkey, -1, g_pwent_msg[idx]);
			dbcache_set(dbpair_len, cache, userkey, *ppid, -1 /*not in sys db*/);
			return -1;
		}
		return n;
	} else {
		cname = return_name_by_cache(dbpair_len, cache, id, &is_in_sysdb);
		if (cname) {
			/*
 			 * Found in cache
 			 */

			if (strlen(cname) >  TARU_SYSDBNAME_LEN - 1) {
				fprintf(stderr, "%s: user name too long for ustar headers: %s\n", swlib_utilname_get(), cname);

			}
			
			retval = is_in_sysdb;
		} else {
			/*
 			 * Not found in cache, check system database
 			 */ 
			cname = (*(v_get_name))(id, NULL, NULL);
			if (cname) {
				/*
				 * found in system database
				 */
				dbcache_set(dbpair_len, cache, cname, id, 0);
				retval = 0;
			} else {
				/*
				 * Not found
				 */
				/* strob_strcpy(buf, AHS_USERNAME_NOBODY); */
				cname = AHS_USERNAME_NOBODY;
				dbcache_set(dbpair_len, cache, AHS_USERNAME_NOBODY, id, -1);
				error_msg_control((char*)NULL, id, g_pwent_msg[idx]);
				retval = -1;
			}
		}

		if (cname && tarbuf) {
			strncpy(tarbuf, cname , TARU_SYSDBNAME_LEN - 1);
			tarbuf[TARU_SYSDBNAME_LEN - 1] = '\0';
		}

		if (cname && paxbuf) {
			strob_strcpy(paxbuf, cname);
		}
		return retval;
	}
}


void
taru_set_filehdr_sysdb_nameid_by_policy(char * vselect,
		struct new_cpio_header *file_hdr,
		int termch, int tar_iflags_numeric_uids)
{
	unsigned long psid;
	unsigned long c_id;
	unsigned char c_c;
	gid_t pgid;
	uid_t puid;
	int G;
	char * sysusername;

	if (strncasecmp("G", vselect, 1) == 0) {
		G = 1; /* Group */
		c_id = file_hdr->c_gid;
		c_c = file_hdr->c_cg;
		sysusername = ahsStaticGetTarGroupname(file_hdr);
	} else {
		G = 0; /* User */
		c_id = file_hdr->c_uid;
		c_c = file_hdr->c_cu;
		sysusername = ahsStaticGetTarUsername(file_hdr);
	}

	if ((sysusername == NULL || strlen(sysusername) == 0) &&
				c_c != TARU_C_BY_UNONE) {
		E_DEBUG("sysusername nil");
		/*
 		 * Get name by id number
 		 */
		if (G) {
			l_tar_sysdata_getgroup(c_id, NULL, (STROB*)(file_hdr->c_groupname));
		} else {
			l_tar_sysdata_getuser(c_id, NULL, (STROB*)(file_hdr->c_username));
		}
		/* if (tar_hdr) swlib_strncpy(tar_hdr_dest, NULL, SNAME_LEN); */
	} else {
		if ( /* No need to abstract the TARU_C_BY_<> values because
			user and group have the same value */
			c_c == TARU_C_BY_USYS || 
			c_c == TARU_C_BY_UNAME
			) 
			{
			/*
 			 * Get id by name
 			 */
			E_DEBUG2("finding id for sysusername = %s", sysusername);

			if (G) {
				if (taru_get_gid_by_name(sysusername, &pgid)) {
					pgid = NOBODY_ID;
					E_DEBUG3("user name [%s] not found setting uid to %d", sysusername, (int)pgid);
				}
				psid =  (unsigned long)pgid;
			} else {
				if (taru_get_uid_by_name(sysusername, &puid)) {
					puid = NOBODY_ID;
					E_DEBUG3("user name [%s] not found setting uid to %d", sysusername, (int)puid);
				}
				psid =  (unsigned long)puid;
			}

			/* if (tar_hdr) swlib_strncpy(tar_hdr_dest, sysusername, SNAME_LEN); */
		} else if (
			c_c == TARU_C_BY_UID /* same as TARU_C_BY_GID */
			) 
			{
			psid = c_id;
			E_DEBUG2("finding user for uid  %d", (int)psid);
			if (tar_iflags_numeric_uids == 0) {
				if (G) {
					l_tar_sysdata_getgroup(c_id, NULL, (STROB*)(file_hdr->c_groupname));
				} else {
					l_tar_sysdata_getuser(c_id, NULL, (STROB*)(file_hdr->c_username));
				}
				/* if (tar_hdr) swlib_strncpy(tar_hdr_dest, tarbuf, SNAME_LEN); */
			} else {
				/* if (tar_hdr) memset(tar_hdr_dest, '\0', SNAME_LEN); */
			}
		} else {
			E_DEBUG("Using user and uid with no lookups");
			psid = c_id;
			/* if (tar_hdr) swlib_strncpy(tar_hdr_dest, sysusername, SNAME_LEN); */
		}	

		if (G) {
			/* if (tar_hdr) GID_TO_CHARS(psid, tar_hdr_id, termch);*/
		} else {
			/* if (tar_hdr) UID_TO_CHARS(psid, tar_hdr_id, termch);*/
		}
	}
}


void
taru_set_sysdb_nameid_by_policy(struct tar_header *tar_hdr, char * vselect, char * sysusername,
		char * tarbuf, STROB *paxbuf,
		struct new_cpio_header *file_hdr,
		int termch, int tar_iflags_numeric_uids)
{
	char * tar_hdr_dest;
	char * tar_hdr_id;
	unsigned long psid;
	unsigned long c_id;
	unsigned char c_c;
	gid_t pgid;
	uid_t puid;
	int G;

	tar_hdr_id = NULL;
	tar_hdr_dest = NULL;
	if (strncasecmp("G", vselect, 1) == 0) {
		G = 1; /* Group */
		if (tar_hdr) tar_hdr_dest = tar_hdr->gname;
		if (tar_hdr) tar_hdr_id = tar_hdr->gid;
		c_id = file_hdr->c_gid;
		c_c = file_hdr->c_cg;
	} else {
		G = 0; /* User */
		if (tar_hdr) tar_hdr_dest = tar_hdr->uname;
		if (tar_hdr) tar_hdr_id = tar_hdr->uid;
		c_id = file_hdr->c_uid;
		c_c = file_hdr->c_cu;
	}

	if ((sysusername == NULL || strlen(sysusername) == 0) &&
				c_c != TARU_C_BY_UNONE) {
		E_DEBUG("sysusername nil");
		/*
 		 * Get name by id number
 		 */
		if (G) {
			l_tar_sysdata_getgroup(c_id, tarbuf, paxbuf);
		} else {
			l_tar_sysdata_getuser(c_id, tarbuf, paxbuf);
		}
		if (tar_hdr) swlib_strncpy(tar_hdr_dest, tarbuf, SNAME_LEN);
	} else {
		if ( /* No need to abstract the TARU_C_BY_<> values because
			user and group have the same value */
			c_c == TARU_C_BY_USYS || 
			c_c == TARU_C_BY_UNAME
			) 
			{
			/*
 			 * Get id by name
 			 */
			E_DEBUG2("finding id for sysusername = %s", sysusername);

			if (G) {
				if (taru_get_gid_by_name(sysusername, &pgid)) {
					pgid = NOBODY_ID;
					E_DEBUG3("user name [%s] not found setting uid to %d", sysusername, (int)pgid);
				}
				psid =  (unsigned long)pgid;
			} else {
				if (taru_get_uid_by_name(sysusername, &puid)) {
					puid = NOBODY_ID;
					E_DEBUG3("user name [%s] not found setting uid to %d", sysusername, (int)puid);
				}
				psid =  (unsigned long)puid;
			}

			if (tar_hdr) swlib_strncpy(tar_hdr_dest, sysusername, SNAME_LEN);
		} else if (
			c_c == TARU_C_BY_UID /* same as TARU_C_BY_GID */
			) 
			{
			psid = c_id;
			E_DEBUG2("finding user for uid  %d", (int)psid);
			if (tar_iflags_numeric_uids == 0) {
				if (G) {
					taru_get_tar_group_by_gid(psid, tarbuf);
					if (paxbuf) taru_get_pax_group_by_gid(psid, paxbuf);
				} else {
					taru_get_tar_user_by_uid(psid, tarbuf);
					if (paxbuf) taru_get_pax_user_by_uid(psid, paxbuf);
				}
				if (tar_hdr) swlib_strncpy(tar_hdr_dest, tarbuf, SNAME_LEN);
			} else {
				if (tar_hdr) memset(tar_hdr_dest, '\0', SNAME_LEN);
			}
		} else {
			E_DEBUG("Using user and uid with no lookups");
			psid = c_id;
			if (tar_hdr) swlib_strncpy(tar_hdr_dest, sysusername, SNAME_LEN);
		}	

		if (G) {
			if (tar_hdr) GID_TO_CHARS(psid, tar_hdr_id, termch); /* these really are the same */
		} else {
			if (tar_hdr) UID_TO_CHARS(psid, tar_hdr_id, termch); /* these really are the same */
		}
	}
}

/* ============================================================ */
/* ================  Public Routines  ========================= */
/* ============================================================ */

void
taru_mode_to_chars(mode_t v, char *p, size_t s, int termch)
{
	mode_to_chars(v, p, s, POSIX_FORMAT, termch);
}

int 
taru_get_uid_by_name(char *username, uid_t *puid)
{
	long pid;
	int ret;
	E_DEBUG("");
	ret = get_pwent(0, 0, username, NULL, NULL,
			&pid, 
			l_getuidbyname, 
			l_tar_sysdata_getuser,
			&dbpair_user_nset,
			userCacheM
		);
	*puid = (uid_t)(pid);
	return ret;
}


int 
taru_get_pax_user_by_uid(uid_t uid, STROB * buf)
{
	int ret;
	E_DEBUG("");
	ret = get_pwent(0, uid, NULL, NULL, buf, NULL,
			l_getuidbyname, 
			l_tar_sysdata_getuser,
			&dbpair_user_nset,
			userCacheM
		);
	return ret;
}



int 
taru_get_tar_user_by_uid(uid_t uid, char * buf)
{
	int ret;
	E_DEBUG("");
	ret = get_pwent(0, uid, NULL, buf, NULL, NULL,
			l_getuidbyname, 
			l_tar_sysdata_getuser,
			&dbpair_user_nset,
			userCacheM
		);
	return ret;
}

int 
taru_get_gid_by_name(char *groupname, gid_t *guid)
{
	int ret;
	long pid;
	E_DEBUG("");
	ret =  get_pwent(1, 0, groupname, NULL, NULL,
			&pid,
			l_getgidbyname, 
			l_tar_sysdata_getgroup,
			&dbpair_group_nset,
			groupCacheM
		);
	*guid = (gid_t)(pid);
	return ret;
}

int 
taru_get_pax_group_by_gid(gid_t gid, STROB * buf)
{
	int ret;
	E_DEBUG("");
	ret = get_pwent(1, gid, NULL, NULL, buf, NULL,
			l_getgidbyname, 
			l_tar_sysdata_getgroup,
			&dbpair_group_nset,
			groupCacheM
		);
	return ret;
}

int 
taru_get_tar_group_by_gid(gid_t gid, char * tarbuf)
{
	int ret;
	E_DEBUG("");
	ret = get_pwent(1, gid, NULL, tarbuf, NULL, NULL,
			l_getgidbyname, 
			l_tar_sysdata_getgroup,
			&dbpair_group_nset,
			groupCacheM
		);
	return ret;
}

int
taru_split_name_ustar(struct tar_header *tar_hdr, char * name, int tar_iflags)
{
	size_t length = strlen(name);
	size_t i;
	char * uh = (char*)tar_hdr;

	memset(uh + THB_BO_name, '\0', NAME_FIELD_SIZE);
	memset(uh + THB_BO_prefix, '\0', PREFIX_FIELD_SIZE);

	if (length > PREFIX_FIELD_SIZE + NAME_FIELD_SIZE + 1) {
		fprintf(stderr, "%s: name too long (max): %s\n", swlib_utilname_get(), name);
		return -1;
	}
	
	i = split_long_name(name, length);
	if (length - i - 1 > NAME_FIELD_SIZE)
	{
		fprintf(stderr, "%s: name too long (cannot be split): (i=%d) [%s]\n", swlib_utilname_get(), (int)i, name);
		return -1;
	}
	if (i == 0) {
		return 0;	
	}
	memcpy((void*)(uh+THB_BO_name), name + i + 1, length - i - 1);
	memcpy((void*)(uh+THB_BO_prefix), name, i);
	return 0;
}

int 
taru_set_filetype_from_tartype(char ch, mode_t * mode, char * filename)
{

	(*mode) &= (~(S_IFMT));
	switch (ch) {
	case REGTYPE:
	case CONTTYPE:		/* For now, punt.  */
		(*mode) |= CP_IFREG;
		break;
	case DIRTYPE:
		(*mode) |= CP_IFDIR;
		break;
	case CHRTYPE:
		(*mode) |= CP_IFCHR;
		break;
	case BLKTYPE:
		(*mode) |= CP_IFBLK;
		break;
#ifdef CP_IFIFO
	case FIFOTYPE:
		(*mode) |= CP_IFIFO;
		break;
#endif
	case SYMTYPE:
#ifdef CP_IFLNK
		(*mode) |= CP_IFLNK;
		break;
		/* Else fall through.  */
#endif
	case LNKTYPE:
		(*mode) |= CP_IFREG;
		break;
	case AREGTYPE:
		/* Old tar format; if the last char in filename
		   is '/' then it is a directory, otherwise it's a regular
		   file.  */
		if (filename == (char*)NULL) {
			fprintf(stderr, 
		"taru_set_filetype_from_tartype():"
		" warning: AREGTYPE: filename not given, assuming REGTYPE.\n");
			(*mode) |= CP_IFREG;
		} else {
			if (filename[strlen(filename) - 1] == '/')
				(*mode) |= CP_IFDIR;
			else
				(*mode) |= CP_IFREG;
		}
		break;
	case NOTDUMPEDTYPE:
		(*mode) |= CP_IFSOCK;
		break;
	default:
		(*mode) |= CP_IFREG;
		fprintf(stderr, 
		"%s: warning:  typeflag [%c] not supported, ignoring file\n", swlib_utilname_get(), ch);
		return 1;
	}
	return 0;
}

static
void
tarui_get_filetypes (mode_t mode, int * cpio_mode, char *tarflag)
{
	if (S_ISREG(mode)) {
		*tarflag=REGTYPE;
		*cpio_mode= CP_IFREG;
	} else if (S_ISDOOR(mode)) {
		*tarflag=NOTDUMPEDTYPE;
		*cpio_mode= CP_IFSOCK;
	} else if (S_ISDIR(mode)) {
		*tarflag=DIRTYPE;
		*cpio_mode = CP_IFDIR;
#ifdef S_ISBLK
	} else if (S_ISBLK(mode)) {
		*tarflag=BLKTYPE;
		*cpio_mode= CP_IFBLK;
#endif
#ifdef S_ISCHR
	} else if (S_ISCHR(mode)) {
		*tarflag=CHRTYPE;
		*cpio_mode= CP_IFCHR;
#endif
#ifdef S_ISFIFO
	} else if (S_ISFIFO(mode)) {
		*tarflag=FIFOTYPE;
		*cpio_mode= CP_IFIFO;
#endif
#ifdef S_ISLNK
	} else if (S_ISLNK(mode)) {
		*tarflag=SYMTYPE;
		*cpio_mode= CP_IFLNK;
#endif
#ifdef S_ISSOCK
	} else if (S_ISSOCK(mode)) {
		*tarflag=NOTDUMPEDTYPE;
		*cpio_mode= CP_IFSOCK;
#endif
#ifdef S_ISNWK
	} else if (S_ISNWK(mode)) {
		*tarflag=REGTYPE;
		*cpio_mode= CP_IFNWK;
#endif
	} else {
		fprintf (stderr,"%s: unrecognized type in mode: %d\n", swlib_utilname_get(), (int)mode);	
		*tarflag=(char)(-1);
		*cpio_mode= -1;
	}
}

int 
taru_get_tar_filetype (mode_t mode)
{
	char  c;
	int  cm;
	tarui_get_filetypes(mode, &cm, &c);
	return (int)c;
}

int
taru_get_cpio_filetype (mode_t mode)
{
	char  c;
	int  cm;
	tarui_get_filetypes(mode, &cm, &c);
	return cm;
}


static int
i_taru_read_in_tar_header2(TARU * taru, struct new_cpio_header *file_hdr,
			int in_des, char * fsource_buffer,
			int * eoa, int tarheaderflags, int fsource_buffer_len, int fp_retval)
{
	mode_t modet;
	int retval = 0;
	unsigned char * ext_data_buffer;
	int ext_data_buffer_len;
	long bytes_skipped = 0;
	int warned = FALSE;
	union tar_record tar_rec;
	struct tar_header *tar_hdr;
	char * aregfilename;
	char * tmpname;
	char * read_buffer;
	int do_record_header = 0;
	int tar_iflags_retain_header_id = 0;
	int buffer_offset;
	int xhd_errorcode;
	int eoaret;
	int tmpret;
	int tmpret_ustar;
	int tmpret_extdata;
	int read_so_far = 0;
	long uidp;
	long gidp;

	
	E_DEBUG("BEGIN");

	buffer_offset = 0;
	if (taru) {
		tar_iflags_retain_header_id = (tarheaderflags & TARU_TAR_RETAIN_HEADER_IDS);
	}
	if (eoa) {
		*eoa = 0;
	}
	ext_data_buffer = NULL;
	
	if (taru) {
		do_record_header = taru->do_record_headerM;
	}

	E_DEBUG("");

	strob_set_length(taru->read_bufferM, taru->read_buffer_posM + TARRECORDSIZE);
	read_buffer = strob_str(taru->read_bufferM);
	
	if (fsource_buffer == NULL) {
		tmpret_ustar = taru_tape_buffered_read(in_des, (void *)(read_buffer), TARRECORDSIZE);
		if (tmpret_ustar > 0 && tmpret_ustar != TARRECORDSIZE) {
			fprintf(stderr,
				"%s: otar.c: short read in read_in_tar_header2 return=%d\n",
					swlib_utilname_get(), tmpret_ustar);
			return -1;
		} else if (tmpret_ustar == 0) {
			return 0;
		} else if (tmpret_ustar < 0) {
			fprintf(stderr,
				"%s: otar.c: read error from read_in_tar_header2 return=%d\n",
					swlib_utilname_get(), tmpret_ustar);
			return -1;
		} else {
			; /* Ok */
		}
	} else {
		memcpy((void *)(read_buffer), fsource_buffer + taru->read_buffer_posM, TARRECORDSIZE);
		fsource_buffer_len -= TARRECORDSIZE;
		tmpret_ustar = TARRECORDSIZE;
	}
	taru->read_buffer_posM += TARRECORDSIZE;
	strob_set_length(taru->read_bufferM, taru->read_buffer_posM + TARRECORDSIZE);
	read_buffer = strob_str(taru->read_bufferM);

	E_DEBUG("");	
	if (do_record_header) {
		strob_set_memlength(taru->headerM, taru->header_lengthM + 512);
		memcpy((void*)(strob_str(taru->headerM) + taru->header_lengthM),
				 (void*)(read_buffer), 512);
		taru->header_lengthM += 512;
	}

	retval += 512;

	E_DEBUG("");	
	/* 
 	 * Check for a block of 0's.
 	 */
	if (null_block((long *) read_buffer, TARRECORDSIZE)) {
		ahsStaticSetTarFilename(file_hdr, CPIO_INBAND_EOA_FILENAME);

		/*
		 * Allow one (1) null block to signal EOA, this is wrong as two
		 * should be required. Check all remaining blocks to determine length
		 * of trailer blocks
		 */
		if (fsource_buffer == NULL) {
			eoaret = taru_tape_buffered_read(in_des, (void *)read_buffer, TARRECORDSIZE);
		} else {
			eoaret = TARRECORDSIZE;
			memset(read_buffer, '\0', TARRECORDSIZE);
		}
		if (
			eoaret != TARRECORDSIZE ||
			null_block((long *) read_buffer, TARRECORDSIZE) == 0
		) {
			/*
			 * This is unexpected as it indicates a possible corrupt
			 * archive.
			 */

			fprintf(stderr,
			"%s: possible corrupt archive, non-null block found after one null block\n",
							swlib_utilname_get());
			retval = -1;
		} else {
			/*
			 * normal exit, two (2) null blocks
			 */
			retval = 1024;
		}
		taru->read_buffer_posM += TARRECORDSIZE;
		if (eoa) *eoa = retval;
		return retval;  /* End of Archive */
	}

	tar_hdr = (struct tar_header*)(read_buffer);
	E_DEBUG("");	
	if (
		tar_hdr->typeflag == GNUTYPE_LONGNAME ||
		tar_hdr->typeflag == GNUTYPE_LONGLINK ||
		tar_hdr->typeflag == XHDTYPE ||
		tar_hdr->typeflag == XGLTYPE
	  ) {
		E_DEBUG2("GOT Extened Header data type:[%c]", tar_hdr->typeflag);

		/*
 		 * Determine the size of the exteneded header data
 		 */

		taru_otoumax(tar_hdr->size, &file_hdr->c_filesize);

		/*
 		 * Read the Extended Header Data blocks
 		 */

		if (fsource_buffer) {
			E_DEBUG("in fsource_buffer");
			if ((unsigned long)fsource_buffer_len < file_hdr->c_filesize) {
				E_DEBUG3("fsource_buffer_len < file_hdr->c_filesize: %d < %d", (int) fsource_buffer_len, (int)file_hdr->c_filesize);
				tmpret_extdata = -1;
			} else {
				E_DEBUG("reading pax data blocks");
				tmpret_extdata = taru_read_pax_data_blocks(taru, -1,
					fsource_buffer,
					file_hdr->c_filesize,
					taru_tape_buffered_read);
			}
			if (tmpret_extdata > 0) {
				fsource_buffer_len -= tmpret_extdata;
			}
		} else {
			E_DEBUG("fsource_buffer is NULL");
			tmpret_extdata = taru_read_pax_data_blocks(taru, in_des,
					NULL,
					file_hdr->c_filesize,
					taru_tape_buffered_read);
		}

		if (tmpret_extdata < 0) {
			fprintf(stderr, "fatal error reading extended header data blocks\n");
			exit(24);
		}

		/*
 		 * Now interpret the exteneded header data blocks and assign the
 		 * values to the file_hdr fields setting the usage mask to mark
 		 * the field as set (thus overriding the value in the regular ustar header)
 		 * to be read next.
		 */

		ext_data_buffer = (unsigned char*)strob_str(taru->read_bufferM)
						+ taru->read_buffer_posM 
						- tmpret_extdata;
		ext_data_buffer_len = file_hdr->c_filesize;

		switch (tar_hdr->typeflag) {
			case GNUTYPE_LONGNAME:
				/* Null terminated by taru_read_pax_data_blocks */
				ahsStaticSetTarFilename(file_hdr, (char*)ext_data_buffer);
				E_DEBUG2("Setting LongName Data: [%s]", ahsStaticGetTarFilename(file_hdr));	
				file_hdr->extHeader_usage_maskM |= TARU_EHUM_PATH;
			break;
			case GNUTYPE_LONGLINK:
				/* Null terminated by taru_read_pax_data_blocks */
				ahsStaticSetPaxLinkname(file_hdr, (char*)ext_data_buffer);
				E_DEBUG2("Setting LongLink Data: [%s]", ahsStaticGetTarLinkname(file_hdr));	
				file_hdr->extHeader_usage_maskM |= TARU_EHUM_LINKPATH;
			break;
			case XGLTYPE:
				fprintf(stderr, "type XGLTYPE (Pax Global Header) not supported\n");
				exit(26);
				break;
			case XHDTYPE:
				tmpret = taru_read_all_ext_header_records (file_hdr,
									(char*)ext_data_buffer,
									ext_data_buffer_len,
									&xhd_errorcode);
				if (tmpret < 0 ) {
					fprintf(stderr, "%s: error reading Pax extended header, errorcode=%d\n",
						swlib_utilname_get(),  xhd_errorcode);
					return -11;
				}
				break;
			default:
				/* error never should happen */
				fprintf(stderr, "fatal internal error\n");
				exit(26);

			break;
		}

		/*
		 * Now make recursive call with the extened header info and
		 * the header_buffer pointer moved ahead 'read_so_far' bytes ahead
 		 */ 

		E_DEBUG("Calling i_taru_read_in_tar_header2");	
		read_so_far = tmpret_ustar + tmpret_extdata;
		retval = i_taru_read_in_tar_header2(taru, file_hdr, in_des,
					fsource_buffer ? fsource_buffer + read_so_far : (char*)NULL,
					eoa, tarheaderflags, fsource_buffer_len, fp_retval + read_so_far);
		return retval;
	}

	E_DEBUG("");	
	while (1) {
		E_DEBUG("");	
		taru_otoul(tar_hdr->chksum, &file_hdr->c_chksum);

		if (file_hdr->c_chksum != taru_tar_checksum(tar_hdr)) {
			if (!fsource_buffer) {

		/* If the checksum is bad, skip 1 byte and try again.  When
		   we try again we do not look for an EOF record (all zeros),
		   because when we start skipping bytes in a corrupted archive
		   the chances are pretty good that we might stumble across
		   2 blocks of 512 zeros (that probably is not really the last
		   record) and it is better to miss the EOF and give the user
		   a "premature EOF" error than to give up too soon on a
		   corrupted archive.  */
				if (!warned) {
					fprintf(stderr,
					"%s: invalid header: checksum error\n",
					swlib_utilname_get());
					warned = TRUE;
				}
				if (tarheaderflags & TARU_TAR_FRAGILE_FORMAT)
						return -2;
				bcopy(((char *) &tar_rec) + 1,
						(char *) &tar_rec,
				   		   TARRECORDSIZE - 1);
				taru_tape_buffered_read(in_des,
			(void *)(((char *)(&tar_rec)) + (TARRECORDSIZE - 1)),
					1);
				++bytes_skipped;
				continue;


			}
			/* not header buffer */ 
			else {
				return -3;
			}

		}	/* bad check sum */
				

		if (((file_hdr->extHeader_usage_maskM) & TARU_EHUM_PATH) == 0) {
			tmpname = taru_dup_tar_name((void*)tar_hdr);
			ahsStaticSetTarFilename(file_hdr, tmpname);
		} else {
			;
			tmpname = NULL;
			E_DEBUG("Path set by extended headers");	
		}

		E_DEBUG("");	
		file_hdr->c_namesize = strlen(ahsStaticGetTarFilename(file_hdr)) + 1;

		if ((char)(tar_hdr->typeflag) == LNKTYPE) {
			/* hard link */
			E_DEBUG("setting file_hdr->c_nlink = 2");
			file_hdr->c_nlink = 2;
		} else {
			E_DEBUG("setting file_hdr->c_nlink = 1");	
			file_hdr->c_nlink = 1;
		}

		taru_otoul(tar_hdr->mode, &(file_hdr->c_mode));
		file_hdr->c_mode = file_hdr->c_mode & 07777;
		
		E_DEBUG("");	
		ahsStaticSetTarUsername(file_hdr, tar_hdr->uname);
		if (
			tar_iflags_retain_header_id == 0 &&
			l_cache_tar_getuidbyname(tar_hdr->uname, &uidp) == 0
		) {
			E_DEBUG("");	
			file_hdr->c_uid = uidp;
		} else {
			E_DEBUG("");	
			taru_otoul(tar_hdr->uid, &file_hdr->c_uid);
		}
		E_DEBUG("");	
		taru_set_filetype_from_tartype((char)(tar_hdr->typeflag),
			(modet=(mode_t)(file_hdr->c_mode), &modet), tmpname);
	
		E_DEBUG("");	
		switch ((int)(tar_hdr->typeflag)) {
			case LNKTYPE:
				file_hdr->c_is_tar_lnktype = 1;
				break;
			default:
				file_hdr->c_is_tar_lnktype = 0;
		}
	
		E_DEBUG("");	
		ahsStaticSetTarGroupname(file_hdr, tar_hdr->gname);
		if (
			tar_iflags_retain_header_id == 0 &&
			l_cache_tar_getgidbyname(tar_hdr->gname, &gidp) == 0
		) {
			file_hdr->c_gid = gidp;
		} else {
			taru_otoul(tar_hdr->gid, &file_hdr->c_gid);
		}

		taru_otoumax(tar_hdr->size, &file_hdr->c_filesize);
		taru_otoul(tar_hdr->mtime, &file_hdr->c_mtime);
		taru_otoul(tar_hdr->devmajor,
				(unsigned long *) &file_hdr->c_rdev_maj);
		taru_otoul(tar_hdr->devminor,
				(unsigned long *) &file_hdr->c_rdev_min);
		E_DEBUG("");	


		/* This closes (frees) the buffer, don't know why to do this
 		 * in retrospect
		ahsStaticSetTarLinkname(file_hdr, (char *)(NULL));
 		 */

		switch (tar_hdr->typeflag) {
		case REGTYPE:
		case CONTTYPE:	/* For now, punt.  */
		default:
			file_hdr->c_mode |= CP_IFREG;
			break;
		case DIRTYPE:
			file_hdr->c_mode |= CP_IFDIR;
			break;
		case CHRTYPE:
			file_hdr->c_mode |= CP_IFCHR;

		/* If a POSIX tar header has a valid linkname it's always 
		   supposed to set typeflag to be LNKTYPE.  System V.4 tar 
		   seems to be broken, and for device files with multiple
		   links it puts the name of the link into linkname,
		   but leaves typeflag as CHRTYPE, BLKTYPE, FIFOTYPE, etc.  */

			ahsStaticSetTarLinkname(file_hdr, tar_hdr->linkname);

		/* Does POSIX say that the filesize must be 0 for devices?  We
		   assume so, but HPUX's POSIX tar sets it to be 1 which causes
		   us problems (when reading an archive we assume we can always
		   skip to the next file by skipping filesize bytes).  For 
		   now at least, it's easier to clear filesize for devices,
		   rather than check everywhere we skip in copyin.c.  */

			file_hdr->c_filesize = 0;
			break;
		case BLKTYPE:
			file_hdr->c_mode |= CP_IFBLK;
			ahsStaticSetTarLinkname(file_hdr, tar_hdr->linkname);
			file_hdr->c_filesize = 0;
			break;
#ifdef CP_IFIFO
		case FIFOTYPE:
			file_hdr->c_mode |= CP_IFIFO;
			ahsStaticSetTarLinkname(file_hdr, tar_hdr->linkname);
			file_hdr->c_filesize = 0;
			break;
#endif
		case SYMTYPE:
#ifdef CP_IFLNK
			E_DEBUG("at SYMLINK");
			file_hdr->c_mode |= CP_IFLNK;

			if (((file_hdr->extHeader_usage_maskM) & TARU_EHUM_LINKPATH) == 0) {
				E_DEBUG2("linkname=[%s]", tar_hdr->linkname);
				ahsStaticSetTarLinkname(file_hdr, tar_hdr->linkname);
				E_DEBUG2("ahs linkname=[%s]", ahsStaticGetTarLinkname(file_hdr));
			} else {
				;
				E_DEBUG("Path set by extended headers");	
			}
			file_hdr->c_filesize = 0;
			break;
			/* Else fall through.  */
#endif
		case LNKTYPE:
			file_hdr->c_mode |= CP_IFREG;
			ahsStaticSetTarLinkname(file_hdr, tar_hdr->linkname);
			file_hdr->c_filesize = 0;
			break;
		case AREGTYPE:
			/* Old tar format; if the last char in 
			   filename is '/' then it is
			   a directory, otherwise it's a regular file.  */
			aregfilename = ahsStaticGetTarFilename(file_hdr);
			if (aregfilename[strlen(aregfilename) - 1] == '/')
				file_hdr->c_mode |= CP_IFDIR;
			else
				file_hdr->c_mode |= CP_IFREG;
			break;
		}
		break;
	}
	if (bytes_skipped > 0) {
		fprintf(stderr,
			"%s: warning: skipped %ld bytes of junk\n", 
				swlib_utilname_get(), bytes_skipped);
		if (tarheaderflags & TARU_TAR_FRAGILE_FORMAT) return -4;
        }
	return retval + fp_retval;
}



/* Read a tar header, including the file name, from file descriptor IN_DES
   into FILE_HDR.  */
int
taru_read_in_tar_header2(TARU * taru, struct new_cpio_header *file_hdr,
			int in_des, char * fsource_buffer,
			int * eoa, int tarheaderflags, int fsource_buffer_len)
{
	EXATT * p;
	CPLOB * extlist;
	int index;
	int ret;

	E_DEBUG("");

	if (taru) taru->read_buffer_posM = 0;
	if (taru && taru->do_record_headerM) {
		taru->header_lengthM = 0;	
	}
	file_hdr->extHeader_usage_maskM = 0;

	/*
 	 * Initialize the list of extended header records to
 	 * a status of not used
 	 */
	taru_exattlist_init(file_hdr);

	ahsStaticSetTarFilename(file_hdr, "");
	ahsStaticSetPaxLinkname(file_hdr, "");

	E_DEBUG2("fsource_buffer=%p", fsource_buffer);
	E_DEBUG2("fsource_buffer_len=%d", fsource_buffer_len);

	ret = i_taru_read_in_tar_header2(taru, file_hdr, in_des, fsource_buffer,
			eoa, tarheaderflags, fsource_buffer_len, 0);

	/*
 	 * Now write the values from the extended headers
 	 */

	if (file_hdr->extHeader_usage_maskM) {
		extlist = (CPLOB*)(file_hdr->extattlistM);
		if (extlist) {
			/*
 			 * Loop over the extended header records
 			 * to override the the values from the ustar header
 			 * that have extended header records.
 			 */
			index = 0;
			while((p=(EXATT*) cplob_val(file_hdr->extattlistM, index))) {
				taru_exatt_override_ustar (file_hdr, p);
				index++;
			}
		}
	}
	return ret;
}


int
taru_write_out_tar_header2(TARU * taru, struct new_cpio_header *file_hdr,
			int out_des, char *header_buffer,
			char * username, char * groupname, int tar_iflags)
{
	int ret;
	union tar_record tar_rec;
	struct tar_header *tar_hdr = (struct tar_header *) &tar_rec;
	char * u_name_buffer;
	char * u_ent_buffer;
	unsigned long sum;
	int termch;
	char * tmpp;
	int tar_iflags_like_star;
	int tar_iflags_like_pax;
	int tar_iflags_numeric_uids;
	int tar_iflags_like_oldgnu;
	int tar_iflags_like_oldgnuposix;
	int tar_iflags_like_gnu; 
	int do_record_header = 0;
	
	if (!taru) {
		fprintf(stderr,
			"fatal internal error: taru is NULL, this needs to be fixed\n");
		return -1;
	}
	
	if (*((char*)(taru)) != 'A' /* magic bytes */) {
		fprintf(stderr,
	"%s: fatal error: taru is uninitialized, this needs to be fixed\n",
		swlib_utilname_get());
		return -1;
	}
	
	E_DEBUG3("ENTERING fd=%d, name=[%s]", out_des, ahsStaticGetTarFilename(file_hdr));
	E_DEBUG3("username=[%s] groupname=[%s]", username, groupname);
	E_DEBUG3("file_hdr: uid=[%d] gid=[%d]", (int)(file_hdr->c_uid), (int)(file_hdr->c_gid));

	bzero((char *) &tar_rec, TARRECORDSIZE);

	tar_iflags_like_star = (tar_iflags & TARU_TAR_BE_LIKE_STAR);
	tar_iflags_like_pax = (tar_iflags & TARU_TAR_BE_LIKE_PAX);
	tar_iflags_numeric_uids = (tar_iflags & TARU_TAR_NUMERIC_UIDS);
	tar_iflags_like_gnu = (tar_iflags & TARU_TAR_GNU_GNUTAR);
	tar_iflags_like_oldgnu = (tar_iflags & TARU_TAR_GNU_OLDGNUTAR);
	tar_iflags_like_oldgnuposix = (tar_iflags & TARU_TAR_GNU_OLDGNUPOSIX);
	E_DEBUG2("tar_iflags_numeric_uids is %d", tar_iflags_numeric_uids);
	
	do_record_header = taru->do_record_headerM;

	if (tar_iflags_like_star) {
		termch = '\040';  /* space */
	} else {
		termch = 0;   /* NUL */
	}

	strob_strcpy(taru->u_name_bufferM,
			ahsStaticGetTarFilename(file_hdr));

	if (strob_strlen(taru->u_name_bufferM) == 0) {
		fprintf(stderr, "%s: internal error: taru->u_name_bufferM zero length\n", swlib_utilname_get());
		return -1;
	}

	/*
	 * Add a trailing slash if a directory
	 */
	switch (file_hdr->c_mode & CP_IFMT) {
		case CP_IFDIR:
			if (
				*(strob_str(taru->u_name_bufferM) +
					strob_strlen(taru->u_name_bufferM) - 1) != '/'
			) {
				if (
					(strob_strlen(taru->u_name_bufferM) >= TARNAMESIZE - 1) &&
					(tar_iflags_like_oldgnu || tar_iflags_like_oldgnuposix)
				) {
					/*
					 * Don't un-NUL terminate the name field for OLD gnu formats.
					 */
					;
				} else {
					strob_strcat(taru->u_name_bufferM, "/");
				}
			}
		break;
	}

	u_name_buffer = strob_str(taru->u_name_bufferM);
	u_ent_buffer = strob_str(taru->u_ent_bufferM);
	/*
	 * the pathname is now in (char*)u_name_buffer
	 */

	if (taru_set_new_name(/*taru, */ tar_hdr, -1, u_name_buffer, taru->taru_tarheaderflagsM)) {
		fprintf(stderr, "%s: %s not dumped\n", swlib_utilname_get(), u_name_buffer);
		return -13;
	}

	if (tar_iflags_like_oldgnu) {
		MODE_TO_OLDGNU_CHARS(file_hdr->c_mode, tar_hdr->mode, termch);
	} else {
		MODE_TO_CHARS(file_hdr->c_mode, tar_hdr->mode, termch);
	}

	UID_TO_CHARS(file_hdr->c_uid, tar_hdr->uid, termch);
	GID_TO_CHARS(file_hdr->c_gid, tar_hdr->gid, termch);
	OFF_TO_CHARS(file_hdr->c_filesize,  tar_hdr->size, termch);
	TIME_TO_CHARS(file_hdr->c_mtime, tar_hdr->mtime, termch);

	*(tar_hdr->version) = '0';
	*(tar_hdr->version + 1)= '0';

	switch (file_hdr->c_mode & CP_IFMT) {
	case CP_IFREG:
		tmpp = ahsStaticGetTarLinkname(file_hdr);
		if (tmpp && strlen(tmpp)) {
			/*
			 * This code branch handles hard links
			 */
			/* 
			 * makes sure that linkname is shorter than
			 * TARLINKNAMESIZE.  
			 */
			if (strlen(tmpp) > TARLINKNAMESIZE) {
				fprintf(stderr,
					"%s, link name [%s] too long for tar (returning -4).\n",
						swlib_utilname_get(), tmpp);
				E_DEBUG("LEAVING");
				return -4;
			}
			strncpy(tar_hdr->linkname, tmpp, TARLINKNAMESIZE);
			SIZE_TO_CHARS(0, tar_hdr->size, termch);
			tar_hdr->typeflag = LNKTYPE;
		} else {
			tar_hdr->typeflag = REGTYPE;
		}
		break;
	case CP_IFDIR:
		tar_hdr->typeflag = DIRTYPE;
		break;
	case CP_IFCHR:
		tar_hdr->typeflag = CHRTYPE;
		break;
	case CP_IFBLK:
		tar_hdr->typeflag = BLKTYPE;
		break;
#ifdef CP_IFSOCK
	case CP_IFSOCK:	/* GNU Tar does this, makes a SOCK a FIFO type */
#endif
#ifdef CP_IFIFO
	case CP_IFIFO:
		tar_hdr->typeflag = FIFOTYPE;
		break;
#endif				/* CP_IFIFO */
#ifdef CP_IFLNK
	case CP_IFLNK:
		tmpp = ahsStaticGetTarLinkname(file_hdr);
		if (
			strcmp(ahsStaticGetTarFilename(file_hdr), GNU_LONG_LINK) == 0 
		) {
			if (strlen(tmpp) > 510) {
				fprintf(stderr, "%s: link name [%s] too long this implementation at this time.\n",
					swlib_utilname_get(), tmpp);
				return -5;
			}
			tar_hdr->typeflag = GNUTYPE_LONGLINK;
			SIZE_TO_CHARS(strlen(tmpp),  tar_hdr->size, termch);
		} else { 
			tar_hdr->typeflag = SYMTYPE;
			if (strlen(tmpp) > TARLINKNAMESIZE) {
				fprintf(stderr, "%s: link name [%s] too long for tar.\n",
						swlib_utilname_get(), tmpp);
				E_DEBUG("LEAVING");
				return -5;
			}
			SIZE_TO_CHARS(0,  tar_hdr->size, termch);
		}
		strncpy(tar_hdr->linkname, tmpp, TARLINKNAMESIZE);
		break;
#endif				/* CP_IFLNK */

	default:
		if (file_hdr->c_mode == 0 &&
			strcmp(ahsStaticGetTarFilename(file_hdr),
						GNU_LONG_LINK) == 0) {
			/*
 			 * Special Case for GN_LONG_LINK file types
 			 */

			if (strlen(ahsStaticGetTarLinkname(file_hdr)) == 0) {
				tar_hdr->typeflag = GNUTYPE_LONGNAME;
			} else {
				/*
 				 * this is the code path for hard links
 				 */
				tar_hdr->typeflag = GNUTYPE_LONGLINK;
			}
		} else {
			fprintf(stderr, 
	"bad CP_?? filetype in taru_write_out_tar_header2: mode = %d\n",
				(int)(file_hdr->c_mode));
		}
		break;
	} /*--- end switch ---*/


	if (
		tar_iflags_like_gnu == 0 && 
		tar_iflags_like_oldgnu == 0
	) {
		strncpy(tar_hdr->magic, TMAGIC, TMAGLEN);
		strncpy(tar_hdr->magic + TMAGLEN, TVERSION, TVERSLEN);
	} else {
		/*
		 * GNU tar 1.13.25 and tar 1.15.1 --format=gnu do it like this
		 */
		strncpy(tar_hdr->magic, "ustar ", 6);
		strncpy(tar_hdr->magic + 6, " ", 2);
	}

	/* -------------------------------------------------------------------- */


	if ((username == NULL || strlen(username) == 0) &&
				file_hdr->c_cu != TARU_C_BY_UNONE) {
		E_DEBUG("username nil");
		l_tar_sysdata_getuser(file_hdr->c_uid, u_ent_buffer, NULL);
		swlib_strncpy(tar_hdr->uname, u_ent_buffer, SNAME_LEN);
	} else {
		uid_t puid;
		if (
			file_hdr->c_cu == TARU_C_BY_USYS || 
			file_hdr->c_cu == TARU_C_BY_UNAME
			) 
			{
			E_DEBUG2("finding uid for username = %s", username);
			if (taru_get_uid_by_name(username, &puid)) {
				puid = NOBODY_ID;
				E_DEBUG3("user name [%s] not found setting uid to %d", username, (int)puid);
			}
			swlib_strncpy(tar_hdr->uname, username, SNAME_LEN);
		} else if (
			file_hdr->c_cu == TARU_C_BY_UID
			) 
			{
			puid = file_hdr->c_uid;
			E_DEBUG2("finding user for uid  %d", (int)puid);
			if (tar_iflags_numeric_uids == 0) {
				taru_get_tar_user_by_uid(puid, u_ent_buffer);
				swlib_strncpy(tar_hdr->uname, u_ent_buffer, SNAME_LEN);
			} else {
				swlib_strncpy(tar_hdr->uname, "", SNAME_LEN);
			}
		} else {
			E_DEBUG("Using user and uid with no lookups");
			puid = file_hdr->c_uid;
			swlib_strncpy(tar_hdr->uname, username, SNAME_LEN);
		}	
		UID_TO_CHARS(puid, tar_hdr->uid, termch);
	}

	if ((groupname == NULL || strlen(groupname) == 0)
				&& file_hdr->c_cg != TARU_C_BY_GNONE) {
		E_DEBUG("groupname nil");
		l_tar_sysdata_getgroup(file_hdr->c_gid, u_ent_buffer, NULL);
		strncpy(tar_hdr->gname, u_ent_buffer, SNAME_LEN);
	} else {
		gid_t pgid;

		if (
			file_hdr->c_cg == TARU_C_BY_GSYS || 
			file_hdr->c_cg == TARU_C_BY_GNAME
			) 
			{
			E_DEBUG2("finding gid for groupname=%s", groupname);
			swlib_strncpy(tar_hdr->gname, groupname, SNAME_LEN);
			if (taru_get_gid_by_name(groupname, &pgid)) {
				pgid = NOBODY_ID;
				E_DEBUG3("group name [%s] not found setting uid to %d", groupname, (int)pgid);
			}
		} else if (
			file_hdr->c_cg == TARU_C_BY_GID
			) 
			{
			pgid = file_hdr->c_gid;
			E_DEBUG2("finding gname for gid  %d", (int)pgid);
			if (tar_iflags_numeric_uids == 0) {
				taru_get_tar_group_by_gid(pgid, u_ent_buffer);
				swlib_strncpy(tar_hdr->gname, u_ent_buffer, SNAME_LEN);
			} else {
				swlib_strncpy(tar_hdr->gname, "", SNAME_LEN);
			}
		} else {
			E_DEBUG("Using user and gid with no lookups");
			pgid = file_hdr->c_gid;
			swlib_strncpy(tar_hdr->gname, groupname, SNAME_LEN);
		}	
		GID_TO_CHARS(pgid, tar_hdr->gid, termch);
	}


	if (tar_iflags_numeric_uids) {
		memset((void*)tar_hdr->uname, '\0', SNAME_LEN);
		memset((void*)tar_hdr->gname, '\0', SNAME_LEN);
	}

	if (tar_hdr->typeflag == CHRTYPE || tar_hdr->typeflag == BLKTYPE) {
		MAJOR_TO_CHARS(file_hdr->c_rdev_maj, tar_hdr->devmajor, termch);
		MINOR_TO_CHARS(file_hdr->c_rdev_min, tar_hdr->devminor, termch);
	} else {
		if (tar_iflags_like_pax) {
			strncpy(tar_hdr->devmajor, "0000000", 8);
			strncpy(tar_hdr->devminor, "0000000", 8);
		} else if (tar_iflags_like_star) {
			strncpy(tar_hdr->devmajor, "0000000 ", 8);
			strncpy(tar_hdr->devminor, "0000000 ", 8);
		} else if (
			tar_iflags_like_gnu ||
			tar_iflags_like_oldgnu ||
			tar_iflags_like_oldgnuposix
			)
		{
			/*
			 * GNU tar 1.13.25 leaves the fields null (zero length strings)
			 * padded with 0x00.
			 */
			;
		} else {
			/*
			 * Now, as of Jan 2005 version>0.420, the default is like GNU tar 1.15.1
			 */
			strncpy(tar_hdr->devmajor, "0000000", 8);
			strncpy(tar_hdr->devminor, "0000000", 8);
		}
	}

	memcpy(tar_hdr->chksum, CHKBLANKS, sizeof(tar_hdr->chksum));
	sum = taru_tar_checksum(tar_hdr);

	if (tar_iflags_like_pax || tar_iflags_like_star) {
		/* This mimics pax v3.0 */
		uintmax_to_chars(sum, tar_hdr->chksum, 8, POSIX_FORMAT, termch);
	} else {
		uintmax_to_chars(sum, tar_hdr->chksum, 7, POSIX_FORMAT, termch);
	}

	if (header_buffer) {
		if (do_record_header) {
			strob_set_memlength(taru->headerM, TARRECORDSIZE);
			memcpy((void*)(strob_str(taru->headerM)), (void*)(&tar_rec), TARRECORDSIZE);
			taru->header_lengthM = TARRECORDSIZE;
		}
		memcpy(header_buffer, (void *) &tar_rec, 512);
		E_DEBUG("LEAVING");
		return 512;
	} else {
		if (do_record_header) {
			strob_set_memlength(taru->headerM, TARRECORDSIZE);
			memcpy((void*)(strob_str(taru->headerM)), (void*)(&tar_rec), TARRECORDSIZE);
			taru->header_lengthM = TARRECORDSIZE;
		}
		ret = taru_safewrite(out_des, (void *) &tar_rec, TARRECORDSIZE);
		E_DEBUG("LEAVING");
		return ret;
	}
}

long
taru_from_oct (int digs, char *where)
{
  long value;

  while (ISSPACE (*where))
    {                           /* skip spaces */
      where++;
      if (--digs <= 0)
        return -1;              /* all blank field */
    }
  value = 0;
  while (digs > 0 && ISODIGIT (*where))
    {

      /* Scan til nonoctal.  */

      value = (value << 3) | (*where++ - '0');
      --digs;
    }

  if (digs > 0 && *where && !ISSPACE (*where))
    return -1;                  /* ended on non-space/nul */

  return value;
}

void 
taru_to_oct(register long value, register int digits,
			register char *  where)
{
  --digits;                     /* Leave the trailing NUL slot alone.  */
  where[--digits] = ' ';        /* Put in the space, though.  */

  /* Produce the digits -- at least one.  */
  do
    {
      where[--digits] = '0' + (char) (value & 7); /* One octal digit.  */
      value >>= 3;
    }
  while (digits > 0 && value != 0);

  /* Add leading spaces, if necessary.  */
  while (digits > 0)
    where[--digits] = ' ';
}

/*
* Decimal ascii to unsigned long
*     return 0 if conversion OK
*     return 1 if failed to convert string to end.
*     return 2 if overflow.
*/
int
taru_datoul (char * s, unsigned long *n)
{
  unsigned long oldval = 0;
  unsigned long val = 0;
	
  while (*s == ' ')
    ++s;

  while (*s >= '0' && *s <= '9' && val >= oldval) {
          /* fprintf(stderr, "%lu %lu\n", oldval, val); */
          oldval = val;
          val = 10 * val + *s++ - '0';
  }
  if (oldval && ((val / oldval) < 10)) {
	fprintf(stderr, "%s, taru_datoul : conversion overflow\n", swlib_utilname_get());
	return 2;
  }
  while (*s == ' ') ++s;
  *n = val;
  return (!(*s == '\0'));
}

/*
* Octal Acsii to unsigned long
*/
int
taru_otoul (char * s, unsigned long *n)
{
  unsigned long val = 0;

  while (*s == ' ')
    ++s;
  while (*s >= '0' && *s <= '7')
    val = 8 * val + *s++ - '0';
  while (*s == ' ')
    ++s;
  *n = val;
  return *s == '\0';
}

int
taru_otoumax (char * s, uintmax_t *n)
{
  uintmax_t val = 0;

  while (*s == ' ')
    ++s;
  while (*s >= '0' && *s <= '7')
    val = 8 * val + *s++ - '0';
  while (*s == ' ')
    ++s;
  *n = val;
  return *s == '\0';
}

int taru_tarheader_check ( char * buffer ) {
  unsigned long sumis, sum;

  if ( strncmp (buffer + 257, TMAGIC, 5 )) {
    return 1;
  }
  sumis = (unsigned long) taru_from_oct ( 8 , buffer + 148);
  sum = taru_tar_checksum ((void *) buffer);
  if ( sum != sumis ) {
	return -1;
  }
  return 0;
}

unsigned long taru_tar_checksum (void * hdr)
{
  
  struct tar_header *tar_hdr = (struct tar_header*)(hdr); 
  
  unsigned long sum = 0;
  char *p = (char *) tar_hdr;
  char *q = p + TARRECORDSIZE;
  int i;

  while (p < tar_hdr->chksum)
    sum += *p++ & 0xff;
  for (i = 0; i < 8; ++i)
    {
      sum += ' ';
      ++p;
    }
  while (p < q)
    sum += *p++ & 0xff;
  return sum;
}

int
taru_tape_skip_padding (int in_file_des, uintmax_t offset, 
			enum archive_format archive_format_in)
{
  int pad;

  if (archive_format_in == arf_crcascii || archive_format_in == arf_newascii)
    pad = (4 - (offset % 4)) % 4;
  else if (archive_format_in == arf_binary || archive_format_in == arf_hpbinary)
    pad = (2 - (offset % 2)) % 2;
  else if (archive_format_in == arf_tar || archive_format_in == arf_ustar)
    pad = (512 - (offset % 512)) % 512;
  else
    pad = 0;

  if (pad != 0) {
    if (in_file_des < 0) {
   	return pad; 
    } else { 
    	return taru_read_amount(in_file_des, pad);
    }
  }
  return 0;
}
