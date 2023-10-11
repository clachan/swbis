
# Test the directory walking of swvarfs

TESTSOURCEDIR=../..

newcsum1="`find $TESTSOURCEDIR |  cksum`"
newcsum2="`../../swsupplib/tests/testvarfsdir $TESTSOURCEDIR | cat | sed -e 's@/$@@' | cat | cksum`"

printf "s0: testvarfsdir List Test  ../../swsupplib/testvarfsdir        "
if [ "$newcsum1" != "$newcsum2" ]; then
        echo "s0: List Test failed." 1>&2
	echo 'find $TESTSOURCEDIR sum'
	find $TESTSOURCEDIR |  sum
	echo 
	echo '../../swsupplib/tests/testvarfsdir $TESTSOURCEDIR sum'
	../../swsupplib/tests/testvarfsdir $TESTSOURCEDIR | sum
else
        echo "        PASSED."
fi

# Test the Reg File data output from the filesystem.
olddir=`pwd`
cd $TESTSOURCEDIR
testdir=./swsupplib
printf "s1: Output test (from directory)                             "
newcsum1="`find $testdir |  cpio -o -H ustar 2>/dev/null | tar xf - -O 2>/dev/null | cksum`"
newcsum2="`./swsupplib/tests/testvarfsdir -O $testdir | cat | dd 2>/dev/null | cksum`"
if [ "$newcsum1" != "$newcsum2" ]; then
        echo "FAIL" 1>&2
else
        echo "PASSED."
fi
cd $olddir


# Test the Reg File data output from a archive.
olddir=`pwd`
cd $TESTSOURCEDIR
testdir=./swsupplib
newcsum1="`find $testdir |  cpio -o -H ustar 2>/dev/null | tar xf - -O 2>/dev/null | cksum`"
cd $olddir

for type in ustar newc crc odc
do
	olddir=`pwd`
	cd $TESTSOURCEDIR
       	 printf "s2: Output test (from archive) Format=$type."
	
	if [ "$type" = 'odc' ]; then
		arcmd="./tests/devel/filelist2tar"
	else
		arcmd="cpio -o -H"
	fi

	newcsum2="`
		find $testdir |
		$arcmd $type 2>/dev/null | 
		./swsupplib/tests/testvarfsdir -O - |
		cat | dd 2>/dev/null | 
		cksum
	`"
	if [ "$newcsum1" != "$newcsum2" ]; then
       	 echo "                      FAIL" 1>&2
	else
       	 echo "                      PASSED."
	fi
	cd $olddir
done


