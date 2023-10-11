

#../testfiles/psf.6.1.3 \
#../testfiles/psf.6.1.4 \
#../testfiles/psf.6.1.8 \

testset="\
../testinputfiles/psf.7.1.1 \
../testinputfiles/psf.7.1.2 \
../testinputfiles/psf.swbis.1.2.1.1 \
../testinputfiles/psf.swbis.1.2.1.2 \
../testinputfiles/psf.swbis.1.2.1.3 \
../testinputfiles/psf.swbis.1.2.1.4 \
../testinputfiles/psf.swbis.1.2.1.5
"
if [ ! -e ../devel/testpsf4 ]; then
	echo "../devel/testpsf4 not found" 1>&2
	exit 1
fi


for ifile in $testset
do
	printf "swexdist test 1: input file: $ifile:   "
	writesize="`../../swprogs/swpackage -W list-psf 2>/dev/null <$ifile | cksum`"
	writesizex="`../../swprogs/swbisparse --psf -b 2>/dev/null <$ifile | cksum`"
	if [ "$writesize" != "$writesizex"  ]; then
		echo "       FAILED."
		echo "cksum1 = $writesize ;   cksum2 = $writesizex" 1>&2
		exit 2
	else
		echo "       PASSED."
	fi
done


exit 0
