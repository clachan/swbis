dnl Process this file with autoconf to produce a configure script.
dnl

AC_INIT([swsupplib], [0.1], [bug-swbis@gnu.org])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h:config.h.in])

dnl XFLEX=flex
dnl XBISON=bison
XFLEX=_
XBISON=_

usewithrpm=no
usewithselfrpm=yes  
usewithoutrpm=no  
AC_ARG_WITH(rpm, --with-rpm   compile RPM decoding and translating program, usewithrpm=yes, usewithrpm=no)
AC_ARG_WITH(tests, --with-tests  compile test programs, compile_test_pgms=yes, compile_test_pgms=no)
AC_ARG_WITH(self-rpm, --with-self-rpm   compile RPM decoding and translating program, usewithselfrpm=yes, usewithselfrpm=yes)
AC_ARG_WITH(no-rpm, --with-no-rpm   do not compile RPM decoding and translating program, usewithrpm=no; usewithselfrpm=no; usewithoutrpm=yes, usewithoutrpm=no)
AC_ARG_WITH(no-flex,  --with-no-flex  dont use flex, usewithoutflex=yes, usewithoutflex=no)
AC_ARG_WITH(no-bison,  --with-no-bison  dont use bison, usewithoutbison=yes, usewithoutbison=no)

if test $usewithoutrpm = yes; then
	usewithselfrpm=no
fi

dnl AC_CHECK_FUNCS(tcsetattr stty setsid geteuid seteuid  \
dnl   snprintf vprintf vsnprintf vsyslog getopt_long openpty posix_openpt)

AC_DEFUN(jm_MACROS,
[ 
AC_REQUIRE([jm_FUNC_FPENDING])
])

dnl From Jim Meyering
dnl Using code from emacs, based on suggestions from Paul Eggert
dnl and Ulrich Drepper.

dnl Find out how to determine the number of pending output bytes on a stream.
dnl glibc (2.1.93 and newer) and Solaris provide __fpending.  On other systems,
dnl we have to grub around in the FILE struct.

