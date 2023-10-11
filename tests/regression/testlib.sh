
check_for_gnu_tar()
{
	gnu_tar_version="`$TAR --version 2>&1 | $GREP GNU | $HEAD -1 | $AWK '{print $NF}'`"
	case "$gnu_tar_version" in 
		2.[0-9].*)
			echo 1>/dev/null
		;;
		1.14|1.14.*|1.15*|1.13.17|1.13.25|1.1[6-9]|1.1[6-9].*|1.[2-9][0-9]|1.[2-9][0-9]*)
			echo 1>/dev/null
		;;
		*)
		echo "GNU tar version 1.13.17,25 or 1.15.x is required" 1>&2
		echo "You have tar version $gnu_tar_version" 1>&2
		return 1
		;;
	esac
	return 0
}

make_format_option()
{
	case "$gnu_tar_version" in 
		1.14|1.14.*|1.15*|1.13.17|1.13.25|1.1[6-9]|1.1[6-9].*|1.[2-9][0-9]|1.[2-9][0-9]*)
			case "$opt_format" in
				gnu)
					GNUTAR_FORMAT_OPTIONS="-b1 --format=gnu"
					SWPACKAGE_FORMAT_OPTIONS="--format=gnu"
					;;
				gnutar|oldgnu)
					echo "$0: oldgnu format not supported for GNU tar $gnu_tar_version" 1>&2 
					return 1
					;;
				ustar)
					GNUTAR_FORMAT_OPTIONS="-b1 --format=ustar"
					SWPACKAGE_FORMAT_OPTIONS="--format=ustar"
					;;
 				ustar0)
 					echo "$0: ustar0 format not supported for GNU tar $gnu_tar_version" 1>&2 
 					return 1
 					;;
			esac
		;;
		1.13.*)
			case "$opt_format" in
				gnu)
					echo "$0: gnu format not supported for GNU tar $gnu_tar_version" 1>&2 
					return 1
					;;
				gnutar|oldgnu)
					GNUTAR_FORMAT_OPTIONS="-b1"
					SWPACKAGE_FORMAT_OPTIONS="--format=oldgnu"
					;;
				ustar|ustar0)
					GNUTAR_FORMAT_OPTIONS="-b1 --posix"
					SWPACKAGE_FORMAT_OPTIONS="--format=ustar0"
					;;
			esac
		;;
		*)
			GNUTAR_FORMAT_OPTIONS=""
			echo "$0: internal error in make_format_options" 1>&2
			return 1
		;;
	esac
	return 0
}

missing_which() {
	pgm="$1"
	xxname=`which $pgm 2>/dev/null`
	test -f "$xxname" -o -h "$xxname"
	case "$?" in
		0)
			echo "$xxname"
			return 0
			;;
		*)
			echo "Missing $pgm" 1>&2
			echo "/"
			return 1
			;;
	esac
}

case "$GTAR" in "") GTAR=tar; ;; esac

MD5SUM=`which md5sum 2>/dev/null`
DIFF=`which diff` || exit 1
TEE=`missing_which tee` || exit 1
TAR=`missing_which $GTAR` || exit 1
GZIP=`missing_which gzip` || exit 1
SED=`missing_which sed` || exit 1
SORT=`missing_which sort` || exit 1
RM=`missing_which rm` || exit 1
CAT=`missing_which cat` || exit 1
GREP=`missing_which grep` || exit 1
FIND=`missing_which find` || exit 1
MKDIR=`missing_which mkdir` || exit 1
AWK=`missing_which awk` || exit 1
ID=`missing_which id` || exit 1
PAX=`missing_which pax 2>/dev/null`
HEAD=`missing_which head` || exit 1
OPATH="$PATH"
PATH="${SWBISLIBEXECDIR}/swbis:$OPATH"
MTIMETOUCH=`missing_which mtimetouch` || exit 1
EPOCHTIME=`missing_which epochtime` || exit 1
GETOPT=`missing_which getopt` || exit 1
PATH="$OPATH"
OPENSSL=`which openssl 2>/dev/null`

