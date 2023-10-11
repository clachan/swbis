case "$1" in
	swbis_swbis)
	sed \
		-e 's/swbis.html#/swbis_swbis.html#/g'
		;;
	*)
sed \
	-e 's/#g_t_0028swbis_005f\(.[^_]*\)_0029/\1.html#/g' \
	-e 's/#_0028swbis_005f\(.[^_]*\)_0029/\1.html#/g' \
	-e 's/_003a//g' |
sed \
	-e 's/swpackage.html#"/swpackage_8.html"/' \
	-e 's/swinstall.html#"/swinstall_8.html"/' \
	-e 's/swcopy.html#"/swcopy_8.html"/' \
	-e 's/swverify.html#"/swverify_8.html"/' \
	-e 's/sw.html#"/sw_5.html"/' \
	-e 's/swign.html#"/swign_1.html"/' \
	-e 's/lxpsf.html#"/lxpsf_1.html"/'
;;
esac