AC_DEFUN(jm_FUNC_FPENDING,
[
  AC_CHECK_HEADERS(stdio_ext.h)
  AC_REPLACE_FUNCS([__fpending])
  fp_headers='
#     if HAVE_STDIO_EXT_H
#      include <stdio_ext.h>
#     endif
'
  AC_CHECK_DECLS([__fpending], , , $fp_headers)
  if test $ac_cv_func___fpending = no; then
    AC_CACHE_CHECK(
	      [how to determine the number of pending output bytes on a stream],
		   ac_cv_sys_pending_output_n_bytes,
      [
        fp_save_DEFS=$DEFS
	for ac_expr in						\
								\
	    '# glibc2'						\
	    'fp->_IO_write_ptr - fp->_IO_write_base'		\
								\
	    '# traditional Unix'				\
	    'fp->_ptr - fp->_base'				\
								\
	    '# BSD'						\
	    'fp->_p - fp->_bf._base'				\
								\
	    '# SCO, Unixware'					\
	    'fp->__ptr - fp->__base'				\
								\
	    '# old glibc?'					\
	    'fp->__bufp - fp->__buffer'				\
								\
	    '# old glibc iostream?'				\
	    'fp->_pptr - fp->_pbase'				\
								\
	    '# VMS'						\
	    '(*fp)->_ptr - (*fp)->_base'			\
								\
	    '# e.g., DGUX R4.11; the info is not available'	\
	    1							\
	    ; do

	  # Skip each embedded comment.
	  case "$ac_expr" in '#'*) continue;; esac

	  DEFS="$DEFS -DPENDING_OUTPUT_N_BYTES=$ac_expr"
	  AC_TRY_COMPILE(
	    [#include <stdio.h>
	    ],
	    [FILE *fp = stdin; (void) ($ac_expr);],
	    fp_done=yes
	  )
	  DEFS=$fp_save_DEFS
	  test "$fp_done" = yes && break
	done

	ac_cv_sys_pending_output_n_bytes=$ac_expr
      ]
    )
    AC_DEFINE_UNQUOTED(PENDING_OUTPUT_N_BYTES,
      $ac_cv_sys_pending_output_n_bytes,
      [the number of pending output bytes on stream `fp'])
  fi
])

# intmax_t.m4 serial 5
dnl Copyright (C) 1997-2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

AC_PREREQ(2.13)

# Define intmax_t to 'long' or 'long long'
# if it is not already defined in <stdint.h> or <inttypes.h>.

AC_DEFUN([gl_AC_TYPE_INTMAX_T],
[
  dnl For simplicity, we assume that a header file defines 'intmax_t' if and
  dnl only if it defines 'uintmax_t'.
  AC_REQUIRE([gl_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([gl_AC_HEADER_STDINT_H])
  if test $gl_cv_header_inttypes_h = no && test $gl_cv_header_stdint_h = no; then
    AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
    test $ac_cv_type_long_long = yes \
      && ac_type='long long' \
      || ac_type='long'
    AC_DEFINE_UNQUOTED(intmax_t, $ac_type,
     [Define to long or long long if <inttypes.h> and <stdint.h> don't define.])
  else
    AC_DEFINE(HAVE_INTMAX_T, 1,
      [Define if you have the 'intmax_t' type in <stdint.h> or <inttypes.h>.])
  fi
])

dnl An alternative would be to explicitly test for 'intmax_t'.

AC_DEFUN([gt_AC_TYPE_INTMAX_T],
[
  AC_REQUIRE([gl_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([gl_AC_HEADER_STDINT_H])
  AC_CACHE_CHECK(for intmax_t, gt_cv_c_intmax_t,
    [AC_TRY_COMPILE([
#include <stddef.h>
#include <stdlib.h>
#if HAVE_STDINT_H_WITH_UINTMAX
#include <stdint.h>
#endif
#if HAVE_INTTYPES_H_WITH_UINTMAX
#include <inttypes.h>
#endif
], [intmax_t x = -1; return !x;], gt_cv_c_intmax_t=yes, gt_cv_c_intmax_t=no)])
  if test $gt_cv_c_intmax_t = yes; then
    AC_DEFINE(HAVE_INTMAX_T, 1,
      [Define if you have the 'intmax_t' type in <stdint.h> or <inttypes.h>.])
  else
    AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
    test $ac_cv_type_long_long = yes \
      && ac_type='long long' \
      || ac_type='long'
    AC_DEFINE_UNQUOTED(intmax_t, $ac_type,
     [Define to long or long long if <stdint.h> and <inttypes.h> don't define.])
  fi
])


# absolute-header.m4 serial 6
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Derek Price.

# gl_ABSOLUTE_HEADER(HEADER1 HEADER2 ...)
# ---------------------------------------
# Find the absolute name of a header file, assuming the header exists.
# If the header were sys/inttypes.h, this macro would define
# ABSOLUTE_SYS_INTTYPES_H to the `""' quoted absolute name of sys/inttypes.h
# in config.h
# (e.g. `#define ABSOLUTE_SYS_INTTYPES_H "///usr/include/sys/inttypes.h"').
# The three "///" are to pacify Sun C 5.8, which otherwise would say
# "warning: #include of /usr/include/... may be non-portable".
# Use `""', not `<>', so that the /// cannot be confused with a C99 comment.
AC_DEFUN([gl_ABSOLUTE_HEADER],
[AC_LANG_PREPROC_REQUIRE()dnl
AC_FOREACH([gl_HEADER_NAME], [$1],
  [AS_VAR_PUSHDEF([gl_absolute_header],
                  [gl_cv_absolute_]m4_quote(m4_defn([gl_HEADER_NAME])))dnl
  AC_CACHE_CHECK([absolute name of <]m4_quote(m4_defn([gl_HEADER_NAME]))[>],
    m4_quote(m4_defn([gl_absolute_header])),
    [AS_VAR_PUSHDEF([ac_header_exists],
                    [ac_cv_header_]m4_quote(m4_defn([gl_HEADER_NAME])))dnl
    AC_CHECK_HEADERS_ONCE(m4_quote(m4_defn([gl_HEADER_NAME])))dnl
    if test AS_VAR_GET(ac_header_exists) = yes; then
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[#include <]]m4_dquote(m4_defn([gl_HEADER_NAME]))[[>]])])
dnl eval is necessary to expand ac_cpp.
dnl Ultrix and Pyramid sh refuse to redirect output of eval, so use subshell.
      AS_VAR_SET(gl_absolute_header,
[`(eval "$ac_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD |
sed -n '\#/]m4_quote(m4_defn([gl_HEADER_NAME]))[#{s#.*"\(.*/]m4_quote(m4_defn([gl_HEADER_NAME]))[\)".*#\1#;s#^/[^/]#//&#;p;q;}'`])
    fi
    AS_VAR_POPDEF([ac_header_exists])dnl
    ])dnl
  AC_DEFINE_UNQUOTED(AS_TR_CPP([ABSOLUTE_]m4_quote(m4_defn([gl_HEADER_NAME]))),
                     ["AS_VAR_GET(gl_absolute_header)"],
                     [Define this to an absolute name of <]m4_quote(m4_defn([gl_HEADER_NAME]))[>.])
  AS_VAR_POPDEF([gl_absolute_header])dnl
])dnl
])# gl_ABSOLUTE_HEADER

# stdint.m4 serial 20
dnl Copyright (C) 2001-2002, 2004-2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert and Bruno Haible.
dnl Test whether <stdint.h> is supported or must be substituted.

AC_DEFUN([gl_STDINT_H],
[
  AC_PREREQ(2.59)dnl

  dnl Check for long long int and unsigned long long int.
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  if test $ac_cv_type_long_long_int = yes; then
    HAVE_LONG_LONG_INT=1
  else
    HAVE_LONG_LONG_INT=0
  fi
  AC_SUBST([HAVE_LONG_LONG_INT])
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
  if test $ac_cv_type_unsigned_long_long_int = yes; then
    HAVE_UNSIGNED_LONG_LONG_INT=1
  else
    HAVE_UNSIGNED_LONG_LONG_INT=0
  fi
  AC_SUBST([HAVE_UNSIGNED_LONG_LONG_INT])

  dnl Check for <wchar.h>.
  AC_CHECK_HEADERS_ONCE([wchar.h])
  if test $ac_cv_header_wchar_h = yes; then
    HAVE_WCHAR_H=1
  else
    HAVE_WCHAR_H=0
  fi
  AC_SUBST([HAVE_WCHAR_H])

  dnl Check for <inttypes.h>.
  dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_inttypes_h.
  if test $ac_cv_header_inttypes_h = yes; then
    HAVE_INTTYPES_H=1
  else
    HAVE_INTTYPES_H=0
  fi
  AC_SUBST([HAVE_INTTYPES_H])

  dnl Check for <sys/types.h>.
  dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_sys_types_h.
  if test $ac_cv_header_sys_types_h = yes; then
    HAVE_SYS_TYPES_H=1
  else
    HAVE_SYS_TYPES_H=0
  fi
  AC_SUBST([HAVE_SYS_TYPES_H])

  dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_stdint_h.
  if test $ac_cv_header_stdint_h = yes; then
    gl_ABSOLUTE_HEADER([stdint.h])
    ABSOLUTE_STDINT_H=\"$gl_cv_absolute_stdint_h\"
    HAVE_STDINT_H=1
  else
    ABSOLUTE_STDINT_H=\"no/such/file/stdint.h\"
    HAVE_STDINT_H=0
  fi
  AC_SUBST([ABSOLUTE_STDINT_H])
  AC_SUBST([HAVE_STDINT_H])

  dnl Now see whether we need a substitute <stdint.h>.  Use
  dnl ABSOLUTE_STDINT_H, not <stdint.h>, so that it also works during
  dnl a "config.status --recheck" if a stdint.h has been
  dnl created in the build directory.
  if test $ac_cv_header_stdint_h = yes; then
    AC_CACHE_CHECK([whether stdint.h conforms to C99],
      [gl_cv_header_working_stdint_h],
      [gl_cv_header_working_stdint_h=no
       AC_COMPILE_IFELSE([
	 AC_LANG_PROGRAM([[
#include <stddef.h>
#define __STDC_LIMIT_MACROS 1 /* to make it work also in C++ mode */
#define __STDC_CONSTANT_MACROS 1 /* to make it work also in C++ mode */
#include ABSOLUTE_STDINT_H
#ifdef INT8_MAX
int8_t a1 = INT8_MAX;
int8_t a1min = INT8_MIN;
#endif
#ifdef INT16_MAX
int16_t a2 = INT16_MAX;
int16_t a2min = INT16_MIN;
#endif
#ifdef INT32_MAX
int32_t a3 = INT32_MAX;
int32_t a3min = INT32_MIN;
#endif
#ifdef INT64_MAX
int64_t a4 = INT64_MAX;
int64_t a4min = INT64_MIN;
#endif
#ifdef UINT8_MAX
uint8_t b1 = UINT8_MAX;
#else
typedef int b1[(unsigned char) -1 != 255 ? 1 : -1];
#endif
#ifdef UINT16_MAX
uint16_t b2 = UINT16_MAX;
#endif
#ifdef UINT32_MAX
uint32_t b3 = UINT32_MAX;
#endif
#ifdef UINT64_MAX
uint64_t b4 = UINT64_MAX;
#endif
int_least8_t c1 = INT8_C (0x7f);
int_least8_t c1max = INT_LEAST8_MAX;
int_least8_t c1min = INT_LEAST8_MIN;
int_least16_t c2 = INT16_C (0x7fff);
int_least16_t c2max = INT_LEAST16_MAX;
int_least16_t c2min = INT_LEAST16_MIN;
int_least32_t c3 = INT32_C (0x7fffffff);
int_least32_t c3max = INT_LEAST32_MAX;
int_least32_t c3min = INT_LEAST32_MIN;
int_least64_t c4 = INT64_C (0x7fffffffffffffff);
int_least64_t c4max = INT_LEAST64_MAX;
int_least64_t c4min = INT_LEAST64_MIN;
uint_least8_t d1 = UINT8_C (0xff);
uint_least8_t d1max = UINT_LEAST8_MAX;
uint_least16_t d2 = UINT16_C (0xffff);
uint_least16_t d2max = UINT_LEAST16_MAX;
uint_least32_t d3 = UINT32_C (0xffffffff);
uint_least32_t d3max = UINT_LEAST32_MAX;
uint_least64_t d4 = UINT64_C (0xffffffffffffffff);
uint_least64_t d4max = UINT_LEAST64_MAX;
int_fast8_t e1 = INT_FAST8_MAX;
int_fast8_t e1min = INT_FAST8_MIN;
int_fast16_t e2 = INT_FAST16_MAX;
int_fast16_t e2min = INT_FAST16_MIN;
int_fast32_t e3 = INT_FAST32_MAX;
int_fast32_t e3min = INT_FAST32_MIN;
int_fast64_t e4 = INT_FAST64_MAX;
int_fast64_t e4min = INT_FAST64_MIN;
uint_fast8_t f1 = UINT_FAST8_MAX;
uint_fast16_t f2 = UINT_FAST16_MAX;
uint_fast32_t f3 = UINT_FAST32_MAX;
uint_fast64_t f4 = UINT_FAST64_MAX;
#ifdef INTPTR_MAX
intptr_t g = INTPTR_MAX;
intptr_t gmin = INTPTR_MIN;
#endif
#ifdef UINTPTR_MAX
uintptr_t h = UINTPTR_MAX;
#endif
intmax_t i = INTMAX_MAX;
uintmax_t j = UINTMAX_MAX;
struct s {
  int check_PTRDIFF: PTRDIFF_MIN < 0 && 0 < PTRDIFF_MAX ? 1 : -1;
  int check_SIG_ATOMIC: SIG_ATOMIC_MIN <= 0 && 0 < SIG_ATOMIC_MAX ? 1 : -1;
  int check_SIZE: 0 < SIZE_MAX ? 1 : -1;
  int check_WCHAR: WCHAR_MIN <= 0 && 0 < WCHAR_MAX ? 1 : -1;
  int check_WINT: WINT_MIN <= 0 && 0 < WINT_MAX ? 1 : -1;

  /* Detect bugs in glibc 2.4 and Solaris 10 stdint.h, among others.  */
  int check_UINT8_C:
	(-1 < UINT8_C (0)) == (-1 < (uint_least8_t) 0) ? 1 : -1;
  int check_UINT16_C:
	(-1 < UINT16_C (0)) == (-1 < (uint_least16_t) 0) ? 1 : -1;

  /* Detect bugs in OpenBSD 3.9 stdint.h.  */
#ifdef UINT8_MAX
  int check_uint8: (uint8_t) -1 == UINT8_MAX ? 1 : -1;
#endif
#ifdef UINT16_MAX
  int check_uint16: (uint16_t) -1 == UINT16_MAX ? 1 : -1;
#endif
#ifdef UINT32_MAX
  int check_uint32: (uint32_t) -1 == UINT32_MAX ? 1 : -1;
#endif
#ifdef UINT64_MAX
  int check_uint64: (uint64_t) -1 == UINT64_MAX ? 1 : -1;
#endif
  int check_uint_least8: (uint_least8_t) -1 == UINT_LEAST8_MAX ? 1 : -1;
  int check_uint_least16: (uint_least16_t) -1 == UINT_LEAST16_MAX ? 1 : -1;
  int check_uint_least32: (uint_least32_t) -1 == UINT_LEAST32_MAX ? 1 : -1;
  int check_uint_least64: (uint_least64_t) -1 == UINT_LEAST64_MAX ? 1 : -1;
  int check_uint_fast8: (uint_fast8_t) -1 == UINT_FAST8_MAX ? 1 : -1;
  int check_uint_fast16: (uint_fast16_t) -1 == UINT_FAST16_MAX ? 1 : -1;
  int check_uint_fast32: (uint_fast32_t) -1 == UINT_FAST32_MAX ? 1 : -1;
  int check_uint_fast64: (uint_fast64_t) -1 == UINT_FAST64_MAX ? 1 : -1;
  int check_uintptr: (uintptr_t) -1 == UINTPTR_MAX ? 1 : -1;
  int check_uintmax: (uintmax_t) -1 == UINTMAX_MAX ? 1 : -1;
  int check_size: (size_t) -1 == SIZE_MAX ? 1 : -1;
};
	 ]])],
         [gl_cv_header_working_stdint_h=yes])])
  fi
  if test "$gl_cv_header_working_stdint_h" != yes; then

    dnl Check for <sys/inttypes.h>, and for
    dnl <sys/bitypes.h> (used in Linux libc4 >= 4.6.7 and libc5).
    AC_CHECK_HEADERS([sys/inttypes.h sys/bitypes.h])
    if test $ac_cv_header_sys_inttypes_h = yes; then
      HAVE_SYS_INTTYPES_H=1
    else
      HAVE_SYS_INTTYPES_H=0
    fi
    AC_SUBST([HAVE_SYS_INTTYPES_H])
    if test $ac_cv_header_sys_bitypes_h = yes; then
      HAVE_SYS_BITYPES_H=1
    else
      HAVE_SYS_BITYPES_H=0
    fi
    AC_SUBST([HAVE_SYS_BITYPES_H])

    gl_STDINT_TYPE_PROPERTIES
    STDINT_H=stdint.h
  fi
  AC_SUBST(STDINT_H)
])

dnl gl_STDINT_BITSIZEOF(TYPES, INCLUDES)
dnl Determine the size of each of the given types in bits.
AC_DEFUN([gl_STDINT_BITSIZEOF],
[
  dnl Use a shell loop, to avoid bloating configure, and
  dnl - extra AH_TEMPLATE calls, so that autoheader knows what to put into
  dnl   config.h.in,
  dnl - extra AC_SUBST calls, so that the right substitutions are made.
  AC_FOREACH([gltype], [$1],
    [AH_TEMPLATE([BITSIZEOF_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]),
       [Define to the number of bits in type ']gltype['.])])
  for gltype in $1 ; do
    AC_CACHE_CHECK([for bit size of $gltype], [gl_cv_bitsizeof_${gltype}],
      [AC_COMPUTE_INT([result], [sizeof ($gltype) * CHAR_BIT],
	 [$2
#include <limits.h>], [result=unknown])
       eval gl_cv_bitsizeof_${gltype}=\$result
      ])
    eval result=\$gl_cv_bitsizeof_${gltype}
    if test $result = unknown; then
      dnl Use a nonempty default, because some compilers, such as IRIX 5 cc,
      dnl do a syntax check even on unused #if conditions and give an error
      dnl on valid C code like this:
      dnl   #if 0
      dnl   # if  > 32
      dnl   # endif
      dnl   #endif
      result=0
    fi
    GLTYPE=`echo "$gltype" | tr 'abcdefghijklmnopqrstuvwxyz ' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'`
    AC_DEFINE_UNQUOTED([BITSIZEOF_${GLTYPE}], [$result])
    eval BITSIZEOF_${GLTYPE}=\$result
  done
  AC_FOREACH([gltype], [$1],
    [AC_SUBST([BITSIZEOF_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]))])
])

dnl gl_CHECK_TYPES_SIGNED(TYPES, INCLUDES)
dnl Determine the signedness of each of the given types.
dnl Define HAVE_SIGNED_TYPE if type is signed.
AC_DEFUN([gl_CHECK_TYPES_SIGNED],
[
  dnl Use a shell loop, to avoid bloating configure, and
  dnl - extra AH_TEMPLATE calls, so that autoheader knows what to put into
  dnl   config.h.in,
  dnl - extra AC_SUBST calls, so that the right substitutions are made.
  AC_FOREACH([gltype], [$1],
    [AH_TEMPLATE([HAVE_SIGNED_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]),
       [Define to 1 if ']gltype[' is a signed integer type.])])
  for gltype in $1 ; do
    AC_CACHE_CHECK([whether $gltype is signed], [gl_cv_type_${gltype}_signed],
      [AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([$2[
            int verify[2 * (($gltype) -1 < ($gltype) 0) - 1];]])],
         result=yes, result=no)
       eval gl_cv_type_${gltype}_signed=\$result
      ])
    eval result=\$gl_cv_type_${gltype}_signed
    GLTYPE=`echo $gltype | tr 'abcdefghijklmnopqrstuvwxyz ' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'`
    if test "$result" = yes; then
      AC_DEFINE_UNQUOTED([HAVE_SIGNED_${GLTYPE}], 1)
      eval HAVE_SIGNED_${GLTYPE}=1
    else
      eval HAVE_SIGNED_${GLTYPE}=0
    fi
  done
  AC_FOREACH([gltype], [$1],
    [AC_SUBST([HAVE_SIGNED_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]))])
])

