
while read rpmfile
do
	printf "$rpmfile "
	rm -fr foodir
	mkdir foodir
	rpm2cpio $rpmfile | (cd foodir && cpio -idum 2>/dev/null)
	rpmsum="`(cd foodir && tar cf - *) | tar xf - -O | md5sum`"
	echo "$rpmsum"

	rm -fr foodir
	mkdir foodir
	../../swsupplib/progs/lxpsf <$rpmfile | dd bs=10240 2>/dev/null | (cd foodir && cpio -idum 2>/dev/null)
	lxpsfsum="`(cd foodir && tar cf - --exclude PSF --exclude unpreinstall --exclude unpostinstall --exclude postinstall --exclude preinstall *) | tar xf - -O | md5sum`"
	
	if [ "$rpmsum" != "$lxpsfsum" ]; then
		echo "1: sums failed $lxpsfsum" 1>&2
		rm -fr foodir
		exit 3
	fi

	rm -fr foodir
	mkdir foodir
	cat $rpmfile | ../../swsupplib/progs/lxpsf |  dd bs=10240 2>/dev/null  | (cd foodir && cpio -idum 2>/dev/null)
	lxpsfsum="`(cd foodir && tar cf - --exclude PSF --exclude unpreinstall --exclude unpostinstall --exclude postinstall --exclude preinstall *) | tar xf - -O | md5sum`"
	
	if [ "$rpmsum" != "$lxpsfsum" ]; then
		echo "2: sums failed $lxpsfsum" 1>&2
		rm -fr foodir
		exit 4
	fi

	rm -fr foodir
	mkdir foodir
	cat $rpmfile | ../../swsupplib/progs/lxpsf --format=ustar | dd bs=10240 2>/dev/null | (cd foodir && cpio -idum 2>/dev/null)
	lxpsfsum="`(cd foodir && tar cf - --exclude PSF --exclude unpreinstall --exclude unpostinstall --exclude postinstall --exclude preinstall *) | tar xf - -O | md5sum`"
	
	if [ "$rpmsum" != "$lxpsfsum" ]; then
		echo "3: sums failed $lxpsfsum" 1>&2
		rm -fr foodir
		exit 5
	fi

done

rm -f /usr/tmp/testswparse[0-1].out /usr/tmp/testswspsf0.out
rm -fr foodir


