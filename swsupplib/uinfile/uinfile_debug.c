/*  uinfile_debug.c:
 */

/*
 * Copyright (C) 1999  James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "ahs.h"
#include "uinfile.h"
#include "swlib.h"

#include "debug_config.h"
#ifdef UINFILENEEDDEBUG
#define UINFILE_E_DEBUG(format) SWBISERROR("UINFILE DEBUG: ", format)
#define UINFILE_E_DEBUG2(format, arg) SWBISERROR2("UINFILE DEBUG: ", format, arg)
#define UINFILE_E_DEBUG3(format, arg, arg1) SWBISERROR3("UINFILE DEBUG: ", format, arg, arg1)
#else
#define UINFILE_E_DEBUG(arg)
#define UINFILE_E_DEBUG2(arg, arg1)
#define UINFILE_E_DEBUG3(arg, arg1, arg2)
#endif /* UINFILENEEDDEBUG */


/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
#undef NODEFINE
#ifdef NODEFINE
typedef struct {
   int fdM;
   int typeM;		/* Package Format */
   int ztypeM;		/* Compression Format */
   int layout_typeM;	/* Layout type */
   int did_dupeM;
   struct new_cpio_header * file_hdrM;
   int pidlistM[20];
   int verboseM;
} UINFORMAT;
#endif
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */


static STROB * buf = NULL;

char *
uinfile_debug(UINFORMAT * uin, char * prefix)
{
	int i;
	char prebuf[200];

	if (!buf) buf = strob_open(100);

	if (!prefix) prefix = "";

	strob_sprintf(buf, 0, "%s%p UINFORMAT*\n", prefix, uin);
	strob_sprintf(buf, 1, "%s%p->fdM          = [%d]\n", prefix, (void*)(uin), uin->fdM);
	if (uin->fdM >= 0) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->fdM ", prefix, (void*)(uin));
		strob_sprintf(buf, 1, "%s", uxfio_dump_string_s(uin->fdM, prebuf));
	}
	strob_sprintf(buf, 1, "%s%p->typeM        = [%d]\n", prefix, (void*)(uin), uin->typeM);
	strob_sprintf(buf, 1, "%s%p->ztypeM       = [%d]\n", prefix, (void*)(uin), uin->ztypeM);
	strob_sprintf(buf, 1, "%s%p->layout_typeM = [%d]\n", prefix, (void*)(uin), uin->layout_typeM);
	strob_sprintf(buf, 1, "%s%p->did_dupeM    = [%d]\n", prefix, (void*)(uin), uin->did_dupeM);
	strob_sprintf(buf, 1, "%s%p->file_hdrM    = [%p]\n", prefix, (void*)(uin), (void*)(uin->file_hdrM));

	strob_sprintf(buf, 1, "%s%p->pidlist[] = ", prefix, (void*)(uin->fdM));
	i = 0;
	while (i < sizeof(uin->pidlistM)/sizeof(int)) {
		if ((uin->pidlistM)[i] != 0) {
			strob_sprintf(buf, 1, " ");
			strob_sprintf(buf, 1, "[%d]", (uin->pidlistM)[i]);
		}
		i++;
	}
	strob_sprintf(buf, 1, "\n");
	return strob_str(buf);
}

