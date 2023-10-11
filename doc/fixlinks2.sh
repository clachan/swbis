#!/bin/bash
cat |
# OLD sed -e 's@&s=\(.\)\"@_\1.html@'  |
# OLD sed -e 's@"http://.*m=@@' |
sed -e 's@a href="http://localhost.localdomain/cgi-bin/yman2html?m=@a href="@g' |
sed -e 's@&s=\(.\)@_\1.html@g' |
cat
exit 0
