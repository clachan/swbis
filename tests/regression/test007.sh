#!/bin/sh

TR0=/usr/tmp/test007.0.out
TR1=/usr/tmp/test007.1.out


while read rpmfile
do
	echo $rpmfile
	
	cat $rpmfile | ../../swsupplib/progs/lxpsf -p | ../devel/testpsf2 | ../../swprogs/swbisparse --psf -b >$TR0
	cat $rpmfile | ../../swsupplib/progs/lxpsf -p | ../../swprogs/swbisparse --psf -b >$TR1
	
	
	diff -u $TR0 $TR1
	if [ $? -ne 0 ]; then
		echo "$0 : error 1 in $rpmfile " 1>&2
		exit 1
	fi
done

rm -f /usr/tmp/test007.*


