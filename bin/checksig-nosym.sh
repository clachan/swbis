#
# NOTE: you probably want to use ./checkdigest, this file is no
# longer maintained and the name 'checksig' is deprectated in
# favor of 'checkdigest'
#
# Script:  checksig-nosysm
# Usage:  checksig-nosysm [gpg_homedir]
#
# This file may be copied without restriction.
#
# ===============================================================
# Package Requirements for this script to work:
# ===============================================================
#	0) The target package unpacked with a tar reading utility
#	   that preserves modification times such as GNU tar 1.13.17,
#          1.13.25, pax v3.0, etc.
#	1) Probably, A leading package path other than "."
#       2) Package made with the apparent same tar creating utility as
#          is now being used to verify the package.
#       3) A package file list named "files" found in:
#		<path>/catalog/dfiles/files
#	4) If the archive is a binary package with multiple user and
#	   groups then the correct GNU_TAR_OWNER_OPTIONS must be used
#          and the checker must be root.
#	5) Archive Signatures as defined by the swbis implementation
#		ascii armored and stored as files:
#		<path>/catalog/dfiles/md5sum
#		<path>/catalog/dfiles/sha1sum  (optional)
#		<path>/catalog/dfiles/adjunct_md5sum
#	6) Gpg signature of the signed file described by the
#		swbis implementation stored as:
#		<path>/catalog/dfiles/signature
#       7) The signer's public key imported onto your gpg keyring.
#            NOTE: Obtaining the signers public key from an untrusted
#                  locatation and subsequently verifying the package
#                  does not prove authenticity,  it only proves the
#                  the package may be an authentic fake.  Likewise
#                  it is always possible an imposter stole the alleged 
#                  signers private key and made this package.
#            To import:  gpg --import "signers_public_key"

premg="checksig: "

# ===============================================================
#  4.1.6.1 Control Script Execution Environment
# ===============================================================

PATH=/bin:/usr/bin:/usr/local/bin:/usr/bin/X11:/usr/X11R6/bin

# These should be set by the calling program into this env.
# Most are not being used here and are merely listed for completeness.
SW_CONTROL_DIRECTORY=""
EXECDISP=${SW_CONTROL_TAG-"no_set_"}

SW_LOCATION=""
SW_ROOT_DIRECTORY=""
PATH=${SW_PATH-${PATH}}
SW_SESSIONS_OPTIONS=""
SW_SOFTWARE_SPEC=""

# ===============================================================
#  User Variables
# ===============================================================

# Customize PACKAGEDIRNAME for your package
# This is the package proper name without any dashes or version.
# If this is an empty string then it will be determined by looking
# at the invocation runtime path.
PACKAGEDIRNAME=""

# ===============================================================
#  Constants
# ===============================================================

TAR_FORMAT_UTILITY_PAT="tar_format_emulation_utility "
TAR_FORMAT_OPTIONS_PAT='tar_format_emulation_options '
GNU_TAR_OWNER_OPTIONS_PAT='gnu_tar_owner_options '
GNU_TAR_FORMAT_OPTIONS_PAT='gnu_tar_format_options '
# Customize TAR_FORMAT_OPTIONS for your package
#TAR_FORMAT_OPTIONS="--posix -b1"

username=${SWBIS_CONTROL_OWNER-"root"}
groupname=${SWBIS_CONTROL_GROUP-"root"}
GNU_TAR_OWNER_OPTIONS="--owner=$username --group=$groupname"

# ===============================================================
#  Internal Routines
# ===============================================================

missing_which() {
	pgm="$1"
	xxname=`which $pgm`
	test -n "$xxname"
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

missing_fatal() {
	pgm="$1"
	"${TEST}" -n "$pgm"
	case "$?" in
		0)
			;;
		*)
			echo "Missing $pgm"; 
			exit 1 
			;;
	esac

	"${TEST}" -x "$pgm" 
	case "$?" in
		0)
			;;
		*)
			echo "Missing $pgm (not executable)"; 
			exit 1 
			;;
	esac
}

not_authentic() {
	echo "${premg}The package should be considered not authentic." 1>&2
}

normalizeMD5() {
	#
	# Convert 'openssl md5' format to md5sum
	#
	sed -e 's/^MD5(//' -e 's/)=/ /' -e 's/^\([^ ]*\)  \(.*\)/\2  \1/'
}

get_dfiles() {
	#
	# Get dfiles value.
	#
	prefixdir="$1"
	DFILES=`${EGREP} "^[[:space:]]*dfiles[[:space:]]" "$prefixdir"/catalog/INDEX |\
	"${TAIL}" -1 |\
	"${SED}" -e 's/[[:space:]]*dfiles[[:space:]][[:space:]]*//'`
	echo "$DFILES"
}

get_tar_creating_utility() {
	id=`get_tar_utility_id "$1"`
	cuans="/bin/false"
	case "$id" in
		gnutar|tar*)
			cuans="${TAR}"
			;;
		bsdpax3)
			missing_fatal "$PAX"
			cuans="${PAX}"
			;;
	esac
	echo "$cuans"
}

