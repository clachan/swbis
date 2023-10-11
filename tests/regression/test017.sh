#!/bin/sh

retval=0
CATSIZE=/usr/tmp/signtest_catsize
CATFILES=/usr/tmp/signtest_catfiles
SFILES=/usr/tmp/signtest_sfiles
UFILE=/usr/tmp/signtest_uncompressed_file

ztestfilename=../examples/packages/posix-software-2.0.sw.tar.gz
cpio1=../examples/packages/posix-software-2.0.sw.cpio-odc.gz
cpio2=../examples/packages/posix-software-2.0.sw.cpio-newc.gz
cpio3=../examples/packages/posix-software-2.0.sw.cpio-crc.gz
zcat <$ztestfilename >$UFILE

for testfilename in $ztestfilename $UFILE $cpio1 $cpio2 $cpio3
do
	if [ $testfilename = $UFILE ]; then
		ztestfilename="../examples/packages/posix-software-2.0.sw.tar.gz"
	else
		ztestfilename="$testfilename"
	fi

	wsize="`zcat <$ztestfilename | wc -c | sed -e 's/ *//g'`"
	wsum="`zcat <$ztestfilename | md5sum`"

	../devel/get_catalog_file <$testfilename 2>$CATSIZE 1>$CATFILES
	catsum="`cat $CATFILES | md5sum`"
	catsize="`cat <$CATSIZE`"


	../devel/get_storage_file <$testfilename 2>$CATSIZE 1>$SFILES
	filesum="`cat <$SFILES | md5sum`"
	filesize="`cat <$CATSIZE`"

	wcatsum="`zcat  <$ztestfilename | dd bs=1 count=$catsize 2>/dev/null | dd 2>/dev/null  | md5sum`"
	wfilesum="`zcat  <$ztestfilename | dd bs=1 skip=$catsize 2>/dev/null | dd 2>/dev/null | md5sum`"

	sum="`cat $CATFILES $SFILES | md5sum`"
	size="`cat $CATFILES $SFILES | wc -c | sed -e 's/ *//g'`"

	# ASSERTIONS

	/usr/bin/test "$TEST017_DEBUG" &&  echo "DB: $testfilename: L1 size=[$size]  wsize=[$wsize]" 1>&2
	if [ "$size" -ne "$wsize" ]; then
		echo "$testfilename: SIZE CHECK(1)       FAILED" 1>&2
		echo "if [ $size -ne $wsize ]; then" 1>&2
		retval=1
		exit 1
	else
		echo "$testfilename: File size check(1)    PASSED"
	fi


	/usr/bin/test "$TEST017_DEBUG" && echo "DB: $testfilename: L1a catsize=[$catsize]  filesize=[$filesize]" 1>&2
	size="`expr $catsize + $filesize`"
	/usr/bin/test "$TEST017_DEBUG" && echo "DB: $testfilename: L2 size=[$size]  wsize=[$wsize]" 1>&2
	if [ "$size" -ne "$wsize" ]; then
		echo "$testfilename: SIZE CHECK(2)        FAILED" 1>&2
		echo "if [ $size -ne $wsize ]; then" 1>&2
		retval=1
		exit 2
	else
		echo "$testfilename: File size check(2)    PASSED"
	fi

	/usr/bin/test "$TEST017_DEBUG" && echo "DB: $testfilename: L3 wfilesum=[$wfilesum]  filesum=[$filesum]" 1>&2
	if [ "$wfilesum" != "$filesum" ]; then
		echo "$testfilename: FileSum        FAILED" 1>&2
		retval=2
		exit 3
	else
		echo "$testfilename: FileSum      PASSED"
	fi

	/usr/bin/test "$TEST017_DEBUG" && echo "DB: $testfilename: L4 wcatsum=[$wfilesum]  catsum=[$filesum]" 1>&2
	if [ "$wcatsum" != "$catsum" ]; then
		echo "$testfilename: CatSum      FAILED" 1>&2
		retval=2
		exit 4
	else
		echo "$testfilename: CatSum     PASSED"
	fi

	if [ "$wsum" != "$sum" ]; then
		echo "$testfilename: Total Sum      FAILED" 1>&2
		echo "if [ $wsum != $sum ]; then" 1>&2
		retval=2
		exit 5
	else
		echo "$testfilename: Total Sum     PASSED"
	fi

	rm -f "$CATSIZE"  "$CATFILES" "$SFILES"
done
rm -f /usr/tmp/signtest_*
exit $retval
