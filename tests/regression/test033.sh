#
# Test storage section aggreement with GNU tar.

TESTDIR=/etc

opt_format=ustar
case "$1" in "") ;; *) opt_format=$1; ;; esac

PATH=`pwd`/tmp/bin:`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH

. tests/regression/testlib.sh

uid=`id | awk 'BEGIN { FS=" " } { gsub(/^.*=/, "",  $1); gsub(/\(.*/, "",  $1); print $1}'`
case "$uid" in
	0)
		;;
	*)
		printf "You must be root for this test  " 
		exit 2
		;;
esac

check_for_gnu_tar
make_format_option
case $? in 0) ;; *) echo $0: exiting with status 1; exit 1; ;; esac

export PATH
export GTAR
case "$GTAR" in "") GTAR=tar; ;; esac
bin/testforgnutar.sh || exit 2

echo "Testing storage section aggreement with GNU tar" 1>&2
echo "Using Directories /etc/ and /usr/sbin" 1>&2

echo $0: Using swpackage at `which swpackage`
echo $0: Using GNU tar version $gnu_tar_version
echo $0: GNUTAR_FORMAT_OPTIONS=$GNUTAR_FORMAT_OPTIONS
echo $0: SWPACKAGE_FORMAT_OPTIONS=$SWPACKAGE_FORMAT_OPTIONS

cd /

printf "$0: swpackage cksum: "
swsum=`printf "\
		distribution\n\
		product\n\
		tag PROD\n\
		control_directory \"\"\n\
		fileset\n\
		tag FSET\n\
		control_directory \"\"\n\
		directory ./etc /etc\n\
		file *\n\n\
		fileset\n\
		tag FSET1\n\
		control_directory \"\"\n\
		directory ./usr/sbin /usr/sbin\n\
		file *\n\n" \
		| swpackage -s - \
			$SWPACKAGE_FORMAT_OPTIONS \
			--no-catalog \
			--no-front-dir \
			--dir=. @- | cksum`
printf "$swsum\n"
printf "$0: tar       cksum: "
tarsum=`$GTAR cf - $GNUTAR_FORMAT_OPTIONS ./etc ./usr/sbin | cksum`
printf "$tarsum\n"

if [ "$tarsum" = "$swsum" ]; then
	exit 0
else
	exit 1
fi
exit 1
