#
# Script:  checkdigest
# Usage:  checkdigest
#
# This file may be copied and modified without restriction.
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

premg="checkdigest: "
CMD_NOT_FOUND=/
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
			echo $CMD_NOT_FOUND
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

normalize_textmode() {
	#
	# remove the asterisk in the output of md5sum and sha1sum
	#
	sed -e 's/^\([^ ]*\) \*/\1  /'
}

normalize_openssl() {
	case "$1" in
		md*)
			normalize_openssl_MD5
			;;
		sha1*)
			normalize_openssl_SHA1
			;;
	esac
}

normalize_openssl_MD5() {
	#
	# Convert 'openssl md5' format to md5sum
	#
	sed -e 's/^MD5(//' -e 's/)=/ /' -e 's/^\([^ ]*\)  \(.*\)/\2  \1/'
}

normalize_openssl_SHA1() {
	#
	# Convert 'openssl sha1' format to sha1sum
	#
	sed -e 's/^SHA1(//' -e 's/)=/ /' -e 's/^\([^ ]*\)  \(.*\)/\2  \1/'
}

INFO_to_digests() {
	E_DIGTYPE="$1"	
	export E_DIGTYPE
	$VAWK -- '
	BEGIN {
        	# Global Variables
        	#
		PROGNAME="INFO_to_digests()";
		e_digtype=ENVIRON["E_DIGTYPE"];		
		g_exitval=0;
		g_arridx=0;
		g_mode_single_line=1;
		Lines[0]="";
		Keywords[0]="";
		Values[0]="";
		in_file = 0; in_control_file = 0;
		filename="catalog/INFO";
		while (getline g_x <filename > 0) {
			parse_info(g_x);
		}; process_info_object();
	}
	function parse_keyword_value(S, K, V,      l_v, l_k) {
		aa_i=0;
		while(length(S[aa_i]) > 0) {
			l_k = get_keyword(S[aa_i]);
			K[aa_i] = l_k;
			check_errno();
			l_v = get_value(S[aa_i]);
			V[aa_i] = l_v;
			check_errno();
			aa_i++;
        	}
	}
	function get_keyword(line) {
                # Remove the value part and leading whitespace
                # before the keyword
                sub(/^[ \t]*/,"", line);
                sub(/[ \t]+.*/,"", line);
                return line;
	}
	function get_value(line,     junk) {
                # Remove the keyword
                g_errno=0;
                sub(/^[ \t]*[^ \t]+[ \t]+/,"", line);

                if ( line ~ /^[^"]+/) {
                        # Unquoted Value
                        # Remove the comment after the value
                        sub(/#.*/,"", line);
                        sub(/[ \t]*$/,"", line);
                } else if ( line ~ /^".*/) {
                        # Quoted Value
                        # Remove the First Quote
                        sub(/[ \t]*"/,"", line);
                        # Remove the last Quote
                        sub(/".*/,"", line);
                } else {
                        errmsg("error in get_value for [" line "]");
                        line="";
                        g_errno=2;
                }
                return line;
	}
	function find_value(O, keyword,        idx, retval, l_k) {
		idx=0;
		retval=0;
		g_found=0;
		while(length(O[idx]) > 0) {
			l_k = get_keyword(O[idx])
			if (l_k == keyword "") {
				g_found=1;
				return "" get_value(O[idx]);
			}
			idx++;
		}
		g_found=0;
		return "";
	}
	function print_sum(O,             v, p, mtime, att, retval) {
		retval=0;
		if (length(O[0]) == 0) return 0;
		att=e_digtype;
		v = find_value(O, att); check_errno();
		if (g_found != 0) { 
			att="path";
			p = find_value(O, att); check_errno();
			if (g_found == 0) { errmsg(att " attribute not found"); debug_print_object(O, Keywords, Values); return 1; }
			print v "  " p
		}
	}
	function debug_print_object(O, K, V) {
		parse_keyword_value(O, K, V);
		g_arridx=0;
		printf("Object=[%s]\n",  K[g_arridx++]) | "cat 1>&2";
		while(length(O[g_arridx]) > 0) {
			printf("keyword=[%s] value=[%s]\n",  K[g_arridx], V[g_arridx]) | "cat 1>&2";
			g_arridx++;
		}
	}
	function obcopy(D, S) {
		D[0] = S[0];
		for(i=1; i<g_arridx; i++) {
			D[i] = S[i];
		}
		D[g_arridx]=""; # This add a terminating empty string
	}
	function process_info_object (       i, ret) {
		if (in_file == 1) {
			obcopy(O_tmp_file, Lines);
			ret = print_sum(O_tmp_file);
			if (ret != 0) g_exitval=1;
		} else if (in_control_file == 1) {
			obcopy(O_tmp_file, Lines);
			ret = print_sum(O_tmp_file);
			if (ret != 0) g_exitval=1;
		} else {
			;
		}
		return;
	}
	function init_info_object (line,      i) {
		#
		# Got new object, Store the one that just was completed.
		process_info_object();

		#
		# Now determine the object type from the keyword name and set the context
		# variables.

		if (line ~ /file/) {
			# reset_context(); in_file = 1;
			in_file = 1;
		} else if (line ~ /control_file/) {
			# reset_context(); in_control_file = 1;
			in_control_file = 1;
		} else {
			errmsg("error: unrecognized object: " line);
		}
		g_arridx=0;
		Lines[g_arridx++]=line;
	}
	function parse_info(rc) {
		if (g_mode_single_line == 1 &&  rc ~ /(^[ \t]*file|^[ \t]*control_file)/) { init_info_object(rc); return;}
		if (g_mode_single_line == 1 && rc ~ /^[\t ]*#/  ) { return; }
		###  Quoted string all on one line
		if (g_mode_single_line == 1 && rc ~ /^[ \t]*[a-zA-Z_]+[ \t]+"[^"]*"/ ) { process_keyword(rc); return; }
		###  Quoted string all on more than one line
		if (g_mode_single_line == 1 && rc ~ /^[ \t]*[a-zA-Z_]+[ \t]+"[^"]*/ ) { errmsg("INFO file: invalid input");  return; }
		# process the intermediate line of a quoted multi-line value
		if (g_mode_single_line == 0 && rc ~ /^[^"]*$/ ) { errmsg("INFO file: invalid input"); return; }
		# process the last line of a quoted multi-line value
		if (g_mode_single_line == 0 && rc ~ /^[^"]*"($|[ \t]*$)/ ) { errmsg("INFO file: invalid input"); return; }
		###  Any line, including single line attributes
		if (rc ~ /^[ \t]*[a-zA-Z_]/ ) { process_keyword(rc);  return; }
	}
	function process_keyword(line) {
		g_mode_single_line=1;
		Lines[g_arridx]=line;
		g_arridx++;
	}
	function check_errno() {
		if (g_errno != 0) {
			errmsg("error: code=" g_errno);
		}
	}
	function dmsg(msg) {
		printf("%s: <debug>: %s\n", PROGNAME, msg) | "cat 1>&2"
	}
	function errmsg(msg) {
		printf("%s: error: %s\n", PROGNAME, msg) | "cat 1>&2"
	}
	'
}

checkINFOdigests() {
	dig_type="$1"
	dig_utility="$2"

	case "$dig_utility" in
		/)
		case "$dig_type" in
			md5sum|sha1sum)
				return 1
				;;
			*)
			return 98
			;;
		esac
		;;
	esac

	(
	cd ${package_dir}
	case "$?" in
		0)
			;;
		*)
			return 1
			;;
	esac
	tmpdir=/var/tmp/checkdigest.info.$$
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
	"${CAT}" catalog/INFO | INFO_to_digests "$dig_type" | "${SORT}"  >"$tmpfile"
	grep ^. $tmpfile 1>/dev/null
	case "$?" in
		0) ;;
		*)
			#
			# digests were missing
			#
			"${RM}" -fr "$tmpdir"
			case "$dig_type" in
				md5sum)
					return 99
					;;
				sha1sum)
					# For now, make sha1sums in the INFO file optional.
					echo " SKIPPED (not tested)"
					echo "checkdigest: Warning: sha1 digests not present in the INFO file" 1>&2
					return 199
					;;
				sha512sum)
					# For now, make sha512sums in the INFO file optional.
					echo " SKIPPED (not tested)"
					return 199
					;;
			esac
			;;
		*)
			;;
	esac

	"${FIND}" . -type f  -print | "${GREP}" -v '^./catalog/' |
	grep -v "^$tmpfile" |
	(
		while read file
		do
			$dig_utility "$file"
		done
	) |
	( 
		case "$dig_utility" in
			*openssl*)
				normalize_openssl "$dig_type"
				;;
			*)
				normalize_textmode
				;;
		esac

	) | "${SORT}" |
	"${DIFF}" $tmpfile - 1>&2
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
		printf "\
            CAVEAT: Obtaining the signers public key from an untrusted locatation 
	    and subsequently verifying the package does not prove authenticity,\n\
	    it only proves the the package may be an authentic fake.  Likewise\n\
            it is always possible an imposter stole the alleged signers private key\n\
	    and made this package.\n" | sed -e 's/^[[:space:]][[:space:]]*//'
}

