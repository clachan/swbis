/* swvarf_debugs.c  --  File I/O calls on an archive stream and directory file.

   Copyright (C) 1999  Jim Lowe 
   All rights reserved.

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
#include "swlib.h"

#include "debug_config.h"
#ifdef SWVARFSNEEDDEBUG
#define SWVARFS_E_DEBUG(format) SWBISERROR("SWVARFS DEBUG: ", format)
#define SWVARFS_E_DEBUG2(format, arg) SWBISERROR2("SWVARFS DEBUG: ", format, arg)
#define SWVARFS_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWVARFS DEBUG: ", format, arg, arg1)
#else
#define SWVARFS_E_DEBUG(arg)
#define SWVARFS_E_DEBUG2(arg, arg1)
#define SWVARFS_E_DEBUG3(arg, arg1, arg2)
#endif /* SWVARFSNEEDDEBUG */


static STROB * buf = NULL;

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
#undef NODEFINED
#ifdef NODEFINED
typedef struct {
	UINFORMAT * format_descM;	/* The Format descriptor, see uinfile.c */
	int formatM;			/* The format,  see uinfile.h */
	STROB * openpathM;		/* The opening path. */
	STROB * opencwdpathM;		/* The current working directory. */
	STROB * u_nameM;		/* Temporary name of current user file. */
	STROB * dirscopeM;		/* The current scope, trversals are limited to this directory. */
	STROB * tmpM;			/* Tempory memory. */
	AHS * ahsM;			/* File metadata object. */
	int do_close_ahsM;
	int fdM;
	int did_dupM;
	
	/* Archive member files. */
	char * u_current_name_;		/* u_<*> are attributes for the opening of */
	char * u_linkname_;		/* individual files. */
	int    u_fdM;			/* file descriptor, maybe a uxfio_fd */

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
	int next_file_offsetM;
	int is_unix_pipeM;
	int eoaM;			/* End of archive reached ?? */
	int makefilechainM;		/* Override in-order traversal optimization. */
} SWVARFS;
#endif
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/


