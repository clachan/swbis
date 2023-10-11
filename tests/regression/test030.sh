#
# Test storage section agreement with GNU tar.

PATH=`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH
export GTAR

case "$GTAR" in "") GTAR=tar; ;; esac

opt_format=ustar
case $# in 1) opt_format="$1"; ;; esac

TOUCH=`which touch`

. tests/regression/testlib.sh

bin/testforgnutar.sh || exit 2
# bin/testforgnupg.sh || exit 2

echo "$0: Testing storage section aggreement with GNU tar" 1>&2
echo "$0: Directory: "`pwd` 1>&2

check_for_gnu_tar
make_format_option
case $? in 0) ;; *) echo $0: exiting with status 1; exit 1; ;; esac

#GNUTAR_FORMAT_OPTIONS="-b1"
#SWPACKAGE_FORMAT_OPTIONS="--format=gnutar"

echo $0: Using swpackage at `which swpackage`
echo $0: Using GNU tar version $gnu_tar_version
echo $0: GNUTAR_FORMAT_OPTIONS=$GNUTAR_FORMAT_OPTIONS
echo $0: SWPACKAGE_FORMAT_OPTIONS=$SWPACKAGE_FORMAT_OPTIONS

printf "$0: swpackage cksum: "
swsum=`printf "\
		distribution\n\
		product\n\
		tag PROD\n\
		control_directory \"\"\n\
		fileset\n\
		tag FSET\n\
		control_directory \"\"\n\
		directory . /\n\
		file *\n\n" \
		| swpackage -s - --no-defaults \
			--swbis-numeric-owner=false \
			$SWPACKAGE_FORMAT_OPTIONS \
			--no-catalog \
			--no-front-dir \
			--dir=. @- | cksum`

printf "$swsum\n"
printf "$0: tar       cksum: "
tarsum=`$GTAR cf - $GNUTAR_FORMAT_OPTIONS . | cksum`
printf "$tarsum\n"

if [ "$tarsum" = "$swsum" ]; then
	exit 0
else
	exit 1
fi
