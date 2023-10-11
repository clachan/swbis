#!/bin/bash

distdir="$1"

if [ ! -d "$distdir" ]; then
	exit 1
fi

cat catalog/dfiles/files |
sed -e 's@^[^/]*/@@' |
grep -v -e '^catalog/' -e '^$' |
cat |
tar cf - --no-recursion --files-from=- |
(cd $distdir && tar xpf -)


