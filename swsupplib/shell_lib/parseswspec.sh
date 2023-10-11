# parseswspec.sh
#

spec="$1"

case "$spec" in
	*[\`\'\"]*)
		echo "illegal swspec [$1]"
		exit 1
		;;
esac


. shell_lib.sh

# This script requires a pre 2007-04-25 version of shell_lib.sh
echo "This script requires a pre 2007-04-25 version of shell_lib.sh" 1>&2
exit 1


arg1="$1"
remspec="$1"
delim="$2"

do_stop=""
c=0
MAX_TOKENS=20

shls_id_cur_token "$remspec"
cursep="$gg_id_cur_sep"
shls_sh_brk_split "$remspec" "$cursep"
curspec="$gg_ab_fir"
remspec="$gg_ab_sec"
gg_id_cur_token="SX_newspec"
curtoken_type=$gg_id_cur_token

PARSED_SPEC=""
while :
do
	c=$(($c+1))
	case $(($MAX_TOKENS-$c)) in
		0)
			#
			# This is to prevent infinite loop
			#
			echo $c
			break
			;;
	esac

	case "$curtoken_type" in
		SX_newspec)
			shls_swspec_init
			PARSED_SPEC="tag=$curspec"
			;;
		SX_tag)
			case "$curspec" in "") ;; *)
			PARSED_SPEC="$PARSED_SPEC\ntag=$curspec"
			;; esac
			;;
		"SX_ver_item")
			PARSED_SPEC="$PARSED_SPEC\nver_id=$curspec"
			;;			
		*)
			echo "error: $curtoken_type" 1>&2
			;;
	esac
	shls_id_cur_token "$remspec"
	curtoken_type="$gg_id_cur_token"
	cursep="$gg_id_cur_sep"
	shls_sh_brk_split "$remspec" "$cursep"
	curspec="$gg_ab_fir"
	remspec="$gg_ab_sec"
	case "$do_stop" in yes) break; ;; esac
	case "$remspec" in
		"")
			do_stop=yes
			;;
	esac	
done

echo -e "$PARSED_SPEC"

case "$c" in
	$MAX_TOKENS)
		echo parse error for \""$arg1\"" 1>&2
		;;
esac
exit 0

