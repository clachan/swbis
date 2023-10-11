#!/bin/sh

rm -fr foodir
newcsum="`find . | ../devel/filelist2tar newc | cksum`"
newcsum1="`find . | ../devel/filelist2tar newc | ../devel/pa2pa | cksum`"
if [ "$newcsum" != "$newcsum1" ]; then
	find . | ../devel/filelist2tar newc | rawdump >/usr/tmp/xx1
	find . | ../devel/filelist2tar newc | ../devel/pa2pa | rawdump >/usr/tmp/xx1pa
	echo diff /usr/tmp/xx1 /usr/tmp/xx1pa
	diff /usr/tmp/xx1 /usr/tmp/xx1pa
	#rm -fr /usr/tmp/xx1 /usr/tmp/xx1pa
        echo "s1: newc sums failed" 1>&2
        exit 1
fi

newcsum="`find . | ../devel/filelist2tar ustar | cksum`"
newcsum1="`find . | ../devel/filelist2tar ustar | ../devel/pa2pa | cksum`"
if [ "$newcsum" != "$newcsum1" ]; then
        echo "s2: ustar sums failed" 1>&2
        exit 2
fi

newcsum="`find . | ../../swsupplib/progs/list2tar -H ustar | cksum`"
newcsum1="`find . | ../../swsupplib/progs/list2tar -H ustar | ../devel/pa2pa | cksum`"
if [ "$newcsum" != "$newcsum1" ]; then
        echo "s2a: ustar sums failed" 1>&2
        exit 2
fi


while read rpmfile
do
        echo "$rpmfile "
#        cat $rpmfile | ../../swsupplib/progs/lxpsf >test013.out
#        newcsum="`cat test013.out | cksum`"
#        newcsum1="`cat test013.out | ../devel/pa2pa | cksum`"
#        if [ "$newcsum" != "$newcsum1" ]; then
#                echo "3: lxpsf native sums failed" 1>&2
#                exit 3
#        fi


#	echo $rpmfile
	cat $rpmfile | ../../swsupplib/progs/lxpsf -H ustar >test013.out
        newcsum="`cat test013.out | cksum`"
        newcsum1="`cat test013.out | ../devel/pa2pa | cksum`"
        if [ "$newcsum" != "$newcsum1" ]; then
                echo "4: lxpsf ustar sums failed" 1>&2
                #exit 4
        fi
done
rm -f test013.out

