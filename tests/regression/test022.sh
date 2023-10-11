
testset="\
../testinputfiles/psf.7.1.1 \
../testinputfiles/psf.7.1.2 \
../testinputfiles/psf.6.1.3 \
../testinputfiles/psf.6.1.4 \
../testinputfiles/psf.6.1.8 \
../testinputfiles/psf.swbis.1.2.1.1 \
../testinputfiles/psf.swbis.1.2.1.2
"
if [ ! -e ../devel/testpsf4 ]; then
	echo "../devel/testpsf4 not found" 1>&2
	exit 1
fi

for ifile in $testset
do
	echo -e "write size test 1: input file: $ifile: \c"
	writesize="`../devel/testpsf4 -x -p $ifile -d / 2>&1 1>/dev/null`"
	measuredwritesize="`../devel/testpsf4 -x -p $ifile -d / 2>/dev/null | wc -c | sed -e 's/  *//g'`"
	if [ "$writesize" -ne "$measuredwritesize"  ]; then
		echo "       FAILED."
		echo "written size = $writesize ;   measured size = $measuredwritesize" 1>&2
		exit 2
	else
		echo "  PASSED."
	fi
done

for ifile in $testset
do
	echo -e "swexdist test 1: input file: $ifile: \c"
	writesize="`../devel/testpsf4 -p $ifile -d / 2>/dev/null | cksum`"
	writesizex="`../devel/testpsf4 -x -p $ifile -d / 2>/dev/null | cksum`"
	if [ "$writesize" != "$writesizex"  ]; then
		echo "       FAILED."
		echo "cksum1 = $writesize ;   cksum2 = $writesizex" 1>&2
		exit 2
	else
		echo "  PASSED."
	fi
done





exit 0
