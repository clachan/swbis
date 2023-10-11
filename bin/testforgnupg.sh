#!/bin/sh

#gpg (GnuPG) 1.0.7
#gpg (GnuPG) 1.2.1

GPG=`which gpg`
version="`$GPG --version 2>/dev/null | grep Gnu | head -1 | awk '{print $NF}'`"

case "$version" in 
	1.0.[0-6])
		echo "GnuPG 1.0.[0-6] cannot be used for this test." 1>&2
		echo "You need revision 1.0.7 or 1.2.1" 1>&2
		exit 1
		;;	
	1.2.[0-9]|1.[3-9]*|1.1[0-9]*)
		# OK
		echo 1>/dev/null
		;;
	2.*)
		echo "" 1>&2
		echo "***" 1>&2
		echo "*** gnupg-2.X (aka gpg2) cannot be used for the tests." 1>&2
		echo "*** You must install gnupg-1.X and unlink gpg as gpg2." 1>&2
		echo "*** gpg2 (as gpg) does work fine in production provided you use the gpg-agent." 1>&2
		echo "***" 1>&2
		echo "" 1>&2
		exit 1
		;;
	1.0.7)
		# OK
		echo 1>/dev/null
		;;
	*)
		echo "GNU Privacy Guard revision 1.0.7 or 1.2.x or higher is required." 1>&2
		echo "You have gpg [$version]." 1>&2
		echo "This test has not been run." 1>&2
		exit 1
		;;
esac

exit 0
