#!/bin/sh
#
# Test Prorgam for the installed software catalog selection of dependencies from
# an installed software catalog.  This script is similar to what is generated
# by the sw<utilty> for dependency checks and selections.

echo "Examples of the software selection queries and responses"
echo

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
echo export SW_SOC_SPEC
echo export SWBIS_LIST_FORM

echo "cd \"\$TARGETPATH\"/\"\$ISC_PATH\" || exit 121"

# echo SWBIS_LIST_FORM=lform_dir1
# echo SWBIS_LIST_FORM=lform_dep1
# echo SWBIS_LIST_FORM=lform_p1
SWBIS_LIST_FORM=${SWBIS_LIST_FORM:="lform_p1"}
echo SWBIS_LIST_FORM="$SWBIS_LIST_FORM"

echo SW_SOC_SPEC=units
echo SWBIS_SOC_SPEC1="\"units\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"==\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=0
echo "shls_get_vendor_tag_list2 | shls_apply_socspec"

echo SW_SOC_SPEC="\"swbis,r>0.600\""
echo SWBIS_SOC_SPEC1="\"swbis\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\"0.600\""
echo SWBIS_REVISION_RELOP="\">\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=1
echo "shls_get_vendor_tag_list2 | shls_apply_socspec"

echo SW_SOC_SPEC="\"swbis,r<=1\""
echo SWBIS_SOC_SPEC1="\"swbis\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=2
echo "shls_get_vendor_tag_list2  | shls_apply_socspec"

echo SW_SOC_SPEC="\"*\""
echo SWBIS_SOC_SPEC1="\".*\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"==\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=3
echo "shls_get_vendor_tag_list2 | shls_apply_socspec"

echo SW_SOC_SPEC="\"sw,r<=1\""
echo SWBIS_SOC_SPEC1="\"^sw$\""
echo SWBIS_SOC_SPEC2=""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=4
echo "shls_get_vendor_tag_list2  | shls_apply_socspec"

echo SW_SOC_SPEC="\"sw.swbis,r<=1\""
echo SWBIS_SOC_SPEC1="\"sw\""
echo SWBIS_SOC_SPEC2="\"swbis\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=5
echo "shls_get_vendor_tag_list2  | shls_apply_socspec"

echo SW_SOC_SPEC="\"swbis.fileset_tag,r<=1\""
echo SWBIS_SOC_SPEC1="\"^swbis$\""
echo SWBIS_SOC_SPEC2="\"fileset_tag\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=6
echo "shls_get_vendor_tag_list2  | shls_apply_socspec"

echo SW_SOC_SPEC="\"none.and_none\""
echo SWBIS_SOC_SPEC1="\"^none$\""
echo SWBIS_SOC_SPEC2="\"andnone\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"==\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=7
echo "shls_get_vendor_tag_list2  | shls_apply_socspec"

echo SW_SOC_SPEC="\"*,q=exp\""
echo SWBIS_SOC_SPEC1="\".*\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\".*\""
echo SWBIS_QUALIFIER_SPEC="\"^exp$\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=8
echo "shls_get_verid_list | shls_apply_socspec"

echo SW_SOC_SPEC="\"*,l=/unionfs\""
echo SWBIS_SOC_SPEC1="\".*\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\"/unionfs\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=9
echo "shls_get_verid_list | shls_apply_socspec"

echo SW_SOC_SPEC="\"*,l=/var/sys/unionfs\""
echo SWBIS_SOC_SPEC1="\".*\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\"^/var/sys/unionfs$\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=10
echo "shls_get_verid_list | shls_apply_socspec"

echo SW_SOC_SPEC="\"*,l=/\""
echo SWBIS_SOC_SPEC1="\".*\""
echo SWBIS_SOC_SPEC2="\".*\""
echo SWBIS_VENDOR_TAG="\".*\""
echo SWBIS_REVISION_SPEC="\".*\""
echo SWBIS_REVISION_RELOP="\"<\""
echo SWBIS_LOCATION_SPEC="\"^/$|^$\""
echo SWBIS_QUALIFIER_SPEC="\".*\""
echo SWBIS_INSTANCE="\".*\""
echo SWBIS_QUERY_NUM=10
echo "shls_get_verid_list | shls_apply_socspec"


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
