#
# Example control directory using the Enviroment created by 
# a conforming 1387.2 utility.
#

SW_ROOT_DIRECTORY="${SW_ROOT_DIRECTORY:?unset fatal}"
SW_CONTROL_DIRECTORY="${SW_CONTROL_DIRECTORY:?unset fatal}"
SW_LOCATION="${SW_LOCATION:?unset fatal}"
SW_SESSION_OPTIONS="${SW_SESSION_OPTIONS:?unset fatal}"
SW_CATALOG="${SW_CATALOG:?unset fatal}"
SW_SOFTWARE_SPEC="${SW_SOFTWARE_SPEC:?unset fatal}"
SW_CONTROL_TAG="${SW_CONTROL_TAG:?unset fatal}"

cd "$SW_ROOT_DIRECTORY"/"$SW_LOCATION" || exit 4

#-----------------------------------#
#-----------------------------------#
# Everything above is general
#-----------------------------------#
#-----------------------------------#
# Everything below is script specific
#-----------------------------------#
#-----------------------------------#
# Redirect stdout to stderr, stdout.
{
	# Your code goes here
} 1>&2
exit $?
