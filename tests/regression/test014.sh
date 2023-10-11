#!/bin/sh
rm -fr foodir

newcsum="`find . | cpio -o -H newc 2>/dev/null | cksum`"
newcsum1="`find . | cpio -o -H  newc 2>/dev/null | ../devel/pa2pa /dev/null - | cksum`"
if [ "$newcsum" != "$newcsum1" ]; then
        echo "$newcsum $newcsum1" 1>&2
        echo "s1: newc sums failed" 1>&2
        exit 1
else
        echo "s1: newc sums                                               PASSED"
fi


newcsum="`tar cf - . | cksum`"
newcsum1="`tar cf - . | ../devel/pa2pa /dev/null - | cksum`"

if [ "$newcsum" != "$newcsum1" ]; then
        echo "tar $newcsum" 1>&2
        echo "tar pa2pa $newcsum1" 1>&2
        echo "s2: ustar sums failed" 1>&2
        exit 2
else
        echo "tar $newcsum" 1>&2
        echo "tar pa2pa $newcsum1" 1>&2
        echo "s2: ustar sums                                              PASSED"
fi
exit 0