get_tar_utility_id() {
	#
	# TAR_FORMAT_UTILITY_PAT="tar_format_emulation_utility "
	#
	prefixdir="$1"
	TAR_FORMAT_UTILITY_ID=`grep "${TAR_FORMAT_UTILITY_PAT}" "$prefixdir"/catalog/INDEX \
	| ${SED} -e s/"${TAR_FORMAT_UTILITY_PAT}"// | sed -e 's/  *//g'`
	echo "$TAR_FORMAT_UTILITY_ID"
}

get_tar_options1() {
	#
	# Get the gnu_tar_format_options value from the INDEX file.
	#
	prefixdir="$1"
	X_FORMAT_OPTIONS_PAT="$2"

	TAR_FORMAT_OPTIONS=`grep "$X_FORMAT_OPTIONS_PAT" "$prefixdir"/catalog/INDEX \
	| ${SED} -e s/"$X_FORMAT_OPTIONS_PAT"//`
	echo "$TAR_FORMAT_OPTIONS"
}

get_tar_options() {
	export X_FORMAT_OPTIONS_PAT="$GNU_TAR_FORMAT_OPTIONS_PAT"
	xx1=`get_tar_options1 "$1" "$X_FORMAT_OPTIONS_PAT"`
	case "$xx1" in
		"")
			export X_FORMAT_OPTIONS_PAT="$TAR_FORMAT_OPTIONS_PAT"
			xx1=`get_tar_options1 "$1" "$X_FORMAT_OPTIONS_PAT"`
			;;
	esac
	echo "$xx1"
}

check_files() {
	csum=`${CAT} ${package_dir}/catalog/dfiles/files | ${SED} -e 's@/$@@' | ${SORT} | ${MD5SUM}`
	fsum=`${TAR} cf - ${package_dir} | ${TAR} tf - | ${SED} -e 's@/$@@' | ${SORT} | ${MD5SUM}`
	if [ "$csum" = "$fsum" ]; then
		return 0
	else
		return 1
	fi
}

create_archive_stream() {
	tarcreateutil="$1"
	case "$tarcreateutil" in
		"$TAR")
			"${TAR}" cf - ${TAR_FORMAT_OPTIONS}  --no-recursion --files-from=- \
			--exclude "${package_dir}"/catalog \
			--exclude "${package_dir}"/catalog/\* \
			${GNU_TAR_OWNER_OPTIONS} 
			;;
		"$PAX")
			"$PAX" -d -w -b 512 
			;;
	esac
}

check_digest_from_catalog_files() {
	digest_program="$1"
	digestfile="$2"
	adjunct="$3"
	tarcreateutil="$4"

	if [ -z "${digest_program}" ]; then
		printf " (User utility missing)"
		return 1
	fi

	csum=`\
	(
	"${CAT}" ${package_dir}/catalog/dfiles/files | 
		(
		if [ "$adjunct" ]; then
			while read file
			do
				if [ ! -h "$file" ]; then
					echo "$file"
				fi
			done
		else
			cat
		fi
		) | sed -e 's@/$@@' |
	"${GREP}" -v "${package_dir}"/catalog
	) | \
		create_archive_stream "$tarcreateutil" | ${digest_program} | sed -e 's/ .*$//'`

	psum=`cat "${digestfile}"`
	if [ "$csum" != "$psum" ]; then
		return 1
	else 
		return 0
	fi
}

# ===============================================================
# Utilities
# ===============================================================
GTAR=${GTAR:-"tar"}

SH=/bin/sh
AWK=`missing_which awk 2>/dev/null`
GAWK=`missing_which gawk 2>/dev/null`
TEST=`missing_which test` || exit 1
GREP=`missing_which grep` || exit 1
EGREP=`missing_which egrep` || exit 1
TAIL=`missing_which tail` || exit 1
EXPAND=`missing_which expand` || exit 1
SED=`missing_which sed` || exit 1
CAT=`missing_which cat` || exit 1
MD5SUM=`which md5sum 2>/dev/null`
SHA1SUM=`which sha1sum 2>/dev/null`
TAR=`missing_which $GTAR` || exit 1
PAX=`which pax 2>/dev/null`
SORT=`missing_which sort` || exit 1
FIND=`missing_which find` || exit 1
RM=`missing_which rm` || exit 1
CMP=`missing_which cmp` || exit 1
MKDIR=`missing_which mkdir` || exit 1
OPENSSL=`which openssl 2>/dev/null`

if [ ! -f "$MD5SUM" ]; then
	if [ -f "$OPENSSL" ]; then
		MD5SUM="openssl md5"	
	else
		echo "missing md5sum and openssl, no way to verify package." 1>&2
		exit 1
	fi
fi

if [ ! -f "$SHA1SUM" ]; then
	if [ -f "$OPENSSL" ]; then
		SHA1SUM="openssl sha1"	
	else
		echo "missing sha1sum and openssl, no way to verify package." 1>&2
		exit 1
	fi
fi

if [ "$GAWK" != "/" ]; then
	AWK="$GAWK"
fi

if [ "$AWK" = "/" -a "$GAWK" = "/" ]; then
	echo "warning: missing awk." 1>&2
fi