get_dfiles() {
	#
	# Get dfiles value.
	#
	prefixdir="$1"
	DFILES=`${EXPAND} "$prefixdir"/catalog/INDEX | ${EGREP} "^ *dfiles " | \
	"${TAIL}" -1 | \
	"${SED}" -e 's/ *dfiles  *//'`
	echo "$DFILES"
}

get_option() {
	prefixdir="$1"
	arg="$2"
	aOPT=`${EXPAND} "$prefixdir"/catalog/INDEX | ${EGREP} '^ *'"${arg}"' ' | \
	"${TAIL}" -1 | \
	"${SED}" -e 's/ *'"${arg}"'  *//'`
	echo "$aOPT"
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

check_files2() {
	tmpdir_f=/tmp/checkdigest.files.$$
	"${MKDIR}" -m u+rw,o-rwx,g-rwx $tmpdir_f
	case $? in
		0)
			;;
		*)
			echo "Error making tmpdir : $tmpdir_f"
			not_authentic
			exit 1
			;;
	esac
	tmpfifo="$tmpdir_f"/fifo
	"${MKFIFO}" "$tmpfifo"
	case $? in 0) ;; *) echo "error mkfifo" 1>&2; exit 1 ;; esac

	(${CAT} "${package_dir}"/catalog/dfiles/files | ${SED} -e 's@/$@@' | ${SORT} | dd of="$tmpfifo" 2>/dev/null) &
	find "${package_dir}" | ${SED} -e 's@/$@@' | ${SORT} | ${DIFF} -h - "$tmpfifo"
	ret_f=$?
	wait
	"${RM}" -f "$tmpfifo"
	"${RMDIR}" "$tmpdir_f"
	return $ret_f
}

