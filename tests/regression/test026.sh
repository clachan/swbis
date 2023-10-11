#
trap '/bin/rm -fr /var/tmp/swbis-test-user$$ tmp/swpackage; exit 1' 1 2 15

PATH=`pwd`/tmp:`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH

export GPG_TTY=tty

which runswpackagetest 1>/dev/null || \
(echo; echo "need tmp/runswpackagetest, compile then try again"; echo; exit 2) || exit 2

bin/testforgnutar.sh || exit 2
bin/testforgnupg.sh || exit 2
ln -s ../swprogs/runswpackagetest tmp/swpackage

echo
ls `pwd`/tmp/swpackage 1>&2
echo "$0 : Using `which swpackage`" 1>&2
echo

TMPGNUPGDIR=/var/tmp/swbis-test-user$$/gnupg
PASSPHRASE=Iforgot
AUTHOR="Test User"
GNUPGHOME="$TMPGNUPGDIR"
export GNUPGHOME
bin/gpgme gpg "$GNUPGHOME"
PASSFILE="$TMPGNUPGDIR"/passphrase

echo "Your Identity: $AUTHOR"
echo "Your GPG homedir : $GNUPGHOME"
echo $0: Using swpackage at `which swpackage`
echo $0: Using swverify at `which swverify`
echo $0: Using swign at `which swign`

PASSPHRASE="willbe delivered by /runswpackagetest"
AUTHOR="doesntwillbe overridden by /runswpackagetest"

swign -s . -u "$AUTHOR" @- | swverify -d @-
ret=$?
rm -fr /var/tmp/swbis-test-user$$ tmp/swpackage
exit $ret