# inttypes-pri.m4 serial 4 (gettext-0.16)
dnl Copyright (C) 1997-2002, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

AC_PREREQ(2.52)

# Define PRI_MACROS_BROKEN if <inttypes.h> exists and defines the PRI*
# macros to non-string values.  This is the case on AIX 4.3.3.

AC_DEFUN([gt_INTTYPES_PRI],
[
  AC_CHECK_HEADERS([inttypes.h])
  if test $ac_cv_header_inttypes_h = yes; then
    AC_CACHE_CHECK([whether the inttypes.h PRIxNN macros are broken],
      gt_cv_inttypes_pri_broken,
      [
        AC_TRY_COMPILE([#include <inttypes.h>
#ifdef PRId32
char *p = PRId32;
#endif
], [], gt_cv_inttypes_pri_broken=no, gt_cv_inttypes_pri_broken=yes)
      ])
  fi
  if test "$gt_cv_inttypes_pri_broken" = yes; then
    AC_DEFINE_UNQUOTED(PRI_MACROS_BROKEN, 1,
      [Define if <inttypes.h> exists and defines unusable PRI* macros.])
    PRI_MACROS_BROKEN=1
  else
    PRI_MACROS_BROKEN=0
  fi
  AC_SUBST([PRI_MACROS_BROKEN])
])

# inttypes.m4 serial 7
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Derek Price, Bruno Haible.
dnl Test whether <inttypes.h> is supported or must be substituted.

