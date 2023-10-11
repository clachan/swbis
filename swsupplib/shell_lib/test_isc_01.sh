#!/bin/sh
#
# Test Prorgam for the installed software catalog selection

PATH=`getconf PATH`
export POSIXSHELL
POSIXSHELL=${POSIXSHELL:=sh}

case "$MAKE" in "") MAKE=make; ;; esac

cwdir=`pwd`
relroot=/
cd ../.. || exit 1
relroot=`pwd`
cd "$relroot"/tests/examples/installed_software || exit 2
$MAKE unpack 1>/dev/null || exit 3
cd $cwdir || exit 4

export TARGETPATH
export ISC_PATH
TARGETPATH="$relroot"/tests/examples/installed_software
ISC_PATH=catalog

export SWBIS_SOC_SPEC1
export SWBIS_SOC_SPEC2
export SWBIS_VENDOR_TAG
export SWBIS_REVISION_SPEC
export SWBIS_REVISION_RELOP
export SWBIS_STATE
export SWBIS_LIST_FORM

soc_spec1=".*"
soc_spec2=".*"
vendor_tag=".*"
revision_spec=".*"
revision_relop="=="
state="0"

while [ $# -gt 0 ]
do
    case "$1" in
        --soc-spec1=*)
			optarg=${1#*=}	
			soc_spec1="$optarg"
			;;
        --soc-spec2=*)
			optarg=${1#*=}	
			soc_spec2="$optarg"
			;;
        --vendor-tag=*)
			optarg=${1#*=}	
			vendor_tag="$optarg"
			;;
        --revision=*)
			optarg=${1#*=}	
			revision_spec="$optarg"
			;;
        --relop=*)
			optarg=${1#*=}	
			revision_relop="$optarg"
			;;
        --state=*)
			optarg=${1#*=}	
			state="$optarg"
			;;

	-*) echo >&2 \
	    "usage: $0 [--soc-spec1=soc]  [--soc-spec2=soc] [--vendor-tag=tag] [--revision=rev] [--relop=relop] [--state={0|1|2}]"
	    exit 1;;
	*)  break;;	# terminate while loop
    esac
    shift
done

SWBIS_SOC_SPEC1=${soc_spec1}
SWBIS_SOC_SPEC2=${soc_spec2}
SWBIS_VENDOR_TAG=${vendor_tag}
SWBIS_REVISION_SPEC=${revision_spec}
SWBIS_REVISION_RELOP=${revision_relop}
SWBIS_STATE=${state}
SWBIS_LIST_FORM=lform_p1

(
./test_write
echo "PATH=`getconf PATH`:\"$PATH\""
echo "cd \"$TARGETPATH\"/\"$ISC_PATH\" || exit 121"
echo "shls_get_vendor_tag_list2 | shls_apply_socspec"
) 2>test_script.error | tee test_script.out | $POSIXSHELL -s 
ret=$?

errormsg="`cat test_script.error`"
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
