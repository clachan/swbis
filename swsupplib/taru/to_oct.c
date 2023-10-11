/* to_oct.c : derived from create.c to isolate the to_oct routines.
		GNU tar version tar-1.13.17(.tar.gz)

   Copyright 1985, 92, 93, 94, 96, 97, 99, 2000 Free Software Foundation, Inc.
   Written by John Gilmore, on 1985-08-25.

   Modified by Jim Lowe for inclusion in swbis.

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

/* #include "system.h" */
#include "swuser_config.h"
#include "swutilname.h"
#include "ahs.h"

#define STRINGIFY_BIGINT(i, b) \
  stringify_uintmax_t_backwards ((uintmax_t) (i), (b) + UINTMAX_STRSIZE_BOUND)

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#ifndef CHAR_MAX
# define CHAR_MAX TYPE_MAXIMUM (char)
#endif

#ifndef UCHAR_MAX
# define UCHAR_MAX TYPE_MAXIMUM (unsigned char)
#endif

#ifndef LONG_MAX
# define LONG_MAX TYPE_MAXIMUM (long)
#endif

#if HAVE_SIGNED_CHAR
# define signed_char(x) ((signed char) (x))
#else
# define signed_char(x) \
    ('\200' < 0 \
     ? (char) (x) \
     : ((char) (x) \
        << CHAR_BIT * (sizeof (int) - 1) \
	>> CHAR_BIT * (sizeof (int) - 1)))
#endif

# include <stdio.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <pwd.h>
# include <grp.h>
# include <utime.h>
# include <tar.h>
# include "to_oct.h"
#include "taru.h"

/*
#if HAVE_UTIME_H
# include <utime.h>
#else
struct utimbuf
  {
    long actime;
    long modtime;
  };
#endif
*/

/*----------------------------------------------------------------------.
| Format O as a null-terminated decimal string into BUF _backwards_;    |
| return pointer to start of result.                                    |
`----------------------------------------------------------------------*/
char *
stringify_uintmax_t_backwards (uintmax_t o, char *buf)
{
  *--buf = '\0';
  do
    *--buf = '0' + (int) (o % 10);
  while ((o /= 10) != 0);
  return buf;
}

/* The maximum uintmax_t value that can be represented with DIGITS digits,
   assuming that each digit is BITS_PER_DIGIT wide.  */
#define MAX_VAL_WITH_DIGITS(digits, bits_per_digit) \
   ((digits) * (bits_per_digit) < sizeof (uintmax_t) * CHAR_BIT \
    ? ((uintmax_t) 1 << ((digits) * (bits_per_digit))) - 1 \
    : (uintmax_t) -1)

/* Convert VALUE to an octal representation suitable for tar headers.
   Output to buffer WHERE with size SIZE.
   The result is undefined if SIZE is 0 or if VALUE is too large to fit.  */

static void
to_octal (uintmax_t value, char *where, size_t size)
{
  uintmax_t v = value;
  size_t i = size;

  do
    {
      where[--i] = '0' + (v & ((1 << LG_8) - 1));
      v >>= LG_8;
    }
  while (i);
}

/* Convert NEGATIVE VALUE to a base-256 representation suitable for
   tar headers.  NEGATIVE is 1 if VALUE was negative before being cast
   to uintmax_t, 0 otherwise.  Output to buffer WHERE with size SIZE.
   The result is undefined if SIZE is 0 or if VALUE is too large to
   fit.  */

static void
to_base256 (int negative, uintmax_t value, char *where, size_t size)
{
  uintmax_t v = value;
  uintmax_t propagated_sign_bits =
    ((uintmax_t) - negative << (CHAR_BIT * sizeof v - LG_256));
  size_t i = size;

  do
    {
      where[--i] = v & ((1 << LG_256) - 1);
      v = propagated_sign_bits | (v >> LG_256);
    }
  while (i);
}

/* Convert NEGATIVE VALUE (which was originally of size VALSIZE) to
   external form, using SUBSTITUTE (...) if VALUE won't fit.  Output
   to buffer WHERE with size SIZE.  NEGATIVE is 1 iff VALUE was
   negative before being cast to uintmax_t; its original bitpattern
   can be deduced from VALSIZE, its original size before casting.
   TYPE is the kind of value being output (useful for diagnostics).
   Prefer the POSIX format of SIZE - 1 octal digits (with leading zero
   digits), followed by '\0'.  If this won't work, and if GNU or
   OLDGNU format is allowed, use '\200' followed by base-256, or (if
   NEGATIVE is nonzero) '\377' followed by two's complement base-256.
   If neither format works, use SUBSTITUTE (...)  instead.  Pass to
   SUBSTITUTE the address of an 0-or-1 flag recording whether the
   substitute value is negative.  */

