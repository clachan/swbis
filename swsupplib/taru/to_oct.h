/* to_oct.h: Common declarations to_oct module.
   Based on common.c
   Copyright 1988, 92, 93, 94, 96, 97, 99, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef TO_OCT_H_20050303
#define TO_OCT_H_20050303
#include "swuser_config.h"

/* Declare the GNU tar archive format.  */
/* #include "tar.h" */

/* The checksum field is filled with this while the checksum is computed.  */
#define CHKBLANKS	"        "	/* 8 blanks, no null */

/* Some constants from POSIX are given names.  */
#define NAME_FIELD_SIZE   100
#define PREFIX_FIELD_SIZE 155
#define UNAME_FIELD_SIZE   32
#define GNAME_FIELD_SIZE   32

/*
 * #ifdef HAVE_INTTYPES_H
 * #include <inttypes.h>
 * #ifndef uintmax_t
 * #define uintmax_t unsigned long int
 * #endif
 * #else
 * #define uintmax_t unsigned long int
 * #endif
 */

/* These macros work even on ones'-complement hosts (!).
   The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))

/* Removed by jhl on 17MAR2013 to eliminate compiler warning.
   They don't appear anywhere.
#define TYPE_MINIMUM(t) (TYPE_SIGNED (t) \
			 ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) \
			 : (t) 0)
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))
*/


/* Bound on length of the string representing an integer value of type t.
   Subtract one for the sign bit if t is signed;
   302 / 1000 is log10 (2) rounded up;
   add one for integer division truncation;
   add one more for a minus sign if t is signed.  */
#define x_INT_STRLEN_BOUND(t) \
  ((sizeof (t) * CHAR_BIT - TYPE_SIGNED (t)) * 302 / 1000 \
   + 1 + TYPE_SIGNED (t))

#define UINTMAX_STRSIZE_BOUND (x_INT_STRLEN_BOUND (uintmax_t) + 1)


enum gnutar_archive_format
{
  DEFAULT_FORMAT,		/* format to be decided later */
  V7_FORMAT,			/* old V7 tar format */
  OLDGNU_FORMAT,		/* GNU format as per before tar 1.12 */
  POSIX_FORMAT,			/* restricted, pure POSIX format */
  GNU_FORMAT			/* POSIX format with GNU extensions */
};

#if MSDOS
# define TTY_NAME "con"
#else
# define TTY_NAME "/dev/tty"
#endif

#define TAREXIT_SUCCESS 0
#define TAREXIT_DIFFERS 1
#define TAREXIT_FAILURE 2

/* Both WARN and ERROR write a message on stderr and continue processing,
   however ERROR manages so tar will exit unsuccessfully.  FATAL_ERROR
   writes a message on stderr and aborts immediately, with another message
   line telling so.  USAGE_ERROR works like FATAL_ERROR except that the
   other message line suggests trying --help.  All four macros accept a
   single argument of the form ((0, errno, _("FORMAT"), Args...)).  `errno'
   is `0' when the error is not being detected by the system.  */

#define WARN(Args) \
  error Args
#define ERROR(Args) \
  (error Args, exit_status = TAREXIT_FAILURE)
#define FATAL_ERROR(Args) \
  (error Args, fatal_exit ())
#define USAGE_ERROR(Args) \
  (error Args, usage (TAREXIT_FAILURE))

/* Log base 2 of common values.  */
#define LG_8 3
#define LG_64 6
#define LG_256 8


#define GID_TO_CHARS(val, where, termch) gid_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define MAJOR_TO_CHARS(val, where, termch) major_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define MINOR_TO_CHARS(val, where, termch) minor_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define MODE_TO_CHARS(val, where, termch) mode_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define MODE_TO_OLDGNU_CHARS(val, where, termch) mode_to_chars (val, where, sizeof (where), OLDGNU_FORMAT, termch)
#define OFF_TO_CHARS(val, where, termch) off_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define SIZE_TO_CHARS(val, where, termch) size_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define TIME_TO_CHARS(val, where, termch) time_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define UID_TO_CHARS(val, where, termch) uid_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)
#define UINTMAX_TO_CHARS(val, where, termch) uintmax_to_chars (val, where, sizeof (where), POSIX_FORMAT, termch)

#define PARAMS(Args) Args
void gid_to_chars PARAMS ((gid_t, char *, size_t, int OSIX_FORMAT, int termch));
void major_to_chars PARAMS ((major_t, char *, size_t, int OSIX_FORMAT, int termch));
void minor_to_chars PARAMS ((minor_t, char *, size_t, int OSIX_FORMAT, int termch));
void mode_to_chars PARAMS ((mode_t, char *, size_t, int OSIX_FORMAT, int termch));
void off_to_chars PARAMS ((off_t, char *, size_t, int OSIX_FORMAT, int termch));
void size_to_chars PARAMS ((size_t, char *, size_t, int OSIX_FORMAT, int termch));
void time_to_chars PARAMS ((time_t, char *, size_t, int OSIX_FORMAT, int termch));
void uid_to_chars PARAMS ((uid_t, char *, size_t, int OSIX_FORMAT, int termch));
void uintmax_to_chars PARAMS ((uintmax_t, char *, size_t, int OSIX_FORMAT, int termch));


char *stringify_uintmax_t_backwards PARAMS ((uintmax_t, char *));

/* char const *tartime PARAMS ((time_t)); */

#define GID_FROM_HEADER(where) gid_from_header (where, sizeof (where))
#define MAJOR_FROM_HEADER(where) major_from_header (where, sizeof (where))
#define MINOR_FROM_HEADER(where) minor_from_header (where, sizeof (where))
#define MODE_FROM_HEADER(where) mode_from_header (where, sizeof (where))
#define OFF_FROM_HEADER(where) off_from_header (where, sizeof (where))
#define SIZE_FROM_HEADER(where) size_from_header (where, sizeof (where))
#define TIME_FROM_HEADER(where) time_from_header (where, sizeof (where))
#define UID_FROM_HEADER(where) uid_from_header (where, sizeof (where))
#define UINTMAX_FROM_HEADER(where) uintmax_from_header (where, sizeof (where))

gid_t gid_from_header PARAMS ((const char *, size_t));
major_t major_from_header PARAMS ((const char *, size_t));
minor_t minor_from_header PARAMS ((const char *, size_t));
mode_t mode_from_header PARAMS ((const char *, size_t));
off_t off_from_header PARAMS ((const char *, size_t));
size_t size_from_header PARAMS ((const char *, size_t));
time_t time_from_header PARAMS ((const char *, size_t));
uid_t uid_from_header PARAMS ((const char *, size_t));
uintmax_t uintmax_from_header PARAMS ((const char *, size_t));

#endif
