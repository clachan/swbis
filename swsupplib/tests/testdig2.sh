#
trap '/bin/rm -f ./testdig.test.*; exit 1' 1 2 15

echo "This test requires /dev/urandom and openssl utilities."

runshorttest=""
arg1="$1"
case "$arg1" in
	--short)
		runshorttest="1"
		;;
esac

#PATH=`getconf PATH`:$PATH
#export PATH


ECHO=`which echo`
case "$ECHO" in
	""|"no *")
			exit 22;
			;;
esac


OPENSSL_ENGINE=`which openssl`
DEVURAN=/dev/urandom

if [ ! -c $DEVURAN ]; then
	echo "$DEVURAN does not exist, test not run." 1>&2
	exit 2
fi

if [ ! -f $OPENSSL_ENGINE ]; then
	echo "$OPENSSL_ENGINE does not exist, test not run." 1>&2
	exit 2
fi

echo "Found $OPENSSL_ENGINE , and $DEVURAN" 

sourcefile=./testdig.test.source.o
file=./testdig.test.data.o

echo "Creating source data file $sourcefile"
dd if=/dev/urandom bs=1024 count=100 2>/dev/null >$sourcefile

runtest() {
	bs=$1
	len=$2
	printf "Testing data sizes thru $len in $bs byte increments ..."
	size=0
	while [ $size  -lt $len ]
	do
		dd if=$sourcefile bs=$bs count=$size 2>/dev/null >$file

		goodsha1=`openssl sha1 <$file | sed -e s/^'(stdin)= '//`
		goodmd5=`openssl md5 <$file   | sed -e s/^'(stdin)= '//`

		mymd5=`./testdigests --no-name md5 $file`
		mysha1=`./testdigests --no-name sha1 $file`

		if [ "$goodsha1" != "$mysha1" ]; then
			echo "FAIL"
			echo "SHA1 ERROR [$file] good=$goodsha1  bad=$mysha1" 1>&2
			echo "The data file is $file" 1>&2
			echo "size = $size  bs = $bs" 1>&2
			exit 1
		fi

		if [ "$goodmd5" != "$mymd5" ]; then
			echo "FAIL"
			echo "MD5 ERROR [$file] good=$goodmd5  bad=$mymd5" 1>&2
			echo "The data file is $file" 1>&2
			echo "size = $size  bs = $bs" 1>&2
			exit 1
		fi
		#echo "$file bs=$bs $size of $len OK"
		size=`expr $size + $bs`
	done
	echo " PASS"
	return 0
}

if [ "$runshorttest" ]; then
runtest 1 10
runtest 63 200
runtest 64 256
runtest 512 2048
else
echo "Running test, this will take several minutes." 
runtest 1 64
runtest 8 8200
runtest 63 5200
runtest 64 5200
runtest 111 100000
runtest 512 100000
echo "This may take a while, so far so good"
runtest 1 8200
fi

/bin/rm -f ./testdig.test.*
exit 0