AC_DEFUN([gl_INTTYPES_H],
[
  AC_REQUIRE([gl_STDINT_H])
  AC_REQUIRE([gt_INTTYPES_PRI])
  AC_CHECK_DECLS_ONCE([imaxabs])
  AC_CHECK_DECLS_ONCE([imaxdiv])
  AC_CHECK_DECLS_ONCE([strtoimax])
  AC_CHECK_DECLS_ONCE([strtoumax])

  dnl Now see if we need a substitute <inttypes.h>.
  dnl A complete <inttypes.h> requires
  dnl   - a complete <stdint.h>,
  dnl   - the existence of an <inttypes.h>,
  dnl   - that imaxabs, imaxdiv, strtoimax, strtoumax are declared,
  dnl   - some additional tests.
  AC_CACHE_CHECK([whether inttypes.h conforms to C99],
    [gl_cv_header_working_inttypes_h],
    [gl_cv_header_working_inttypes_h=no
     if test "$gl_cv_header_working_stdint_h" = yes \
	&& test $ac_cv_header_inttypes_h = yes \
	&& test "$ac_cv_have_decl_imaxabs" = yes \
	&& test "$ac_cv_have_decl_imaxdiv" = yes \
	&& test "$ac_cv_have_decl_strtoimax" = yes \
	&& test "$ac_cv_have_decl_strtoumax" = yes; then
       AC_COMPILE_IFELSE([
	 AC_LANG_PROGRAM([
#include <stddef.h>
#define __STDC_LIMIT_MACROS 1 /* to make it work also in C++ mode */
#define __STDC_CONSTANT_MACROS 1 /* to make it work also in C++ mode */
#define __STDC_FORMAT_MACROS 1 /* to make it work also in C++ mode */
#include ABSOLUTE_INTTYPES_H

/* No need to duplicate the tests of stdint.m4; they are subsumed by
   $gl_cv_header_working_stdint_h = yes.  */

/* Tests for macros supposed to be defined in inttypes.h.  */

const char *k = /* implicit string concatenation */
#ifdef INT8_MAX
  PRId8 PRIi8
#endif
#ifdef UINT8_MAX
  PRIo8 PRIu8 PRIx8 PRIX8
#endif
#ifdef INT16_MAX
  PRId16 PRIi16
#endif
#ifdef UINT16_MAX
  PRIo16 PRIu16 PRIx16 PRIX16
#endif
#ifdef INT32_MAX
  PRId32 PRIi32
#endif
#ifdef UINT32_MAX
  PRIo32 PRIu32 PRIx32 PRIX32
#endif
#ifdef INT64_MAX
  PRId64 PRIi64
#endif
#ifdef UINT64_MAX
  PRIo64 PRIu64 PRIx64 PRIX64
#endif
  PRIdLEAST8 PRIiLEAST8 PRIoLEAST8 PRIuLEAST8 PRIxLEAST8 PRIXLEAST8
  PRIdLEAST16 PRIiLEAST16 PRIoLEAST16 PRIuLEAST16 PRIxLEAST16 PRIXLEAST16
  PRIdLEAST32 PRIiLEAST32 PRIoLEAST32 PRIuLEAST32 PRIxLEAST32 PRIXLEAST32
  PRIdLEAST64 PRIiLEAST64
  PRIoLEAST64 PRIuLEAST64 PRIxLEAST64 PRIXLEAST64
  PRIdFAST8 PRIiFAST8 PRIoFAST8 PRIuFAST8 PRIxFAST8 PRIXFAST8
  PRIdFAST16 PRIiFAST16 PRIoFAST16 PRIuFAST16 PRIxFAST16 PRIXFAST16
  PRIdFAST32 PRIiFAST32 PRIoFAST32 PRIuFAST32 PRIxFAST32 PRIXFAST32
  PRIdFAST64 PRIiFAST64
  PRIoFAST64 PRIuFAST64 PRIxFAST64 PRIXFAST64
  PRIdMAX PRIiMAX PRIoMAX PRIuMAX PRIxMAX PRIXMAX
#ifdef INTPTR_MAX
  PRIdPTR PRIiPTR
#endif
#ifdef UINTPTR_MAX
  PRIoPTR PRIuPTR PRIxPTR PRIXPTR
#endif
  ;
const char *l = /* implicit string concatenation */
#ifdef INT8_MAX
  SCNd8 SCNi8
#endif
#ifdef UINT8_MAX
  SCNo8 SCNu8 SCNx8
#endif
#ifdef INT16_MAX
  SCNd16 SCNi16
#endif
#ifdef UINT16_MAX
  SCNo16 SCNu16 SCNx16
#endif
#ifdef INT32_MAX
  SCNd32 SCNi32
#endif
#ifdef UINT32_MAX
  SCNo32 SCNu32 SCNx32
#endif
#ifdef INT64_MAX
  SCNd64 SCNi64
#endif
#ifdef UINT64_MAX
  SCNo64 SCNu64 SCNx64
#endif
  SCNdLEAST8 SCNiLEAST8 SCNoLEAST8 SCNuLEAST8 SCNxLEAST8
  SCNdLEAST16 SCNiLEAST16 SCNoLEAST16 SCNuLEAST16 SCNxLEAST16
  SCNdLEAST32 SCNiLEAST32 SCNoLEAST32 SCNuLEAST32 SCNxLEAST32
  SCNdLEAST64 SCNiLEAST64
  SCNoLEAST64 SCNuLEAST64 SCNxLEAST64
  SCNdFAST8 SCNiFAST8 SCNoFAST8 SCNuFAST8 SCNxFAST8
  SCNdFAST16 SCNiFAST16 SCNoFAST16 SCNuFAST16 SCNxFAST16
  SCNdFAST32 SCNiFAST32 SCNoFAST32 SCNuFAST32 SCNxFAST32
  SCNdFAST64 SCNiFAST64
  SCNoFAST64 SCNuFAST64 SCNxFAST64
  SCNdMAX SCNiMAX SCNoMAX SCNuMAX SCNxMAX
#ifdef INTPTR_MAX
  SCNdPTR SCNiPTR
#endif
#ifdef UINTPTR_MAX
  SCNoPTR SCNuPTR SCNxPTR
#endif
  ;
	 ])],
	 [gl_cv_header_working_inttypes_h=yes])
     fi])
  if test $gl_cv_header_working_inttypes_h = yes; then
    dnl Use the existing <inttypes.h>.
    INTTYPES_H=''
  else

    dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_inttypes_h.
    if test $ac_cv_header_inttypes_h = yes; then
      gl_ABSOLUTE_HEADER([inttypes.h])
      ABSOLUTE_INTTYPES_H=\"$gl_cv_absolute_inttypes_h\"
    else
      ABSOLUTE_INTTYPES_H=\"no/such/file/inttypes.h\"
    fi
    AC_SUBST([ABSOLUTE_INTTYPES_H])

    PRIPTR_PREFIX=
    if test -n "$STDINT_H"; then
      dnl Using the gnulib <stdint.h>. It always defines intptr_t to 'long'.
      PRIPTR_PREFIX='"l"'
    else
      dnl Using the system's <stdint.h>.
      for glpfx in '' l ll I64; do
        case $glpfx in
          '')  gltype1='int';;
          l)   gltype1='long int';;
          ll)  gltype1='long long int';;
          I64) gltype1='__int64';;
        esac
        AC_COMPILE_IFELSE(
          [AC_LANG_PROGRAM([#include <stdint.h>
             extern intptr_t foo;
             extern $gltype1 foo;])],
          [PRIPTR_PREFIX='"'$glpfx'"'])
        test -n "$PRIPTR_PREFIX" && break
      done
    fi
    AC_SUBST([PRIPTR_PREFIX])

    if test "$ac_cv_have_decl_imaxabs" = yes; then
      HAVE_DECL_IMAXABS=1
    else
      HAVE_DECL_IMAXABS=0
    fi
    AC_SUBST([HAVE_DECL_IMAXABS])

    if test "$ac_cv_have_decl_imaxdiv" = yes; then
      HAVE_DECL_IMAXDIV=1
    else
      HAVE_DECL_IMAXDIV=0
    fi
    AC_SUBST([HAVE_DECL_IMAXDIV])

    if test "$ac_cv_have_decl_strtoimax" = yes; then
      HAVE_DECL_STRTOIMAX=1
    else
      HAVE_DECL_STRTOIMAX=0
    fi
    AC_SUBST([HAVE_DECL_STRTOIMAX])

    if test "$ac_cv_have_decl_strtoumax" = yes; then
      HAVE_DECL_STRTOUMAX=1
    else
      HAVE_DECL_STRTOUMAX=0
    fi
    AC_SUBST([HAVE_DECL_STRTOUMAX])

    INTTYPES_H='inttypes.h'
  fi
  AC_SUBST(INTTYPES_H)
])


