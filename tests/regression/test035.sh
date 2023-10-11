#!/bin/bash
# Test swpackage extended definition processing 
#

# The test number
TESTNO=035

PATH=$PATH:../../swsupplib/progs/:swsupplib/progs

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

run_swpackage() {
	psfname="$1"
	baselinename="$2"
	new_baseline="$3"
	match_pattern="$4"
	$SWPACKAGE \
		-s "$psfname" \
		--uuid=1000 \
		--create-time=100000 \
		@- |
	$SWBISTAR -Dv |
	egrep -v "$match_pattern" |
	(
		case "$new_baseline" in
			"")
				dd 2>/dev/null
				;;
			*)
				dd of="$baselinename" 2>/dev/null
				;;
		esac
	)
	return $?
}


SWPACKAGE=`missing_which swpackage` || exit 1
SWBISTAR=`missing_which swbistar` || exit 1


printf "$0: Extended Definition Explicit File Test\n"
printf "$0: Using swpackage at $SWPACKAGE \n"
printf "$0: Using swbistar at $SWBISTAR \n"
printf "$0: Using `swpackage --version | head -1`\n"


testset="\
../testinputfiles/psf.9.2.1 \
"
construct_baseline=""

#
# parse args
#

arg1="$1"
case "$arg1" in
	--construct-baseline)
		construct_baseline="x"
		;;
	--*)
		echo "$0: unrecognized arg" 1>&2
		exit 1
		;;
esac

if [ ! -d baseline ]; then
	echo "$0 this test must be run in the tests/regression directory" 1>&2
	exit 1
fi

for ifile in $testset
do
	ret="x"
	ret=""
	filename=`basename $ifile`
	baselinename="baseline/test${TESTNO}.${filename}.out"

	printf "$0: input file: $ifile:   "
	dematch_pattern='(^mtime)|(^uname)|(^gname)'
	run_swpackage \
		"$ifile" \
		"$baselinename" \
		"$construct_baseline" \
		"$dematch_pattern" |
	(
		case "$construct_baseline" in
			"")
				#
				# do the test
				#
				diff -u $baselinename -
				case "$?" in
					0)	
						echo "       PASSED."
						;;
					*)
						echo "       FAILED."
						exit 1
						;;
				esac
				;;
			*)
				cat >/dev/null
				echo "       NOT RUN (new baseline)."
				;;
		esac
	)
done
ret=$?
case $ret in
	0)
		printf "$0: extended definition test: complete, STATUS: PASSED\n"
		;;
	*)
		printf "$0: extended definition test: complete, STATUS: FAILED\n"
		;;
esac
exit $ret
