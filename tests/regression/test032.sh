#
# Test storage section aggreement with GNU tar.

TESTDIR=/etc

uid=`id | awk 'BEGIN { FS=" " } { gsub(/^.*=/, "",  $1); gsub(/\(.*/, "",  $1); print $1}'`
case "$uid" in
	0)
		;;
	*)
		printf "You must be root for this test  " 
		exit 2
		;;
esac

PATH=`pwd`/tmp/bin:`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH
export GTAR
case "$GTAR" in "") GTAR=tar; ;; esac
bin/testforgnutar.sh || exit 1
sh tests/regression/testtar_.sh $TESTDIR
exit $?
