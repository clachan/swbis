#!/bin/sh

if [ "$GTAR" = "" ]; then
	GTAR=tar
fi

TAR=`which $GTAR`
version="`$TAR --version | grep GNU | head -1 | awk '{print $NF}'`"


case "$version" in 
	1.13.19)
		echo "GNU Tar 1.13.19 is busted, this test cannot be run." 1>&2
		echo "Get 1.13.25 or 1.13.17" 1>&2
		exit 1
		;;	
	2.[0-9].*)
		#echo "It looks like you have a suitable version of GNU tar [$version]."
		echo 1>/dev/null
		;;
	1.14.*|1.14|1.15.*|1.1[3-9].[1-9][0-9]|1.1[6-9]|1.1[6-9].*|1.[2-9][0-9]|1.[2-9][0-9]*|2.0)
		echo 1>/dev/null
		#echo "It looks like you have a suitable version of GNU tar [$version]."
		;;
	*)
		echo "GNU tar version 1.13.17,25 is required for this test." 1>&2
		echo "You have tar [$version]." 1>&2
		echo "This test has not been run." 1>&2
		exit 1
		;;
esac

exit 0
