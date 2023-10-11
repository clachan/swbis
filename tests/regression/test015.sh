#!/bin/sh

version="`tar --version | grep GNU | head -1 | sed -e 's/.* \([^ ]*$\)/\1/'`"
#echo $version

case "$version" in 
	2.[0-9].*)
		echo "It looks like you have a suitable version of GNU tar."
		;;
	1.1[3-9].[1-9][0-9])
		echo "It looks like you have a suitable version of GNU tar."
		;;
	*)
		echo GNU tar version 1.13.17 is required for this test.
		echo You have tar $version.
		echo This test has not been run.
		exit 0
		;;
esac

rm -fr foodir
newcsum="`tar cf - --posix -b 1 . |  cksum`"
newcsum1="`../../swsupplib/tests/testtar . | cksum`"
newcsum2="`find . | ../devel/filelist2tar | cksum`"

if [ "$newcsum1" != "$newcsum2" ]; then
        echo "s0: ../devel/filelist2tar and ../../swsupplib/tests/testtar sums failed." 1>&2
else
        echo \
"s0: ../devel/filelist2tar and ../../swsupplib/tests/testtar sums    PASSED."
fi

if [ "$newcsum" != "$newcsum1" ]; then
        echo "s1: GNU tar and ../../swsupplib/tests/testtar sums ++++++++++++++++++++++++++failed" 1>&2
        exit 1
else
        echo "s1: GNU tar and ../../swsupplib/tests/testtar sums       PASSED."
fi

if [ "$newcsum" != "$newcsum2" ]; then
        echo "s2: GNU tar ../devel/filelist2tar sums ++++++++++++++++++++++++++++++++++++failed" 1>&2
        exit 2
else
        echo "s2: GNU tar ../devel/filelist2tar sums                   PASSED."
fi

