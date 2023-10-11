#!/bin/sh

TMPFILE=/usr/tmp/test016.tmp
#../testinputfiles/testpath.12 \
#../testinputfiles/testpath.13 \
#../testinputfiles/testpath.14

for ifile in \
../testinputfiles/testpath.1 \
../testinputfiles/testpath.2 \
../testinputfiles/testpath.3 \
../testinputfiles/testpath.4 \
../testinputfiles/testpath.5 \
../testinputfiles/testpath.6 \
../testinputfiles/testpath.7 \
../testinputfiles/testpath.8 \
../testinputfiles/testpath.12 \
../testinputfiles/testpath.13 \
../testinputfiles/testpath.14
do
	sed -e 's/\/$//' $ifile >$TMPFILE
	result="`../devel/testpathr <$ifile | diff - $TMPFILE`"
	if [ "$result" ]; then
		/bin/rm -f $TMPFILE
		echo "testpathr test: $ifile failed."
		echo "$result"
		exit 2
	else
		echo "testpathr test: $ifile               PASSED."
	fi
done
/bin/rm -f $TMPFILE
#../testinputfiles/testpath.12 \
#../testinputfiles/testpath.13 \
#../testinputfiles/testpath.14

for ifile in \
../testinputfiles/testpath.1 \
../testinputfiles/testpath.2 \
../testinputfiles/testpath.3 \
../testinputfiles/testpath.4 \
../testinputfiles/testpath.5 \
../testinputfiles/testpath.6 \
../testinputfiles/testpath.7 \
../testinputfiles/testpath.8 \
../testinputfiles/testpath.12 \
../testinputfiles/testpath.13 \
../testinputfiles/testpath.14
do
	base="`basename $ifile`"
	baseline="baseline/$base.out"
	result="`../../swsupplib/tests/testswpath <$ifile | diff - $baseline`"
	if [ "$result" ]; then
		echo "testswpath test: $ifile failed."
		echo "$result"
		exit 2
	else
		echo "testswpath test: $ifile                PASSED."
	fi
done