dnl gl_INTEGER_TYPE_SUFFIX(TYPES, INCLUDES)
dnl Determine the suffix to use for integer constants of the given types.
dnl Define t_SUFFIX for each such type.
AC_DEFUN([gl_INTEGER_TYPE_SUFFIX],
[
  dnl Use a shell loop, to avoid bloating configure, and
  dnl - extra AH_TEMPLATE calls, so that autoheader knows what to put into
  dnl   config.h.in,
  dnl - extra AC_SUBST calls, so that the right substitutions are made.
  AC_FOREACH([gltype], [$1],
    [AH_TEMPLATE(translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_])[_SUFFIX],
       [Define to l, ll, u, ul, ull, etc., as suitable for
	constants of type ']gltype['.])])
  for gltype in $1 ; do
    AC_CACHE_CHECK([for $gltype integer literal suffix],
      [gl_cv_type_${gltype}_suffix],
      [eval gl_cv_type_${gltype}_suffix=no
       eval result=\$gl_cv_type_${gltype}_signed
       if test "$result" = yes; then
	 glsufu=
       else
	 glsufu=u
       fi
       for glsuf in "$glsufu" ${glsufu}l ${glsufu}ll ${glsufu}i64; do
	 case $glsuf in
	   '')  gltype1='int';;
	   l)	gltype1='long int';;
	   ll)	gltype1='long long int';;
	   i64)	gltype1='__int64';;
	   u)	gltype1='unsigned int';;
	   ul)	gltype1='unsigned long int';;
	   ull)	gltype1='unsigned long long int';;
	   ui64)gltype1='unsigned __int64';;
	 esac
	 AC_COMPILE_IFELSE(
	   [AC_LANG_PROGRAM([$2
	      extern $gltype foo;
	      extern $gltype1 foo;])],
	   [eval gl_cv_type_${gltype}_suffix=\$glsuf])
	 eval result=\$gl_cv_type_${gltype}_suffix
	 test "$result" != no && break
       done])
    GLTYPE=`echo $gltype | tr 'abcdefghijklmnopqrstuvwxyz ' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'`
    eval result=\$gl_cv_type_${gltype}_suffix
    test "$result" = no && result=
    eval ${GLTYPE}_SUFFIX=\$result
    AC_DEFINE_UNQUOTED([${GLTYPE}_SUFFIX], $result)
  done
  AC_FOREACH([gltype], [$1],
    [AC_SUBST(translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_])[_SUFFIX])])
])

dnl gl_STDINT_INCLUDES
AC_DEFUN([gl_STDINT_INCLUDES],
[[
  #include <stddef.h>
  #include <signal.h>
  #if HAVE_WCHAR_H
    /* BSD/OS 4.1 has a bug: <stdio.h> and <time.h> must be included before
       <wchar.h>.  */
  # include <stdio.h>
  # include <time.h>
  # include <wchar.h>
  #endif
]])

dnl gl_STDINT_TYPE_PROPERTIES
dnl Compute HAVE_SIGNED_t, BITSIZEOF_t and t_SUFFIX, for all the types t
dnl of interest to stdint_.h.
AC_DEFUN([gl_STDINT_TYPE_PROPERTIES],
[
  gl_STDINT_BITSIZEOF([ptrdiff_t sig_atomic_t size_t wchar_t wint_t],
    [gl_STDINT_INCLUDES])
  gl_CHECK_TYPES_SIGNED([sig_atomic_t wchar_t wint_t],
    [gl_STDINT_INCLUDES])
  gl_cv_type_ptrdiff_t_signed=yes
  gl_cv_type_size_t_signed=no
  gl_INTEGER_TYPE_SUFFIX([ptrdiff_t sig_atomic_t size_t wchar_t wint_t],
    [gl_STDINT_INCLUDES])
])

dnl Autoconf >= 2.61 has AC_COMPUTE_INT built-in.
dnl Remove this when we can assume autoconf >= 2.61.
m4_ifdef([AC_COMPUTE_INT], [], [
  AC_DEFUN([AC_COMPUTE_INT], [_AC_COMPUTE_INT([$2],[$1],[$3],[$4])])
])

# longlong.m4 serial 10
dnl Copyright (C) 1999-2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

# Define HAVE_LONG_LONG_INT if 'long long int' works.
# This fixes a bug in Autoconf 2.60, but can be removed once we
# assume 2.61 everywhere.

# Note: If the type 'long long int' exists but is only 32 bits large
# (as on some very old compilers), HAVE_LONG_LONG_INT will not be
# defined. In this case you can treat 'long long int' like 'long int'.

