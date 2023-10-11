#
trap '/bin/rm -fr /var/tmp/swbis-test-user$$; exit 1' 1 2 15

PATH=`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH

bin/testforgnutar.sh || exit 2
bin/testforgnupg.sh || exit 2

PASSPHRASE=Iforgot
AUTHOR="Test User"
TMPGNUPGDIR=/var/tmp/swbis-test-user$$/gnupg

rm -fr "$TMPGNUPGDIR"
mkdir -p "$TMPGNUPGDIR"
chmod  700 "$TMPGNUPGDIR"
cp -p var/gnupg/* "$TMPGNUPGDIR"	
gpg_homedir="$TMPGNUPGDIR"
echo "Your Identity: $AUTHOR"
echo "Your GPG homedir : $gpg_homedir"
echo "Your passphrase is :  $PASSPHRASE"
echo

GNUPGHOME="$gpg_homedir"
export GNUPGHOME

swign -s . -u "$AUTHOR" -D bin/checkdigest.sh -S
swverify --scm -d @.
ret=$?
rm -fr /var/tmp/swbis-test-user$$
exit $ret
