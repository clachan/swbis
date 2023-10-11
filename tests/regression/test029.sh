#
trap '/bin/rm -fr /var/tmp/swbis-test-user$$; exit 1' 1 2 15

#
# This test verifies immediately after signing
#

PATH=`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH

TOUCH=`which touch`

bin/testforgnutar.sh || exit 2
bin/testforgnupg.sh || exit 2

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
echo $0: Using swverify at `which swverify`
echo $0: Using swign at `which swign`
echo
(
	export SWPACKAGEPASSFD=3
	exec 3<$PASSFILE swign -s .  -u "$AUTHOR" -D bin/checkdigest.sh -S
)
swverify --scm -d @.
ret=$?
rm -fr /var/tmp/swbis-test-user$$
exit $ret
