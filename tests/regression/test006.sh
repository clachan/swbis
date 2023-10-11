#!/bin/sh

while read rpmfile
do
	echo $rpmfile
	
	
	
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf |
	../devel/testswspsf 			>/usr/tmp/testswspsf0.out
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf |
	../../swprogs/swbisparse --psf -b 		>/usr/tmp/testswparse0.out
	
	
	diff -u /usr/tmp/testswparse0.out /usr/tmp/testswspsf0.out
	if [ $? -ne 0 ]; then
		echo "test006: error 1 in $rpmfile " 1>&2
		exit 1
	fi



	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf |
	../../swprogs/swbisparse --psf 			>/usr/tmp/testswparse0.out
	
	
	
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf |
	../../swprogs/swbisparse --psf 			>/usr/tmp/testswparse1.out
	diff -u /usr/tmp/testswparse0.out /usr/tmp/testswparse1.out
	if [ $? -ne 0 ]; then
		echo "test006: error 2 in $rpmfile " 1>&2
		exit 2
	fi



	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf |
	../../swprogs/swbisparse -b --psf |
	../../swprogs/swbisparse --psf 			>/usr/tmp/testswparse1.out

	diff -u /usr/tmp/testswparse0.out /usr/tmp/testswparse1.out
	if [ $? -ne 0 ]; then
		echo "test006: error 3 in $rpmfile " 1>&2
		exit 3
	fi




	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf |
	../../swprogs/swbisparse -b --psf |
	../../swprogs/swbisparse -b --psf |
	../../swprogs/swbisparse --psf 			>/usr/tmp/testswparse1.out
	diff -u /usr/tmp/testswparse0.out /usr/tmp/testswparse1.out
	if [ $? -ne 0 ]; then
		echo "test006: error 4 in $rpmfile " 1>&2
		exit 4
	fi



done

rm -f /usr/tmp/testswparse[0-1].out /usr/tmp/testswspsf0.out


