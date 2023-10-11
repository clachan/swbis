#
trap '/bin/rm -fr /var/tmp/swbis-test-user$$; exit 1' 1 2 15

#
# This test simulates a md5(sha1) failure and a adjunct_md5 success 
# which trips the execution of the vendor checksig script.
#

PATH=`pwd`/swprogs:`pwd`/swsupplib/progs:$PATH
export PATH

opt_format=ustar
case $# in 1) opt_format="$1"; ;; esac

TOUCH=`which touch`

. tests/regression/testlib.sh

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
echo

#
# make a sym link and simulate a md5
# failure to force checking of the adjunct md5 and
# execution of the checksig script.
#

check_for_gnu_tar
make_format_option

echo $0: Using swpackage at `which swpackage`
echo $0: Using swverify at `which swverify`
echo $0: Using GNU tar version $gnu_tar_version
echo $0: GNUTAR_FORMAT_OPTIONS=$GNUTAR_FORMAT_OPTIONS
echo $0: SWPACKAGE_FORMAT_OPTIONS=$SWPACKAGE_FORMAT_OPTIONS

create_date=`date "+%Y%m%d%H%M.%S"`

(rm -f tmp/xx; cd tmp; ln -s ../swprogs/xx xx)

${TOUCH} -t $create_date tmp

(
	export SWPACKAGEPASSFD=3
	exec 3<$PASSFILE swign -s . -u "$AUTHOR" -D bin/checkdigest.sh -S
)

(rm -f tmp/xx; cd tmp; ln -s ../swprogs/xx xx)
${TOUCH} -t $create_date tmp
sleep 1
swverify --scm -d @.
ret=$?
rm -fr /var/tmp/swbis-test-user$$
exit $ret
