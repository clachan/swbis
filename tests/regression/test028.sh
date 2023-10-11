#
trap '/bin/rm -fr /var/tmp/swbis-test-user$$; exit 1' 1 2 15

PATH=`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH

. tests/regression/testlib.sh

# bin/testforgnutar.sh || exit 2
bin/testforgnupg.sh || exit 2

opt_format=ustar
case $# in 1) opt_format="$1"; ;; esac

TMPGNUPGDIR=/var/tmp/swbis-test-user$$/gnupg
PASSPHRASE=Iforgot
AUTHOR="Test User"
GNUPGHOME="$TMPGNUPGDIR"
export GNUPGHOME
bin/gpgme gpg "$GNUPGHOME" 
PASSFILE="$GNUPGHOME"/source/passphrase

echo "Your Identity: $AUTHOR"
echo "Your GPG homedir : $GNUPGHOME"
echo "Your passphrase is :  $PASSPHRASE"
echo $0: Using swpackage at `which swpackage`
echo $0: Using swcopy at `which swcopy`
echo $0: Using swign at `which swign`
echo

(
	export SWPACKAGEPASSFD=3
	exec 3<$PASSFILE swign -s . -u "$AUTHOR" -D bin/checksig.sh @-
) | swcopy -Waudit -s - @- | swverify --checksig

ret=$?
rm -fr /var/tmp/swbis-test-user$$
exit $ret
