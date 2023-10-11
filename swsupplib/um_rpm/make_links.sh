#!/bin/sh

/bin/rm -f dbindex.h 2>/dev/null
/bin/rm -f popt.h 2>/dev/null
/bin/rm -f header.h 2>/dev/null
/bin/rm -f rpmio.h 2>/dev/null
/bin/rm -f self_dbindex.h 2>/dev/null
/bin/rm -f self_header.h 2>/dev/null
/bin/rm -f self_rpmio.h 2>/dev/null
/bin/rm -f self_rpmlib.h 2>/dev/null
/bin/rm -f self_rpmts.h 2>/dev/null

case "$1" in
	"remove")
		exit 0
		;;
esac

ln -s rpm-3.0.5/lib/dbindex.h dbindex.h
ln -s rpm-3.0.5/popt/popt.h popt.h
ln -s rpm-3.0.5/lib/header.h  header.h
ln -s rpm-3.0.5/lib/dbindex.h  self_dbindex.h
ln -s rpm-3.0.5/lib/header.h  self_header.h
ln -s rpm-3.0.5/lib/rpmio.h  rpmio.h
ln -s rpm-3.0.5/lib/rpmio.h  self_rpmio.h
ln -s rpm-3.0.5/lib/rpmlib.h  self_rpmlib.h
ln -s rpm-3.0.5/lib/rpmts.h  self_rpmts.h

exit 0