check_files_by_md5() {
	csum=`${CAT} "${package_dir}"/catalog/dfiles/files | ${SED} -e 's@/$@@' | ${SORT} | ${MD5SUM} | sed -e s/^'(stdin)= '//`
	fsum=`find "${package_dir}" | ${SED} -e 's@/$@@' | ${SORT} | ${MD5SUM} | sed -e s/^'(stdin)= '//`
	# OLD slow way:  fsum=`${TAR} cf - ${package_dir} | ${TAR} tf - | ${SED} -e 's@/$@@' | ${SORT} | ${MD5SUM} | sed -e s/^'(stdin)= '//`
	case "$csum" in
		"") return 1
		;;
	esac

	case "$fsum" in
		"") return 1
		;;
	esac

	if [ "$csum" = "$fsum" ]; then
		return 0
	else
		return 1
	fi
}

check_files() {
	check_files_by_md5
	ret_aa=$?
	check_files2
	ret_bb=$?
	case "$ret_aa" in
		0) ;;
		*) return 1
		;;
	esac

	case "$ret_bb" in
		0) ;;
		*) return 1
		;;
	esac
	return 0
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
	"${GREP}" -v ^"${package_dir}"/catalog
	) | \
		create_archive_stream "$tarcreateutil" | ${digest_program} | sed -e 's/ .*$//' | sed -e s/^'(stdin)= '//`


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
SHA512SUM=`missing_which sha512sum 2>/dev/null`
TAR=`missing_which $GTAR` || exit 1
PAX=`missing_which pax 2>/dev/null`
SORT=`missing_which sort` || exit 1
FIND=`missing_which find` || exit 1
RM=`missing_which rm` || exit 1
CMP=`missing_which cmp` || exit 1
MKDIR=`missing_which mkdir` || exit 1
MKFIFO=`missing_which mkfifo` || exit 1
RMDIR=`missing_which rmdir` || exit 1
DIFF=`missing_which diff` || exit 1
OPENSSL=`missing_which openssl 2>/dev/null`

$DIFF --version 1>/dev/null  2>&1
case $? in
        0)
        ;;
        *)
        case "`uname -s`" in
                SunOS)
