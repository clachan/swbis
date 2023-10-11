swanlib_ret_ver()
{
rem="$1"
gg_ret=""
case "$rem" in "") gg_ret="" ;; *) num="${rem%%.*}"
case "$rem" in *.*) rem="${rem#*.}" ;; *) rem="" ;; "") rem="" ;; esac
gg_ver="$rem"
gg_ret="$num"
;;
esac
}
component_compare()
{
cc_old=$1
cc_new=$2
gg_dig_ver=$cc_old
gg_dig_old=$cc_old
gg_dig_new=$cc_new
ans_old=0
ans_new=0
ans_suf_old=""
ans_suf_new=""
while [ "$gg_dig_new" -o "$gg_dig_old" ]; do
case "$gg_dig_old" in "") dig_old="" ;; *) swanlib_ret_dig $gg_dig_old; gg_dig_old=$gg_dig_ver; dig_old="$gg_dig_ret" ;; esac
case "$gg_dig_new" in "") dig_new="" ;; *) swanlib_ret_dig $gg_dig_new; gg_dig_new=$gg_dig_ver; dig_new="$gg_dig_ret" ;; esac
case "$dig_old" in [0-9]) ans_old=$(($ans_old * 10 + $dig_old)); ;; [a-z]|[A-Z]) ans_suf_old="${ans_suf_old}${dig_old}" ;; "") ;; esac
case "$dig_new" in [0-9]) ans_new=$(($ans_new * 10 + $dig_new)); ;; [a-z]|[A-Z]) ans_suf_new="${ans_suf_new}${dig_new}" ;; "") ;; esac
done
if [ "$ans_new" -a ! "$ans_old" ]; then
return 2
elif [ ! "$ans_new" -a "$ans_old" ]; then
return 1
elif [ ! "$ans_new" -a ! "$ans_old" ]; then
return 0
fi
cc_is_eq=$(($ans_new == $ans_old))
case "$cc_is_eq" in
0)
if [ "${ans_new}" -gt "${ans_old}" ]; then
return 2
elif [ "${ans_new}" -lt "${ans_old}" ]; then
return 1
else
return 0
fi	
;;
1)
if test "${ans_suf_new}" ">" "${ans_suf_old}" ; then
return 2
elif test "${ans_suf_new}" "<" "${ans_suf_old}" ; then
return 1
else
return 0
fi
;;
esac
}
swanlib_ret_dig()
{
rem="$1"
gg_dig_ret=""
doit=""
case "$rem" in
"") gg_dig_ret=""; ;;
?) num="${rem}"; rem=""; doit="x"; ;;
??) num="${rem%%?}"; doit="x"; ;;
???) num="${rem%%??}"; doit="x"; ;;
????) num="${rem%%???}"; doit="x"; ;;
?????) num="${rem%%????}"; doit="x"; ;;
??????) num="${rem%%?????}"; doit="x"; ;;
???????) num="${rem%%??????}"; doit="x"; ;;
????????) num="${rem%%???????}"; doit="x"; ;;
?????????) num="${rem%%????????}"; doit="x"; ;;
??????????) num="${rem%%?????????}"; doit="x"; ;;
???????????) num="${rem%%??????????}"; doit="x"; ;;
????????????) num="${rem%%???????????}"; doit="x"; ;;
*) echo "number too large"; doit=""; rem="";;
esac

case "$doit" in
"")
gg_dig_ver=""
gg_dig_ret=""
;;
*)
case "$rem" in
*)
rem="${rem#?}"
;;
"")
rem=""
;;
esac
gg_dig_ver="$rem"
gg_dig_ret="$num"
;;
esac
}
ver_compare()
{
old="$1"
new="$2"
gg_ver="$old"
gg_old="$old"
gg_new="$new"
while [ "$gg_new" -o "$gg_old" ]; do
swanlib_ret_ver "$gg_old"; gg_old="$gg_ver"
old_comp="$gg_ret"
swanlib_ret_ver "$gg_new"; gg_new="$gg_ver"
new_comp="$gg_ret"
#echo "$old_comp" "$new_comp"
component_compare "$old_comp" "$new_comp"
ret=$?
case "$ret" in
0)
;;
*)
return "$ret"
;;
esac
done
}
arg1="$1"
arg2="$2"
ver_compare $arg1 $arg2
ret=$?
case $ret in
1)  echo downdate ;;
2)  echo update  ;;
0)  echo exists   ;;
*)  echo error   ;;
esac
exit $ret
