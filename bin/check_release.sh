#!/bin/sh

# Check to make sure the include/swprog_versions.h file
# has been updated.




PROJECT_NAME="swbis[^/]*"

# Get the package path.
	if [ "$1" ]; then
		base1="$1"
	else
		base1=`pwd`
	fi
	
	package_path=`echo "$base1" |
		sed -e s@'\(.*'"${PROJECT_NAME}"'\).*@\1@'` 

	package_dir=`echo "$base1" |
		sed -e s@'.*/\('"${PROJECT_NAME}"'\)@\1@' |
		sed -e s@'\('"${PROJECT_NAME}"'\).*@\1@'`


	echo "$package_dir" | grep / 1>/dev/null
	case $? in
		0)
			exit 12
	esac


	echo "$package_dir" | grep . 1>/dev/null
	case $? in
		1)
			exit 11
	esac

cd "$package_path"
	case $? in
		0)
			;;
		*)
			exit 10
			;;
	esac

release=`\
cat include/swprog_versions.h | 
	grep SWBIS_RELEASE |
	head -1 |
	sed -e 's/^.*SWBIS_RELEASE[^0-9]*\([0-9].*\)/\1/' \
		-e 's/.$//'`

#echo "release = $release"

if [ "swbis-$release" != "$package_dir" ]; then
	exit 1
fi	
exit 0
