#
# NOTE: you probably want to use ./checkdigest, this file is no
# longer maintained and the name 'checksig' is deprectated in
# favor of 'checkdigest'

# Script:  checksig
# Usage:  checksig [gpg_homedir]
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


# ===============================================================
#  Internal Routines
# ===============================================================

missing_which() {
	pgm="$1"
	xxname=`which $pgm`
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

normalizeSHA1() {
	#
	# Convert 'openssl sha1' format to sha1sum
	#
	sed -e 's/^SHA1(//' -e 's/)=/ /' -e 's/^\([^ ]*\)  \(.*\)/\2  \1/'
}

INFO_to_digests() {
	(
	sed    -e 's/^[[:space:][:space:]]*file.*/\
	file/' -e 's/^[[:space:][:space:]]*control_file.*/\
	control_file/' |
	$VAWK -- '
	BEGIN { RS = ""; FS="\n"; }
	{
	#print $0
	if (/[ \t]*file[# \t]*/) { 
		#printf ("HERE %s\n", $0);
		path = field("path"); 
		md5sum = field("md5sum");
		type = field("type");
		if (type == "f") {
			if (md5sum == "") {
				md5sum = "X"
			}
			printf("%s  %s\n", md5sum, path); 
		}
		next;
	}
	}
	END {  }
	function field(name,	i,f) {
		#printf("NF = %d  NR = %d\n", NF, NR);
		for (i = 1; i <= NF; i++) {
			split($i, f, " ");
			if (match($i, "^ *" name " ")) {
			     sub("^ *" name " ", "", $i);
			     return $i;
			}
       	 }
		printf("error: no field %s in record\n", name)  | "cat 1>&2"
	}
	'
	) 2>/dev/null 
}

checkINFOdigests() {
	(
	cd ${package_dir}
	case "$?" in
		0)
			;;
		*)
			return 1
			;;
	esac
	tmpdir=/var/tmp/checksig.info.$$
	"${MKDIR}" -m u+rw,o-rwx,g-rwx $tmpdir
	case $? in
		0)
			;;
		*)
			echo "Error making tmpdir : $tmpdir"
			not_authentic
			exit 1
			;;
	esac
	tmpfile="${tmpdir}/checkINFOdigests"
	"${CAT}" catalog/INFO | INFO_to_digests | "${SORT}"  >"$tmpfile"
	grep ^X $tmpfile 1>/dev/null
	case "$?" in
		0)
			"${RM}" -fr "$tmpdir"
			return 99
			;;
		*)
			;;
	esac

	"${FIND}" . -type f  -print | "${GREP}" -v '^./catalog/' |
	grep -v "^$tmpfile" |
	(
		while read file
		do
			# xargs $MD5SUM |
			$MD5SUM "$file"
		done
	) |
	( 
		case "$MD5SUM" in
			*openssl*)
				normalizeMD5
				;;
			*)
				cat
				;;
		esac

	) | "${SORT}" |
	diff $tmpfile -
	ret=$?
	"${RM}" -fr "$tmpdir"
	return $ret
	)
}

files2symlinks() {
	while read file
	do
		if [ -h "$file" ]; then
			ls -ld "$file"
		fi
	done |
	sed -e 's@.* \([^ ]* -> [^ ]*\)$@\1@'
}

