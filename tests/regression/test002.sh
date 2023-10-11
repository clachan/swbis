
while read rpmfile
do
	echo $rpmfile
	
	
	cat $rpmfile | ../../swsupplib/progs/rpm2swpsf --with-archive >/usr/tmp/aaa
	
	
	../../swsupplib/tests/testvarfs \*/PSF </usr/tmp/aaa >/usr/tmp/bbb
	
	
	../devel/testswspsf </usr/tmp/bbb | ../../swprogs/swbisparse --psf 1>/dev/null 
	
	
	if [ $? -ne 0 ]; then
		echo "test002: error in $rpmfile " 1>&2
		exit 2
	fi

done

rm /usr/tmp/aaa
rm /usr/tmp/bbb


