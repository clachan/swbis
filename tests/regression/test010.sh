#!/bin/sh

while read rpmfile
do
	echo $rpmfile
	
	
	cat $rpmfile | 
	../../swsupplib/progs/rpm2swpsf | 
	../devel/testswspsf  >/usr/tmp/testswspsf.out
	
	../../swsupplib/progs/rpm2swpsf < $rpmfile | 
	../devel/testswspsf1 >/usr/tmp/testswspsf1.out
	
	diff -u /usr/tmp/testswspsf.out /usr/tmp/testswspsf1.out	
	if [ $? -ne 0 ]; then
		echo "test010: error 1 in $rpmfile " 1>&2
		exit 1
	fi

done

rm -f /usr/tmp/testswsps*.out