# Why won't /usr/bin/diff on SunOS (Oracle Solaris 11.1 c.Mar2014) read correctly from a FIFO ?
# It seems to be busted in the usage where the file is a FIFO (below) whilst GNU diff works OK.
                        echo "$0: looking for a suitable version of diff on `uname -s` system" 1>&2
                        DIFF=`missing_which /usr/gnu/bin/diff` || exit 1
                ;;
        esac
esac

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
	$GAWK --version | head -1 | grep GNU | grep "Awk 3.1.2" 1>/dev/null
	case $? in
		0)
			echo "checkdigest: Warning: GNU awk (gawk) 3.1.2 is broken" 1>&2
			echo "checkdigest: GNU awk will not be used" 1>&2
			;;
		*)
			VAWK="$GAWK"
			;;
	esac
	;;
esac

if [ "$VAWK" = "/" ]; then
	if [ "$MAWK" != "/" ]; then
		echo "checkdigest: Using mawk for AWK" 1>&2
		VAWK="$MAWK"
	fi
fi

if [ "$VAWK" = "/" ]; then
	if [ "$AWK" != "/" ]; then
		echo "checkdigest: Using awk for AWK" 1>&2
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
	echo "To run this script manually set the environment variable:" 1>&2
	echo "SW_CONTROL_TAG=checkdigest # or" 1>&2
	echo "SW_CONTROL_TAG=checkfile" 1>&2
	echo "Exiting with value 1." 1>&2
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

filelist_error_condition=0

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
	echo "${premg}The file list <catalog/dfiles/files> does not match the package." 1>&2
	not_authentic
	case "$SW_CONTROL_TAG" in
		checkfile)
				#
				# Continue with files check
				#
				filelist_error_condition=1;
				;;
			*)
				exit 1
				;;
	esac
else
	echo  "OK (Good)"
fi

adjunct_md5sum_ret=1
archive_digest_failure=""

md5sumfile="${package_dir}/catalog/${DFILES}/md5sum"
sha1sumfile="${package_dir}/catalog/${DFILES}/sha1sum"
sha512sumfile="${package_dir}/catalog/${DFILES}/sha512sum"

