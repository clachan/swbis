
while read rpmfile
do
	printf "$rpmfile "
	
	rpmlines="`rpm2cpio  $rpmfile | cpio -itv 2>/dev/null | wc -l`"
	echo "$rpmlines"
	
	lxpsflines1="`cat $rpmfile | ../../swsupplib/progs/lxpsf | dd bs=10240 2>/dev/null |  cpio -itv 2>/dev/null | \
	grep -v -e /PSF -e /unpostinstall -e /unpreinstall -e /preinstall -e /postinstall | wc -l`"

	if [ $rpmlines -ne $lxpsflines1 ]; then
		echo "lxpsflines1 failed: $lxpsflines1" 1>&2
		exit 3
	fi

	lxpsflines2="`../../swsupplib/progs/lxpsf $rpmfile | dd bs=10240 2>/dev/null | cpio -itv 2>/dev/null | \
	grep -v -e /PSF -e /unpostinstall -e /unpreinstall -e /preinstall -e /postinstall | wc -l`"
	
	if [ $rpmlines -ne $lxpsflines2 ]; then
		echo "lxpsflines1 failed: $lxpsflines2" 1>&2
		exit 3
	fi
done

rm -f /usr/tmp/testswparse[0-1].out /usr/tmp/testswspsf0.out


