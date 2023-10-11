#
# Example control script to compile the swbis package
# as the postinstall and configure scripts
#

SW_ROOT_DIRECTORY="${SW_ROOT_DIRECTORY:?unset fatal}"
SW_CONTROL_DIRECTORY="${SW_CONTROL_DIRECTORY:?unset fatal}"
SW_LOCATION="${SW_LOCATION:?unset fatal}"
SW_SESSION_OPTIONS="${SW_SESSION_OPTIONS:?unset fatal}"
SW_CATALOG="${SW_CATALOG:?unset fatal}"
SW_SOFTWARE_SPEC="${SW_SOFTWARE_SPEC:?unset fatal}"
SW_CONTROL_TAG="${SW_CONTROL_TAG:?unset fatal}"

cd "$SW_ROOT_DIRECTORY"/"$SW_LOCATION" || exit 4

SWBB_BUILD=build
SWBB_RUNTIME=run

{
ret=0
case "$SW_CONTROL_TAG" in
	postinstall)
			./configure --prefix="$SW_ROOT_DIRECTORY"/"$SWBB_RUNTIME" && make 
			ret=$?
			;;
	configure)
			mkdir -p "$SW_ROOT_DIRECTORY"/"$SWBB_RUNTIME"
			make install 
			ret=$?
			;;
	*)		exit 0
			;;
esac
exit $ret
} 1>&2
exit $?
