#!/bin/sh
#
# Test Prorgam for the installed software catalog selection of dependencies from
# an installed software catalog.  This script ./test_script.out is similar to what
# is (would be) generated by the sw<utilty> for dependency checks and selections.

case "$MAKE" in "") MAKE=make; ;; esac

cwdir=`pwd`
relroot=/
cd ../.. || exit 1
relroot=`pwd`
cd "$relroot"/tests/examples/installed_software || exit 2
$MAKE unpack 1>/dev/null || exit 3
cd $cwdir || exit 4

(
echo "("

./test_write

echo export TARGETPATH="$relroot"/tests/examples/installed_software
echo export ISC_PATH=catalog
echo export SWBIS_SOC_SPEC1
echo export SWBIS_SOC_SPEC2
echo export SWBIS_VENDOR_TAG
echo export SWBIS_REVISION_SPEC
echo export SWBIS_REVISION_RELOP
echo export SWBIS_INSTANCE
echo export SWBIS_LOCATION_SPEC
echo export SWBIS_QUALIFIER_SPEC
echo export SWBIS_QUERY_NUM
echo export SWBIS_LIST_FORM

echo SWBIS_LIST_FORM=lform_p1

echo "cd \"\$TARGETPATH\"/\"\$ISC_PATH\" || exit 121"

echo SWBIS_SOC_SPEC1="\"units\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"==\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo "shls_get_vendor_tag_list2 | shls_apply_socspec"


echo ")"
) 2>test_script.error | tee test_script.out | sh -s 
ret=$?

errormsg="$(cat test_script.error)"
case "$errormsg" in
	"")
		;;
	*)
		echo "$0: errors" 1>&2
		echo "$errormsg" 1>&2  
		exit 9
		;;
esac
exit $ret