AC_DEFUN([AC_TYPE_LONG_LONG_INT],
[
  AC_CACHE_CHECK([for long long int], [ac_cv_type_long_long_int],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
	  [[long long int ll = 9223372036854775807ll;
	    long long int nll = -9223372036854775807LL;
	    typedef int a[((-9223372036854775807LL < 0
			    && 0 < 9223372036854775807ll)
			   ? 1 : -1)];
	    int i = 63;]],
	  [[long long int llmax = 9223372036854775807ll;
	    return ((ll << 63) | (ll >> 63) | (ll < i) | (ll > i)
		    | (llmax / ll) | (llmax % ll));]])],
       [dnl This catches a bug in Tandem NonStop Kernel (OSS) cc -O circa 2004.
	dnl If cross compiling, assume the bug isn't important, since
	dnl nobody cross compiles for this platform as far as we know.
        AC_RUN_IFELSE(
	  [AC_LANG_PROGRAM(
	     [[@%:@include <limits.h>
	       @%:@ifndef LLONG_MAX
	       @%:@ define HALF \
			(1LL << (sizeof (long long int) * CHAR_BIT - 2))
	       @%:@ define LLONG_MAX (HALF - 1 + HALF)
	       @%:@endif]],
	     [[long long int n = 1;
	       int i;
	       for (i = 0; ; i++)
		 {
		   long long int m = n << i;
		   if (m >> i != n)
		     return 1;
		   if (LLONG_MAX / 2 < m)
		     break;
		 }
	       return 0;]])],
	  [ac_cv_type_long_long_int=yes],
	  [ac_cv_type_long_long_int=no],
	  [ac_cv_type_long_long_int=yes])],
       [ac_cv_type_long_long_int=no])])
  if test $ac_cv_type_long_long_int = yes; then
    AC_DEFINE([HAVE_LONG_LONG_INT], 1,
      [Define to 1 if the system has the type `long long int'.])
  fi
])

# This macro is obsolescent and should go away soon.
AC_DEFUN([gl_AC_TYPE_LONG_LONG],
[
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  ac_cv_type_long_long=$ac_cv_type_long_long_int
  if test $ac_cv_type_long_long = yes; then
    AC_DEFINE(HAVE_LONG_LONG, 1,
      [Define if you have the 'long long' type.])
  fi
])

# stdint_h.m4 serial 6
dnl Copyright (C) 1997-2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

# Define HAVE_STDINT_H_WITH_UINTMAX if <stdint.h> exists,
# doesn't clash with <sys/types.h>, and declares uintmax_t.

AC_DEFUN([gl_AC_HEADER_STDINT_H],
[
  AC_CACHE_CHECK([for stdint.h], gl_cv_header_stdint_h,
  [AC_TRY_COMPILE(
    [#include <sys/types.h>
#include <stdint.h>],
    [uintmax_t i = (uintmax_t) -1; return !i;],
    gl_cv_header_stdint_h=yes,
    gl_cv_header_stdint_h=no)])
  if test $gl_cv_header_stdint_h = yes; then
    AC_DEFINE_UNQUOTED(HAVE_STDINT_H_WITH_UINTMAX, 1,
      [Define if <stdint.h> exists, doesn't clash with <sys/types.h>,
       and declares uintmax_t. ])
  fi
])

# inttypes_h.m4 serial 7
dnl Copyright (C) 1997-2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

# Define HAVE_INTTYPES_H_WITH_UINTMAX if <inttypes.h> exists,
# doesn't clash with <sys/types.h>, and declares uintmax_t.

AC_DEFUN([gl_AC_HEADER_INTTYPES_H],
[
  AC_CACHE_CHECK([for inttypes.h], gl_cv_header_inttypes_h,
  [AC_TRY_COMPILE(
    [#include <sys/types.h>
#include <inttypes.h>],
    [uintmax_t i = (uintmax_t) -1; return !i;],
    gl_cv_header_inttypes_h=yes,
    gl_cv_header_inttypes_h=no)])
  if test $gl_cv_header_inttypes_h = yes; then
    AC_DEFINE_UNQUOTED(HAVE_INTTYPES_H_WITH_UINTMAX, 1,
      [Define if <inttypes.h> exists, doesn't clash with <sys/types.h>,
       and declares uintmax_t. ])
  fi
])

# strtoull.m4 serial 4
dnl Copyright (C) 2002, 2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOULL],
[
  dnl We don't need (and can't compile) the replacement strtoull
  dnl unless the type 'unsigned long long int' exists.
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
  if test "$ac_cv_type_unsigned_long_long_int" = yes; then
    AC_REPLACE_FUNCS(strtoull)
    if test $ac_cv_func_strtoull = no; then
      gl_PREREQ_STRTOULL
    fi
  fi
])

# Prerequisites of lib/strtoull.c.
AC_DEFUN([gl_PREREQ_STRTOULL], [
  :
])

# strtoul.m4 serial 3
dnl Copyright (C) 2002, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOUL],
[
  AC_REPLACE_FUNCS(strtoul)
])


# strtoimax.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOIMAX],
[
  dnl Work around a bug of AC_EGREP_CPP in autoconf-2.57.
  AC_REQUIRE([AC_PROG_CPP])
  AC_REQUIRE([AC_PROG_EGREP])

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoimax as a macro],
    jm_cv_func_strtoimax_macro,
    [AC_EGREP_CPP([inttypes_h_defines_strtoimax], [#include <inttypes.h>
#ifdef strtoimax
 inttypes_h_defines_strtoimax
#endif],
       jm_cv_func_strtoimax_macro=yes,
       jm_cv_func_strtoimax_macro=no)])

  if test "$jm_cv_func_strtoimax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoimax)
    if test $ac_cv_func_strtoimax = no; then
      gl_PREREQ_STRTOIMAX
    fi
  fi
])

# Prerequisites of lib/strtoimax.c.
AC_DEFUN([gl_PREREQ_STRTOIMAX], [
  AC_CHECK_DECLS(strtoll)
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
])



# strtol.m4 serial 4
dnl Copyright (C) 2002, 2003, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOL],
[
  AC_REPLACE_FUNCS(strtol)
])

# strtoll.m4 serial 4
dnl Copyright (C) 2002, 2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOLL],
[
  dnl We don't need (and can't compile) the replacement strtoll
  dnl unless the type 'long long int' exists.
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  if test "$ac_cv_type_long_long_int" = yes; then
    AC_REPLACE_FUNCS(strtoll)
    if test $ac_cv_func_strtoll = no; then
      gl_PREREQ_STRTOLL
    fi
  fi
])

# Prerequisites of lib/strtoll.c.
AC_DEFUN([gl_PREREQ_STRTOLL], [
  :
])


# strtoul.m4 serial 3
dnl Copyright (C) 2002, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOUL],
[
  AC_REPLACE_FUNCS(strtoul)
])

# strtoumax.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOUMAX],
[
  dnl Work around a bug of AC_EGREP_CPP in autoconf-2.57.
  AC_REQUIRE([AC_PROG_CPP])
  AC_REQUIRE([AC_PROG_EGREP])

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoumax as a macro],
    jm_cv_func_strtoumax_macro,
    [AC_EGREP_CPP([inttypes_h_defines_strtoumax], [#include <inttypes.h>
#ifdef strtoumax
 inttypes_h_defines_strtoumax
#endif],
       jm_cv_func_strtoumax_macro=yes,
       jm_cv_func_strtoumax_macro=no)])

  if test "$jm_cv_func_strtoumax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoumax)
    if test $ac_cv_func_strtoumax = no; then
      gl_PREREQ_STRTOUMAX
    fi
  fi
])

# Prerequisites of lib/strtoumax.c.
AC_DEFUN([gl_PREREQ_STRTOUMAX], [
  AC_CHECK_DECLS(strtoull)
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
])

dnl +++++++++++++++++++++++++++++++++++++++++++++

# System features
AC_SYS_LARGEFILE

AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_CPP			dnl Later checks need this.
AC_PROG_CC_C_O
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_AIX
AC_ISC_POSIX
AC_MINIX
AC_HEADER_STDC
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
  
AC_CHECK_DECLS(strtoull)

if [ "$ac_cv_prog_cc_c99" = "" ]; then
	ac_cv_prog_cc_c99=no
fi

gl_STDINT_H
gl_AC_TYPE_INTMAX_T
gl_INTTYPES_H
gl_AC_HEADER_INTTYPES_H
gl_AC_HEADER_STDINT_H
gl_FUNC_STRTOL
gl_FUNC_STRTOLL
gl_FUNC_STRTOUL
gl_FUNC_STRTOULL
gl_FUNC_STRTOIMAX
gl_FUNC_STRTOUMAX
gl_PREREQ_STRTOIMAX

case "$usewithoutflex" in
	"yes")
		XFLEX=_
		;;
	*)
		XFLEX=flex
		;;
esac

case "$usewithoutbison" in
	"yes")
		XBISON=_
		;;
	*)
		XBISON=bison
		;;
esac

# Checks for system/compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST

ac_rpm_version=unknown
dnl RedHat 9.0
case "$ac_rpm_version" in
	unknown)
dnl Lie about the RPM version
	AC_EGREP_HEADER(RPMTAG_SOURCEPKGID.*1146, /usr/include/rpm/rpmlib.h, ac_rpm_version=4.1, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 8.0
case "$ac_rpm_version" in
	unknown)
	AC_EGREP_HEADER(RPMTAG_CACHEPKGMTIME.*1139, /usr/include/rpm/rpmlib.h, ac_rpm_version=4.1, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 7.3
case "$ac_rpm_version" in
	unknown)
	AC_EGREP_HEADER(RPMTAG_PLATFORM.*1132, /usr/include/rpm/rpmlib.h, ac_rpm_version=4.0.4, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 7.2
case "$ac_rpm_version" in
	unknown)
	AC_EGREP_HEADER(RPMTAG_SHA1RHN.*1130, /usr/include/rpm/rpmlib.h, ac_rpm_version=4.0.3, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 7.1
case "$ac_rpm_version" in
	unknown)
	AC_EGREP_HEADER(RPMTAG_REMOVETID.*1129, /usr/include/rpm/rpmlib.h, ac_rpm_version=4.0.2, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 7.0
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_CPP(RPMTAG_FIRSTFREE_TAG.*1128, /usr/include/rpm/rpmlib.h, ac_rpm_version=4.0, ac_rpm_version=unknown)
	egrep 'RPMTAG_FIRSTFREE_TAG.*1128' /usr/include/rpm/rpmlib.h 1>/dev/null
	if test $? -eq 0; then
		ac_rpm_version=4.0
	else
		ac_rpm_version=unknown
	fi
	;;
esac

case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_CPP(RPMTAG_FIRSTFREE_TAG.*1127, /usr/include/rpm/rpmlib.h, ac_rpm_version=3.0.5, ac_rpm_version=unknown)
	egrep 'RPMTAG_FIRSTFREE_TAG.*1127' /usr/include/rpm/rpmlib.h 1>/dev/null
	if test $? -eq 0; then
		ac_rpm_version=3.0.5
	else
		ac_rpm_version=unknown
	fi
	;;
esac

dnl RedHat 6.2
case "$ac_rpm_version" in
	unknown)
	AC_EGREP_CPP(RPMTAG_FIRSTFREE_TAG.*1122, /usr/include/rpm/rpmlib.h, ac_rpm_version=3.0.4, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 6.1
case "$ac_rpm_version" in
	unknown)
	AC_EGREP_CPP(RPMTAG_FIRSTFREE_TAG.*1116, /usr/include/rpm/rpmlib.h, ac_rpm_version=3.0.3, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 6.0
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_BUILDPREREQ.*1108, /usr/include/rpm/rpmlib.h, ac_rpm_version=3.0, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_BUILDPREREQ
	yes
	#endif
	], ac_rpm_version=3.0, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 5.2
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_CAPABILITY.*1105, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.5.5, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_CAPABILITY
	yes
	#endif
	], ac_rpm_version=2.5.5, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 5.1
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_AUTOPROV.*1104, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.5.1, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_AUTOPROV
	yes
	#endif
	], ac_rpm_version=2.5.1, ac_rpm_version=unknown)
	;;
esac

case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_FILELANGS.*1097, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.4.12, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_FILELANGS
	yes
	#endif
	], ac_rpm_version=2.4.12, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 5.0
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_COOKIE.*1094, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.4.10, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_COOKIE
	yes
	#endif
	], ac_rpm_version=2.4.10, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 4.2
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_BROKENMD5.*1083, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.3.11, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_BROKENMD5
	yes
	#endif
	], ac_rpm_version=2.3.11, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 4.1
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_TRIGGERINDEX.*1069, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.3.1, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_TRIGGERINDEX
	yes
	#endif
	], ac_rpm_version=2.3.1, ac_rpm_version=unknown)
	;;
esac

dnl RedHat 4.0
case "$ac_rpm_version" in
	unknown)
dnl	AC_EGREP_HEADER(RPMTAG_RPMVERSION.*1064, /usr/include/rpm/rpmlib.h, ac_rpm_version=2.2.7, ac_rpm_version=unknown)
	AC_EGREP_CPP(yes,
	[#include <rpm/rpmlib.h>
	#ifdef RPMTAG_RPMVERSION
	yes
	#endif
	], ac_rpm_version=2.2.7, ac_rpm_version=unknown)
	;;
esac

case "$ac_rpm_version" in
	unknown)
	echo "RPM Version not determined, defaulting to 3.0"
	ac_rpm_version=3.0
	;;
	*)
	echo "RPM Version determined to be $ac_rpm_version"
	;;
esac

case "$usewithselfrpm" in
	"yes")
		echo "Setting RPM version to the self contained version which is 3.0.5"
		ac_rpm_version=3.0.5
		;;
esac

AC_EGREP_HEADER(RPMTAG_PAYLOADCOMPRESSOR.*1125, /usr/include/rpm/rpmlib.h, ac_rpm_payloadcompressor=yes, ac_rpm_payloadcompressor=no)
AC_EGREP_HEADER(RPMTAG_FILELANGS.*1097, /usr/include/rpm/rpmlib.h, ac_rpm_filelangs=yes, ac_rpm_filelangs=no)

AC_EGREP_HEADER(RPMTAG_BASENAMES.*1117, rpm/rpmlib.h, ac_rpm_basenames=yes, ac_rpm_basenames=no)
case "$ac_rpm_basenames" in
	no)
AC_EGREP_CPP(yes,
[#include <rpm/rpmlib.h>
#ifdef RPMTAG_BASENAMES
yes
#endif
], ac_rpm_basenames=yes, ac_rpm_basenames=no)
	;;
esac

AC_EGREP_CPP(yes,
[#include <rpm/rpmio.h>
#ifdef fdNew
yes
#endif
], ac_rpm_new_fdnew=yes, ac_rpm_new_fdnew=no)

case "$usewithselfrpm" in
	"yes")
		ac_rpm_basenames=yes
		ac_rpm_new_fdnew=yes
		ac_rpm_payloadcompressor=yes
		ac_rpm_filelangs=yes
		;;
esac

case "$ac_rpm_basenames" in
	yes) AC_DEFINE(USE_RPMTAG_BASENAMES, 1, [FIXME]) ;;
esac
echo "fdNew evaluation $ac_rpm_new_fdnew"
echo "ac_rpm_version is [$ac_rpm_version]"

case "$ac_rpm_new_fdnew" in
	yes)   case "$ac_rpm_version" in
			4.2)
				echo "Setting USE_NEW_FDNEW72"
				AC_DEFINE(USE_NEW_FDNEW72, 1, [FIXME]) 
				;;
			4.1.?)
				echo "Setting USE_NEW_FDNEW72"
				AC_DEFINE(USE_NEW_FDNEW72, 1, [FIXME]) 
				;;
			4.1)
				echo "Setting USE_NEW_FDNEW72"
				AC_DEFINE(USE_NEW_FDNEW72, 1, [FIXME]) 
				;;
			4.0.4)
				echo "Setting USE_NEW_FDNEW72"
				AC_DEFINE(USE_NEW_FDNEW72, 1, [FIXME]) 
				;;
			4.0.3)
				echo "Setting USE_NEW_FDNEW72"
				AC_DEFINE(USE_NEW_FDNEW72, 1, [FIXME]) 
				;;
			*)
				echo "Setting USE_NEW_FDNEW62"
				AC_DEFINE(USE_NEW_FDNEW62, 1, [FIXME]) 
				;;
		esac
		;;
esac

case "$SWBISNEEDDEBUG" in
	0) /bin/true
		;;
	"") /bin/true
		;;
	*)  AC_DEFINE(SWBISNEEDDEBUG, 1, [FIXME]) 
		;;
esac

case "$SWBISNEEDFAIL" in
	0) /bin/true
		;;
	"") AC_DEFINE(SWBISNEEDFAIL, 1, [FIXME]) 
		;;
	*) AC_DEFINE(SWBISNEEDFAIL, 1, [FIXME]) 
		;;
esac

case "$usewithrpm" in
	"yes")
		AC_DEFINE(USE_WITH_RPM, 1, [FIXME])
		;;
esac

case "$usewithselfrpm" in
	"yes")
		AC_DEFINE(USE_WITH_SELF_RPM, 1, [FIXME])
		;;
esac

case "$ac_rpm_filelangs" in
	"yes")
		AC_DEFINE(RPMPSF_RPM_HAS_FILELANGS, 1, [FIXME])
		;;
esac

case "$ac_rpm_payloadcompressor" in
	"yes")
		AC_DEFINE(RPMPSF_RPM_HAS_PAYLOADCOMPRESSOR, 1, [FIXME])
		;;
esac

case "$ac_rpm_version" in
	4.2) AC_DEFINE(RPMPSF_RPM_VERSION_42, 1, [FIXME])
		;;	
	4.1) AC_DEFINE(RPMPSF_RPM_VERSION_41, 1, [FIXME])
		;;	
	4.0.4) AC_DEFINE(RPMPSF_RPM_VERSION_404, 1, [FIXME]) 
		;;	
	4.0.3) AC_DEFINE(RPMPSF_RPM_VERSION_403, 1, [FIXME]) 
		;;
	4.0.2) AC_DEFINE(RPMPSF_RPM_VERSION_402, 1, [FIXME]) 
		;;
	4.0) AC_DEFINE(RPMPSF_RPM_VERSION_40, 1, [FIXME]) 
		;;
	3.0.5) AC_DEFINE(RPMPSF_RPM_VERSION_305, 1, [FIXME]) 
		;;
	3.0.4) AC_DEFINE(RPMPSF_RPM_VERSION_304, 1, [FIXME]) 
		;;
	3.0.3) AC_DEFINE(RPMPSF_RPM_VERSION_303, 1, [FIXME]) 
		;;
	3.0) AC_DEFINE(RPMPSF_RPM_VERSION_30, 1, [FIXME]) 
		;;
	2.5.5) AC_DEFINE(RPMPSF_RPM_VERSION_255, 1, [FIXME]) 
		;;
	2.5.1) AC_DEFINE(RPMPSF_RPM_VERSION_251, 1, [FIXME]) 
		;;
	2.4.12) AC_DEFINE(RPMPSF_RPM_VERSION_2412, 1, [FIXME]) 
		;;
	2.4.10) AC_DEFINE(RPMPSF_RPM_VERSION_2410, 1, [FIXME]) 
		;;
	2.3.11) AC_DEFINE(RPMPSF_RPM_VERSION_2311, 1, [FIXME]) 
		;;
	2.3.1) AC_DEFINE(RPMPSF_RPM_VERSION_231, 1, [FIXME]) 
		;;
	2.2.7) AC_DEFINE(RPMPSF_RPM_VERSION_227, 1, [FIXME]) 
		;;
	*) AC_DEFINE(RPMPSF_RPM_VERSION_30, 1, [FIXME]) 
		;;
esac

echo "checking compiler for -Wno-unused-parameter"
gcc -Wno-unused-parameter ../var/try.c -o /dev/null 2>/dev/null
if test $? -eq 0; then
nounusedparameter=no-unused-parameter
else
nounusedparameter=no-unused
fi
AC_SUBST(nounusedparameter)

topincdir=`(cd .. && pwd)`/include
topdir=`pwd`
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(bz2, main)
dnl AC_CHECK_LIB(popt, poptParseArgvString)
dnl AC_CHECK_LIB(rpmio, fdOpen)
AC_CHECK_LIB(db, main)
AC_CHECK_LIB(db1, main)
AC_CHECK_LIB(z, main)
AC_CHECK_HEADERS(limits.h locale.h)

AC_CHECK_HEADERS( \
        libutil.h \
        getopt.h \
        sys/ioctl.h \
        sgtty.h \
        stdarg.h \
        termio.h \
        termios.h \
        sys/types.h \
        sys/sysmacros.h \
        unistd.h \
        )

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)

if test -f /usr/include/inttypes.h; then 
AC_DEFINE(HAVE_INTTYPES_H, 1, [FIXME])
fi

if test -f /usr/include/sys/sysmacros.h; then 
AC_DEFINE(HAVE_SYSMACROS_H, 1, [FIXME])
fi

if test -f /usr/include/sys/mkdev.h; then 
AC_DEFINE(HAVE_SYSMKDEV_H, 1, [FIXME])
fi

if test -f /usr/include/sys/select.h; then 
AC_DEFINE(HAVE_SYSSELECT_H, 1, [FIXME])
fi

RPMINCPATH=/tmp/no
case "$usewithselfrpm" in
	"yes")
		RPMINCPATH=um_rpm/rpm-3.0.5/lib
		;;
esac

RPMINCOPT=
case "$usewithrpm" in
	"yes")
RPMINCOPT="-I/usr/include/rpm"
RPMINCPATH=/usr/include/rpm
if test -f $RPMINCPATH/header.h; then 
AC_DEFINE(HAVE_RPM_HEADER_H, 1, [FIXME])
fi
if test -f $RPMINCPATH/rpmdb.h; then
AC_DEFINE(HAVE_RPM_RPMDB_H, 1, [FIXME])
fi
if test -f $RPMINCPATH/rpmlib.h; then
AC_DEFINE(HAVE_RPM_RPMLIB_H, 1, [FIXME])
fi
if test -f $RPMINCPATH/rpmio.h; then
AC_DEFINE(HAVE_RPM_RPMIO_H, 1, [FIXME])
fi
	;;
esac

case "$usewithselfrpm" in
	"yes")
AC_DEFINE(HAVE_RPM_HEADER_H, 1, [FIXME])
AC_DEFINE(HAVE_RPM_RPMLIB_H, 1, [FIXME])
AC_DEFINE(HAVE_RPM_RPMIO_H, 1, [FIXME])
	;;
esac

echo "checking for --with-debugdump configure option"
if test $usewithdbdump = "yes"; then
AC_DEFINE(HAVE_WITH_DEBUGDUMP, 1, [FIXME])
DO_DEBUG=yes
else
DO_DEBUG=""
fi

RPMLIBS=""
RPMLIBPATH=/usr/lib
RPMLIBPATH1=/lib
LIBPATHLIST="/lib /usr/lib /usr/local/lib /opt/lib"

case "$usewithselfrpm" in
	"yes")
		RPMLIBPATHLIST=""
		;;
esac

case "$usewithrpm" in
	"yes")
		RPMLIBPATHLIST="/lib /usr/lib /usr/local/lib /opt/lib"
		;;
esac

for RPMLIBPATH in $RPMLIBPATHLIST; do
if test -f $RPMLIBPATH/libpopt.a -a "$usewithrpm" = "yes"; then 
RPMLIBS="$RPMLIBS -lpopt"
break;
fi
done

for RPMLIBPATH in $RPMLIBPATHLIST; do
if test -f $RPMLIBPATH/librpm.a -a "$usewithrpm" = "yes"; then 
RPMLIBS="$RPMLIBS -lrpm"
break;
fi
done

for RPMLIBPATH in $RPMLIBPATHLIST; do
if test -f $RPMLIBPATH/librpmio.a -a "$usewithrpm" = "yes"; then 
RPMLIBS="$RPMLIBS -lrpmio"
break;
fi
done

for RPMLIBPATH in $RPMLIBPATHLIST; do
if test -f $RPMLIBPATH/librpmdb.so -a "$usewithrpm" = "yes"; then 
RPMLIBS="$RPMLIBS -lrpmdb"
break;
fi
done

for xLIBPATH in $LIBPATHLIST; do
if test -f $xLIBPATH/librt.so -o -h $xLIBPATH/librt.so; then 
LIBS="$LIBS -lrt"
break;
fi
done

for xLIBPATH in $LIBPATHLIST; do
if test -f $xLIBPATH/libutil.so -o -h $xLIBPATH/libutil.so; then 
LIBS="$LIBS -lutil"
break;
fi
done

case "$usewithselfrpm" in
	"yes")
		RPMLIBS=../um_rpm/um_librpm.a
		;;
esac

for xLIBPATH in $LIBPATHLIST; do
if test -f $xLIBPATH/libnsl.so -o -h $xLIBPATH/libnsl.so; then 
RPMLIBS="$RPMLIBS -lnsl"
break;
fi
done

for xLIBPATH in $LIBPATHLIST; do
if test -f $xLIBPATH/libsocket.so -o -h $xLIBPATH/libsocket.so; then 
RPMLIBS="$RPMLIBS -lsocket"
break;
fi
done

RPMPROGS=""
if test "$usewithoutrpm" = "yes"; then 
RPMLIBOBJECTS_="RPMLIBOBJECTS="
AC_SUBST(RPMLIBOBJECTS_)
fi

if test "$compile_test_pgms" != "yes"; then 
TESTPROGS_="TESTPROGS="
AC_SUBST(TESTPROGS_)
if test "$usewithoutrpm" = "yes"; then 
TESTRPMPROGS_="TESTRPMPROGS="
AC_SUBST(TESTRPMPROGS_)
fi
fi

if test "$usewithoutrpm" != "yes"; then 
case "$usewithrpm" in
	"yes")
		RPMPROGS="rpmheaderdump rpm2swpsf list2tar rpmarchivedump lxpsf"
		;;
esac

case "$usewithselfrpm" in
	"yes")
		RPMPROGS="rpmheaderdump rpm2swpsf list2tar rpmarchivedump lxpsf"
		;;
esac
else
RPMPROGS_="RPMPROGS="
AC_SUBST(RPMPROGS_)
fi

case "$usewithselfrpm" in
	"yes")
		DO_UM_RPM="um_rpm"
		;;	
	*)
		DO_UM_RPM=""
		;;
esac

if test "$GCC" = yes
then
   CFLAGS="-W -Wall -Wstrict-prototypes -Wno-unused $CFLAGS"
else
   CFLAGS="$CFLAGS"
fi

AC_CHECK_FUNCS(tcsetattr stty setsid geteuid seteuid  \
  snprintf vprintf vsnprintf vsyslog getopt_long openpty posix_openpt)

AC_CHECK_TYPE(major_t, int)
AC_CHECK_TYPE(minor_t, int)
AC_CHECK_TYPE(minor_t, int)

jm_MACROS

AC_SUBST(INCPATH)
AC_SUBST(LIBMISC)
AC_SUBST(LIBINTL)
AC_SUBST(varprefix)
AC_SUBST(tmpdir)
AC_SUBST(topincdir)
AC_SUBST(topdir)
AC_SUBST(INTLDEF)
AC_SUBST(XGETTEXT)
AC_SUBST(LIBINTL)
AC_SUBST(RPMLIBS)
AC_SUBST(RPMINCOPT)
AC_SUBST(DO_DEBUG)
AC_SUBST(DO_UM_RPM)
AC_SUBST(RPMPROGS)
AC_SUBST(LIBS)
AC_SUBST(PO)
AC_SUBST(GETTEXTSTUB)
AC_SUBST(FIXPERMS)
AC_SUBST(nounusedparameters)
SWBISDEFS="-DSWBISLIBEXECDIR=\\\"${libexecdir}\\\" -DSWBISLIBDIR=\\\"${libdir}\\\""
AC_SUBST(SWBISDEFS)
AC_SUBST(XFLEX)
AC_SUBST(XBISON)

case "$usewithselfrpm" in
	yes)
AC_CONFIG_SUBDIRS(um_rpm/rpm-3.0.5)
		;;
	*)
		;;
esac

AC_OUTPUT(Makefile Makefile.inc \
		taru/Makefile \
		uxfio/Makefile \
		misc/Makefile \
		strob/Makefile \
		uinfile/Makefile \
		sw_parse/Makefile \
		cplob/Makefile \
		progs/Makefile \
		getopt/Makefile \
		progressmeter/Makefile \
		ls_list/Makefile \
		um_rpm/Makefile \
		compression/Makefile \
		shell_lib/Makefile \
		swi/Makefile \
		rpmbis/Makefile \
		tests/Makefile)


if test "$usewithselfrpm" = "yes"; then 
echo "You are using the self contained rpmlib and header files."
fi

if test "$usewithrpm" = "yes"; then 
echo "You are using the system rpmlib and header files."
fi

if test "$usewithrpm" = "yes" -o "$usewithselfrpm" = "yes"; then 
echo "You are compiling rpm dependent utilities."
else 
echo "You are NOT compiling rpm dependent utilities."
fi

if test "$compile_test_pgms" = "yes"; then 
echo "You are compiling the test programs."
else 
echo "You are NOT compiling the test programs."
fi