info2symlinks() {
	sed    -e 's/^[[:space:][:space:]]*file.*/\
	file/' -e 's/^[[:space:][:space:]]*control_file.*/\
	control_file/' |
	$VAWK -- '
	BEGIN { RS = ""; FS="\n"; }
	{
	#print $0
	if (/[ \t]*file[# \t]*/) { 
		type = field("type");
		if (type == "s") {
			path = field("path"); 
			link_source = field("link_source");
			printf("%s -> %s\n", path, link_source); 
		}
		next;
	}
	}
	END {  }
	function field(name,	i,f) {
		#printf("NF = %d  NR = %d\n", NF, NR);
		for (i = 1; i <= NF; i++) {
			#printf ("IIII %s\n", $i);
			split($i, f, " ")
			if (f[1] == name)
			     return f[2]
       	 }
		printf("error: no field %s in record\n", name)  | "cat 1>&2"
	}
	' 2>/dev/null
}

echo_caveat() {
		echo  -e -n "\
            CAVEAT: Obtaining the signers public key from an untrusted locatation 
	    and subsequently verifying the package does not prove authenticity,\n\
	    it only proves the the package may be an authentic fake.  Likewise\n\
            it is always possible an imposter stole the alleged signers private key\n\
	    and made this package.\n\
		  " | sed -e 's/^[[:space:]][[:space:]]*//'
}

get_dfiles() {
	#
	# Get dfiles value.
	#
	prefixdir="$1"
	DFILES=`${EGREP} "^[[:space:]]*dfiles[[:space:]]" "$prefixdir"/catalog/INDEX | \
	"${TAIL}" -1 | \
	"${SED}" -e 's/[[:space:]]*dfiles[[:space:]][[:space:]]*//'`
	echo "$DFILES"
}

get_option() {
	prefixdir="$1"
	arg="$2"
	DFILES=`${EGREP} '^[[:space:]]*'"${arg}"'[[:space:]]' "$prefixdir"/catalog/INDEX | \
	"${TAIL}" -1 | \
	"${SED}" -e 's/[[:space:]]*'"${arg}"'[[:space:]][[:space:]]*//'`
	echo "$DFILES"
}

get_tar_creating_utility() {
	id=`get_tar_utility_id "$1"`
	cuans="/bin/false"
	case "$id" in
		tar*|gnutar)
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
			# --exclude "${package_dir}"/catalog 
			# --exclude "${package_dir}"/catalog/\* 
			"${TAR}" cf - ${TAR_FORMAT_OPTIONS} --no-recursion --files-from=- \
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

does_have_symlinks() {
	"${FIND}" "${package_dir}" -type l | "${GREP}" '..' 1>/dev/null
	return $?
}

check_symlinks() {
	(
	scat=`${CAT} ${package_dir}/catalog/$DFILES/files | \
		files2symlinks | \
		${SED} -e 's@'${package_dir}/'@@'  | ${MD5SUM}`
	echo $scat
	( cd ${package_dir};
		icat=`${CAT} catalog/INFO | info2symlinks | ${SED} -e 's@^\./@@'  | ${MD5SUM}`
		echo $icat
	)
	) | (
	read line1; read line2
	if [ "$line1" != "$line2" ]; then
		return 1
	else
		return 0
	fi
	)
}

# ===============================================================
# Utilities
# ===============================================================

GTAR=${GTAR:-"tar"}
SH=/bin/sh
AWK=`missing_which awk 2>/dev/null`
GAWK=`missing_which gawk 2>/dev/null`
MAWK=`missing_which mawk 2>/dev/null`
TEST=`missing_which test` || exit 1
GREP=`missing_which grep` || exit 1
EGREP=`missing_which egrep` || exit 1
TAIL=`missing_which tail` || exit 1
EXPAND=`missing_which expand` || exit 1
SED=`missing_which sed` || exit 1
CAT=`missing_which cat` || exit 1
MD5SUM=`missing_which md5sum 2>/dev/null`
SHA1SUM=`missing_which sha1sum 2>/dev/null`
TAR=`missing_which $GTAR` || exit 1
PAX=`missing_which pax 2>/dev/null`
SORT=`missing_which sort` || exit 1
FIND=`missing_which find` || exit 1
RM=`missing_which rm` || exit 1
CMP=`missing_which cmp` || exit 1
MKDIR=`missing_which mkdir` || exit 1
OPENSSL=`missing_which openssl 2>/dev/null`

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

VAWK=/

case "$GAWK" in
/)
	;;
*)
	$GAWK --version | head -1 | grep GNU | grep 3.1.2 1>/dev/null
	case $? in
		0)
			echo "checksig: Warning: GNU awk (gawk) 3.1.2 is broken" 1>&2
			echo "checksig: GNU awk will not be used" 1>&2
			;;
		*)
			VAWK="$GAWK"
			;;
	esac
	;;
esac

if [ "$VAWK" = "/" ]; then
	if [ "$MAWK" != "/" ]; then
		echo "checksig: Using mawk for AWK" 1>&2
		VAWK="$MAWK"
	fi
fi

if [ "$VAWK" = "/" ]; then
	if [ "$AWK" != "/" ]; then
		echo "checksig: Using awk for AWK" 1>&2
		VAWK="$AWK"
	fi
fi

if [ "$VAWK" = "/" ]; then
	echo "warning: missing versions of awk." 1>&2
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

DIST_OWNER=`get_option "${package_dir}" owner`
DIST_GROUP=`get_option "${package_dir}" group`

optowner=""
optgroup=""

case "$DIST_OWNER" in 
		"") 
			;; 
		*)
			optowner="--owner=$DIST_OWNER"
			;;
esac

case "$DIST_GROUP" in 
		"") 
			;;
		*)
			optgroup="--group=$DIST_GROUP"
			;;
esac

GNU_TAR_OWNER_OPTIONS="$optowner $optgroup $SWBIS_CONTROL_NUMERIC_OWNER"

DFILES=`get_dfiles "${package_dir}"`
TAR_FORMAT_OPTIONS=`get_tar_options "${package_dir}"`
tarcreateutil=`get_tar_creating_utility "${package_dir}"`

md5sum_did_check=""
adjunct_md5sum_did_check=""
symlinks_did_check=""

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

