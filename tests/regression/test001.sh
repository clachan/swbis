
while read rpmfile
do
	echo $rpmfile

#	../swsupplib/progs/rpm2swpsf <$rpmfile >/usr/tmp/78aaa 
#	  
#	../swsupplib/tests/testvarfs  \*/PSF  </usr/tmp/78aaa |
#	swbisparse --psf 1>/dev/null 
#
	../../swsupplib/progs/rpm2swpsf -a <$rpmfile | ../../swsupplib/tests/testvarfs \*/PSF | ../devel/testswspsf | ../../swprogs/swbisparse --psf 1>/dev/null 

	if [ $? -ne 0 ]; then
		echo " error in $rpmfile " 1>&2
		exit 2
	fi
done


