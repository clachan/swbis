#!/bin/sh

PATH=/bin/:/usr/bin:/sbin/:/usr/sbin:$PATH

case "$SW_CONTROL_TAG"  in
	configure)
			echo "swbis configure: writing to stderr" 1>&2
			# echo "swbis configure: writing to stdout"
			exit 0
			;;
esac

l_install_info() {
	install-info "$1" "$2"/dir
}

install-info --version | grep "GNU texinfo" 1>/dev/null

case "$?" in
        0)
                ;;
        *)
                echo "install-info not found or unsupported version" 1>&2
                exit 0
                ;;
esac

# echo "hello from postinstall 1"
printf "Installing the info pages ..." 1>&2

MY_INSTALL_INFO=l_install_info
DESTDIR=""
infodir=/usr/share/info

ret=0;
${MY_INSTALL_INFO} $infodir/swbis.info ${DESTDIR}${infodir} || ret=1; 
${MY_INSTALL_INFO} $infodir/swbis_swpackage.info ${DESTDIR}${infodir} || ret=1;
${MY_INSTALL_INFO} $infodir/swbis_swinstall.info ${DESTDIR}${infodir} || ret=1;
${MY_INSTALL_INFO} $infodir/swbis_swverify.info ${DESTDIR}${infodir} || ret=1;
${MY_INSTALL_INFO} $infodir/swbis_swcopy.info ${DESTDIR}${infodir} || ret=1;
${MY_INSTALL_INFO} $infodir/swbis_swlist.info ${DESTDIR}${infodir} || ret=1;
${MY_INSTALL_INFO} $infodir/swbis_swign.info ${DESTDIR}${infodir} || ret=1;

printf "Done \n" 1>&2

exit $ret