md5sumfile="${package_dir}/catalog/${DFILES}/md5sum"
sha1sumfile="${package_dir}/catalog/${DFILES}/sha1sum"
if [ -f "${md5sumfile}" ]; then
	md5sum_did_check=1
	sha1sum_ret=0
	printf "${premg}Checking Archive md5 "


	check_digest_from_catalog_files "$MD5SUM" "${md5sumfile}" "" "$tarcreateutil"
	md5sum_ret=$?
	if [ "$md5sum_ret" -ne 0 ]; then
		echo  "FAIL (deferred)"
	else
		echo  "OK (Good)"
	fi
	if [ -f "${sha1sumfile}" ]; then
		printf "${premg}Checking Archive sha1 "
		check_digest_from_catalog_files "$SHA1SUM" "${sha1sumfile}" "" "$tarcreateutil"
		sha1sum_ret=$?
	fi
	if [ "$md5sum_ret" -eq 0 -a "$sha1sum_ret" -ne 0 -a -n "${SHA1SUM}" ]; then
		echo  "FAIL (Bad)"
		echo "Archive sha1 digest invalid".
		not_authentic
		exit 1
	fi
	if [ "$sha1sum_ret" -eq 0 ]; then
		echo  "OK (Good)"
	else
		echo  "FAIL (deferred)"
	fi
else
	echo "Archive md5sum file missing.  No way to verify package."
	not_authentic
	exit 1
fi

adjunct_md5sum_ret=1

if [ "$md5sum_ret" -ne 0 ]; then

	does_have_symlinks
	case $? in
		0)
			# Yes.
			echo "${premg}Symlinks found in package, this explains md5sum failure."
			echo "${premg}Continuing with adjunct_md5sum and symlink tests ..."
			;;
		*)
			# No
			echo "${premg}No symlinks found in package, md5sum failure not averted."
			not_authentic
			exit 1
			;;
	esac
	
	printf "${premg}Checking INFO file md5 digests "
	checkINFOdigests
	infodigestret=$?
	case $infodigestret in
		99)
			# No md5 in info file.
			echo " INFO file does not have md5sum attributes"
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file md5sum attribute did not match the package files." 1>&2
			not_authentic
			exit 1
			;;
	esac

	md5sumfile="${package_dir}/catalog/${DFILES}/adjunct_md5sum"
	if [ -f "${md5sumfile}" ]; then
		adjunct_md5sum_did_check=1
		printf "${premg}Checking Archive adjunct md5 "
		check_digest_from_catalog_files "$MD5SUM" "${md5sumfile}" "do_adjunct" "$tarcreateutil"
		adjunct_md5sum_ret=$?
	fi

	case "$adjunct_md5sum_ret" in
		0)
			echo  " OK (Good)"
			;;
		*)
			echo  " FAIL (Bad)"

				echo  -e -n  "\
		The adjunct_md5sum failed. The package should be considered\n\
		non-authentic and should be rejected.\n\
		** Signature not checked. **\n\
				  " | sed -e 's/^[[:space:]][[:space:]]*//' 1>&2
	
		echo  1>&2
				echo  -e -n  "\
		But wait, not so fast...\n\
		The ability to check the installed directory form of a package\n\
		is fragile.  Gnu tar version 1.13.25 and pax version 3.0 seem\n\
		to work, 1.13.19 may not preserve directory modification times\n\
		and this will cause a md5sum failure.\n\
				  " | sed -e 's/^[[:space:]][[:space:]]*//' 1>&2
		echo  1>&2

				echo  -e -n  "\
		To check the archive directly you need the swverify program.\n\
		The swverify utility has no external dependence and its result\n\
		is definitive.  Only archives delivered in GNU tar format with or\n\
		without --posix option and unpacked in a GNU/Linux POSIX.2\n\
		environment can be checked from the the directory form of the\n\
		distribution.  Also if the package was delivered in the default\n\
		format (i.e GNU tar --posix) and a pathname length is greater then\n\
		99 chars then you will unable to authenticate the directory form\n\
		with the current GNU tar utility.\n\
		Exiting with status 1\n\
				  " | sed -e 's/^[[:space:]][[:space:]]*//' 1>&2
		not_authentic
		exit 1
		;;
	esac
	symret=0
	printf "${premg}Checking symlinks "
	symlinks_did_check=1
	check_symlinks
	symret=$?
	case $symret in
		0)
			echo  "OK (Good)"
			;;
		*)
			echo  " FAIL (Bad)"
			echo "${premg}The symlink check did not pass, the package may be corrupted." 1>&2
			echo "${premg}Do not use it." 1>&2
			not_authentic
			exit 1
			;;
	esac
else
	#
	# md5sum OK
	#
	symret=0
fi

if [ "$md5sum_ret" -ne 0 ]; then
	case "$adjunct_md5sum_did_check" in
		"")
			exit 1
			;;
		*)
			if [ "$adjunct_md5sum_ret" -ne 0 ]; then
				exit 1
			fi
	esac
fi

case $symret in
	0)
		;;
	*)
		not_authentic
		exit 1
		;;
esac
echo "${premg}Signature: not checked [see swverify above]"
exit 0
