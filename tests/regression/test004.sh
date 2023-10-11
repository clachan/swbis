

while read rpmfile
do
	echo $rpmfile
	
	
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf | ../devel/testheader | ../../swprogs/swbisparse --psf >/usr/tmp/testheader.out
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf | ../devel/testswspsf | ../../swprogs/swbisparse --psf >/usr/tmp/testswspsf.out
	
	diff -u /usr/tmp/testheader.out /usr/tmp/testswspsf.out	
	if [ $? -ne 0 ]; then
		echo "test004: error in $rpmfile " 1>&2
		exit 1
	fi
	
done

rm -f /usr/tmp/testheader.out
rm -f /usr/tmp/testswspsf.out


