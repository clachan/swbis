
TR0=test008.out
prefix=../testinputfiles

retval=0

#
# Test set 1

FILES="psf.skel psf1 psf2 psf3 psf4.0 psf4.1 psf4.1.1 psf4.1.2 psf4.1.3"

#
# Test set 1

testset=S1
>${TR0}.${testset}
for psffile in $FILES
do
	file=${prefix}/$psffile
	printf "$testset $file: " >> ${TR0}.$testset
	cat $file | ../../swprogs/swbisparse --psf >> ${TR0}.$testset
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++ >> ${TR0}.$testset
done
diff ${TR0}.$testset baseline/${TR0}.${testset}
retval="`expr $retval + $?`"

#
# Test set 2

testset=S2
>${TR0}.${testset}
for psffile in $FILES
do
	file=${prefix}/$psffile
	printf "$testset $file: " >> ${TR0}.${testset}
	cat $file | ../../swprogs/swbisparse -b --psf >> ${TR0}.${testset}
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++ >> ${TR0}.$testset
done
diff ${TR0}.$testset baseline/${TR0}.${testset}
retval="`expr $retval + $?`"

#
# Test set 3

testset=S3
>${TR0}.${testset}
for psffile in $FILES
do
	file=${prefix}/$psffile
	printf "$testset $file: " >> ${TR0}.${testset}
	cat $file | ../../swprogs/swbisparse -n --psf >> ${TR0}.${testset}
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++ >> ${TR0}.$testset
done
diff ${TR0}.$testset baseline/${TR0}.${testset}
retval="`expr $retval + $?`"

#
# Test set 4

testset=S4
>${TR0}.${testset}
for psffile in $FILES
do
	file=${prefix}/$psffile
	printf "$testset $file: " >> ${TR0}.${testset}
	../../swprogs/swbisparse --psf $file >> ${TR0}.${testset}
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++ >> ${TR0}.$testset
done
diff ${TR0}.$testset baseline/${TR0}.${testset}
retval="`expr $retval + $?`"

#
# Test set 5

testset=S5
>${TR0}.${testset}
for psffile in $FILES
do
	file=${prefix}/$psffile
	printf "$testset $file: " >> ${TR0}.${testset}
	../../swprogs/swbisparse --psf -b $file  >> ${TR0}.${testset}
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++ >> ${TR0}.$testset
done
diff ${TR0}.$testset baseline/${TR0}.${testset}
retval="`expr $retval + $?`"

#
# Test set 6

testset=S6
>${TR0}.${testset}
for psffile in $FILES
do
	file=${prefix}/$psffile
	printf "$testset $file: " >> ${TR0}.${testset}
	../../swprogs/swbisparse --psf -n $file  >> ${TR0}.${testset}
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++ >> ${TR0}.$testset
done
diff ${TR0}.$testset baseline/${TR0}.${testset}
retval="`expr $retval + $?`"

if [ $retval -ne 0 ]; then
	rm -f  test008.out*
	exit 1
else
	echo  "test008                                                    PASSED"
	rm -f  test008.out*
	exit 0
fi
rm -f  test008.out*

