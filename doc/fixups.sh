
case "$1" in
	swbis_swpackage*)
	# -rw-r----- root/root     33 2003-06-03 ... catalog/dfiles/adjunct@t{_}md5sum
	# -rw-r----- root/root    512 2003-06-03 ... catalog/dfiles/sig@t{_}header

	sed \
		-e 's/sig@t{_}header/sig_header/' \
		-e 's/adjunct@t{_}md5sum/adjunct_md5sum/'

		;;
	*)
		cat
		;;
esac