static void
to_chars (int negative, uintmax_t value, size_t valsize,
	  uintmax_t (*substitute) PARAMS ((int *)),
	  char *where, size_t size, const char *type, int archive_format, int termch)
{
  int base256_allowed = (archive_format == GNU_FORMAT
			 || archive_format == OLDGNU_FORMAT);

  /* Generate the POSIX octal representation if the number fits.  */
  if (! negative && value <= MAX_VAL_WITH_DIGITS (size - 1, LG_8))
    {
      where[size - 1] = termch;
      to_octal (value, where, size - 1);
    }
  /* Otherwise, if uid or gid type store the maximum value that
     can be stored in tar format.  */
  else if (strcmp(type, "uid_t") == 0 || strcmp(type, "gid_t") == 0) {
	value = 8 ^ (size - 1);  /* i.e. 07777777 */
	to_chars(negative, value, valsize, substitute, where, size, "id_again", archive_format, termch);
  }
  /* Otherwise, generate the base-256 representation if we are
     generating an old or new GNU format and if the number fits.  */
  else if (((negative ? -1 - value : value)
	    <= MAX_VAL_WITH_DIGITS (size - 1, LG_256))
	   && base256_allowed)
    {
      fprintf(stderr, "%s: to_chars: making old gnu field\n", swlib_utilname_get());
      where[0] = negative ? -1 : 1 << (LG_256 - 1);
      to_base256 (negative, value, where + 1, size - 1);
    }

  /* Otherwise, if the number is negative, and if it would not cause
     ambiguity on this host by confusing positive with negative
     values, then generate the POSIX octal representation of the value
     modulo 2**(field bits).  The resulting tar file is
     machine-dependent, since it depends on the host word size.  Yuck!
     But this is the traditional behavior.  */
  else if (negative && valsize * CHAR_BIT <= (size - 1) * LG_8)
    {
      static int warned_once;
      fprintf(stderr, "%s: to_chars: making non portable field\n", swlib_utilname_get());
      if (! warned_once)
	{
	  warned_once = 1;
	  fprintf(stderr,"Generating negative octal headers");
	}
      where[size - 1] = '\0';
      to_octal (value & MAX_VAL_WITH_DIGITS (valsize * CHAR_BIT, 1),
		where, size - 1);
    }

  /* Otherwise, output a substitute value if possible (with a
     warning), and an error message if not.  */
  else
    {
      uintmax_t maxval = (base256_allowed
			  ? MAX_VAL_WITH_DIGITS (size - 1, LG_256)
			  : MAX_VAL_WITH_DIGITS (size - 1, LG_8));
      char valbuf[UINTMAX_STRSIZE_BOUND + 1];
      char maxbuf[UINTMAX_STRSIZE_BOUND];
      char minbuf[UINTMAX_STRSIZE_BOUND + 1];
      char const *minval_string;
      char const *maxval_string = STRINGIFY_BIGINT (maxval, maxbuf);
      char const *value_string;

      fprintf(stderr, "%s: to_chars: making substitute value.\n", swlib_utilname_get());
      if (base256_allowed)
	{
	  uintmax_t m = maxval + 1 ? maxval + 1 : maxval / 2 + 1;
	  char *p = STRINGIFY_BIGINT (m, minbuf + 1);
	  *--p = '-';
	  minval_string = p;
	}
      else
	minval_string = "0";

      if (negative)
	{
	  char *p = STRINGIFY_BIGINT (- value, valbuf + 1);
	  *--p = '-';
	  value_string = p;
	}
      else
	value_string = STRINGIFY_BIGINT (value, valbuf);

      if (substitute)
	{
	  int negsub;
	  uintmax_t sub = substitute (&negsub) & maxval;
	  uintmax_t s = (negsub &= archive_format == GNU_FORMAT) ? - sub : sub;
	  char subbuf[UINTMAX_STRSIZE_BOUND + 1];
	  char *sub_string = STRINGIFY_BIGINT (s, subbuf + 1);
	  if (negsub)
	    *--sub_string = '-';
	  fprintf(stderr, "value %s out of %s range %s..%s; substituting %s\n",
		 value_string, type, minval_string, maxval_string,
		 sub_string);
	  to_chars (negsub, s, valsize, 0, where, size, type, archive_format, termch);
	}
      else
	fprintf(stderr,"value %s out of %s range %s..%s\n", value_string, type, minval_string, maxval_string);
    }
}