case "$SW_CONTROL_TAG" in
checkdigest|checksig)
	if [ -f "${md5sumfile}" ]; then
		md5sum_did_check=1
		sha1sum_ret=0
		sha512sum_ret=1
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
		
		if [ -f "${sha512sumfile}" ]; then
			printf "${premg}Checking Archive sha512 "
			case "$SHA512SUM" in
				/)
					echo \
					"${premg}The sha512sum utility was not found" 1>&2
					sha512sum_ret=0
					;;
				*)
					check_digest_from_catalog_files "$SHA512SUM" "${sha512sumfile}" "" "$tarcreateutil"
					sha512sum_ret=$?
					;;
			esac
		else
			sha512sum_ret=99
		fi
	
		case "$sha512sum_ret" in
			99)
				;;
			*)
			if [ "$md5sum_ret" -eq 0 -a "$sha1sum_ret" -eq 0 -a "$sha512sum_ret" -ne 0 -a "${SHA512SUM}" != $CMD_NOT_FOUND ]; then
				echo  "FAIL (Bad)"
				echo "Archive sha512 digest invalid".
				not_authentic
				exit 1
			fi

			if [ "$sha512sum_ret" -eq 0 ]; then
				echo  "OK (Good)"
			else
				echo  "FAIL (deferred)"
			fi
				;;
		esac
	else
		echo "Archive md5sum file missing.  No way to verify package."
		not_authentic
		exit 1
	fi

	if [ "$md5sum_ret" -ne 0 ]; then

	does_have_symlinks
	case $? in
		0)
		# Yes.
		echo "${premg}Symlinks found in package, this explains md5sum failure."
		echo "${premg}Continuing with adjunct_md5sum and symlink tests ..."
		archive_digest_failure=""
		;;
		*)
		# No
		echo "${premg}No symlinks found in package, the archive digest failure(s) cannot be"
		echo "${premg}explained, therefore not averted. The package will be deemed"
		echo "${premg}not authentic regardless of additional test results."
		archive_digest_failure="yes"
		echo "${premg}Continuing with file digests, adjunct_md5sum and symlink tests ..."
		# not_authentic
		# exit 1
		;;
	esac
	
	printf "${premg}Checking INFO file md5 digests "
	checkINFOdigests "md5sum" "$MD5SUM"
	infodigestret=$?
	case $infodigestret in
		98)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The md5sum utility was not found" 1>&2
			not_authentic
			exit 1
			;;
		99)
			# No md5 in info file.
			echo "checkdigest: Warning INFO file does not have md5sum attributes" 1>&2
			filelist_error_condition=1;
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file md5sum attributes did not all match the package files." 1>&2
			not_authentic
			exit 1
			;;
	esac

	printf "${premg}Checking INFO file sha1 digests "
	checkINFOdigests "sha1sum" "$SHA1SUM"
	infodigestret=$?
	case $infodigestret in
		199)
			# No sha1 in info file.
			echo "checkdigest: Warning INFO file does not have sha1sum attributes" 1>&2
			;;
		98)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The sha1sum utility was not found" 1>&2
			not_authentic
			exit 1
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file sha1sum attributes did not all match the package files." 1>&2
			not_authentic
			exit 1
			;;
	esac

	printf "${premg}Checking INFO file sha512 digests "
	checkINFOdigests "sha512sum" "$SHA512SUM"
	infodigestret=$?
	case $infodigestret in
		98)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}Warning: the sha512sum utility was not found" 1>&2
			;;
		199)
			# No sha512 in info file.
			echo "checkdigest: Warning INFO file does not have sha512sum attributes" 1>&2
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file sha512sum attributes did not all match the package files." 1>&2
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
			archive_digest_failure="yes"
			echo  " FAIL (Bad)"

				printf "\
		The adjunct_md5sum failed. The package should be considered\n\
		non-authentic and should be rejected.\n" | sed -e 's/^[[:space:]][[:space:]]*//' 1>&2
	
		echo  1>&2
				printf "\
		The ability to check the archive message digest from the\n\
		installed directory form of a package is fragile.\n\
		A possible cause is a file timestamp or permission is wrong,\n\
		Try unpacking from scratch using GNU tar with --preserve-permissions.\n\
		Other causes are GNU tar and swpackage version incompatibilities,\n\
		and, file names longer than 99 chars with various versions of GNU tar\n\
		and swpackage.\n" | sed -e 's/^[[:space:]][[:space:]]*//' 1>&2
		echo  1>&2

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
;;
checkfile)
	symret=0
	md5sum_ret=0
	echo "${premg}Archive digests will not be checked."

	printf "${premg}Checking INFO file md5 digests "
	checkINFOdigests "md5sum" "$MD5SUM"
	infodigestret=$?
	case $infodigestret in
		99)
			# No md5 in info file.
			echo "checkdigest: Warning INFO file does not have md5sum attributes" 1>&2
			filelist_error_condition=1;
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file md5sum attributes did not all match the package files." 1>&2
			not_authentic
			exit 1
			;;
	esac

	printf "${premg}Checking INFO file sha1 digests "
	checkINFOdigests "sha1sum" "$SHA1SUM"
	infodigestret=$?
	case $infodigestret in
		199)
			# No sha1 in info file.
			echo "checkdigest: Warning INFO file does not have sha1sum attributes" 1>&2
			filelist_error_condition=1;
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file sha1sum attributes did not all match the package files." 1>&2
			not_authentic
			exit 1
			;;
	esac

	printf "${premg}Checking INFO file sha512 digests "
	checkINFOdigests "sha512sum" "$SHA512SUM"
	infodigestret=$?
	case $infodigestret in
		199)
			# No sha512 in info file.
			echo "checkdigest: Warning INFO file does not have sha512sum attributes" 1>&2
			;;
		98)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}Warning: the sha512sum utility was not found" 1>&2
			;;
		0)
			echo \
			" OK (Good)"
			;;
		*)
			echo \
			" FAIL (Bad)"
			echo \
			"${premg}The INFO file sha512sum attributes did not all match the package files." 1>&2
			not_authentic
			exit 1
			;;
	esac
;;
*)
	echo "${premg}unsupported SW_CONTROL_TAG value" 1>&2
	not_authentic
	exit 1
;;

esac

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

case "$filelist_error_condition" in
	0)
		;;
	*)
		echo \
		"${premg}There was a problem with the filelist or file digests." 1>&2
		not_authentic
		exit 1
		;;
esac

echo "${premg}Signature not checked [see swverify above]"

case "$archive_digest_failure" in
	"")
		exit 0;
		;;
	*)
		exit 1;
		;;
esac
exit 1
