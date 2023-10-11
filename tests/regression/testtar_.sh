#
# Test storage section aggreement with GNU tar.
case "$GTAR" in "") GTAR=tar; ;; esac

. tests/regression/testlib.sh

opt_format=ustar
case "$1" in "") dir=.; ;; *) dir=$1; ;; esac
case "$2" in "") ;; *) opt_format=$2; ;; esac

cd "$dir"
case "$?" in "0") ;; *) exit 1; ;; esac

echo "Testing storage section aggreement with GNU tar" 1>&2
echo "Test Directory: "`pwd` 1>&2

check_for_gnu_tar
make_format_option
case $? in 0) ;; *) echo $0: exiting with status 1; exit 1; ;; esac

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
		| swpackage -s -\
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