static uintmax_t
gid_substitute (int *negative)
{
  intmax_t r;
  int eid = 0;
  static gid_t gid_nobody;
#ifdef GID_NOBODY
  r = GID_NOBODY;
#else
   if (!gid_nobody) {
	if (taru_get_gid_by_name(AHS_USERNAME_NOBODY, &gid_nobody)) {
		eid = -2;
    		gid_nobody = -2;
	}
   }
   r = (gid_t)gid_nobody;
#endif
  *negative = eid < 0;
  return r;
}

void
gid_to_chars (gid_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (v < 0, (uintmax_t) v, sizeof v, gid_substitute, p, s, "gid_t", archive_format, termch);
}

void
major_to_chars (major_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (v < 0, (uintmax_t) v, sizeof v, 0, p, s, "major_t", archive_format, termch);
}

void
minor_to_chars (minor_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (v < 0, (uintmax_t) v, sizeof v, 0, p, s, "minor_t", archive_format, termch);
}


/* JL 2014-06-21
 * FIXME: this should be handled by autoconf and ./configure.
 * /usr/include/tar.h seems to have changed in Slackware 1.14
 * in that #define TSVTX 01000 is now conditionally included.
 * (TSVTX may be a very old name for this mask value).
 * There is probably no harm in defining it here.
 */

#ifndef TSVTX
# define TSVTX  01000
#endif

void
mode_to_chars (mode_t v, char *p, size_t s, int archive_format, int termch)
{
  /* In the common case where the internal and external mode bits are the same,
     and we are not using POSIX or GNU format,
     propagate all unknown bits to the external mode.
     This matches historical practice.
     Otherwise, just copy the bits we know about.  */
  int negative;
  uintmax_t u;
  if (S_ISUID == TSUID && S_ISGID == TSGID && S_ISVTX == TSVTX
      && S_IRUSR == TUREAD && S_IWUSR == TUWRITE && S_IXUSR == TUEXEC
      && S_IRGRP == TGREAD && S_IWGRP == TGWRITE && S_IXGRP == TGEXEC
      && S_IROTH == TOREAD && S_IWOTH == TOWRITE && S_IXOTH == TOEXEC
      && archive_format != POSIX_FORMAT
      && archive_format != GNU_FORMAT)
    {
      negative = v < 0;
      u = v;
    }
  else
    {
      negative = 0;
      u = ((v & S_ISUID ? TSUID : 0)
	   | (v & S_ISGID ? TSGID : 0)
	   | (v & S_ISVTX ? TSVTX : 0)
	   | (v & S_IRUSR ? TUREAD : 0)
	   | (v & S_IWUSR ? TUWRITE : 0)
	   | (v & S_IXUSR ? TUEXEC : 0)
	   | (v & S_IRGRP ? TGREAD : 0)
	   | (v & S_IWGRP ? TGWRITE : 0)
	   | (v & S_IXGRP ? TGEXEC : 0)
	   | (v & S_IROTH ? TOREAD : 0)
	   | (v & S_IWOTH ? TOWRITE : 0)
	   | (v & S_IXOTH ? TOEXEC : 0));
    }
  to_chars (negative, u, sizeof v, 0, p, s, "mode_t", archive_format, termch);
}

void
off_to_chars (off_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (v < 0, (uintmax_t) v, sizeof v, 0, p, s, "off_t", archive_format, termch);
}

void
size_to_chars (size_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (0, (uintmax_t) v, sizeof v, 0, p, s, "size_t", archive_format, termch);
}

void
time_to_chars (time_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (v < 0, (uintmax_t) v, sizeof v, 0, p, s, "time_t", archive_format, termch);
}

static uintmax_t
uid_substitute (int *negative)
{
  intmax_t r;
  int eid = 0;
  static uid_t uid_nobody;
#ifdef UID_NOBODY
  r = UID_NOBODY;
#else
  
   if (!uid_nobody) {
        if (taru_get_uid_by_name(AHS_USERNAME_NOBODY, &uid_nobody)) {
		eid = -2;
    		uid_nobody = -2;
	}
   }
  r = (uid_t)uid_nobody;
#endif
  *negative = eid < 0;
  return r;
}

void
uid_to_chars (uid_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (v < 0, (uintmax_t) v, sizeof v, uid_substitute, p, s, "uid_t", archive_format, termch);
}

void
uintmax_to_chars (uintmax_t v, char *p, size_t s, int archive_format, int termch)
{
  to_chars (0, v, sizeof v, 0, p, s, "uintmax_t", archive_format, termch);
}

