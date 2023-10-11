/* uxfioextra.c : buffered u*ix I/O functions.
 */

/*
 * Copyright (C) 1997-2002 James H. Lowe, Jr. <jhlowe@acm.org>
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

#include "uxfio.h"
#include <errno.h>
#include <time.h>

#define o__inline__

#include "uxfio_i.h"
#include "strob.h"

#include "debug_config.h"

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
#define UXFIO_E_DEBUG3(format, arg, arg1) SWBISERROR3("UXFIO DEBUG: ", format, arg, arg1)
#else
#define UXFIO_E_DEBUG(arg)
#define UXFIO_E_DEBUG2(arg, arg1)
#define UXFIO_E_DEBUG3(arg, arg1, arg2)
#endif /* UXFIONEEDDEBUG */

int 
uxfio_dump_internal(int uxfio_fildes, UXFIO * uxfio)
{
	fprintf(stderr, "%s", uxfio_dump_string(uxfio_fildes));
	return 0;
}


static int
uxfio_dump_string_recursive(int uxfio_fildes, UXFIO * uxfio,  char * prefix, char ** a)
{
	char *p;
	char * prebuf = malloc(400);

	static char aerror[3];

	aerror[0] = '\0';
	p = *a;

	if (!prefix) prefix="";	


	if (uxfio_fildes < UXFIO_FD_MIN) {
		p += snprintf(p, 300, "%sUnix fd = [%d]\n", prefix, uxfio_fildes);
		p += snprintf(p, 300, "%s[%d] lseek(fd, 0, SEEKCUR) = [%d].\n",
			prefix, uxfio_fildes, (int)lseek(uxfio_fildes, (off_t)0, SEEK_CUR));
		free(prebuf);
		return (int)(p - *a);
	}

	if (!uxfio) 
		uxfio = uxfio_debug_get_object_address(uxfio_fildes);

	if (!uxfio) {
		p += snprintf(p, 300, "%s UXFIO DEBUG ERROR on fd = [%d]\n", prefix, uxfio_fildes);
		free(prebuf);
		return (int)(p - *a);
	}

	p += snprintf(p, 270, "%s uxfiofd          = [%d]\n", prefix,  uxfio_fildes);
	p += snprintf(p, 270, "%s uxfdM            = [%d]\n", prefix,  uxfio->uxfdM);
	
	strncpy(prebuf, prefix, 300);
	snprintf(prebuf + strlen(prebuf), 70, "[%d]", uxfio->uxfdM);

	p += uxfio_dump_string_recursive(uxfio->uxfdM, NULL,  prebuf,  &p);

	p += snprintf(p, 270, "%s uxfd_can_seekM   = [%d]\n", prefix,  uxfio->uxfd_can_seekM);
	p += snprintf(p, 270, "%s posM             = [%d]\n", prefix,  uxfio->posM);
	p += snprintf(p, 270, "%s startM           = [%d]\n", prefix,  uxfio->startM);
	p += snprintf(p, 270, "%s endM             = [%d]\n", prefix,  uxfio->endM);
	p += snprintf(p, 270, "%s lenM             = [%d]\n", prefix,  uxfio->lenM);
	p += snprintf(p, 270, "%s errorM           = [%d]\n", prefix,  uxfio->errorM);
	p += snprintf(p, 270, "%s current_offsetM  = [%d]\n", prefix,  uxfio->current_offsetM);
	p += snprintf(p, 270, "%s virtual_offsetM  = [%d]\n", prefix,  uxfio->virtual_offsetM);
	p += snprintf(p, 270, "%s bytesreadM       = [%d]\n", prefix,  uxfio->bytesreadM);
	p += snprintf(p, 270, "%s buffd            = [%d]\n", prefix,  uxfio->buffdM);
	p += snprintf(p, 270, "%s buffer active    = [%d]\n", prefix,  uxfio->buffer_activeM);
	p += snprintf(p, 270, "%s buffer type      = [%d]\n", prefix,  uxfio->buffertypeM);
	p += snprintf(p, 270, "%s buffilename      = [%s]\n", prefix,  uxfio->buffilenameM);
	p += snprintf(p, 270, "%s use count        = [%d]\n", prefix,  uxfio->use_countM);
	p += snprintf(p, 270, "%s offset eof       = [%d]\n", prefix,  (int)(uxfio->offset_eofM));
	p += snprintf(p, 270, "%s offset_eofM         = [%d]\n", prefix, (int)(uxfio->offset_eofM));
	p += snprintf(p, 270, "%s offset_eof_savedM   = [%d]\n", prefix, (int)(uxfio->offset_eof_savedM));
	p += snprintf(p, 270, "%s offset_bofM         = [%d]\n", prefix, (int)(uxfio->offset_bofM));
	p += snprintf(p, 270, "%s v_endM              = [%d]\n", prefix, (int)(uxfio->v_endM));

	free(prebuf);
	return  (int)(p - *a);
}


