case "$1" in
	"")
		echo "Usage: sh reversion.sh NEW_VERSION" 1>&2
		exit 1;
		;;
	*)
		;;
esac
touch compile
version="$1"
set -vx
cat configure.ac |
sed -e "s/^AC_INIT(\\[swbis\\], \\[[^\\]*\\], \\[bug-swbis@gnu.org\\])/AC_INIT(\\[swbis\\], \\[$version\\], \\[bug-swbis@gnu.org\\])/" |
cat > configure.ac.new
diff configure.ac configure.ac.new
cat < configure.ac.new >configure.ac
rm configure.ac.new
rm aclocal.m4
aclocal
automake
echo "$version" >VERSION
aclocal && autoconf && automake --add-missing
rm compile
exit $?
