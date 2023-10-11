

while read rpmfile
do
	echo $rpmfile
	
	
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf | ../devel/testheader | ../../swprogs/swbisparse --psf  >/usr/tmp/aaa.1
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf | ../../swprogs/swbisparse --psf  >/usr/tmp/aaa.2
	
	diff -u /usr/tmp/aaa.1 /usr/tmp/aaa.2
	if [ $? -ne 0 ]; then
		echo "test003: error in $rpmfile " 1>&2
		exit 2
	fi

done

rm /usr/tmp/aaa.1
rm /usr/tmp/aaa.2