char *
uxfio_dump_string_s(int uxfio_fildes, char * prefix)
{
	char * prebuf = malloc(500);
	static char * newbuf = NULL;
	char * newbufp;
	UXFIO * uxfio = uxfio_debug_get_object_address(uxfio_fildes);

	if (!newbuf) {
		newbuf = (char*) malloc(10240);
	}
	newbufp = newbuf;

	if (!prefix) prefix="";

	snprintf(prebuf, 70, "%s[%d]", prefix, uxfio_fildes);

	uxfio_dump_string_recursive(uxfio_fildes, uxfio,  prebuf, &newbufp);
	
	free(prebuf);
	return newbuf;
}

char *
uxfio_dump_string(int uxfio_fildes)
{
	char * s;
	s = uxfio_dump_string_s(uxfio_fildes, "");
	return s;
}

/*
char *
uxfio_dump_string_internal(int uxfio_fildes, UXFIO * uxfio_object)
{
	UXFIO *uxfio;
	static char a[1024];
	char *p;
	struct stat st;

	a[0] = '\0';
	p = a;
	if (uxfio_fildes >=0 && uxfio_fildes < UXFIO_FD_MIN) {
		fprintf(stderr, "fd [%d] is not a uxfio descriptor.\n", uxfio_fildes);
		p += snprintf(p, 50, "uxfio_dump_string: <unix fd>\n");
		p += snprintf(p, 50, "file desc: %d\n", uxfio_fildes);
		p += snprintf(p, 50, "file desc: %d current offset=%d\n", uxfio_fildes, (int)lseek(uxfio_fildes, (off_t)0, SEEK_CUR));
		fstat(uxfio_fildes, &st);
		p += snprintf(p, 50, "file desc: %d st_size=%d\n", uxfio_fildes, (int)st.st_size);
		return a;
	} else if (uxfio_object == NULL && uxfio_fildes >= UXFIO_FD_MIN) {
		if (table_find(uxfio_fildes, (void **) (&uxfio))) {
			UXFIO_E_DEBUG("error");
			fprintf(stderr, "uxfio error: file desc %d not found.\n", uxfio_fildes);
			return a;
		}
	} else if (uxfio_object){
		uxfio = uxfio_object;
	} else {
		return "internal error in uxfio_dump_string_internal";
	}	

	p += snprintf(p, 50, "uxfiofd          : %d\n", uxfio_fildes);
	p += snprintf(p, 50, "uxfdM            : %d\n", uxfio->uxfdM);
	if (uxfio->uxfdM < UXFIO_FD_MIN) {
		fstat(uxfio->uxfdM, &st);
		p += snprintf(p, 50, "    Unix file desc: %d st_size=%d\n", uxfio->uxfdM, (int)st.st_size);
	}
	p += snprintf(p, 50, "uxfd_can_seekM        : %d\n", uxfio->uxfd_can_seekM);
	p += snprintf(p, 50, "posM             : %d\n", uxfio->posM);
	p += snprintf(p, 50, "startM           : %d\n", uxfio->startM);
	p += snprintf(p, 50, "endM             : %d\n", uxfio->endM);
	p += snprintf(p, 50, "lenM             : %d\n", uxfio->lenM);
	p += snprintf(p, 50, "errorM           : %d\n", uxfio->errorM);
	p += snprintf(p, 50, "current_offsetM   : %d\n", uxfio->current_offsetM);
	p += snprintf(p, 50, "virtual_offsetM   : %d\n", uxfio->virtual_offsetM);
	p += snprintf(p, 50, "bytesreadM        : %d\n", uxfio->bytesreadM);
	p += snprintf(p, 50, "buffd             : %d\n", uxfio->buffdM);
	p += snprintf(p, 50, "buffer_activeM     : %d\n", uxfio->buffer_activeM);
	p += snprintf(p, 50, "buffertypeM        : %d\n", uxfio->buffertypeM);
	p += snprintf(p, 50, "buffilename        : %s\n", uxfio->buffilenameM);
	p += snprintf(p, 50, "use_countM         : %d\n", uxfio->use_countM);
	p += snprintf(p, 50, "offset_eofM        : %d\n", (int)(uxfio->offset_eofM));
	p += snprintf(p, 50, "offset_eof_savedM  : %d\n", (int)(uxfio->offset_eof_savedM));
	p += snprintf(p, 50, "offset_bofM        : %d\n", (int)(uxfio->offset_bofM));
	p += snprintf(p, 50, "v_endM             : %d\n", (int)(uxfio->v_endM));
	

	return a;
}

char *
uxfio_dump_string_from_object(UXFIO * object)
{
	char * p;
	p = uxfio_dump_string(object->uxfio_fildesM);
	return p;
}
*/

int 
uxfio_debug_dump(int uxfio_fildes)
{
	int ret;
	UXFIO_E_DEBUG("error");
	ret = fprintf(stderr, "%s", uxfio_dump_string(uxfio_fildes));
	/*
	if (uxfio_fildes >= UXFIO_FD_MIN) {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "%s", uxfio_dump_string(uxfio_fildes));
		return 0;
	} else {
		UXFIO_E_DEBUG("error");
		fprintf(stderr, "[%d] Unix file descriptor.\n", uxfio_fildes);
		fprintf(stderr, "[%d] lseek(fd, 0, SEEKCUR) = %d.\n", uxfio_fildes, (int)lseek(uxfio_fildes, (off_t)0, SEEK_CUR));
	}
	*/

	return 0;
}