char * 
swvarfs_dump_string_s(SWVARFS * swvarfs, char * prefix)
{
	char prebuf[200];
	if (buf == (STROB*)NULL) buf = strob_open(100);

	strob_sprintf(buf, 0, "%s%p (SWVARFS*)\n", prefix,  (void*)swvarfs);
	strob_sprintf(buf, 1, "%s%p->format_descM    = [%p]\n",  prefix, (void*)swvarfs, (void*)(swvarfs->format_descM));
	
	if (swvarfs->format_descM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(swvarfs), (void*)(swvarfs->format_descM));
		strob_sprintf(buf, 1, "%s", uinfile_debug(swvarfs->format_descM, prebuf));
	}

	strob_sprintf(buf, 1, "%s%p->formatM         = [%d]\n", prefix, (void*)swvarfs, (void*)(swvarfs->formatM));
	strob_sprintf(buf, 1, "%s%p->openpathM       = [%s]\n", prefix, (void*)swvarfs, strob_str(swvarfs->openpathM));
	strob_sprintf(buf, 1, "%s%p->opencwdpathM    = [%s]\n", prefix, (void*)swvarfs, strob_str(swvarfs->opencwdpathM));
	strob_sprintf(buf, 1, "%s%p->u_nameM         = [%s]\n", prefix, (void*)swvarfs, strob_str(swvarfs->u_nameM));
	strob_sprintf(buf, 1, "%s%p->dirscopeM       = [%s]\n", prefix, (void*)swvarfs, strob_str(swvarfs->dirscopeM));
	strob_sprintf(buf, 1, "%s%p->vcwdM           = [%s]\n", prefix, (void*)swvarfs, strob_str(swvarfs->vcwdM));
	strob_sprintf(buf, 1, "%s%p->do_close_ahs    = [%d]\n", prefix, (void*)swvarfs, swvarfs->do_close_ahsM);
	strob_sprintf(buf, 1, "%s%p->ahsM            = [%p]\n",  prefix, (void*)swvarfs, (void*)(swvarfs->ahsM));
	if (swvarfs->ahsM) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->%p ", prefix, (void*)(swvarfs), (void*)(swvarfs->ahsM));
		strob_sprintf(buf, 1, "%s", ahs_dump_string_s(swvarfs->ahsM, prebuf));
	}
	
	
	strob_sprintf(buf, 1, "%s%p->fdM             = [%d]\n", prefix, (void*)swvarfs, swvarfs->fdM);

	if (swvarfs->fdM >= 0) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->fdM ", prefix, (void*)(swvarfs));
		strob_sprintf(buf, 1, "%s", uxfio_dump_string_s(swvarfs->fdM, prebuf));
	}

	strob_sprintf(buf, 1, "%s%p->did_dupM        = [%d]\n", prefix, (void*)swvarfs, swvarfs->did_dupM);
	strob_sprintf(buf, 1, "%s%p->u_current_name_ = [%s]\n", prefix, (void*)swvarfs, swvarfs->u_current_name_);
	strob_sprintf(buf, 1, "%s%p->g_linkname_     = [%s]\n", prefix, (void*)swvarfs, swvarfs->g_linkname_);
	strob_sprintf(buf, 1, "%s%p->u_fdM_          = [%d]\n", prefix, (void*)swvarfs, swvarfs->u_fdM);
	strob_sprintf(buf, 1, "%s%p->direntpathM     = [%s]\n", prefix, (void*)swvarfs, strob_str(swvarfs->direntpathM));
	strob_sprintf(buf, 1, "%s%p->stackixM        = [%d]\n", prefix, (void*)swvarfs, swvarfs->stackixM);
	strob_sprintf(buf, 1, "%s%p->derrM           = [%d]\n", prefix, (void*)swvarfs, swvarfs->derrM);
	strob_sprintf(buf, 1, "%s%p->have_read_filesM = [%d]\n", prefix, (void*)swvarfs, swvarfs->have_read_filesM);
	strob_sprintf(buf, 1, "%s%p->uxfio_buftype    = [%d]\n", prefix, (void*)swvarfs, swvarfs->uxfio_buftype);
	strob_sprintf(buf, 1, "%s%p->is_unix_pipeM     = [%d]\n", prefix, (void*)swvarfs, swvarfs->is_unix_pipeM);
	strob_sprintf(buf, 1, "%s%p->eoaM              = [%d]\n", prefix, (void*)swvarfs, swvarfs->eoaM);
	strob_sprintf(buf, 1, "%s%p->has_leading_slashM= [%d]\n", prefix, (void*)swvarfs, swvarfs->has_leading_slashM);
	strob_sprintf(buf, 1, "%s%p->makefilechainM    = [%d]\n", prefix, (void*)swvarfs, swvarfs->makefilechainM);
	strob_sprintf(buf, 1, "%s%p->headM              = [%p]\n", prefix, (void*)swvarfs, swvarfs->headM);

	{
		struct fileChain *last=swvarfs->headM;
		while(last) {
			snprintf(prebuf, sizeof(prebuf)-1, "%s%p [%p]", prefix, (void*)swvarfs, (void*)last);

			strob_sprintf(buf, 1, "%s  nameFC=[%s]\n",  prebuf, last->nameFC);
			strob_sprintf(buf, 1, "%s  header_offsetFC=[%s]\n",  prebuf, last->header_offsetFC);
			strob_sprintf(buf, 1, "%s  data_offsetFC=[%s]\n",  prebuf, last->data_offsetFC);

			last=last->nextFC;
		}
	}

	return strob_str(buf);
}

int
swvarfs_debug_list_files(SWVARFS * swvarfs, int ofd)
{
	char * path;
	struct stat st;
	STROB * buffer = strob_open(10);
	int ret = 0;
                path = swvarfs_get_next_dirent(swvarfs, &st);
                while (path) {
			ret += swlib_writef(ofd, buffer, "%s\n", path);
			path = swvarfs_get_next_dirent(swvarfs, &st);
                }

	strob_close(buffer);
	return ret;	
}
