
AC_INIT([swstructlib], [0.1], [bug-swbis@gnu.org])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h:config.h.in])

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
AC_ISC_POSIX
AC_HEADER_STDC
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_C_CONST

topincdir=`(cd .. && pwd)`/include
topdir=`pwd`

echo "checking for --with-debugdump configure option"
if test $usewithdbdump = "yes"; then
AC_DEFINE(HAVE_WITH_DEBUGDUMP, 1, [FIXME])
DO_DEBUG=yes
else
DO_DEBUG=""
fi

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

echo "checking compiler for -Wno-unused-parameter"
gcc -Wno-unused-parameter ../../var/try.c -o /dev/null 2>/dev/null
if test $? -eq 0; then
nounusedparameter=no-unused-parameter
else
nounusedparameter=no-unused
fi
AC_SUBST(nounusedparameter)

if test "$GCC" = yes
then
   CFLAGS="-W -Wall $CFLAGS"
else
   CFLAGS="$CFLAGS"
fi  

AC_SUBST(INCPATH)
AC_SUBST(LIBMISC)
AC_SUBST(LIBINTL)
AC_SUBST(varprefix)
AC_SUBST(tmpdir)
AC_SUBST(topincdir)
AC_SUBST(topdir)
AC_SUBST(topdirswsupplib)
AC_SUBST(DO_DEBUG)
AC_SUBST(INTLDEF)
AC_SUBST(XGETTEXT)
AC_SUBST(LIBINTL)
AC_SUBST(LIBS)
AC_SUBST(PO)
AC_SUBST(GETTEXTSTUB)
AC_SUBST(FIXPERMS)

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)

if test -f /usr/include/sys/sysmacros.h; then 
AC_DEFINE(HAVE_SYSMACROS_H, 1, [FIXME])
fi

if test -f /usr/include/sys/mkdev.h; then 
AC_DEFINE(HAVE_SYSMKDEV_H, 1, [FIXME])
fi

if test "$compile_test_pgms" != "yes"; then 
SWSTRUCT_DIR_="SWSTRUCT_DIR="
AC_SUBST(SWSTRUCT_DIR_)
SWSTRUCTLOADLIB_="SWSTRUCTLOADLIB="
AC_SUBST(SWSTRUCTLOADLIB_)
fi

AC_CONFIG_SUBDIRS(src)
AC_OUTPUT(Makefile Makefile.inc)
