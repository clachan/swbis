
PATH=/sbin/:/usr/sbin:$PATH

install-info --version | grep "GNU texinfo" 1>/dev/null

case "$?" in
	0)
		;;
	*)
		echo "install-info not found or unsupported version" 1>&2
		exit 0
		;;
esac

install-info "$1" "$2"/dir