# ===============================================================
#  Main Starts Here
# ===============================================================

if [ "$EXECDISP" = "no_set_" ]; then
	echo "The shell variable SW_CONTROL_TAG is not set." 1>&2
	echo "This indicates execution was not as expected." 1>&2
	echo "If you override this mechanism, you alone take" 1>&2
	echo "responsibility that this script itself is authentic." 1>&2
	echo "If you see this message unexpectedly perhaps" 1>&2
	echo "swverify is not setting this environment variable." 1>&2
	exit 1
fi

missing_fatal "$GREP"
missing_fatal "$EGREP"
missing_fatal "$TAIL"
missing_fatal "$EXPAND"
missing_fatal "$SED"
missing_fatal "$CAT"
missing_fatal "$TAR"
missing_fatal "$SORT"
missing_fatal "$FIND"
missing_fatal "$CMP"

case "${PACKAGEDIRNAME}" in
	"")
		# autodetect.
		base1=`pwd`
		package_dir=`echo $base1 | sed -e 's@/catalog.*@@'`
		package_dir=`basename "$package_dir"`
		PACKAGEDIRNAME=`echo "$package_dir" | sed -e 's@-.*@@'`	
		;;
	*)
		;;
esac

PACKAGEDIRNAME_PAT="${PACKAGEDIRNAME}[^/]*"

gpg_homedir="~/.gnupg"
if [ $# -ge 1 ]; then
	if [ "$1" ]; then
		gpg_homedir="$1"
	fi
fi


#+++++++++++++++++++++++
# Get the package path.
#+++++++++++++++++++++++
	base1=`pwd`
	
	package_path=`echo "$base1" |
		sed -e s@'\(.*'"${PACKAGEDIRNAME_PAT}"'\).*@\1@'` 

	package_dir=`echo "$base1" |
		sed -e s@'.*/\('"${PACKAGEDIRNAME_PAT}"'\)@\1@' |
		sed -e s@'\('"${PACKAGEDIRNAME_PAT}"'\).*@\1@'`


	echo "$package_dir" | grep / 1>/dev/null
	case $? in
		0)
		echo "${premg}Try running from within the package directory." 1>&2
		echo "${premg}script failed loc = 1" 1>&2
		exit 1
		;;
	esac


	echo "$package_dir" | grep . 1>/dev/null
	case $? in
		1)
		echo "${premg}Try running from within the package directory." 1>&2
		echo "${premg}script failed loc = 2" 1>&2
		exit 1
		;;
	esac

#+++++++++++++++++++++++++++++++
# Now chdir to the package path.
#+++++++++++++++++++++++++++++++

# FIXME  It would seem "$package_path" is SW_ROOT_DIRECTORY
# which means all this code should be factored up a level to
# the swverify program.
cd "$package_path"
	case $? in
		0)
			;;
		*)
		echo "Try running from within the package directory." 1>&2
		echo "script failed loc = 3" 1>&2
		exit 1
		;;
	esac

cd ..

DFILES=`get_dfiles "${package_dir}"`
TAR_FORMAT_OPTIONS=`get_tar_options "${package_dir}"`
tarcreateutil=`get_tar_creating_utility "${package_dir}"`

md5sum_did_check=""

printf "${premg}Checking files "
check_files
check_files_ret=$?
if [ "$check_files_ret" -ne 0 ]; then
	echo " FAIL (Bad)"
	echo "The file list <catalog/dfiles/files> does not match the package." 1>&2
	not_authentic
	exit 1
fi
echo  "OK (Good)"

symret=""
md5sumfile="${package_dir}/catalog/${DFILES}/md5sum"
sha1sumfile="${package_dir}/catalog/${DFILES}/sha1sum"
if [ -f "${md5sumfile}" ]; then
	md5sum_did_check=1
	sha1sum_ret=0
	printf "${premg}Checking Archive md5 "


	check_digest_from_catalog_files "$MD5SUM" "${md5sumfile}" "" "$tarcreateutil"
	md5sum_ret=$?
	if [ "$md5sum_ret" -ne 0 ]; then
		echo  "FAIL"
		symret=1
	else
		echo  "OK (Good)"
	fi
	if [ -f "${sha1sumfile}" ]; then
		printf "${premg}Checking Archive sha1 "
		check_digest_from_catalog_files "$SHA1SUM" "${sha1sumfile}" "" "$tarcreateutil"
		sha1sum_ret=$?
	fi
	if [ "$md5sum_ret" -eq 0 -a "$sha1sum_ret" -ne 0 -a \
					-n "${SHA1SUM}" ]; then
		echo  "FAIL (Bad)"
		echo "Archive sha1 digest invalid".
		not_authentic
		symret=1
		exit 1
	fi
	if [ "$sha1sum_ret" -eq 0 ]; then
		echo  "OK (Good)"
	else
		echo  "FAIL"
		symret=1
	fi
else
	echo "Archive md5sum file missing.  No way to verify package."
	not_authentic
	exit 1
fi

case "$symret" in
	"")
		;;
	*)
		not_authentic
		exit 1
		;;
esac
echo "${premg}Signature : not checked [see swverify above]"
exit 0
