#!/bin/sh
# Last Updated: 15 Sep 2007

# This script adjustes the hrefs of the yman2html formatted html
# so they point to the local file.

# <a href="http://low20.lowland.com/cgi-bin/yman2html?m=swverify&s=8"><b>swverify</b>(8)</a>
#  needs to become this
# <a href="swverify_8.html"><b>swverify</b>(8)</a>

sed -e 's@<a href=\"../index.html.*javascript.*/a>@@' |
sed -e 's@<a href="[^#][^>]*/cgi-bin/[^>]*>@YYYY@g' |
sed -e 's@YYYY<b>@YYYY@' |
sed -e 's@YYYY\(sw[a-z]*\)</b>@YYYY\1@' |
sed \
	-e 's@YYYY\([^(]*\)(\([1-9]\))@\<a href=\1_\2.html\>\1(\2)@g' \
	-e 's@YYYY\([^(]*\)@\<a href=\1.html\>\1@g' \
	|
cat
