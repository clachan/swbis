#!/bin/sh

while read rpmfile
do
	echo $rpmfile
	
	
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf | ../devel/testheader | ../../swprogs/swbisparse --psf >/usr/tmp/testheader.out
	../../swsupplib/progs/rpm2swpsf < $rpmfile | ../devel/testswspsf | ../../swprogs/swbisparse --psf >/usr/tmp/testswspsf.out
	
	diff -u /usr/tmp/testheader.out /usr/tmp/testswspsf.out	
	if [ $? -ne 0 ]; then
		echo "test005: error 1 in $rpmfile " 1>&2
		exit 1
	fi

	../../swsupplib/progs/rpm2swpsf < $rpmfile > /usr/tmp/test005.tmp
	../devel/testswspsf < /usr/tmp/test005.tmp | ../../swprogs/swbisparse --psf >/usr/tmp/testswspsf.out
	
	diff -u /usr/tmp/testheader.out /usr/tmp/testswspsf.out	
	if [ $? -ne 0 ]; then
		echo "test005: error 2 in $rpmfile " 1>&2
		exit 2
	fi


done

rm -f /usr/tmp/test005.tmp
rm -f /usr/tmp/testheader.out
rm -f /usr/tmp/testswspsf.out


