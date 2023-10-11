#

local_cleantmp()
{
	/bin/rm -f "$datafile"
	/bin/rm -f ../tmp/test021*
}




runfile()
{
	l_datafile="$1"

	fromcat=`cat $l_datafile | $testprog 2>../tmp/test021.xxa | cksum`
	fromfile=`$testprog <$l_datafile 2>../tmp/test021.xxb | cksum`

	xret=""
	xx=`cat ../tmp/test021.xxa`
	if [ "$xx" ]; then
		echo "FAILED at loc=1"
		echo "$xx" 1>&2
		xret=1
	fi

	xx=`cat ../tmp/test021.xxb`
	if [ "$xx" ]; then
		echo "FAILED at loc=2 [$xx]"
		xret=1
	fi

	if [ "$fromcat" != "$fromfile" ]; then
		echo "FAILED at loc=3 [$xx]"
	fi
	
	if [ "$xret" ]; then
		local_cleantmp
		return 3
	fi

	echo " PASSED"
	return 0
}

cd ../devel || exit 1

if [ -f  "/usr/bin/bzip2" ]; then
	BZIP2PATH=/usr/bin/
else
	BZIP2PATH=/usr/local/bin/
fi


datafile="../tmp/test021.data"
testprog=./testswpackagefile3
printf "test021: Running uncompressed tar test : "
tar cf -  ./*.cxx  >"$datafile"

runfile "$datafile"
case "$?" in
	0)
		;;
	*)
		local_cleantmp
		exit $?
		;;
esac

gzip "$datafile"
printf "test021: Running gzip compressed tar test : "
runfile "${datafile}.gz"
case "$?" in
	0)
		;;
	*)
		local_cleantmp
		exit $?
		;;
esac

gzip -d "${datafile}.gz"
"$BZIP2PATH"/bzip2 -1 "${datafile}"
printf "test021: Running bz2 compressed tar test : "
runfile "${datafile}.bz2"
case "$?" in
	0)
		;;
	*)
		local_cleantmp
		exit $?
		;;
esac

gzip "${datafile}".bz2
printf "test021: Running bz2.gz compound compressed tar test : "
runfile "${datafile}.bz2.gz"
case "$?" in
	0)
		;;
	*)
		local_cleantmp
		exit $?
		;;
esac



local_cleantmp
exit 0

