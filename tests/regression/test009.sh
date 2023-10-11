
TR0=test009.out
prefix=../testinputfiles


FILES="psf.5.1.1 psf.5.1.2"

for psffile in $FILES
do
	file=${prefix}/$psffile
	cat $file | ../../swprogs/swbisparse --psf -b  | diff $file -
	if [ $? -ne 0 ]; then exit 1; fi
	../../swprogs/swbisparse --psf -b  <$file | diff $file -
	if [ $? -ne 0 ]; then exit 1; fi
done
echo  "test009                                                    PASSED"

#rm -f  test009.out*
