

for ifile in \
../testinputfiles/testpath.9 \
../testinputfiles/testpath.10 \
../testinputfiles/testpath.11
do
	base="`basename $ifile`"
	baseline="baseline/$base.out"

	#../../swsupplib/tests/testswpath1 <$ifile >$baseline


	result="`../../swsupplib/tests/testswpath1 <$ifile | diff -u - $baseline`"
	if [ "$result" ]; then
		echo "testswpath test: $ifile failed."
		echo "$result"
		exit 2
	else
		echo "testswpath test: $ifile                PASSED."
	fi
done

